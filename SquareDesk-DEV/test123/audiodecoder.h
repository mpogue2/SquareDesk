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

#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#if defined(Q_OS_LINUX)
#include <QtMultimedia/QAudioDecoder>
#include <QtMultimedia/QMediaPlayer>
#else /* end of Q_OS_LINUX */
#include <QAudioDecoder>
#include <QMediaPlayer>
#endif /* else if defined Q_OS_LINUX */


#include <QTextStream>
#if defined(Q_OS_LINUX)
#include <QtMultimedia/QWaveDecoder>
#else /* end of Q_OS_LINUX */
#include <QWaveDecoder>
#endif /* else if defined Q_OS_LINUX */
#include <QBuffer>
#include <QByteArray>
#include <QTimer>
#if defined(Q_OS_LINUX)
#include <QtMultimedia/QAudioDevice>
#include <QtMultimedia/QAudioSink>
#else /* end of Q_OS_LINUX */
#include <QAudioDevice>
#include <QAudioSink>
#endif /* else if defined Q_OS_LINUX */

#include <QProcess>
#include "perftimer.h"

// BASS_ChannelIsActive return values
#define BASS_ACTIVE_STOPPED 0
#define BASS_ACTIVE_PLAYING 1
#define BASS_ACTIVE_STALLED 2
#define BASS_ACTIVE_PAUSED  3

#include <vector>

class AudioDecoder : public QObject
{
    Q_OBJECT

public:
    AudioDecoder();
    ~AudioDecoder();

    void newSystemAudioOutputDevice();

    void setSource(const QString &fileName);
    void start();
    void stop();
    QAudioDecoder::Error getError();

    // MUSIC PLAYER ------
    void Play();
    void Pause();
    void Stop();
    bool isPlaying();

    void fadeOutAndPause(float finalVol, float secondsToGetThere);
    void StartVolumeDucking(int duckToPercent, double forSeconds);
    void StopVolumeDucking();

    void setLoop(double from, double to);
    void clearLoop();

    void setVolume(unsigned int v);
    void setPan(double p);
    void setMono(bool on);

    void setBassBoost(float b);
    void setMidBoost(float m);
    void setTrebleBoost(float t);

    void SetIntelBoost(unsigned int which, float val);
    void SetIntelBoostEnabled(bool enable);

    void SetPanEQVolumeCompensation(float val);

    void setPitch(float p);
    void setTempo(float t);

    void   setStreamPosition(double p);
    double getStreamPosition();
    double getStreamLength();

    double getPeakLevelL_mono();    // for VU meter
    double getPeakLevelR();         // for VU meter

    double getBPM();

    float BPMsample(float sampleStart_sec, float sampleLength_sec, float BPMbase, float BPMtolerance);
    int beatBarDetection(); // returns -1 if no Vamp

#define GRANULARITY_NONE 0
#define GRANULARITY_BEAT 1
#define GRANULARITY_MEASURE 2

    double snapToClosest(double time_sec, unsigned char granularity);

    unsigned char getCurrentState();

    unsigned int playPosition_frames;

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

    QString m_currentAudioOutputDeviceName;

    std::vector<double> beatMap;    // contains all beats
    std::vector<double> measureMap; // contains all the beat 1's

    QProcess vamp;
    QString WAVfilename;        // these are in the temp directory, and when done, need to be deleted
    QString resultsFilename;

    PerfTimer *t;
};

#endif // AUDIODECODER_H
