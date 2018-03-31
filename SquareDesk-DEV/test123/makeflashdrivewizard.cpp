/****************************************************************************
**
** Copyright (C) 2016, 2017, 2018 Mike Pogue, Dan Lyke
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
#include <QStringList>

#include "prefsmanager.h"
#include "makeflashdrivewizard.h"

// ===========================================================================
MakeFlashDriveWizard::MakeFlashDriveWizard(QWidget *parent)
    : QWizard(parent)
{
    addPage(new IntroMakeFlashDrivePage);
    addPage(new VolumeMakeFlashDrivePage);
    addPage(new ConclusionMakeFlashDrivePage);


#if !defined(Q_OS_MAC)
//    setPixmap(QWizard::BannerPixmap, QPixmap(":/startupwizardimages/banner.png"));
//    setPixmap(QWizard::BackgroundPixmap, QPixmap(":/startupwizardimages/background.png"));
    setPixmap(QWizard::WatermarkPixmap, QPixmap(":/startupwizardimages/watermark1.png"));
#endif

    setWindowTitle(tr("Make Flash Drive Wizard"));
}

// --------------------------------------
void MakeFlashDriveWizard::accept()
{
    QDialog::accept();
}

// --------------------------------------
IntroMakeFlashDrivePage::IntroMakeFlashDrivePage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Introduction"));

#if !defined(Q_OS_MAC)
    setPixmap(QWizard::WatermarkPixmap, QPixmap(":/startupwizardimages/watermark1.png"));
#endif

    label = new QLabel(tr("This wizard will help you make a copy of your Music Directory to "
                          "an external flash drive.  It will also copy the SquareDesk "
                          "application itself to that same external flash drive. "
                          "\n\nThen, just plug the flash drive into a laptop to be able to use SquareDesk and all your music on that laptop. "
                          "\n\nTo get started, insert a flash drive, and click Continue."
    ));
    label->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    setLayout(layout);
}

// ===========================================================================
VolumeMakeFlashDrivePage::VolumeMakeFlashDrivePage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Choose a Flash Drive"));

#if !defined(Q_OS_MAC)
    setPixmap(QWizard::WatermarkPixmap, QPixmap(":/startupwizardimages/watermark1.png"));
#endif

    label = new QLabel(tr("Volume select page"));
    label->setWordWrap(true);

    combobox = new QComboBox();

    registerField("destinationVolume", combobox, "currentText");

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(combobox);
    setLayout(layout);
}

void VolumeMakeFlashDrivePage::initializePage()
{

    QDir dir("/Volumes/");
    dir.setFilter(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);

    QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
//        qDebug() << qPrintable(QString("%1 %2").arg(fileInfo.size(), 10)
//                                                .arg(fileInfo.fileName()));
        combobox->addItem(fileInfo.fileName());
    }

    QString confirmText = "Choose the flash drive you want to copy to, and click Continue:\n\n";
    label->setText(confirmText);
}


// ===========================================================================
ConclusionMakeFlashDrivePage::ConclusionMakeFlashDrivePage(QWidget *parent)
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
void ConclusionMakeFlashDrivePage::initializePage()
{
    QString confirmText;
    QString volName = field("destinationVolume").toString();
    confirmText = QString("Flash drive to copy to: '%1'\n     ").arg(volName);
    confirmText += "\nItems to copy:\n";
    confirmText += QString("     <Music Directory> --> /Volumes/%1/<Music Directory>\n").arg(volName);
    confirmText += QString("     SquareDesk Application  --> /Volumes/%1/SquareDesk.app\n").arg(volName);

    confirmText += "\nNOTE: only those files which do not already exist on the destination drive will be copied.\n\n";

#if defined(Q_OS_MAC) | defined(Q_OS_LINUX)
    confirmText += "Click Done to copy your entire Music Directory and the SquareDesk application itself.";
#elif defined(Q_OS_WIN)
    confirmText += "Click Finish to copy your entire Music Directory and the SquareDesk application itself.";
#endif



    label->setText(confirmText);
}
