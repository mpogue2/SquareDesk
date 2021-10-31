/****************************************************************************
**
** Copyright (C) 2016-2021 Mike Pogue, Dan Lyke
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

#include "flexible_audio.h"
#include <math.h>
#include <stdio.h>
#include <QDebug>
#include <QTimer>

// GLOBALS =========
float gStream_Pan = 0.0;
bool  gStream_Mono = false;
float gStream_replayGain = 1.0f;    // replayGain

// ------------------------------------------------------------------
flexible_audio::flexible_audio(void)
{
//    Stream = (HSTREAM)NULL;
//    FXStream = (HSTREAM)NULL;  // also initialize the FX stream

//    Stream_State = (HSTREAM)NULL;
    Stream_Volume = 100; ///10000 (multipled on output)
    Stream_Tempo = 100;  // current tempo, relative to 100
    Stream_Pitch = 0;    // current pitch correction, in semitones; -5 .. 5
    Stream_BPM = 0.0;    // current estimated BPM (original song, without tempo correction)
//    Stream_Pan = 0.0;
//    Stream_Mono = false; // true, if FORCE MONO mode

//    fxEQ = 0;            // equalizer handle
    Stream_Eq[0] = 50.0;  // current EQ (3 bands), 0 = Bass, 1 = Mid, 2 = Treble
    Stream_Eq[1] = 50.0;
    Stream_Eq[2] = 50.0;

    Global_IntelBoostEq[FREQ_KHZ] = 1.6;  // current Global EQ (one band for Intelligibility Boost)
    Global_IntelBoostEq[BW_OCT]  = 2.0;
    Global_IntelBoostEq[GAIN_DB] = 0.0;

    IntelBoostShouldBeEnabled = false;

    FileLength = 0.0;
    Current_Position = 0.0;
    bPaused = false;

    loopFromPoint_sec = -1.0;
    loopToPoint_sec = -1.0;
    startPoint_bytes = 0;
    endPoint_bytes = 0;

    currentSoundEffectID = 0;    // no soundFX playing now
}

// ------------------------------------------------------------------
flexible_audio::~flexible_audio(void)
{
}

// ------------------------------------------------------------------
void flexible_audio::Init(void)
{
    qDebug() << "Init()";
}

// ------------------------------------------------------------------
void flexible_audio::Exit(void)
{
    qDebug() << "Exit()";
}

// ------------------------------------------------------------------
void flexible_audio::SetVolume(int inVolume)
{
    qDebug() << "Setting new volume: " << inVolume;
    Stream_Volume = inVolume;
}

// uses the STREAM volume, rather than global volume
void flexible_audio::SetReplayGainVolume(double replayGain_dB)
{
    qDebug() << "SetReplayGainVolume" << replayGain_dB;
}

// ------------------------------------------------------------------
void flexible_audio::SetTempo(int newTempo)
{
    qDebug() << "Setting new tempo: " << newTempo;
    Stream_Tempo = newTempo;
}

// ------------------------------------------------------------------
void flexible_audio::SetPitch(int newPitch)
{
    qDebug() << "Setting new pitch: " << newPitch;
    Stream_Pitch = newPitch;
}

// ------------------------------------------------------------------
void flexible_audio::SetPan(double newPan)
{
    qDebug() << "Setting new pan: " << newPan;
    gStream_Pan = static_cast<float>(newPan);
}

// ------------------------------------------------------------------
// SetEq (band = 0,1,2), value = -15..15 (double)
void flexible_audio::SetEq(int band, double val)
{
    qDebug() << "Setting new EQ: " << band << val;
    Stream_Eq[band] = val;
}

// ------------------------------------------------------------------
//
void flexible_audio::SetCompression(unsigned int which, float val)
{
    qDebug() << "Setting new Compression: " << which << val;
}

void flexible_audio::SetCompressionEnabled(bool enable) {
    qDebug() << "SetCompressionEnabled" << enable;
}

// which = (FREQ_KHZ, BW_OCT, GAIN_DB)
void flexible_audio::SetIntelBoost(unsigned int which, float val)
{
    qDebug() << "SetIntelBoost:" << which << val;
}

void flexible_audio::SetIntelBoostEnabled(bool enable)
{
    qDebug() << "SetIntelBoostEnabled";
    IntelBoostShouldBeEnabled = enable;
}

void flexible_audio::SetGlobals()
{
    // global EQ, like Inteliigibility Boost, can only be set AFTER the song is loaded.
    //   before that, there is no Stream to set EQ on.
    // So, call this from loadMP3File() after song is loaded, so that global EQ is set, too,
    //   from the Global_IntelBoostEq parameters.
    qDebug() << "SetGlobals:";
    SetIntelBoost(FREQ_KHZ, Global_IntelBoostEq[FREQ_KHZ]);
    SetIntelBoost(BW_OCT, Global_IntelBoostEq[BW_OCT]);
    SetIntelBoost(GAIN_DB, Global_IntelBoostEq[GAIN_DB]);
}

void flexible_audio::songStartDetector(const char *filepath, double  *pSongStart, double  *pSongEnd) {
    qDebug() << "songStartDetector:" << *filepath << *pSongStart << *pSongEnd;
}

// ------------------------------------------------------------------
void flexible_audio::StreamCreate(const char *filepath, double  *pSongStart_sec, double  *pSongEnd_sec, double intro1_frac, double outro1_frac)
{
    qDebug() << "StreamCreate: " << *filepath << *pSongStart_sec << *pSongEnd_sec << intro1_frac << outro1_frac;
}

// ------------------------------------------------------------------
bool flexible_audio::isPaused(void)
{
    return(bPaused);
}

// ------------------------------------------------------------------
void flexible_audio::StreamSetPosition(double Position_sec)
{
    qDebug() << "StreamSetPosition:" << Position_sec;
}

// ------------------------------------------------------------------
void flexible_audio::StreamGetLength(void)
{
    qDebug() << "StreamGetLength:";
    FileLength = 100.0;  // FIX
}

// ------------------------------------------------------------------
void flexible_audio::StreamGetPosition(void)
{
    qDebug() << "StreamGetPosition";
    Current_Position = 50.0;  // FIX
}

// always asks the engine what the state is (NOT CACHED), then returns one of:
//    BASS_ACTIVE_STOPPED, BASS_ACTIVE_PLAYING, BASS_ACTIVE_STALLED, BASS_ACTIVE_PAUSED
uint32_t flexible_audio::currentStreamState() {
    return(BASS_ACTIVE_STOPPED); // FIX
}

// ------------------------------------------------------------------
int flexible_audio::StreamGetVuMeter(void)
{
    uint32_t Stream_State = currentStreamState();

    if (Stream_State == BASS_ACTIVE_PLAYING) {
        return(16000);  // FIX: 1/2 way up
    }
    else {
        return 0;
    }
}

// ------------------------------------------------------------------
void flexible_audio::SetLoop(double fromPoint_sec, double toPoint_sec)
{
    qDebug() << "SetLoop" << fromPoint_sec << toPoint_sec;
    loopFromPoint_sec = fromPoint_sec;
    loopToPoint_sec = toPoint_sec;
}

void flexible_audio::ClearLoop()
{
    qDebug() << "ClearLoop";
    loopFromPoint_sec = loopToPoint_sec = 0.0;
    startPoint_bytes = endPoint_bytes = 0;
}

void flexible_audio::SetMono(bool on)
{
    gStream_Mono = on;
}

// ------------------------------------------------------------------
void flexible_audio::Play(void)
{
    qDebug() << "Play";
    bPaused = false;
    StreamGetPosition();  // tell the position bar in main window where we are
}

// ------------------------------------------------------------------
void flexible_audio::Stop(void)
{
    qDebug() << "Stop";
    StreamSetPosition(0);
    StreamGetPosition();  // tell the position bar in main window where we are
    bPaused = true;
}

// ------------------------------------------------------------------
void flexible_audio::Pause(void)
{
    qDebug() << "Pause";
    StreamGetPosition();  // tell the position bar in main window where we are
    bPaused = true;
}


void flexible_audio::FadeOutAndPause(void) {
    qDebug() << "FadeOutAndPause";
}

void flexible_audio::StartVolumeDucking(int duckToPercent, double forSeconds) {
    qDebug() << "Start volume ducking to: " << duckToPercent << " for " << forSeconds << " seconds...";
    qDebug() << "StartVolumeDucking volume set to: " << static_cast<float>(Stream_MaxVolume * duckToPercent)/100.0f;

// WARNING:
// https://github.com/KDE/clazy/blob/master/docs/checks/README-connect-3arg-lambda.md
//    QTimer::singleShot(forSeconds*1000.0, [=] {
//        StopVolumeDucking();
//    });

}

void flexible_audio::StopVolumeDucking() {
    qDebug() << "StopVolumeducking, vol set to: " << Stream_MaxVolume;
}


// ------------------------------------------------------------------
void flexible_audio::PlayOrStopSoundEffect(int which, const char *filename, int volume) {
    qDebug() << "PlayOrStopSoundEffect:" << which << *filename << volume;
}

void flexible_audio::StopAllSoundEffects() {
    qDebug() << "StopAllSoundEffects";
}
