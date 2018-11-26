QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

TARGET = mp3gain
INCLUDEPATH += /usr/local/include

LIBS += -L/usr/local/lib -lm -lmpg123
QMAKE_CXXFLAGS += -DHAVE_MEMCPY

mac {
    SOURCES += \
        apetag.c \
        gain_analysis.c \
        id3tag.c \
        mp3gain.c \
        rg_error.c

    HEADERS += \
        apetag.h \
        gain_analysis.h \
        id3tag.h \
        mp3gain.h \
        resource.h \
        rg_error.h
}

DISTFILES += \
    lgpl.txt
