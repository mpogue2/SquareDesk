/****************************************************************************
**
** Copyright (C) 2016-2023 Mike Pogue, Dan Lyke
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

#include "audiodecoder.h"
#include "audiothread.h"
#include <QFile>
#include <stdio.h>
#include <iostream>
#if defined(Q_OS_LINUX)
#include <QtMultimedia/QMediaDevices>
#include <QtMultimedia/QAudioDevice>
#else /* end of Q_OS_LINUX */
#include <QMediaDevices>
#include <QAudioDevice>
#endif /* else if defined Q_OS_LINUX */
#include <QElapsedTimer>
#include <QThread>


// BPM Estimation ==========

// SoundTouch BPM estimation ----------
// #include "BPMDetect.h"

// BreakfastQuay BPM detection --------
#include "MiniBpm.h"
using namespace breakfastquay;

QElapsedTimer timer1;

// TODO: VU METER (kfr) ********
// TODO: SOUND EFFECTS (later) ********
// TODO: allow changing the output device (new feature!) *****

// ===========================================================================
AudioThread myPlayer;  // singleton

// ===========================================================================
AudioDecoder::AudioDecoder()
{
//    qDebug() << "In AudioDecoder() constructor";
    m_sampleRate = 48000;
    connect(&m_decoder, &QAudioDecoder::bufferReady,
            this, &AudioDecoder::bufferReady);
    connect(&m_decoder, QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error),
            this, QOverload<QAudioDecoder::Error>::of(&AudioDecoder::error));
    connect(&m_decoder, &QAudioDecoder::isDecodingChanged,
            this, &AudioDecoder::isDecodingChanged);
    connect(&m_decoder, &QAudioDecoder::finished,
            this, &AudioDecoder::finished);
    connect(&m_decoder, &QAudioDecoder::positionChanged,
            this, &AudioDecoder::updateProgress);
    connect(&m_decoder, &QAudioDecoder::durationChanged,
            this, &AudioDecoder::updateProgress);

    m_progress = -1.0;
    
    m_data = new QByteArray();      // raw data, starts out at zero size, will be expanded when m_input is append()'ed
    m_input = new QBuffer(m_data);  // QIODevice interface

//    QString t = QString("0x%1").arg((quintptr)(m_data->data()), QT_POINTER_SIZE * 2, 16, QChar('0'));
//    qDebug() << "audio data is at:" << t;

    myPlayer.setBytesPerFrameAndSampleRate(8,m_sampleRate);
    myPlayer.assignDataAndTotalFrames((unsigned char *)(m_data->data()),0);

    m_audioSink = 0;                        // nothing yet
    m_currentAudioOutputDeviceName = "";    // nothing yet

    newSystemAudioOutputDevice();  // this will make m_audiosink and m_currentAudioOutputDeviceName valid

    myPlayer.start();  // starts the playback thread (in STOPPED mode)
}

AudioDecoder::~AudioDecoder()
{
    if (myPlayer.isRunning()) {
        myPlayer.Stop();
    }
    myPlayer.quit();
}

void AudioDecoder::newSystemAudioOutputDevice() {
    // we want a format that will be no resampling for 99% of the MP3 files, but is float for kfr/rubberband processing
    QAudioDevice defaultAudioOutput = QMediaDevices::defaultAudioOutput();
    QString defaultADName = defaultAudioOutput.description();
    QAudioFormat preferredFormat = defaultAudioOutput.preferredFormat();

    QAudioFormat desiredAudioFormat; // = device.preferredFormat();  // 48000, 2ch, float = WHY?  Why not 16-bit int?  Less memory used.
    m_sampleRate = preferredFormat.sampleRate();
    desiredAudioFormat.setSampleRate(m_sampleRate);
    desiredAudioFormat.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    desiredAudioFormat.setSampleFormat(QAudioFormat::Float);  // Note: 8 bytes per frame

    myPlayer.setBytesPerFrameAndSampleRate(8,m_sampleRate);

//    qDebug() << "desiredAudioFormat: " << desiredAudioFormat << " )";
//    qDebug() << "newSystemAudioOutputDevice -- LAST AUDIO OUTPUT DEVICE NAME" << m_currentAudioOutputDeviceName << ", CURRENT AUDIO OUTPUT DEVICE NAME: " << defaultADName;

    if (defaultADName != m_currentAudioOutputDeviceName) {
        if (m_audioSink != 0) {
            // if we already have an AudioSink, make a new one

            m_audioSink->stop();
            QAudioSink *oldOne = m_audioSink;
//            qDebug() << "     making a new one atomically...";
            m_audioSink = new QAudioSink(defaultAudioOutput, desiredAudioFormat);  // uses the current default audio device, ATOMIC
            delete oldOne;
        } else {
//            qDebug() << "     making a new one for the first time...";
            m_audioSink = new QAudioSink(defaultAudioOutput, desiredAudioFormat);  // uses the current default audio device, ATOMIC
        }
        m_currentAudioOutputDeviceName = defaultADName;    // assuming that succeeded, this is the one we are sending audio to
    }

    m_audioDevice = m_audioSink->start();  // do write() to this to play music

//    qDebug() << "BUFFER SIZE: " << m_audioBufferSize;

    m_decoder.setAudioFormat(desiredAudioFormat);

    // give the data pointers to myPlayer
    myPlayer.assignAudioDeviceAudioSinkAndZeroBufferSize(m_audioDevice, m_audioSink);
}


void AudioDecoder::setSource(const QString &fileName)
{
//    qDebug() << "AudioDecoder:: setSource" << fileName;
    if (m_decoder.isDecoding()) {
//        qDebug() << "\thad to stop decoding...";
        m_decoder.stop();
    }
    if (m_input->isOpen()) {
//        qDebug() << "\thad to close m_input...";
        m_input->close();
    }
    if (!m_data->isEmpty()) {
//        qDebug() << "\thad to throw away m_data...";
        delete(m_data);  // throw the whole thing away
        m_data = new QByteArray();
        m_input->setBuffer(m_data); // and make a new one (empty)
    }
//    qDebug() << "m_input now has " << m_input->size() << " bytes (should be zero).";
    m_decoder.setSource(QUrl::fromLocalFile(fileName));
}

void AudioDecoder::start()
{
//    qDebug() << "AudioDecoder::start -- starting to decode...";
    if (!m_input->isOpen()) {
//        qDebug() << "\thad to open m_input...";
        m_input->open(QIODeviceBase::ReadWrite);
    }
    m_progress = -1;  // reset the progress bar to the beginning, because we're about to start.
    timer1.start();
    BPM = -1.0;  // -1 means "no BPM yet"
    m_decoder.start(); // starts the decode process
}

void AudioDecoder::stop()
{
//    qDebug() << "stopping...";
    m_decoder.stop();
//    m_input->close();
}

QAudioDecoder::Error AudioDecoder::getError()
{
//    qDebug() << "getError...";
    return m_decoder.error();
}

void AudioDecoder::bufferReady()
{
//    qDebug() << "audioDecoder::bufferReady";
    // read a buffer from audio decoder
    QAudioBuffer buffer = m_decoder.read();
    if (!buffer.isValid()) {
        return;
    }
    
    m_input->write(buffer.constData<char>(), buffer.byteCount());  // append bytes to m_input

//    // Force Mono is ON, so let's mixdown right here, and write a single array of float to the m_input
//    unsigned int numFrames = buffer.byteCount()/(2*myPlayer.bytesPerFrame); // this is the PRE-mixdown bytesPerFrame, which is 2X
//    float * outArray = new float[numFrames];
//    const float *inArray = (const float *)(buffer.constData<char>());
//    for (unsigned int i = 0; i < numFrames; i++) {
//        outArray[i] = (0.5*inArray[2*i]) + (0.5*inArray[2*i + 1]); // mixdown
//    }

//    m_input->write((const char *)outArray, buffer.byteCount()/2);  // append to m_input (half the number of bytes because 1 channel now)

//    delete [] outArray;
}

void AudioDecoder::error(QAudioDecoder::Error error)
{
//    qDebug() << "audiodecoder:: error";
    switch (error) {
    case QAudioDecoder::NoError:
        return;
    case QAudioDecoder::ResourceError:
        qDebug() << "Resource error";
        break;
    case QAudioDecoder::FormatError:
        qDebug() << "Format error";
        break;
    case QAudioDecoder::AccessDeniedError:
        qDebug() << "Access denied error";
        break;
    case QAudioDecoder::NotSupportedError:
        qDebug() << "Service missing error";
        break;
    }

//    emit done();  // I'm not sure that this is right, so commenting out.
}

void AudioDecoder::isDecodingChanged(bool isDecoding)
{
    if (isDecoding) {
//        qDebug() << "Decoding...";
    } else {
//        qDebug() << "Decoding stopped...";
    }
}

void AudioDecoder::finished()
{
//    qDebug() << "AudioDecoder::finished()";
//    qDebug() << "Decoding progress:  100%; m_input:" << m_input->size() << " bytes, m_data:" << m_data->size() << " bytes";
//    qDebug() << timer1.elapsed() << "milliseconds to decode";  // currently about 250ms to fully read in, decode, and save to the buffer.

    unsigned int bytesPerFrame = myPlayer.getBytesPerFrame();
    unsigned int totalFramesInSong = bytesPerFrame ? (m_data->size()/bytesPerFrame) : 0;
    unsigned char *p_data = (unsigned char *)(m_data->data());
    myPlayer.assignDataAndTotalFrames(p_data, totalFramesInSong);
//    qDebug() << "** totalFramesInSong: " << myPlayer.totalFramesInSong;  // TODO: this is really frames

    // BPM detection -------
    //   this estimate will be based on mono mixed-down samples from T={30,40} sec
    const float *songPointer = (const float *)p_data;

    float sampleStart_sec = 60.0;
    float sampleLength_sec = 10.0; // 30.0 - 40.0 sec
    float sampleEnd_sec = sampleStart_sec + sampleLength_sec;

    float songLength_sec = totalFramesInSong/ (double)(m_sampleRate);
//    float start_sec =  (songLength_sec >= sampleStart_sec + sampleLength_sec ? 10.0 : 0.0);
//    float end_sec   =  (songLength_sec >= start_sec + 10.0 ? start_sec + 10.0 : songLength_sec );

    // if song is longer than 40, end_sec will be 40; else end_sec will be the end of the song.
    float end_sec = (songLength_sec >= sampleEnd_sec ? sampleEnd_sec : songLength_sec);

    // if end_sec going backward by sampleLength is within the song, then use that point for the start_sec
    //   else, just use the start of the song
    float start_sec = (end_sec - sampleLength_sec > 0.0 ? end_sec - sampleLength_sec : 0.0 );

    unsigned int offsetIntoSong_samples = m_sampleRate * start_sec;            // start looking at time T = 10 sec
    unsigned int numSamplesToLookAt = m_sampleRate * (end_sec - start_sec);    //   look at 10 sec of samples

//    qDebug() << "***** BPM DEBUG: " << songLength_sec << start_sec << end_sec << offsetIntoSong_samples << numSamplesToLookAt;

    float monoBuffer[numSamplesToLookAt];
    for (unsigned int i = 0; i < numSamplesToLookAt; i++) {
        // mixdown to mono
        monoBuffer[i] = 0.5*songPointer[2*(i+offsetIntoSong_samples)] + 0.5*songPointer[2*(i+offsetIntoSong_samples)+1];
    }

    MiniBPM BPMestimator((double)(m_sampleRate));
    BPMestimator.setBPMRange(125.0-15.0, 125.0+15.0);  // limited range for square dance songs, else use percent
    BPM = BPMestimator.estimateTempoOfSamples(monoBuffer, numSamplesToLookAt); // 10 seconds of samples

    if (end_sec - start_sec < 10.0) {
        // if we don't have enough song left to really know what the BPM is, just say "I don't know"
        BPM = 0.0;
    }

//    qDebug() << "***** BPM RESULT: " << BPM;  // -1 = overwritten by here, 0 = undetectable, else double

    emit done();
}

void AudioDecoder::updateProgress()
{
    qint64 position = m_decoder.position();
    qint64 duration = m_decoder.duration();
    qreal progress = m_progress;
    if (position >= 0 && duration > 0)
        progress = position / (qreal)duration;

    if (progress >= m_progress + 0.1) {
//        qDebug() << "Decoding progress: " << (int)(progress * 100.0) << "%; m_input:" << m_input->size() << " bytes, m_data:" << m_data->size() << " bytes";
        m_progress = progress;
    }
}

// --------------------------------------------------------------------------------
void AudioDecoder::Play() {
//    qDebug() << "AudioDecoder::Play";

    QString defaultADName = QMediaDevices::defaultAudioOutput().description();
//    qDebug() << "AudioDecoder::Play -- LAST AUDIO OUTPUT DEVICE NAME" << m_currentAudioOutputDeviceName << ", CURRENT AUDIO OUTPUT DEVICE NAME: " << defaultADName;

    if (defaultADName != m_currentAudioOutputDeviceName) {
//        qDebug() << "AudioDecoder::Play -- making a new audio output device";
        newSystemAudioOutputDevice();
    }

    myPlayer.Play();
}

void AudioDecoder::Pause() {
//    qDebug() << "AudioDecoder::Pause";
    myPlayer.Pause();
}

void AudioDecoder::Stop() {  // FIX: why are there 2 of these, stop and Stop?
//    qDebug() << "AudioDecoder::Stop";
    myPlayer.Stop();
}

bool AudioDecoder::isPlaying() {
    return(getCurrentState() == BASS_ACTIVE_PLAYING);
}

void AudioDecoder::setVolume(unsigned int v) {
    myPlayer.setVolume(v);
}

void AudioDecoder::setPan(double p) {
    myPlayer.setPan(p);
}

void AudioDecoder::setMono(bool on) {
    myPlayer.setMono(on);
}

void AudioDecoder::setStreamPosition(double p) {
    myPlayer.setStreamPosition(p);
}

double AudioDecoder::getStreamPosition() {
    return(myPlayer.getStreamPosition());
}

double AudioDecoder::getStreamLength() {
    return(myPlayer.getTotalFramesInSong()/ (double)(m_sampleRate));
}

unsigned char AudioDecoder::getCurrentState() {
    unsigned int currentState = myPlayer.getCurrentState();
    return(currentState);
}

void AudioDecoder::setBassBoost(float b) {
    myPlayer.setBassBoost(b);
}

void AudioDecoder::setMidBoost(float m) {
    myPlayer.setMidBoost(m);
}

void AudioDecoder::setTrebleBoost(float t) {
    myPlayer.setTrebleBoost(t);
}

void AudioDecoder::SetIntelBoost(unsigned int which, float val) {
    myPlayer.SetIntelBoost(which, val);
}

void AudioDecoder::SetIntelBoostEnabled(bool enable) {
    myPlayer.SetIntelBoostEnabled(enable);
}

double AudioDecoder::getBPM() {
    return (BPM);  // -1 = no BPM yet, 0 = out of range or undetectable, else returns a BPM
}

// ------------------------------------------------------------------
void AudioDecoder::setTempo(float newTempoPercent)
{
//    qDebug() << "AudioDecoder::setTempo: " << newTempoPercent << "%";
    myPlayer.setTempo(newTempoPercent);
}

// ------------------------------------------------------------------
void AudioDecoder::setPitch(float newPitchSemitones)
{
//    qDebug() << "AudioDecoder::setPitch: " << newPitchSemitones << " semitones";
    myPlayer.setPitch(newPitchSemitones);
}

// ------------------------------------------------------------------
void AudioDecoder::setLoop(double fromPoint_sec, double toPoint_sec)
{
//    qDebug() << "AudioDecoder::SetLoop: (" << fromPoint_sec << "," << toPoint_sec << ")";
    myPlayer.setLoop(fromPoint_sec, toPoint_sec);
}

void AudioDecoder::clearLoop()
{
//    qDebug() << "AudioDecoder::ClearLoop";
    myPlayer.clearLoop();
}

double AudioDecoder::getPeakLevelL_mono() {
    return(myPlayer.getPeakLevelL_mono());
}

double AudioDecoder::getPeakLevelR() {
    return(myPlayer.getPeakLevelR());
}

void AudioDecoder::fadeOutAndPause(float finalVol, float secondsToGetThere) {
    myPlayer.fadeOutAndPause(finalVol, secondsToGetThere);
}

void AudioDecoder::StartVolumeDucking(int duckToPercent, double forSeconds) {
    myPlayer.StartVolumeDucking(duckToPercent, forSeconds);
}

void AudioDecoder::StopVolumeDucking() {
    myPlayer.StopVolumeDucking();
}
