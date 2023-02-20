#ifndef AUDIOTHREAD_H_INCLUDED
#define AUDIOTHREAD_H_INCLUDED

#if defined(Q_OS_LINUX)
#include <QtMultimedia/QMediaDevices>
#include <QtMultimedia/QAudioDevice>
#include <QtMultimedia/QAudioSink>
#else /* end of Q_OS_LINUX */
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioSink>
#endif /* else if defined Q_OS_LINUX */
#include <QElapsedTimer>
#include <QThread>
#include <QMutex>

// EQ ----------
#include <kfr/base.hpp>
#include <kfr/dsp/biquad.hpp>
// PITCH/TEMPO ==============
// SoundTouch BPM detection and pitch/tempo changing --------
#include "SoundTouch.h"

// BASS_ChannelIsActive return values
#define BASS_ACTIVE_STOPPED 0
#define BASS_ACTIVE_PLAYING 1
#define BASS_ACTIVE_STALLED 2
#define BASS_ACTIVE_PAUSED  3

#define PROCESSED_DATA_BUFFER_SIZE 65536

class AudioThread : public QThread
{

public:
    AudioThread();
    virtual ~AudioThread();

    void Play();
    void Stop();
    void setVolume(unsigned int vol);
    unsigned int getVolume();
    void Pause();
    void fadeOutAndPause(float finalVol, float secondsToGetThere);
    void fadeComplete();
    void StartVolumeDucking(float duckToPercent, float forSeconds);
    void StopVolumeDucking();
    void setPan(double pan);
    void setMono(bool on);
    void setStreamPosition(double p);
    double getStreamPosition();
    unsigned int getCurrentState();
    double getPeakLevelL_mono();
    double getPeakLevelR();
    void setLoop(double from_sec, double to_sec);
    void clearLoop();
    void setBassBoost(double b);
    void setMidBoost(double m);
    void setTrebleBoost(double t);
    void SetIntelBoost(unsigned int which, float val);
    void SetIntelBoostEnabled(bool enable);
    void setPitch(float newPitchSemitones);
    void setTempo(float newTempoPercent);

private:
    unsigned int bytesAvailable(QAudioSink *audioSink);
    unsigned int processDSP(const char *inData, unsigned int inLength_bytes);    
    int processSoundTouchData(float *inDataFloat, unsigned int scaled_inLength_frames, float *outDataFloat, unsigned int inLength_frames);
    void updateEQ();
    

    
// SOUNDTOUCH LOOP
    void run() override;
    // ---------------------
    

public:
    void assignAudioDeviceAudioSinkAndZeroBufferSize(QIODevice *audioDevice, QAudioSink *audioSink);
private:
    QIODevice      *m_audioDevice;
    QMutex m_audioSinkAssignmentMutex;
    QAudioSink     *m_audioSink;
    unsigned int    m_audioSinkBufferSize;

private:
    QMutex m_dataAndTotalFramesMutex;
    unsigned char  *m_data;
    unsigned int   m_totalFramesInSong;
public:
    void assignDataAndTotalFrames(unsigned char *data, unsigned int totalFramesInSong);
    unsigned int getTotalFramesInSong();

    unsigned int getBytesPerFrame() { return m_bytesPerFrame; }
private:
    unsigned int m_bytesPerFrame;
    unsigned int m_sampleRate;  // rename - this is FRAME rate
public:
    void setBytesPerFrameAndSampleRate(unsigned int bytesPerFrame, unsigned int sampleRate) {
        m_bytesPerFrame = bytesPerFrame;
        m_sampleRate = sampleRate;
    }
    float  m_currentFadeFactor;               // goes from 1.0 to zero in say 6 seconds
    float  m_fadeFactorDecrementPerFrame;     // decrements the fadeFactor, then resets itself

    float  m_currentDuckingFactor;            // goes from 1.0 to something like 0.75 (75%), then resets to 1.0
    float  m_duckingFactorFramesRemaining;    // decrements each until 0.0, then resets itself

    // **************************************
    // updateEQ related stuff
private:
    // EQ -----------------
    float m_bassBoost_dB;
    float m_midBoost_dB;
    float m_trebleBoost_dB;

    // INTELLIGIBILITY BOOST ----------
    float m_intelligibilityBoost_fKHz;
    float m_intelligibilityBoost_widthOctaves;
    float m_intelligibilityBoost_dB;
    bool  m_intelligibilityBoost_enabled;

private:
    QMutex m_filterParameterMutex;
    kfr::biquad_params<float> m_bq[4];
    kfr::biquad_filter<float> *m_filter;   // filter initialization (also holds context between apply calls), for mono and L stereo
    kfr::biquad_filter<float> *m_filterR;  // filter initialization (also holds context between apply calls), for R stereo only


private:
    QMutex m_soundTouchMutex;
    // PITCH/TEMPO ============
    soundtouch::SoundTouch m_soundTouch;  // SoundTouch
    float m_tempoSpeedup_percent;
    float m_pitchUp_semitones;
    bool m_clearSoundTouch;

private:
    unsigned int m_volume;
    double       m_pan;
    bool         m_mono;  // true when Force Mono is on

    double       m_peakLevelL_mono;     // for VU meter
    double       m_peakLevelR;          // for VU meter
    bool         m_resetPeakDetector;

    unsigned int m_currentState;
    bool         m_activelyPlaying;

    unsigned int m_playPosition_frames;
    double       m_loopFrom_sec;  // when both From and To are zero, that means "no looping"
    double       m_loopTo_sec;

    float       m_processedData[PROCESSED_DATA_BUFFER_SIZE];
    float       m_processedDataR[PROCESSED_DATA_BUFFER_SIZE];
    unsigned int m_numProcessedFrames;   // number of frames in the processedData array that are VALID
    unsigned int m_sourceFramesConsumed; // number of source (input) frames used to create those processed (output) frames

    bool m_newFilterNeeded;
    bool m_threadDone;
};


#endif /* ifndef AUDIOTHREAD_H_INCLUDED */
