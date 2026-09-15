#-------------------------------------------------
#
# Project created by QtCreator 2016-07-07T21:58:24
#
# This file holds the project's identity and configuration.  Everything else lives in the
# .pri files included at the bottom:
#
#   sources.pri        what gets compiled: SOURCES, HEADERS, FORMS, RESOURCES, DISTFILES
#   thirdparty.pri     external libraries: taglib, JUCE, KFR, SoundTouch, MiniBPM, sdlib
#   deploy_macos.pri   building the .app bundle: Info.plist, bundle files, signing, DMG
#   asan.pri           the optional CONFIG+=asan build
#
#-------------------------------------------------

QT       += core gui sql network printsupport svg svgwidgets
unix {
    QT += webenginewidgets
    PRE_TARGETDEPS += $$OUT_PWD/../sdlib/libsdlib.a
}
macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 13.0

    # for BULK operations
    QT += concurrent

    # for Taminations HTTP server
    QT += httpserver
}

# =========================================================================================
# BUILD CONFIGURATION
#
#   Every value here can be overridden on the qmake command line, or in QtCreator under
#   Projects > Build & Run > Build > qmake step > "Additional arguments", e.g.
#       qmake ... CODESIGN_ID=- JUCE_ROOT=$HOME/JUCE/libJUCEstatic
#   so that nobody has to edit this file just to build on their own machine.
#
macx {
    # Apple SDK to build against.  See the NOTE in the next block before changing it.
    isEmpty(MAC_SDK):      MAC_SDK = macosx27.0

    # Where ../juce-install puts the static JUCE library, and where the JUCE modules live.
    isEmpty(JUCE_ROOT):    JUCE_ROOT = $$(HOME)/JUCEProjects/libJUCEstatic
    isEmpty(JUCE_MODULES): JUCE_MODULES = /Applications/JUCE

    # Which libJUCEstatic build to link against.
    #   NOTE: this is the Debug build even for Release builds of SquareDesk, which is what
    #   this project has always done.  See issue #1729 before changing it.
    isEmpty(JUCE_BUILD):   JUCE_BUILD = Debug
    isEmpty(JUCE_LIB):     JUCE_LIB = JUCE_debug

    # Code signing identity for the post-link re-sign (see the codesign section, far below).
    #   To find yours:  security find-identity -v -p codesigning | grep "Apple Development"
    #   Use CODESIGN_ID=- for ad-hoc signing if you have no Apple Developer certificate.
    isEmpty(CODESIGN_ID):  CODESIGN_ID = "Apple Development: Michael Pogue (6K9PD3928V)"
}

macx {
  # VARIABLE REFERENCE: https://doc.qt.io/qt-6/qmake-variable-reference.html
  #
  # NOTE: QMAKE_MAC_SDK MUST BE ALL LOWER CASE, and fully spelled out ("macosx27.0", not
  #   "macosx27").  Otherwise the command that <QtDir>/macos/mkspecs/features/macos/sdk.prf
  #   issues, "/usr/bin/xcrun --sdk macosx27.0 --show-sdk-version", fails with:
  #   "Could not resolve SDK SDKVersion for 'MacOSX27.0' using --show-sdk-version"
  #
  # When Apple ships a new SDK, make this match the latest version in:
  #   ls /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs
  #
  # NOTE: when this changes, you must delete the stale .qmake.stash in the BUILD directory by
  #   hand.  It lives one level above test123, so it is not regenerated.  See QTBUG-43015.
  QMAKE_MAC_SDK = $$MAC_SDK
}

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SquareDesk
TEMPLATE = app

macx {
    # M1MAC is defined on macOS only, and is tested by mainwindow.cpp, mainwindow_init.cpp
    #   and mainwindow_filemgmt.cpp.  It is not defined for the Linux build.
    DEFINES += M1MAC=1

    # TEMPORARY: turn off complaining when the result of file.open() is not looked at
    DEFINES += QT_NO_USE_NODISCARD_FILE_OPEN

    QT += multimedia
}

#  turn off QML warning for Debug builds
DEFINES += QT_QML_DEBUG_NO_WARNING

CONFIG += c++17

include(sources.pri)
include(thirdparty.pri)
include(deploy_macos.pri)
include(asan.pri)
