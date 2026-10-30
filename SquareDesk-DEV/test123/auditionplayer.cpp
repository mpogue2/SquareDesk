/****************************************************************************
**
** Copyright (C) 2016-2026 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk application.
**
** $SQUAREDESK_BEGIN_LICENSE$
**
** Commercial License Usage
** For commercial licensing terms and conditions, contact the authors via the
** email address above.
**
** GNU General Public License Usage
** This file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appear in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.
**
** $SQUAREDESK_END_LICENSE$
**
****************************************************************************/

#include "auditionplayer.h"
#include "audiodecoder.h"
#include "SoundTouch.h"

#include <QAudioFormat>
#include <QDebug>
#include <QUrl>

#include <algorithm>
#include <cmath>

#define AUDITION_SAMPLE_RATE      44100
#define AUDITION_BYTES_PER_FRAME  8         // stereo float
#define AUDITION_CHUNK_FRAMES     4096      // frames per trip through SoundTouch
#define AUDITION_PUMP_MS          10        // how often the pump tops up the sink
#define AUDITION_SINK_BUFFER_MS   200       // sink buffer: rides out a busy main thread

// What has to be decoded before playback starts: a little past the start point, and the same
//   60-90 sec window that AudioDecoder::finished() uses for BPM detection.
#define AUDITION_LEAD_SEC         2
#define AUDITION_BPM_START_SEC    60
#define AUDITION_BPM_LENGTH_SEC   30

using namespace soundtouch;

static float s_inBuffer[2 * AUDITION_CHUNK_FRAMES];
static float s_outBuffer[2 * AUDITION_CHUNK_FRAMES];

AuditionPlayer::AuditionPlayer(QObject *parent)
    : QObject(parent)
    , m_soundTouch(std::make_unique<SoundTouch>())
{
    QAudioFormat format;
    format.setSampleRate(AUDITION_SAMPLE_RATE);
    format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    format.setSampleFormat(QAudioFormat::Float);
    m_decoder.setAudioFormat(format);

    connect(&m_decoder, &QAudioDecoder::bufferReady, this, &AuditionPlayer::decoderBufferReady);
    connect(&m_decoder, &QAudioDecoder::finished,    this, &AuditionPlayer::decoderFinished);
    connect(&m_decoder, QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error),
            this, &AuditionPlayer::decoderError);

    m_pumpTimer.setInterval(AUDITION_PUMP_MS);
    connect(&m_pumpTimer, &QTimer::timeout, this, &AuditionPlayer::pump);

    // same SoundTouch setup as PlayerThread, so the audition sounds like the real thing
    m_soundTouch->setSampleRate(AUDITION_SAMPLE_RATE);
    m_soundTouch->setChannels(2);
    m_soundTouch->setRateChange(0.0);
    m_soundTouch->setSetting(SETTING_USE_QUICKSEEK, false);  // NO QUICKSEEK (better quality)
    m_soundTouch->setSetting(SETTING_USE_AA_FILTER, true);   // USE AA FILTER (better quality)
}

AuditionPlayer::~AuditionPlayer()
{
    stop();
}

void AuditionPlayer::start(const QString &filePath, const QAudioDevice &device, qint64 startMs,
                           int pitchSemitones, double volumeGain,
                           bool needsDetectedBPM, TempoForDetectedBPM tempoForDetectedBPM)
{
    stop();  // only one audition at a time

    m_device              = device;
    m_startFrame          = static_cast<unsigned int>(std::max<qint64>(0, startMs) * AUDITION_SAMPLE_RATE / 1000);
    m_playFrame           = m_startFrame;
    m_pitchSemitones      = pitchSemitones;
    m_volumeGain          = volumeGain;
    m_needsDetectedBPM    = needsDetectedBPM;
    m_tempoForDetectedBPM = tempoForDetectedBPM;
    m_decodeDone          = false;
    m_flushed             = false;
    m_playing             = false;
    m_active              = true;

    m_decoder.setSource(QUrl::fromLocalFile(filePath));
    m_decoder.start();
}

void AuditionPlayer::stop()
{
    m_active  = false;
    m_playing = false;
    m_pumpTimer.stop();

    m_decoder.stop();

    if (m_audioSink != nullptr) {
        m_audioDevice = nullptr;
        // reset(), NOT stop(): reset() discards the buffered audio (we want it to stop NOW), and
        //   destroys Qt's sink stream right here, so no readyRead already queued to it can be
        //   delivered after the sink is gone (see #1807 in AudioDecoder::newSystemAudioOutputDevice()).
        m_audioSink->reset();
        delete m_audioSink;
        m_audioSink = nullptr;
    }

    m_soundTouch->clear();

    m_data.clear();
    m_data.squeeze();  // a whole song is ~100MB of float, so give it back
}

unsigned int AuditionPlayer::decodedFrames() const
{
    return(static_cast<unsigned int>(m_data.size() / AUDITION_BYTES_PER_FRAME));
}

void AuditionPlayer::decoderBufferReady()
{
    QAudioBuffer buffer = m_decoder.read();
    if (!m_active || !buffer.isValid()) {
        return;
    }

    m_data.append(buffer.constData<char>(), buffer.byteCount());
    startPlaybackIfReady();
}

void AuditionPlayer::decoderFinished()
{
    if (!m_active || m_data.isEmpty()) {
        // AudioDecoder::finished() has seen finished() arrive once before any data, and then
        //   again for real (Qt 6.5+), so an empty "finished" is not believed here either.
        return;
    }

    m_decodeDone = true;
    startPlaybackIfReady();
}

void AuditionPlayer::decoderError(QAudioDecoder::Error error)
{
    if (!m_active || error == QAudioDecoder::NoError) {
        return;
    }
    qDebug() << "AuditionPlayer: can't decode audition song:" << m_decoder.errorString();
    stop();
}

void AuditionPlayer::startPlaybackIfReady()
{
    if (m_playing) {
        pump();  // more decoded audio may be what the pump was waiting for
        return;
    }

    unsigned int framesNeeded = m_startFrame + AUDITION_LEAD_SEC * AUDITION_SAMPLE_RATE;
    if (m_needsDetectedBPM) {
        framesNeeded = std::max<unsigned int>(framesNeeded,
                                              (AUDITION_BPM_START_SEC + AUDITION_BPM_LENGTH_SEC) * AUDITION_SAMPLE_RATE);
    }
    if (!m_decodeDone && decodedFrames() < framesNeeded) {
        return;  // not yet
    }

    // Exactly the BPM detection a real load does (AudioDecoder::finished()).  When the song is
    //   longer than the 90 sec window, the decoded part gives the same answer as the whole song;
    //   when it is shorter, the decode is done and we have the whole song.
    int detectedBPM = 0;
    if (m_needsDetectedBPM) {
        detectedBPM = static_cast<int>(round(AudioDecoder::estimateBPM((const float *)m_data.constData(), decodedFrames(),
                                                                       AUDITION_BPM_START_SEC, AUDITION_BPM_LENGTH_SEC, 125, 15)));
    }
    int tempoPercent = (m_tempoForDetectedBPM ? m_tempoForDetectedBPM(detectedBPM) : 100);

    m_soundTouch->clear();
    m_soundTouch->setTempo(tempoPercent/100.0);
    m_soundTouch->setPitchSemiTones(m_pitchSemitones);

    QAudioFormat format;
    format.setSampleRate(AUDITION_SAMPLE_RATE);
    format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    format.setSampleFormat(QAudioFormat::Float);

    m_audioSink = new QAudioSink(m_device, format);
    m_audioSink->setBufferSize(AUDITION_SAMPLE_RATE * AUDITION_BYTES_PER_FRAME * AUDITION_SINK_BUFFER_MS / 1000);
    m_audioDevice = m_audioSink->start();  // push mode: pump() write()s to this
    if (m_audioDevice == nullptr) {
        qDebug() << "AuditionPlayer: can't start the audition audio device";
        stop();
        return;
    }

    m_playing = true;
    pump();
    m_pumpTimer.start();
}

void AuditionPlayer::pump()
{
    if (!m_playing || m_audioDevice == nullptr) {
        return;
    }

    unsigned int framesFree = static_cast<unsigned int>(m_audioSink->bytesFree() / AUDITION_BYTES_PER_FRAME);

    while (framesFree > 0) {
        if (m_soundTouch->numSamples() == 0) {
            // SoundTouch has nothing ready, so feed it the next chunk of the song
            unsigned int decoded = decodedFrames();
            unsigned int available = (m_playFrame < decoded ? decoded - m_playFrame : 0);
            if (available == 0) {
                if (m_decodeDone && !m_flushed) {
                    m_soundTouch->flush();  // end of song: get the last of it out of SoundTouch
                    m_flushed = true;
                    continue;
                }
                break;  // waiting for the decoder (or the song is over)
            }

            unsigned int n = std::min<unsigned int>(available, AUDITION_CHUNK_FRAMES);
            const float *in = (const float *)(m_data.constData()) + 2 * m_playFrame;
            for (unsigned int i = 0; i < 2 * n; i++) {
                s_inBuffer[i] = std::max(-1.0f, std::min(1.0f, (float)(m_volumeGain * in[i])));  // volume + hard limiter, as in PlayerThread
            }
            m_soundTouch->putSamples(s_inBuffer, n);
            m_playFrame += n;
            continue;
        }

        unsigned int n = std::min<unsigned int>(framesFree, AUDITION_CHUNK_FRAMES);
        unsigned int got = m_soundTouch->receiveSamples(s_outBuffer, n);
        if (got == 0) {
            break;
        }
        m_audioDevice->write((const char *)s_outBuffer, got * AUDITION_BYTES_PER_FRAME);
        framesFree -= got;
    }
}
