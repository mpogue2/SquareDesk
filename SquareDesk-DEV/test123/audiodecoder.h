/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include <QAudioDecoder>
#include <QSoundEffect>
#include <QTextStream>
#include <QAudioDecoder>
#include <QWaveDecoder>
#include <QBuffer>
#include <QByteArray>
#include <QTimer>
#include <QAudioDevice>
#include <QAudioSink>

// BASS_ChannelIsActive return values
#define BASS_ACTIVE_STOPPED 0
#define BASS_ACTIVE_PLAYING 1
#define BASS_ACTIVE_STALLED 2
#define BASS_ACTIVE_PAUSED  3

class AudioDecoder : public QObject
{
    Q_OBJECT

public:
    AudioDecoder();
    ~AudioDecoder();

    void setSource(const QString &fileName);
    void start();
    void stop();
    QAudioDecoder::Error getError();

    // MUSIC PLAYER ------
    void Play();
    void Pause();
    void Stop();
    bool isPlaying();
    void setVolume(unsigned int v);
    void setPan(double p);
    void setMono(bool on);

    void setBassBoost(float b);
    void setMidBoost(float m);
    void setTrebleBoost(float t);

    void setPitch(float p);
    void setTempo(float t);

    void   setStreamPosition(double p);
    double getStreamPosition();
    double getStreamLength();

    double getBPM();

    unsigned char getCurrentState();

    unsigned int playPosition_samples;

    QTimer *playTimer;
    bool activelyPlaying;

signals:
    void done();

public slots:
    void bufferReady();
    void error(QAudioDecoder::Error error);
    void isDecodingChanged(bool isDecoding);
    void finished();

private slots:
    void updateProgress();

private:
    QAudioDecoder m_decoder;

    QAudioSink   *m_audioSink;
    QIODevice    *m_audioDevice;
    unsigned int  m_audioBufferSize;

    qreal m_progress;

    QBuffer *m_input;
    QByteArray *m_data;

    double BPM;
};

#endif // AUDIODECODER_H
