#-------------------------------------------------
#
# Project created by QtCreator 2016-07-07T21:58:24
#
#-------------------------------------------------

QT       += core gui

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
    analogclock.cpp

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
    common_enums.h

FORMS    += mainwindow.ui \
    preferencesdialog.ui

INCLUDEPATH += $$PWD/
DEPENDPATH += $$PWD/

# NOTE: there is no debug version of libbass
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/ -lbass -lbass_fx -lbassmix
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/ -lbass -lbass_fx -lbassmix
else:unix: LIBS += -L$$PWD/ -lbass -lbass_fx -lbassmix

win32 {
    RC_FILE = desk1d.rc
}

macx {
    # http://stackoverflow.com/questions/1361229/using-a-static-library-in-qt-creator
    LIBS += $$PWD/libbass.dylib $$PWD/libbass_fx.dylib $$PWD/libbassmix.dylib
    mylib.path = Contents/MacOS
    mylib.files = $$PWD/libbass.dylib $$PWD/libbass_fx.dylib $$PWD/libbassmix.dylib
    QMAKE_BUNDLE_DATA += mylib

    # add taglib static library, FIX: move to the squaredesk directory!
    # LIBS += -L/Users/mpogue/_squareMike/taglib-1.10/binaries/lib -ltag
    #INCLUDEPATH += /Users/mpogue/_squareMike/taglib-1.10/binaries/include
    #/usr/lib/libz.dylib
    #LIBS += /usr/lib/libz.dylib

    ICON = $$PWD/desk1d.icns
    DISTFILES += desk1d.icns

    # https://forum.qt.io/topic/58926/solved-xcode-7-and-qt-error/2
    # Every time you get this error, do "ls /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/"
    #   in Terminal and change the QMAKE_MAC_SDK variable accordingly.
    QMAKE_MAC_SDK = macosx10.12
}

RESOURCES += resources.qrc

#DISTFILES += \
#    README.txt
