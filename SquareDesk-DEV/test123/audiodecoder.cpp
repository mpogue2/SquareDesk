#include "audiodecoder.h"
#include <QFile>
#include <stdio.h>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QElapsedTimer>
#include <QThread>

#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>
#include <kfr/math.hpp>
#include <kfr/dsp/biquad.hpp>

using namespace kfr;

QElapsedTimer timer1;

// TODO: EQ *********
// TODO: PITCH/TEMPO ********
// TODO: VU METER ********
// TODO: LOOPS ********
// TODO: SOUND EFFECTS ********
// TODO: allow changing the output device *****
// TODO: sample accurate loops

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
        bassBoost_dB   =  0.0;
        midBoost_dB    =  0.0;
        trebleBoost_dB =  0.0;

        bq[0] = biquad_peak( 125.0/44100.0,  4.0,   bassBoost_dB);    // tweaked Q to match Intel version
        bq[1] = biquad_peak(1000.0/44100.0,  0.9,   midBoost_dB);     // tweaked Q to match Intel version
        bq[2] = biquad_peak(8000.0/44100.0,  0.9,   trebleBoost_dB);  // tweaked Q to match Intel version
    }

    virtual ~PlayerThread() {
//        this->quit();
        threadDone = true;
        msleep(100);  // HACK? -- give run() time to wake up and exit normally, to avoid a crash.

//  TODO: I think we maybe want to do cancel and wait, like this.
//        if (m_fileReadThread) {
//            m_fileReadThread->cancel();
//            m_fileReadThread->wait();
//            delete m_fileReadThread;
//            m_fileReadThread = 0;
//        }
    }

    void run() override {
        while (!threadDone) {
            unsigned int bytesFree = m_audioSink->bytesFree();
            if ( activelyPlaying && bytesFree > 0) {
//                QString t = QString("0x%1").arg((quintptr)(m_data), QT_POINTER_SIZE * 2, 16, QChar('0'));
//                qDebug() << "audio data is at:" << t << "bytesFree:" << bytesFree << "playPosition_samples: " << playPosition_samples;
                const char *p_data = (const char *)(m_data) + (bytesPerFrame * playPosition_samples);

                // write the smaller of bytesFree and how much we have left in the song
                unsigned int bytesToWrite = bytesFree;
                if (bytesPerFrame * (totalSamplesInSong - playPosition_samples) < bytesFree) {
                    bytesToWrite = bytesPerFrame * (totalSamplesInSong - playPosition_samples);
                }

                if (bytesToWrite > 0) {
                    processDSP(p_data, bytesToWrite);  // processes 8-byte-per-frame stereo to 4-byte-per-frame *processedData (mono)
//                    m_audioDevice->write(p_data, bytesToWrite);  // just write up to the end; original PCM audio
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

    void Play() {
        qDebug() << "PlayerThread::Play";
        activelyPlaying = true;
        currentState = BASS_ACTIVE_PLAYING;
    }

    void Stop() {
        qDebug() << "PlayerThread::Stop";
        activelyPlaying = false;
        playPosition_samples = 0;
        currentState = BASS_ACTIVE_STOPPED;
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

    // ================================================================================
    void processDSP(const char *inData, unsigned int inLength_bytes) {
        // PAN --------
        const float PI_OVER_2 = 3.14159265f/2.0f;
        float theta = PI_OVER_2 * (m_pan + 1.0f)/2.0f;  // convert to 0-PI/2
        float KL = cos(theta);
        float KR = sin(theta);

//        // INS and OUTS ----------
//        const short *inDataInt16 = (const short *)inData;
//              short *outDataInt16 =      (short *)(&processedData);
//        const unsigned int inLength_frames = inLength_bytes/bytesPerFrame;

//        // FORCE MONO, VOLUME (0-100), and PAN/MIX -------
//        // TODO: MONO needs to be split into two sides of an if statement, ON and OFF
//        float scaleFactor = m_volume/(100.0*2.0);  // the 2.0 is from the Force Mono function
//        for (unsigned int i = 0; i < inLength_frames; i++) {
//            // right now the output format is stereo
//            outDataInt16[2*i] = outDataInt16[2*i+1] = (short)(scaleFactor*KL*inDataInt16[2*i] + scaleFactor*KR*inDataInt16[2*i+1]); // stereo to mono + volume + pan without overflow
//        }

        // INS and OUTS ----------
        const float *inDataFloat   = (const float *)inData; // input is stereo interleaved (8 bytes per frame)
              float *outDataFloatL = (float *)(&processedDataL);
//              float *outDataFloatR = (float *)(&processedDataR);  // this will be used later, when Force Mono is OFF
        const unsigned int inLength_frames = inLength_bytes/bytesPerFrame;  // pre-mixdown frames are 8 bytes

        // FORCE MONO, VOLUME (0-100), and PAN/MIX -------
        // TODO: MONO needs to be split into two sides of an if statement, ON and OFF
        float scaleFactor = m_volume/(100.0*2.0);  // the 2.0 is from the Force Mono function
        for (unsigned int i = 0; i < inLength_frames; i++) {
            // OLD: right now the output format is stereo interleaved (both channels identical, so sounds like mono)
//            outDataFloat[2*i] = outDataFloat[2*i+1] = (float)(scaleFactor*KL*inDataFloat[2*i] + scaleFactor*KR*inDataFloat[2*i+1]); // stereo to mono + volume + pan
            // NEW: output is mono (post-mixdown), use only L channel for now
            outDataFloatL[i] = (float)(scaleFactor*KL*inDataFloat[2*i] + scaleFactor*KR*inDataFloat[2*i+1]); // stereo to mono + volume + pan
        }

        // EQ and RUBBERBAND HERE (these work right now on deinterleaved mono data = one 4 byte float per frame, at outDataFloatL)
        //
        //

        // EQ params for BiQuad from: https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html
        // something like this from: https://www.kfrlib.com/blog/biquad-filters-cpp-using-kfr/
//#include <kfr/math.hpp>
//#include <kfr/dsp/biquad.hpp>

//using namespace kfr;
//        filter.apply(data);  // also maintains state.  do filter.reset() to start over and reinitialize state
//        // perform the filtering and write the result back to data
//            // under the hood the following steps will have been performed:
//            // 1. create expression_biquads<1, double, univector<double>>
//            // 2. call process(data, <created expression>)
//            // The first argument can also be a static array of biquad_params<> structures
//            data = biquad(bq, data);

//  More info here: https://github.com/kfrlib/kfr/issues/59
//    std::unique_ptr<biquad_filter<float>> bqfilter;

//    void initialize()
//    {
//        // initialize chebyshev2 filter of order 8
//        zpk<float> filt                       = iir_lowpass(chebyshev2<float>(8, 80), 0.09);
//        // convert to 2nd order sections (suitable for biquad filter) or provide your own coefficients in bqs
//        std::vector<biquad_params<float>> bqs = to_sos(filt);
//        // create filter object
//        bqfilter.reset(new biquad_filter<float>(bqs.data(), bqs.size()));
//    }

//    void process_buffer(float* dest, float* source, size_t numSamples)
//    {
//        // this applies filter to source and writes to dest
//        bqfilter->apply(dest, source, numSamples);
//        // inplace variant: bqfilter->apply(buffer, numSamples)
//    }

        // MAYBE ALSO: https://cpp.hotexamples.com/site/file?hash=0x87e134c23323e9e5ea81df507e8244a643f67857be7ab0dc676e4771dd06203c&fullName=kfr-master/include/kfr/ebu.hpp&project=dlevin256/kfr

// ******************************************
// MAYBE BEST ********, from biquads.cpp

        biquad_filter<float> filter(bq);                // filter initialization (also holds context between apply calls)
        filter.apply(outDataFloatL, inLength_frames);   // applies IN PLACE

        // Now reinterleave, so that we can send to the speakers
        float *outDataFloat  = (float *)(&processedData);
        for (unsigned int i = 0; i < inLength_frames; i++) {
            outDataFloat[2*i] = outDataFloat[2*i+1] = outDataFloatL[i];  // result is mono in stereo interleaved (8 bytes per frame) format
        }

}

public:
    QIODevice      *m_audioDevice;
    QAudioSink     *m_audioSink;
    unsigned char  *m_data;
    unsigned int   totalSamplesInSong;

    unsigned int bytesPerFrame;
    unsigned int sampleRate;

    float bassBoost_dB = 0.0;
    float midBoost_dB = 0.0;
    float trebleBoost_dB = 0.0;

    biquad_params<float> bq[3];

private:
    unsigned int m_volume;
    double m_pan;
    bool   m_mono;  // true when Force Mono is on

    unsigned int currentState;
    bool activelyPlaying;

    unsigned int   playPosition_samples;

    // deinterleaved
    unsigned char  processedDataL[8192 * sizeof(float)];  // biggest possible is 8192 floats
//    unsigned char  processedDataR[8192 * sizeof(float)];  // this will be used later when Force Mono is OFF

    // interleaved
    unsigned char  processedData[8192 * 2 * sizeof(float)];

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

void AudioDecoder::Stop() {
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
