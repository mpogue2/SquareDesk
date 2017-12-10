/****************************************************************************
**
** Copyright (C) 2016, 2017 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk/SquareDeskPlayer application.
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
#include <QTimer>

class bass_audio
{
    //-------------------------------------------------------------
public:
    //---------------------------------------------------------
    double                  FileLength;
    double                  Current_Position;
    int                     Stream_Volume;
    int                     Stream_Tempo;
    float                   Stream_Eq[3];
    int                     Stream_Pitch;
//    float                   Stream_Pan;
//    bool                    Stream_Mono;
    float                   Stream_BPM;
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
    void SetTempo(int newTempo);  // 100 = normal, 95 = 5% slower than normal
    void SetEq(int band, float val);  // band = 0,1,2; val = -15.0 .. 15.0 (float) nominal 0.0
    void SetPitch(int newPitch);  // in semitones, -5 .. 5
    void SetPan(float newPan);  // -1.0 .. 0.0 .. 1.0

    void SetLoop(double fromPoint_sec, double toPoint_sec);  // if fromPoint < 0, then disabled
    void ClearLoop();

    void SetMono(bool on);

    //Stream
    void songStartDetector(const char *filepath, float *pSongStart, float *pSongEnd);
    void StreamCreate(const char *filepath, float *pSongStart, float *pSongEnd);  // returns start of non-silence (seconds)

    void StreamGetLength(void);
    void StreamSetPosition(double Position);
    void StreamGetPosition(void);

    //Controls
    void Play(void);
    void Stop(void);
    void FadeOutAndPause(void);

    int StreamGetVuMeter(void); // get VU meter level (mono)

    // FX
    void PlayOrStopSoundEffect(int which, const char *filename, int volume = 100);
    void StopAllSoundEffects();

    void StartVolumeDucking(int duckToPercent, float forSeconds);
    void StopVolumeDucking();

    int  currentSoundEffectID;

    DWORD                           Stream_State;  // intentionally public // FIX: add getStreamState()
    //-------------------------------------------------------------
private:
    //-------------------------------------------------------------
    //-------------------------------------------------------------
    HSTREAM                         Stream;
    HSTREAM                         FXStream;

    HFX fxEQ;     // dsp peaking eq handle

    HSYNC  syncHandle;
};
