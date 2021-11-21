#include "audiodecoder.h"
#include <QFile>
#include <stdio.h>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QElapsedTimer>
#include <QThread>

// EQ ----------
#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>
#include <kfr/math.hpp>
#include <kfr/dsp/biquad.hpp>

using namespace kfr;

// BPM Estimation ==========

// #include "BPMDetect.h"
// BreakfastQuay BPM detection --------

#include "MiniBpm.h"
using namespace breakfastquay;

// PITCH/TEMPO ==============
#define USE_SOUND_TOUCH

#ifdef USE_SOUND_TOUCH
// SoundTouch BPM detection and pitch/tempo changing --------
#include "SoundTouch.h"
// Processing chunk size (size chosen to be divisible by 2, 4, 6, etc. channels ...)
#define SOUNDTOUCH_BUFF_SIZE           6720
using namespace soundtouch;
#else
// RubberBand -----------
#include <rubberband/rubberband/RubberBandStretcher.h>
using namespace RubberBand;
#endif

QElapsedTimer timer1;

// TODO: VU METER (kfr) ********
// TODO: LOOPS/sample accurate loops (myPlayer)
// TODO: SOUND EFFECTS (later) ********
// TODO: allow changing the output device (new feature!) *****

// ===========================================================================
class PlayerThread : public QThread
{
    //Q_OBJECT

public:
    PlayerThread()  {
        playPosition_samples = 0;
        activelyPlaying = false;
        currentState = BASS_ACTIVE_STOPPED;
        threadDone = false;
        m_volume = 100;
        bytesPerFrame = 0;
        sampleRate = 0;

        // EQ settings
        bassBoost_dB   =  0.0;  // +/-15dB
        midBoost_dB    =  0.0;  // +/-15dB
        trebleBoost_dB =  0.0;  // +/-15dB
        intelligibilityBoost_dB = 0.0;  // +/-10dB
        updateEQ();  // update the bq[], based on the current *Boost_dB settings

        // PITCH/TEMPO ---------
#ifdef USE_SOUND_TOUCH
        tempoSpeedup_percent = 0.0;
        pitchUp_semitones = 0.0;

        soundTouch.setSampleRate(44100);
        soundTouch.setChannels(2);  // we are already setup for stereo processing, just don't copy-to-mono.

        soundTouch.setTempoChange(tempoSpeedup_percent);  // this is in PERCENT
        soundTouch.setPitchSemiTones(pitchUp_semitones); // this is in SEMITONES
        soundTouch.setRateChange(0.0);   // this is in PERCENT (always ZERO, because no sample rate change

        soundTouch.setSetting(SETTING_USE_QUICKSEEK, false);  // NO QUICKSEEK (better quality)
        soundTouch.setSetting(SETTING_USE_AA_FILTER, true);   // USE AA FILTER (better quality)
#else
        timeRatio = 1.0;  // 2.0 makes the audio twice as long, i.e. slows it down
        pitchRatio = 0.8; // 2.0 makes it an octave higher; set to pow(2.0, S / 12.0) where S = semitones up

        stretcher = new RubberBandStretcher(44100,  // sample rate
                                            1,      // channels
                                            RubberBand::RubberBandStretcher::OptionProcessRealTime  |  // options bitwise-OR'd
                                            RubberBand::RubberBandStretcher::OptionPitchHighQuality |  // or OptionPitchHighSpeed
                                            RubberBand::RubberBandStretcher::OptionTransientsSmooth,   // don't use standard
                                            timeRatio,
                                            pitchRatio);
        stretcher->setMaxProcessSize(8192);  // I THINK that we'll get smaller requests, but not sure...
#endif
    }

    virtual ~PlayerThread() {
        threadDone = true;

        // from: https://stackoverflow.com/questions/28660852/qt-qthread-destroyed-while-thread-is-still-running-during-closing
        if(!this->wait(300))    // Wait until it actually has terminated (max. 300 msec)
        {
            qDebug() << "force terminating the PlayerThread";
            this->terminate();  // Thread didn't exit in time, probably deadlocked, terminate it!
            this->wait();       // We have to wait again here!
        }
    }

#ifdef USE_SOUND_TOUCH
    // SOUNDTOUCH LOOP
    void run() override {
        while (!threadDone) {
            unsigned int bytesFree = m_audioSink->bytesFree();  // the audioSink has room to accept this many bytes
            if ( activelyPlaying && bytesFree > 0) {
                const char *p_data = (const char *)(m_data) + (bytesPerFrame * playPosition_samples);  // next samples to play

                // write the smaller of bytesFree and how much we have left in the song
                unsigned int bytesNeededToWrite = bytesFree;  // default is to write all we can
                if (bytesPerFrame * (totalSamplesInSong - playPosition_samples) < bytesFree) {
                    // but if the song ends sooner than that, just send the last samples in the song
                    bytesNeededToWrite = bytesPerFrame * (totalSamplesInSong - playPosition_samples);
                }

                if (bytesNeededToWrite > 0) {
                    // if we need bytes, let's go get them.  BUT, if processDSP only gives us less than that, then write just those.
                    numProcessedFrames = 0;
                    sourceFramesConsumed = 0;
                    processDSP(p_data, bytesNeededToWrite);  // processes 8-byte-per-frame stereo to 8-byte-per-frame *processedData (dual mono)

                    playPosition_samples += sourceFramesConsumed; // move the data pointer to the next place to read from
                                                                  // if at the end, this will point just beyond the last sample
                                                                  // these were consumed (sent to soundTouch) in any case.

                    if (numProcessedFrames > 0) {  // but, maybe we didn't get any back from soundTouch.
                        m_audioDevice->write((const char *)&processedData, bytesPerFrame * numProcessedFrames);  // DSP processed audio is 8 bytes/frame floats
                    }
                } else {
                    Stop(); // we reached the end, so reset the player back to the beginning (disable the writing, move playback position to 0)
                }
            } // activelyPlaying
            msleep(5); // sleep 10 milliseconds, this sets the polling rate for writing bytes to the audioSink  // CHECK THIS!
        } // while
    }
#else
    // RUBBERBAND LOOP
    void run() override {
        while (!threadDone) {
            unsigned int bytesFree = m_audioSink->bytesFree();  // the audioSink has room to accept this many bytes
            if ( activelyPlaying && bytesFree > 0) {
                const char *p_data = (const char *)(m_data) + (bytesPerFrame * playPosition_samples);

                // write the smaller of bytesFree and how much we have left in the song
                unsigned int bytesToWrite = bytesFree;
                if (bytesPerFrame * (totalSamplesInSong - playPosition_samples) < bytesFree) {
                    bytesToWrite = bytesPerFrame * (totalSamplesInSong - playPosition_samples);
                }

                if (bytesToWrite > 0) {
                    unsigned int bytesAvailableToWrite = processDSP(p_data, bytesToWrite);  // processes 8-byte-per-frame stereo to 8-byte-per-frame *processedData (dual mono)
                    (void)bytesAvailableToWrite;  // Q_UNUSED (right now)

                    m_audioDevice->write((const char *)&processedData, bytesToWrite);  // DSP processed audio is 8 bytes/frame floats
                    playPosition_samples += bytesToWrite/bytesPerFrame;      // move the data pointer to the next place to read from
                                                                 // if at the end, this will point just beyond the last sample
                } else {
                    Stop(); // we reached the end, so reset the player back to the beginning (disable the writing, move playback position to 0)
                }
            }
            msleep(10); // sleep 10 milliseconds, this sets the polling rate for writing bytes to the audioSink
        }
    }
#endif

    void Play() {
        qDebug() << "PlayerThread::Play";

        // flush the state ------
        if (filter != NULL) {
            filter->reset();
        }
#ifdef USE_SOUND_TOUCH
#else
        if (stretcher != NULL) {
            stretcher->reset();
        }
#endif
        activelyPlaying = true;
        currentState = BASS_ACTIVE_PLAYING;
    }

    void Stop() {
        qDebug() << "PlayerThread::Stop";
        activelyPlaying = false;
#ifdef USE_SOUND_TOUCH
        clearSoundTouch = true;  // at next opportunity, flush all the soundTouch buffers, because we're stopped now.
#endif
        playPosition_samples = 0;
        currentState = BASS_ACTIVE_STOPPED;

        // flush the state ------
        if (filter != NULL) {
            filter->reset();
        }
#ifdef USE_SOUND_TOUCH
#else
        if (stretcher != NULL) {
            stretcher->reset();
        }
#endif
    }

    void Pause() {
        qDebug() << "PlayerThread::Pause";
        activelyPlaying = false;
        currentState = BASS_ACTIVE_PAUSED;
    }

    void setVolume(unsigned int vol) {
        m_volume = vol;
    }

    unsigned int getVolume() {
        return(m_volume);
    }

    void setPan(double pan) {
        m_pan = pan;
    }

    void setMono(bool on) {
        m_mono = on;
    }

    void setStreamPosition(double p) {
        playPosition_samples = (unsigned int)(sampleRate * p); // this should be atomic
    }

    double getStreamPosition() {
        return((double)(playPosition_samples/sampleRate));  // returns current position in seconds as double
    }

    unsigned char getCurrentState() {
        return(currentState);  // returns current state as int {0,3}
    }

    // EQ --------------------------------------------------------------------------------
    void updateEQ() {
        // given bassBoost_dB, etc., recreate the bq's.
        bq[0] = biquad_peak( 125.0/44100.0,  4.0,   bassBoost_dB);    // tweaked Q to match Intel version
        bq[1] = biquad_peak(1000.0/44100.0,  0.9,   midBoost_dB);     // tweaked Q to match Intel version
        bq[2] = biquad_peak(8000.0/44100.0,  0.9,   trebleBoost_dB);  // tweaked Q to match Intel version
        bq[3] = biquad_peak(1600.0/44100.0,  0.9,   intelligibilityBoost_dB);  //

        newFilterNeeded = true;
        qDebug() << "\tbiquad's are updated: ("<< bassBoost_dB << midBoost_dB << trebleBoost_dB << intelligibilityBoost_dB << ")";
    }

    void setBassBoost(double b) {
        qDebug() << "new BASS EQ value: " << b;
        bassBoost_dB = b;
        updateEQ();
    }

    void setMidBoost(double m) {
        qDebug() << "new MID EQ value: " << m;
        midBoost_dB = m;
        updateEQ();
    }

    void setTrebleBoost(double t) {
        qDebug() << "new TREBLE EQ value: " << t;
        trebleBoost_dB = t;
        updateEQ();
    }

    void setPitch(float newPitchSemitones) {
        qDebug() << "new Pitch value: " << newPitchSemitones << " semitones";
#ifdef USE_SOUND_TOUCH
        soundTouch.setPitchSemiTones(newPitchSemitones);
        //clearSoundTouch = true;  // clear at next opportunity
#else
        stretcher->setPitchScale(pow(2.0, newPitchSemitones / 12.0));
#endif
    }

    void setTempo(float newTempoPercent) {
//        qDebug() << "UNIMPLEMENTED: new Tempo value: " << newTempo;
        qDebug() << "new Tempo value: " << newTempoPercent;
#ifdef USE_SOUND_TOUCH
        soundTouch.setTempo(newTempoPercent/100.0);
        //clearSoundTouch = true;  // clear at next opportunity
#else
        stretcher->setTimeRatio(100.0/newTempoPercent);  // the ratio is a TIME ratio, but tempoPercent is a SPEED/BPM ratio
#endif
    }

    // ================================================================================
    unsigned int processDSP(const char *inData, unsigned int inLength_bytes) {

#ifdef USE_SOUND_TOUCH
        // returns 0

        // INS and OUTS ----------
        const float *inDataFloat   = (const float *)inData;      // input is stereo float interleaved (8 bytes per frame)
              float *outDataFloat  = (float *)(&processedData);  // output is stereo float interleaved (8 bytes per frame)

        const unsigned int inLength_frames = inLength_bytes/bytesPerFrame;  // pre-mixdown frames are 8 bytes

        // PAN/VOLUME/FORCE MONO --------
        const float PI_OVER_2 = 3.14159265f/2.0f;
        float theta = PI_OVER_2 * (m_pan + 1.0f)/2.0f;  // convert to 0-PI/2
        float KL = cos(theta);
        float KR = sin(theta);

//        float scaleFactor = m_volume/(100.0*2.0);  // the 2.0 is from the Force Mono function
        float scaleFactor = m_volume/(100.0);
        for (unsigned int i = 0; i < inLength_frames; i++) {
            // output data is stereo interleaved ("dual mono")
//            outDataFloat[2*i] = outDataFloat[2*i+1] = (float)(scaleFactor*KL*inDataFloat[2*i] + scaleFactor*KR*inDataFloat[2*i+1]); // stereo to 2ch mono + volume + pan
            outDataFloat[2*i]   = (float)(scaleFactor*KL*inDataFloat[2*i]);   // stereo to 2ch stereo + volume + pan
            outDataFloat[2*i+1] = (float)(scaleFactor*KR*inDataFloat[2*i+1]); // stereo to 2ch stereo + volume + pan
        }

        // EQ -----------
        // if the EQ has changed, make a new filter, but do it here only, just before it's used,
        //   to avoid crashing the thread when EQ is changed.
//        if (newFilterNeeded) {
//            qDebug() << "making a new filter...";
//            if (filter != NULL) {
//                delete filter;
//            }
//            filter = new biquad_filter<float>(bq);  // (re)initialize the filter from the latest bq coefficients
//            newFilterNeeded = false;
//        }

//        // APPLY EQ (4 biquads, including B/M/T and Intelligibility Boost) --------------------------------------------------------------------
//        filter->apply(outDataFloat, inLength_frames);   // NOTE: applies IN PLACE

        // SOUNDTOUCH PITCH/TEMPO -----------
//#define NO_SOUNDTOUCH
#ifdef NO_SOUNDTOUCH
        // get ready to return --------------
        // output is "dual mono" (8 bytes per frame)
        numProcessedFrames   = inLength_frames;     // it gave us this many frames back
        sourceFramesConsumed = inLength_frames;     // we consumed all the frames we were given
#else
        if (clearSoundTouch) {
            // don't try to clear while the soundTouch buffers are in use.  Clear HERE, which is safe, because
            //   we know we're NOT currently using the soundTouch buffers until putSamples() point in time.
            soundTouch.clear();
            clearSoundTouch = false;
        }

//        qDebug() << "inOutRatio:" << soundTouch.getInputOutputSampleRatio();
        soundTouch.putSamples(outDataFloat, inLength_frames);  // Feed the samples into SoundTouch processor
                                                               //  NOTE: it always takes ALL of them in, so outDataFloat is now unused.

        qDebug() << "unprocessed: " << soundTouch.numUnprocessedSamples() << ", ready: " << soundTouch.numSamples();
        int nFrames = soundTouch.receiveSamples(outDataFloat, inLength_frames);
        qDebug() << "    room to write: " << inLength_frames <<  ", received: " << nFrames;
        // get ready to return --------------
        numProcessedFrames   = nFrames;         // it gave us this many frames back
        sourceFramesConsumed = inLength_frames; // we consumed all the frames we were given
#endif
        return(0);
#else
        // returns number of bytes that are valid and can be written to audio device
        // PAN --------
        const float PI_OVER_2 = 3.14159265f/2.0f;
        float theta = PI_OVER_2 * (m_pan + 1.0f)/2.0f;  // convert to 0-PI/2
        float KL = cos(theta);
        float KR = sin(theta);

        // INS and OUTS ----------
        const float *inDataFloat   = (const float *)inData; // input is stereo interleaved (8 bytes per frame)
              float *outDataFloatL = (float *)(&processedDataL);
//              float *outDataFloatR = (float *)(&processedDataR);  // this will be used later, when Force Mono is OFF
        const unsigned int inLength_frames = inLength_bytes/bytesPerFrame;  // pre-mixdown frames are 8 bytes

        // FORCE MONO, VOLUME (0-100), and PAN/MIX -------
        // TODO: MONO needs to be split into two sides of an if statement, ON and OFF
        float scaleFactor = m_volume/(100.0*2.0);  // the 2.0 is from the Force Mono function
        for (unsigned int i = 0; i < inLength_frames; i++) {
            // NEW: output is mono (post-mixdown), use only L channel for now
            outDataFloatL[i] = (float)(scaleFactor*KL*inDataFloat[2*i] + scaleFactor*KR*inDataFloat[2*i+1]); // stereo to mono + volume + pan
        }

        // if the EQ has changed, make a new filter, but do it here only, just before it's used,
        //   to avoid crashing the thread when EQ is changed.
        if (newFilterNeeded) {
            qDebug() << "making a new filter...";
            if (filter != NULL) {
                delete filter;
            }
            filter = new biquad_filter<float>(bq);  // (re)initialize the filter from the latest bq coefficients
            newFilterNeeded = false;
        }

        // APPLY EQ (4 biquads, including B/M/T and Intelligibility Boost) --------------------------------------------------------------------
        filter->apply(outDataFloatL, inLength_frames);   // applies IN PLACE

        // APPLY PITCH/TEMPO ---------------
        //   outDataFloatL is the input here, already downmixed to mono, vol and EQ applied
        //
        float *inBuf[1] = {outDataFloatL};
        float outBuffer[8192];
        float *outBuf[1] = {outBuffer};

        stretcher->process(inBuf, inLength_frames, false);  // set to true if last one, pass in one block at a time
//        qDebug() << "stretcher given: " << inLength_frames << " frames";

        if ((const unsigned int)(stretcher->available()) > inLength_frames) {
            // now have more than can be accepted by the speaker
            // qDebug() << "Stretcher available: " << stretcher->available();
            stretcher->retrieve(outBuf, inLength_frames);  // pull out as many as the speaker can take

            // REINTERLEAVE --------------------------------------------------------------------------------------------
            // ... so that we can send to the speakers
            float *outDataFloat  = (float *)(&processedData);
            for (unsigned int i = 0; i < inLength_frames; i++) {
    //            outDataFloat[2*i] = outDataFloat[2*i+1] = outDataFloatL[i];  // result is mono in stereo interleaved (8 bytes per frame) format
                outDataFloat[2*i] = outDataFloat[2*i+1] = outBuffer[i];  // result is mono in stereo interleaved (8 bytes per frame) format
            }
        }

        return(4 * inLength_frames);
#endif
}

public:
    QIODevice      *m_audioDevice;
    QAudioSink     *m_audioSink;
    unsigned char  *m_data;
    unsigned int   totalSamplesInSong;

    unsigned int bytesPerFrame;
    unsigned int sampleRate;

    // EQ -----------------
    float bassBoost_dB = 0.0;
    float midBoost_dB = 0.0;
    float trebleBoost_dB = 0.0;
    float intelligibilityBoost_dB = 0.0;

    biquad_params<float> bq[4];
    biquad_filter<float> *filter;  // filter initialization (also holds context between apply calls)

    // PITCH/TEMPO ============
#ifdef USE_SOUND_TOUCH
    SoundTouch soundTouch;  // SoundTouch
    float tempoSpeedup_percent = 0.0;
    float pitchUp_semitones    = 0.0;
    bool clearSoundTouch = false;
#else
    RubberBandStretcher *stretcher;  // RubberBand
    double timeRatio = 1.0;
    double pitchRatio = 1.0;
#endif

private:
    unsigned int m_volume;
    double       m_pan;
    bool         m_mono;  // true when Force Mono is on

    unsigned int currentState;
    bool activelyPlaying;

    unsigned int   playPosition_samples;

#ifdef USE_SOUND_TOUCH
    float        processedData[8192];
    unsigned int numProcessedFrames;   // number of frames in the processedData array that are VALID
    unsigned int sourceFramesConsumed; // number of source (input) frames used to create those processed (output) frames
#else
    // deinterleaved
    unsigned char  processedDataL[8192 * sizeof(float)];  // biggest possible is 8192 floats
//    unsigned char  processedDataR[8192 * sizeof(float)];  // this will be used later when Force Mono is OFF

    // interleaved
    unsigned char  processedData[8192 * 2 * sizeof(float)];
#endif

    bool newFilterNeeded;
    bool threadDone;
};

PlayerThread myPlayer;  // singleton

// ===========================================================================
AudioDecoder::AudioDecoder()
{
    qDebug() << "In AudioDecoder() constructor";

    // we want a format that will be no resampling for 99% of the MP3 files, but is float for kfr/rubberband processing
    QAudioFormat desiredAudioFormat; // = device.preferredFormat();  // 48000, 2ch, float = WHY?  Why not 16-bit int?  Less memory used.
    desiredAudioFormat.setSampleRate(44100);
    desiredAudioFormat.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    desiredAudioFormat.setSampleFormat(QAudioFormat::Float);  // Note: 8 bytes per frame

    myPlayer.bytesPerFrame = 8;  // post-mixdown
    myPlayer.sampleRate = 44100;

    qDebug() << "desiredAudioFormat: " << desiredAudioFormat << " )";

//    QAudioDevice m_device = QMediaDevices::defaultAudioOutput();
    m_audioSink = new QAudioSink(desiredAudioFormat);
    m_audioDevice = m_audioSink->start();  // do write() to this to play music

    m_audioBufferSize = m_audioSink->bufferSize();
    qDebug() << "BUFFER SIZE: " << m_audioBufferSize;

    m_decoder.setAudioFormat(desiredAudioFormat);

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

    // give the data pointers to myPlayer
    myPlayer.m_audioDevice = m_audioDevice;
    myPlayer.m_audioSink = m_audioSink;
    myPlayer.m_data = (unsigned char *)(m_data->data());

    myPlayer.start();  // starts the playback thread (in STOPPED mode)
}

AudioDecoder::~AudioDecoder()
{
    if (myPlayer.isRunning()) {
        myPlayer.Stop();
    }
    myPlayer.quit();
}

void AudioDecoder::setSource(const QString &fileName)
{
    qDebug() << "AudioDecoder:: setSource" << fileName;
    if (m_decoder.isDecoding()) {
        qDebug() << "\thad to stop decoding...";
        m_decoder.stop();
    }
    if (m_input->isOpen()) {
        qDebug() << "\thad to close m_input...";
        m_input->close();
    }
    if (!m_data->isEmpty()) {
        qDebug() << "\thad to throw away m_data...";
        delete(m_data);  // throw the whole thing away
        m_data = new QByteArray();
        m_input->setBuffer(m_data); // and make a new one (empty)
    }
    qDebug() << "m_input now has " << m_input->size() << " bytes (should be zero).";
    m_decoder.setSource(QUrl::fromLocalFile(fileName));
}

void AudioDecoder::start()
{
    qDebug() << "AudioDecoder::start -- starting to decode...";
    if (!m_input->isOpen()) {
        qDebug() << "\thad to open m_input...";
        m_input->open(QIODeviceBase::ReadWrite);
    }
    m_progress = -1;  // reset the progress bar to the beginning, because we're about to start.
    timer1.start();
    BPM = -1.0;  // -1 means "no BPM yet"
    m_decoder.start(); // starts the decode process
}

void AudioDecoder::stop()
{
    qDebug() << "stopping...";
    m_decoder.stop();
//    m_input->close();
}

QAudioDecoder::Error AudioDecoder::getError()
{
    qDebug() << "getError...";
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
    qDebug() << "audiodecoder:: error";
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
        qDebug() << "Decoding...";
    } else {
        qDebug() << "Decoding stopped...";
    }
}

void AudioDecoder::finished()
{
    qDebug() << "AudioDecoder::finished()";
    qDebug() << "Decoding progress:  100%; m_input:" << m_input->size() << " bytes, m_data:" << m_data->size() << " bytes";
    qDebug() << timer1.elapsed() << "milliseconds to decode";  // currently about 250ms to fully read in, decode, and save to the buffer.

    unsigned char *p_data = (unsigned char *)(m_data->data());
    myPlayer.m_data = p_data;  // we are done decoding, so tell the player where the data is

    myPlayer.totalSamplesInSong = m_data->size()/myPlayer.bytesPerFrame; // pre-mixdown is 2 floats per frame = 8
    qDebug() << "** totalSamplesInSong: " << myPlayer.totalSamplesInSong;  // TODO: this is really frames

    // BPM detection -------
    //   this estimate will be based on mono mixed-down samples from T={10,20} sec
    const float *songPointer = (const float *)p_data;

    float songLength_sec = myPlayer.totalSamplesInSong/44100.0;
    float start_sec =  (songLength_sec >= 10.0 ? 10.0 : 0.0);
    float end_sec   =  (songLength_sec >= start_sec + 10.0 ? start_sec + 10.0 : songLength_sec );

    unsigned int offsetIntoSong_samples = 44100 * start_sec;            // start looking at time T = 10 sec
    unsigned int numSamplesToLookAt = 44100 * (end_sec - start_sec);    //   look at 10 sec of samples

    qDebug() << "***** BPM DEBUG: " << songLength_sec << start_sec << end_sec << offsetIntoSong_samples << numSamplesToLookAt;

    float monoBuffer[numSamplesToLookAt];
    for (unsigned int i = 0; i < numSamplesToLookAt; i++) {
        // mixdown to mono
        monoBuffer[i] = 0.5*songPointer[2*(i+offsetIntoSong_samples)] + 0.5*songPointer[2*(i+offsetIntoSong_samples)+1];
    }

    MiniBPM BPMestimator(44100.0);
    BPMestimator.setBPMRange(125.0-25.0, 125.0+25.0);  // limited range for square dance songs, else use percent
    BPM = BPMestimator.estimateTempoOfSamples(monoBuffer, numSamplesToLookAt); // 10 seconds of samples
    qDebug() << "***** BPM RESULT: " << BPM;  // -1 = overwritten by here, 0 = undetectable, else double

    emit done();
}

void AudioDecoder::updateProgress()
{
    qint64 position = m_decoder.position();
    qint64 duration = m_decoder.duration();
    qreal progress = m_progress;
    if (position >= 0 && duration > 0)
        progress = position / (qreal)duration;

    if (progress >= m_progress + 0.1) {
        qDebug() << "Decoding progress: " << (int)(progress * 100.0) << "%; m_input:" << m_input->size() << " bytes, m_data:" << m_data->size() << " bytes";
        m_progress = progress;
    }
}

// --------------------------------------------------------------------------------
void AudioDecoder::Play() {
    qDebug() << "AudioDecoder::Play";
    myPlayer.Play();
}

void AudioDecoder::Pause() {
    qDebug() << "AudioDecoder::Pause";
    myPlayer.Pause();
}

void AudioDecoder::Stop() {  // FIX: why are there 2 of these, stop and Stop?
    qDebug() << "AudioDecoder::Stop";
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
    return(myPlayer.totalSamplesInSong/44100.0);
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

double AudioDecoder::getBPM() {
    return (BPM);  // -1 = no BPM yet, 0 = out of range or undetectable, else returns a BPM
}

// ------------------------------------------------------------------
void AudioDecoder::setTempo(float newTempoPercent)
{
    qDebug() << "AudioDecoder::setTempo: " << newTempoPercent << "%";
    myPlayer.setTempo(newTempoPercent);
}

// ------------------------------------------------------------------
void AudioDecoder::setPitch(float newPitchSemitones)
{
    qDebug() << "AudioDecoder::setPitch: " << newPitchSemitones << " semitones";
    myPlayer.setPitch(newPitchSemitones);
}
