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

#ifndef AUDITIONPLAYER_H
#define AUDITIONPLAYER_H

#include <QObject>
#include <QAudioDecoder>
#include <QAudioDevice>
#include <QAudioSink>
#include <QByteArray>
#include <QTimer>

#include <functional>
#include <memory>

namespace soundtouch { class SoundTouch; }  // so that mainwindow.h does not pull in all of SoundTouch.h

// Plays a song for Audition (press-and-hold), at the pitch, tempo and volume that a real load
//   of that song would use (issue #1817).
//
// This is a deliberately small cousin of AudioDecoder/PlayerThread: decode the song into memory
//   (QAudioDecoder, 44.1kHz stereo float), run it through SoundTouch, and push it into a
//   QAudioSink on the audition device.  There is no beat/bar or segment detection, no waveform,
//   no EQ and no LoudMax, and no thread of its own: the pump runs on a timer on the main thread,
//   so there is nothing extra to stop at quit time (issue #1266).
class AuditionPlayer : public QObject
{
    Q_OBJECT

public:
    explicit AuditionPlayer(QObject *parent = nullptr);
    ~AuditionPlayer() override;

    // Called once, as soon as enough of the song is decoded, with MiniBPM's BPM for it (rounded,
    //   or 0 when needsDetectedBPM is false).  Returns the SoundTouch tempo in percent (100 = as recorded).
    using TempoForDetectedBPM = std::function<int(int detectedBPM)>;

    // volumeGain: 1.0 = full volume.  needsDetectedBPM: false skips BPM detection (e.g. the song has an ID3 TBPM tag).
    void start(const QString &filePath, const QAudioDevice &device, qint64 startMs,
               int pitchSemitones, double volumeGain,
               bool needsDetectedBPM, TempoForDetectedBPM tempoForDetectedBPM);
    void stop();  // safe to call any time, and more than once

private slots:
    void decoderBufferReady();
    void decoderFinished();
    void decoderError(QAudioDecoder::Error error);
    void pump();  // move processed audio into the audio sink

private:
    void startPlaybackIfReady();
    unsigned int decodedFrames() const;

    QAudioDecoder m_decoder;
    QByteArray    m_data;               // decoded song so far, interleaved stereo float

    QAudioDevice  m_device;
    QAudioSink   *m_audioSink = nullptr;
    QIODevice    *m_audioDevice = nullptr;  // owned by m_audioSink
    QTimer        m_pumpTimer;

    std::unique_ptr<soundtouch::SoundTouch> m_soundTouch;

    bool          m_active = false;      // between start() and stop()
    bool          m_playing = false;     // the sink has been started
    bool          m_decodeDone = false;
    bool          m_flushed = false;     // SoundTouch has been flushed at the end of the song

    unsigned int  m_startFrame = 0;
    unsigned int  m_playFrame = 0;       // next source frame to feed to SoundTouch
    int           m_pitchSemitones = 0;
    double        m_volumeGain = 1.0;
    bool          m_needsDetectedBPM = true;
    TempoForDetectedBPM m_tempoForDetectedBPM;
};

#endif // AUDITIONPLAYER_H
