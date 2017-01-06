#-------------------------------------------------
#
# Project created by QtCreator 2016-07-07T21:58:24
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SquareDeskPlayer
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    bass_audio.cpp \
    preferencesdialog.cpp \
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
    sdhighlighter.cpp

HEADERS  += mainwindow.h \
    bass.h \
    bass_fx.h \
    bass_audio.h \
    myslider.h \
    bassmix.h \
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
    sdhighlighter.h

FORMS    += mainwindow.ui \
    preferencesdialog.ui

INCLUDEPATH += $$PWD/
DEPENDPATH += $$PWD/

# NOTE: there is no debug version of libbass
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/ -lbass -lbass_fx -lbassmix -luser32 -lsqlite3
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/ -lbass -lbass_fx -lbassmix -luser32 -lsqlite3
else:unix: LIBS += -L$$PWD/ -lbass -lbass_fx -lbassmix -lsqlite3

win32 {
    RC_FILE = desk1d.rc
    LIBS += -L$$OUT_PWD/../taglib -ltaglib
    INCLUDEPATH += $$PWD/../taglib/binaries/include
}

# copy the 3 libbass DLLs and the allcalls.csv file to the executable directory (DEBUG ONLY)
win32:CONFIG(debug, debug|release): {
    copydata.commands = xcopy /q /y $$shell_path($$PWD/windll/*.dll) $$shell_path($$OUT_PWD\debug)
    copydata3.commands = xcopy /q /y $$shell_path($$PWD/allcalls.csv) $$shell_path($$OUT_PWD\debug)
    first.depends = $(first) copydata copydata3
    export(first.depends)
    export(copydata.commands)
    export(copydata3.commands)
    QMAKE_EXTRA_TARGETS += first copydata copydata3
}

# copy the 3 libbass DLLs and the allcalls.csv file to the executable directory (DEBUG ONLY)
win32:CONFIG(release, debug|release): {
    copydata.commands = xcopy /q /y $$shell_path($$PWD/windll/*.dll) $$shell_path($$OUT_PWD\release)
    copydata3.commands = xcopy /q /y $$shell_path($$PWD/allcalls.csv) $$shell_path($$OUT_PWD\release)
    first.depends = $(first) copydata copydata3
    export(first.depends)
    export(copydata.commands)
    export(copydata3.commands)
    QMAKE_EXTRA_TARGETS += first copydata copydata3
}

macx {
    # http://stackoverflow.com/questions/1361229/using-a-static-library-in-qt-creator
    LIBS += $$PWD/libbass.dylib $$PWD/libbass_fx.dylib $$PWD/libbassmix.dylib
    LIBS += -framework CoreFoundation
    mylib.path = Contents/MacOS
    mylib.files = $$PWD/libbass.dylib $$PWD/libbass_fx.dylib $$PWD/libbassmix.dylib
    QMAKE_BUNDLE_DATA += mylib

#     add taglib static library, FIX: move to the squaredesk directory!
#     LIBS += -L/Users/mpogue/_squareMike/taglib-1.10/binaries/lib -ltag
#    LIBS += -L/Users/mpogue/_squareMike/taglib-1.10/taglib-debug -ltag
    LIBS += -L$$OUT_PWD/../taglib -ltaglib
#    INCLUDEPATH += /Users/mpogue/_squareMike/taglib-1.10/binaries/include
    INCLUDEPATH += $$PWD/../taglib/binaries/include
#    /usr/lib/libz.dylib
    LIBS += /usr/lib/libz.dylib

    ICON = $$PWD/desk1d.icns
    DISTFILES += desk1d.icns
    DISTFILES += $$PWD/allcalls.csv  # RESOURCE: list of calls, and which level they are

    # https://forum.qt.io/topic/58926/solved-xcode-7-and-qt-error/2
    # Every time you get this error, do "ls /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/"
    #   in Terminal and change the QMAKE_MAC_SDK variable accordingly.
    QMAKE_MAC_SDK = macosx10.12
}

mac: {
    # Copy the sd executable and the sd_calls.dat data file to the same place as the sd executable
    #  (inside the SquareDeskPlayer.app bundle)
    # This way, it's easy for SDP to find the executable for sd, and it's easy for SDP to start up sd.
    copydata1.commands = $(COPY_DIR) $$PWD/../sd/sd_calls.dat $$OUT_PWD/SquareDeskPlayer.app/Contents/MacOS
    copydata2.commands = $(COPY_DIR) $$OUT_PWD/../sd/sd $$OUT_PWD/SquareDeskPlayer.app/Contents/MacOS
    copydata3.commands = $(COPY_DIR) $$PWD/allcalls.csv $$OUT_PWD/SquareDeskPlayer.app/Contents/Resources

#    first.depends = $(first) copydata copydata2 copydata3
#    export(first.depends)
#    export(copydata.commands)
#    export(copydata2.commands)
#    export(copydata3.commands)
#    QMAKE_EXTRA_TARGETS += first copydata copydata2 copydata3

    # Copy the ps executable and the .dict and .jsgf data files to the same place as the ps executable
    #  (inside the SquareDeskPlayer.app bundle)
    # This way, it's easy for SDP to find the executable for ps, and it's easy for SDP to start up ps.
    # ***** NOTE: the path to pocketsphinx is specific to my particular installation! *****
#    copydata4.commands = $(COPY_DIR) /usr/local/Cellar/cmu-pocketsphinx/HEAD-584be6e/bin/pocketsphinx_continuous $$OUT_PWD/SquareDeskPlayer.app/Contents/MacOS
    copydata5.commands = $(COPY_DIR) $$PWD/5365a.dic $$OUT_PWD/SquareDeskPlayer.app/Contents/MacOS
    copydata6.commands = $(COPY_DIR) $$PWD/plus.jsgf $$OUT_PWD/SquareDeskPlayer.app/Contents/MacOS

    # TODO: need to include the dependent libraries of pocketsphinx, too.
    # NO.  Dependent libraries (there are actually 3 of them) are done via the postBuildStepMac.  And, that's only
    #   done when needed, not every build, because it modified the pocketsphinx executable and libraries.
    #   Do not muck with this, unless you have to, because it's fragile.

#    copydata7.commands = $(COPY_DIR) /usr/local/opt/cmu-sphinxbase/lib/libsphinxbase.3.dylib $$OUT_PWD/SquareDeskPlayer.app/Contents/MacOS
#    copydata8.commands = $(COPY_DIR) /usr/local/opt/cmu-sphinxbase/lib/libsphinxad.3.dylib   $$OUT_PWD/SquareDeskPlayer.app/Contents/MacOS

    #   https://github.com/auriamg/macdylibbundler  <-- BEST, and the one I used
    #
    #   https://doc.qt.io/archives/qq/qq09-mac-deployment.html
    #   http://stackoverflow.com/questions/1596945/building-osx-app-bundle
    #   http://www.chilkatforum.com/questions/4235/how-to-distribute-a-dylib-with-a-mac-os-x-application
    #   http://stackoverflow.com/questions/2092378/macosx-how-to-collect-dependencies-into-a-local-bundle

#    first.depends = $(first) copydata1 copydata2 copydata3 copydata4 copydata5 copydata6 copydata7 copydata8
    first.depends = $(first) copydata1 copydata2 copydata3 copydata5 copydata6

    export(first.depends)
    export(copydata1.commands)
    export(copydata2.commands)
    export(copydata3.commands)
#    export(copydata4.commands)
    export(copydata5.commands)
    export(copydata6.commands)
#    export(copydata7.commands)
#    export(copydata8.commands)

#    QMAKE_EXTRA_TARGETS += first copydata1 copydata2 copydata3 copydata4 copydata5 copydata6 copydata7 copydata8
    QMAKE_EXTRA_TARGETS += first copydata1 copydata2 copydata3 copydata5 copydata6
}


RESOURCES += resources.qrc

#DISTFILES += \
#    README.txt

OBJECTIVE_SOURCES += \
    macUtils.mm

DISTFILES += \
    LICENSE.GPL3 \
    LICENSE.GPL2
