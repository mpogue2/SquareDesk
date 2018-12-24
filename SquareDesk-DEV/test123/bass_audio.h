/****************************************************************************
**
** Copyright (C) 2016, 2017, 2018 Mike Pogue, Dan Lyke
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
#include "bass.h"
#include "bass_fx.h"

#include <QTimer>

// define this, if you want the compressor ---------------
//#if defined(Q_OS_MAC)
//#define WANTCOMPRESSOR 1
//#endif

class bass_audio
{
    //-------------------------------------------------------------
public:
    //---------------------------------------------------------
    double                  FileLength;
    double                  Current_Position;
    int                     Stream_Volume;
    double                  Stream_MaxVolume;       // used for ReplayGain, which sets this to something other then 1.0
    double                  Stream_replayGain_dB;   // used for ReplayGain, which sets this to something other then 0.0
    int                     Stream_Tempo;
    double                  Stream_Eq[3];
    int                     Stream_Pitch;
//    double                    Stream_Pan;
//    bool                    Stream_Mono;
    double                  Stream_BPM;

#if defined(WANTCOMPRESSOR)
    bool                    compressorShouldBeEnabled;
    BASS_BFX_COMPRESSOR2    compressor;       // last compressor settings
#endif

    bool                    bPaused;

    // LOOP stuff...
    double                  loopFromPoint_sec;
    double                  loopToPoint_sec;

    QWORD                   startPoint_bytes;
    QWORD                   endPoint_bytes;     // jump to point.  If 0, don't jump at all.

//---------------------------------------------------------
    //constructors
    bass_audio(void);
    ~bass_audio(void);
    //---------------------------------------------------------
    //System
    void Init(void);
    void Exit(void);

    //Settings
    void SetVolume(int inVolume);
    void SetReplayGainVolume(double replayGain_dB);
    void SetTempo(int newTempo);  // 100 = normal, 95 = 5% slower than normal
    void SetEq(int band, double val);  // band = 0,1,2; val = -15.0 .. 15.0 (double ) nominal 0.0

    void SetCompression(unsigned int which, float val); // Global compressor parameters
    void SetCompressionEnabled(bool enable);             // Global compressor parameters

    void SetPitch(int newPitch);  // in semitones, -5 .. 5
    void SetPan(double  newPan);  // -1.0 .. 0.0 .. 1.0

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

    // always asks the engine what the state is (NOT CACHED), then returns one of:
    //    BASS_ACTIVE_STOPPED, BASS_ACTIVE_PLAYING, BASS_ACTIVE_STALLED, BASS_ACTIVE_PAUSED
    uint32_t currentStreamState();

    int  currentSoundEffectID;

//    DWORD                           Stream_State;  // intentionally public // FIX: add getStreamState()
    //-------------------------------------------------------------
private:
    //-------------------------------------------------------------
    //-------------------------------------------------------------
    HSTREAM                         Stream;
    HSTREAM                         FXStream;

    HFX fxEQ;           // dsp peaking eq handle
    HFX fxCompressor;   // global compressor

    HSYNC  syncHandle;
};

