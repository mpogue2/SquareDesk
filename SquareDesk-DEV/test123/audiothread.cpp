#include "audiothread.h"

#include <iostream>
#include <assert.h>

#include <kfr/dsp.hpp>
#include <kfr/io.hpp>
#include <kfr/math.hpp>
using namespace kfr;

#ifdef Q_OS_LINUX
#include <mcheck.h>
#endif

inline void DoAMemoryCheck() {
//    mcheck(0);
}

class LockHolder {
private:
    QMutex &myMutex;
public :
    LockHolder(QMutex &mutex) : myMutex(mutex) {
        myMutex.lock();
    }
    ~LockHolder() {
        myMutex.unlock();
    }
};


// Processing chunk size (size chosen to be divisible by 2, 4, 6, etc. channels ...)
using namespace soundtouch;


AudioThread::AudioThread() {
    m_playPosition_frames = 0;
    m_activelyPlaying = false;
    m_currentState = BASS_ACTIVE_STOPPED;
    m_threadDone = false;
    m_volume = 100;
    m_bytesPerFrame = 0;
    m_sampleRate = 0;

    m_clearSoundTouch = false;
    m_currentFadeFactor = 1.0;
    m_fadeFactorDecrementPerFrame = 0.0;

    StopVolumeDucking();
    
    clearLoop();
    
    // EQ settings
    m_bassBoost_dB   =  0.0;  // +/-15dB
    m_midBoost_dB    =  0.0;  // +/-15dB
    m_trebleBoost_dB =  0.0;  // +/-15dB

    m_intelligibilityBoost_fKHz = 1.6;
    m_intelligibilityBoost_widthOctaves = 2.0;
    m_intelligibilityBoost_dB = 0.0;  // also turns this off
    m_intelligibilityBoost_enabled = false;
    
    updateEQ();  // update the m_bq[4], based on the current *Boost_* settings
    
        // PITCH/TEMPO ---------
    m_tempoSpeedup_percent = 0.0;
    m_pitchUp_semitones = 0.0;

    m_soundTouch.setSampleRate(m_sampleRate);
    m_soundTouch.setChannels(2);  // we are already setup for stereo processing, just don't copy-to-mono.

    m_soundTouch.setTempoChange(m_tempoSpeedup_percent);  // this is in PERCENT
    m_soundTouch.setPitchSemiTones(m_pitchUp_semitones); // this is in SEMITONES
    m_soundTouch.setRateChange(0.0);   // this is in PERCENT (always ZERO, because no sample rate change

    m_soundTouch.setSetting(SETTING_USE_QUICKSEEK, false);  // NO QUICKSEEK (better quality)
    m_soundTouch.setSetting(SETTING_USE_AA_FILTER, true);   // USE AA FILTER (better quality)

    m_peakLevelL_mono = 0.0;
    m_peakLevelR      = 0.0;
    m_resetPeakDetector = true;
}

AudioThread::~AudioThread() {
    m_threadDone = true;

    // from: https://stackoverflow.com/questions/28660852/qt-qthread-destroyed-while-thread-is-still-running-during-closing
    if(!this->wait(300))    // Wait until it actually has terminated (max. 300 msec)
    {
//            qDebug() << "force terminating the AudioThread";
        this->terminate();  // Thread didn't exit in time, probably deadlocked, terminate it!
        this->wait();       // We have to wait again here!
    }
}

unsigned int AudioThread::bytesAvailable(QAudioSink *audioSink) {
    DoAMemoryCheck();
    if (0 == m_audioSinkBufferSize) {
        m_audioSinkBufferSize = audioSink->bytesFree();
    }
    const unsigned int wantedBufferSize =  8 * m_sampleRate / 10;
    unsigned int padding = m_audioSinkBufferSize > wantedBufferSize ? (m_audioSinkBufferSize - wantedBufferSize) : 0;
    unsigned int bytesFree = audioSink->bytesFree();
    if (bytesFree > padding) {
        unsigned int toSend =  bytesFree - padding;
        if (wantedBufferSize < toSend) {
            return wantedBufferSize;
        }
        return toSend;
    }
    return 0;
}

void AudioThread::run() {
    while (!m_threadDone) {
//            unsigned int bytesFree = bytesAvailable(m_audioSink);  // the audioSink has room to accept this many bytes
        unsigned int bytesFree = 0;
        m_audioSinkAssignmentMutex.lock();
        if ( m_activelyPlaying && (m_audioSink) && (bytesFree = bytesAvailable(m_audioSink)) > 0) {  // let's be careful not to dereference a null pointer
            m_audioSinkAssignmentMutex.unlock();
            LockHolder dataAndTotalFramesLockHolder(m_dataAndTotalFramesMutex);
            // write the smaller of bytesFree and how much we have left in the song
            unsigned int bytesNeededToWrite = bytesFree;  // default is to write all we can
            if (m_bytesPerFrame * (m_totalFramesInSong - m_playPosition_frames) < bytesFree) {
                // but if the song ends sooner than that, just send the last samples in the song
                bytesNeededToWrite = m_bytesPerFrame * (m_totalFramesInSong - m_playPosition_frames);
            }
            
            unsigned int framesFree = bytesFree/m_bytesPerFrame;  // One frame = LR
            unsigned int loopEndpoint_frames = m_sampleRate * m_loopFrom_sec;
            bool straddlingLoopFromPoint =
                !(m_loopFrom_sec == 0.0 && m_loopTo_sec == 0.0) &&
                (m_playPosition_frames < loopEndpoint_frames) &&
                (m_playPosition_frames + framesFree) >= loopEndpoint_frames;

            if (straddlingLoopFromPoint) {
                // but if the song loops here, just send the frames up to the loop point
                bytesNeededToWrite = m_bytesPerFrame * (loopEndpoint_frames - m_playPosition_frames);
            }

            if (bytesNeededToWrite > 0) {
                // if we need bytes, let's go get them.  BUT, if processDSP only gives us less than that, then write just those.
                m_numProcessedFrames = 0;
                m_sourceFramesConsumed = 0;
                DoAMemoryCheck();
                ASSERT(m_playPosition_frames + (bytesNeededToWrite / m_bytesPerFrame) <= m_totalFramesInSong);
                const char *p_data = (const char *)(m_data) + (m_bytesPerFrame * m_playPosition_frames);  // next samples to play
                processDSP(p_data, bytesNeededToWrite);  // processes 8-byte-per-frame stereo to 8-byte-per-frame *m_processedData (dual mono)
                DoAMemoryCheck();

                if (straddlingLoopFromPoint) {
                    m_playPosition_frames = (unsigned int)(m_loopTo_sec * (double)(m_sampleRate)); // reset the playPosition to the loop start point
                } else {
                    m_playPosition_frames += m_sourceFramesConsumed; // move the data pointer to the next place to read from
                    // if at the end, this will point just beyond the last sample
                    // these were consumed (sent to m_soundTouch) in any case.
                }
                if (m_numProcessedFrames > 0) {  // but, maybe we didn't get any back from m_soundTouch.
                    DoAMemoryCheck();
                    ASSERT((const char *)(m_processedData) + m_bytesPerFrame * m_numProcessedFrames < (const char *)(m_processedData + PROCESSED_DATA_BUFFER_SIZE));
                    m_audioDevice->write((const char *)&m_processedData, m_bytesPerFrame * m_numProcessedFrames);  // DSP processed audio is 8 bytes/frame floats
                    DoAMemoryCheck();
                    
                }
            } else {
                Stop(); // we reached the end, so reset the player back to the beginning (disable the writing, move playback position to 0)
            }
        } // m_activelyPlaying
        else {
            m_audioSinkAssignmentMutex.unlock();
        }
        msleep(5); // sleep 10 milliseconds, this sets the polling rate for writing bytes to the audioSink  // CHECK THIS!
    } // while
}

void AudioThread::Play() {
//        qDebug() << "AudioThread::Play";

    // flush the state ------
    if (m_filter != NULL) {
        m_filter->reset();
    }
    if (m_filterR != NULL) {
        m_filterR->reset();
    }
    m_activelyPlaying = true;
    m_currentState = BASS_ACTIVE_PLAYING;

    // Play cancels FadeAndPause ------
    m_currentFadeFactor = 1.0;
    m_fadeFactorDecrementPerFrame = 0.0;
}

void AudioThread::Stop() {
//        qDebug() << "AudioThread::Stop";
    m_activelyPlaying = false;
    m_clearSoundTouch = true;  // at next opportunity, flush all the m_soundTouch buffers, because we're stopped now.
    m_playPosition_frames = 0;
    m_currentState = BASS_ACTIVE_STOPPED;
    
        // flush the state ------
    if (m_filter != NULL) {
        m_filter->reset();
    }
    if (m_filterR != NULL) {
        m_filterR->reset();
    }
    
    // Stop cancels FadeAndPause ------
    m_currentFadeFactor = 1.0;
    m_fadeFactorDecrementPerFrame = 0.0;
}

void AudioThread::Pause() {
//        qDebug() << "AudioThread::Pause";
    m_activelyPlaying = false;
    m_currentState = BASS_ACTIVE_PAUSED;
    
    // Pause cancels FadeAndPause ------
    m_currentFadeFactor = 1.0;
    m_fadeFactorDecrementPerFrame = 0.0;
}

// ---------------------
void AudioThread::setVolume(unsigned int vol) {
    m_volume = vol;
}

unsigned int AudioThread::getVolume() {
    return(m_volume);
}

void AudioThread::fadeOutAndPause(float finalVol, float secondsToGetThere) {
    Q_UNUSED(finalVol);
    m_currentFadeFactor           = 1.0;  // starts at 1.0 * m_volume, goes to 0.0 * m_volume
    m_fadeFactorDecrementPerFrame = 1.0 / (secondsToGetThere * m_sampleRate);  // when non-zero, starts fading.  when 0.0, pauses and restores fade factor to 1.0
}

void AudioThread::fadeComplete() {
    Pause();                            // pause (which also explicitly cancels and reinits fadeFactor and decrement)
}

void AudioThread::StartVolumeDucking(float duckToPercent, float forSeconds) {
    m_currentDuckingFactor = ((float)duckToPercent)/100.0;
    m_duckingFactorFramesRemaining = forSeconds * m_sampleRate;  // FIX: really FRAME rate
}

void AudioThread::StopVolumeDucking() {
    m_currentDuckingFactor = 1.0;
    m_duckingFactorFramesRemaining = 0.0;  // FIX: really FRAME rate
}

// ---------------------
void AudioThread::setPan(double pan) {
    m_pan = pan;
}

// ---------------------
void AudioThread::setMono(bool on) {
//        qDebug() << "AudioThread::setMono" << on;
    m_mono = on;
}

void AudioThread::setStreamPosition(double p) {
    m_playPosition_frames = (unsigned int)(m_sampleRate * p); // this should be atomic
}

double AudioThread::getStreamPosition() {
    return((double)m_playPosition_frames/(double)m_sampleRate);  // returns current position in seconds as double
}

unsigned int AudioThread::getCurrentState() {
    return(m_currentState);  // returns current state as int {0,3}
}

double AudioThread::getPeakLevelL_mono() {
    m_resetPeakDetector = true;  // we've used it, so start a new accumulation
    return(m_peakLevelL_mono);   // for VU meter, calculated when music is playing in DSP
}

double AudioThread::getPeakLevelR() { // ALWAYS CALL THIS FIRST, BECAUSE IT WILL NOT RESET THE PEAK DETECTOR
    // m_resetPeakDetector = true;  // we've used it, so start a new accumulation
    return(m_peakLevelR);           // for VU meter, calculated when music is playing in DSP
}

void AudioThread::setLoop(double from_sec, double to_sec) {
    m_loopFrom_sec = from_sec;
    m_loopTo_sec   = to_sec;
}

void AudioThread::clearLoop() {
    m_loopFrom_sec = m_loopTo_sec = 0.0;
}

// EQ --------------------------------------------------------------------------------
void AudioThread::updateEQ() {
    LockHolder filterLock(m_filterParameterMutex);
    // given m_bassBoost_dB, etc., recreate the bq's.
    m_bq[0] = biquad_peak( 125.0/((double)(m_sampleRate)),  4.0,   m_bassBoost_dB);    // tweaked Q to match Intel version
    m_bq[1] = biquad_peak(1000.0/((double)(m_sampleRate)),  0.9,   m_midBoost_dB);     // tweaked Q to match Intel version
    m_bq[2] = biquad_peak(8000.0/((double)(m_sampleRate)),  0.9,   m_trebleBoost_dB);  // tweaked Q to match Intel version

    float W0 = 2 * 3.14159265 * (1000.0 * m_intelligibilityBoost_fKHz/m_sampleRate);
    float Q = 1/(2.0 * sinhf( (logf(2.0)/2.0) * m_intelligibilityBoost_widthOctaves * (W0/sinf(W0)) ) );
//        qDebug() << "Q: " << Q << ", boost_dB: " << m_intelligibilityBoost_dB;
    m_bq[3] = biquad_peak(1000.0 * m_intelligibilityBoost_fKHz/ (double)(m_sampleRate), Q, m_intelligibilityBoost_dB);

    m_newFilterNeeded = true;
//        qDebug() << "\tbiquad's are updated: ("<< m_bassBoost_dB << m_midBoost_dB << m_trebleBoost_dB << m_intelligibilityBoost_dB << ")";
}

void AudioThread::setBassBoost(double b) {
//        qDebug() << "new BASS EQ value: " << b;
    m_bassBoost_dB = b;
    updateEQ();
}

void AudioThread::setMidBoost(double m) {
//        qDebug() << "new MID EQ value: " << m;
    m_midBoost_dB = m;
    updateEQ();
}

void AudioThread::setTrebleBoost(double t) {
//        qDebug() << "new TREBLE EQ value: " << t;
    m_trebleBoost_dB = t;
    updateEQ();
}

void AudioThread::SetIntelBoost(unsigned int which, float val) {
//        qDebug() << "INTEL BOOST: (" << which << ", " << val << ")";

    switch (which) {
    case 0: m_intelligibilityBoost_fKHz            = val;  break;
    case 1: m_intelligibilityBoost_widthOctaves    = val;  break;
    case 2: m_intelligibilityBoost_dB              = -val; break; // NOTE MINUS SIGN (control is positive, but Boost is negative (suppression)
    default:
//                qDebug() << "ERROR: UNKNOWN INTEL BOOST: (" << which << ", " << val << ")";
        break;
    }
    updateEQ();
}

void AudioThread::SetIntelBoostEnabled(bool enable) {
//        qDebug() << "INTEL BOOST ENABLED: " << enable;
    m_intelligibilityBoost_enabled = enable;
    updateEQ();
}

void AudioThread::setPitch(float newPitchSemitones) {
//        qDebug() << "new Pitch value: " << newPitchSemitones << " semitones";
    LockHolder soundTouchLockHolder(m_soundTouchMutex);
    m_soundTouch.setPitchSemiTones(newPitchSemitones);
}

void AudioThread::setTempo(float newTempoPercent) {
//        qDebug() << "new Tempo value: " << newTempoPercent;
    LockHolder soundTouchLockHolder(m_soundTouchMutex);
    m_soundTouch.setTempo(newTempoPercent/100.0);
}

// ================================================================================
unsigned int AudioThread::processDSP(const char *inData, unsigned int inLength_bytes) {

    // INS and OUTS ----------
    const float *inDataFloat   = (const float *)inData;     // input is stereo float interleaved (8 bytes per frame)
    const float *inDataFloatEndOfBuffer = (const float *)(inData + inLength_bytes);
    float *outDataFloat  = (float *)(&m_processedData);       // final output is stereo float interleaved (8 bytes per frame), also used as intermediate
    float *outDataFloatR  = (float *)(&m_processedDataR);     // intermediate output is R channel mono float (4 bytes per frame)

    const unsigned int inLength_frames = inLength_bytes/m_bytesPerFrame;  // pre-mixdown frames are 8 bytes

    // So, the AudioSink can take X frames, but after stretch/squish, we will have processed Y = K * X frames.
    //    scaled_inLength_frames is how many frames we need to process, so that we get the request inLength_frames output (which
    //    is what the AudioSink needs.  We have to scale the number of samples for BOTH the EQ processing (which has state)
    //    and the SoundTouch processing (which also has state).
    m_soundTouchMutex.lock();
    double inOutRatio = m_soundTouch.getInputOutputSampleRatio();
    m_soundTouchMutex.unlock();
    unsigned int scaled_inLength_frames = floor(((double)inLength_frames) / inOutRatio);
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
    if (m_newFilterNeeded) {
        LockHolder filterLock(m_filterParameterMutex);
//            qDebug() << "making a new filter...";
        if (m_filter != NULL) {
            delete m_filter;
        }
        m_filter = new biquad_filter<float>(m_bq);  // (re)initialize the mono/L filter from the latest bq coefficients
        if (m_filterR != NULL) {
            delete m_filterR;
        }
        m_filterR = new biquad_filter<float>(m_bq);  // (re)initialize the R filter from the latest bq coefficients
        m_newFilterNeeded = false;
    }
    
    float thePeakLevelL_mono = 0.0;
    float thePeakLevelR = 0.0;
    if (m_mono) {
        // Force Mono is ENABLED
        scaleFactor = m_currentDuckingFactor * m_currentFadeFactor * m_volume / (100.0 * 2.0);  // divide by 2, to avoid overflow; fade factor goes from 1.0 -> 0.0
//    qDebug() << "scaleFactor: " << scaleFactor;
        ASSERT(scaled_inLength_frames <= PROCESSED_DATA_BUFFER_SIZE);
        for (int i = 0; i < (int)scaled_inLength_frames; i++) {
            // output data is MONO (de-interleaved)
            ASSERT(&inDataFloat[2*i+1] < inDataFloatEndOfBuffer);
            outDataFloat[i] = (float)(scaleFactor*KL*inDataFloat[2*i] + scaleFactor*KR*inDataFloat[2*i+1]); // stereo to 2ch mono + volume + pan
        }
        
//            if (outDataFloat[0] != 0.0) {
//                for (int i = 0; i < 40; i++) { // DEBUG DEBUG DEBUG
//                    qDebug() << "B-outDataFloat[" << i << "]: " << outDataFloat[i];
//                }
//            }

            // APPLY EQ TO MONO (4 biquads, including B/M/T and Intelligibility Boost) ----------------------
        {
            LockHolder filterLock(m_filterParameterMutex);
            m_filter->apply(outDataFloat, scaled_inLength_frames);   // NOTE: applies IN PLACE
        }
        
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
        scaleFactor = m_currentDuckingFactor * m_currentFadeFactor * m_volume / (100.0);
        for (unsigned int i = 0; i < scaled_inLength_frames; i++) {
            // output data is de-interleaved into first and second half of outDataFloat buffer
//                outDataFloat[2*i]   = (float)(scaleFactor*KL*inDataFloat[2*i]);   // L: stereo to 2ch stereo + volume + pan
//                outDataFloat[2*i+1] = (float)(scaleFactor*KR*inDataFloat[2*i+1]); // R: stereo to 2ch stereo + volume + pan
            outDataFloat[i]  = (float)(scaleFactor*KL*inDataFloat[2*i]);   // L:
            outDataFloatR[i] = (float)(scaleFactor*KR*inDataFloat[2*i+1]); // R:
        }
        // APPLY EQ TO EACH CHANNEL SEPARATELY (4 biquads, including B/M/T and Intelligibility Boost) ----------------------
        
        ASSERT(outDataFloat + scaled_inLength_frames <= (float *)(m_processedData + PROCESSED_DATA_BUFFER_SIZE));
        ASSERT(outDataFloatR + scaled_inLength_frames <= (float *)(m_processedDataR + PROCESSED_DATA_BUFFER_SIZE));
        {
            LockHolder filterLock(m_filterParameterMutex);
            m_filter->apply(outDataFloat,   scaled_inLength_frames);    // NOTE: applies L filter IN PLACE (filter and storage used for mono and L)
            m_filterR->apply(outDataFloatR, scaled_inLength_frames);    // NOTE: applies R filter IN PLACE (separate filter for R, because has separate state)
        }

        // output data is INTERLEAVED STEREO (normal LR stereo) -- re-interleave to the outDataFloat buffer (which is the final output buffer)
        ASSERT((outDataFloat + (2 * scaled_inLength_frames)) <= (float *)(m_processedData + PROCESSED_DATA_BUFFER_SIZE)); 
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

    int nFrames = processSoundTouchData(outDataFloat, scaled_inLength_frames, outDataFloat, inLength_frames);
//        qDebug() << "    room to write: " << inLength_frames <<  ", received: " << nFrames;
    // geta ready to return --------------
    m_numProcessedFrames   = nFrames;         // it gave us this many frames back (might be less thatn inLength_frames)
    m_sourceFramesConsumed = scaled_inLength_frames; // we consumed all the input frames we were given
    
    m_currentFadeFactor -= m_numProcessedFrames * m_fadeFactorDecrementPerFrame;  // fade takes place here
    if (m_currentFadeFactor <= 0.0) {
        fadeComplete();  // if we reached final volume, pause the whole playback
    }
    
    m_duckingFactorFramesRemaining -= m_numProcessedFrames;  // it's assumed that we decrement one FrameRemaining for every frame processed (which is what we hear)
    if (m_duckingFactorFramesRemaining <= 0) {
        StopVolumeDucking();  // resets duckingFactor to 1.0, ducking stops
    }
    
    return(0);  // TODO: remove
}

int AudioThread::processSoundTouchData(float *inDataFloat, unsigned int scaled_inLength_frames, float *outDataFloat, unsigned int inLength_frames) {
    LockHolder soundTouchLockHolder(m_soundTouchMutex);
    // SOUNDTOUCH PITCH/TEMPO -----------
    if (m_clearSoundTouch) {
        // don't try to clear while the m_soundTouch buffers are in use.  Clear HERE, which is safe, because
        //   we know we're NOT currently using the m_soundTouch buffers until putSamples() point in time.
        m_soundTouch.clear();
        m_clearSoundTouch = false;
    }

    DoAMemoryCheck();
    m_soundTouch.putSamples(inDataFloat, scaled_inLength_frames);  // Feed the samples into SoundTouch processor
        //  NOTE: it always takes ALL of them in, so outDataFloat is now unused.
        
    DoAMemoryCheck();
//        qDebug() << "unprocessed: " << m_soundTouch.numUnprocessedSamples() << ", ready: " << m_soundTouch.numSamples() << "scaled_frames: " << scaled_inLength_frames;
    DoAMemoryCheck();
    ASSERT(inLength_frames <= PROCESSED_DATA_BUFFER_SIZE);
    int nFrames = m_soundTouch.receiveSamples(outDataFloat, inLength_frames);  // this is how many frames the AudioSink wants
    return nFrames;
}

void AudioThread::assignDataAndTotalFrames(unsigned char *data, unsigned int totalFrames) {
    LockHolder dataAndTotalFramesLockHolder(m_dataAndTotalFramesMutex);
    m_data = data;  // we are done decoding, so tell the player where the data is
    m_totalFramesInSong = totalFrames; // pre-mixdown is 2 floats per frame = 8
}

unsigned int AudioThread::getTotalFramesInSong() {
    LockHolder dataAndTotalFramesLockHolder(m_dataAndTotalFramesMutex);
    unsigned int total = m_totalFramesInSong;
    return total;
}

void AudioThread::assignAudioDeviceAudioSinkAndZeroBufferSize(QIODevice *audioDevice, QAudioSink *audioSink) {
    LockHolder lockHolder(m_audioSinkAssignmentMutex);
    m_audioDevice = audioDevice;
    m_audioSink   = audioSink;
    m_audioSinkBufferSize = 0;
}
