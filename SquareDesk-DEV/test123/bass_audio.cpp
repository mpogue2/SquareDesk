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

#include "bass_audio.h"
#include "bass_fx.h"
#include <math.h>
#include <stdio.h>
#include <QDebug>
//#include <QElapsedTimer>
#include <QTimer>

// GLOBALS =========
float  gStream_Pan = 0.0;
bool gStream_Mono = false;
HDSP gMono_dsp = 0;  // DSP handle (u32)
float gStream_replayGain = 1.0f;    // replayGain

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

    float *d = static_cast<float *>(buffer);
    DWORD a;
    float outL, outR;
    float inL,inR;
    float mono;

    const float PI_OVER_2 = 3.14159265f/2.0f;
    float theta = PI_OVER_2 * (gStream_Pan + 1.0f)/2.0f;  // convert to 0-PI/2
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
            mono = (outL + outR)/2.0f;  // mix down to mono for BOTH output channels
            d[a] = d[a+1] = mono * gStream_replayGain;
        }

    } else {
        // NORMAL STEREO mode
        for (a=0; a<length/4; a+=2)
        {
            inL = d[a];
            inR = d[a+1];
            outL = KL * inL;
            outR = KR * inR;  // constant power pan
            d[a] = outL * gStream_replayGain;
            d[a+1] = outR * gStream_replayGain;
        }
    }
}

// ------------------------------------------------------------------
bass_audio::bass_audio(void)
{
    Stream = (HSTREAM)NULL;
    FXStream = (HSTREAM)NULL;  // also initialize the FX stream

//    Stream_State = (HSTREAM)NULL;
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

    Global_IntelBoostEq[FREQ_KHZ] = 1.6;  // current Global EQ (one band for Intelligibility Boost)
    Global_IntelBoostEq[BW_OCT]  = 2.0;
    Global_IntelBoostEq[GAIN_DB] = 0.0;

    IntelBoostShouldBeEnabled = false;

    // compressor initial settings (defaults to OFF)
//    Stream_Compressor[0] = 0.0f;       // threshold (OFF)
//    Stream_Compressor[1] = 4.0f;       // ratio
//    Stream_Compressor[2] = 0.0f;       // gain
//    Stream_Compressor[3] = 10.0f;      // attack
//    Stream_Compressor[4] = 200.0f;     // release

#ifdef WANTCOMPRESSOR
    compressor.fThreshold = 0.0f;
    compressor.fRatio = 4.0f;
    compressor.fGain = 0.0f;
    compressor.fAttack = 10.0f;
    compressor.fRelease = 200.0f;
    compressor.lChannel = BASS_BFX_CHANALL;  // compress all channels

    compressorShouldBeEnabled = false;
#endif

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
//    qDebug() << "Setting new volume: " << inVolume;
    Stream_Volume = inVolume;
    BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, Stream_Volume * 100);  // this uses the GLOBAL volume control
}

// uses the STREAM volume, rather than global volume
void bass_audio::SetReplayGainVolume(double replayGain_dB) {
    if (!BASS_ChannelIsSliding(Stream, BASS_ATTRIB_VOL) &&
            (BASS_ChannelIsActive(FXStream) != BASS_ACTIVE_PLAYING) ) {
        // if we are not fading AND we are not ducked because of a sound effect
        //   (in both cases, when playback starts again, it will go to current volume w/ReplayGain)
        double voltageRatio = pow(10.0, replayGain_dB/20.0); // 0 = silent, 1.0 = normal, above 1.0 = amplification
        Stream_replayGain_dB = replayGain_dB;  // for later restore

//        voltageRatio = (replayGain_dB == 0.0 ? 0.1 : voltageRatio);  // DEBUG
        Stream_MaxVolume = voltageRatio;

//        qDebug() << "   setReplayGainVolume: " << replayGain_dB << "dB, Voltage ratio (max volume): " << voltageRatio;

        gStream_replayGain = static_cast<float>(voltageRatio);  // this is done in the DSP, because BASS_ATTRIB_VOL can't be > 1.0

//        if (!BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_VOL, static_cast<float>(voltageRatio))) {
//            qDebug() << "ERROR: ChannelSetAttribute to" << voltageRatio << "failed. ";
//            qDebug() << "   error code: " << BASS_ErrorGetCode();
//        }  // LOCAL volume control of just the music stream

//        float val;
//        BASS_ChannelGetAttribute(Stream, BASS_ATTRIB_VOL, &val);
//        qDebug() << "Channel GetAttribute Volume: " << val;
    }
}

// ------------------------------------------------------------------
void bass_audio::SetTempo(int newTempo)
{
//    qDebug() << "Setting new tempo: " << newTempo;
    Stream_Tempo = newTempo;
    BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_TEMPO, static_cast<float>(newTempo-100.0f)); // pass -10 to go 10% slower
}

// ------------------------------------------------------------------
void bass_audio::SetPitch(int newPitch)
{
//    qDebug() << "Setting new pitch: " << newPitch;
    Stream_Pitch = newPitch;
    BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_TEMPO_PITCH, static_cast<float>(newPitch));
}

// ------------------------------------------------------------------
void bass_audio::SetPan(double newPan)
{
    gStream_Pan = static_cast<float>(newPan);
//    BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_PAN, newPan);  // Panning/Mixing now done in MONO routine
}

// ------------------------------------------------------------------
// SetEq (band = 0,1,2), value = -15..15 (double)
void bass_audio::SetEq(int band, double val)
{
    Stream_Eq[band] = val;

    BASS_BFX_PEAKEQ eq;
    eq.lBand = band;    // get all values of the selected band
    bool success = BASS_FXGetParameters(fxEQ, &eq);
    if (!success) {
        qDebug() << "SetEq: BASS_FXGetParameters failed" << BASS_ErrorGetCode();
        return;
    }
//    qDebug() << "SetEq before: (" << eq.fCenter << eq.fBandwidth << eq.fGain << eq.fQ << eq.lBand << eq.lChannel << ")";

    eq.fGain = static_cast<float>(val);     // modify just the level of the selected band, and set all
    success = BASS_FXSetParameters(fxEQ, &eq);
    if (!success) {
        qDebug() << "SetEq: BASS_FXSetParameters failed" << BASS_ErrorGetCode();
        qDebug() << "tried to set: (" << eq.fCenter << eq.fBandwidth << eq.fGain << eq.fQ << eq.lBand << eq.lChannel << ")";
        return;
    }

//    qDebug() << "SetEq after: (" << eq.fCenter << eq.fBandwidth << eq.fGain << eq.fQ << eq.lBand << eq.lChannel << ")";
//    qDebug() << "success: " << success;
}

// ------------------------------------------------------------------
//
void bass_audio::SetCompression(unsigned int which, float val)
{

#ifdef WANTCOMPRESSOR
//    qDebug() << "SetCompression: " << which << "to: " << val;

    switch (which) {
        case 0: compressor.fThreshold = val; break;
        case 1: compressor.fRatio     = val; break;
        case 2: compressor.fGain      = val; break;
        case 3: compressor.fAttack    = val; break;
        case 4: compressor.fRelease   = val; break;
        default: break;
    }

    if (fxCompressor != (HFX)NULL) {
        // compressor exists, so go ahead and modify it!
        BASS_FXSetParameters(fxCompressor, &compressor);
//        qDebug() << "   Actual: " << compressor.fThreshold << compressor.fRatio << compressor.fGain << compressor.fAttack << compressor.fRelease;
    }
#else
    Q_UNUSED(which)
    Q_UNUSED(val)
#endif

}

void bass_audio::SetCompressionEnabled(bool enable) {

#ifdef WANTCOMPRESSOR
    if (enable) {
        // enabled
//        qDebug() << "compressor should be enabled...";

        if ((Stream != (HSTREAM)NULL) && (fxCompressor == (HFX)NULL)) {
            // compressor doesn't exist yet, so create one and initialize it
            // instantiate and init the compressor -----------------
            fxCompressor = BASS_ChannelSetFX(Stream, BASS_FX_BFX_COMPRESSOR2, 0);  // 0 = after EQ
            if (fxCompressor != (HFX)0) {
//                qDebug() << "   compressor is up and running";
                BASS_FXSetParameters(fxCompressor, &compressor);  // set parameters on compressor
            } else {
//                qDebug() << "error in turning on the compressor: " << BASS_ErrorGetCode();
            }
        }
        compressorShouldBeEnabled = true;
    } else {
        // disabled
//        qDebug() << "disabling compressor...";

        if ((Stream != (HSTREAM)NULL) && (fxCompressor != (HFX)NULL)) {
//            qDebug() << "   compressor is gone now...";
            BASS_ChannelRemoveFX(Stream, fxCompressor);
            fxCompressor = (HFX)NULL;  // compressor is gone now
        }
        compressorShouldBeEnabled = false;
    }
#else
    Q_UNUSED(enable)
#endif

}

// which = (FREQ_KHZ, BW_OCT, GAIN_DB)
void bass_audio::SetIntelBoost(unsigned int which, float val)
{
    Global_IntelBoostEq[which] = val;

    if (fxEQ == 0) {
        return;  // if EQ is not set up yet, don't bother...
    }

//    if (which == GAIN_DB) {
//        qDebug() << "SetIntelBoost: " << which << "to: " << -val << "dB";
//    } else {
//        qDebug() << "SetIntelBoost: " << which << "to: " << val;
//    }

    BASS_BFX_PEAKEQ eq;
    eq.lBand = 3;    // get all values of the global intelligibility EQ band
    bool success = BASS_FXGetParameters(fxEQ, &eq);

    if (!success) {
//        qDebug() << "SetIntelBoost: BASS_FXGetParameters failed" << BASS_ErrorGetCode();
        return;
    }

//    qDebug() << "before: (" << eq.fCenter << eq.fBandwidth << eq.fGain << eq.fQ << eq.lBand << eq.lChannel << ")";

    switch (which) {
        case FREQ_KHZ:  eq.fCenter    = val * 1000.0f;  // input val is KHz
                        break;
        case BW_OCT:    eq.fBandwidth = val;
                        break;
        case GAIN_DB:   eq.fGain      = (IntelBoostShouldBeEnabled ? -val : 0.0f);   // val is amount of suppression in dB, so use negative
                        break;
        default:
//                    qDebug() << "SetIntelBoost: bad value" << which << ", val = " << val;
                    return;  // don't set parameters, if bad
    }

//    if (!IntelBoostShouldBeEnabled && (which == GAIN_DB) && (val != 0.0f)) {
//        qDebug() << "Warning: GAIN_DB overridden to 0.0f, because IntelBoost not enabled.";
//    }

    success = BASS_FXSetParameters(fxEQ, &eq);

    if (!success) {
//        qDebug() << "SetIntelBoost: BASS_FXSetParameters failed" << BASS_ErrorGetCode();
//        qDebug() << "tried to set: (" << eq.fCenter << eq.fBandwidth << eq.fGain << eq.fQ << eq.lBand << eq.lChannel << ")";
        return;
    }

//    qDebug() << "success: " << success;
}

void bass_audio::SetIntelBoostEnabled(bool enable)
{
    IntelBoostShouldBeEnabled = enable;

    if (fxEQ == 0) {
//        qDebug() << "SetIntelBoostEnabled: fxEQ == 0";
        return;  // if EQ is not enabled yet, don't bother...
    }

    BASS_BFX_PEAKEQ eq;
    eq.lBand = 3;    // get all values of the global intelligibility EQ band
    bool success = BASS_FXGetParameters(fxEQ, &eq);

    if (!success) {
//        qDebug() << "SetIntelBoostEnabled: BASS_FXGetParameters failed" << BASS_ErrorGetCode();
        return;
    }

    if (enable) {
        eq.fGain = -Global_IntelBoostEq[GAIN_DB]; // restore to previously-saved gain
    } else {
        eq.fGain = 0.0;  // set to zero (NOTE: do NOT remember this value)
    }

    success = BASS_FXSetParameters(fxEQ, &eq);
    if (!success) {
//        qDebug() << "SetIntelBoostEnabled: BASS_FXSetParameters failed" << BASS_ErrorGetCode();
//        qDebug() << "tried to set: (" << eq.fCenter << eq.fBandwidth << eq.fGain << eq.fQ << eq.lBand << eq.lChannel << ")";
        return;
    } else {
//        qDebug() << "SetIntelBoostEnabled: " << enable << ", new gain = " << eq.fGain;
    }
}

void bass_audio::SetGlobals()
{
    // global EQ, like Inteliigibility Boost, can only be set AFTER the song is loaded.
    //   before that, there is no Stream to set EQ on.
    // So, call this from loadMP3File() after song is loaded, so that global EQ is set, too,
    //   from the Global_IntelBoostEq parameters.
//    qDebug() << "***** cBass.SetGlobals:";
    SetIntelBoost(FREQ_KHZ, Global_IntelBoostEq[FREQ_KHZ]);
    SetIntelBoost(BW_OCT, Global_IntelBoostEq[BW_OCT]);
    SetIntelBoost(GAIN_DB, Global_IntelBoostEq[GAIN_DB]);
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

        static_cast<bass_audio*>(user)->StreamGetPosition();
        static_cast<bass_audio*>(user)->bPaused = true;
    }
}

void bass_audio::songStartDetector(const char *filepath, double  *pSongStart, double  *pSongEnd) {
    // returns start of non-silence in seconds, max 20 seconds

    int peak = 0;
    int sampleNum = 0;

    int *levels = (int *)malloc((int)(FileLength * 10 + 10) * sizeof(int));  // allocate on the heap
    BASS_Init(0 /* "NO SOUND" device */, 44100, 0, 0, NULL);

    HSTREAM chan = BASS_StreamCreateFile(FALSE, filepath, 0, 0, BASS_STREAM_DECODE);
    if ( chan )
    {
        double  block = 20;  // take a 20ms level sample every 20ms

        // BASS_ChannelGetLevel takes 20ms from the channel
        QWORD len = BASS_ChannelSeconds2Bytes(chan, block/1000.0 - 0.02);  // always takes a 0.02s level sample

        //char data[len];  // data sink
        char *data = new char[len];
        DWORD level, left, right;

        int j = 0;
        int sum = 0;
        while ( (-1 != (int)(level = BASS_ChannelGetLevel(chan))) && (true || sampleNum < 200) ) // takes 20ms sample every 100ms for 20 sec
        {
            left=LOWORD(level); // the left level
            right=HIWORD(level); // the right level
            unsigned int avg = (left+right)/2;
            if (j++ < 4) {
                sum += avg;
            } else {
                levels[sampleNum] = sum/5;  // sum the 20ms contributions every 100ms, result is 100ms samples of energy
                if (true || sampleNum < 80) {
//                    printf("%d: %d\n", sampleNum, sum/5);
                }
                sampleNum++;
                peak = (peak < sum/5 ? sum/5 : peak);  // this finds the peak of the 100ms samples
                sum = 0;
                j = 0;
            }
            BASS_ChannelGetData(chan, data, len); // get data away from the channel
        }
        BASS_StreamFree( chan );
    }

    BASS_Free();

    // find START of song
    int k;
    for (k = 0; k < 200; k++) {
        if (levels[k] > 2500) {
            break;  // we've found where the silence ends
        }
    }
    double startOfSong_sec = k/10.0;
//    printf("Song start (sec): %f\n", startOfSong_sec);

    // find END of song
    for (k = sampleNum-1; k > 0; k--) {
        if (levels[k] > 1000) {
            break;  // we've found where the silence starts at the end of the song
        }
    }
    double endOfSong_sec = k/10.0;
//    printf("Song end (sec): %f\n", endOfSong_sec);
    fflush(stdout);

    free(levels);  // a tidy heap is a happy heap

    // return values:
    *pSongStart = startOfSong_sec;
    *pSongEnd = endOfSong_sec;
}

// ------------------------------------------------------------------
void bass_audio::StreamCreate(const char *filepath, double  *pSongStart_sec, double  *pSongEnd_sec, double intro1_frac, double outro1_frac)
{
    Q_UNUSED(intro1_frac)
    Q_UNUSED(outro1_frac)

    if (Stream != (HSTREAM)NULL) {
        // if there is a valid stream,
        if (fxEQ != (HFX)NULL) {
            // and there's a valid EQ processor
            BASS_ChannelRemoveFX(Stream, fxEQ);  // remove the EQ
            fxEQ = (HFX)NULL;
        }
#ifdef WANTCOMPRESSOR
        if (fxCompressor != (HFX)NULL) {
            // and there's a valid compressor
            BASS_ChannelRemoveFX(Stream, fxCompressor);  // remove the compressor
            fxCompressor = (HFX)NULL;  // other code looks at this to determine whether a compressor exists
        }
#endif

    }
    BASS_StreamFree(Stream);  // free the old stream

    // OPEN THE STREAM FOR PLAYBACK ------------------------
    Stream = BASS_StreamCreateFile(false, filepath, 0, 0,BASS_SAMPLE_FLOAT|BASS_STREAM_DECODE);
    Stream = BASS_FX_TempoCreate(Stream, BASS_FX_FREESOURCE);
//    BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_VOL, 100.0f/100.0f);  // now set by ReplayGain below...
    BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_TEMPO, 0.0f);
    StreamGetLength(); // sets FileLength

    // finds song start and end points ------------
//    QElapsedTimer t3;
//    t3.start();
//    if (intro1_frac != 0.0 || outro1_frac != 0.0) {
//        qDebug() << "Not running songStartDetector, so using: " << intro1_frac << ", " << outro1_frac;
    *pSongStart_sec = 0.0;
    *pSongEnd_sec   = FileLength;
//    } else {
        // both fractions (0-1.0) == zero means we haven't figured out the song length yet
        //   so, figure it out now
//        songStartDetector(filepath, pSongStart_sec, pSongEnd_sec);
//    }
//    qDebug() << "t3: " << t3.elapsed();
    // ------------------------------

    BASS_BFX_PEAKEQ eq;

    // set peaking equalizer effect with no bands -----------------
    fxEQ = BASS_ChannelSetFX(Stream, BASS_FX_BFX_PEAKEQ, 1);

    float fGain = 0.0f;
    float fBandwidth = 2.5f;
    float fQ = 0.0f;  // unnecessary, because non-zero fBandwidth overrides fQ
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
    eq.lBand = 2;
    eq.fCenter = fCenter_Treble;
    BASS_FXSetParameters(fxEQ, &eq);

    // create 4th band for global Intelligibility Boost (initial gain = 0.0)
    eq.lBand = 3;
    eq.fCenter = 1600.0f;
    eq.fBandwidth = 2.0f;
    BASS_FXSetParameters(fxEQ, &eq);

#ifdef WANTCOMPRESSOR
    // instantiate and init the compressor, if it's not already -----------------
    SetCompressionEnabled(compressorShouldBeEnabled);
#endif

    // -------------------------------------------------------------
    bPaused = true;

    ClearLoop();

    // OPEN THE STREAM FOR BPM DETECTION -------------------------------------------
    double startSec1, endSec1;

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

    double bpmValue1 = 0;
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

    SetReplayGainVolume(0.0);  // initialize the replayGain to "disabled", sets the LOCAL volume
}

// ------------------------------------------------------------------
bool bass_audio::isPaused(void)
{
    return(bPaused);
}

// ------------------------------------------------------------------
void bass_audio::StreamSetPosition(double Position_sec)
{
    BASS_ChannelSetPosition(Stream, BASS_ChannelSeconds2Bytes(Stream, Position_sec), BASS_POS_BYTE);
//    Current_Position = Position_sec; // ??
}

// ------------------------------------------------------------------
void bass_audio::StreamGetLength(void)
{
    QWORD Length = BASS_ChannelGetLength(Stream, BASS_POS_BYTE);
    FileLength = BASS_ChannelBytes2Seconds(Stream, Length);
}

// ------------------------------------------------------------------
void bass_audio::StreamGetPosition(void)
{
    QWORD Position = BASS_ChannelGetPosition(Stream, BASS_POS_BYTE);
    Current_Position = BASS_ChannelBytes2Seconds(Stream, Position);
}

// always asks the engine what the state is (NOT CACHED), then returns one of:
//    BASS_ACTIVE_STOPPED, BASS_ACTIVE_PLAYING, BASS_ACTIVE_STALLED, BASS_ACTIVE_PAUSED
uint32_t bass_audio::currentStreamState() {
    DWORD Stream_State = BASS_ChannelIsActive(Stream);
    return(static_cast<uint32_t>(Stream_State));
}

// ------------------------------------------------------------------
int bass_audio::StreamGetVuMeter(void)
{
    uint32_t Stream_State = currentStreamState();

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
    QWORD endPoint_bytes = *(static_cast<QWORD *>(user));
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

    startPoint_bytes = BASS_ChannelSeconds2Bytes(Stream, fromPoint_sec);
    endPoint_bytes = BASS_ChannelSeconds2Bytes(Stream, toPoint_sec);
    // TEST
    HSYNC handle = BASS_ChannelSetSync(Stream, BASS_SYNC_POS, startPoint_bytes, MySyncProc, &endPoint_bytes);
    // Q_UNUSED(handle)
    syncHandle = handle;  // save the handle
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
    bPaused = false;
//    BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_VOL, 1.0);  // ramp quickly to full volume
//    qDebug() << "   Play: Play volume set to: " << Stream_MaxVolume;
    BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_VOL, Stream_MaxVolume);  // ramp quickly to full volume (with ReplayGain applied)
    BASS_ChannelPlay(Stream, false);
    StreamGetPosition();  // tell the position bar in main window where we are
}

// ------------------------------------------------------------------
void bass_audio::Stop(void)
{
    BASS_ChannelPause(Stream);
    StreamSetPosition(0);
    StreamGetPosition();  // tell the position bar in main window where we are
    bPaused = true;
}

// ------------------------------------------------------------------
void bass_audio::Pause(void)
{
    BASS_ChannelPause(Stream);
    StreamGetPosition();  // tell the position bar in main window where we are
    bPaused = true;
}


void bass_audio::FadeOutAndPause(void) {
//    Stream_State = BASS_ChannelIsActive(Stream);
    uint32_t Stream_State = currentStreamState();
    if (Stream_State == BASS_ACTIVE_PLAYING) {
//        qDebug() << "starting fade...current streamvolume is " << Stream_Volume;

        BASS_ChannelSlideAttribute(Stream, BASS_ATTRIB_VOL, 0.0, 6000);  // fade to 0 in 6s
    }
}

void bass_audio::StartVolumeDucking(int duckToPercent, double forSeconds) {
//    qDebug() << "Start volume ducking to: " << duckToPercent << " for " << forSeconds << " seconds...";
    qDebug() << "StartVolumeDucking volume set to: " << static_cast<float>(Stream_MaxVolume * duckToPercent)/100.0f;

    BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_VOL, static_cast<float>(Stream_MaxVolume * duckToPercent)/100.0f); // drop Stream (main music stream) to a % of current max volume (w/ReplayGain)

    QTimer::singleShot(forSeconds*1000.0, [=] {
        StopVolumeDucking();
    });

}

void bass_audio::StopVolumeDucking() {
//    BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_VOL, 1.0); // drop Stream (main music stream) to a % of current volume
    qDebug() << "End volume ducking, vol set to: " << Stream_MaxVolume;
    BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_VOL, Stream_MaxVolume); // Stream (main music stream) to 100% of current volume (w/ReplayGain)
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
    qDebug() << "Sound FX stream vol set to: " << static_cast<float>(Stream_MaxVolume * volume)/100.0f;
    BASS_ChannelSetAttribute(FXStream, BASS_ATTRIB_VOL, static_cast<float>(Stream_MaxVolume * volume)/100.0f);  // volume relative to 100% of Music (w/ReplayGain)

    QWORD Length = BASS_ChannelGetLength(FXStream, BASS_POS_BYTE);
    double FXLength_seconds = BASS_ChannelBytes2Seconds(FXStream, Length);

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
