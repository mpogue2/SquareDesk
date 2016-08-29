#pragma once
#include "bass.h"

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
    float                   Stream_Pan;
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
    void StreamCreate(const char *filepath);

    void StreamGetLength(void);
    void StreamSetPosition(double Position);
    void StreamGetPosition(void);

    //Controls
    void Play(void);
    void Stop(void);

    DWORD                           Stream_State;  // intentionally public // FIX: add getStreamState()
    //-------------------------------------------------------------
private:
    //-------------------------------------------------------------
    //-------------------------------------------------------------
    HSTREAM                         Stream;
    HFX fxEQ;     // dsp peaking eq handle
    //-------------------------------------------------------------
};
