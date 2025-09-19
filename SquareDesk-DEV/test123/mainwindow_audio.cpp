#include <QAudioDecoder>
#include <QAudioBuffer>
#include <QEventLoop>
#include <QTimer>
#include <QUrl>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <cstring>
#include <cstdlib>

#include "minimp3_ex.h"

// Drop-in replacement for mp3dec_load() that uses Qt's QAudioDecoder
// to support multiple audio formats (M4A, MP3, WAV, FLAC, etc.)
int audiodec_load(mp3dec_t *mp3d, const char *file_name, mp3dec_file_info_t *info, MP3D_PROGRESS_CB progress_cb, void *user_data)
{
    // Parameter validation
    if (!mp3d || !file_name || !info)
        return MP3D_E_PARAM;
    
    // Initialize info structure
    memset(info, 0, sizeof(*info));
    
    // Check if file exists
    QString fileName = QString::fromUtf8(file_name);
    QFileInfo fileInfo(fileName);
    if (!fileInfo.exists() || !fileInfo.isFile())
        return MP3D_E_IOERROR;
    
    // Create QAudioDecoder
    QAudioDecoder decoder;
    
    // Set up the audio format we want (float to match mp3d_sample_t with MINIMP3_FLOAT_OUTPUT)
    QAudioFormat desiredFormat;
    desiredFormat.setSampleRate(44100); // Will be updated when we know the actual rate
    desiredFormat.setChannelCount(2);   // Will be updated when we know the actual channels
    desiredFormat.setSampleFormat(QAudioFormat::Float);
    
    decoder.setAudioFormat(desiredFormat);
    decoder.setSource(QUrl::fromLocalFile(fileName));
    
    // Storage for decoded samples
    QByteArray audioData;
    int sampleRate = 0;
    int channels = 0;
    int layer = 3; // Default for MP3-like behavior
    int avgBitrate = 0;
    bool formatDetected = false;
    
    // Event loop for synchronous operation
    QEventLoop loop;
    bool finished = false;
    bool hasError = false;
    QString errorString;
    
    // Connect signals
    QObject::connect(&decoder, &QAudioDecoder::bufferReady, [&]() {
        QAudioBuffer buffer = decoder.read();
        if (!buffer.isValid())
            return;
            
        // On first buffer, capture format information
        if (!formatDetected) {
            QAudioFormat format = buffer.format();
            sampleRate = format.sampleRate();
            channels = format.channelCount();
            formatDetected = true;
            
            // Update desired format to match source
            desiredFormat.setSampleRate(sampleRate);
            desiredFormat.setChannelCount(channels);
            
            // qDebug() << "Audio format detected: " << sampleRate << "Hz, " << channels << "channels";
        }
        
        // Convert buffer data to float format if needed
        if (buffer.format().sampleFormat() == QAudioFormat::Float) {
            // Already in float format, just append
            audioData.append(buffer.constData<char>(), buffer.byteCount());
        } else {
            // Need to convert to float format
            int frameCount = buffer.frameCount();
            int channelCount = buffer.format().channelCount();
            QByteArray convertedData;
            convertedData.resize(frameCount * channelCount * sizeof(float));
            float* outputData = reinterpret_cast<float*>(convertedData.data());
            
            if (buffer.format().sampleFormat() == QAudioFormat::Int16) {
                // Convert from Int16 to Float
                const int16_t* inputData = buffer.constData<int16_t>();
                for (int i = 0; i < frameCount * channelCount; ++i) {
                    outputData[i] = static_cast<float>(inputData[i]) / 32768.0f; // Convert to -1.0 to 1.0 range
                }
            } else if (buffer.format().sampleFormat() == QAudioFormat::Int32) {
                // Convert from Int32 to Float
                const int32_t* inputData = buffer.constData<int32_t>();
                for (int i = 0; i < frameCount * channelCount; ++i) {
                    outputData[i] = static_cast<float>(inputData[i]) / 2147483648.0f; // Convert to -1.0 to 1.0 range
                }
            } else {
                // For other formats, try to use Qt's built-in conversion by creating a new buffer
                QAudioFormat floatFormat = buffer.format();
                floatFormat.setSampleFormat(QAudioFormat::Float);
                
                // Create new buffer with converted data - we'll do a simple copy and hope Qt handles it
                // This is a fallback for unsupported formats
                audioData.append(buffer.constData<char>(), buffer.byteCount());
                qDebug() << "Warning: Unsupported audio format conversion from" << buffer.format().sampleFormat() << "to Float";
            }
            
            if (buffer.format().sampleFormat() == QAudioFormat::Int16 || buffer.format().sampleFormat() == QAudioFormat::Int32) {
                audioData.append(convertedData);
            }
        }
        
        // Call progress callback if provided
        if (progress_cb) {
            mp3dec_frame_info_t frameInfo;
            memset(&frameInfo, 0, sizeof(frameInfo));
            frameInfo.channels = channels;
            frameInfo.hz = sampleRate;
            frameInfo.layer = layer;
            frameInfo.bitrate_kbps = avgBitrate;
            
            // Note: We don't have accurate file size progress with QAudioDecoder
            // so we'll just call with dummy values
            int ret = progress_cb(user_data, 100, audioData.size() / (channels * sizeof(float)), &frameInfo);
            if (ret) {
                hasError = true;
                errorString = "Progress callback requested stop";
                decoder.stop();
                return;
            }
        }
    });
    
    QObject::connect(&decoder, &QAudioDecoder::finished, [&]() {
        finished = true;
        loop.quit();
    });
    
    QObject::connect(&decoder, QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error), [&](QAudioDecoder::Error error) {
        Q_UNUSED(error)
        hasError = true;
        errorString = decoder.errorString();
        finished = true;
        loop.quit();
    });
    
    // Timeout timer (30 seconds max)
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.setInterval(30000);
    QObject::connect(&timeoutTimer, &QTimer::timeout, [&]() {
        hasError = true;
        errorString = "Decode timeout";
        decoder.stop();
        finished = true;
        loop.quit();
    });
    
    // Start decoding
    timeoutTimer.start();
    decoder.start();
    
    // Wait for completion
    if (!finished) {
        loop.exec();
    }
    
    timeoutTimer.stop();
    
    // Check for errors
    if (hasError) {
        qDebug() << "audiodec_load error:" << errorString;
        if (errorString.contains("timeout"))
            return MP3D_E_USER;
        return MP3D_E_DECODE;
    }
    
    if (!formatDetected || audioData.isEmpty()) {
        qDebug() << "audiodec_load: No audio data decoded";
        return MP3D_E_DECODE;
    }
    
    // Calculate number of samples
    size_t sampleCount = audioData.size() / sizeof(float);
    
    // Allocate buffer for samples (same as mp3dec_load)
    info->buffer = (mp3d_sample_t*)malloc(audioData.size());
    if (!info->buffer) {
        return MP3D_E_MEMORY;
    }
    
    // Copy audio data to buffer
    memcpy(info->buffer, audioData.constData(), audioData.size());
    
    // Fill in the info structure
    info->samples = sampleCount;
    info->channels = channels;
    info->hz = sampleRate;
    info->layer = layer;
    info->avg_bitrate_kbps = avgBitrate; // QAudioDecoder doesn't provide bitrate info easily
    
    // qDebug() << "audiodec_load success: " << sampleCount << "samples, " << channels << "ch, " << sampleRate << "Hz";
    
    return 0; // Success
}
