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

#include "bass_audio.h"
#include "bass_fx.h"
#include <math.h>
#include <stdio.h>
#include <QDebug>
#include <QTimer>

// GLOBALS =========
float gStream_Pan = 0.0;
bool gStream_Mono = false;
HDSP gMono_dsp = 0;  // DSP handle (u32)

// ========================================================================
// Mix/Pan, then optionally mix down to mono
// http://bass.radio42.com/help/html/b8b8a713-7af4-465e-a612-1acd769d4639.htm
// http://www.delphipraxis.net/141676-bass-dll-stereo-zu-mono-2.html
// also see the dsptest.c example (e.g. echo)
void CALLBACK DSP_Mono(HDSP handle, DWORD channel, void *buffer, DWORD length, void *user)
{
    Q_UNUSED(user)
    Q_UNUSED(channel)
    Q_UNUSED(handle)

    // NOTE: uses globals: gStream_Mono (true|false) and gStream_Pan (-1.0 = all L, 1.0 = all R)

    float *d = (float *)buffer;
    DWORD a;
    float outL, outR;
    float inL,inR;
    float mono;

    const float PI_OVER_2 = (float)3.14159265/2.0;
    float theta = PI_OVER_2 * (gStream_Pan + 1.0)/2.0;  // convert to 0-PI/2
    float KL = cos(theta);
    float KR = sin(theta);

    if (gStream_Mono) {
        // FORCE MONO mode
        for (a=0; a<length/4; a+=2)
        {
            inL = d[a];
            inR = d[a+1];
            outL = KL * inL;
            outR = KR * inR;             // constant power pan
            mono = (outL + outR)/2.0;  // mix down to mono for BOTH output channels
            d[a] = d[a+1] = mono;
        }

    } else {
        // NORMAL STEREO mode
        for (a=0; a<length/4; a+=2)
        {
            inL = d[a];
            inR = d[a+1];
            outL = KL * inL;
            outR = KR * inR;  // constant power pan
            d[a] = outL;
            d[a+1] = outR;
        }
    }
}

// ------------------------------------------------------------------
bass_audio::bass_audio(void)
{
    Stream = (HSTREAM)NULL;
    FXStream = (HSTREAM)NULL;  // also initialize the FX stream

    Stream_State = (HSTREAM)NULL;
    Stream_Volume = 100; ///10000 (multipled on output)
    Stream_Tempo = 100;  // current tempo, relative to 100
    Stream_Pitch = 0;    // current pitch correction, in semitones; -5 .. 5
    Stream_BPM = 0.0;    // current estimated BPM (original song, without tempo correction)
//    Stream_Pan = 0.0;
//    Stream_Mono = false; // true, if FORCE MONO mode

    fxEQ = 0;            // equalizer handle
    Stream_Eq[0] = 50.0;  // current EQ (3 bands), 0 = Bass, 1 = Mid, 2 = Treble
    Stream_Eq[1] = 50.0;
    Stream_Eq[2] = 50.0;

    FileLength = 0.0;
    Current_Position = 0.0;
    bPaused = false;

    loopFromPoint_sec = -1.0;
    loopToPoint_sec = -1.0;
    startPoint_bytes = 0;
    endPoint_bytes = 0;

    currentSoundEffectID = 0;    // no soundFX playing now

    syncHandle = 0;  // means "no handle"
}

// ------------------------------------------------------------------
bass_audio::~bass_audio(void)
{
}

// ------------------------------------------------------------------
void bass_audio::Init(void)
{
    //-------------------------------------------------------------
    if (!BASS_Init(-1, 44100, 0, NULL, NULL)) {   // the "default" device
        qDebug() << "ERROR " << BASS_ErrorGetCode() << " in bass_audio::Init(-1)";
    }
    BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, Stream_Volume * 100);
    //-------------------------------------------------------------
}

// ------------------------------------------------------------------
void bass_audio::Exit(void)
{
    BASS_Free();
}

// ------------------------------------------------------------------
void bass_audio::SetVolume(int inVolume)
{
    Stream_Volume = inVolume;
    BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, Stream_Volume * 100);
}

// ------------------------------------------------------------------
void bass_audio::SetTempo(int newTempo)
{
    Stream_Tempo = newTempo;
    BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_TEMPO, (float)(newTempo-100.0f)); // pass -10 to go 10% slower
}

// ------------------------------------------------------------------
void bass_audio::SetPitch(int newPitch)
{
    Stream_Pitch = newPitch;
    BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_TEMPO_PITCH, (float)newPitch);
}

// ------------------------------------------------------------------
void bass_audio::SetPan(float newPan)
{
    gStream_Pan = newPan;
//    BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_PAN, newPan);  // Panning/Mixing now done in MONO routine
}

// ------------------------------------------------------------------
// SetEq (band = 0,1,2), value = -15..15 (float)
void bass_audio::SetEq(int band, float val)
{
    Stream_Eq[band] = val;

    BASS_BFX_PEAKEQ eq;
    eq.lBand = band;    // get all values of the selected band
    BASS_FXGetParameters(fxEQ, &eq);

    eq.fGain = val;     // modify just the level of the selected band, and set all
    BASS_FXSetParameters(fxEQ, &eq);
}

// *******************
// http://bass.radio42.com/help/html/ee197eac-8482-1f9a-e0e1-8ec9e4feda9b.htm
void CALLBACK MyFadeIsDoneProc(HSYNC handle, DWORD channel, DWORD data, void *user)
{
    Q_UNUSED(handle)
    Q_UNUSED(data)
    Q_UNUSED(user)
//    qDebug() << "FADE IS DONE.";
    DWORD Stream_State = BASS_ChannelIsActive(channel);
    if (Stream_State == BASS_ACTIVE_PLAYING) {
//        qDebug() << "     Pausing playback";
        // if active, PAUSE the stream
        BASS_ChannelPause(channel);

        ((bass_audio*)user)->StreamGetPosition();
        ((bass_audio*)user)->bPaused = true;
    }
}

float bass_audio::songStartDetector(const char *filepath) {
    // returns start of non-silence in seconds, max 20 seconds

    int peak = 0;
    int levels[200];

    BASS_Init(0 /* "NO SOUND" device */, 44100, 0, 0, NULL);

    HSTREAM chan = BASS_StreamCreateFile(FALSE, filepath, 0, 0, BASS_STREAM_DECODE);
    if ( chan )
    {
        float block = 20;  // take a 20ms level sample every 20ms

        // BASS_ChannelGetLevel takes 20ms from the channel
        QWORD len = BASS_ChannelSeconds2Bytes(chan, block/1000.0 - 0.02);  // always takes a 0.02s level sample

        char data[len];  // data sink
        DWORD level, left, right;

        int sampleNum = 0;
        int j = 0;
        int sum = 0;
        while ( sampleNum < 200 && -1 != (int)(level = BASS_ChannelGetLevel(chan) ) ) // takes 20ms sample every 100ms for 20 sec
        {
            left=LOWORD(level); // the left level
            right=HIWORD(level); // the right level
            unsigned int avg = (left+right)/2;
            if (j++ < 4) {
                sum += avg;
            } else {
                levels[sampleNum] = sum/5;  // sum the 20ms contributions every 100ms, result is 100ms samples of energy
                printf("%d: %d\n", sampleNum++, sum/5);
                peak = (peak < sum/5 ? sum/5 : peak);  // this finds the peak of the 100ms samples
                sum = 0;
                j = 0;
            }
            BASS_ChannelGetData(chan, data, len); // get data away from the channel
        }
        BASS_StreamFree( chan );
    }

    BASS_Free();

    int k = 0;
    for (k = 0; k < 200; k++) {
        if (levels[k] > 2000) {
            break;  // we've found where the silence ends
        }
    }

    float startOfNonSilence_sec = (float)k/10.0;
    printf("peak value: %d, song start (sec): %f\n", peak, startOfNonSilence_sec);
    fflush(stdout);

    return(startOfNonSilence_sec);  // if it was all silence, 20.0 will be returned
}

// ------------------------------------------------------------------
float bass_audio::StreamCreate(const char *filepath)
{
    // finds non-silence
    float startOfNonSilence = songStartDetector(filepath);
    qDebug() << "non-silence detected @ " << startOfNonSilence << " seconds";

    BASS_StreamFree(Stream);

    // OPEN THE STREAM FOR PLAYBACK ------------------------
    Stream = BASS_StreamCreateFile(false, filepath, 0, 0,BASS_SAMPLE_FLOAT|BASS_STREAM_DECODE);
    Stream = BASS_FX_TempoCreate(Stream, BASS_FX_FREESOURCE);
    BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_VOL, (float)100.0/100.0f);
    BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_TEMPO, (float)0);
    StreamGetLength();

    BASS_BFX_PEAKEQ eq;

    // set peaking equalizer effect with no bands
    fxEQ = BASS_ChannelSetFX(Stream, BASS_FX_BFX_PEAKEQ, 0);

    float fGain = 0.0f;
    float fBandwidth = 2.5f;
    float fQ = 0.0f;
    float fCenter_Bass = 125.0f;
    float fCenter_Mid = 1000.0f;
    float fCenter_Treble = 8000.0f;

    eq.fGain = fGain;
    eq.fQ = fQ;
    eq.fBandwidth = fBandwidth;
    eq.lChannel = BASS_BFX_CHANALL;

    // create 1st band for bass
    eq.lBand = 0;
    eq.fCenter = fCenter_Bass;
    BASS_FXSetParameters(fxEQ, &eq);

    // create 2nd band for mid
    eq.lBand = 1;
    eq.fCenter = fCenter_Mid;
    BASS_FXSetParameters(fxEQ, &eq);

    // create 3rd band for treble
    eq.lBand=2;
    eq.fCenter=fCenter_Treble;
    BASS_FXSetParameters(fxEQ, &eq);

    bPaused = true;

    ClearLoop();

    // OPEN THE STREAM FOR BPM DETECTION -------------------------------------------
    float startSec1, endSec1;

    // look at a segment from T=10 to T=30sec
    if (FileLength > 30.0) {
        startSec1 = 10.0;
        endSec1 = 30.0;
    }
    else {
        // for very short songs, look at the whole song
        startSec1 = 0.0;
        endSec1 = FileLength;
    }

    unsigned int MINBPM = 100;
    unsigned int MAXBPM = 150;

    HSTREAM bpmChan;

    float bpmValue1 = 0;
    bpmChan = BASS_StreamCreateFile(FALSE, filepath, 0, 0, BASS_STREAM_DECODE);
    // detect bpm in background and return progress in GetBPM_ProgressCallback function
    if (bpmChan) {
        bpmValue1 = BASS_FX_BPM_DecodeGet(bpmChan,
                                          startSec1, endSec1,
                                          MAKELONG(MINBPM, MAXBPM),  // min/max BPM
                                          BASS_FX_FREESOURCE, // free handle when done
                                          0,0);
//        printf("DETECTED BPM = %5.2f\n", bpmValue1);
//        fflush(stdout);
    }
    else {
        bpmValue1 = 0.0;  // BPM not detectable
        printf("ERROR: BASS_StreamCreateFile()\n");
        fflush(stdout);
    }
    // free decode bpm stream and resources
    BASS_FX_BPM_Free(bpmChan);

    // ----------------------------------------------------------------
    // Now, make a decision as to whether we really know the BPM or not
    // NOTE: averaging from multiple places in the song doesn't work well.  It takes a while
    //   to stream midway into the song, and it's not any more reliable than looking at a section
    //   early in the song (which is quick, and reliable 98% of the time).
    Stream_BPM = bpmValue1;  // save original detected BPM

    // ALWAYS do channel processing
    gMono_dsp = BASS_ChannelSetDSP(Stream, &DSP_Mono, 0, 1);

    // when the fade is done, call a SYNCPROC that pauses playback
    DWORD handle = BASS_ChannelSetSync(Stream, BASS_SYNC_SLIDE, 0, MyFadeIsDoneProc, this);
    Q_UNUSED(handle)

    return(startOfNonSilence);
}

// ------------------------------------------------------------------
void bass_audio::StreamSetPosition(double Position_sec)
{
    BASS_ChannelSetPosition(Stream, BASS_ChannelSeconds2Bytes(Stream, (double)Position_sec), BASS_POS_BYTE);
//    Current_Position = Position_sec; // ??
}

// ------------------------------------------------------------------
void bass_audio::StreamGetLength(void)
{
    QWORD Length = BASS_ChannelGetLength(Stream, BASS_POS_BYTE);
    FileLength = (double)BASS_ChannelBytes2Seconds(Stream, Length);
}

// ------------------------------------------------------------------
void bass_audio::StreamGetPosition(void)
{
    QWORD Position = BASS_ChannelGetPosition(Stream, BASS_POS_BYTE);
    Current_Position = (double)BASS_ChannelBytes2Seconds(Stream, Position);
}

// ------------------------------------------------------------------
int bass_audio::StreamGetVuMeter(void)
{
    Stream_State = BASS_ChannelIsActive(Stream);

    if (Stream_State == BASS_ACTIVE_PLAYING) {
        int level;          // can return -1 for error
        DWORD left, right;
        level = BASS_ChannelGetLevel(Stream);
        if (level != -1) {
            left = LOWORD(level);   // the left level
            right = HIWORD(level);  // the right level
            return (left+right)/2;
        }
        else {
            // error in getting level
            return 0;
        }
    }
    else {
        return 0;
    }
}

// *******************
// http://bass.radio42.com/help/html/ee197eac-8482-1f9a-e0e1-8ec9e4feda9b.htm
void CALLBACK MySyncProc(HSYNC handle, DWORD channel, DWORD data, void *user)
{
    Q_UNUSED(data)
    Q_UNUSED(handle)
    QWORD endPoint_bytes = *((QWORD *)user);
    if (endPoint_bytes > 0) {
        BASS_ChannelSetPosition(channel, endPoint_bytes, BASS_POS_BYTE);
    }
}

// ------------------------------------------------------------------
void bass_audio::SetLoop(double fromPoint_sec, double toPoint_sec)
{
    ClearLoop(); // clear the existing loop (if one exists), so we don't end up with more than 1 at a time

//    qDebug() << "Loop from " << fromPoint_sec << " to " << toPoint_sec;
    loopFromPoint_sec = fromPoint_sec;
    loopToPoint_sec = toPoint_sec;

    startPoint_bytes = (QWORD)(BASS_ChannelSeconds2Bytes(Stream, fromPoint_sec));
    endPoint_bytes = (QWORD)(BASS_ChannelSeconds2Bytes(Stream, toPoint_sec));
    // TEST
    DWORD handle = BASS_ChannelSetSync(Stream, BASS_SYNC_POS, startPoint_bytes, MySyncProc, &endPoint_bytes);
    // Q_UNUSED(handle)
    syncHandle = (HSYNC)handle;  // save the handle
}

void bass_audio::ClearLoop()
{
//    qDebug() << "Loop points cleared";
    loopFromPoint_sec = loopToPoint_sec = 0.0;
    startPoint_bytes = endPoint_bytes = 0;

    if (syncHandle) {
        if (!BASS_ChannelRemoveSync(Stream, syncHandle)) {
            // qDebug() << "ERROR: BASS_ChannelRemoveSync()";  // FIX: revisit this later...
        }
        syncHandle = 0; // no sync set right now
    }
}

void bass_audio::SetMono(bool on)
{
    gStream_Mono = on;
}

// ------------------------------------------------------------------
void bass_audio::Play(void)
{
    // Check for current state
    bPaused = false;
    Stream_State = BASS_ChannelIsActive(Stream);
//    qDebug() << "current state: " << Stream_State << ", curPos: " << Current_Position;
    switch (Stream_State) {
        // Play
        case BASS_ACTIVE_STOPPED:
            // put the volume back where it was, now that we're paused.
//            qDebug() << "PLAY: Setting volume back to" << 1.0;
            BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_VOL, 1.0);  // ramp quickly to full volume

            // if stopped (initial load or reached EOS), start from
            //  wherever the currentposition is (set by the slider)
            BASS_ChannelPlay(Stream, false);
            StreamGetPosition();
            break;
        // Resume
        case BASS_ACTIVE_PAUSED:
            // put the volume back where it was, now that we're paused.
//            qDebug() << "RESUME: Setting volume back to" << 1.0;
            BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_VOL, 1.0);  // ramp quickly to full volume

            BASS_ChannelPlay(Stream, false);
            StreamGetPosition();
            break;
        // Pause
        case BASS_ACTIVE_PLAYING:
            BASS_ChannelPause(Stream);
            StreamGetPosition();
            bPaused = true;
            break;
        default:
            break;
    }
}

// ------------------------------------------------------------------
void bass_audio::Stop(void)
{
    BASS_ChannelPause(Stream);
    StreamSetPosition(0);
    bPaused = true;
}

void bass_audio::FadeOutAndPause(void) {
    Stream_State = BASS_ChannelIsActive(Stream);
    if (Stream_State == BASS_ACTIVE_PLAYING) {
//        qDebug() << "starting fade...current streamvolume is " << Stream_Volume;

        BASS_ChannelSlideAttribute(Stream, BASS_ATTRIB_VOL, 0.0, 3000);  // fade to 0 in 3s
    }
}

void bass_audio::StartVolumeDucking(int duckToPercent, float forSeconds) {
//    qDebug() << "Start volume ducking to: " << duckToPercent << " for " << forSeconds << " seconds...";

    BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_VOL, (float)duckToPercent/100.0); // drop Stream (main music stream) to a % of current volume

    QTimer::singleShot(forSeconds*1000.0, [=] {
        StopVolumeDucking();
    });

}

void bass_audio::StopVolumeDucking() {
//    qDebug() << "End volume ducking...";
    BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_VOL, 1.0); // drop Stream (main music stream) to a % of current volume
}


// ------------------------------------------------------------------
void bass_audio::PlayOrStopSoundEffect(int which, const char *filename, int volume) {
//    printf("SFXID: %d, which: %d\n", currentSoundEffectID, which);
//    printf("FXStream: %x, Active: %d\n\n", FXStream, BASS_ChannelIsActive(FXStream)==BASS_ACTIVE_PLAYING);
//    fflush(stdout);

    if (which == currentSoundEffectID &&
            FXStream != (HSTREAM)NULL &&
            BASS_ChannelIsActive(FXStream)==BASS_ACTIVE_PLAYING) {
        // if the user pressed the same key again, while playing back...
        BASS_ChannelStop(FXStream);  // stop the current stream
        currentSoundEffectID = 0;    // nothing playing now
        StopVolumeDucking();
        return;
    }

    // OPEN THE STREAM FOR PLAYBACK ------------------------
    if (FXStream != (HSTREAM)NULL) {
        BASS_StreamFree(FXStream);                                                  // clean up the old stream
    }
    FXStream = BASS_StreamCreateFile(false, filename, 0, 0, 0);
    BASS_ChannelSetAttribute(FXStream, BASS_ATTRIB_VOL, (float)volume/100.0f);  // volume relative to 100% of Music

    QWORD Length = BASS_ChannelGetLength(FXStream, BASS_POS_BYTE);
    double FXLength_seconds = (double)BASS_ChannelBytes2Seconds(FXStream, Length);

    StartVolumeDucking(20, FXLength_seconds);

    BASS_ChannelPlay(FXStream, true);                                           // play it all the way through
    currentSoundEffectID = which;
}

void bass_audio::StopAllSoundEffects() {
    if (FXStream != (HSTREAM)NULL &&
        BASS_ChannelIsActive(FXStream)==BASS_ACTIVE_PLAYING) {

        BASS_ChannelStop(FXStream);  // stop the current stream

        StopVolumeDucking();  // stop ducking of music, if it was in effect...

        currentSoundEffectID = 0;    // nothing playing now
    }
}
