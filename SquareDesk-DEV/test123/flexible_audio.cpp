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
//#include "perftimer.h"
#include "audiodecoder.h"

// GLOBALS =========

AudioDecoder decoder; // TEST of AudioDecoder from example, must call setSource!
QMediaDevices md;

//PerfTimer gPT("flexible_audio::StreamCreate", 0);

// ------------------------------------------------------------------
flexible_audio::flexible_audio(void)
{
      connect(&decoder, SIGNAL(done()), this, SLOT(decoderDone()));  //

    currentSoundEffectID = 0;
    soundEffect.setAudioOutput(new QAudioOutput);
    connect(&soundEffect, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), this, SLOT(FXChannelStatusChanged(QMediaPlayer::MediaStatus)));

    QList<QAudioDevice> audioOutputList = QMediaDevices::audioOutputs();

//    qDebug() << "AUDIO OUTPUT DEVICES:";
//    QAudioDevice ad;
//    foreach( ad, audioOutputList ) {
//      qDebug() << ad.description();
//    }
    Q_UNUSED(audioOutputList)

    QAudioDevice defaultAD = QMediaDevices::defaultAudioOutput();
//    qDebug() << "DEFAULT AUDIO DEVICE: " << defaultAD.description();

    currentAudioDevice = defaultAD.description();  // this will show up in the statusBar

    bool b = connect(&md, SIGNAL(audioOutputsChanged()), this, SLOT(systemAudioOutputsChanged()));
    Q_UNUSED(b)
//    qDebug() << "Connection: " << b;
}

// ------------------------------------------------------------------
flexible_audio::~flexible_audio(void)
{
}

// ------------------------------------------------------------------
void flexible_audio::systemAudioOutputsChanged()
{
//    qDebug() << "***** systemAudioOutputsChanged!";

    QList<QAudioDevice> audioOutputList = QMediaDevices::audioOutputs();
    Q_UNUSED(audioOutputList)

    // Qt BUG:  This function should be called whenever I change the default in the Sound Preferences panel.
    //   However, it actually only changes right now when the devices list itself in the Sound Preferences panel changes.
    //   This happens when I plug or unplug something into the headphone jack, but it does NOT happen when I change the default.
    //   It seems to me that this is a bug.
    //   Also, ALL of the isDefault() flags are false, regardless of what I do, headphones plugged in or not, and
    //     default device changed or not.  That's probably the root cause of the bug.

    // The above is no longer true.  The isDefault appears to work.  BUT, sometimes when switching dynamically (while music is playing)
    //   playback will switch to the new device, and sometimes it doesn't.

//    qDebug() << "AUDIO OUTPUT DEVICES:";
//    QAudioDevice ad;
//    foreach( ad, audioOutputList ) {
//      qDebug() << ad.description() << ad.isDefault();
//    }

    QAudioDevice defaultAD = QMediaDevices::defaultAudioOutput();
//    qDebug() << "DEFAULT AUDIO DEVICE IS NOW: " << defaultAD.description();

    currentAudioDevice = defaultAD.description();

    uint32_t currentState = currentStreamState();
    if (currentState != BASS_ACTIVE_STOPPED) {
        decoder.Pause();
    }

    // decoder must be stopped to do this, otherwise we risk a crash
    decoder.newSystemAudioOutputDevice(); // this should switch us over, since we know we might need to switch

    if (currentState == BASS_ACTIVE_PLAYING) {
        decoder.Play();  // if it was playing, restart it.  If it was paused or stopped, we're already still paused/stopped.
    }
}


// ------------------------------------------------------------------
void flexible_audio::Init(void)
{
//    qDebug() << "flexible_audio::Init()";
}

// ------------------------------------------------------------------
void flexible_audio::Exit(void)
{
//    qDebug() << "flexible_audio::Exit()";
}

// ------------------------------------------------------------------
void flexible_audio::SetVolume(int inVolume)  // inVolume range: {0, 100}
{
//    qDebug() << "Setting new volume: " << inVolume;
    decoder.setVolume(inVolume);
}

// ------------------------------------------------------------------
void flexible_audio::SetTempo(int newTempoPercent)  // range: {80, 120}
{
//    qDebug() << "Setting new tempo: " << newTempoPercent << "%";
    Stream_Tempo = newTempoPercent;
    decoder.setTempo(newTempoPercent);
}

// ------------------------------------------------------------------
void flexible_audio::SetPitch(int newPitch)  // range: {-5, 5}
{
//    qDebug() << "Setting new pitch: " << newPitch;
    Stream_Pitch = newPitch;
    decoder.setPitch(newPitch);
}

// ------------------------------------------------------------------
void flexible_audio::SetPan(double newPan)  // range: {-1.0,1.0}
{
//    qDebug() << "Setting new pan: " << newPan;
    decoder.setPan(newPan);
}

// ------------------------------------------------------------------
// SetEq (band = 0,1,2), value = -15..15 (double)
void flexible_audio::SetEq(int band, double val)
{
//    qDebug() << "Setting new EQ: " << band << val;
    Stream_Eq[band] = val;

    switch (band) {
    case 0:
        decoder.setBassBoost(val);
        break;
    case 1:
        decoder.setMidBoost(val);
        break;
    case 2:
        decoder.setTrebleBoost(val);
        break;
    default:
//        qDebug() << "BAD EQ BAND: " << band << ", val:" << val;
        break;
    }
}

// ------------------------------------------------------------------
// which = (FREQ_KHZ, BW_OCT, GAIN_DB)
void flexible_audio::SetIntelBoost(unsigned int which, float val)
{
    Global_IntelBoostEq[which] = val;
    decoder.SetIntelBoost(which, val);
}

void flexible_audio::SetIntelBoostEnabled(bool enable)
{
    IntelBoostShouldBeEnabled = enable;
    decoder.SetIntelBoostEnabled(enable);
}

void flexible_audio::SetGlobals()
{
    // global EQ, like Intelligibility Boost, can only be set AFTER the song is loaded.
    //   before that, there is no Stream to set EQ on.
    // So, call this from loadMP3File() after song is loaded, so that global EQ is set, too,
    //   from the Global_IntelBoostEq parameters.
//    qDebug() << "NOT IMPLEMENTED: SetGlobals:";
    SetIntelBoost(FREQ_KHZ, Global_IntelBoostEq[FREQ_KHZ]);
    SetIntelBoost(BW_OCT, Global_IntelBoostEq[BW_OCT]);
    SetIntelBoost(GAIN_DB, Global_IntelBoostEq[GAIN_DB]);
}

void flexible_audio::songStartDetector(const char *filepath, double  *pSongStart, double  *pSongEnd) {
    Q_UNUSED(filepath)
    Q_UNUSED(pSongStart)
    Q_UNUSED(pSongEnd)
//    qDebug() << "NOT IMPLEMENTED: songStartDetector:" << *filepath << *pSongStart << *pSongEnd;
}

// ------------------------------------------------------------------
void flexible_audio::StreamCreate(const char *filepath, double  *pSongStart_sec, double  *pSongEnd_sec, double intro1_frac, double outro1_frac)
{
    Q_UNUSED(pSongStart_sec)
    Q_UNUSED(pSongEnd_sec)
    Q_UNUSED(intro1_frac)
    Q_UNUSED(outro1_frac)

//    qDebug() << "flexible_audio::StreamCreate: " << filepath << *pSongStart_sec << *pSongEnd_sec << intro1_frac << outro1_frac;
    decoder.setSource(filepath);  // decode THIS file
    Stream_BPM = -1;  // means "not available yet"
    decoder.start();              // start the decode
}

qint64 flexible_audio::readData(char* data, qint64 maxlen)
{
    Q_UNUSED(data);
    Q_UNUSED(maxlen);
//    qDebug() << "UNEXPECTED flexible_audio::readData()";
    return(0);
}

qint64 flexible_audio::writeData(const char* data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
//    qDebug() << "UNEXPECTED flexible_audio::writeData()";
    return 0;
}

void flexible_audio::decoder_error()
{
//    qDebug() << "***** flexible_audio::decoder_error";
}

void flexible_audio::posChanged(qint64 a)
{
    Q_UNUSED(a)
//    qDebug() << "***** flexible_audio::posChanged" << a;
}

void flexible_audio::durChanged(qint64 a)
{
    Q_UNUSED(a)
//    qDebug() << "***** flexible_audio::durChanged" << a;
}

void flexible_audio::decoderDone() // SLOT
{
//    qDebug() << "flexible_audio::decoder_done() time to alert the cBass!";
    Stream_BPM = decoder.getBPM();  // -1 = not ready yet, 0 = undetectable, else returns double
    emit haveDuration();  // tell others that we have a valid duration and Stream_BPM now
}

void flexible_audio::bufferReady() // SLOT
{
//    qDebug() << "***** flexible_audio::bufferReady()";
}

void flexible_audio::finished() // SLOT
{
//    qDebug() << "***** flexible_audio::finished()";
}
// ------------------------------------------------------------------
bool flexible_audio::isPaused(void)
{
    return(bPaused);
}

// ------------------------------------------------------------------
void flexible_audio::StreamSetPosition(double Position_sec)
{
//    qDebug() << "StreamSetPosition:" << Position_sec;
    decoder.setStreamPosition(Position_sec);  // tell the decoder, which will tell myPlayer
}

// ------------------------------------------------------------------
void flexible_audio::StreamGetLength(void)
{
    FileLength = decoder.getStreamLength();
//    qDebug() << "StreamGetLength:" << FileLength;
}

// ------------------------------------------------------------------
void flexible_audio::StreamGetPosition(void)
{
    Current_Position = decoder.getStreamPosition(); // double in seconds
//    qDebug() << "flexible_audio::StreamGetPosition:" << Current_Position << ", isDecoding:" << m_decoder.isDecoding() << ", errorStr:" << m_decoder.errorString();
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
        return(decoder.getPeakLevel());
    }
    else {
        return 0;
    }
}

// ------------------------------------------------------------------
void flexible_audio::SetLoop(double fromPoint_sec, double toPoint_sec)
{
//    qDebug() << "flexible_audio::SetLoop: (" << fromPoint_sec << "," << toPoint_sec << ")";
    loopFromPoint_sec = fromPoint_sec;
    loopToPoint_sec = toPoint_sec;

    decoder.setLoop(loopFromPoint_sec, loopToPoint_sec);
}

void flexible_audio::ClearLoop()
{
//    qDebug() << "flexible_audio::ClearLoop";
    loopFromPoint_sec = loopToPoint_sec = 0.0;  // both 0.0 means disabled
    decoder.clearLoop();
}

// ------------------------------------------------------------------
void flexible_audio::SetMono(bool on)
{
//    gStream_Mono = on;
//    qDebug() << "flexible_audio::SetMono()";
    decoder.setMono(on);
}

// ------------------------------------------------------------------
void flexible_audio::Play(void)
{
//    QAudioDevice defaultAD = QMediaDevices::defaultAudioOutput();
//    qDebug() << "PLAY (DEFAULT AUDIO DEVICE: " << defaultAD.description() << ")";

//    qDebug() << "flexible_audio::Play";
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
//    qDebug() << "flexible_audio::Stop";
    decoder.Stop();
    StreamSetPosition(0);
    StreamGetPosition();  // tell the position bar in main window where we are
    bPaused = true;
}

void flexible_audio::Pause(void)
{
//    qDebug() << "flexible_audio::Pause";
    decoder.Pause();
    StreamGetPosition();  // tell the position bar in main window where we are
    bPaused = true;
}

// ------------------------------------------------------------------
void flexible_audio::FadeOutAndPause(void) {
//    qDebug() << "FadeOutAndPause";
    decoder.fadeOutAndPause(0.0, 6.0);  // go to volume 0.0 in 6.0 seconds
}

// SOUND FX ------------------------------------------------------------------
void flexible_audio::StartVolumeDucking(int duckToPercent, double forSeconds) {
//    qDebug() << "Start volume ducking to: " << duckToPercent << " for " << forSeconds << " seconds...";
//    qDebug() << "StartVolumeDucking volume set to: " << static_cast<float>(Stream_MaxVolume * duckToPercent)/100.0f;

    decoder.StartVolumeDucking(duckToPercent, forSeconds);
}

void flexible_audio::StopVolumeDucking() {
//    qDebug() << "StopVolumeducking, vol set to: " << Stream_MaxVolume;
    decoder.StopVolumeDucking();
}

void flexible_audio::FXChannelStartPlaying(const char *filename) {
//    qDebug() << "FXChannelStartPlaying():" << filename;
    soundEffect.setSource(QUrl::fromLocalFile(filename));
//    soundEffect.setVolume(1.0f);
    soundEffect.play();
}

void flexible_audio::FXChannelStopPlaying() {
//    qDebug() << "FXChannelStopPlaying()";
    soundEffect.stop();
}

bool flexible_audio::FXChannelIsPlaying() {
//    qDebug() << "FXChannelIsPlaying()";
//    return(soundEffect.isPlaying());
    return(soundEffect.playbackState() == QMediaPlayer::PlayingState);
}

void flexible_audio::FXChannelStatusChanged(QMediaPlayer::MediaStatus status) {
//    QMediaPlayer::MediaStatus status = soundEffect.mediaStatus();
//    qDebug() << "flexible_audio::FXChannelStatusChange to: " << status;
    if (status == QMediaPlayer::EndOfMedia) {
//        qDebug() << "status change caused early stoppage of volume ducking";
        StopVolumeDucking();  // stop ducking early, if media finishes before 2.0 seconds
    }
}

void flexible_audio::PlayOrStopSoundEffect(int which, const char *filename, int volume) {
    Q_UNUSED(volume)

    if (which == currentSoundEffectID &&
        FXChannelIsPlaying()) {
        // if the user pressed the same key again, while playing back...
        FXChannelStopPlaying();      // stop the current FX stream
        currentSoundEffectID = 0;    // nothing playing anymore
        StopVolumeDucking();
        return;
    }

    FXChannelStartPlaying(filename);   // play sound effect file here...

    double FXLength_seconds = 10.0;  // LONGEST EXPECTED SOUND FX EVER (failsafe)
    StartVolumeDucking(20, FXLength_seconds);   // duck music to 20%
    currentSoundEffectID = which;               // keep track of which sound effect is playing
}

void flexible_audio::StopAllSoundEffects() {
//    qDebug() << "StopAllSoundEffects";

    FXChannelStopPlaying();     // stop any in-progress sound FX
    StopVolumeDucking();        // stop ducking of music, if it was in effect...
    currentSoundEffectID = 0;   // no sound effect is playing now
}

#endif
