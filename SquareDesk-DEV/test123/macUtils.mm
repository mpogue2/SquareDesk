/****************************************************************************
**
** Copyright (C) 2016-2020 Mike Pogue, Dan Lyke
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

#include "macUtils.h"

//#import <AppKit/AppKit.h>

// https://forum.qt.io/topic/38504/solved-qdialog-in-fullscreen-disable-os-screensaver/27
MacUtils::MacUtils() {
}

MacUtils::~MacUtils() {
}

//void MacUtils::disableScreensaver() {
//    // I could not get the standard kIOPMAssertionTypePreventUserIdleDisplaySleep method
//    //   to return success, and screensaver was not getting disabled, so switching to an alternate method
//    UpdateSystemActivity(UsrActivity);  // alternate method, call it every N seconds
//}

//void MacUtils::reenableScreensaver() {
//    // IOPMAssertionRelease(assertionID);
//}

// Disables auto window tabbing where supported, otherwise a no-op.
//    from: http://lists.qt-project.org/pipermail/interest/2016-September/024488.html
void MacUtils::disableWindowTabbing() {
//    if ([NSWindow respondsToSelector:@selector(allowsAutomaticWindowTabbing)]) {
//        NSWindow.allowsAutomaticWindowTabbing = NO;
//    }
}

// This code doesn't work right now, but I'm leaving it here for somebody smarter than me to figure out why...
// #define THISCODEWORKS
#ifdef THISCODEWORKS
// ---------------------------
// sample code from: https://github.com/owncloud/client/commit/4041ff7ebe17b25a242b1d1ff410743f97133bb1

#include "QtCore/qtpreprocessorsupport.h" // for Q_UNUSED

#import <AppKit/NSApplication.h>

@interface OwnAppDelegate : NSObject <NSApplicationDelegate>
- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app;
@end

@implementation OwnAppDelegate {
}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app
{
    Q_UNUSED(app)

    printf("******** YES YES YES ***********");  // proof of execution

    // We do not use `NSCoder` classes (nor does Qt), nor override the `initWithCoder` method, so we are fine with SecureCoding.
    return YES;
}

@end
#endif

// Now Playing integration for iOS/watchOS remote control
#import <MediaPlayer/MediaPlayer.h>
#import <AppKit/AppKit.h>

// C callback function pointers
static void (*g_playCallback)() = nullptr;
static void (*g_pauseCallback)() = nullptr;  
static void (*g_nextCallback)() = nullptr;
static void (*g_prevCallback)() = nullptr;
static void (*g_seekCallback)(double) = nullptr;

// Target class for handling remote commands - must be defined before use
@interface MPNowPlayingTarget : NSObject
@end

@implementation MPNowPlayingTarget

- (MPRemoteCommandHandlerStatus)handlePlayCommand:(MPRemoteCommandEvent *)event {
    // printf("=== PLAY COMMAND RECEIVED ===\n");
    // printf("g_playCallback pointer: %p\n", g_playCallback);
    if (g_playCallback) {
        // printf("Calling g_playCallback...\n");
        g_playCallback();
        // printf("g_playCallback completed\n");
        return MPRemoteCommandHandlerStatusSuccess;
    }
    // printf("No play callback set\n");
    return MPRemoteCommandHandlerStatusCommandFailed;
}

- (MPRemoteCommandHandlerStatus)handlePauseCommand:(MPRemoteCommandEvent *)event {
    // printf("=== PAUSE COMMAND RECEIVED ===\n");
    if (g_pauseCallback) {
        g_pauseCallback();
        return MPRemoteCommandHandlerStatusSuccess;
    }
    // printf("No pause callback set\n");
    return MPRemoteCommandHandlerStatusCommandFailed;
}

- (MPRemoteCommandHandlerStatus)handleTogglePlayPauseCommand:(MPRemoteCommandEvent *)event {
    // printf("=== F8 PRESSED - handleTogglePlayPauseCommand called ===\n");
    // printf("g_playCallback pointer: %p\n", g_playCallback);
    
    // For toggle, we need to determine current state and call appropriate callback
    // We'll use the play callback which should handle the toggle logic properly
    if (g_playCallback) {
        // printf("Calling g_playCallback for toggle...\n");
        g_playCallback(); 
        // printf("g_playCallback completed\n");
        return MPRemoteCommandHandlerStatusSuccess;
    }
    // printf("ERROR: No play callback set for toggle\n");
    return MPRemoteCommandHandlerStatusCommandFailed;
}

- (MPRemoteCommandHandlerStatus)handleNextTrackCommand:(MPRemoteCommandEvent *)event {
    if (g_nextCallback) {
        g_nextCallback();
        return MPRemoteCommandHandlerStatusSuccess;
    }
    return MPRemoteCommandHandlerStatusCommandFailed;
}

- (MPRemoteCommandHandlerStatus)handlePreviousTrackCommand:(MPRemoteCommandEvent *)event {
    if (g_prevCallback) {
        g_prevCallback();
        return MPRemoteCommandHandlerStatusSuccess;
    }
    return MPRemoteCommandHandlerStatusCommandFailed;
}

- (MPRemoteCommandHandlerStatus)handleChangePlaybackPositionCommand:(MPChangePlaybackPositionCommandEvent *)event {
    if (g_seekCallback) {
        g_seekCallback(event.positionTime);
        return MPRemoteCommandHandlerStatusSuccess;
    }
    return MPRemoteCommandHandlerStatusCommandFailed;
}

- (MPRemoteCommandHandlerStatus)handleSkipForwardCommand:(MPSkipIntervalCommandEvent *)event {
    // printf("handleSkipForwardCommand called with interval: %.1f\n", event.interval);
    if (g_seekCallback) {
        // Get current position and add the skip interval
        // We'll let the C++ side handle the actual position calculation
        g_seekCallback(-999.0 + event.interval); // Special encoding: negative means relative skip
        return MPRemoteCommandHandlerStatusSuccess;
    }
    return MPRemoteCommandHandlerStatusCommandFailed;
}

- (MPRemoteCommandHandlerStatus)handleSkipBackwardCommand:(MPSkipIntervalCommandEvent *)event {
    // printf("handleSkipBackwardCommand called with interval: %.1f\n", event.interval);
    if (g_seekCallback) {
        // Get current position and subtract the skip interval
        // We'll let the C++ side handle the actual position calculation
        g_seekCallback(-999.0 - event.interval); // Special encoding: negative means relative skip
        return MPRemoteCommandHandlerStatusSuccess;
    }
    return MPRemoteCommandHandlerStatusCommandFailed;
}

@end

extern "C" {
    void setupMediaRemoteCommands() {
        // printf("setupMediaRemoteCommands() called\n");
        
        MPRemoteCommandCenter *commandCenter = [MPRemoteCommandCenter sharedCommandCenter];
        // printf("Got command center: %p\n", commandCenter);
        
        // First, completely disable all commands to reset state
        // printf("Resetting all remote commands...\n");
        commandCenter.playCommand.enabled = NO;
        commandCenter.pauseCommand.enabled = NO;
        commandCenter.togglePlayPauseCommand.enabled = NO;
        commandCenter.nextTrackCommand.enabled = NO;
        commandCenter.previousTrackCommand.enabled = NO;
        commandCenter.changePlaybackPositionCommand.enabled = NO;
        commandCenter.stopCommand.enabled = NO;
        commandCenter.skipBackwardCommand.enabled = NO;
        commandCenter.skipForwardCommand.enabled = NO;
        
        // Enable play command
        commandCenter.playCommand.enabled = YES;
        [commandCenter.playCommand addTarget:[[MPNowPlayingTarget alloc] init] action:@selector(handlePlayCommand:)];
        // printf("Play command enabled\n");
        
        // Enable pause command  
        commandCenter.pauseCommand.enabled = YES;
        [commandCenter.pauseCommand addTarget:[[MPNowPlayingTarget alloc] init] action:@selector(handlePauseCommand:)];
        // printf("Pause command enabled\n");
        
        // Enable toggle play/pause command
        commandCenter.togglePlayPauseCommand.enabled = YES;
        [commandCenter.togglePlayPauseCommand addTarget:[[MPNowPlayingTarget alloc] init] action:@selector(handleTogglePlayPauseCommand:)];
        // printf("Toggle play/pause command enabled\n");
        
        // Enable next track command
        commandCenter.nextTrackCommand.enabled = YES;
        [commandCenter.nextTrackCommand addTarget:[[MPNowPlayingTarget alloc] init] action:@selector(handleNextTrackCommand:)];
        // printf("Next track command enabled\n");
        
        // Enable previous track command
        commandCenter.previousTrackCommand.enabled = YES;
        [commandCenter.previousTrackCommand addTarget:[[MPNowPlayingTarget alloc] init] action:@selector(handlePreviousTrackCommand:)];
        // printf("Previous track command enabled\n");
        
        // Enable change playback position command (seeking)
        commandCenter.changePlaybackPositionCommand.enabled = YES;
        [commandCenter.changePlaybackPositionCommand addTarget:[[MPNowPlayingTarget alloc] init] action:@selector(handleChangePlaybackPositionCommand:)];
        // printf("Change playback position command enabled\n");
        
        // Enable skip forward command (FF button)
        commandCenter.skipForwardCommand.enabled = YES;
        commandCenter.skipForwardCommand.preferredIntervals = @[@15]; // 15 second skips
        [commandCenter.skipForwardCommand addTarget:[[MPNowPlayingTarget alloc] init] action:@selector(handleSkipForwardCommand:)];
        // printf("Skip forward command enabled\n");
        
        // Enable skip backward command (REWIND button)  
        commandCenter.skipBackwardCommand.enabled = YES;
        commandCenter.skipBackwardCommand.preferredIntervals = @[@15]; // 15 second skips
        [commandCenter.skipBackwardCommand addTarget:[[MPNowPlayingTarget alloc] init] action:@selector(handleSkipBackwardCommand:)];
        // printf("Skip backward command enabled\n");
        
        // printf("setupMediaRemoteCommands() completed successfully\n");
    }
    
    void updateNowPlayingInfo(const char* title, const char* artist, const char* album, 
                             double duration, double currentTime, bool isPlaying) {
        printf("updateNowPlayingInfo() called:\n");
        printf("  Title: %s\n", title ? title : "(null)");
        printf("  Artist: %s\n", artist ? artist : "(null)");
        printf("  Album: %s\n", album ? album : "(null)");
        printf("  Duration: %.2f seconds\n", duration);
        printf("  Current Time: %.2f seconds\n", currentTime);
        printf("  Is Playing: %s\n", isPlaying ? "YES" : "NO");
        
        // Check if MPNowPlayingInfoCenter is available
        MPNowPlayingInfoCenter *center = [MPNowPlayingInfoCenter defaultCenter];
        if (!center) {
            printf("ERROR: MPNowPlayingInfoCenter not available!\n");
            return;
        }
        printf("MPNowPlayingInfoCenter available: %p\n", center);
        
        NSMutableDictionary *nowPlayingInfo = [[NSMutableDictionary alloc] init];
        
        if (title) {
            nowPlayingInfo[MPMediaItemPropertyTitle] = [NSString stringWithUTF8String:title];
        }
        if (artist) {
            nowPlayingInfo[MPMediaItemPropertyArtist] = [NSString stringWithUTF8String:artist];
        }
        if (album) {
            nowPlayingInfo[MPMediaItemPropertyAlbumTitle] = [NSString stringWithUTF8String:album];
        }
        
        nowPlayingInfo[MPMediaItemPropertyPlaybackDuration] = @(duration);
        nowPlayingInfo[MPNowPlayingInfoPropertyElapsedPlaybackTime] = @(currentTime);
        nowPlayingInfo[MPNowPlayingInfoPropertyPlaybackRate] = isPlaying ? @(1.0) : @(0.0);
        
        // printf("Setting playback rate to: %.1f (isPlaying: %s)\n", isPlaying ? 1.0 : 0.0, isPlaying ? "YES" : "NO");
        
        // Add media type to indicate this is audio
        nowPlayingInfo[MPMediaItemPropertyMediaType] = @(MPMediaTypeMusic);
        
        // Add additional metadata to help system recognize this as a media app
        nowPlayingInfo[MPMediaItemPropertyAlbumArtist] = [NSString stringWithUTF8String:artist];
        nowPlayingInfo[MPNowPlayingInfoPropertyDefaultPlaybackRate] = @(1.0);
        nowPlayingInfo[MPNowPlayingInfoPropertyPlaybackQueueIndex] = @(0);
        nowPlayingInfo[MPNowPlayingInfoPropertyPlaybackQueueCount] = @(1);
        
        // Add SquareDesk artwork
        NSString *artworkPath = @"/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/test123/images/icon1.png";
        NSImage *artworkImage = [[NSImage alloc] initWithContentsOfFile:artworkPath];
        if (artworkImage) {
            // printf("Loaded artwork image: %s (size: %.0fx%.0f)\n",
            //        [artworkPath UTF8String], artworkImage.size.width, artworkImage.size.height);
            
            MPMediaItemArtwork *artwork = [[MPMediaItemArtwork alloc] initWithBoundsSize:artworkImage.size 
                                                                          requestHandler:^NSImage * _Nonnull(CGSize size) {
                (void)size; // Suppress unused parameter warning
                return artworkImage;
            }];
            nowPlayingInfo[MPMediaItemPropertyArtwork] = artwork;
            // printf("Added artwork to Now Playing info\n");
        } else {
            // printf("WARNING: Could not load artwork from: %s\n", [artworkPath UTF8String]);
        }
        
        // printf("Setting now playing info with %lu items\n", (unsigned long)[nowPlayingInfo count]);
        
        // Set the new info
        center.nowPlayingInfo = nowPlayingInfo;
        
        // Ensure our commands are enabled (but don't do the disable/re-enable cycle)
        MPRemoteCommandCenter *commandCenter = [MPRemoteCommandCenter sharedCommandCenter];
        commandCenter.playCommand.enabled = YES;
        commandCenter.pauseCommand.enabled = YES;
        commandCenter.togglePlayPauseCommand.enabled = YES;
        
        // Verify it was set
        // NSDictionary *verifyInfo = center.nowPlayingInfo;
        // printf("Verified now playing info has %lu items\n", verifyInfo ? (unsigned long)[verifyInfo count] : 0);
        
        // printf("Now playing info set successfully\n");
    }
    
    void updateNowPlayingInfoWithArtwork(const char* title, const char* artist, const char* album, 
                                        double duration, double currentTime, bool isPlaying, const char* artworkPath) {
        // printf("updateNowPlayingInfoWithArtwork() called:\n");
        // printf("  Title: %s\n", title ? title : "(null)");
        // printf("  Artist: %s\n", artist ? artist : "(null)");
        // printf("  Album: %s\n", album ? album : "(null)");
        // printf("  Duration: %.2f seconds\n", duration);
        // printf("  Current Time: %.2f seconds\n", currentTime);
        // printf("  Is Playing: %s\n", isPlaying ? "YES" : "NO");
        // printf("  Artwork Path: %s\n", artworkPath ? artworkPath : "(null)");
        
        // Check if MPNowPlayingInfoCenter is available
        MPNowPlayingInfoCenter *center = [MPNowPlayingInfoCenter defaultCenter];
        if (!center) {
            // printf("ERROR: MPNowPlayingInfoCenter not available!\n");
            return;
        }
        
        NSMutableDictionary *nowPlayingInfo = [[NSMutableDictionary alloc] init];
        
        if (title) {
            nowPlayingInfo[MPMediaItemPropertyTitle] = [NSString stringWithUTF8String:title];
        }
        if (artist) {
            nowPlayingInfo[MPMediaItemPropertyArtist] = [NSString stringWithUTF8String:artist];
        }
        if (album) {
            nowPlayingInfo[MPMediaItemPropertyAlbumTitle] = [NSString stringWithUTF8String:album];
        }
        
        nowPlayingInfo[MPMediaItemPropertyPlaybackDuration] = @(duration);
        nowPlayingInfo[MPNowPlayingInfoPropertyElapsedPlaybackTime] = @(currentTime);
        nowPlayingInfo[MPNowPlayingInfoPropertyPlaybackRate] = isPlaying ? @(1.0) : @(0.0);
        
        // printf("Setting playback rate to: %.1f (isPlaying: %s)\n", isPlaying ? 1.0 : 0.0, isPlaying ? "YES" : "NO");
        
        // Add media type and metadata
        nowPlayingInfo[MPMediaItemPropertyMediaType] = @(MPMediaTypeMusic);
        nowPlayingInfo[MPMediaItemPropertyAlbumArtist] = [NSString stringWithUTF8String:artist];
        nowPlayingInfo[MPNowPlayingInfoPropertyDefaultPlaybackRate] = @(1.0);
        nowPlayingInfo[MPNowPlayingInfoPropertyPlaybackQueueIndex] = @(0);
        nowPlayingInfo[MPNowPlayingInfoPropertyPlaybackQueueCount] = @(1);
        
        // Add SquareDesk artwork
        if (artworkPath) {
            NSString *artworkPathStr = [NSString stringWithUTF8String:artworkPath];
            NSImage *artworkImage = [[NSImage alloc] initWithContentsOfFile:artworkPathStr];
            if (artworkImage) {
                // printf("Loaded artwork image: %s (size: %.0fx%.0f)\n",
                //        artworkPath, artworkImage.size.width, artworkImage.size.height);
                
                MPMediaItemArtwork *artwork = [[MPMediaItemArtwork alloc] initWithBoundsSize:artworkImage.size 
                                                                              requestHandler:^NSImage * _Nonnull(CGSize size) {
                    (void)size; // Suppress unused parameter warning
                    return artworkImage;
                }];
                nowPlayingInfo[MPMediaItemPropertyArtwork] = artwork;
                // printf("Added artwork to Now Playing info\n");
            } else {
                // printf("WARNING: Could not load artwork from: %s\n", artworkPath);
            }
        }
        
        // printf("Setting now playing info with %lu items\n", (unsigned long)[nowPlayingInfo count]);
        
        // Set the new info
        center.nowPlayingInfo = nowPlayingInfo;
        
        // Ensure our commands are enabled (but don't do the disable/re-enable cycle)
        MPRemoteCommandCenter *commandCenter = [MPRemoteCommandCenter sharedCommandCenter];
        commandCenter.playCommand.enabled = YES;
        commandCenter.pauseCommand.enabled = YES;
        commandCenter.togglePlayPauseCommand.enabled = YES;
        
        // Verify it was set
        // NSDictionary *verifyInfo = center.nowPlayingInfo;
        // printf("Verified now playing info has %lu items\n", verifyInfo ? (unsigned long)[verifyInfo count] : 0);
        
        // printf("Now playing info with artwork set successfully\n");
    }
    
    void setNowPlayingCallbacks(void (*playCallback)(), void (*pauseCallback)(), 
                               void (*nextCallback)(), void (*prevCallback)(),
                               void (*seekCallback)(double)) {
        // printf("setNowPlayingCallbacks() called\n");
        g_playCallback = playCallback;
        g_pauseCallback = pauseCallback;
        g_nextCallback = nextCallback;
        g_prevCallback = prevCallback;
        g_seekCallback = seekCallback;
        // printf("Callbacks set: play=%p, pause=%p, next=%p, prev=%p, seek=%p\n", 
        //        playCallback, pauseCallback, nextCallback, prevCallback, seekCallback);
    }
}
