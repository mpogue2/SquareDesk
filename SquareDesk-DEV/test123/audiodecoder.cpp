#include "audiodecoder.h"
#include <QFile>
#include <stdio.h>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QElapsedTimer>

QElapsedTimer timer1;

AudioDecoder::AudioDecoder()
{
    qDebug() << "In AudioDecoder() constructor";

    // we want a format that will be no resampling for 99% of the MP3 files
    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    QAudioFormat desiredAudioFormat; // = device.preferredFormat();  // 48000, 2ch, float = WHY?  Why not 16-bit int?  Less memory used.
    desiredAudioFormat.setSampleRate(44100);  // even if the system converts on-the-fly to 48000 later, the memory usage is less
    desiredAudioFormat.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    desiredAudioFormat.setSampleFormat(QAudioFormat::Int16);

    qDebug() << "desiredAudioFormat" << desiredAudioFormat;

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
}

AudioDecoder::~AudioDecoder()
{
}

void AudioDecoder::setSource(const QString &fileName)
{
    qDebug() << "setSource" << fileName;
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
