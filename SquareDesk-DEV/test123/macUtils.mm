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
