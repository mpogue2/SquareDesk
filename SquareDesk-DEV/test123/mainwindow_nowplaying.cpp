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
    // printf("updateNowPlayingMetadata() called\n");
    
    // Get current song information
    QString title = "Unknown Title";
    QString artist = "Unknown Artist";
    QString album = "SquareDesk";
    
    // Try to extract title and artist from the current filename or metadata
    if (!currentMP3filenameWithPath.isEmpty()) {
        QFileInfo fileInfo(currentMP3filenameWithPath);
        QString baseName = fileInfo.baseName();
        // printf("Song filename: %s\n", baseName.toUtf8().constData());
        
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
        // printf("Stream state changed during position update: %u -> %u\n", Stream_State, Stream_State_After);
        Stream_State = Stream_State_After;
    }
    
    bool isPlaying = (Stream_State == BASS_ACTIVE_PLAYING);
    
    // printf("Stream state check: %u, BASS_ACTIVE_PLAYING=%u, isPlaying=%s\n",
    //        Stream_State, BASS_ACTIVE_PLAYING, isPlaying ? "true" : "false");
    
    // Both FileLength and Current_Position are already in seconds (not milliseconds)
    double duration = static_cast<double>(cBass->FileLength);
    double currentTime = static_cast<double>(cBass->Current_Position);
    
    // printf("Raw values:\n");
    // printf("  FileLength (seconds): %.2f\n", cBass->FileLength);
    // printf("  Current_Position (seconds): %.2f\n", cBass->Current_Position);
    // printf("Converted values:\n");
    // printf("  Duration: %.2f seconds\n", duration);
    // printf("  Current Time: %.2f seconds\n", currentTime);
    // printf("  Progress: %.2f%%\n", (duration > 0 ? (currentTime / duration * 100.0) : 0));
    
    // printf("Now Playing Info:\n");
    // printf("  Title: %s\n", title.toUtf8().constData());
    // printf("  Artist: %s\n", artist.toUtf8().constData());
    // printf("  Album: %s\n", album.toUtf8().constData());
    // printf("  Is Playing: %s\n", isPlaying ? "true" : "false");
    // printf("  Stream State: %u\n", Stream_State);
    
    // Check the condition that might be preventing the update
    // printf("Condition check: currentMP3filenameWithPath.isEmpty()=%s, duration=%.2f\n",
    //        currentMP3filenameWithPath.isEmpty() ? "true" : "false", duration);
    
    // Only update if we have valid song information  
    if (!currentMP3filenameWithPath.isEmpty() && duration > 0) {
        // printf("CONDITION MET: Proceeding with Now Playing update\n");
        
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
            // printf("C++: About to call updateNowPlayingInfoWithArtwork\n");
            updateNowPlayingInfoWithArtwork(title.toUtf8().constData(), 
                                          artist.toUtf8().constData(), 
                                          album.toUtf8().constData(),
                                          duration, 
                                          currentTime, 
                                          isPlaying,
                                          artworkPath.toUtf8().constData());
            // printf("C++: updateNowPlayingInfoWithArtwork completed\n");
        } else {
            // Update the Now Playing info without artwork
            // printf("C++: About to call updateNowPlayingInfo\n");
            updateNowPlayingInfo(title.toUtf8().constData(), 
                                artist.toUtf8().constData(), 
                                album.toUtf8().constData(),
                                duration, 
                                currentTime, 
                                isPlaying);
            // printf("C++: updateNowPlayingInfo completed\n");
        }
        
        // printf("updateNowPlayingInfo() called with above parameters\n");
        
        // Only do the media activation for the first registration of a new song
        static QString lastRegisteredSong;
        if (lastRegisteredSong != currentMP3filenameWithPath && duration > 0) {
            lastRegisteredSong = currentMP3filenameWithPath;
            // printf("C++: First Now Playing registration for this song - attempting brief activation\n");


            // NOTE: This code makes F8 right after load work, but it breaks a lot of other stuff,
            //  like the automatic timers, the auto-switch-to-cuesheet, etc.
            // So, before F8 will work, you need to press play/pause twice manually.
            //
            // QTimer::singleShot(500, [this]() {
            //     if (!currentMP3filenameWithPath.isEmpty() && cBass->FileLength > 0) {
            //         printf("C++: Starting brief media activation cycle using UI button...\n");
                    
            //         // Use the same UI button that Control Center would trigger
            //         ui->darkPlayButton->click();
                    
            //         QTimer::singleShot(5, [this]() {
            //             // Click again to pause after just 5ms
            //             ui->darkPlayButton->click();
            //             printf("C++: Media activation cycle complete using UI - F8 should now work!\n");
            //             updateNowPlayingMetadata();
            //         });
            //     }
            // });
        }
        
    } else {
        // printf("CONDITION NOT MET: Skipping Now Playing update\n");
        // printf("  currentMP3filenameWithPath isEmpty: %s\n", currentMP3filenameWithPath.isEmpty() ? "true" : "false");
        // printf("  duration: %.2f\n", duration);
    }
#endif
}

// Callback methods that handle remote commands
void MainWindow::nowPlayingPlay() {
    // printf("nowPlayingPlay() called from remote control\n");
    
    // Check if we have a song loaded
    if (currentMP3filenameWithPath.isEmpty()) {
        // printf("Remote play: No song loaded, ignoring play command\n");
        return;
    }
    
    // Check current stream state to determine what action to take
    uint32_t Stream_State = cBass->currentStreamState();
    // printf("Remote play: Current stream state is %u (BASS_ACTIVE_PLAYING=%u)\n", Stream_State, BASS_ACTIVE_PLAYING);
    // printf("Remote play: Song loaded: %s\n", currentMP3filenameWithPath.toUtf8().constData());
    
    // For F8 toggle behavior, we need to handle all possible states:
    // State 0 = Never been played (should start)
    // State 1 = Currently playing (should pause) 
    // State 3 = Paused (should resume)
    // Any other state = Try to start/resume
    
    if (Stream_State == BASS_ACTIVE_PLAYING) {
        // Currently playing, so pause it
        // printf("Remote play: Currently playing, pausing\n");
        ui->darkPlayButton->click();
    } else {
        // Not playing (could be never played, paused, or stopped), so start/resume
        // printf("Remote play: Not playing (state %u), starting/resuming playback\n", Stream_State);
        ui->darkPlayButton->click();
    }
    
    // Check the state after clicking to see what happened
    QTimer::singleShot(50, []() {
        // Note: can't access 'this' in this context, but that's okay for this debug check
        // printf("Remote play: Button click completed\n");
    });
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
    // Use the same 15-second skip as Control Center buttons
    double currentPos = static_cast<double>(cBass->Current_Position);
    double newPos = currentPos + 15.0; // 15 second skip forward
    
    if (newPos >= static_cast<double>(cBass->FileLength)) {
        newPos = static_cast<double>(cBass->FileLength) - 0.1;
    }

    // Make sure we don't skip past the end
    // if (newPos <= static_cast<double>(cBass->FileLength)) {
    cBass->StreamSetPosition(newPos);
    Info_Seekbar(false);

    // Update Now Playing position immediately after skip
    QTimer::singleShot(50, [this]() {
        cBass->StreamGetPosition();
        updateNowPlayingMetadata();
        // printf("Now Playing updated after F9 skip forward (15 seconds)\n");
    });
    // }
}

void MainWindow::nowPlayingPrevious() {
    // Use the same 15-second skip as Control Center buttons
    double currentPos = static_cast<double>(cBass->Current_Position);
    double newPos = currentPos - 15.0; // 15 second skip backward
    
    // Make sure we don't skip before the beginning
    if (newPos < 0.0) {
        newPos = 0.0;
    }

    // if (newPos >= 0.0) {
    cBass->StreamSetPosition(newPos);
    Info_Seekbar(false);

    // Update Now Playing position immediately after skip
    QTimer::singleShot(50, [this]() {
        cBass->StreamGetPosition();
        updateNowPlayingMetadata();
        // printf("Now Playing updated after F7 skip backward (15 seconds)\n");
    });
    // }
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
    
    if (positionSeconds < 0.0) {
        positionSeconds = 0.0;
    } else if (positionSeconds >= static_cast<double>(cBass->FileLength)) {
        positionSeconds = static_cast<double>(cBass->FileLength - 0.1);
    }

    // Make sure the position is within bounds  
    if (positionSeconds >= 0 && positionSeconds <= static_cast<double>(cBass->FileLength)) {
        // Use the same method as the existing seek functionality
        cBass->StreamSetPosition(positionSeconds);
        Info_Seekbar(false);
        
        // Give BASS a moment to update its position, then update Now Playing info
        QTimer::singleShot(50, [this]() {
            // Force the position update first
            cBass->StreamGetPosition();
            updateNowPlayingMetadata();
            // printf("Now Playing updated after seek operation\n");
        });
    } else {
        // printf("Now Playing seek out of bounds: %.2f seconds (max: %.2f)\n", positionSeconds, static_cast<double>(cBass->FileLength));
    }
}
