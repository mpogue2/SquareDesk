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

    const float PI_OVER_2 = 3.14159265/2.0;
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
}

// ------------------------------------------------------------------
bass_audio::~bass_audio(void)
{
}

// ------------------------------------------------------------------
void bass_audio::Init(void)
{
    //-------------------------------------------------------------
    BASS_Init(-1, 44100, 0, NULL, NULL);
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

// ------------------------------------------------------------------
void bass_audio::StreamCreate(const char *filepath)
{
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
}

// ------------------------------------------------------------------
void bass_audio::StreamSetPosition(double Position)
{
    BASS_ChannelSetPosition(Stream, BASS_ChannelSeconds2Bytes(Stream, (double)Position), BASS_POS_BYTE);
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
    loopFromPoint_sec = fromPoint_sec;
    loopToPoint_sec = toPoint_sec;

    startPoint_bytes = (QWORD)(BASS_ChannelSeconds2Bytes(Stream, fromPoint_sec));
    endPoint_bytes = (QWORD)(BASS_ChannelSeconds2Bytes(Stream, toPoint_sec));
    // TEST
    DWORD handle = BASS_ChannelSetSync(Stream, BASS_SYNC_POS, startPoint_bytes, MySyncProc, &endPoint_bytes);
    Q_UNUSED(handle)
}

void bass_audio::ClearLoop()
{
    loopFromPoint_sec = loopToPoint_sec = 0.0;
    startPoint_bytes = endPoint_bytes = 0;
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
            // if stopped (initial load or reached EOS), start from
            //  wherever the currentposition is (set by the slider)
            BASS_ChannelPlay(Stream, false);
            StreamGetPosition();
            break;
        // Resume
        case BASS_ACTIVE_PAUSED:
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

// ------------------------------------------------------------------
void bass_audio::PlaySoundEffect(const char *filename, int volume) {
    // OPEN THE STREAM FOR PLAYBACK ------------------------
    if (FXStream != (HSTREAM)NULL) {
        BASS_StreamFree(FXStream);                                                  // clean up the old stream
    }
    FXStream = BASS_StreamCreateFile(false, filename, 0, 0, 0);
    BASS_ChannelSetAttribute(FXStream, BASS_ATTRIB_VOL, (float)volume/100.0f);  // volume relative to 100% of Music
    BASS_ChannelPlay(FXStream, true);                                           // play it all the way through
}
