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

#include <QtWidgets>
#include <QDebug>
#include <QDir>
#include "prefsmanager.h"

#include "startupwizard.h"

// --------------------------------------
StartupWizard::StartupWizard(QWidget *parent)
    : QWizard(parent)
{
    addPage(new IntroPage);
    addPage(new OutputFilesPage);
    addPage(new CopyFilesPage);
    addPage(new ConclusionPage);

#if !defined(Q_OS_MAC)
//    setPixmap(QWizard::BannerPixmap, QPixmap(":/startupwizardimages/banner.png"));
//    setPixmap(QWizard::BackgroundPixmap, QPixmap(":/startupwizardimages/background.png"));
    setPixmap(QWizard::WatermarkPixmap, QPixmap(":/startupwizardimages/watermark1.png"));
#endif

    setWindowTitle(tr("Startup Wizard"));
}

// --------------------------------------
void StartupWizard::accept()
{
    // make the top level Music Directory
    QString outputDir = field("outputDir").toString();
//    qDebug() << "Making " << outputDir;
    QDir dir(outputDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString patterDirName = outputDir + "/" + field("patterDirName").toString();
//    qDebug() << "Making " << patterDirName;
    QDir dir2(patterDirName);
    if (!dir2.exists()) {
        dir2.mkpath(".");
    }

    QString singingCallsDirName = outputDir + "/" + field("singingCallsDirName").toString();
//    qDebug() << "Making " << singingCallsDirName;
    QDir dir3(singingCallsDirName);
    if (!dir3.exists()) {
        dir3.mkpath(".");
    }

    QString vocalsDirName = outputDir + "/" + field("vocalsDirName").toString();
//    qDebug() << "Making " << vocalsDirName;
    QDir dir4(vocalsDirName);
    if (!dir4.exists()) {
        dir4.mkpath(".");
    }

    QString extrasDirName = outputDir + "/" + field("extrasDirName").toString();
//    qDebug() << "Making " << extrasDirName;
    QDir dir5(extrasDirName);
    if (!dir5.exists()) {
        dir5.mkpath(".");
    }

    // always make this directory for soundfx
    QString sfxDirName = outputDir + "/soundfx";
//    qDebug() << "Making " << sdDirName;
    QDir dir5b(sfxDirName);
    if (!dir5b.exists()) {
        dir5b.mkpath(".");
    }

    // always make this directory for sd
    QString sdDirName = outputDir + "/sd";
//    qDebug() << "Making " << sdDirName;
    QDir dir6(sdDirName);
    if (!dir6.exists()) {
        dir6.mkpath(".");
    }

    // always make this directory for playlists
    QString playlistsDirName = outputDir + "/playlists";
//    qDebug() << "Making " << playlistsDirName;
    QDir dir7(playlistsDirName);
    if (!dir7.exists()) {
        dir7.mkpath(".");
    }

    // always make this directory for lyrics
    QString lyricsDirName = outputDir + "/lyrics";
//    qDebug() << "Making " << playlistsDirName;
    QDir dir75(lyricsDirName);
    if (!dir75.exists()) {
        dir75.mkpath(".");
    }

    // always make this directory for internal housekeeping files
    QString squaredeskDirName = outputDir + "/.squaredesk";
//    qDebug() << "Making " << squaredeskDirName;
    QDir dir8(squaredeskDirName);
    if (!dir8.exists()) {
        dir8.mkpath(".");
    }

    // TODO: factor that stuff into a single function...
    // TODO: progress bar

    // copy files (it's OK if there are zero files to copy from a certain fromDir) -------------
    if (field("patterDoCopy").toBool()) {
        copyMP3FilesFromTo(QDir(field("patterFrom").toString()), patterDirName);
    }

    if (field("singingCallsDoCopy").toBool()) {
        copyMP3FilesFromTo(QDir(field("singingCallsFrom").toString()), singingCallsDirName);
    }

    if (field("vocalsDoCopy").toBool()) {
        copyMP3FilesFromTo(QDir(field("vocalsFrom").toString()), vocalsDirName);
    }

    if (field("extrasDoCopy").toBool()) {
        copyMP3FilesFromTo(QDir(field("extrasFrom").toString()), extrasDirName);
    }

    // USER SAID "OK", SO HANDLE THE UPDATED PREFS ---------------
    PreferencesManager prefsManager;
    prefsManager.SetmusicPath(outputDir);
    prefsManager.SetMusicTypePatter(field("patterDirName").toString());
    prefsManager.SetMusicTypeSinging(field("singingCallsDirName").toString());
    prefsManager.SetMusicTypeCalled(field("vocalsDirName").toString());
    prefsManager.SetMusicTypeExtras(field("extrasDirName").toString());
    // NOTE: after we return to the MainWindow, the internal variables need to be updated from these new Preferences

    QDialog::accept();
}

// --------------------------------------
IntroPage::IntroPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Introduction"));

#if !defined(Q_OS_MAC)
    setPixmap(QWizard::WatermarkPixmap, QPixmap(":/startupwizardimages/watermark1.png"));
#endif

    label = new QLabel(tr("This wizard will help you set up a new SquareDesk 'Music Directory' "
                          "for your MP3 files.  It will also make some subdirectories, "
                          "so that SquareDesk can figure out what kinds of files "
                          "you have (for example, patter vs singing calls).  You can always change these names "
                          "later, using the Preferences dialog.\n\n"
                          "And, if you want to, you'll be able to copy patter and singing call MP3 files from existing "
                          "directories into the new Music Directory."));
    label->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    setLayout(layout);
}

// --------------------------------------
OutputFilesPage::OutputFilesPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Choose directory names"));
    setSubTitle(tr("Specify where you want the wizard to create a new 'Music Directory', and "
                   "what you want the music subdirectory names to be.  Each subdirectory contains "
                   "MP3 files of the same musical type, e.g. 'patter' or 'singing calls'."));
#if !defined(Q_OS_MAC)
//    setPixmap(QWizard::LogoPixmap, QPixmap(":/startupwizardimages/logo3.png"));
    setPixmap(QWizard::WatermarkPixmap, QPixmap(":/startupwizardimages/watermark1.png"));
#endif

    outputDirLabel = new QLabel("New Music Directory:");
    outputDirLineEdit = new QLineEdit;
    outputDirLabel->setBuddy(outputDirLineEdit);

    patterLabel = new QLabel("   Patter subdirectory:");
    patterLineEdit = new QLineEdit;
    patterLabel->setBuddy(patterLineEdit);

    singingCallLabel = new QLabel("   Singing call subdirectory:");
    singingCallLineEdit = new QLineEdit;
    singingCallLabel->setBuddy(singingCallLineEdit);

    vocalsLabel = new QLabel("   Singing calls (called) subdirectory:");
    vocalsLineEdit = new QLineEdit;
    vocalsLabel->setBuddy(vocalsLineEdit);

    extrasLabel = new QLabel("   Extras (e.g. line dance) subdirectory:");
    extrasLineEdit = new QLineEdit;
    extrasLabel->setBuddy(extrasLineEdit);

    registerField("outputDir", outputDirLineEdit);
    registerField("patterDirName", patterLineEdit);
    registerField("singingCallsDirName", singingCallLineEdit);
    registerField("vocalsDirName", vocalsLineEdit);
    registerField("extrasDirName", extrasLineEdit);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(outputDirLabel, 0, 0);
    layout->addWidget(outputDirLineEdit, 0, 1);
    layout->addWidget(patterLabel, 1, 0);
    layout->addWidget(patterLineEdit, 1, 1);
    layout->addWidget(singingCallLabel, 2, 0);
    layout->addWidget(singingCallLineEdit, 2, 1);
    layout->addWidget(vocalsLabel, 3, 0);
    layout->addWidget(vocalsLineEdit, 3, 1);
    layout->addWidget(extrasLabel, 4, 0);
    layout->addWidget(extrasLineEdit, 4, 1);
    setLayout(layout);
}

// --------------------------------------
void OutputFilesPage::initializePage()
{
    outputDirLineEdit->setText(QDir::toNativeSeparators(QDir::homePath() + QString("/SquareDeskMusic")));
    patterLineEdit->setText("patter");
    singingCallLineEdit->setText("singing");
    vocalsLineEdit->setText("vocals");
    extrasLineEdit->setText("xtras");
}

// --------------------------------------
// NOTE: GLOBAL
int countMP3FilesInDir(QDir rootDir) {
    QDirIterator it(rootDir, QDirIterator::NoIteratorFlags);  // non-recursive copy!
    int count = 0;
    while(it.hasNext()) {
        QString s1 = it.next();
        if (s1.right(4)==".mp3") {
//            qDebug() << "S1:" << s1;
            count++;
        }
    }
    return(count);
}

void copyMP3FilesFromTo(QDir fromDir, QDir toDir) {
    QDirIterator it(fromDir, QDirIterator::NoIteratorFlags);  // non-recursive copy!
    while(it.hasNext()) {
        QString fromFilename = it.next();
        QFileInfo fromFile(fromFilename);
        if (fromFilename.right(4)==".mp3") {
//            qDebug() << "copying from" << fromFilename << " to " << toDir.absolutePath() + "/" + fromFile.fileName();
            QFile::copy(fromFilename, toDir.absolutePath() + "/" + fromFile.fileName());
        }
    }
}

// --------------------------------------
CopyFilesPage::CopyFilesPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Copy MP3 Files into the Music Directory"));
    setSubTitle(tr("Specify where you want the wizard to copy MP3 files from. \n"
                   "NOTE: the original MP3 files in these directories will not be changed.\n\nLeave a checkbox "
                   "unchecked, if you don't want to copy in any files of that type."
                   ));
#if !defined(Q_OS_MAC)
//    setPixmap(QWizard::LogoPixmap, QPixmap(":/startupwizardimages/logo3.png"));
    setPixmap(QWizard::WatermarkPixmap, QPixmap(":/startupwizardimages/watermark1.png"));
#endif

#if defined(Q_OS_MAC)
    QFont labelFont("Arial", 12, -1, true); // for MAC
#else
    QFont labelFont("Arial", 8, -1, true);  // for WIN and LINUX
#endif
    patterCheckBox = new QCheckBox();
    patterLineEdit = new QLineEdit;
    patterLineEdit->setFont(labelFont);
    patterLineEdit->setEnabled(false);
    patterPushButton = new QPushButton("Choose...");
    patterLabel = new QLabel();

    singingCallCheckBox = new QCheckBox();
    singingCallLineEdit = new QLineEdit;
    singingCallLineEdit->setFont(labelFont);
    singingCallLineEdit->setEnabled(false);
    singingCallPushButton = new QPushButton("Choose...");

    vocalsCheckBox = new QCheckBox();
    vocalsLineEdit = new QLineEdit;
    vocalsLineEdit->setFont(labelFont);
    vocalsLineEdit->setEnabled(false);
    vocalsPushButton = new QPushButton("Choose...");

    extrasCheckBox = new QCheckBox();
    extrasLineEdit = new QLineEdit;
    extrasLineEdit->setFont(labelFont);
    extrasLineEdit->setEnabled(false);
    extrasPushButton = new QPushButton("Choose...");

    registerField("patterFrom", patterLineEdit);
    registerField("singingCallsFrom", singingCallLineEdit);
    registerField("vocalsFrom", vocalsLineEdit);
    registerField("extrasFrom", extrasLineEdit);

    registerField("patterDoCopy", patterCheckBox);
    registerField("singingCallsDoCopy", singingCallCheckBox);
    registerField("vocalsDoCopy", vocalsCheckBox);
    registerField("extrasDoCopy", extrasCheckBox);

    // ---------------
    connect(patterPushButton, &QAbstractButton::clicked,
            [=]() {
        QString dir =
                QFileDialog::getExistingDirectory(this, tr("Select Directory to copy Patter music from:"),
                                                  QDir::homePath(),
                                                  QFileDialog::ShowDirsOnly
                                                  | QFileDialog::DontResolveSymlinks);

        if (!dir.isNull()) {
            patterLineEdit->setText(dir);
            int count = countMP3FilesInDir(QDir(patterLineEdit->text()));
//            qDebug() << "COUNT:" << count;
            patterCheckBox->setText("Copy " + QString::number(count) + " files to '" + field("patterDirName").toString() + "' from:");
            patterCheckBox->setChecked(true);

//            qDebug() << "field:" << field("patterFrom").toString();  // DEBUG
        }
    }
    );

    // ---------------
    connect(singingCallPushButton, &QAbstractButton::clicked,
            [=]() {
        QString dir =
                QFileDialog::getExistingDirectory(this, tr("Select Directory to copy Singing Calls from:"),
                                                  QDir::homePath(),
                                                  QFileDialog::ShowDirsOnly
                                                  | QFileDialog::DontResolveSymlinks);

        if (!dir.isNull()) {
            singingCallLineEdit->setText(dir);
            int count = countMP3FilesInDir(QDir(singingCallLineEdit->text()));
//            qDebug() << "COUNT:" << count;
            singingCallCheckBox->setText("Copy " + QString::number(count) + " files to '" + field("singingCallsDirName").toString() + "' from: ");
            singingCallCheckBox->setChecked(true);
        }
    }
    );

    // ---------------
    connect(vocalsPushButton, &QAbstractButton::clicked,
            [=]() {
        QString dir =
                QFileDialog::getExistingDirectory(this, tr("Select Directory to copy Singing Calls (called) from: "),
                                                  QDir::homePath(),
                                                  QFileDialog::ShowDirsOnly
                                                  | QFileDialog::DontResolveSymlinks);

        if (!dir.isNull()) {
            vocalsLineEdit->setText(dir);
            int count = countMP3FilesInDir(QDir(vocalsLineEdit->text()));
//            qDebug() << "COUNT:" << count;
            vocalsCheckBox->setText("Copy " + QString::number(count) + " files to '" + field("vocalsDirName").toString() + "' from: ");
            vocalsCheckBox->setChecked(true);
        }
    }
    );

    // ---------------
    connect(extrasPushButton, &QAbstractButton::clicked,
            [=]() {
        QString dir =
                QFileDialog::getExistingDirectory(this, tr("Select Directory to copy Extras music from:"),
                                                  QDir::homePath(),
                                                  QFileDialog::ShowDirsOnly
                                                  | QFileDialog::DontResolveSymlinks);

        if (!dir.isNull()) {
            extrasLineEdit->setText(dir);
            int count = countMP3FilesInDir(QDir(extrasLineEdit->text()));
//            qDebug() << "COUNT:" << count;
            extrasCheckBox->setText("Copy " + QString::number(count) + " files to '" + field("extrasDirName").toString() + "' from: ");
            extrasCheckBox->setChecked(true);
        }
    }
    );

    // ---------------
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(patterCheckBox, 0, 0);
    layout->addWidget(patterLineEdit, 0, 1);
    layout->addWidget(patterPushButton, 0, 2);

    layout->addWidget(singingCallCheckBox, 1, 0);
    layout->addWidget(singingCallLineEdit, 1, 1);
    layout->addWidget(singingCallPushButton, 1, 2);

    layout->addWidget(vocalsCheckBox, 2, 0);
    layout->addWidget(vocalsLineEdit, 2, 1);
    layout->addWidget(vocalsPushButton, 2, 2);

    layout->addWidget(extrasCheckBox, 3, 0);
    layout->addWidget(extrasLineEdit, 3, 1);
    layout->addWidget(extrasPushButton, 3, 2);

    setLayout(layout);
}

// --------------------------------------
void CopyFilesPage::initializePage()
{
    patterCheckBox->setText("Copy '" + field("patterDirName").toString() + "' files from: ");
    singingCallCheckBox->setText("Copy '" + field("singingCallsDirName").toString() + "' files from: ");
    vocalsCheckBox->setText("Copy '" + field("vocalsDirName").toString() + "' files from: ");
    extrasCheckBox->setText("Copy '" + field("extrasDirName").toString() + "' files from: ");

    patterPushButton->setDefault(false);
    singingCallPushButton->setDefault(false);
    vocalsPushButton->setDefault(false);
    extrasPushButton->setDefault(false);

    patterPushButton->setFixedWidth(100);
    singingCallPushButton->setFixedWidth(100);
    vocalsPushButton->setFixedWidth(100);
    extrasPushButton->setFixedWidth(100);

    patterLineEdit->setText("<Choose a directory>");
    singingCallLineEdit->setText("<Choose a directory>");
    vocalsLineEdit->setText("<Choose a directory>");
    extrasLineEdit->setText("<Choose a directory>");
}

// --------------------------------------
ConclusionPage::ConclusionPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Final Confirmation"));

#if !defined(Q_OS_MAC)
//    setPixmap(QWizard::WatermarkPixmap, QPixmap(":/startupwizardimages/watermark2.png"));
    setPixmap(QWizard::WatermarkPixmap, QPixmap(":/startupwizardimages/watermark1.png"));
#endif

    label = new QLabel;
    label->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    setLayout(layout);
}

// --------------------------------------
void ConclusionPage::initializePage()
{
    QString count;
#if defined(Q_OS_MAC) | defined(Q_OS_LINUX)
    QString confirmText = "Click Done to make the directories, copy MP3 files, and update Preferences.\n\n";
#elif defined(Q_OS_WIN)
    QString confirmText = "Click Finish to make the directories, copy MP3 files, and update Preferences.\n\n";
#endif
    confirmText += "Directories to create:\n";
    confirmText += "   " + field("outputDir").toString() + " (main Music Directory)\n";
    confirmText += "   " + field("outputDir").toString() + "/" + field("patterDirName").toString() + " (for patter)\n";
    confirmText += "   " + field("outputDir").toString() + "/" + field("singingCallsDirName").toString() + " (for singing calls)\n";
    confirmText += "   " + field("outputDir").toString() + "/" + field("vocalsDirName").toString() + " (for vocals)\n";
    confirmText += "   " + field("outputDir").toString() + "/" + field("extrasDirName").toString() + " (for other music)\n";
    confirmText += "   " + field("outputDir").toString() + "/lyrics (for cuesheets and lyrics editor preferences)\n";
    confirmText += "   " + field("outputDir").toString() + "/soundfx (for sound effects)\n";
    confirmText += "   " + field("outputDir").toString() + "/playlists (for playlists)\n";
    confirmText += "   " + field("outputDir").toString() + "/sd (for sd output files)\n\n";
//    confirmText += "   " + field("outputDir").toString() + "/.squaredesk (for internal SquareDesk files)\n\n";  // this will be our little secret
    // Not so secret on Windows, though, is it?

    bool someFilesCopied = false;
    confirmText += "Files to copy:\n";
    if (field("patterDoCopy").toBool() && field("patterFrom").toString() != "<Choose a directory>") {
        count = QString::number(countMP3FilesInDir(QDir(field("patterFrom").toString())));
        confirmText += "   " + count + " MP3 files from " + field("patterFrom").toString() + " to '" + field("patterDirName").toString() + "'\n";
        someFilesCopied = true;
    }

    if (field("singingCallsDoCopy").toBool() && field("singingCallsFrom").toString() != "<Choose a directory>") {
        count = QString::number(countMP3FilesInDir(QDir(field("singingCallsFrom").toString())));
        confirmText += "   " + count + " MP3 files from " + field("singingCallsFrom").toString() + " to '" + field("singingCallsDirName").toString() + "'\n";
        someFilesCopied = true;
    }

    if (field("vocalsDoCopy").toBool() && field("vocalsFrom").toString() != "<Choose a directory>") {
        count = QString::number(countMP3FilesInDir(QDir(field("vocalsFrom").toString())));
        confirmText += "   " + count + " MP3 files from " + field("vocalsFrom").toString() + " to '" + field("vocalsDirName").toString() + "'\n";
        someFilesCopied = true;
    }

    if (field("extrasDoCopy").toBool() && field("extrasFrom").toString() != "<Choose a directory>") {
        count = QString::number(countMP3FilesInDir(QDir(field("extrasFrom").toString())));
        confirmText += "   " + count + " MP3 files from " + field("extrasFrom").toString() + " to '" + field("extrasDirName").toString() + "'\n";
        someFilesCopied = true;
    }

    if (!someFilesCopied) {
        confirmText += "   <none>";
    }

    label->setText(confirmText);
}
