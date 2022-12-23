/****************************************************************************
**
** Copyright (C) 2016-2022 Mike Pogue, Dan Lyke
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

#pragma once
//#include "flexible_audio.h"

#include <QTimer>
#if defined(Q_OS_LINUX)
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/QAudioBuffer>
#include <QtMultimedia/QAudioDevice>
#include <QtMultimedia/QAudioDecoder>
#include <QtMultimedia/QAudioFormat>
#include <QtMultimedia/QMediaDevices>
#include <QtMultimedia/QMediaPlayer>
#else /* end of Q_OS_LINUX */
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QAudioBuffer>
#include <QAudioDevice>
#include <QAudioDecoder>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QMediaPlayer>
#endif /* else if defined Q_OS_LINUX */
#include <QFile>
#include <QBuffer>
#include "perftimer.h"

#include "audiodecoder.h"

// indices into Global_IntelBoostEq[] for Global Intelligibility Boost EQ
#define FREQ_KHZ 0
#define BW_OCT  1
#define GAIN_DB 2

// BASS_ChannelIsActive return values
#define BASS_ACTIVE_STOPPED 0
#define BASS_ACTIVE_PLAYING 1
#define BASS_ACTIVE_STALLED 2
#define BASS_ACTIVE_PAUSED  3

class flexible_audio : public QIODevice
{
    Q_OBJECT

private:
    AudioDecoder decoder; // TEST of AudioDecoder from example, must call setSource!

    //-------------------------------------------------------------
public:
    //---------------------------------------------------------
    double                  FileLength;
    double                  Current_Position;
    int                     Stream_Volume;
    double                  Stream_MaxVolume;       // used for ReplayGain, which sets this to something other then 1.0
//    double                  Stream_replayGain_dB;   // used for ReplayGain, which sets this to something other then 0.0
    int                     Stream_Tempo;
    double                  Stream_Eq[3];

    bool                    IntelBoostShouldBeEnabled;
    float                   Global_IntelBoostEq[3]; // one band for Intelligibility Boost (FREQ_KHZ, BW_OCT, GAIN_DB)

    int                     Stream_Pitch;
//    double                    Stream_Pan;
//    bool                    Stream_Mono;
    double                  Stream_BPM;

    bool                    bPaused;

    // LOOP stuff...
    double                  loopFromPoint_sec;
    double                  loopToPoint_sec;

    unsigned long           startPoint_bytes;
    unsigned long           endPoint_bytes;     // jump to point.  If 0, don't jump at all.

    QMediaPlayer *player;

//---------------------------------------------------------
    //constructors
    flexible_audio(void);
    virtual ~flexible_audio(void);
    //---------------------------------------------------------
    //System
    void Init(void);
    void Exit(void);

    //Settings
    void SetVolume(int inVolume);
//    void SetReplayGainVolume(double replayGain_dB);
    void SetTempo(int newTempo);  // 100 = normal, 95 = 5% slower than normal
    void SetEq(int band, double val);  // band = 0,1,2; val = -15.0 .. 15.0 (double ) nominal 0.0

    void SetIntelBoost(unsigned int which, float val);    // Global intelligibility boost parameters
    void SetIntelBoostEnabled(bool enable);               // Global intelligibility boost parameters

    void SetPitch(int newPitch);  // in semitones, -5 .. 5
    void SetPan(double  newPan);  // -1.0 .. 0.0 .. 1.0

    void SetGlobals(void);  // sets Global EQ, after song is loaded

    void SetLoop(double fromPoint_sec, double toPoint_sec);  // if fromPoint < 0, then disabled
    void ClearLoop();

    void SetMono(bool on);

    //Stream
    void songStartDetector(const char *filepath, double  *pSongStart, double  *pSongEnd);
    void StreamCreate(const char *filepath, double  *pSongStart, double  *pSongEnd, double i1, double o1);  // returns start of non-silence (seconds)

    void StreamGetLength(void);
    void StreamSetPosition(double Position);
    void StreamGetPosition(void);

    bool isPaused(void); // returns true if paused, false if playing

    //Controls
    void Play(void);  // forces stream to play
    void Stop(void);  // forces stream to stop playback, and rewinds to 0
    void Pause(void); // forces stream to stop playback
    void FadeOutAndPause(void);  // 6 second fade, then pause

    int StreamGetVuMeter(void); // get VU meter level (mono)

    // FX
    void PlayOrStopSoundEffect(int which, const char *filename, int volume = 100);
    void StopAllSoundEffects();

    void StartVolumeDucking(int duckToPercent, double  forSeconds);
    void StopVolumeDucking();

    void FXChannelStartPlaying(const char *filename);
    void FXChannelStopPlaying();
    bool FXChannelIsPlaying();

    // always asks the engine what the state is (NOT CACHED), then returns one of:
    //    BASS_ACTIVE_STOPPED, BASS_ACTIVE_PLAYING, BASS_ACTIVE_STALLED, BASS_ACTIVE_PAUSED
    uint32_t currentStreamState();

    int  currentSoundEffectID;

    QString currentAudioDevice;  // which audio device are we talking to?  intentionally public

private:
    QAudioOutput  *audioOutput;

    QFile m_file;
    QBuffer m_input;
    QByteArray m_data;
    QAudioDecoder m_decoder;

    int totalBytes;

    QMediaPlayer soundEffect;  // dedicated player for sound effects

protected:
    qint64 readData(char* data, qint64 maxlen) override;
    qint64 writeData(const char* data, qint64 len) override;

signals:
    void haveDuration();

private slots:
    void bufferReady();
    void finished();  // decode finished
    void decoder_error();
    void posChanged(qint64);
    void durChanged(qint64);
    void FXChannelStatusChanged(QMediaPlayer::MediaStatus);
    void systemAudioOutputsChanged();

public slots:
    void decoderDone();
};

