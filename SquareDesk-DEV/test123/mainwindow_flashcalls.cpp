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
// Disable warning, see: https://github.com/llvm/llvm-project/issues/48757
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop

#include "ui_mainwindow.h"
#include <QGraphicsItemGroup>
#include <QGraphicsTextItem>
#include <QCoreApplication>
#include <QInputDialog>
#include <QLineEdit>
#include <QClipboard>
#include <QtSvg/QSvgGenerator>
#include <algorithm>  // for random_shuffle

// ----------------------------------------------------------------------
void MainWindow::on_flashcallbasic_toggled(bool checked)
{
    ui->actionFlashCallBasic->setChecked(checked);

    // the Flash Call settings are persistent across restarts of the application
    prefsManager.Setflashcallbasic(ui->actionFlashCallBasic->isChecked());
}

void MainWindow::on_actionFlashCallBasic_triggered()
{
    on_flashcallbasic_toggled(ui->actionFlashCallBasic->isChecked());
    readFlashCallsList();
}

void MainWindow::on_flashcallmainstream_toggled(bool checked)
{
    ui->actionFlashCallMainstream->setChecked(checked);

    // the Flash Call settings are persistent across restarts of the application
    prefsManager.Setflashcallmainstream(ui->actionFlashCallMainstream->isChecked());
}

void MainWindow::on_actionFlashCallMainstream_triggered()
{
    on_flashcallmainstream_toggled(ui->actionFlashCallMainstream->isChecked());
    readFlashCallsList();
}

void MainWindow::on_flashcallplus_toggled(bool checked)
{
    ui->actionFlashCallPlus->setChecked(checked);

    // the Flash Call settings are persistent across restarts of the application
    prefsManager.Setflashcallplus(ui->actionFlashCallPlus->isChecked());
}

void MainWindow::on_actionFlashCallPlus_triggered()
{
    on_flashcallplus_toggled(ui->actionFlashCallPlus->isChecked());
    readFlashCallsList();
}

void MainWindow::on_flashcalla1_toggled(bool checked)
{
    ui->actionFlashCallA1->setChecked(checked);

    // the Flash Call settings are persistent across restarts of the application
    prefsManager.Setflashcalla1(ui->actionFlashCallA1->isChecked());
}

void MainWindow::on_actionFlashCallA1_triggered()
{
    on_flashcalla1_toggled(ui->actionFlashCallA1->isChecked());
    readFlashCallsList();
}

void MainWindow::on_flashcalla2_toggled(bool checked)
{
    ui->actionFlashCallA2->setChecked(checked);

    // the Flash Call settings are persistent across restarts of the application
    prefsManager.Setflashcalla2(ui->actionFlashCallA2->isChecked());
}

void MainWindow::on_actionFlashCallA2_triggered()
{
    on_flashcalla2_toggled(ui->actionFlashCallA2->isChecked());
    readFlashCallsList();
}

void MainWindow::on_flashcallc1_toggled(bool checked)
{
    ui->actionFlashCallC1->setChecked(checked);

    // the Flash Call settings are persistent across restarts of the application
    prefsManager.Setflashcallc1(ui->actionFlashCallC1->isChecked());
}

void MainWindow::on_actionFlashCallC1_triggered()
{
    on_flashcallc1_toggled(ui->actionFlashCallC1->isChecked());
    readFlashCallsList();
}

void MainWindow::on_flashcallc2_toggled(bool checked)
{
    ui->actionFlashCallC2->setChecked(checked);

    // the Flash Call settings are persistent across restarts of the application
    prefsManager.Setflashcallc2(ui->actionFlashCallC2->isChecked());
}

void MainWindow::on_actionFlashCallC2_triggered()
{
    on_flashcallc2_toggled(ui->actionFlashCallC2->isChecked());
    readFlashCallsList();
}

void MainWindow::on_flashcallc3a_toggled(bool checked)
{
    ui->actionFlashCallC3a->setChecked(checked);

    // the Flash Call settings are persistent across restarts of the application
    prefsManager.Setflashcallc3a(ui->actionFlashCallC3a->isChecked());
}

void MainWindow::on_actionFlashCallC3a_triggered()
{
    on_flashcallc3a_toggled(ui->actionFlashCallC3a->isChecked());
    readFlashCallsList();
}

void MainWindow::on_flashcallc3b_toggled(bool checked)
{
    ui->actionFlashCallC3b->setChecked(checked);

    // the Flash Call settings are persistent across restarts of the application
    prefsManager.Setflashcallc3b(ui->actionFlashCallC3b->isChecked());
}

void MainWindow::on_actionFlashCallC3b_triggered()
{
    on_flashcallc3b_toggled(ui->actionFlashCallC3b->isChecked());
    readFlashCallsList();
}

void MainWindow::on_flashcalluserfile_toggled(bool checked)
{
    ui->actionFlashCallUserFile->setChecked(checked);

    // the Flash Call settings are persistent across restarts of the application
    prefsManager.Setflashcalluserfile(ui->actionFlashCallUserFile->isChecked());
}

void MainWindow::on_actionFlashCallUserFile_triggered()
{
    on_flashcalluserfile_toggled(ui->actionFlashCallUserFile->isChecked());
    readFlashCallsList();
}

void MainWindow::on_actionFlashCallFilechooser_triggered()
{
    // on_flashcallfilechooser_toggled(ui->actionFlashCallFilechooser->isChecked());
    selectUserFlashFile();      // choose a file
    readFlashCallsList();       // if there was one, read it
}


// -----
void MainWindow::updateFlashFileMenu() {
    QString menuItemDisplay;
    if (lastFlashcardsUserFile == "") {
        menuItemDisplay = "No flashcards file selected";
        ui->actionFlashCallUserFile->setEnabled(false);
        // ensure it isn't checked!
        ui->actionFlashCallUserFile->setChecked(false);
    } else {
        menuItemDisplay = "File: " + lastFlashcardsUserFile;
        QFileInfo fi(lastFlashcardsUserFile);
        menuItemDisplay = "File: " + fi.completeBaseName();
        ui->actionFlashCallUserFile->setEnabled(true);
    }
    ui->actionFlashCallUserFile->setText(menuItemDisplay);
}

void MainWindow::selectUserFlashFile() {
    QString defaultDir = lastFlashcardsUserDirectory;
    if (!QFile::exists(defaultDir)) {
        defaultDir = QDir::homePath();
    }

    trapKeypresses = false;
    QString flashcardsFileName =
        QFileDialog::getOpenFileName(this,
                                     tr("Load Flashcards"),
                                     defaultDir,
                                     tr("Flashcards Files (*.txt)"));
    trapKeypresses = true;

    if (flashcardsFileName.isNull()) {
        // user cancelled, so don't change anything
        return;
    }
    QFileInfo fi(flashcardsFileName);
    lastFlashcardsUserFile = fi.fileName();
    lastFlashcardsUserDirectory = fi.absolutePath();
    // Save it in Settings...
    prefsManager.Setlastflashcalluserfile(lastFlashcardsUserFile);
    prefsManager.Setlastflashcalluserdirectory(lastFlashcardsUserDirectory);

    // ...and display it in the menu
    updateFlashFileMenu();
}

void MainWindow::readFlashCallsList() {
#if defined(Q_OS_MAC)
    QString appPath = QApplication::applicationFilePath();
    QString allcallsPath = appPath + "/Contents/Resources/allcalls.csv";
    allcallsPath.replace("Contents/MacOS/SquareDesk/","");
#elif defined(Q_OS_WIN32)
    // TODO: There has to be a better way to do this.
    QString appPath = QApplication::applicationFilePath();
    QString allcallsPath = appPath + "/allcalls.csv";
    allcallsPath.replace("SquareDesk.exe/","");
#else
    QString allcallsPath = "allcalls.csv";
    // Linux
    QStringList paths;
    paths.append(QApplication::applicationFilePath());
    paths.append("/usr/share/SquareDesk");
    paths.append(".");

    for (auto path : paths)
    {
        QString filename(path + "/allcalls.csv");
        QFileInfo check_file(filename);
        if (check_file.exists() && check_file.isFile())
        {
            allcallsPath = filename;
            break;
        }
    }
#endif


    flashCalls.clear();  // remove all calls, let's read them in again

    lastFlashcardsUserFile = prefsManager.Getlastflashcalluserfile();
    lastFlashcardsUserDirectory = prefsManager.Getlastflashcalluserdirectory();
    if (lastFlashcardsUserFile != "" &&
        (ui->actionFlashCallUserFile->isChecked())) {

        // There is a user flash card file, so read it

        QFile file(lastFlashcardsUserDirectory + "/" + lastFlashcardsUserFile);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // qDebug() << "Could not open " << lastFlashcardsUserFile;
            // clear the name
            lastFlashcardsUserFile = "";
            prefsManager.Setlastflashcalluserfile(lastFlashcardsUserFile);
            updateFlashFileMenu();
        } else {
            while (!file.atEnd()) {
                QString line = file.readLine().simplified();
                if (!line.startsWith("#")) {
                    flashCalls.append(line);
                }
            }
            file.close();
        }
    }


    // Now look check for the calls from relevant levels

    QFile file(allcallsPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Could not open 'allcalls.csv' file. Flash calls will not work.";
        qDebug() << "looked here:" << allcallsPath;
        return;
    }

    while (!file.atEnd()) {
        QString line = file.readLine().simplified();
        QStringList lineparts = line.split(',');
        QString level = lineparts[0];
        QString call = lineparts[1].replace("\"","");

        if ((level == "basic" && ui->actionFlashCallBasic->isChecked()) ||
                (level == "ms" && ui->actionFlashCallMainstream->isChecked()) ||
                (level == "plus" && ui->actionFlashCallPlus->isChecked()) ||
                (level == "a1" && ui->actionFlashCallA1->isChecked()) ||
                (level == "a2" && ui->actionFlashCallA2->isChecked()) ||
                (level == "c1" && ui->actionFlashCallC1->isChecked()) ||
                (level == "c2" && ui->actionFlashCallC2->isChecked()) ||
                (level == "c3a" && ui->actionFlashCallC3a->isChecked()) ||
                (level == "c3b" && ui->actionFlashCallC3b->isChecked()) ) {
            flashCalls.append(call);
        }
    }
//    qDebug() << "Flash calls" << flashCalls;
//    qsrand(static_cast<uint>(QTime::currentTime().msec()));  // different random sequence of calls each time, please.
    if (flashCalls.length() == 0) {
        randCallIndex = 0;
    } else {
//        randCallIndex = qrand() % flashCalls.length();   // start out with a number other than zero, please.
        randCallIndex = QRandomGenerator::global()->bounded(flashCalls.length());   // start out with a number other than zero, please.
    }

//    qDebug() << "flashCalls: " << flashCalls;
//    qDebug() << "randCallIndex: " << randCallIndex;
}

// ===========================================
void MainWindow::on_action5_seconds_triggered()
{
    prefsManager.Setflashcalltiming("5");
}

void MainWindow::on_action10_seconds_triggered()
{
    prefsManager.Setflashcalltiming("10");
}

void MainWindow::on_action15_seconds_triggered()
{
    prefsManager.Setflashcalltiming("15");
}

void MainWindow::on_action20_seconds_triggered()
{
    prefsManager.Setflashcalltiming("20");
}

