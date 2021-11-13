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

// IMPORTANT: If we're compiling for an M1 Silicon MAC, then use THIS FILE.  Else use contents of bass_audio.cpp
//   This is a short term thing, until the flexible audio class comes up to the same level as bass_audio.
// M1MAC is defined only on Mac's and only on M1 Silicon Macs.
#ifndef M1MAC
#include "bass_audio.cpp"
#else

#include "flexible_audio.h"
#include <math.h>
#include <stdio.h>
#include <QDebug>
#include <QTimer>
#include "perftimer.h"
#include "audiodecoder.h"

// GLOBALS =========

AudioDecoder decoder; // TEST of AudioDecoder from example, must call setSource!

//PerfTimer gPT("flexible_audio::StreamCreate", 0);

// ------------------------------------------------------------------
flexible_audio::flexible_audio(void)
{
      connect(&decoder, SIGNAL(done()), this, SLOT(decoderDone()));  //
}

// ------------------------------------------------------------------
flexible_audio::~flexible_audio(void)
{
}

// ------------------------------------------------------------------
void flexible_audio::Init(void)
{
    qDebug() << "flexible_audio::Init()";
}

// ------------------------------------------------------------------
void flexible_audio::Exit(void)
{
    qDebug() << "flexible_audio::Exit()";
}

// ------------------------------------------------------------------
void flexible_audio::SetVolume(int inVolume)  // inVolume range: {0, 100}
{
    qDebug() << "Setting new volume: " << inVolume;
    decoder.setVolume(inVolume);
}

// uses the STREAM volume, rather than global volume
void flexible_audio::SetReplayGainVolume(double replayGain_dB)
{
    qDebug() << "OBSOLETE: SetReplayGainVolume" << replayGain_dB;
}

// ------------------------------------------------------------------
void flexible_audio::SetTempo(int newTempo)  // range: {80, 120}
{
    qDebug() << "Setting new tempo: " << newTempo;
    Stream_Tempo = newTempo;
}

// ------------------------------------------------------------------
void flexible_audio::SetPitch(int newPitch)  // range: {-5, 5}
{
    qDebug() << "Setting new pitch: " << newPitch;
    Stream_Pitch = newPitch;
}

// ------------------------------------------------------------------
void flexible_audio::SetPan(double newPan)  // range: {-1.0,1.0}
{
    qDebug() << "Setting new pan: " << newPan;
    decoder.setPan(newPan);
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
    qDebug() << "NOT IMPLEMENTED: Setting new Compression: " << which << val;
}

void flexible_audio::SetCompressionEnabled(bool enable) {
    qDebug() << "NOT IMPLEMENTED: SetCompressionEnabled" << enable;
}

// which = (FREQ_KHZ, BW_OCT, GAIN_DB)
void flexible_audio::SetIntelBoost(unsigned int which, float val)
{
    qDebug() << "NOT IMPLEMENTED: SetIntelBoost:" << which << val;
}

void flexible_audio::SetIntelBoostEnabled(bool enable)
{
    qDebug() << "NOT IMPLEMENTED: SetIntelBoostEnabled";
    IntelBoostShouldBeEnabled = enable;
}

void flexible_audio::SetGlobals()
{
    // global EQ, like Intelligibility Boost, can only be set AFTER the song is loaded.
    //   before that, there is no Stream to set EQ on.
    // So, call this from loadMP3File() after song is loaded, so that global EQ is set, too,
    //   from the Global_IntelBoostEq parameters.
    qDebug() << "NOT IMPLEMENTED: SetGlobals:";
    SetIntelBoost(FREQ_KHZ, Global_IntelBoostEq[FREQ_KHZ]);
    SetIntelBoost(BW_OCT, Global_IntelBoostEq[BW_OCT]);
    SetIntelBoost(GAIN_DB, Global_IntelBoostEq[GAIN_DB]);
}

void flexible_audio::songStartDetector(const char *filepath, double  *pSongStart, double  *pSongEnd) {
    qDebug() << "NOT IMPLEMENTED: songStartDetector:" << *filepath << *pSongStart << *pSongEnd;
}

// ------------------------------------------------------------------
void flexible_audio::StreamCreate(const char *filepath, double  *pSongStart_sec, double  *pSongEnd_sec, double intro1_frac, double outro1_frac)
{
    qDebug() << "flexible_audio::StreamCreate: " << filepath << *pSongStart_sec << *pSongEnd_sec << intro1_frac << outro1_frac;
    decoder.setSource(filepath);  // decode THIS file
    decoder.start();              // start the decode
}

qint64 flexible_audio::readData(char* data, qint64 maxlen)
{
    Q_UNUSED(data);
    Q_UNUSED(maxlen);
    qDebug() << "UNEXPECTED flexible_audio::readData()";
    return(0);
}

qint64 flexible_audio::writeData(const char* data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    qDebug() << "UNEXPECTED flexible_audio::writeData()";
    return 0;
}

void flexible_audio::decoder_error()
{
    qDebug() << "***** flexible_audio::decoder_error";
}

void flexible_audio::posChanged(qint64 a)
{
    qDebug() << "***** flexible_audio::posChanged" << a;
}

void flexible_audio::durChanged(qint64 a)
{
    qDebug() << "***** flexible_audio::durChanged" << a;
}

void flexible_audio::decoderDone() // SLOT
{
    qDebug() << "flexible_audio::decoder_done() time to alert the cBass!";
    emit haveDuration();  // tell others that we have a valid duration now
}

void flexible_audio::bufferReady() // SLOT
{
    qDebug() << "***** flexible_audio::bufferReady()";
}

void flexible_audio::finished() // SLOT
{
    qDebug() << "***** flexible_audio::finished()";
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
    decoder.setStreamPosition(Position_sec);  // tell the decoder, which will tell myPlayer
}

// ------------------------------------------------------------------
void flexible_audio::StreamGetLength(void)
{
    FileLength = decoder.getStreamLength();
    qDebug() << "StreamGetLength:" << FileLength;
}

// ------------------------------------------------------------------
void flexible_audio::StreamGetPosition(void)
{
    Current_Position = decoder.getStreamPosition(); // double in seconds
//    qDebug() << "StreamGetPosition:" << Current_Position << ", isDecoding:" << m_decoder.isDecoding() << ", errorStr:" << m_decoder.errorString();
}

// always asks the engine what the state is (NOT CACHED), then returns one of:
//    BASS_ACTIVE_STOPPED, BASS_ACTIVE_PLAYING, BASS_ACTIVE_STALLED, BASS_ACTIVE_PAUSED
uint32_t flexible_audio::currentStreamState() {
//    qDebug() << "flexible_audio::currentStreamState()";
    return(decoder.getCurrentState());
}

// ------------------------------------------------------------------
int flexible_audio::StreamGetVuMeter(void)
{
    // qDebug() << "NOT IMPLEMENTED: StreamGetVuMeter()";
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
    qDebug() << "NOT IMPLEMENTED: SetLoop" << fromPoint_sec << toPoint_sec;
    loopFromPoint_sec = fromPoint_sec;
    loopToPoint_sec = toPoint_sec;
}

void flexible_audio::ClearLoop()
{
    qDebug() << "NOT IMPLEMENTED: ClearLoop";
    loopFromPoint_sec = loopToPoint_sec = 0.0;
    startPoint_bytes = endPoint_bytes = 0;
}

// ------------------------------------------------------------------
void flexible_audio::SetMono(bool on)
{
//    gStream_Mono = on;
    qDebug() << "flexible_audio::SetMono()";
    decoder.setMono(on);
}

// ------------------------------------------------------------------
void flexible_audio::Play(void)
{
    qDebug() << "flexible_audio::Play";
    if (decoder.isPlaying()) {
        decoder.Pause();
    } else {
        decoder.Play();
    }
    bPaused = false;
    StreamGetPosition();  // tell the position bar in main window where we are
}

void flexible_audio::Stop(void)
{
    qDebug() << "flexible_audio::Stop";
    decoder.Stop();
    StreamSetPosition(0);
    StreamGetPosition();  // tell the position bar in main window where we are
    bPaused = true;
}

void flexible_audio::Pause(void)
{
    qDebug() << "flexible_audio::Pause";
    decoder.Pause();
    StreamGetPosition();  // tell the position bar in main window where we are
    bPaused = true;
}

// ------------------------------------------------------------------
void flexible_audio::FadeOutAndPause(void) {
    qDebug() << "NOT IMPLEMENTED: FadeOutAndPause";
}

void flexible_audio::StartVolumeDucking(int duckToPercent, double forSeconds) {
    qDebug() << "NOT IMPLEMENTED: Start volume ducking to: " << duckToPercent << " for " << forSeconds << " seconds...";
    qDebug() << "NOT IMPLEMENTED: StartVolumeDucking volume set to: " << static_cast<float>(Stream_MaxVolume * duckToPercent)/100.0f;
}

void flexible_audio::StopVolumeDucking() {
    qDebug() << "NOT IMPLEMENTED: StopVolumeducking, vol set to: " << Stream_MaxVolume;
}


// ------------------------------------------------------------------
void flexible_audio::PlayOrStopSoundEffect(int which, const char *filename, int volume) {
    qDebug() << "NOT IMPLEMENTED: PlayOrStopSoundEffect:" << which << *filename << volume;
}

void flexible_audio::StopAllSoundEffects() {
    qDebug() << "NOT IMPLEMENTED: StopAllSoundEffects";
}

#endif
