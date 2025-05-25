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

// ------------------------------------------------------------------
void MainWindow::populatePlaybackDeviceMenu()
{
    // Find the "Preview Playback Device" menu item
    QAction *previewPlaybackAction = nullptr;
    foreach (QAction *action, ui->menuMusic->actions()) {
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
    
    // Get list of available audio output devices
    QList<QAudioDevice> audioOutputList = QMediaDevices::audioOutputs();
    QAudioDevice defaultDevice = QMediaDevices::defaultAudioOutput();
    
    QString previewDeviceName = prefsManager.GetcurrentPreviewPlaybackDeviceName();

    // Initialize auditionPlaybackDeviceName - use saved preference or default device
    if (previewDeviceName.isEmpty()) {
        auditionPlaybackDeviceName = defaultDevice.description();
    } else {
        auditionPlaybackDeviceName = previewDeviceName;
    }

    // Add each device as a menu item
    foreach (const QAudioDevice &device, audioOutputList) {
        QString deviceName = device.description();
        QAction *deviceAction = deviceMenu->addAction(deviceName);
        deviceAction->setCheckable(true);
        
        // Mark the default device as checked
        if (previewDeviceName == "") {
            // if we've never done this before, just [X] default system device
            if (device.id() == defaultDevice.id()) {
                deviceAction->setChecked(true);
                setPreviewPlaybackDevice(deviceName); // and remember this choice for "next time"
            }
        } else {
            // if we HAVE done this before (it's "next time"), use what we chose last time
            if (deviceName == previewDeviceName) {
                deviceAction->setChecked(true);
            }
        }
        
        // Connect the action to our slot
        connect(deviceAction, &QAction::triggered, [this, deviceName]() {
            setPreviewPlaybackDevice(deviceName);
        });
    }
    
    // Make device selections mutually exclusive
    QActionGroup *deviceActionGroup = new QActionGroup(this);
    foreach (QAction *action, deviceMenu->actions()) {
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
    QList<QAudioDevice> audioOutputList = QMediaDevices::audioOutputs();
    
    // First, try to find the device by exact name match
    foreach (const QAudioDevice &device, audioOutputList) {
        if (device.description() == deviceName) {
            return device;
        }
    }
    
    // If not found, return the default device
    qDebug() << "Audio device not found:" << deviceName << ", using default device";
    return QMediaDevices::defaultAudioOutput();
}
