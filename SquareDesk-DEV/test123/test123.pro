#-------------------------------------------------
#
# Project created by QtCreator 2016-07-07T21:58:24
#
#-------------------------------------------------

QT       += core gui sql network printsupport svg

macx {
    QT += webenginewidgets
    PRE_TARGETDEPS += $$OUT_PWD/../sdlib/libsdlib.a
}

win32:CONFIG(debug, debug|release): {
    QT += webenginewidgets
    PRE_TARGETDEPS += $$OUT_PWD/../sdlib/debug/sdlib.lib
}
win32:CONFIG(release, debug|release): {
    QT += webenginewidgets
    PRE_TARGETDEPS += $$OUT_PWD/../sdlib/release/sdlib.lib
}

unix:!macx {
    QT += webkitwidgets
    PRE_TARGETDEPS += $$OUT_PWD/../sdlib/libsdlib.a
}

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SquareDesk
TEMPLATE = app

#  turn off QML warning for Debug builds
DEFINES += QT_QML_DEBUG_NO_WARNING

SOURCES += main.cpp\
        mainwindow.cpp \
    bass_audio.cpp \
    preferencesdialog.cpp \
    importdialog.cpp \
    exportdialog.cpp \
    songhistoryexportdialog.cpp \
    mytablewidget.cpp \
    tablenumberitem.cpp \
    myslider.cpp \
    levelmeter.cpp \
    analogclock.cpp \
    prefsmanager.cpp \
    clickablelabel.cpp \
    songsettings.cpp \
    typetracker.cpp \
    console.cpp \
    renderarea.cpp \
    sdhighlighter.cpp \
    utility.cpp \
    danceprograms.cpp \
    startupwizard.cpp \
    keybindings.cpp \
    calllistcheckbox.cpp \
    sdlineedit.cpp \
    downloadmanager.cpp \
    sdinterface.cpp \
    mainwindow_sd.cpp \
    songtitlelabel.cpp \
    sdsequencecalllabel.cpp \
    perftimer.cpp \
    tablewidgettimingitem.cpp \
    mydatetimeedit.cpp \
    sdredostack.cpp \
    makeflashdrivewizard.cpp

macx {
SOURCES += ../qpdfjs/src/communicator.cpp

# this hint from: https://forum.qt.io/topic/74432/os-x-deployment-problem-rpath-framework/5
QMAKE_LFLAGS += -Wl,-rpath,@loader_path/../,-rpath,@executable_path/../,-rpath,@executable_path/../Frameworks
}

HEADERS  += mainwindow.h \
    bass.h \
    bass_fx.h \
    bass_audio.h \
    myslider.h \
    bassmix.h \
    importdialog.h \
    exportdialog.h \
    songhistoryexportdialog.h \
    preferencesdialog.h \
    utility.h \
    mytablewidget.h \
    tablenumberitem.h \
    levelmeter.h \
    analogclock.h \
    common_enums.h \
    prefs_options.h \
    prefsmanager.h \
    default_colors.h \
    macUtils.h \
    clickablelabel.h \
    typetracker.h \
    console.h \
    renderarea.h \
    common.h \
    sdhighlighter.h \
    danceprograms.h \
    startupwizard.h \
    songsettings.h \
    platform.h \
    keybindings.h \
    calllistcheckbox.h \
    sdlineedit.h \
    downloadmanager.h \
    sdinterface.h \
    songtitlelabel.h \
    sdsequencecalllabel.h \
    perftimer.h \
    tablewidgettimingitem.h \
    mydatetimeedit.h \
    sdredostack.h \
    makeflashdrivewizard.h

macx {
HEADERS += ../qpdfjs/src/communicator.h
INCLUDEPATH += $$PWD/../qpdfjs
}

win32 {
SOURCES += ../qpdfjs/src/communicator.cpp
HEADERS += ../qpdfjs/src/communicator.h
INCLUDEPATH += $$PWD/../qpdfjs
}

FORMS    += mainwindow.ui \
    importdialog.ui \
    exportdialog.ui \
    songhistoryexportdialog.ui \
    preferencesdialog.ui

macx {
INCLUDEPATH += $$PWD/ $$PWD/../local/include
DEPENDPATH += $$PWD/ $$PWD/../local/include
}

win32 {
INCLUDEPATH += $$PWD/ $$PWD/../local_win32/include
DEPENDPATH += $$PWD/ $$PWD/../local_win32/include
}

unix:!macx {
INCLUDEPATH += $$PWD/ $$PWD/../local/include
DEPENDPATH += $$PWD/ $$PWD/../local/include
LIBS += -L$$PWD/../sdlib -lsdlib
}


# NOTE: there is no debug version of libbass
win32: LIBS += -L$$PWD/ -L$$PWD/../local_win32/lib -lbass -lbass_fx -lbassmix -luser32 -ltidy -lquazip
else:unix:!macx: LIBS += -L$$PWD/ -L$$PWD/../local/lib -lbass -lbass_fx -lbassmix -ltag -lsqlite3 -ltidys
# macx: see below...

win32:CONFIG(debug, debug|release): {
    RC_FILE = desk1d.rc
    LIBS += -L$$OUT_PWD/../taglib -ltaglib
    INCLUDEPATH += $$PWD/../taglib/binaries/include

    LIBS += -L$$OUT_PWD/../sdlib/debug -lsdlib
}
win32:CONFIG(release, debug|release): {
    RC_FILE = desk1d.rc
    LIBS += -L$$OUT_PWD/../taglib -ltaglib
    INCLUDEPATH += $$PWD/../taglib/binaries/include

    LIBS += -L$$OUT_PWD/../sdlib/release -lsdlib
}

# copy the 3 libbass DLLs and the allcalls.csv file to the executable directory (DEBUG ONLY)
win32:CONFIG(debug, debug|release): {
    copydata.commands = xcopy /q /y $$shell_path($$PWD/windll/*.dll) $$shell_path($$OUT_PWD\debug)
    copydata3.commands = xcopy /q /y $$shell_path($$PWD/allcalls.csv) $$shell_path($$OUT_PWD\debug)
    copydata30.commands = xcopy /q /y $$shell_path($$PWD/sd_calls.dat) $$shell_path($$OUT_PWD\debug)
    first.depends = $(first) copydata copydata3 copydata30
    export(first.depends)
    export(copydata.commands)
    export(copydata3.commands)
    export(copydata30.commands)
    QMAKE_EXTRA_TARGETS += first copydata copydata3 copydata30

    # LYRICS AND PATTER TEMPLATES --------------------------------------------
    # Copy the lyrics.template.html and patter.template.html files to the right place
    copydata0a.commands = xcopy /q /y  $$shell_path($$PWD/lyrics.template.html) $$shell_path($$OUT_PWD\debug)
    copydata0b.commands = xcopy /q /y  $$shell_path($$PWD/cuesheet2.css)        $$shell_path($$OUT_PWD\debug)
    copydata0c.commands = xcopy /q /y  $$shell_path($$PWD/patter.template.html) $$shell_path($$OUT_PWD\debug)
    first.depends += $(first) copydata0a copydata0b copydata0c
    export(copydata0a.commands)
    export(copydata0b.commands)
    export(copydata0c.commands)
    QMAKE_EXTRA_TARGETS += copydata0a copydata0b copydata0c
}

# copy the 3 libbass DLLs and the allcalls.csv file to the executable directory (DEBUG ONLY)
win32:CONFIG(release, debug|release): {
    copydata.commands = xcopy /q /y $$shell_path($$PWD/windll/*.dll) $$shell_path($$OUT_PWD\release)
    copydata3.commands = xcopy /q /y $$shell_path($$PWD/allcalls.csv) $$shell_path($$OUT_PWD\release)
    copydata30.commands = xcopy /q /y $$shell_path($$PWD/sd_calls.dat) $$shell_path($$OUT_PWD\release)
    first.depends = $(first) copydata copydata3 copydata30
    export(first.depends)
    export(copydata.commands)
    export(copydata3.commands)
    export(copydata30.commands)
    QMAKE_EXTRA_TARGETS += first copydata copydata3 copydata30

    # LYRICS AND PATTER TEMPLATES --------------------------------------------
    # Copy the lyrics.template.html and patter.template.html files to the right place
    copydata0a.commands = xcopy /q /y  $$shell_path($$PWD/lyrics.template.html) $$shell_path($$OUT_PWD\release)
    copydata0b.commands = xcopy /q /y  $$shell_path($$PWD/cuesheet2.css)        $$shell_path($$OUT_PWD\release)
    copydata0c.commands = xcopy /q /y  $$shell_path($$PWD/patter.template.html) $$shell_path($$OUT_PWD\release)
    first.depends += $(first) copydata0a copydata0b copydata0c
    export(copydata0a.commands)
    export(copydata0b.commands)
    export(copydata0c.commands)
    QMAKE_EXTRA_TARGETS += copydata0a copydata0b copydata0c
}

macx {
    # LIBBASS, LIBBASS_FX, LIBBASSMIX ---------------
    # http://stackoverflow.com/questions/1361229/using-a-static-library-in-qt-creator
    LIBS += $$PWD/libbass.dylib $$PWD/libbass_fx.dylib $$PWD/libbassmix.dylib
    #LIBS += $$PWD/libquazip.1.dylib
    LIBS += $$OUT_PWD/../quazip/quazip/libquazip.1.0.0.dylib
    LIBS += $$PWD/../local/lib/libtidy.5.dylib
    LIBS += -framework CoreFoundation
    LIBS += -framework AppKit

    mylib.path = Contents/MacOS
    mylib.files = $$PWD/libbass.dylib $$PWD/libbass_fx.dylib $$PWD/libbassmix.dylib
    mylib.files += $$PWD/../local/lib/libtidy.5.dylib
    #mylib.files += $$PWD/libquazip.1.dylib
    mylib.files += $$OUT_PWD/../quazip/quazip/libquazip.1.0.0.dylib
    QMAKE_BUNDLE_DATA += mylib

    # NOTE: I compiled QuaZIP in the Qt environment, then copied the Quazip.1.0.0.dylib to the test123 directory with
    #   with the name quazip.1.dylib .  This allows it to link.  There's gotta be a better way to reference these
    #   libs that is cross platform.  Maybe here is a clue:  https://www.youtube.com/watch?v=mxlcKmvMK9Q&ab_channel=VoidRealms

    INCLUDEPATH += $$PWD/../quazip/quazip  # reference includes like this:  #include "JlCompress.h"

    # TAGLIB ----------------------------------------
    LIBS += -L$$OUT_PWD/../taglib -ltaglib
    INCLUDEPATH += $$PWD/../taglib/binaries/include

    # ZLIB ------------------------------------------
    LIBS += /usr/lib/libz.dylib

    # SDLIB ------------------------------------------
    LIBS += -L$$OUT_PWD/../sdlib -lsdlib

    # ICONS, ALLCALLS.CSV ---------------------------
    ICON = $$PWD/desk1d.icns
    DISTFILES += desk1d.icns
    DISTFILES += $$PWD/allcalls.csv  # RESOURCE: list of calls, and which level they are

    # https://forum.qt.io/topic/58926/solved-xcode-7-and-qt-error/2
    # Every time you get this error, do "ls /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/"
    #   in Terminal and change the QMAKE_MAC_SDK variable accordingly.
    QMAKE_MAC_SDK = macosx10.13

    # LYRICS AND PATTER TEMPLATES --------------------------------------------
    # Copy the lyrics.template.html and patter.template.html files to the right place
    copydata0a.commands = $(COPY_DIR) $$PWD/lyrics.template.html $$OUT_PWD/SquareDesk.app/Contents/Resources
    copydata0b.commands = $(COPY_DIR) $$PWD/cuesheet2.css        $$OUT_PWD/SquareDesk.app/Contents/Resources
    copydata0c.commands = $(COPY_DIR) $$PWD/patter.template.html $$OUT_PWD/SquareDesk.app/Contents/Resources

    # SD --------------------------------------------
    # Copy the sd executable and the sd_calls.dat data file to the same place as the sd executable
    #  (inside the SquareDesk.app bundle)
    # This way, it's easy for SDP to find the executable for sd, and it's easy for SDP to start up sd.
#    copydata1.commands = $(COPY_DIR) $$PWD/../sd/sd_calls.dat $$OUT_PWD/SquareDesk.app/Contents/MacOS
    copydata1.commands = $(COPY_DIR) $$PWD/sd_calls.dat $$OUT_PWD/SquareDesk.app/Contents/MacOS

    #copydata2.commands = $(COPY_DIR) $$OUT_PWD/../sd/sd $$OUT_PWD/SquareDesk.app/Contents/MacOS
    copydata3.commands = $(COPY_DIR) $$PWD/allcalls.csv $$OUT_PWD/SquareDesk.app/Contents/Resources

    # PS --------------------------------------------
    # SEE the postBuildStepMacOS for a description of how pocketsphinx is modified for embedding.
    #   https://github.com/auriamg/macdylibbundler  <-- BEST, and the one I used
    #   https://doc.qt.io/archives/qq/qq09-mac-deployment.html
    #   http://stackoverflow.com/questions/1596945/building-osx-app-bundle
    #   http://www.chilkatforum.com/questions/4235/how-to-distribute-a-dylib-with-a-mac-os-x-application
    #   http://stackoverflow.com/questions/2092378/macosx-how-to-collect-dependencies-into-a-local-bundle

    # Copy the ps executable and the libraries it depends on (into the SquareDesk.app bundle)
    # ***** WARNING: the path to pocketsphinx source files is specific to my particular laptop! *****
    copydata4.commands = $(COPY_DIR) /Users/mpogue/Documents/QtProjects/SquareDeskPlayer/SquareDesk-DEV/pocketsphinx/binaries/macosx_yosemite/exe/pocketsphinx_continuous $$OUT_PWD/SquareDesk.app/Contents/MacOS
    copydata5.commands = $(COPY_DIR) /Users/mpogue/Documents/QtProjects/SquareDeskPlayer/SquareDesk-DEV/pocketsphinx/binaries/macosx_yosemite/libs $$OUT_PWD/SquareDesk.app/Contents

    copydata6a.commands = $(MKDIR) $$OUT_PWD/SquareDesk.app/Contents/models/en-us
    copydata6b.commands = $(COPY_DIR) /Users/mpogue/Documents/QtProjects/SquareDeskPlayer/SquareDesk-DEV/pocketsphinx/binaries/macosx_yosemite/models/en-us $$OUT_PWD/SquareDesk.app/Contents/models

    # SQUAREDESK-SPECIFIC DICTIONARY, LANGUAGE MODEL --------------------------------------------
    copydata7.commands = $(COPY_DIR) $$PWD/5365a.dic $$OUT_PWD/SquareDesk.app/Contents/MacOS
    copydata8.commands = $(COPY_DIR) $$PWD/plus.jsgf $$OUT_PWD/SquareDesk.app/Contents/MacOS

    #first.depends = $(first) copydata0a copydata0b copydata0c copydata1 copydata2 copydata3 copydata4 copydata5 copydata6a copydata6b copydata7 copydata8
    first.depends = $(first) copydata0a copydata0b copydata0c copydata1 copydata3 copydata4 copydata5 copydata6a copydata6b copydata7 copydata8

    #export(first.depends)
    export(copydata0a.commands)
    export(copydata0b.commands)
    export(copydata0c.commands)
    export(copydata1.commands)
    export(copydata2.commands)
    export(copydata3.commands)
    export(copydata4.commands)
    export(copydata5.commands)
    export(copydata6a.commands)
    export(copydata6b.commands)
    export(copydata7.commands)
    export(copydata8.commands)

    QMAKE_EXTRA_TARGETS += first copydata0a copydata0b copydata0c copydata1 copydata2 copydata3 copydata4 copydata5 copydata6a copydata6b copydata7 copydata8

    # SOUNDFX STARTER SET --------------------------------------------
    copydata10.commands = $(MKDIR) $$OUT_PWD/SquareDesk.app/Contents/soundfx
    copydata11a.commands = $(COPY_DIR) $$PWD/soundfx/1.whistle.mp3 $$OUT_PWD/SquareDesk.app/Contents/soundfx
    copydata11b.commands = $(COPY_DIR) $$PWD/soundfx/2.clown_honk.mp3 $$OUT_PWD/SquareDesk.app/Contents/soundfx
    copydata11c.commands = $(COPY_DIR) $$PWD/soundfx/3.submarine.mp3 $$OUT_PWD/SquareDesk.app/Contents/soundfx
    copydata11d.commands = $(COPY_DIR) $$PWD/soundfx/4.applause.mp3 $$OUT_PWD/SquareDesk.app/Contents/soundfx
    copydata11e.commands = $(COPY_DIR) $$PWD/soundfx/5.fanfare.mp3 $$OUT_PWD/SquareDesk.app/Contents/soundfx
    copydata11f.commands = $(COPY_DIR) $$PWD/soundfx/6.fade.mp3 $$OUT_PWD/SquareDesk.app/Contents/soundfx
    copydata11f2.commands = $(COPY_DIR) $$PWD/soundfx/7.short_bell.mp3 $$OUT_PWD/SquareDesk.app/Contents/soundfx
    copydata11f3.commands = $(COPY_DIR) $$PWD/soundfx/8.ding_ding.mp3 $$OUT_PWD/SquareDesk.app/Contents/soundfx
    copydata11g.commands = $(COPY_DIR) $$PWD/soundfx/break_over.mp3 $$OUT_PWD/SquareDesk.app/Contents/soundfx
    copydata11h.commands = $(COPY_DIR) $$PWD/soundfx/long_tip.mp3 $$OUT_PWD/SquareDesk.app/Contents/soundfx
    copydata12h.commands = $(COPY_DIR) $$PWD/soundfx/thirty_second_warning.mp3 $$OUT_PWD/SquareDesk.app/Contents/soundfx

    first.depends += copydata10 copydata11a copydata11b copydata11c copydata11d copydata11e copydata11f copydata11f2 copydata11f3 copydata11g copydata11h copydata12h

    export(first.depends)
    export(copydata10.commands)
    export(copydata11a.commands)
    export(copydata11b.commands)
    export(copydata11c.commands)
    export(copydata11d.commands)
    export(copydata11e.commands)
    export(copydata11f.commands)
    export(copydata11f2.commands)
    export(copydata11f3.commands)
    export(copydata11g.commands)
    export(copydata11h.commands)
    export(copydata12h.commands)

    QMAKE_EXTRA_TARGETS += copydata10 copydata11a copydata11b copydata11c copydata11d copydata11e copydata11f copydata11f2 copydata11f3 copydata11g copydata11h copydata12h

    # For the PDF viewer -----------------
    copydata1p.commands = $(MKDIR) $$OUT_PWD/SquareDesk.app/Contents/MacOS/minified
    copydata2p.commands = $(COPY_DIR) $$PWD/../qpdfjs/minified/web   $$OUT_PWD/SquareDesk.app/Contents/MacOS/minified
    copydata3p.commands = $(COPY_DIR) $$PWD/../qpdfjs/minified/build $$OUT_PWD/SquareDesk.app/Contents/MacOS/minified
    copydata4p.commands = $(RM) $$OUT_PWD/SquareDesk.app/Contents/MacOS/minified/web/compressed.*.pdf

    first.depends += copydata1p copydata2p copydata3p copydata4p
    export(first.depends)
    export(copydata1p.commands)
    export(copydata2p.commands)
    export(copydata3p.commands)
    export(copydata4p.commands)
    QMAKE_EXTRA_TARGETS += copydata1p copydata2p copydata3p copydata4p

    # For the QUAZIP library -- we need exactly the right name on the library -----------------
    # yes, this is a rename.  I don't know how to do this in QMake directly.
    copydata1q.commands = $(RM) $$OUT_PWD/SquareDesk.app/Contents/MacOS/libquazip.1.dylib
    copydata2q.commands = $(COPY) $$OUT_PWD/SquareDesk.app/Contents/MacOS/libquazip.1.0.0.dylib $$OUT_PWD/SquareDesk.app/Contents/MacOS/libquazip.1.dylib
    copydata3q.commands = $(RM) $$OUT_PWD/SquareDesk.app/Contents/MacOS/libquazip.1.0.0.dylib
    first.depends += copydata1q copydata2q copydata3q
    export(first.depends)
    export(copydata1q.commands)
    export(copydata2q.commands)
    export(copydata3q.commands)
    QMAKE_EXTRA_TARGETS += copydata1q copydata2q copydata3q
}

win32:CONFIG(debug, debug|release): {
    # PS --------------------------------------------
    # Copy the ps executable and the libraries it depends on (into the SquareDesk.app bundle)
    # ***** WARNING: the path to pocketsphinx source files is specific to my particular laptop! *****
    copydata4.commands = xcopy /q /y $$shell_path($$PWD/../pocketsphinx/binaries/win32/exe/pocketsphinx_continuous.exe) $$shell_path($$OUT_PWD/debug)
    copydata5.commands = xcopy /q /y $$shell_path($$PWD/../pocketsphinx/binaries/win32/exe/pocketsphinx.dll) $$shell_path($$OUT_PWD/debug)
    copydata6.commands = xcopy /q /y $$shell_path($$PWD/../pocketsphinx/binaries/win32/exe/sphinxbase.dll) $$shell_path($$OUT_PWD/debug)

    copydata7.commands = if not exist $$shell_path($$OUT_PWD/debug/models/en-us) mkdir $$shell_path($$OUT_PWD/debug/models/en-us)
    # NOTE: The models used for Win32 PocketSphinx are NOT the same as the Mac OS X models.
    copydata8.commands = xcopy /q /y $$shell_path($$PWD/../pocketsphinx/binaries/win32/models/en-us/*) $$shell_path($$OUT_PWD/debug/models/en-us)

    # SQUAREDESK-SPECIFIC DICTIONARY, LANGUAGE MODEL --------------------------------------------
    copydata9.commands = xcopy /q /y $$shell_path($$PWD/5365a.dic) $$shell_path($$OUT_PWD/debug)
    copydata10.commands = xcopy /q /y $$shell_path($$PWD/plus.jsgf) $$shell_path($$OUT_PWD/debug)

    first.depends += $(first) copydata4 copydata5 copydata6 copydata7 copydata8 copydata9 copydata10

    export(first.depends)
    export(copydata4.commands)
    export(copydata5.commands)
    export(copydata6.commands)
    export(copydata7.commands)
    export(copydata8.commands)
    export(copydata9.commands)
    export(copydata10.commands)

    QMAKE_EXTRA_TARGETS += first copydata4 copydata5 copydata6 copydata7 copydata8 copydata9 copydata10

    # SOUNDFX STARTER SET --------------------------------------------
    copydata10b.commands = if not exist $$shell_path($$OUT_PWD/debug/soundfx) mkdir $$shell_path($$OUT_PWD/debug/soundfx)
    copydata11a.commands = xcopy /q /y $$shell_path($$PWD/soundfx/1.whistle.mp3) $$shell_path($$OUT_PWD/debug/soundfx)
    copydata11b.commands = xcopy /q /y $$shell_path($$PWD/soundfx/2.clown_honk.mp3) $$shell_path($$OUT_PWD/debug/soundfx)
    copydata11c.commands = xcopy /q /y $$shell_path($$PWD/soundfx/3.submarine.mp3) $$shell_path($$OUT_PWD/debug/soundfx)
    copydata11d.commands = xcopy /q /y $$shell_path($$PWD/soundfx/4.applause.mp3) $$shell_path($$OUT_PWD/debug/soundfx)
    copydata11e.commands = xcopy /q /y $$shell_path($$PWD/soundfx/5.fanfare.mp3) $$shell_path($$OUT_PWD/debug/soundfx)
    copydata11f.commands = xcopy /q /y $$shell_path($$PWD/soundfx/6.fade.mp3) $$shell_path($$OUT_PWD/debug/soundfx)
    copydata11f2.commands = xcopy /q /y $$shell_path($$PWD/soundfx/7.short_bell.mp3) $$shell_path($$OUT_PWD/debug/soundfx)
    copydata11f3.commands = xcopy /q /y $$shell_path($$PWD/soundfx/8.ding_ding.mp3) $$shell_path($$OUT_PWD/debug/soundfx)
    copydata11g.commands = xcopy /q /y $$shell_path($$PWD/soundfx/break_over.mp3) $$shell_path($$OUT_PWD/debug/soundfx)
    copydata11h.commands = xcopy /q /y $$shell_path($$PWD/soundfx/long_tip.mp3) $$shell_path($$OUT_PWD/debug/soundfx)
    copydata12h.commands = xcopy /q /y $$shell_path($$PWD/soundfx/thirty_second_warning.mp3) $$shell_path($$OUT_PWD/debug/soundfx)

    first.depends += copydata10b copydata11a copydata11b copydata11c copydata11d copydata11e copydata11f copydata11f2 copydata11f3 copydata11g copydata11h copydata12h

    export(first.depends)
    export(copydata10b.commands)
    export(copydata11a.commands)
    export(copydata11b.commands)
    export(copydata11c.commands)
    export(copydata11d.commands)
    export(copydata11e.commands)
    export(copydata11f.commands)
    export(copydata11f2.commands)
    export(copydata11f3.commands)
    export(copydata11g.commands)
    export(copydata11h.commands)
    export(copydata12h.commands)

    QMAKE_EXTRA_TARGETS += copydata10b copydata11a copydata11b copydata11c copydata11d copydata11e copydata11f copydata11f2 copydata11f3 copydata11g copydata11h copydata12h

    # For the PDF viewer -----------------
    copydata1p.commands = if not exist $$shell_path($$OUT_PWD/debug/minified) $(MKDIR) $$shell_path($$OUT_PWD/debug/minified)
    copydata2p.commands = $(COPY_DIR) $$shell_path($$PWD/../qpdfjs/minified/web)   $$shell_path($$OUT_PWD/debug/minified/web)
    copydata3p.commands = $(COPY_DIR) $$shell_path($$PWD/../qpdfjs/minified/build) $$shell_path($$OUT_PWD/debug/minified/build)
    #copydata4p.commands = $(RM) $$shell_path($$OUT_PWD/debug/minified/web/compressed.*.pdf)

    first.depends += copydata1p copydata2p copydata3p #copydata4p
    export(first.depends)
    export(copydata1p.commands)
    export(copydata2p.commands)
    export(copydata3p.commands)
    #export(copydata4p.commands)
    QMAKE_EXTRA_TARGETS += copydata1p copydata2p copydata3p #copydata4p

}

win32:CONFIG(release, debug|release): {
    # PS --------------------------------------------
    # Copy the ps executable and the libraries it depends on (into the SquareDesk.app bundle)
    # ***** WARNING: the path to pocketsphinx source files is specific to my particular laptop! *****
    copydata4.commands = xcopy /q /y $$shell_path($$PWD/../pocketsphinx/binaries/win32/exe/pocketsphinx_continuous.exe) $$shell_path($$OUT_PWD/release)
    copydata5.commands = xcopy /q /y $$shell_path($$PWD/../pocketsphinx/binaries/win32/exe/pocketsphinx.dll) $$shell_path($$OUT_PWD/release)
    copydata6.commands = xcopy /q /y $$shell_path($$PWD/../pocketsphinx/binaries/win32/exe/sphinxbase.dll) $$shell_path($$OUT_PWD/release)

    copydata7.commands = if not exist $$shell_path($$OUT_PWD/release/models/en-us) mkdir $$shell_path($$OUT_PWD/release/models/en-us)
    # NOTE: The models used for Win32 PocketSphinx are NOT the same as the Mac OS X models.
    copydata8.commands = xcopy /q /y $$shell_path($$PWD/../pocketsphinx/binaries/win32/models/en-us/*) $$shell_path($$OUT_PWD/release/models/en-us)

    # SQUAREDESK-SPECIFIC DICTIONARY, LANGUAGE MODEL --------------------------------------------
    copydata9.commands = xcopy /q /y $$shell_path($$PWD/5365a.dic) $$shell_path($$OUT_PWD/release)
    copydata10.commands = xcopy /q /y $$shell_path($$PWD/plus.jsgf) $$shell_path($$OUT_PWD/release)

    first.depends += $(first) copydata4 copydata5 copydata6 copydata7 copydata8 copydata9 copydata10

    export(first.depends)
    export(copydata4.commands)
    export(copydata5.commands)
    export(copydata6.commands)
    export(copydata7.commands)
    export(copydata8.commands)
    export(copydata9.commands)
    export(copydata10.commands)

    QMAKE_EXTRA_TARGETS += first copydata4 copydata5 copydata6 copydata7 copydata8 copydata9 copydata10

    # SOUNDFX STARTER SET --------------------------------------------
    copydata10b.commands = if not exist $$shell_path($$OUT_PWD/release/soundfx) mkdir $$shell_path($$OUT_PWD/release/soundfx)
    copydata11a.commands = xcopy /q /y $$shell_path($$PWD/soundfx/1.whistle.mp3) $$shell_path($$OUT_PWD/release/soundfx)
    copydata11b.commands = xcopy /q /y $$shell_path($$PWD/soundfx/2.clown_honk.mp3) $$shell_path($$OUT_PWD/release/soundfx)
    copydata11c.commands = xcopy /q /y $$shell_path($$PWD/soundfx/3.submarine.mp3) $$shell_path($$OUT_PWD/release/soundfx)
    copydata11d.commands = xcopy /q /y $$shell_path($$PWD/soundfx/4.applause.mp3) $$shell_path($$OUT_PWD/release/soundfx)
    copydata11e.commands = xcopy /q /y $$shell_path($$PWD/soundfx/5.fanfare.mp3) $$shell_path($$OUT_PWD/release/soundfx)
    copydata11f.commands = xcopy /q /y $$shell_path($$PWD/soundfx/6.fade.mp3) $$shell_path($$OUT_PWD/release/soundfx)
    copydata11g.commands = xcopy /q /y $$shell_path($$PWD/soundfx/break_over.mp3) $$shell_path($$OUT_PWD/release/soundfx)
    copydata11h.commands = xcopy /q /y $$shell_path($$PWD/soundfx/long_tip.mp3) $$shell_path($$OUT_PWD/release/soundfx)
    copydata12h.commands = xcopy /q /y $$shell_path($$PWD/soundfx/thirty_second_warning.mp3) $$shell_path($$OUT_PWD/release/soundfx)

    first.depends += copydata10b copydata11a copydata11b copydata11c copydata11d copydata11e copydata11f copydata11g copydata11h copydata12h

    export(first.depends)
    export(copydata10b.commands)
    export(copydata11a.commands)
    export(copydata11b.commands)
    export(copydata11c.commands)
    export(copydata11d.commands)
    export(copydata11e.commands)
    export(copydata11f.commands)
    export(copydata11g.commands)
    export(copydata11h.commands)
    export(copydata12h.commands)

    QMAKE_EXTRA_TARGETS += copydata10b copydata11a copydata11b copydata11c copydata11d copydata11e copydata11f copydata11g copydata11h copydata12h

    # For the PDF viewer -----------------
    # For the PDF viewer -----------------
    copydata1p.commands = if not exist $$shell_path($$OUT_PWD/release/minified) $(MKDIR) $$shell_path($$OUT_PWD/release/minified)
    copydata2p.commands = $(COPY_DIR) $$shell_path($$PWD/../qpdfjs/minified/web)   $$shell_path($$OUT_PWD/release/minified/web)
    copydata3p.commands = $(COPY_DIR) $$shell_path($$PWD/../qpdfjs/minified/build) $$shell_path($$OUT_PWD/release/minified/build)
    #copydata4p.commands = $(RM) $$shell_path($$OUT_PWD/debug/minified/web/compressed.*.pdf)

    first.depends += copydata1p copydata2p copydata3p #copydata4p
    export(first.depends)
    export(copydata1p.commands)
    export(copydata2p.commands)
    export(copydata3p.commands)
    #export(copydata4p.commands)
    QMAKE_EXTRA_TARGETS += copydata1p copydata2p copydata3p #copydata4p
}

RESOURCES += resources.qrc
RESOURCES += startupwizard.qrc

#DISTFILES += \
#    README.txt

OBJECTIVE_SOURCES += \
    macUtils.mm

DISTFILES += \
    LICENSE.GPL3 \
    LICENSE.GPL2 \
    cuesheet2.css \
    lyrics.template.html

CONFIG += c++11

