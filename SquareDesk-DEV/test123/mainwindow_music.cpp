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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMediaDevices>
#include <QAudioDevice>
#include <QMenu>
#include <QAction>
#include <QDebug>
#include <utility>

// ------------------------------------------------------------------
#define SYSTEMSOUNDDEVICENAME "System Sound Device"

void MainWindow::populatePlaybackDeviceMenu()
{
    // Find the "Preview Playback Device" menu item
    QAction *previewPlaybackAction = nullptr;
    for (const auto &action : ui->menuMusic->actions()) {
        if (action->text() == "Preview Playback Device") {
            previewPlaybackAction = action;
            break;
        }
    }
    
    if (!previewPlaybackAction) {
        qDebug() << "Could not find 'Preview Playback Device' menu item";
        return;
    }
    
    // Create a submenu for the playback devices
    QMenu *deviceMenu = new QMenu("Preview Playback Device", this);
    previewPlaybackAction->setMenu(deviceMenu);

    // Mutually exclusive devices
    QActionGroup *deviceActionGroup = new QActionGroup(this);

    // SPECIAL HANDLING FOR "SYSTEM SOUND DEVICE" ----------
    QAction *SystemSoundDeviceAction = deviceMenu->addAction(SYSTEMSOUNDDEVICENAME);
    SystemSoundDeviceAction->setCheckable(true);

    // Initialize auditionPlaybackDeviceName - use saved preference or default device
    QAudioDevice defaultDevice = QMediaDevices::defaultAudioOutput();

    QString previewDeviceName = prefsManager.GetcurrentPreviewPlaybackDeviceName();
    if (previewDeviceName.isEmpty() || (previewDeviceName == SYSTEMSOUNDDEVICENAME)) {
        auditionPlaybackDeviceName = SYSTEMSOUNDDEVICENAME;
        SystemSoundDeviceAction->setChecked(true);
        setPreviewPlaybackDevice(SYSTEMSOUNDDEVICENAME); // and remember this choice for "next time"
        previewDeviceName = SYSTEMSOUNDDEVICENAME;
    } else {
        auditionPlaybackDeviceName = previewDeviceName;
    }

    // Connect the action to our slot
    connect(SystemSoundDeviceAction, &QAction::triggered, [this]() {
        setPreviewPlaybackDevice(SYSTEMSOUNDDEVICENAME);
    });

    deviceActionGroup->addAction(SystemSoundDeviceAction); // add to the mutual-exclusion group

    deviceMenu->addSeparator();

    // Get list of available audio output devices ----------
    QList<QAudioDevice> audioOutputList = QMediaDevices::audioOutputs();

    // Add each device as a menu item
    for (const auto &device : std::as_const(audioOutputList)) {
        QString deviceName = device.description();
        QAction *deviceAction = deviceMenu->addAction(deviceName);
        deviceAction->setCheckable(true);
        
        // Mark the default device as checked (none of THESE will be checked, if System Sound Device was checked
        if (previewDeviceName == deviceName) {
            // if we HAVE done this before (it's "next time"), use what we chose last time
            deviceAction->setChecked(true);
        }
        
        // Connect the action to our slot
        connect(deviceAction, &QAction::triggered, [this, deviceName]() {
            setPreviewPlaybackDevice(deviceName);
        });
    }
    
    // Make device selections mutually exclusive
    for (const auto &action : deviceMenu->actions()) {
        deviceActionGroup->addAction(action);
    }
    deviceActionGroup->setExclusive(true);
}

// ------------------------------------------------------------------
void MainWindow::setPreviewPlaybackDevice(const QString &playbackDeviceName)
{
    // Store the selected device name for use in audition playback
    auditionPlaybackDeviceName = playbackDeviceName;
    
    // Also remember our choice in preferences for next time
    prefsManager.SetcurrentPreviewPlaybackDeviceName(playbackDeviceName);
    // qDebug() << "Preview playback device selected:" << playbackDeviceName;
}


// ------------------------------------------------------------------
QAudioDevice MainWindow::getAudioDeviceByName(const QString &deviceName)
{
    if (deviceName == SYSTEMSOUNDDEVICENAME) {
        return QMediaDevices::defaultAudioOutput();
    }

    QList<QAudioDevice> audioOutputList = QMediaDevices::audioOutputs();
    
    // First, try to find the device by exact name match
    for (const auto &device : std::as_const(audioOutputList)) {
        if (device.description() == deviceName) {
            return device;
        }
    }
    
    // If not found, return the default device, and log an error...
    // NOTE: I don't think we need this in the log anymore...
    // qDebug() << "Audio device not found:" << deviceName << ", using default device";
    return QMediaDevices::defaultAudioOutput();
}
