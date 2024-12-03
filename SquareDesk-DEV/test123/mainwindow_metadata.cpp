/****************************************************************************
**
** Copyright (C) 2016-2024 Mike Pogue, Dan Lyke
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
// Disable warning, see: https://github.com/llvm/llvm-project/issues/48757

#include "ui_mainwindow.h"
#include "songlistmodel.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop

#include <QList>
#include <QMap>
#include <QTextStream>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QElapsedTimer>
#include <QApplication>
#include <QDebug>

#include <QtConcurrent>
#include <QtMultimedia>

#define MINIMP3_FLOAT_OUTPUT
#include "minimp3_ex.h"

#include "xxhash64.h"

// TAGLIB stuff is MAC OS X and WIN32 only for now...
#include <taglib/toolkit/tlist.h>
#include <taglib/fileref.h>
#include <taglib/toolkit/tfile.h>
#include <taglib/tag.h>
#include <taglib/toolkit/tpropertymap.h>

#include <taglib/mpeg/mpegfile.h>
#include <taglib/mpeg/id3v2/id3v2tag.h>
#include <taglib/mpeg/id3v2/id3v2frame.h>
#include <taglib/mpeg/id3v2/id3v2header.h>

#include <taglib/mpeg/id3v2/frames/unsynchronizedlyricsframe.h>
#include <taglib/mpeg/id3v2/frames/commentsframe.h>
#include <taglib/mpeg/id3v2/frames/textidentificationframe.h>
#include <string>

#include "rifffile.h"
#include "wavfile.h"

using namespace TagLib;

#ifdef MINIMP3_ONE
// =============================================
// ********************
// These functions are variants that only load a single frame, for speed.
//   We use these to know what the sample rate was before we read it in with the Qt framework,
//   which converts everything to 44.1kHz (at our request).
// ********************
int mp3dec_load_cb_ONE(mp3dec_t *dec, mp3dec_io_t *io, uint8_t *buf, size_t buf_size, mp3dec_file_info_t *info, MP3D_PROGRESS_CB progress_cb, void *user_data)
{
    Q_UNUSED(progress_cb)
    Q_UNUSED(user_data)

    if (!dec || !buf || !info || (size_t)-1 == buf_size || (io && buf_size < MINIMP3_BUF_SIZE))
        return MP3D_E_PARAM;
    uint64_t detected_samples = 0;
    // size_t orig_buf_size = buf_size;
    int to_skip = 0;
    mp3dec_frame_info_t frame_info;
    memset(info, 0, sizeof(*info));
    memset(&frame_info, 0, sizeof(frame_info));

    /* skip id3 */
    size_t filled = 0, consumed = 0;
    int eof = 0;
    // int ret = 0;
    if (io)
    {
        if (io->seek(0, io->seek_data))
            return MP3D_E_IOERROR;
        filled = io->read(buf, MINIMP3_ID3_DETECT_SIZE, io->read_data);
        if (filled > MINIMP3_ID3_DETECT_SIZE)
            return MP3D_E_IOERROR;
        if (MINIMP3_ID3_DETECT_SIZE != filled)
            return 0;
        size_t id3v2size = mp3dec_skip_id3v2(buf, filled);
        if (id3v2size)
        {
            if (io->seek(id3v2size, io->seek_data))
                return MP3D_E_IOERROR;
            filled = io->read(buf, buf_size, io->read_data);
            if (filled > buf_size)
                return MP3D_E_IOERROR;
        } else
        {
            size_t readed = io->read(buf + MINIMP3_ID3_DETECT_SIZE, buf_size - MINIMP3_ID3_DETECT_SIZE, io->read_data);
            if (readed > (buf_size - MINIMP3_ID3_DETECT_SIZE))
                return MP3D_E_IOERROR;
            filled += readed;
        }
        if (filled < MINIMP3_BUF_SIZE)
            mp3dec_skip_id3v1(buf, &filled);
    } else
    {
        mp3dec_skip_id3((const uint8_t **)&buf, &buf_size);
        if (!buf_size)
            return 0;
    }
    /* try to make allocation size assumption by first frame or vbr tag */
    mp3dec_init(dec);
    int samples;
    do
    {
        uint32_t frames;
        int i, delay, padding, free_format_bytes = 0, frame_size = 0;
        const uint8_t *hdr;
        if (io)
        {
            if (!eof && filled - consumed < MINIMP3_BUF_SIZE)
            {   /* keep minimum 10 consecutive mp3 frames (~16KB) worst case */
                memmove(buf, buf + consumed, filled - consumed);
                filled -= consumed;
                consumed = 0;
                size_t readed = io->read(buf + filled, buf_size - filled, io->read_data);
                if (readed > (buf_size - filled))
                    return MP3D_E_IOERROR;
                if (readed != (buf_size - filled))
                    eof = 1;
                filled += readed;
                if (eof)
                    mp3dec_skip_id3v1(buf, &filled);
            }
            i = mp3d_find_frame(buf + consumed, filled - consumed, &free_format_bytes, &frame_size);
            consumed += i;
            hdr = buf + consumed;
        } else
        {
            i = mp3d_find_frame(buf, buf_size, &free_format_bytes, &frame_size);
            buf      += i;
            buf_size -= i;
            hdr = buf;
        }
        if (i && !frame_size)
            continue;
        if (!frame_size)
            return 0;
        frame_info.channels = HDR_IS_MONO(hdr) ? 1 : 2;
        frame_info.hz = hdr_sample_rate_hz(hdr);
        frame_info.layer = 4 - HDR_GET_LAYER(hdr);
        frame_info.bitrate_kbps = hdr_bitrate_kbps(hdr);
        frame_info.frame_bytes = frame_size;
        samples = hdr_frame_samples(hdr)*frame_info.channels;
        if (3 != frame_info.layer)
            break;
        int ret = mp3dec_check_vbrtag(hdr, frame_size, &frames, &delay, &padding);
        if (ret > 0)
        {
            padding *= frame_info.channels;
            to_skip = delay*frame_info.channels;
            detected_samples = samples*(uint64_t)frames;
            if (detected_samples >= (uint64_t)to_skip)
                detected_samples -= to_skip;
            if (padding > 0 && detected_samples >= (uint64_t)padding)
                detected_samples -= padding;
            if (!detected_samples)
                return 0;
        }
        if (ret)
        {
            if (io)
            {
                consumed += frame_size;
            } else
            {
                buf      += frame_size;
                buf_size -= frame_size;
            }
        }
        break;
    } while(1);
    size_t allocated = MINIMP3_MAX_SAMPLES_PER_FRAME*sizeof(mp3d_sample_t);
    if (detected_samples)
        allocated += detected_samples*sizeof(mp3d_sample_t);
    else
        allocated += (buf_size/frame_info.frame_bytes)*samples*sizeof(mp3d_sample_t);
    info->buffer = (mp3d_sample_t*)malloc(allocated);
    if (!info->buffer)
        return MP3D_E_MEMORY;
    /* save info */
    info->channels = frame_info.channels;
    info->hz       = frame_info.hz;
    info->layer    = frame_info.layer;
    /* decode all frames */

    return(0);  // NOTE: THIS MODIFIED VERSION DECODES ZERO MP3 FRAMES *****

    // ================================================

    //     size_t avg_bitrate_kbps = 0, frames = 0;
    //     do
    //     {
    //         if ((allocated - info->samples*sizeof(mp3d_sample_t)) < MINIMP3_MAX_SAMPLES_PER_FRAME*sizeof(mp3d_sample_t))
    //         {
    //             allocated *= 2;
    //             mp3d_sample_t *alloc_buf = (mp3d_sample_t*)realloc(info->buffer, allocated);
    //             if (!alloc_buf)
    //                 return MP3D_E_MEMORY;
    //             info->buffer = alloc_buf;
    //         }
    //         if (io)
    //         {
    //             if (!eof && filled - consumed < MINIMP3_BUF_SIZE)
    //             {   /* keep minimum 10 consecutive mp3 frames (~16KB) worst case */
    //                 memmove(buf, buf + consumed, filled - consumed);
    //                 filled -= consumed;
    //                 consumed = 0;
    //                 size_t readed = io->read(buf + filled, buf_size - filled, io->read_data);
    //                 if (readed != (buf_size - filled))
    //                     eof = 1;
    //                 filled += readed;
    //                 if (eof)
    //                     mp3dec_skip_id3v1(buf, &filled);
    //             }
    //             samples = mp3dec_decode_frame(dec, buf + consumed, filled - consumed, info->buffer + info->samples, &frame_info);
    //             consumed += frame_info.frame_bytes;
    //         } else
    //         {
    //             samples = mp3dec_decode_frame(dec, buf, MINIMP3_MIN(buf_size, (size_t)INT_MAX), info->buffer + info->samples, &frame_info);
    //             buf      += frame_info.frame_bytes;
    //             buf_size -= frame_info.frame_bytes;
    //         }
    //         if (samples)
    //         {
    //             if (info->hz != frame_info.hz || info->layer != frame_info.layer)
    //             {
    //                 ret = MP3D_E_DECODE;
    //                 break;
    //             }
    //             if (info->channels && info->channels != frame_info.channels)
    //             {
    // #ifdef MINIMP3_ALLOW_MONO_STEREO_TRANSITION
    //                 info->channels = 0; /* mark file with mono-stereo transition */
    // #else
    //                 ret = MP3D_E_DECODE;
    //                 break;
    // #endif
    //             }
    //             samples *= frame_info.channels;
    //             if (to_skip)
    //             {
    //                 size_t skip = MINIMP3_MIN(samples, to_skip);
    //                 to_skip -= skip;
    //                 samples -= skip;
    //                 memmove(info->buffer, info->buffer + skip, samples*sizeof(mp3d_sample_t));
    //             }
    //             info->samples += samples;
    //             avg_bitrate_kbps += frame_info.bitrate_kbps;
    //             frames++;
    //             if (progress_cb)
    //             {
    //                 ret = progress_cb(user_data, orig_buf_size, orig_buf_size - buf_size, &frame_info);
    //                 if (ret)
    //                     break;
    //             }
    //         }
    //     } while (
    //         false &&                     // NOTE MODIFIED FROM ORIGINAL mp3dec_load(), ONLY LOAD 1 FRAME
    //         frame_info.frame_bytes);

    //     if (detected_samples && info->samples > detected_samples)
    //         info->samples = detected_samples; /* cut padding */
    //     /* reallocate to normal buffer size */
    //     if (allocated != info->samples*sizeof(mp3d_sample_t))
    //     {
    //         mp3d_sample_t *alloc_buf = (mp3d_sample_t*)realloc(info->buffer, info->samples*sizeof(mp3d_sample_t));
    //         if (!alloc_buf && info->samples)
    //             return MP3D_E_MEMORY;
    //         info->buffer = alloc_buf;
    //     }
    //     if (frames)
    //         info->avg_bitrate_kbps = avg_bitrate_kbps/frames;
    //     return ret;
}

int mp3dec_load_buf_ONE(mp3dec_t *dec, const uint8_t *buf, size_t buf_size, mp3dec_file_info_t *info, MP3D_PROGRESS_CB progress_cb, void *user_data)
{
    return mp3dec_load_cb_ONE(dec, 0, (uint8_t *)buf, buf_size, info, progress_cb, user_data);
}

static int mp3dec_load_mapinfo_ONE(mp3dec_t *dec, mp3dec_map_info_t *map_info, mp3dec_file_info_t *info, MP3D_PROGRESS_CB progress_cb, void *user_data)
{
    int ret = mp3dec_load_buf_ONE(dec, map_info->buffer, map_info->size, info, progress_cb, user_data);
    mp3dec_close_file(map_info);
    return ret;
}

int mp3dec_load_ONE(mp3dec_t *dec, const char *file_name, mp3dec_file_info_t *info, MP3D_PROGRESS_CB progress_cb, void *user_data)
{
    int ret;
    mp3dec_map_info_t map_info;
    if ((ret = mp3dec_open_file(file_name, &map_info)))
        return ret;
    return mp3dec_load_mapinfo_ONE(dec, &map_info, info, progress_cb, user_data);
}
#endif

// ---------------------------------------------------------------------------------
// int MainWindow::MP3FileSampleRate(QString pathToMP3) {

// THIS IMPLEMENTATION IS OBSOLETE, USE TAGLIB-BASED IMPLEMENTATION BELOW

//     if (!pathToMP3.endsWith(".mp3", Qt::CaseInsensitive)) {
//         return(-1); // no error message, just don't do it.
//     }

//     // LOAD TO MEMORY -----------
//     mp3dec_t mp3d;
//     mp3dec_file_info_t info;

//     // qDebug() << "***** ABOUT TO TRY TO LOAD JUST ONE FRAME *****";
//     if (mp3dec_load_ONE(&mp3d, pathToMP3.toStdString().c_str(), &info, NULL, NULL))
//     {
//         qDebug() << "ERROR 79: mp3dec_load()";
//         return(-1);
//     }

//     int sampleRate = info.hz;

//     free(info.buffer);

//     // qDebug() << "sampleRate = " << info.hz << info.channels << info.samples << info.avg_bitrate_kbps;
//     // qDebug() << "***** DONE LOADING *****";

//     return(sampleRate);
// }

// ---------------------------------------------------------------------------------
QString MainWindow::SongFileIdentifier(QString pathToSong) {
    Q_UNUSED(pathToSong)

    // LOAD TO MEMORY -----------
    mp3dec_t mp3d;
    mp3dec_file_info_t info;

    if (mp3dec_load(&mp3d, pathToSong.toStdString().c_str(), &info, NULL, NULL))
    {
        qDebug() << "ERROR: mp3dec_load";
        return(QString("0xFFFFFFFF"));  // ERROR: -1
    }

    qDebug() << "SongFileID: " << info.samples << info.channels << info.hz << sizeof(mp3d_sample_t);

    // NOTE: THIS HASH ALGORITHM IS FOR LITTLE-ENDIAN MACHINES ONLY, e.g. Intel, ARM *********
    // On an M1 mac, this hash takes only about 20ms for 31836672 * 4 bytes = 6400MB/s (!)

    uint64_t myseed   = 0;
    uint64_t numBytes = info.samples * sizeof(mp3d_sample_t);
    uint64_t result2  = XXHash64::hash(info.buffer, numBytes, myseed);

    QString hexHash = QString::number(result2, 16); // QString of exactly 16 hex characters
    // QString hexHash = "0123456789abcdef";  // DEBUG DEBUG

    free(info.buffer);

    return(hexHash);
}

// ID3 ----------------------

// read ID3Tags from an MP3 or WAV file
//   if file is something else, returns -1 result
int MainWindow::readID3Tags(QString fileName, double *bpm, double *tbpm, uint32_t *loopStartSamples, uint32_t *loopLengthSamples) {

    qDebug() << "readID3Tags:" << fileName << *bpm << *tbpm << *loopStartSamples << *loopLengthSamples;

    if (!fileName.endsWith(".mp3", Qt::CaseInsensitive) && !fileName.endsWith(".wav", Qt::CaseInsensitive)) {
        return(-1);
    }

    TagLib::File *theFile;
    ID3v2::Tag *id3v2tag = nullptr;  // NULL if it doesn't have a tag, otherwise the address of the tag

    // get the ID3v2 tag ---------
    if (fileName.endsWith(".mp3", Qt::CaseInsensitive)) {
        theFile = new MPEG::File(fileName.toStdString().c_str());
        id3v2tag = ((MPEG::File *)theFile)->ID3v2Tag(true);
    } else if ((fileName.endsWith(".wav", Qt::CaseInsensitive))) {
        theFile = new TagLib::RIFF::WAV::File(fileName.toStdString().c_str());
        id3v2tag = ((TagLib::RIFF::WAV::File *)theFile)->ID3v2Tag();
    } else {
        return(-1);
    }

    // set all to "not present"
    *bpm = 0.0;
    *tbpm = 0.0;
    *loopStartSamples = 0;
    *loopLengthSamples = 0;

    // iterate over each Frame in the Tag --------
    ID3v2::FrameList::ConstIterator it = id3v2tag->frameList().begin();
    for (; it != id3v2tag->frameList().end(); it++)
    {
        ByteVector b = (*it)->frameID();

        QString theKey; // OK, we're gonna have to do this the hard way, eh?
        for (unsigned int i = 0; i < b.size(); i++) {
            theKey.append(b.at(i));
        }

        // QString theValue((*it)->toString().toCString());
        // qDebug() << "DEBUG: " << b.size() << theKey << ":" << theValue;

        if (theKey == "TXXX") // User Text frame <-- use these for LOOPSTART/LOOPLENGTH
        {
            TagLib::ID3v2::UserTextIdentificationFrame *tf = (TagLib::ID3v2::UserTextIdentificationFrame *)(*it);
            QString description = tf->description().toCString();
            // qDebug() << "DESCRIPTION:" << description;
            if (description == "LOOPSTART") {
                // qDebug() << "TXXX:" << description << ":" << tf->fieldList()[1].toCString();
                QString strValue = tf->fieldList()[1].toCString();
                uint32_t theLoopStart = strValue.toInt();
                *loopStartSamples = theLoopStart;
                // qDebug() << "found LOOPSTART:" << *loopStartSamples;
            } else if (description == "LOOPLENGTH") {
                // qDebug() << "TXXX:" << description << ":" << tf->fieldList()[1].toCString();
                QString strValue = tf->fieldList()[1].toCString();
                uint32_t theLoopLength = strValue.toInt();
                *loopLengthSamples = theLoopLength;
                // qDebug() << "found LOOPLENGTH:" << *loopLengthSamples;
            }
        } else if ((*it)->frameID() == "TBPM") {
            QString TBPM((*it)->toString().toCString());
            double theTBPM = TBPM.toDouble();
            *tbpm = theTBPM;
            // qDebug() << "found TBPM:" << *tbpm;
        } else if ((*it)->frameID() == "BPM") {
            QString BPM((*it)->toString().toCString());
            double theBPM = BPM.toDouble();
            *bpm = theBPM;
            qDebug() << "found BPM:" << *bpm;
        }

        // COMM not currently used.
        // if (theKey == "COMM") // comment (this will NOT be needed, it's sample code for now.)
        // {
        //     TagLib::ID3v2::CommentsFrame *cf = (TagLib::ID3v2::CommentsFrame *)(*it);
        //     qDebug() << "COMM:" << cf->description().toCString() << ":" << cf->text().toCString();
        // }

    }

    return(0);
}

// ------------------------------------------------------------------------
// write ID3Tags: WARNING! THIS WILL OVERWRITE THE FILE ******
//   if file is not MP3 or WAV, returns -1 result
int MainWindow::writeID3Tags(QString fileName, double *bpm, double *tbpm, uint32_t *loopStartSamples, uint32_t *loopLengthSamples) {

    qDebug() << "writeID3Tags:" << fileName << *bpm << *tbpm << *loopStartSamples << *loopLengthSamples;

    if (!fileName.endsWith(".mp3", Qt::CaseInsensitive) && !fileName.endsWith(".wav", Qt::CaseInsensitive)) {
        return(-1);
    }

    return(0);
}

// ------------------------------------------------------------------------
int MainWindow::audioFileSampleRate(QString fileName) {
    int sampleRate = -1;

    TagLib::FileRef f(fileName.toUtf8().constData());

    if (!f.isNull()) {
        TagLib::AudioProperties *properties = f.audioProperties();
        sampleRate = properties->sampleRate();
    }

    return(sampleRate);
}

// --------------------------------------
// Extract TBPM tag from ID3v2 to know (for sure) what the BPM is for a song (overrides beat detection) -----
// double MainWindow::getID3BPM(QString MP3FileName) {
//     // MPEG::File *mp3file;
//     ID3v2::Tag *id3v2tag = nullptr;  // NULL if it doesn't have a tag, otherwise the address of the tag

//     // mp3file = new MPEG::File(MP3FileName.toStdString().c_str()); // FIX: this leaks on read of another file
//     // id3v2tag = mp3file->ID3v2Tag(true);  // if it doesn't have one, create one

//     // qDebug() << "getID3BPM filename:" << MP3FileName;

//     TagLib::File *theFile;
//     QString fileType = "UNKNOWN";

//     if (MP3FileName.endsWith(".mp3", Qt::CaseInsensitive)) {
//         theFile = new MPEG::File(MP3FileName.toStdString().c_str());
//         id3v2tag = ((MPEG::File *)theFile)->ID3v2Tag(true);
//         fileType = "MP3";
//     } else if ((MP3FileName.endsWith(".wav", Qt::CaseInsensitive))) {
//         theFile = new TagLib::RIFF::WAV::File(MP3FileName.toStdString().c_str());
//         id3v2tag = ((TagLib::RIFF::WAV::File *)theFile)->ID3v2Tag();
//         fileType = "WAV";
//     } else {
//         return(0.0);
//     }

//     double theBPM = 0.0;

//     ID3v2::FrameList::ConstIterator it = id3v2tag->frameList().begin();
//     for (; it != id3v2tag->frameList().end(); it++)
//     {
//         ByteVector b = (*it)->frameID();

//         QString theKey;
//         for (unsigned int i = 0; i < b.size(); i++) {
//             theKey.append(b.at(i));
//         }

//         if ((*it)->frameID() == "TBPM" ||  // This is an Apple standard, which means it's everybody's standard now.
//             (*it)->frameID() == "BPM")     // This is what's used elsewhere. Whichever is found LAST wins.
//         {
//             // qDebug() << "TBPM/BPM:" << ":" << theValue;
//             QString BPM((*it)->toString().toCString());
//             theBPM = BPM.toDouble();
//         }

//     }

//     qDebug() << "getID3BPM filename: " << MP3FileName << "BPM: " << theBPM;
//     return(theBPM);
// }
