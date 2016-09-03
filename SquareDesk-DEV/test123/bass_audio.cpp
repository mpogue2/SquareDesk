#include "bass_audio.h"
#include "bass_fx.h"
#include <stdio.h>
#include <QDebug>

// ------------------------------------------------------------------
bass_audio::bass_audio(void)
{
    Stream = (HSTREAM)NULL;
    Stream_State = (HSTREAM)NULL;
    Stream_Volume = 100; ///10000 (multipled on output)
    Stream_Tempo = 100;  // current tempo, relative to 100
    Stream_Pitch = 0;    // current pitch correction, in semitones; -5 .. 5
    Stream_BPM = 0.0;    // current estimated BPM (original song, without tempo correction)
    Stream_Pan = 0.0;

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
    Stream_Pan = newPan;
    BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_PAN, newPan);
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

//    Stream = BASS_StreamCreateFile(false, filepath, 0, 0, 0);
//    printf("filepath='%s'\n", filepath);

    // OPEN THE STREAM FOR BPM DETECTION ------------------------
    HSTREAM bpmChan;
    float bpmValue = 0;
    float startSec = 30.0;
    float endSec = 60.0;
    bpmChan = BASS_StreamCreateFile(FALSE, filepath, 0, 0, BASS_STREAM_DECODE);
    // detect bpm in background and return progress in GetBPM_ProgressCallback function
    if (bpmChan) {
        bpmValue = BASS_FX_BPM_DecodeGet(bpmChan,
                                         startSec, endSec,
//                                       0,  // min/max BPM = 29/200
                                         MAKELONG(110,140),  // min/max BPM
//                                         BASS_FX_BPM_BKGRND|BASS_FX_BPM_MULT2|BASS_FX_FREESOURCE,
                                         BASS_FX_FREESOURCE, // free handle when done
                                         0,0);
//                                                  (BPMPROGRESSPROC*)GetBPM_ProgressCallback, 0);
    }
    else {
        printf("ERROR: BASS_StreamCreateFile()\n");
    }
    // free decode bpm stream and resources
    BASS_FX_BPM_Free(bpmChan);

//    printf("BPM = %5.2f\n", bpmValue);
//    fflush(stdout);

    Stream_BPM = bpmValue;

    // OPEN THE STREAM FOR PLAYBACK ------------------------
    Stream = BASS_StreamCreateFile(false, filepath, 0, 0,BASS_SAMPLE_FLOAT|BASS_STREAM_DECODE);
//    Stream = BASS_FX_TempoCreate(Stream, BASS_SAMPLE_LOOP|BASS_FX_FREESOURCE);
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


// *******************
// http://bass.radio42.com/help/html/ee197eac-8482-1f9a-e0e1-8ec9e4feda9b.htm
// http://useyourloaf.com/blog/disabling-clang-compiler-warnings/
// #pragma clang diagnostic push
// #pragma clang diagnostic ignored "-Wunused-parameter"
void CALLBACK MySyncProc(HSYNC handle, DWORD channel, DWORD data, void *user)
{
    Q_UNUSED(data)
    Q_UNUSED(handle)
    QWORD endPoint_bytes = *((QWORD *)user);
    if (endPoint_bytes > 0) {
        BASS_ChannelSetPosition(channel, endPoint_bytes, BASS_POS_BYTE);
    }
}
// #pragma clang diagnostic pop

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

// ========================================================================
// Mix down to mono
// http://bass.radio42.com/help/html/b8b8a713-7af4-465e-a612-1acd769d4639.htm
// http://www.delphipraxis.net/141676-bass-dll-stereo-zu-mono-2.html
// also see the dsptest.c example (e.g. echo)

HDSP mono_dsp = 0;  // DSP handle (u32)

// http://useyourloaf.com/blog/disabling-clang-compiler-warnings/
// #pragma clang diagnostic push
// #pragma clang diagnostic ignored "-Wunused-parameter"
void CALLBACK DSP_Mono(HDSP handle, DWORD channel, void *buffer, DWORD length, void *user)
{
    Q_UNUSED(user)
    Q_UNUSED(channel)
    Q_UNUSED(handle)
        float *d = (float*)buffer;
        DWORD a;
        float mono;
        float l,r;

        for (a=0; a<length/4; a+=2) {
            l = d[a];
            r = d[a+1];
            mono = (l + r)/2.0;
            d[a] = d[a+1] = mono;
        }
}

// #pragma clang diagnostic pop

void bass_audio::SetMono(bool on) {
    if (on) {
        // enable Stereo -> Mono
        mono_dsp = BASS_ChannelSetDSP(Stream, &DSP_Mono, 0, 1);
    } else {
        // disable it (iff it was enabled)
        if (mono_dsp != 0) {
            BASS_ChannelRemoveDSP(Stream, mono_dsp);
        }
    }
}

// ------------------------------------------------------------------
void bass_audio::Play(void)
{
    //Check for current state
    bPaused = false;
    Stream_State = BASS_ChannelIsActive(Stream);
//    qDebug() << "current state: " << Stream_State << ", curPos: " << Current_Position;
    switch (Stream_State) {
        //Play
        case BASS_ACTIVE_STOPPED:
            // if stopped (initial load or reached EOS), start from
            //  wherever the currentposition is (set by the slider)
            BASS_ChannelPlay(Stream, false);
            StreamGetPosition();
            break;
//      //Resume
        case BASS_ACTIVE_PAUSED:
            BASS_ChannelPlay(Stream, false);
            StreamGetPosition();
            break;
        //Pause
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
