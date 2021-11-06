#include "audiodecoder.h"
#include <QFile>
#include <stdio.h>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QElapsedTimer>
#include <QThread>

QElapsedTimer timer1;

// TODO: BUG: the play button isn't changing properly when paused.
// TODO: STREAM POSITIONS, so that the GUI can be updated, and so I can click to jump forward. *********
// TODO: EQ *********
// TODO: PITCH/TEMPO ********

// ===========================================================================
class PlayerThread : public QThread
{
    //Q_OBJECT

public:
    PlayerThread()  {
        playPosition_samples = 0;
        activelyPlaying = false;
        threadDone = false;
        m_volume = 100;
    }

    virtual ~PlayerThread() {
//        this->quit();
        threadDone = true;
        msleep(100);  // HACK? -- give run() time to wake up and exit normally, to avoid a crash.
    }

    void run() override {
        while (!threadDone) {
            unsigned int bytesFree = m_audioSink->bytesFree();
            if ( activelyPlaying && bytesFree > 0) {
//                QString t = QString("0x%1").arg((quintptr)(m_data), QT_POINTER_SIZE * 2, 16, QChar('0'));
//                qDebug() << "audio data is at:" << t << "bytesFree:" << bytesFree << "playPosition_samples: " << playPosition_samples;
                const char *p_data = (const char *)(m_data) + (4 * playPosition_samples);
//                QString t2 = QString("0x%1").arg((quintptr)(p_data), QT_POINTER_SIZE * 2, 16, QChar('0'));
//                qDebug() << "\tpushing to: " << t2;
//                for (int i = 0; i < 10; i++) {
//                    qDebug() << "data[" << i << "] = " << (unsigned int)(p_data[i]);
//                }

                // write the smaller of bytesFree and how much we have left in the song
                unsigned int bytesToWrite = bytesFree;
                if (4 * (totalSamplesInSong - playPosition_samples) < bytesFree) {
                    bytesToWrite = 4 * (totalSamplesInSong - playPosition_samples);
                }

                if (bytesToWrite > 0) {
                    processDSP(p_data, bytesToWrite);  // processes it to *processedData
//                    m_audioDevice->write(p_data, bytesToWrite);  // just write up to the end; original PCM audio
                    m_audioDevice->write((const char *)&processedData, bytesToWrite);  // DSP processed audio
                    playPosition_samples += bytesToWrite/4;      // move the data pointer to the next place to read from
                                                                 // if at the end, this will point just beyond the last sample
                } else {
                    Stop(); // we reached the end, so reset the player back to the beginning (disable the writing, move playback position to 0)
                }
                // TODO: sample accurate loops
            }
            // TODO: pushing the last few bytes in the buffer needs to push, then stop the player
            msleep(10); // sleep 10 milliseconds
        }
        //qDebug() << "exiting run()";
    }

    void Play() {
        qDebug() << "PlayerThread::Play";
        activelyPlaying = true;
    }
    void Stop() {
        qDebug() << "PlayerThread::Stop";
        activelyPlaying = false;
        playPosition_samples = 0;
    }
    void Pause() {
        qDebug() << "PlayerThread::Pause";
        activelyPlaying = false;
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

    void processDSP(const char *inData, unsigned int inLength_bytes) {
        const short *inDataInt16 = (const short *)inData;
              short *outDataInt16 =      (short *)(&processedData);
        const unsigned int inLength_frames = inLength_bytes/4;

        // PAN --------
        const float PI_OVER_2 = 3.14159265f/2.0f;
        float theta = PI_OVER_2 * (m_pan + 1.0f)/2.0f;  // convert to 0-PI/2
        float KL = cos(theta);
        float KR = sin(theta);

        // FORCE MONO, VOLUME (0-100), and PAN/MIX -------
        float scaleFactor = m_volume/(100.0*2.0);  // the 2.0 is from the Force Mono function
        for (unsigned int i = 0; i < inLength_frames; i++) {
            outDataInt16[2*i] = outDataInt16[2*i+1] = (short)(scaleFactor*KL*inDataInt16[2*i] + scaleFactor*KR*inDataInt16[2*i+1]); // stereo to mono + volume + pan without overflow
        }
//        for (unsigned int i = 0; i < 80; i++) {
//            qDebug() << i << inDataInt16[2*i] << inDataInt16[2*i+1] << outDataInt16[2*i] << outDataInt16[2*i+1];
//        }
//        qDebug() << "stop here";
    }

    unsigned int m_volume;
    double m_pan;

    bool activelyPlaying;
    unsigned int   playPosition_samples;
    unsigned int   totalSamplesInSong;
    QIODevice      *m_audioDevice;
    QAudioSink     *m_audioSink;
    unsigned char  *m_data;
    unsigned char  processedData[8192 * 4]; // make it biggest ever expected (FIX: bufferSize)
    bool threadDone;
};

PlayerThread myPlayer;  // singleton

// ===========================================================================
AudioDecoder::AudioDecoder()
{
    qDebug() << "In AudioDecoder() constructor";

//    // create a timer
//    playTimer = new QTimer(this);

//    // setup signal and slot
//    connect(playTimer, SIGNAL(timeout()),
//            this,      SLOT(pushPlayBuffer()));

//    playPosition_samples = 0;  // start at the beginning
//    activelyPlaying = false;

    // we want a format that will be no resampling for 99% of the MP3 files

    QAudioFormat desiredAudioFormat; // = device.preferredFormat();  // 48000, 2ch, float = WHY?  Why not 16-bit int?  Less memory used.
    desiredAudioFormat.setSampleRate(44100);  // even if the system converts on-the-fly to 48000 later, the memory usage is less
    desiredAudioFormat.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    desiredAudioFormat.setSampleFormat(QAudioFormat::Int16);

    qDebug() << "desiredAudioFormat" << desiredAudioFormat;

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

    QString t = QString("0x%1").arg((quintptr)(m_data->data()), QT_POINTER_SIZE * 2, 16, QChar('0'));
    qDebug() << "audio data is at:" << t;

    // give the data pointers to myPlayer
    myPlayer.m_audioDevice = m_audioDevice;
    myPlayer.m_audioSink = m_audioSink;
    myPlayer.m_data = (unsigned char *)(m_data->data());

    myPlayer.start();
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
    qDebug() << "starting...";
    if (!m_input->isOpen()) {
        qDebug() << "\thad to open m_input...";
        m_input->open(QIODeviceBase::ReadWrite);
    }
    m_progress = -1;  // reset the progress bar to the beginning, because we're about to start.
    timer1.start();
    m_decoder.start();
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
    
    m_input->write(buffer.constData<char>(), buffer.byteCount());  // append to m_input
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

    emit done();
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
    qDebug() << "Decoding progress:  100%; m_input:" << m_input->size() << " bytes, m_data:" << m_data->size() << " bytes";
    qDebug() << timer1.elapsed() << "milliseconds to decode";  // currently about 250ms to fully read in, decode, and save to the buffer.
    emit done();

    unsigned char *p_data = (unsigned char *)(m_data->data());
    myPlayer.m_data = p_data;  // we are done decoding, so tell the player where the data is

    myPlayer.totalSamplesInSong = m_data->size()/4;  // TODO: 4 is numbytes per frame (make this a variable)
    qDebug() << "** totalSamplesInSong: " << myPlayer.totalSamplesInSong;
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

//void AudioDecoder::pushPlayBuffer() {

////    QAudio::State state = m_audioSink->state();
////    qDebug() << "audio state: " << state;
////    qDebug() << "audio format being used: " << m_audioSink->format();
//    unsigned int bytesFree = m_audioSink->bytesFree();
////    qDebug() << "pushPlayBuffer, current position: " << playPosition_samples << "bytesFree:" << bytesFree;

//    if ( bytesFree >= 0) {
//        const char *p_data = (const char *)(m_data->data()) + (4 * playPosition_samples);
////        QString t = QString("0x%1").arg((quintptr)p_data, QT_POINTER_SIZE * 2, 16, QChar('0'));
////        qDebug() << "p_data before write:" << t;

//        m_audioDevice->write(p_data, bytesFree);

//        playPosition_samples += bytesFree/4; // push m_audioBufferSize/4 samples each time
//        qDebug() << "wrote " << bytesFree << " bytes";

////        p_data = (const char *)(m_data) + (4 * playPosition_samples);
////        QString t2 = QString("0x%1").arg((quintptr)p_data, QT_POINTER_SIZE * 2, 16, QChar('0'));
////        qDebug() << "p_data  after write:" << t2;
//    }
//}

void AudioDecoder::Play() {
    qDebug() << "AudioDecoder::PLAY";
//    activelyPlaying = true;
//    playTimer->start(20);  // check for buffer write every 20ms; probably should do this in a thread instead.
    myPlayer.Play();
}

void AudioDecoder::Pause() {
    qDebug() << "AudioDecoder::PAUSE";
//    playTimer->stop();
//    activelyPlaying = false;
    myPlayer.Pause();
}

void AudioDecoder::Stop() {
    qDebug() << "AudioDecoder::STOP";
//    Pause(); // also sets activelyPlaying to false
//    playPosition_samples = 0;
    myPlayer.Stop();
}

bool AudioDecoder::isPlaying() {
//    return(activelyPlaying);
    return(myPlayer.activelyPlaying);
}

void AudioDecoder::setVolume(unsigned int v) {
    myPlayer.setVolume(v);
}

void AudioDecoder::setPan(double p) {
    myPlayer.setPan(p);
}
