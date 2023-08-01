/****************************************************************************
**
** Copyright (C) 2016-2023 Mike Pogue, Dan Lyke
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

#include "audiodecoder.h"
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <stdio.h>
#if defined(Q_OS_LINUX)
#include <QtMultimedia/QMediaDevices>
#include <QtMultimedia/QAudioDevice>
#else /* end of Q_OS_LINUX */
#include <QMediaDevices>
#include <QAudioDevice>
#endif /* else if defined Q_OS_LINUX */
#include <QElapsedTimer>
#include <QMessageBox>
#include <QProcess>
#include <QThread>
#include <QTemporaryFile>

// EQ ----------
#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>
#include <kfr/math.hpp>
#include <kfr/dsp/biquad.hpp>

using namespace kfr;

// BPM Estimation ==========

// SoundTouch BPM estimation ----------
// #include "BPMDetect.h"

// BreakfastQuay BPM detection --------
#include "MiniBpm.h"
using namespace breakfastquay;

// PITCH/TEMPO ==============
// SoundTouch BPM detection and pitch/tempo changing --------
#include "SoundTouch.h"

// BEAT/BAR DETECTION =======
#include "wav_file.h"

#include "perftimer.h"

// Processing chunk size (size chosen to be divisible by 2, 4, 6, etc. channels ...)
#define SOUNDTOUCH_BUFF_SIZE           6720
using namespace soundtouch;

QElapsedTimer timer1;

// TODO: VU METER (kfr) ********
// TODO: SOUND EFFECTS (later) ********
// TODO: allow changing the output device (new feature!) *****

// ===========================================================================
class PlayerThread : public QThread
{
    //Q_OBJECT

public:
    PlayerThread()  {
        playPosition_frames = 0;
        activelyPlaying = false;
        currentState = BASS_ACTIVE_STOPPED;
        threadDone = false;
        m_volume = 100;
        bytesPerFrame = 0;
        sampleRate = 0;

        currentFadeFactor = 1.0;
        fadeFactorDecrementPerFrame = 0.0;

        StopVolumeDucking();

        clearLoop();

        // EQ settings
        bassBoost_dB   =  0.0;  // +/-15dB
        midBoost_dB    =  0.0;  // +/-15dB
        trebleBoost_dB =  0.0;  // +/-15dB

        intelligibilityBoost_fKHz = 1.6;
        intelligibilityBoost_widthOctaves = 2.0;
        intelligibilityBoost_dB = 0.0;  // also turns this off
        intelligibilityBoost_enabled = false;

        updateEQ();  // update the bq[4], based on the current *Boost_* settings

        // PITCH/TEMPO ---------
        tempoSpeedup_percent = 0.0;
        pitchUp_semitones = 0.0;

        soundTouch.setSampleRate(44100);
        soundTouch.setChannels(2);  // we are already setup for stereo processing, just don't copy-to-mono.

        soundTouch.setTempoChange(tempoSpeedup_percent);  // this is in PERCENT
        soundTouch.setPitchSemiTones(pitchUp_semitones); // this is in SEMITONES
        soundTouch.setRateChange(0.0);   // this is in PERCENT (always ZERO, because no sample rate change

        soundTouch.setSetting(SETTING_USE_QUICKSEEK, false);  // NO QUICKSEEK (better quality)
        soundTouch.setSetting(SETTING_USE_AA_FILTER, true);   // USE AA FILTER (better quality)

        m_peakLevelL_mono = 0.0;
        m_peakLevelR      = 0.0;
        m_resetPeakDetector = true;
    }

    virtual ~PlayerThread() {
        threadDone = true;

        // from: https://stackoverflow.com/questions/28660852/qt-qthread-destroyed-while-thread-is-still-running-during-closing
        if(!this->wait(300))    // Wait until it actually has terminated (max. 300 msec)
        {
//            qDebug() << "force terminating the PlayerThread";
            this->terminate();  // Thread didn't exit in time, probably deadlocked, terminate it!
            this->wait();       // We have to wait again here!
        }
    }

    // SOUNDTOUCH LOOP
    void run() override {
        while (!threadDone) {
//            unsigned int bytesFree = m_audioSink->bytesFree();  // the audioSink has room to accept this many bytes
            unsigned int bytesFree = 0;
            if ( activelyPlaying && (m_audioSink) && (bytesFree = m_audioSink->bytesFree()) > 0) {  // let's be careful not to dereference a null pointer
                unsigned int framesFree = bytesFree/bytesPerFrame;  // One frame = LR
                if (framesFree > 100) {
                    // if we are actively playing, AND there are less than 100 frames (2.25ms) available in the m_audioSink, let's wait
                    //   5ms and try again.  This should get around the (currently theoretical) problem where bytesFree > 0, but
                    //   framesFree == 0, which I think might be messing up our straddlingLoopFromPoint calculation below,
                    //   and causing the loop back to the start loop point to be missed.

//                    qDebug() << "framesFree OK:" << framesFree; // most of the time it's 512 LR frames = 11.6ms

                    const char *p_data = (const char *)(m_data) + (bytesPerFrame * playPosition_frames);  // next samples to play

                    // write the smaller of bytesFree and how much we have left in the song
                    unsigned int bytesNeededToWrite = bytesFree;  // default is to write all we can
                    if (bytesPerFrame * (totalFramesInSong - playPosition_frames) < bytesFree) {
                        // but if the song ends sooner than that, just send the last samples in the song
                        bytesNeededToWrite = bytesPerFrame * (totalFramesInSong - playPosition_frames);
                    }

                    unsigned int loopStartpoint_frames = 44100 * loopTo_sec;
                    unsigned int loopEndpoint_frames = 44100 * loopFrom_sec;
                    bool straddlingLoopFromPoint =
                            !(loopFrom_sec == 0.0 && loopTo_sec == 0.0) &&
                            (playPosition_frames < loopEndpoint_frames) &&
                            (playPosition_frames + framesFree) >= loopEndpoint_frames;

//                    qDebug() << "h: " << straddlingLoopFromPoint << playPosition_frames << loopEndpoint_frames << framesFree;

                    if (straddlingLoopFromPoint) {
                        // but if the song loops here, just send the frames up to the loop point
                        bytesNeededToWrite = bytesPerFrame * (loopEndpoint_frames - playPosition_frames);
                    }

                    if (bytesNeededToWrite > 0) {
                        // if we need bytes, let's go get them.  BUT, if processDSP only gives us less than that, then write just those.
                        numProcessedFrames = 0;
                        sourceFramesConsumed = 0;
                        processDSP(p_data, bytesNeededToWrite);  // processes 8-byte-per-frame stereo to 8-byte-per-frame *processedData (dual mono)

                        int excessFramesPlayed = playPosition_frames - loopEndpoint_frames;
                        bool weSnuckPastTheLoopEndpoint = !(loopFrom_sec == 0.0 && loopTo_sec == 0.0) &&  // loop is NOT disabled
                                                          (playPosition_frames >= loopEndpoint_frames);   //  AND we got beyond the endpoint

                        if (straddlingLoopFromPoint || weSnuckPastTheLoopEndpoint) {

                            playPosition_frames = loopStartpoint_frames; // reset the playPosition to the loop start point, in both cases

                            if (weSnuckPastTheLoopEndpoint) {
                                // This can happen when the playPosition exactly equals or is slightly larger than the loopEndpoint, which can happen
                                //   when the tempo control is faster than normal, and so the number of frames consumed is just larger
                                //   that the number of frames free.
                                // I think this is the simplest possible solution, since the DSP has in_frames != out_frames.
                                // If we snuck past, then we need to move the play position later that the loop start by the number of frames
                                //   by which we went past the end point.  That should allow us to still be frame accurate.
//                                qDebug() << "***** WARNING: We snuck past the Loop endpoint: " << playPosition_frames << loopEndpoint_frames << loopFrom_sec << loopTo_sec;
                                playPosition_frames += excessFramesPlayed;
//                                qDebug() << "   New playPosition_frames: " << excessFramesPlayed << playPosition_frames << loopStartpoint_frames;
                            }
                        } else {
                            playPosition_frames += sourceFramesConsumed; // move the data pointer to the next place to read from
                                                                          // if at the end, this will point just beyond the last sample
                                                                          // these were consumed (sent to soundTouch) in any case.
//                            qDebug() << "consumed: " << sourceFramesConsumed;
                        }
                        if (numProcessedFrames > 0) {  // but, maybe we didn't get any back from soundTouch.
                            m_audioDevice->write((const char *)&processedData, bytesPerFrame * numProcessedFrames);  // DSP processed audio is 8 bytes/frame floats
                        }
                    } else {
                        Stop(); // we reached the end, so reset the player back to the beginning (disable the writing, move playback position to 0)
                    }
                } else {
//                    qDebug() << "***** framesFree was small: " << framesFree;
                }
            } // activelyPlaying
            msleep((activelyPlaying ? 5 : 50)); // sleep 10 milliseconds, this sets the polling rate for writing bytes to the audioSink  // CHECK THIS!
        } // while
    }

    // ---------------------
    void Play() {
//        qDebug() << "PlayerThread::Play";

        // flush the state ------
        if (filter != NULL) {
            filter->reset();
        }
        if (filterR != NULL) {
            filterR->reset();
        }
        activelyPlaying = true;
        currentState = BASS_ACTIVE_PLAYING;

        // Play cancels FadeAndPause ------
        currentFadeFactor = 1.0;
        fadeFactorDecrementPerFrame = 0.0;

    }

    void Stop() {
//        qDebug() << "PlayerThread::Stop";
        activelyPlaying = false;
        clearSoundTouch = true;  // at next opportunity, flush all the soundTouch buffers, because we're stopped now.
        playPosition_frames = 0;
        currentState = BASS_ACTIVE_STOPPED;

        // flush the state ------
        if (filter != NULL) {
            filter->reset();
        }
        if (filterR != NULL) {
            filterR->reset();
        }

        // Stop cancels FadeAndPause ------
        currentFadeFactor = 1.0;
        fadeFactorDecrementPerFrame = 0.0;
    }

    void Pause() {
//        qDebug() << "PlayerThread::Pause";
        activelyPlaying = false;
        currentState = BASS_ACTIVE_PAUSED;

        // Pause cancels FadeAndPause ------
        currentFadeFactor = 1.0;
        fadeFactorDecrementPerFrame = 0.0;
    }

    // ---------------------
    void setVolume(unsigned int vol) {
        m_volume = vol;
    }

    unsigned int getVolume() {
        return(m_volume);
    }

    void fadeOutAndPause(float finalVol, float secondsToGetThere) {
        Q_UNUSED(finalVol)
        currentFadeFactor           = 1.0;  // starts at 1.0 * m_volume, goes to 0.0 * m_volume
        fadeFactorDecrementPerFrame = 1.0 / (secondsToGetThere * sampleRate);  // when non-zero, starts fading.  when 0.0, pauses and restores fade factor to 1.0
    }

    void fadeComplete() {
        Pause();                            // pause (which also explicitly cancels and reinits fadeFactor and decrement)
    }

    void StartVolumeDucking(float duckToPercent, float forSeconds) {
        currentDuckingFactor = ((float)duckToPercent)/100.0;
        duckingFactorFramesRemaining = forSeconds * sampleRate;  // FIX: really FRAME rate
    }

    void StopVolumeDucking() {
        currentDuckingFactor = 1.0;
        duckingFactorFramesRemaining = 0.0;  // FIX: really FRAME rate
    }

    // ---------------------
    void setPan(double pan) {
        m_pan = pan;
    }

    // ---------------------
    void setMono(bool on) {
//        qDebug() << "PlayerThread::setMono" << on;
        m_mono = on;
    }

    void setStreamPosition(double p) {
        playPosition_frames = (unsigned int)(sampleRate * p); // this should be atomic
    }

    double getStreamPosition() {
        return((double)playPosition_frames/(double)sampleRate);  // returns current position in seconds as double
    }

    unsigned char getCurrentState() {
        return(currentState);  // returns current state as int {0,3}
    }

    double getPeakLevelL_mono() {
        m_resetPeakDetector = true;  // we've used it, so start a new accumulation
        return(m_peakLevelL_mono);   // for VU meter, calculated when music is playing in DSP
    }

    double getPeakLevelR() { // ALWAYS CALL THIS FIRST, BECAUSE IT WILL NOT RESET THE PEAK DETECTOR
        // m_resetPeakDetector = true;  // we've used it, so start a new accumulation
        return(m_peakLevelR);           // for VU meter, calculated when music is playing in DSP
    }

    void setLoop(double from_sec, double to_sec) {
        loopFrom_sec = from_sec;
        loopTo_sec   = to_sec;
    }

    void clearLoop() {
        loopFrom_sec = loopTo_sec = 0.0;
    }

    // EQ --------------------------------------------------------------------------------
    void updateEQ() {
        // given bassBoost_dB, etc., recreate the bq's.
        bq[0] = biquad_peak( 125.0/44100.0,  4.0,   bassBoost_dB);    // tweaked Q to match Intel version
        bq[1] = biquad_peak(1000.0/44100.0,  0.9,   midBoost_dB);     // tweaked Q to match Intel version
        bq[2] = biquad_peak(8000.0/44100.0,  0.9,   trebleBoost_dB);  // tweaked Q to match Intel version

        float W0 = 2 * 3.14159265 * (1000.0 * intelligibilityBoost_fKHz/sampleRate);
        float Q = 1/(2.0 * sinhf( (logf(2.0)/2.0) * intelligibilityBoost_widthOctaves * (W0/sinf(W0)) ) );
//        qDebug() << "Q: " << Q << ", boost_dB: " << intelligibilityBoost_dB;
        bq[3] = biquad_peak(1000.0 * intelligibilityBoost_fKHz/44100.0, Q, intelligibilityBoost_dB);

        newFilterNeeded = true;
//        qDebug() << "\tbiquad's are updated: ("<< bassBoost_dB << midBoost_dB << trebleBoost_dB << intelligibilityBoost_dB << ")";
    }

    void setBassBoost(double b) {
//        qDebug() << "new BASS EQ value: " << b;
        bassBoost_dB = b;
        updateEQ();
    }

    void setMidBoost(double m) {
//        qDebug() << "new MID EQ value: " << m;
        midBoost_dB = m;
        updateEQ();
    }

    void setTrebleBoost(double t) {
//        qDebug() << "new TREBLE EQ value: " << t;
        trebleBoost_dB = t;
        updateEQ();
    }

    void SetIntelBoost(unsigned int which, float val) {
//        qDebug() << "INTEL BOOST: (" << which << ", " << val << ")";

        switch (which) {
            case 0: intelligibilityBoost_fKHz            = val;  break;
            case 1: intelligibilityBoost_widthOctaves    = val;  break;
            case 2: intelligibilityBoost_dB              = -val; break; // NOTE MINUS SIGN (control is positive, but Boost is negative (suppression)
            default:
//                qDebug() << "ERROR: UNKNOWN INTEL BOOST: (" << which << ", " << val << ")";
                break;
        }
        updateEQ();
    }

    void SetIntelBoostEnabled(bool enable) {
//        qDebug() << "INTEL BOOST ENABLED: " << enable;
        intelligibilityBoost_enabled = enable;
        updateEQ();
    }

    void SetPanEQVolumeCompensation(float val) { // val is signed dB
        currentPanEQFactor = pow(10.0, val/20.0);
//        qDebug() << "AudioDecoder::setPanEQVolumeCompensation:" << val << currentPanEQFactor;
    }

    void setPitch(float newPitchSemitones) {
//        qDebug() << "new Pitch value: " << newPitchSemitones << " semitones";
        soundTouch.setPitchSemiTones(newPitchSemitones);
    }

    void setTempo(float newTempoPercent) {
//        qDebug() << "new Tempo value: " << newTempoPercent;
        soundTouch.setTempo(newTempoPercent/100.0);
    }

    // ================================================================================
    unsigned int processDSP(const char *inData, unsigned int inLength_bytes) {

        // INS and OUTS ----------
        const float *inDataFloat   = (const float *)inData;     // input is stereo float interleaved (8 bytes per frame)
        float *outDataFloat  = (float *)(&processedData);       // final output is stereo float interleaved (8 bytes per frame), also used as intermediate
        float *outDataFloatR  = (float *)(&processedDataR);     // intermediate output is R channel mono float (4 bytes per frame)

        const unsigned int inLength_frames = inLength_bytes/bytesPerFrame;  // pre-mixdown frames are 8 bytes

        // So, the AudioSink can take X frames, but after stretch/squish, we will have processed Y = K * X frames.
        //    scaled_inLength_frames is how many frames we need to process, so that we get the request inLength_frames output (which
        //    is what the AudioSink needs.  We have to scale the number of samples for BOTH the EQ processing (which has state)
        //    and the SoundTouch processing (which also has state).
        double inOutRatio = soundTouch.getInputOutputSampleRatio();
        double scaled_inLength_frames = floor(((double)inLength_frames) / inOutRatio);
//        qDebug() << "inOutRatio:" << inOutRatio;

        // PAN/VOLUME/FORCE MONO/EQ --------
        const float PI_OVER_2 = 3.14159265f/2.0f;
        float theta = PI_OVER_2 * (m_pan + 1.0f)/2.0f;  // convert to 0-PI/2
        float KL = cos(theta);
        float KR = sin(theta);
//        qDebug() << "KL/R: " << KL << KR;
        float scaleFactor;  // volume and mono scaling

        // EQ -----------
        // if the EQ has changed, make a new filter, but do it here only, just before it's used,
        //   to avoid crashing the thread when EQ is changed.
        if (newFilterNeeded) {
//            qDebug() << "making a new filter...";
            if (filter != NULL) {
                delete filter;
            }

            size_t biquadCount = (intelligibilityBoost_enabled ? 4 : 3); // the intelligibility boost must always be the 4th biquad_params

            filter = new biquad_filter<float>(bq, biquadCount);  // (re)initialize the mono/L filter from the latest bq coefficients
            if (filterR != NULL) {
                delete filterR;
            }
            filterR = new biquad_filter<float>(bq, biquadCount);  // (re)initialize the R filter from the latest bq coefficients
            newFilterNeeded = false;
        }

        float thePeakLevelL_mono = 0.0;
        float thePeakLevelR = 0.0;
        if (m_mono) {
            // Force Mono is ENABLED
            scaleFactor = currentPanEQFactor * currentDuckingFactor * currentFadeFactor * m_volume / (100.0 * 2.0);  // divide by 2, to avoid overflow; fade factor goes from 1.0 -> 0.0
//    qDebug() << "scaleFactor: " << scaleFactor;
            for (int i = 0; i < (int)scaled_inLength_frames; i++) {
                // output data is MONO (de-interleaved)
                outDataFloat[i] = (float)(scaleFactor*KL*inDataFloat[2*i] + scaleFactor*KR*inDataFloat[2*i+1]); // stereo to 2ch mono + volume + pan
            }

//            if (outDataFloat[0] != 0.0) {
//                for (int i = 0; i < 40; i++) { // DEBUG DEBUG DEBUG
//                    qDebug() << "B-outDataFloat[" << i << "]: " << outDataFloat[i];
//                }
//            }

            // APPLY EQ TO MONO (4 biquads, including B/M/T and Intelligibility Boost) ----------------------
            filter->apply(outDataFloat, scaled_inLength_frames);   // NOTE: applies IN PLACE

//            if (outDataFloat[0] != 0.0) {
//                for (int i = 0; i < 40; i++) { // DEBUG DEBUG DEBUG
//                    qDebug() << "A-outDataFloat[" << i << "]: " << outDataFloat[i];
//                }
//            }
            // output data is INTERLEAVED DUAL MONO (Stereo with L and R identical) -- re-interleave to the outDataFloat buffer (which is the final output buffer)
            for (int i = scaled_inLength_frames-1; i >= 0; i--) {
                outDataFloat[2*i] = outDataFloat[2*i+1] = max(min(1.0f, outDataFloat[i]),-1.0);  // hard limiter
                thePeakLevelL_mono = fmaxf(thePeakLevelL_mono, outDataFloat[2*i]);
            }
        } else {
            // stereo (Force Mono is DISABLED)
            scaleFactor = currentPanEQFactor * currentDuckingFactor * currentFadeFactor * m_volume / (100.0);
            for (unsigned int i = 0; i < scaled_inLength_frames; i++) {
                // output data is de-interleaved into first and second half of outDataFloat buffer
//                outDataFloat[2*i]   = (float)(scaleFactor*KL*inDataFloat[2*i]);   // L: stereo to 2ch stereo + volume + pan
//                outDataFloat[2*i+1] = (float)(scaleFactor*KR*inDataFloat[2*i+1]); // R: stereo to 2ch stereo + volume + pan
                outDataFloat[i]  = (float)(scaleFactor*KL*inDataFloat[2*i]);   // L:
                outDataFloatR[i] = (float)(scaleFactor*KR*inDataFloat[2*i+1]); // R:
            }
            // APPLY EQ TO EACH CHANNEL SEPARATELY (4 biquads, including B/M/T and Intelligibility Boost) ----------------------
            filter->apply(outDataFloat,   scaled_inLength_frames);    // NOTE: applies L filter IN PLACE (filter and storage used for mono and L)
            filterR->apply(outDataFloatR, scaled_inLength_frames);    // NOTE: applies R filter IN PLACE (separate filter for R, because has separate state)

            // output data is INTERLEAVED STEREO (normal LR stereo) -- re-interleave to the outDataFloat buffer (which is the final output buffer)
            for (int i = scaled_inLength_frames-1; i >= 0; i--) {
                outDataFloat[2*i]   = max(min(1.0f, outDataFloat[i]),-1.0);  // L + hard limiter
                outDataFloat[2*i+1] = max(min(1.0f, outDataFloatR[i]),-1.0); // R + hard limiter
                thePeakLevelL_mono = fmaxf(thePeakLevelL_mono, outDataFloat[2*i]);   // peakL
                thePeakLevelR      = fmaxf(thePeakLevelR,      outDataFloat[2*i+1]); // peakR
            }
        }
        if (thePeakLevelL_mono < 1E-20) {   // ignore very small numbers
            thePeakLevelL_mono = 0.0;
        }
        if (thePeakLevelR < 1E-20) {        // ignore very small numbers
            thePeakLevelR = 0.0;
        }
//        if (thePeakLevel != 0.0) {
//            qDebug() << "peak: " << thePeakLevel;
//        }

        if (m_resetPeakDetector) {
            m_peakLevelL_mono = 32768.0 * thePeakLevelL_mono;
            m_peakLevelR      = 32768.0 * thePeakLevelR;
            m_resetPeakDetector = false;
        } else {
            m_peakLevelL_mono = fmaxf((float)(32768.0 * thePeakLevelL_mono), (float)m_peakLevelL_mono);
            m_peakLevelR      = fmaxf((float)(32768.0 * thePeakLevelR),      (float)m_peakLevelR);
        }

        // SOUNDTOUCH PITCH/TEMPO -----------
        if (clearSoundTouch) {
            // don't try to clear while the soundTouch buffers are in use.  Clear HERE, which is safe, because
            //   we know we're NOT currently using the soundTouch buffers until putSamples() point in time.
            soundTouch.clear();
            clearSoundTouch = false;
        }

        soundTouch.putSamples(outDataFloat, scaled_inLength_frames);  // Feed the samples into SoundTouch processor
                                                               //  NOTE: it always takes ALL of them in, so outDataFloat is now unused.

//        qDebug() << "unprocessed: " << soundTouch.numUnprocessedSamples() << ", ready: " << soundTouch.numSamples() << "scaled_frames: " << scaled_inLength_frames;
        int nFrames = soundTouch.receiveSamples(outDataFloat, inLength_frames);  // this is how many frames the AudioSink wants
//        qDebug() << "    room to write: " << inLength_frames <<  ", received: " << nFrames;
        // get ready to return --------------
        numProcessedFrames   = nFrames;         // it gave us this many frames back (might be less thatn inLength_frames)
        sourceFramesConsumed = scaled_inLength_frames; // we consumed all the input frames we were given

        currentFadeFactor -= numProcessedFrames * fadeFactorDecrementPerFrame;  // fade takes place here
        if (currentFadeFactor <= 0.0) {
            fadeComplete();  // if we reached final volume, pause the whole playback
        }

        duckingFactorFramesRemaining -= numProcessedFrames;  // it's assumed that we decrement one FrameRemaining for every frame processed (which is what we hear)
        if (duckingFactorFramesRemaining <= 0) {
            StopVolumeDucking();  // resets duckingFactor to 1.0, ducking stops
        }

        return(0);  // TODO: remove
}

public:
    QIODevice      *m_audioDevice;
    QAudioSink     *m_audioSink;
    unsigned char  *m_data;
    unsigned int   totalFramesInSong;

    unsigned int bytesPerFrame;
    unsigned int sampleRate;  // rename - this is FRAME rate

    float  currentFadeFactor;               // goes from 1.0 to zero in say 6 seconds
    float  fadeFactorDecrementPerFrame;     // decrements the fadeFactor, then resets itself

    float  currentDuckingFactor;            // goes from 1.0 to something like 0.75 (75%), then resets to 1.0
    float  duckingFactorFramesRemaining;    // decrements each until 0.0, then resets itself

    float currentPanEQFactor;   // compensates for the PAN (0.707) and EQ (0.767) volume losses

    // EQ -----------------
    float bassBoost_dB = 0.0;
    float midBoost_dB = 0.0;
    float trebleBoost_dB = 0.0;

    // INTELLIGIBILITY BOOST ----------
    float intelligibilityBoost_fKHz = 1.6;
    float intelligibilityBoost_widthOctaves = 2.0;
    float intelligibilityBoost_dB = 0.0;
    bool  intelligibilityBoost_enabled = false;

    biquad_params<float> bq[4];
    biquad_filter<float> *filter;   // filter initialization (also holds context between apply calls), for mono and L stereo
    biquad_filter<float> *filterR;  // filter initialization (also holds context between apply calls), for R stereo only

    // PITCH/TEMPO ============
    SoundTouch soundTouch;  // SoundTouch
    float tempoSpeedup_percent = 0.0;
    float pitchUp_semitones    = 0.0;
    bool clearSoundTouch = false;

private:
    unsigned int m_volume;
    double       m_pan;
    bool         m_mono;  // true when Force Mono is on

    double       m_peakLevelL_mono;     // for VU meter
    double       m_peakLevelR;          // for VU meter
    bool         m_resetPeakDetector;

    unsigned int currentState;
    bool         activelyPlaying;

    unsigned int   playPosition_frames;
    double       loopFrom_sec;  // when both From and To are zero, that means "no looping"
    double       loopTo_sec;

    float       processedData[8192];
    float       processedDataR[8192];
    unsigned int numProcessedFrames;   // number of frames in the processedData array that are VALID
    unsigned int sourceFramesConsumed; // number of source (input) frames used to create those processed (output) frames

    bool newFilterNeeded;
    bool threadDone;
};

PlayerThread myPlayer;  // singleton

// ===========================================================================
AudioDecoder::AudioDecoder()
{
//    qDebug() << "In AudioDecoder() constructor";

    connect(&m_decoder, &QAudioDecoder::bufferReady,
            this, &AudioDecoder::bufferReady);
    connect(&m_decoder, QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error),
            this, QOverload<QAudioDecoder::Error>::of(&AudioDecoder::error));
    connect(&m_decoder, &QAudioDecoder::isDecodingChanged,
            this, &AudioDecoder::isDecodingChanged);
    connect(&m_decoder, &QAudioDecoder::finished,
            this, &AudioDecoder::finished);
    connect(&m_decoder, &QAudioDecoder::positionChanged,
            this, &AudioDecoder::updateProgress);
    connect(&m_decoder, &QAudioDecoder::durationChanged,
            this, &AudioDecoder::updateProgress);

    m_progress = -1.0;
    
    m_data = new QByteArray();      // raw data, starts out at zero size, will be expanded when m_input is append()'ed
    m_input = new QBuffer(m_data);  // QIODevice interface

//    QString t = QString("0x%1").arg((quintptr)(m_data->data()), QT_POINTER_SIZE * 2, 16, QChar('0'));
//    qDebug() << "audio data is at:" << t;

    myPlayer.m_data = (unsigned char *)(m_data->data());

    m_audioSink = 0;                        // nothing yet
    m_currentAudioOutputDeviceName = "";    // nothing yet

    newSystemAudioOutputDevice();  // this will make m_audiosink and m_currentAudioOutputDeviceName valid

    myPlayer.start();  // starts the playback thread (in STOPPED mode)
    myPlayer.setPriority(QThread::TimeCriticalPriority);  // this stops audio dropouts on older X86 macbook when screensaver kicks in
}

AudioDecoder::~AudioDecoder()
{
    if (myPlayer.isRunning()) {
        myPlayer.Stop();
    }
    myPlayer.quit();
}

void AudioDecoder::newSystemAudioOutputDevice() {
    // we want a format that will be no resampling for 99% of the MP3 files, but is float for kfr/rubberband processing
    QAudioFormat desiredAudioFormat; // = device.preferredFormat();  // 48000, 2ch, float = WHY?  Why not 16-bit int?  Less memory used.
    desiredAudioFormat.setSampleRate(44100);
    desiredAudioFormat.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    desiredAudioFormat.setSampleFormat(QAudioFormat::Float);  // Note: 8 bytes per frame

    myPlayer.bytesPerFrame = 8;  // post-mixdown
    myPlayer.sampleRate = 44100;

//    qDebug() << "desiredAudioFormat: " << desiredAudioFormat << " )";

    QString defaultADName = QMediaDevices::defaultAudioOutput().description();
//    qDebug() << "newSystemAudioOutputDevice -- LAST AUDIO OUTPUT DEVICE NAME" << m_currentAudioOutputDeviceName << ", CURRENT AUDIO OUTPUT DEVICE NAME: " << defaultADName;

    if (defaultADName != m_currentAudioOutputDeviceName) {
        if (m_audioSink != 0) {
            // if we already have an AudioSink, make a new one

            m_audioSink->stop();
            QAudioSink *oldOne = m_audioSink;
//            qDebug() << "     making a new one atomically...";
            m_audioSink = new QAudioSink(desiredAudioFormat);  // uses the current default audio device, ATOMIC
            delete oldOne;
        } else {
//            qDebug() << "     making a new one for the first time...";
            m_audioSink = new QAudioSink(desiredAudioFormat);  // uses the current default audio device, ATOMIC
        }
        m_currentAudioOutputDeviceName = defaultADName;    // assuming that succeeded, this is the one we are sending audio to
    }

    m_audioDevice = m_audioSink->start();  // do write() to this to play music

    m_audioBufferSize = m_audioSink->bufferSize();
//    qDebug() << "BUFFER SIZE: " << m_audioBufferSize;

    m_decoder.setAudioFormat(desiredAudioFormat);

    // give the data pointers to myPlayer
    myPlayer.m_audioDevice = m_audioDevice;
    myPlayer.m_audioSink   = m_audioSink;
}


void AudioDecoder::setSource(const QString &fileName)
{
//    qDebug() << "AudioDecoder:: setSource" << fileName;
    if (m_decoder.isDecoding()) {
//        qDebug() << "*** had to stop decoding...";
        m_decoder.stop();
    }
    if (m_input->isOpen()) {
//        qDebug() << "*** had to close m_input...";
        m_input->close();
    }
    if (!m_data->isEmpty()) {
//        qDebug() << "*** had to throw away m_data...";
        delete(m_data);  // throw the whole thing away
        m_data = new QByteArray();
        m_input->setBuffer(m_data); // and make a new one (empty)
    }
//    qDebug() << "***** m_input now has " << m_input->size() << " bytes (should be zero).";
    m_decoder.setSource(QUrl::fromLocalFile(fileName));
//    qDebug() << "***** back from setSource()";
}

void AudioDecoder::start()
{
//    qDebug() << "AudioDecoder::start -- starting to decode...";
    if (!m_input->isOpen()) {
//        qDebug() << "\thad to open m_input...";
        m_input->open(QIODeviceBase::ReadWrite);
    }
    m_progress = -1;  // reset the progress bar to the beginning, because we're about to start.
    timer1.start();
    BPM = -1.0;  // -1 means "no BPM yet"
    m_decoder.start(); // starts the decode process
}

void AudioDecoder::stop()
{
//    qDebug() << "stopping...";
    m_decoder.stop();
//    m_input->close();
}

QAudioDecoder::Error AudioDecoder::getError()
{
//    qDebug() << "getError...";
    return m_decoder.error();
}

void AudioDecoder::bufferReady()
{
//    qDebug() << "audioDecoder::bufferReady";
    // read a buffer from audio decoder
    QAudioBuffer buffer = m_decoder.read();
    if (!buffer.isValid()) {
        return;
    }
    
    m_input->write(buffer.constData<char>(), buffer.byteCount());  // append bytes to m_input

//    // Force Mono is ON, so let's mixdown right here, and write a single array of float to the m_input
//    unsigned int numFrames = buffer.byteCount()/(2*myPlayer.bytesPerFrame); // this is the PRE-mixdown bytesPerFrame, which is 2X
//    float * outArray = new float[numFrames];
//    const float *inArray = (const float *)(buffer.constData<char>());
//    for (unsigned int i = 0; i < numFrames; i++) {
//        outArray[i] = (0.5*inArray[2*i]) + (0.5*inArray[2*i + 1]); // mixdown
//    }

//    m_input->write((const char *)outArray, buffer.byteCount()/2);  // append to m_input (half the number of bytes because 1 channel now)

//    delete [] outArray;
}

void AudioDecoder::error(QAudioDecoder::Error error)
{
//    qDebug() << "audiodecoder:: error";
    switch (error) {
    case QAudioDecoder::NoError:
        return;
    case QAudioDecoder::ResourceError:
        qDebug() << "Resource error";
        break;
    case QAudioDecoder::FormatError:
        qDebug() << "Format error";
        break;
    case QAudioDecoder::AccessDeniedError:
        qDebug() << "Access denied error";
        break;
    case QAudioDecoder::NotSupportedError:
        qDebug() << "Service missing error";
        break;
    }

//    emit done();  // I'm not sure that this is right, so commenting out.
}

void AudioDecoder::isDecodingChanged(bool isDecoding)
{
    if (isDecoding) {
//        qDebug() << "Decoding...";
    } else {
//        qDebug() << "Decoding stopped...";
    }
}

float AudioDecoder::BPMsample(float sampleStart_sec, float sampleLength_sec, float BPMbase, float BPMtolerance) {
    // returns BPM if in range
    // return zero if not in range

    float finalBPMresult;

    unsigned char *p_data = (unsigned char *)(m_data->data());
    myPlayer.m_data = p_data;  // we are done decoding, so tell the player where the data is

    myPlayer.totalFramesInSong = m_data->size()/myPlayer.bytesPerFrame; // pre-mixdown is 2 floats per frame = 8
//    qDebug() << "** AudioDecoder::BPMsample totalFramesInSong: " << myPlayer.totalFramesInSong;  // TODO: this is really frames

//    qDebug() << "BPMsample: " << m_data->size()/myPlayer.bytesPerFrame << p_data;

    // BPM detection -------
    //   this estimate will be based on mono mixed-down samples from T={30,40} sec
    const float *songPointer = (const float *)p_data;

    float sampleEnd_sec = sampleStart_sec + sampleLength_sec;
    float songLength_sec = myPlayer.totalFramesInSong/44100.0;

    // if song is longer than 40, end_sec will be 40; else end_sec will be the end of the song.
    float end_sec = (songLength_sec >= sampleEnd_sec ? sampleEnd_sec : songLength_sec);

    // if end_sec going backward by sampleLength is within the song, then use that point for the start_sec
    //   else, just use the start of the song
    float start_sec = (end_sec - sampleLength_sec > 0.0 ? end_sec - sampleLength_sec : 0.0 );

    unsigned int offsetIntoSong_samples = 44100 * start_sec;            // start looking at time T = 10 sec
    unsigned int numSamplesToLookAt = 44100 * (end_sec - start_sec);    //   look at 10 sec of samples

    //    qDebug() << "***** BPM DEBUG: " << songLength_sec << start_sec << end_sec << offsetIntoSong_samples << numSamplesToLookAt;

    // ==================
    float *monoBuffer = new float[numSamplesToLookAt];

    for (unsigned int i = 0; i < numSamplesToLookAt; i++) {
        // mixdown to mono
        monoBuffer[i] = 0.5*songPointer[2*(i+offsetIntoSong_samples)] + 0.5*songPointer[2*(i+offsetIntoSong_samples)+1];
    }

    MiniBPM BPMestimator(44100.0);
    BPMestimator.setBPMRange(BPMbase-BPMtolerance, BPMbase+BPMtolerance);  // limited range for square dance songs, else use percent
    finalBPMresult = BPMestimator.estimateTempoOfSamples(monoBuffer, numSamplesToLookAt); // 10 seconds of samples

//    qDebug() << "finalBPMresult: " << finalBPMresult;

    if (end_sec - start_sec < 10.0) {
        // if we don't have enough song left to really know what the BPM is, just say "I don't know"
        finalBPMresult = 0.0;
    }

    delete[] monoBuffer;

    return finalBPMresult;
}

void AudioDecoder::finished()
{
//    qDebug() << "AudioDecoder::finished()" << m_decoder.isDecoding();
//    qDebug() << "Decoding progress:  100%; m_input:" << m_input->size() << " bytes, m_data:" << m_data->size() << " bytes";
//    qDebug() << timer1.elapsed() << "milliseconds to decode";  // currently about 250ms to fully read in, decode, and save to the buffer.

    if ((m_input->size() == 0) && (m_data->size() == 0)) {
        // if really it's just getting started, just return, and finished() will be called a second time.
        //  This seems to me to be a bug that was introduced in Qt6.5 .
//        qDebug() << "***** IGNORING FINISHED(), BECAUSE IT'S NOT REALLY FINISHED";
        return;
    }

    t = new PerfTimer("AudioDecoder", __LINE__);
    t->start(__LINE__);

    BPM = BPMsample(60,30,125,15);  // this was the best compromise, now gets almost all of the problematic songs right

    t->elapsed(__LINE__); // 74ms

//    qDebug() << "======================================================";
//    qDebug() << "AudioDecoder::finished is calling beatBarDetection...";

    beatMap.clear();  // when a new audio file is loaded, it has no beatMap or measureMap calculated yet
    measureMap.clear();
//    beatBarDetection(); // NOTE: can only call this when the audio is completely in memory, this calculates beatMap and measureMap

    t->elapsed(__LINE__); // 583ms, so starting up the process takes about 0.6sec every time a song is loaded

    emit done(); // triggers haveDuration, which invokes haveDuration2, which initiates beat detection (ONLY if enabled).
}

void AudioDecoder::updateProgress()
{
    qint64 position = m_decoder.position();
    qint64 duration = m_decoder.duration();
    qreal progress = m_progress;
    if (position >= 0 && duration > 0)
        progress = position / (qreal)duration;

    if (progress >= m_progress + 0.1) {
//        qDebug() << "Decoding progress: " << (int)(progress * 100.0) << "%; m_input:" << m_input->size() << " bytes, m_data:" << m_data->size() << " bytes";
        m_progress = progress;
    }
}

// --------------------------------------------------------------------------------
void AudioDecoder::Play() {
//    qDebug() << "AudioDecoder::Play";

    QString defaultADName = QMediaDevices::defaultAudioOutput().description();
//    qDebug() << "AudioDecoder::Play -- LAST AUDIO OUTPUT DEVICE NAME" << m_currentAudioOutputDeviceName << ", CURRENT AUDIO OUTPUT DEVICE NAME: " << defaultADName;

    if (defaultADName != m_currentAudioOutputDeviceName) {
//        qDebug() << "AudioDecoder::Play -- making a new audio output device";
        newSystemAudioOutputDevice();
    }

    myPlayer.Play();
}

void AudioDecoder::Pause() {
//    qDebug() << "AudioDecoder::Pause";
    myPlayer.Pause();
}

void AudioDecoder::Stop() {  // FIX: why are there 2 of these, stop and Stop?
//    qDebug() << "AudioDecoder::Stop";
    myPlayer.Stop();
}

bool AudioDecoder::isPlaying() {
    return(getCurrentState() == BASS_ACTIVE_PLAYING);
}

void AudioDecoder::setVolume(unsigned int v) {
    myPlayer.setVolume(v);
}

void AudioDecoder::setPan(double p) {
    myPlayer.setPan(p);
}

void AudioDecoder::setMono(bool on) {
    myPlayer.setMono(on);
}

void AudioDecoder::setStreamPosition(double p) {
    myPlayer.setStreamPosition(p);
}

double AudioDecoder::getStreamPosition() {
    return(myPlayer.getStreamPosition());
}

double AudioDecoder::getStreamLength() {
    return(myPlayer.totalFramesInSong/44100.0);
}

unsigned char AudioDecoder::getCurrentState() {
    unsigned int currentState = myPlayer.getCurrentState();
    return(currentState);
}

void AudioDecoder::setBassBoost(float b) {
    myPlayer.setBassBoost(b);
}

void AudioDecoder::setMidBoost(float m) {
    myPlayer.setMidBoost(m);
}

void AudioDecoder::setTrebleBoost(float t) {
    myPlayer.setTrebleBoost(t);
}

void AudioDecoder::SetIntelBoost(unsigned int which, float val) {
    myPlayer.SetIntelBoost(which, val);
}

void AudioDecoder::SetIntelBoostEnabled(bool enable) {
    myPlayer.SetIntelBoostEnabled(enable);
}

void AudioDecoder::SetPanEQVolumeCompensation(float val) {
    myPlayer.SetPanEQVolumeCompensation(val);
}


double AudioDecoder::getBPM() {
    return (BPM);  // -1 = no BPM yet, 0 = out of range or undetectable, else returns a BPM
}

// ------------------------------------------------------------------
void AudioDecoder::setTempo(float newTempoPercent)
{
//    qDebug() << "AudioDecoder::setTempo: " << newTempoPercent << "%";
    myPlayer.setTempo(newTempoPercent);
}

// ------------------------------------------------------------------
void AudioDecoder::setPitch(float newPitchSemitones)
{
//    qDebug() << "AudioDecoder::setPitch: " << newPitchSemitones << " semitones";
    myPlayer.setPitch(newPitchSemitones);
}

// ------------------------------------------------------------------
void AudioDecoder::setLoop(double fromPoint_sec, double toPoint_sec)
{
//    qDebug() << "AudioDecoder::SetLoop: (" << fromPoint_sec << "," << toPoint_sec << ")";
    myPlayer.setLoop(fromPoint_sec, toPoint_sec);
}

void AudioDecoder::clearLoop()
{
//    qDebug() << "AudioDecoder::ClearLoop";
    myPlayer.clearLoop();
}

double AudioDecoder::getPeakLevelL_mono() {
    return(myPlayer.getPeakLevelL_mono());
}

double AudioDecoder::getPeakLevelR() {
    return(myPlayer.getPeakLevelR());
}

void AudioDecoder::fadeOutAndPause(float finalVol, float secondsToGetThere) {
    myPlayer.fadeOutAndPause(finalVol, secondsToGetThere);
}

void AudioDecoder::StartVolumeDucking(int duckToPercent, double forSeconds) {
    myPlayer.StartVolumeDucking(duckToPercent, forSeconds);
}

void AudioDecoder::StopVolumeDucking() {
    myPlayer.StopVolumeDucking();
}

int AudioDecoder::beatBarDetection() {
// NOTE: this can only be called after the file is completely loaded into memory!
// returns -1 if no vamp

// set to 1 to enable beat detection
#define DOBEATDETECTION 1

#if DOBEATDETECTION==1
//    qDebug() << "AudioDecoder::beatBarDetection() START =============";

    QString pathNameToVamp(QCoreApplication::applicationDirPath());
    pathNameToVamp.append("/vamp-simple-host");
    //    qDebug() << "VAMP path: " << pathNameToVamp;

    if (!QFileInfo::exists(pathNameToVamp) ) {

        QMessageBox msgBox;
        msgBox.setText(QString("ERROR: Could not find Vamp executable.<P>Snapping is disabled."));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();

        return(-1); // ERROR, VAMP DOES NOT EXIST
    }

    t->elapsed(__LINE__);

    // CREATE MONO VERSION OF AUDIO DATA =============================================
    unsigned char *p_data = (unsigned char *)(m_data->data());
    unsigned int framesInSong = m_data->size()/myPlayer.bytesPerFrame; // pre-mixdown is 2 floats per frame = 8
    const float *songPointer = (const float *)p_data;  // these are floats, range: -1.0 to 1.0

    float *monoBuffer = new float[framesInSong];

    for (unsigned int i = 0; i < framesInSong; i++) {
        monoBuffer[i] = 0.5*(songPointer[2*i] + songPointer[2*i+1]); // mixdown ENTIRE SONG to mono
    }

    t->elapsed(__LINE__); // toMono takes 11ms

    // LOW PASS FILTER IT, TO ELIMINATE THE CHUCK of BOOM-CHUCK =======================
    biquad_params<float> bq[1];
    bq[0] = biquad_lowpass(1500.0 / 44100.0, 0.5); // Q = 0.5
    biquad_filter<float> *filter = new biquad_filter<float>(bq);  // init the filter

    filter->apply(monoBuffer, framesInSong);   // NOTE: applies IN PLACE

    delete filter;

    t->elapsed(__LINE__); // LPF takes 57ms

    // WRITE MONO VERSION OF FILE OUT TO DISK ==========================================
#define USETEMPFILES 1
    // Make a temp file name (or not) -----------
#if USETEMPFILES==1
    QTemporaryFile temp1;           // use a temporary file
    bool errOpen = temp1.open();    // this creates the WAV file
    WAVfilename = temp1.fileName(); // the open created it, if it's a temp file
    temp1.setAutoRemove(false);  // don't remove it until after we process it asynchronously

//    qDebug() << "WAVfilename: " << WAVfilename;

    if (!errOpen) {
        qDebug() << "ERROR: beatBarDetection could not open temporary WAV file!" << WAVfilename;
        delete[] monoBuffer;
        return(-2); // ERROR
    }
#else
    WAVfilename = "/Users/mpogue/beatDetect.wav";
    QFile temp1(WAVfilename);  // use a file in a known location
    if (!temp1.open(QIODevice::WriteOnly)) {
        qDebug() << "ERROR: beatBarDetection could not open temporary WAV file!" << WAVfilename;
        delete[] monoBuffer;
        return(-2);
    }
#endif

//    qDebug() << "Temp WAV filename: " << WAVfilename;

    WAV_FILE_INFO wavInfo2 = wav_set_info(44100, framesInSong,1,16,2,1);   // assumes 44.1KHz sample rate, floats, range: -1.0 to 1.0
    wav_write_file_float1(monoBuffer, WAVfilename.toStdString().c_str(), wavInfo2, framesInSong);   // write entire song as mono to a file

    delete[] monoBuffer;  // we don't need the data anymore (until we decide to call the library directly, instead of via QProcess)

    t->elapsed(__LINE__); // write WAV file takes 484ms (these can be pretty big, 27Mb or so)

    // NOW DO BAR/BEAT DETECTION ON THAT WAV FILE VIA EXTERNAL VAMP PROCESS =============

    // Make a file for the results ----------
#if USETEMPFILES==1
    QTemporaryFile temp2;  // use a temporary file
    bool errOpen2 = temp2.open();
    resultsFilename = temp2.fileName(); // the open created it, if it's a temp file
    temp2.setAutoRemove(false);  // don't remove it until after we process it asynchronously

//    qDebug() << "resultsFilename: " << resultsFilename;

    if (!errOpen2) {
        resultsFilename = temp2.fileName(); // the open created it, if it's a temp file
        qDebug() << "ERROR: beatBarDetection could not open temporary RESULTS file!" << resultsFilename;
        return(-3); // ERROR
    }
#else
    resultsFilename = "/Users/mpogue/beatDetect.results.txt";
    QFile temp2(resultsFilename); // use a file in a known location
    if (!temp2.open(QIODevice::WriteOnly)) {
        resultsFilename = temp2.fileName();
        qDebug() << "ERROR: beatBarDetection could not open temporary RESULTS file!" << resultsFilename;
        return(-3);
    }
#endif

//    qDebug() << "Temp RESULTS filename: " << resultsFilename;

    // TEST: ./vamp-simple-host -s qm-vamp-plugins:qm-barbeattracker hawk_LPF1500.wav -o beats.lpf1500.txt
//    QString pathNameToVamp("/Users/mpogue/_____BarBeatDetect/qm-vamp-plugins-1.8.0/lib/vamp-plugin-sdk/host/vamp-simple-host");  // TODO: stick vamp-simple-host into SquareDesk_1.0.5.app/Contents/MacOS

    // TO BUILD VAMP ON MAC OS X:
    //   add "." to path list on PluginHostAdapter.cpp:L87  <-- this allows the vamp-simple-host to find the .dylib, if in the same folder as the executable
    //   in Makefile.osx, change "./lib/vamp-plugin-sdk/libvamp-sdk.a" to "../vamp-plugin-sdk/libvamp-sdk.a"
    //   NOTE: This will be an X86 executable, that will run on either M1 or X86 Mac
    //
    //   cd qm_vamp-plugins-1.8.0
    //   make -f build/osx/Makefile.osx
    //
    //   cd /Users/mpogue/_____BarBeatDetect/qm-vamp-plugins-1.8.0/lib/vamp-plugin-sdk
    //   make -f build/Makefile.osx
    //
    // TO INSTALL VAMP:
    // cd host
    // cp ../../../qm-vamp-plugins.dylib .
    // ./vamp-simple-host should already be there (because it's built here)
    //
    // Manually copy the dylib and the executable to the folder where the SquareDesk executable lives
    // cp *.dylib /Users/mpogue/clean3/SquareDesk/build-SquareDesk-Qt_6_4_3_for_macOS-Release/test123/SquareDesk.app/Contents/MacOS
    // cp vamp-simple-host /Users/mpogue/clean3/SquareDesk/build-SquareDesk-Qt_6_4_3_for_macOS-Release/test123/SquareDesk.app/Contents/MacOS
    //
    // TEST IT:
    //   cd host   OR cd /Users/mpogue/clean3/SquareDesk/build-SquareDesk-Qt_6_4_3_for_macOS-Release/test123/SquareDesk.app/Contents/MacOS
    //   ./vamp-simple-host -l
    //       should include "." in the path (so we know you got the one we just built)
    //       AND it should list "qm-barbeattracker" (so we know it found the dylib)


    // by this point, we know that the Vamp executable exists -----

//    QProcess vamp;
    vamp.disconnect();  // disconnect all previous connections, so we can start up a new one below (just one)
    vamp.start(pathNameToVamp, QStringList() << "-s" << "qm-vamp-plugins:qm-barbeattracker" << WAVfilename << "-o" << resultsFilename);

//    // SYNCHRONOUS: wait for vamp to finish
//    if (!vamp.waitForFinished()) {
//        return;
//    }

    // ASYNCHRONOUS: beatMap and measureMap will have non-zero lengths when we're done processing.
    QObject::connect(&vamp, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                     [=](int exitCode, QProcess::ExitStatus exitStatus)
                     {
        Q_UNUSED(exitCode)
//        qDebug() << "***** VAMP FINISHED:" << exitCode << exitStatus;

        t->elapsed(__LINE__); // 1600ms to have VAMP process the file

        if (exitStatus == QProcess::NormalExit) {
            // READ IN THE RESULTS, AND SETUP THE BEATMAP AND THE MEASUREMAP =====================

//            qDebug() << "Processing:" << resultsFilename;

            QFile resultsFile(resultsFilename);
            if (!resultsFile.open(QFile::ReadOnly | QFile::Text)) {
                qDebug() << "ERROR 7: Could not open results of Vamp from file:" << resultsFilename;
                return; // ERROR from lambda
            }
//            qDebug() << "OPEN SUCCEEDED:" << resultsFilename;

            QTextStream in(&resultsFile);
            for (QString line = in.readLine(); !line.isNull(); line = in.readLine()) {
                QStringList pieces = line.split(":");
                bool ok1, ok2;
                unsigned long sampleNum = pieces[0].toULong(&ok1);
                unsigned int beatNum = pieces[1].trimmed().toUInt(&ok2);
//                qDebug() << "Line: " << line << sampleNum << beatNum;

                beatMap.push_back(sampleNum/44100.0); // time_sec
                if (beatNum == 1) {
                    measureMap.push_back(sampleNum/44100.0); // time_sec for beat 1's of each measure (NOTE: TODO assumes 4/4 time for all!)
                }
            }

//             // DEBUG: show me the results -----
//            for (unsigned long i = 0; i < beatMap.size(); i+=4)
//            {
//                qDebug() << "BEAT:" << beatMap[i] << beatMap[i+1] << beatMap[i+2] << beatMap[i+3];
//            }

//            for (unsigned long i = 0; i < measureMap.size(); i+=4)
//            {
//                qDebug() << "MEASURE: " << measureMap[i] << measureMap[i+1] << measureMap[i+2] << measureMap[i+3];
//            }

            t->elapsed(__LINE__);

            // delete the temp files
            QFile WAVfile(WAVfilename);
            if (!WAVfile.remove()) {
                qDebug() << "ERROR: Had trouble removing the WAV file:" << WAVfile.fileName();
                return; // ERROR from lambda
            }

            if (!resultsFile.remove()) {
                qDebug() << "ERROR: Had trouble removing the RESULTS file:" << resultsFile.fileName();
                return; // ERROR from lambda
            }

            t->elapsed(__LINE__);
        } else {
            qDebug() << "ERROR: BAD EXIT FROM VAMP.";
            return; // ERROR from lambda
        }

        return;  // NO ERROR from lambda
                     });

    return(0);  // no error from beatBarDetection()

#endif
}

// Helper function (only used below in this file)
unsigned int find_closest(const std::vector<double>& sorted_array, double x) {

    auto iter_geq = std::lower_bound(
        sorted_array.begin(),
        sorted_array.end(),
        x
        );

    if (iter_geq == sorted_array.begin()) {
        return 0;
    }

    double a = *(iter_geq - 1);
    double b = *(iter_geq);

    if (fabs(x - a) < fabs(x - b)) {
        return iter_geq - sorted_array.begin() - 1;
    }

    return iter_geq - sorted_array.begin();
}

// -----------------------------------
double AudioDecoder::snapToClosest(double time_sec, unsigned char granularity) {
    // returns -time_sec, if Vamp error of some kind
    // passing in 0.1 means a post-load init (not an actual snap)
//    qDebug() << "AudioDecoder::snapToClosest: " << time_sec;
    if (granularity == GRANULARITY_NONE) {
        // don't bother to calculate the beatMap and measureMap, if snapping is disabled
        return(time_sec);    // no snapping, just return the time we were given
    }

    // SNAPPING IS ON ==============================
    // if no beapMap has been calculated, do it now
    if (beatMap.empty()) {
//        qDebug() << "AudioDecoder::snapToClosest, beatMap is empty, so calculating it now...";
        int maybeError = beatBarDetection(); // calculate both the beatMap and the measureMap
//        qDebug() << "AudioDecoder::snapToClosest maybeError:" << maybeError;
        if (maybeError != 0) {
            return(-time_sec);
        }
    }

    // if it's a post-load init of beat detection (time_sec == 0.0), just return (beatMap will get filled in after 2 sec)
    if (time_sec == 0.1) {
        // 0.1 means post-load init
//        qDebug() << "Post-load init of beat detection...";
        return(0.0);
    }

    // OOPS: if beatMap is empty here, that means that the Vamp executable wasn't found, and we really can't do anything
    if (beatMap.empty()) {
//        qDebug() << "ERROR: beatMap is empty, maybe VAMP could not be run?";
        return(-time_sec);  // no snapping, just return the time we were given (times -1 to signal that Vamp needs to be disabled)
    }

    double result_sec = time_sec;
    unsigned int result_index = 0;

    switch (granularity) {
       case GRANULARITY_NONE:
        // should never get here, we exited early in this case
        qDebug() << "ERROR 122: unexpected GRANULARITY_NONE";
        result_sec = -time_sec;  // error return
        break;
       case GRANULARITY_BEAT:
        result_index = find_closest(beatMap, time_sec);
        result_sec = beatMap[result_index];
        break;
       case GRANULARITY_MEASURE:
        result_index = find_closest(measureMap, time_sec);
        result_sec = measureMap[result_index];
        break;
       default:
        qDebug() << "ERROR 123: unexpected Granularity" << granularity;
        result_sec = -time_sec;  // error return
        break;
    }

//    qDebug() << "AudioDecoder::snapToClosest: " << time_sec << granularity << ", returns: " << result_index << result_sec;
    return(result_sec); // normal return
}
