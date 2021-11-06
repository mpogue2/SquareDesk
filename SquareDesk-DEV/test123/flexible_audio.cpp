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
float gStream_Pan = 0.0;
bool  gStream_Mono = false;
float gStream_replayGain = 1.0f;    // replayGain

AudioDecoder decoder; // TEST of AudioDecoder from example, must call setSource!

//PerfTimer gPT("flexible_audio::StreamCreate", 0);

#define USEMEDIAPLAYER

// ------------------------------------------------------------------
flexible_audio::flexible_audio(void) :
    m_input(&m_data)
{
//#ifdef USEMEDIAPLAYER
      player = new QMediaPlayer;
      audioOutput = new QAudioOutput;
      player->setAudioOutput(audioOutput);
//#else

#ifndef USEMEDIAPLAYER

      connect(&m_decoder, SIGNAL(finished()), this, SLOT(finished()));  // PROBLEM: NEVER GET THIS MESSAGE
      connect(&m_decoder, SIGNAL(bufferReady()), this, SLOT(bufferReady()));
      // WEIRD:  I don't get calls to bufferReady, unless the finished function is connected.

//      connect(&m_decoder, QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error),
//          [=](QAudioDecoder::Error error){ Q_UNUSED(error); qDebug() << "***** ERROR:" << m_decoder.errorString(); });
      connect(&m_decoder, SIGNAL(positionChanged(qint64)), this, SLOT(posChanged(qint64)));
      connect(&m_decoder, SIGNAL(durationChanged(qint64)), this, SLOT(durChanged(qint64)));
#endif

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
void flexible_audio::SetVolume(int inVolume)  // inVolume range: {0, 100}
{
    qDebug() << "Setting new volume: " << inVolume;
//    Stream_Volume = inVolume;
#ifdef USEMEDIAPLAYER
//    audioOutput->setVolume(inVolume/100.0); // float range: {0.0, 1.0}
#endif
    decoder.setVolume(inVolume);
}

// uses the STREAM volume, rather than global volume
void flexible_audio::SetReplayGainVolume(double replayGain_dB)
{
    qDebug() << "OBSOLETE: SetReplayGainVolume" << replayGain_dB;
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
void flexible_audio::SetPan(double newPan)  // range: {-1.0,1.0}
{
    qDebug() << "Setting new pan: " << newPan;
//    gStream_Pan = static_cast<float>(newPan);
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
    // global EQ, like Intelligibility Boost, can only be set AFTER the song is loaded.
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
    qDebug() << "StreamCreate: " << filepath << *pSongStart_sec << *pSongEnd_sec << intro1_frac << outro1_frac;
#ifdef USEMEDIAPLAYER
//    player->setSource(QUrl::fromLocalFile(filepath));

//    player->setSource(QUrl::fromLocalFile("/Users/mpogue/Google Drive/__squareDanceMusic/archive.org/out123.wav"));

//    // ***** TEST OF AUDIODECODER.CPP *****
//    //QObject::connect(&decoder, &AudioDecoder::done,
//    //                 &app, &QCoreApplication::quit);
//    decoder.setSource("/Users/mpogue/Google Drive/__squareDanceMusic/archive.org/ARCH1 - JacksonsHornpipe (Jackson).mp3");
//    decoder.setSource("/Users/mpogue/Google Drive/__squareDanceMusic/archive.org/silence10s.mp3");
//    decoder.setSource("/Users/mpogue/Google Drive/__squareDanceMusic/archive.org/tone10s.mp3");
//    decoder.setSource("/Users/mpogue/Google Drive/__squareDanceMusic/archive.org/ARCH1 - JacksonsHornpipe (Jackson).mp3");  // WORKS
    decoder.setSource(filepath);  // this guy will also read the file (THIS DOES NOT WORK.  MUST BE DIFFERENT FILE.)
    qDebug() << "Stopping the decoder to make sure its read head is back at zero...";
    decoder.stop();
    qDebug() << "starting the decoder....";
    decoder.start();
//    if (decoder.getError() != QAudioDecoder::NoError) {
//        qDebug() << "decoder ERROR!";
//    }

#else
//    gPT.start(0);

    if (m_decoder.isDecoding()) {
        qDebug() << "stopping decoder";
        m_decoder.stop();  // in case it was already running
    }

    if (m_input.isOpen()) {
//        m_input.seek(0);  // reuse the buffer
        qDebug() << "closing and reopening m_input";
        m_input.close();
        m_input.open(QIODevice::WriteOnly);
    } else {
        qDebug() << "opening m_input";
        m_input.open(QIODevice::WriteOnly);
    }

    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    QAudioFormat desiredAudioFormat; // = device.preferredFormat();  // 48000, 2ch, float = WHY?  Why not 16-bit int?  Less memory used.
    desiredAudioFormat.setSampleRate(44100);
    desiredAudioFormat.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    desiredAudioFormat.setSampleFormat(QAudioFormat::Int16);

    qDebug() << "desiredAudioFormat" << desiredAudioFormat;

    m_decoder.setAudioFormat(desiredAudioFormat);

    m_file.setFileName(filepath);
//  m_file.setFileName("/Users/mpogue/Google Drive/__squareDanceMusic/archive.org/ARCH1 - JacksonsHornpipe (Jackson).mp3");
//    m_file.setFileName("/Users/mpogue/Google Drive/__squareDanceMusic/archive.org/ARCH1 - JacksonsHornpipe (Jackson).wav");
//    m_file.setFileName("/Users/mpogue/Google Drive/__squareDanceMusic/archive.org/ARCH1 - BoilThemCabbageDown (Wimberly).mp3");
    if (!m_file.open(QIODevice::ReadOnly))  // TODO: close the file after we're done reading
    {
        qDebug() << "ERROR: Could not open file:" << filepath;
        return;
    } else {
        qDebug() << "no error on opening audio file:"  << filepath;
    }

    totalBytes = 0;

    qDebug() << "Starting m_decoder...";
    m_decoder.setSourceDevice(&m_file);
    m_decoder.start();
#endif
}


qint64 flexible_audio::readData(char* data, qint64 maxlen)
{
    Q_UNUSED(data);
    Q_UNUSED(maxlen);
    qDebug() << "unexpected readData()";
    return(0);
}

qint64 flexible_audio::writeData(const char* data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    qDebug() << "unexpected writeData()";
    return 0;
}

void flexible_audio::decoder_error()
{
    qDebug() << "***** decoder_error";
}

void flexible_audio::posChanged(qint64 a)
{
    qDebug() << "***** posChanged" << a;
}

void flexible_audio::durChanged(qint64 a)
{
    qDebug() << "***** durChanged" << a;
}

void flexible_audio::bufferReady() // SLOT
{
#ifndef USEMEDIAPLAYER
//    int cnt = 0;
//    for(; m_decoder.bufferAvailable(); cnt++) {
    if (m_decoder.bufferAvailable()) {
        const QAudioBuffer &buffer = m_decoder.read();

        const int length = buffer.byteCount();
//        const char *data = buffer.constData<char>();
        totalBytes += length;

        //m_input.write(data, length);
        qDebug() << "got " << length << "bytes, totalBytes = " << totalBytes << "pos_sec:" << m_decoder.position()/1000.0 << "dur_sec:" << m_decoder.duration()/1000.0 << "error: '" << m_decoder.errorString() << "'";

    } else {
        qDebug() << "call to bufferReady, but buffer is not available" << m_decoder.errorString();
    }

    //qDebug() << cnt << " bufs read" << ", b:" << m_input.size();
#endif
}


void flexible_audio::finished() // SLOT
{
//    gPT.elapsed(0);
    qDebug() << "***** finished()";
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
#ifdef USEMEDIAPLAYER
    player->setPosition((qint64)(Position_sec * 1000.0));
#endif
}

// ------------------------------------------------------------------
void flexible_audio::StreamGetLength(void)
{
#ifdef USEMEDIAPLAYER
    qint64 duration_ms = player->duration();
    FileLength = (double)duration_ms / 1000.0;
#endif
    qDebug() << "StreamGetLength:" << FileLength;
}

// ------------------------------------------------------------------
void flexible_audio::StreamGetPosition(void)
{
#ifdef USEMEDIAPLAYER
    qint64 position_ms = player->position();
    Current_Position = (double)position_ms/1000.0;
#endif
    qDebug() << "StreamGetPosition:" << Current_Position << "isDecoding:" << m_decoder.isDecoding() << "errorStr:" << m_decoder.errorString();
}

// always asks the engine what the state is (NOT CACHED), then returns one of:
//    BASS_ACTIVE_STOPPED, BASS_ACTIVE_PLAYING, BASS_ACTIVE_STALLED, BASS_ACTIVE_PAUSED
uint32_t flexible_audio::currentStreamState() {
//#ifdef USEMEDIAPLAYER
//    QMediaPlayer::PlaybackState state = player->playbackState(); // StoppedState, PlayingState, PausedState

//    switch (state) {
//    case QMediaPlayer::StoppedState: return(BASS_ACTIVE_STOPPED);
//    case QMediaPlayer::PlayingState: return(BASS_ACTIVE_PLAYING);
//    case QMediaPlayer::PausedState:  return(BASS_ACTIVE_PAUSED);
//    default: return(BASS_ACTIVE_STALLED);  // should never happen
//    }
//#else
    return(BASS_ACTIVE_STALLED);
//#endif
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
//#ifdef USEMEDIAPLAYER
//    player->play();
//#else
    if (decoder.isPlaying()) {
        decoder.Pause();
    } else {
        decoder.Play();
    }
//#endif
    bPaused = false;
    StreamGetPosition();  // tell the position bar in main window where we are
}

// ------------------------------------------------------------------
void flexible_audio::Stop(void)
{
    qDebug() << "Stop";
//#ifdef USEMEDIAPLAYER
//    player->stop();
//#else
    decoder.Stop();
//#endif
    StreamSetPosition(0);
    StreamGetPosition();  // tell the position bar in main window where we are
    bPaused = true;
}

// ------------------------------------------------------------------
void flexible_audio::Pause(void)
{
    qDebug() << "Pause";
//#ifdef USEMEDIAPLAYER
//    player->pause();
//#else
    decoder.Pause();
//#endif
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

#endif
