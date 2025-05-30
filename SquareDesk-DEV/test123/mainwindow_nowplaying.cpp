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
#include <QFileInfo>
#include <QDebug>
#include <QTimer>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>

// External audio system reference
extern flexible_audio *cBass;

#ifdef Q_OS_MACOS
extern "C" {
    // Forward declarations for Objective-C functions
    void setupMediaRemoteCommands();
    void updateNowPlayingInfo(const char* title, const char* artist, const char* album, 
                             double duration, double currentTime, bool isPlaying);
    void updateNowPlayingInfoWithArtwork(const char* title, const char* artist, const char* album, 
                                        double duration, double currentTime, bool isPlaying, const char* artworkPath);
    void setNowPlayingCallbacks(void (*playCallback)(), void (*pauseCallback)(), 
                               void (*nextCallback)(), void (*prevCallback)(),
                               void (*seekCallback)(double));
}

// Static callback functions that will call the MainWindow methods
static MainWindow* g_mainWindow = nullptr;

static void playCallback() {
    // printf("playCallback() called in C++\n");
    if (g_mainWindow) {
        // printf("Calling g_mainWindow->nowPlayingPlay()\n");
        g_mainWindow->nowPlayingPlay();
        // printf("g_mainWindow->nowPlayingPlay() completed\n");
    } else {
        // printf("ERROR: g_mainWindow is null!\n");
    }
}

static void pauseCallback() {
    // printf("pauseCallback() called in C++\n");
    if (g_mainWindow) {
        // printf("Calling g_mainWindow->nowPlayingPause()\n");
        g_mainWindow->nowPlayingPause();
        // printf("g_mainWindow->nowPlayingPause() completed\n");
    } else {
        // printf("ERROR: g_mainWindow is null!\n");
    }
}

static void nextCallback() {
    if (g_mainWindow) {
        g_mainWindow->nowPlayingNext();
    }
}

static void prevCallback() {
    if (g_mainWindow) {
        g_mainWindow->nowPlayingPrevious();
    }
}

static void seekCallback(double time) {
    if (g_mainWindow) {
        g_mainWindow->nowPlayingSeek(time);
    }
}
#endif

void MainWindow::setupNowPlaying() {
#ifdef Q_OS_MACOS
    // printf("setupNowPlaying() called - starting Now Playing integration\n");
    
    // Store reference to this MainWindow instance for callbacks
    g_mainWindow = this;
    // printf("g_mainWindow set to: %p\n", g_mainWindow);
    
    // Setup media remote commands (Objective-C)
    setupMediaRemoteCommands();
    // printf("setupMediaRemoteCommands() completed\n");
    
    // Set callback functions
    setNowPlayingCallbacks(playCallback, pauseCallback, nextCallback, prevCallback, seekCallback);
    // printf("setNowPlayingCallbacks() completed\n");
    
    // Try to register immediately as a media app by setting some basic Now Playing info
    // printf("Setting initial Now Playing info to register as media app...\n");
    
    // Extract artwork for initial setup too
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString artworkPath = tempDir + "/squaredesk_icon.png";
    
    QFileInfo tempFile(artworkPath);
    if (!tempFile.exists()) {
        QFile resourceFile(":/images/icon1.png");
        if (resourceFile.exists() && resourceFile.copy(artworkPath)) {
            QFile::setPermissions(artworkPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
            // printf("Extracted icon resource for initial setup: %s\n", artworkPath.toUtf8().constData());
        } else {
            artworkPath.clear();
        }
    }
    
    if (!artworkPath.isEmpty()) {
        updateNowPlayingInfoWithArtwork("SquareDesk", "Ready to Play", "SquareDesk",
                                       0.0, 0.0, false, artworkPath.toUtf8().constData());
    } else {
        updateNowPlayingInfo("SquareDesk", "Ready to Play", "SquareDesk", 0.0, 0.0, false);
    }
    
    // printf("Now Playing integration initialized successfully\n");
    // printf("IMPORTANT: Make sure your iPhone and Mac are:\n");
    // printf("  1. On the same Wi-Fi network\n");
    // printf("  2. Both signed into the same Apple ID\n");
    // printf("  3. Have Handoff enabled in System Settings\n");
    // printf("  4. Have Bluetooth enabled on both devices\n");
    // printf("Check iPhone Control Center and Lock Screen for SquareDesk controls\n");
#endif
}

void MainWindow::updateNowPlayingMetadata() {
#ifdef Q_OS_MACOS
    // qDebug() << "updateNowPlayingMetadata() called";
    
    // Get current song information
    QString title = "Unknown Title";
    QString artist = "Unknown Artist";
    QString album = "SquareDesk";
    
    // Try to extract title and artist from the current filename or metadata
    if (!currentMP3filenameWithPath.isEmpty()) {
        QFileInfo fileInfo(currentMP3filenameWithPath);
        QString baseName = fileInfo.baseName();
        // qDebug() << "Song filename:" << baseName;
        
        // Try to split on common separators to get artist and title
        if (baseName.contains(" - ")) {
            QStringList parts = baseName.split(" - ", Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                artist = parts[0].trimmed();
                title = parts[1].trimmed();
            } else {
                title = baseName;
            }
        } else {
            title = baseName;
        }
    }
    
    // Get playback state - check it twice to make sure it's stable
    uint32_t Stream_State = cBass->currentStreamState();
    
    // Update current position and get duration
    cBass->StreamGetPosition();  // update cBass->Current_Position
    
    // Check stream state again after getting position, in case it changed
    uint32_t Stream_State_After = cBass->currentStreamState();
    if (Stream_State != Stream_State_After) {
        printf("Stream state changed during position update: %u -> %u\n", Stream_State, Stream_State_After);
        Stream_State = Stream_State_After;
    }
    
    bool isPlaying = (Stream_State == BASS_ACTIVE_PLAYING);
    
    // printf("Stream state check: %u, BASS_ACTIVE_PLAYING=%u, isPlaying=%s\n", 
    //        Stream_State, BASS_ACTIVE_PLAYING, isPlaying ? "true" : "false");
    
    // Both FileLength and Current_Position are already in seconds (not milliseconds)
    double duration = static_cast<double>(cBass->FileLength);
    double currentTime = static_cast<double>(cBass->Current_Position);
    
    // qDebug() << "Raw values:";
    // qDebug() << "  FileLength (seconds):" << cBass->FileLength;
    // qDebug() << "  Current_Position (seconds):" << cBass->Current_Position;
    // qDebug() << "Converted values:";
    // qDebug() << "  Duration:" << duration << "seconds";
    // qDebug() << "  Current Time:" << currentTime << "seconds";
    // qDebug() << "  Progress:" << (duration > 0 ? (currentTime / duration * 100.0) : 0) << "%";
    
    // qDebug() << "Now Playing Info:";
    // qDebug() << "  Title:" << title;
    // qDebug() << "  Artist:" << artist;
    // qDebug() << "  Album:" << album;
    // qDebug() << "  Is Playing:" << isPlaying;
    // qDebug() << "  Stream State:" << Stream_State;
    
    // Only update if we have valid song information  
    if (!currentMP3filenameWithPath.isEmpty() && duration > 0) {
        // Extract the icon from Qt resources to a temporary file
        QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        QString artworkPath = tempDir + "/squaredesk_icon.png";
        
        // Copy resource to temp file if it doesn't exist or is outdated
        QFileInfo tempFile(artworkPath);
        if (!tempFile.exists()) {
            QFile resourceFile(":/images/icon1.png");
            if (resourceFile.exists()) {
                // Copy resource to temp location
                if (resourceFile.copy(artworkPath)) {
                    // Make the temp file writable (Qt resources are read-only by default)
                    QFile::setPermissions(artworkPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
                    // printf("Extracted icon resource to: %s\n", artworkPath.toUtf8().constData());
                } else {
                    // printf("Failed to extract icon resource\n");
                    artworkPath.clear();
                }
            } else {
                // printf("Icon resource not found: :/images/icon1.png\n");
                artworkPath.clear();
            }
        } else {
            // printf("Using existing temp icon file: %s\n", artworkPath.toUtf8().constData());
        }
        
        if (!artworkPath.isEmpty()) {
            // Update the Now Playing info with artwork
            updateNowPlayingInfoWithArtwork(title.toUtf8().constData(), 
                                          artist.toUtf8().constData(), 
                                          album.toUtf8().constData(),
                                          duration, 
                                          currentTime, 
                                          isPlaying,
                                          artworkPath.toUtf8().constData());
        } else {
            // Update the Now Playing info without artwork
            updateNowPlayingInfo(title.toUtf8().constData(), 
                                artist.toUtf8().constData(), 
                                album.toUtf8().constData(),
                                duration, 
                                currentTime, 
                                isPlaying);
        }
        
        // printf("updateNowPlayingInfo() called with above parameters\n");
        
    } else {
        // printf("Skipping Now Playing update - no song loaded or invalid duration\n");
        // printf("  currentMP3filenameWithPath isEmpty: %s\n", currentMP3filenameWithPath.isEmpty() ? "true" : "false");
        // printf("  duration: %.2f\n", duration);
    }
#endif
}

// Callback methods that handle remote commands
void MainWindow::nowPlayingPlay() {
    printf("nowPlayingPlay() called from remote control\n");
    
    // Check if we have a song loaded
    if (currentMP3filenameWithPath.isEmpty()) {
        printf("Remote play: No song loaded, ignoring play command\n");
        return;
    }
    
    // Check current stream state to determine if we should play
    uint32_t Stream_State = cBass->currentStreamState();
    
    if (Stream_State != BASS_ACTIVE_PLAYING) {
        // Currently not playing, so start playing
        printf("Remote play: Starting playback (stream state was %u)\n", Stream_State);
        ui->darkPlayButton->click();
    } else {
        printf("Remote play: Already playing (stream state: %u)\n", Stream_State);
    }
}

void MainWindow::nowPlayingPause() {
    // qDebug() << "nowPlayingPause() called from remote control";
    
    // Check current stream state 
    uint32_t Stream_State = cBass->currentStreamState();
    
    if (Stream_State == BASS_ACTIVE_PLAYING) {
        // Currently playing, so pause
        // qDebug() << "Remote pause: Pausing playback (stream state was" << Stream_State << ")";
        ui->darkPlayButton->click();
    } else {
        // qDebug() << "Remote pause: Already paused/stopped (stream state:" << Stream_State << ")";
    }
}

void MainWindow::nowPlayingNext() {
    // Simulate next track action - use the skip forward functionality
    if (ui->actionSkip_Forward) {
        ui->actionSkip_Forward->trigger();
    }
}

void MainWindow::nowPlayingPrevious() {
    // Simulate previous track action - use the skip back functionality  
    if (ui->actionSkip_Backward) {
        ui->actionSkip_Backward->trigger();
    }
}

void MainWindow::nowPlayingSeek(double timeInSeconds) {
    double positionSeconds;
    
    // Check if this is a relative seek (FF/REWIND) or absolute seek (position slider)
    if (timeInSeconds < -900.0) {
        // This is a relative seek - decode the special encoding
        double skipInterval = timeInSeconds + 999.0; // Remove the -999 offset
        double currentPos = static_cast<double>(cBass->Current_Position);
        positionSeconds = currentPos + skipInterval;
        // printf("Now Playing relative seek: current=%.2f, skip=%.2f, target=%.2f\n", 
        //        currentPos, skipInterval, positionSeconds);
    } else {
        // This is an absolute seek from the slider
        positionSeconds = timeInSeconds;
        // printf("Now Playing absolute seek to: %.2f seconds\n", positionSeconds);
    }
    
    // Make sure the position is within bounds  
    if (positionSeconds >= 0 && positionSeconds <= static_cast<double>(cBass->FileLength)) {
        // Use the same method as the existing seek functionality
        cBass->StreamSetPosition(positionSeconds);
        Info_Seekbar(false);
        
        // Give BASS a moment to update its position, then update Now Playing info
        // QTimer::singleShot(100, [this, positionSeconds]() {
        QTimer::singleShot(100, [this]() {
            // printf("Updating Now Playing info after seek to %.2f seconds\n", positionSeconds);
            
            // Force the position update first
            cBass->StreamGetPosition();
            // printf("After StreamGetPosition: Current_Position = %.2f\n", static_cast<double>(cBass->Current_Position));
            
            updateNowPlayingMetadata();
        });
    } else {
        // printf("Now Playing seek out of bounds: %.2f seconds (max: %.2f)\n", positionSeconds, static_cast<double>(cBass->FileLength));
    }
}
