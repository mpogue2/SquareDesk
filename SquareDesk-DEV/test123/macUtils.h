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

#import <IOKit/pwr_mgt/IOPMLib.h>

#ifndef MACUTILS_H
#define MACUTILS_H

class MacUtils
{

public:
    MacUtils();
    ~MacUtils();

//    void disableScreensaver();
//    void reenableScreensaver();
    void disableWindowTabbing();

//private:
//    IOPMAssertionID assertionID;

};

// ----------------------------------------------------------------------------
// Helpers for parenting a JUCE plugin window (e.g. the LoudMax FX panel) to the
//   main SquareDesk window.  Both handles are NSView* (that is what QWindow::winId()
//   and juce::Component::getWindowHandle() both return on macOS); each helper walks
//   up to the enclosing NSWindow itself.
//
// These MUST live in a .mm file.  A previous attempt at this lived in
//   mainwindow_JUCE.cpp inside an "#ifdef __OBJC__", but that file is compiled as
//   plain C++, so the entire body silently compiled away to nothing (Issue #1707).

// Mark an auxiliary window so it can join the main window in its Full Screen Space,
//   instead of yanking the user back to the default Space when it is shown.
// Call this BEFORE the window is first made visible.
void prepareAuxWindowMac(void *auxWindowHandle);

// Make auxWindow a child of mainWindow: it then stays above the main window, moves
//   with it, hides/shows with it, and follows it into and out of Full Screen.
void attachChildWindowMac(void *mainWindowHandle, void *auxWindowHandle);

// Undo attachChildWindowMac(), whoever the parent happens to be.  Safe to call if the
//   window was never attached.
void detachChildWindowMac(void *auxWindowHandle);

#endif
