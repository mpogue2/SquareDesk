/****************************************************************************
**
** Copyright (C) 2016-2025 Mike Pogue, Dan Lyke
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

#include "ui_mainwindow.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop

#include "updateid3tagsdialog.h"
#include "ui_updateID3TagsDialog.h"

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
#include <taglib/audioproperties.h>

#define BADCOLOR "color:red;text-decoration:line-through;"
#define GOODCOLOR "color:chartreuse"
#define NEUTRALCOLOR "color:chartreuse"

// -------------------------------------------------------------------
updateID3TagsDialog::updateID3TagsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::updateID3TagsDialog) {
    mw = (MainWindow *)parent;

    ui->setupUi(this);

    currentBPM = 0.0;
    currentTBPM = 0.0;
    newTBPM = 0;
    newTBPMValid = false;

    currentLoopStart = 0;
    newLoopStart = 0;
    newLoopStartValid = false;

    currentLoopLength = 0;
    newLoopLength = 0;
    newLoopLengthValid = false;

    mw->readID3Tags(mw->currentMP3filenameWithPath, &currentBPM, &currentTBPM, &currentLoopStart, &currentLoopLength);

    // qDebug() << "updateID3TagsDialog constructor:" << mw->currentMP3filenameWithPath << currentBPM << currentTBPM << currentLoopStart << currentLoopLength;

    QString modifyFileName = mw->currentMP3filenameWithPath;
    modifyFileName.replace(mw->musicRootPath + "/", "");
    QString backupFileName = modifyFileName;

    backupFileName.replace(".mp3", ".bu.mp3", Qt::CaseInsensitive);
                  // .replace(".wav", ".bu.wav", Qt::CaseInsensitive);

    ui->fileNameToModify->setText(modifyFileName);
    ui->fileNameToBackupTo->setText(backupFileName);

    // get misc info about the source file
    TagLib::FileRef f((mw->currentMP3filenameWithPath).toUtf8().constData());
    int sampleRate = 0;
    int channels = 0;
    QString stereoMono = "unk";
    QString fileType = "unk";
    QString sampleRateKHz = "unk";

    if (!f.isNull()) {
        TagLib::AudioProperties *properties = f.audioProperties();

        sampleRate = properties->sampleRate();
        sampleRateKHz = QString::number((double)sampleRate/1000.0, 'f', 1).replace(".0", "");

        channels = properties->channels();

        if (channels == 1) {
            stereoMono = "Mono";
        } else if (channels == 2) {
            stereoMono = "Stereo";
        } else {
            stereoMono = QString::number(channels) + " channels";
        }

        if (modifyFileName.endsWith(".mp3", Qt::CaseInsensitive)) {
            fileType = "MPEG Level 3";
        }
    }

    ui->songInfoLabel->setText("[" + fileType + ", " + sampleRateKHz + "KHz, " + stereoMono + "]");

    if (currentTBPM == 0) {
        ui->currentTBPMLabel->setText("-");
    } else {
        ui->currentTBPMLabel->setText(QString::number(currentTBPM));
    }

    if (currentLoopStart == 0) {
        ui->currentLoopStartLabel->setText("-");
    } else {
        ui->currentLoopStartLabel->setText(QString::number(currentLoopStart));
    }

    if (currentLoopLength == 0) {
        ui->currentLoopLengthLabel->setText("-");
    } else {
        ui->currentLoopLengthLabel->setText(QString::number(currentLoopLength));
    }

    // ==========

    // double introTime = ((MainWindow *)mw)->ui->darkSeekBar->getIntro();
    // double outroTime = ((MainWindow *)mw)->ui->darkSeekBar->getOutro();

    // qDebug() << "introTime/outroTime:" << introTime << outroTime;

    QTime currentIntroTime = mw->ui->darkStartLoopTime->time();
    double currentIntroTimeSec = 60.0*currentIntroTime.minute() + currentIntroTime.second() + currentIntroTime.msec()/1000.0;

    QTime currentOutroTime = mw->ui->darkEndLoopTime->time();
    double currentOutroTimeSec = 60.0*currentOutroTime.minute() + currentOutroTime.second() + currentOutroTime.msec()/1000.0;

    // int sampleRate = mw->getMP3SampleRate(mw->currentMP3filenameWithPath);

    // qDebug() << "introTime/outroTime:" << currentIntroTimeSec << currentOutroTimeSec << sampleRate;

    if (sampleRate != -1) {
        uint32_t newIntroTimeSamples = currentIntroTimeSec * sampleRate;
        uint32_t newOutroTimeSamples = currentOutroTimeSec * sampleRate;

        // qDebug() << "introTimeSamples/outroTimeSamples:" << newIntroTimeSamples << newOutroTimeSamples;

        ui->newLoopStartEditBox->setText(QString::number(newIntroTimeSamples));
        on_newLoopStartEditBox_textChanged(QString::number(newIntroTimeSamples));

        ui->newLoopLengthEditBox->setText(QString::number((newOutroTimeSamples-newIntroTimeSamples)));
        on_newLoopLengthEditBox_textChanged(QString::number((newOutroTimeSamples-newIntroTimeSamples)));
    } else {
        ui->newLoopStartEditBox->setText("-");
        ui->newLoopLengthEditBox->setText("-");
        on_newLoopStartEditBox_textChanged("-");
        on_newLoopLengthEditBox_textChanged("-");
    }

    // qDebug() << "TEMPO SLIDER DEFAULT VALUE: " << mw->ui->darkTempoSlider->getDefaultValue();
    // QString newTempo = mw->ui->darkTempoLabel->text();
    QString newTempo = QString::number(mw->ui->darkTempoSlider->getDefaultValue());
    // qDebug() << "newTempo: " << newTempo;

    if (mw->ui->darkTempoLabel->text().endsWith("%")) {
        // qDebug() << "NEW TEMPO ENDS IN %" << newTempo;
        ui->newTBPMEditBox->setText(ui->currentTBPMLabel->text()); // use same as old
        on_newTBPMEditBox_textChanged(ui->currentTBPMLabel->text());
    } else {
        // qDebug() << "SETTING NEW TEMPO TO: " << newTempo;
        ui->newTBPMEditBox->setText(newTempo);
        on_newTBPMEditBox_textChanged(newTempo);
    }
}

// ----------------------------------------------------------------
updateID3TagsDialog::~updateID3TagsDialog()
{
    delete ui;
}

void updateID3TagsDialog::on_newTBPMEditBox_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1)

    int16_t proposedTBPM = arg1.toInt(&newTBPMValid);
    uint8_t allowedBPMspread = 15; // needs to match mainWindow.cpp:10487

    // qDebug() << "newTBPMValid: " << arg1 << proposedTBPM << newTBPMValid;

    newTBPM = proposedTBPM; // VALID

    if (!newTBPMValid ||
        proposedTBPM < 125 - allowedBPMspread ||
        proposedTBPM > 125 + allowedBPMspread) {
        ui->newTBPMEditBox->setStyleSheet(BADCOLOR);
        newTBPM = 0; // NOPE, INVALID
    } else if (ui->newTBPMEditBox->text() == ui->currentTBPMLabel->text()) {
        ui->newTBPMEditBox->setStyleSheet(NEUTRALCOLOR);
    } else {
        ui->newTBPMEditBox->setStyleSheet(GOODCOLOR);
    }
    // only enable the OK button, if everything looks good
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(newTBPMValid && newLoopStartValid && newLoopLengthValid);
}

extern flexible_audio *cBass;  // make this accessible

void updateID3TagsDialog::on_newLoopStartEditBox_textChanged(const QString &arg1)
{
    int32_t startPoint_samples = arg1.toLong(&newLoopStartValid);
    uint32_t sampleRate = mw->currentSongMP3SampleRate;
    int32_t songLength_samples = (int32_t)(cBass->FileLength * sampleRate);

    // qDebug() << "STARTLOOP:" << startPoint_samples << sampleRate << songLength_samples;

    newLoopStart = startPoint_samples; // VALID

    if (!newLoopStartValid ||
        startPoint_samples < 0 ||
        startPoint_samples > songLength_samples) {
        // start before the beginning of the song, or after the end of the song
        ui->newLoopStartEditBox->setStyleSheet(BADCOLOR);
        newLoopStart = 0; // NOPE: INVALID *****
    } else if (ui->newLoopStartEditBox->text() == ui->currentLoopStartLabel->text()) {
        ui->newLoopStartEditBox->setStyleSheet(NEUTRALCOLOR);
    } else {
        // it's valid, and it's different from what's there already
        ui->newLoopStartEditBox->setStyleSheet(GOODCOLOR);
    }

    double startPoint = (double)startPoint_samples / (double)sampleRate;
    QString startPointString = "= " + QString::number(startPoint, 'f', 5) + "s";
    ui->newLoopStartSecondsLabel->setText(startPointString); // exactly 5 digits past the decimal pt
                                                             // for 1/48000 sec accuracy (sample accurate)

    // when the start point changes, it can also invalidate the length indirectly....
    // This will also recalculate the loop end seconds endpoint
    on_newLoopLengthEditBox_textChanged(ui->newLoopLengthEditBox->text());

    // only enable the OK button, if everything looks good
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(newTBPMValid && newLoopStartValid && newLoopLengthValid);
}


void updateID3TagsDialog::on_newLoopLengthEditBox_textChanged(const QString &arg1)
{
    int32_t startPoint_samples = ui->newLoopStartEditBox->text().toLong(&newLoopLengthValid);
    int32_t loopLength_samples = arg1.toLong();
    int32_t loopEnd_samples = startPoint_samples + loopLength_samples;
    uint32_t sampleRate = mw->currentSongMP3SampleRate;
    double loopEnd_seconds = (double)loopEnd_samples / (double)sampleRate;
    int32_t songLength_samples = (uint32_t)(cBass->FileLength * sampleRate);

    // qDebug() << "LENGTHLOOP:" << loopLength_samples << sampleRate << songLength_samples;

    newLoopLength = loopLength_samples; // VALID

    if (!newLoopLengthValid ||
        loopLength_samples < 0 ||
        startPoint_samples + loopLength_samples - 1 > songLength_samples) {
        // loop end beyond the end of the song
        ui->newLoopLengthEditBox->setStyleSheet(BADCOLOR);
        newLoopLength = 0; // NOPE: INVALID *****
    } else if (ui->newLoopLengthEditBox->text() == ui->currentLoopLengthLabel->text()) {
        ui->newLoopLengthEditBox->setStyleSheet(NEUTRALCOLOR);
    } else {
        // it's valid, and it's different from what's there already
        ui->newLoopLengthEditBox->setStyleSheet(GOODCOLOR);
        newLoopLength = loopLength_samples; // VALID
    }

    double lengthSeconds = (double)loopLength_samples / (double)sampleRate;
    QString lengthSecondsString = "= " + QString::number(lengthSeconds, 'f', 5) + "s";
    ui->newLoopLengthSecondsLabel->setText(lengthSecondsString); // exactly 5 digits past the decimal pt
                                                                 // for 1/48000 sec accuracy (sample accurate)

    QString loopEndSecondsString = "= " + QString::number(loopEnd_seconds, 'f', 5) + "s";
    ui->newLoopEndSecondsLabel->setText(loopEndSecondsString);  // exactly 5 digits past the decimal pt
                                                                // for 1/48000 sec accuracy (sample accurate)

    // only enable the OK button, if everything looks good
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(newTBPMValid && newLoopStartValid && newLoopLengthValid);
}
