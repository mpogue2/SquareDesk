# =========================================================================================
# EXTERNAL LIBRARIES
#
#   Included by test123.pro, which defines JUCE_ROOT, JUCE_MODULES, JUCE_BUILD and JUCE_LIB.
# =========================================================================================

unix {
# this hint from: https://forum.qt.io/topic/74432/os-x-deployment-problem-rpath-framework/5
QMAKE_LFLAGS += -Wl,-rpath,@loader_path/../,-rpath,@executable_path/../ #,-rpath,@executable_path/../Frameworks
}

macx {
INCLUDEPATH += $$PWD/
DEPENDPATH += $$PWD/

# FOR JUCE:
INCLUDEPATH += $$JUCE_ROOT/JuceLibraryCode $$JUCE_MODULES/modules
}

unix:!macx {
DEFINES += JUCE_DEBUG
LIBS += -L$$OUT_PWD/../libJUCEstatic/Builds/LinuxMakefile/build -lJUCEstatic 
LIBS += -lz -ljpeg -lcurl
LIBS += -L$$OUT_PWD/../taglib -ltaglib -lz -lfreetype -lpng
INCLUDEPATH += /usr/share/juce/modules
INCLUDEPATH += /usr/include/freetype2
INCLUDEPATH += $$PWD/../taglib/binaries/include
INCLUDEPATH += $$PWD/../taglib
INCLUDEPATH += $$PWD/../taglib/taglib
INCLUDEPATH += $$PWD/../taglib/taglib/toolkit
INCLUDEPATH += $$PWD/../taglib/taglib/mpeg/id3v2
INCLUDEPATH += $$PWD/ $$PWD/../local/include $$(HOME)/local/include $$(HOME)/local/include/soundtouch 
DEPENDPATH += $$PWD/ $$PWD/../local/include
LIBS += -L$$PWD/../sdlib -lsdlib
LIBS += -L$$(HOME)/local/lib -lkfr_dsp -lkfr_io

QT += multimedia httpserver concurrent

# MiniBPM for BPM detection -----------------------------------
INCLUDEPATH += $$PWD/miniBPM

# SoundTouch for pitch/tempo changing -----------------------------------
INCLUDEPATH += $$PWD/soundtouch/include

}

unix:!macx: LIBS += -L$$PWD/ -L$$PWD/../local/lib -ltag -lsqlite3

# macOS ***********************************************************************
#   NOTE: the order of the LIBS lines below is the link order.  Leave it alone unless
#   you mean to change it.
macx {
LIBS += -framework CoreFoundation
LIBS += -framework AppKit
LIBS += -framework MediaPlayer

# TAGLIB ----------------------------------------
LIBS += -L$$OUT_PWD/../taglib -ltaglib
INCLUDEPATH += $$PWD/../taglib/binaries/include
INCLUDEPATH += $$PWD/../taglib
INCLUDEPATH += $$PWD/../taglib/taglib
INCLUDEPATH += $$PWD/../taglib/taglib/toolkit
INCLUDEPATH += $$PWD/../taglib/taglib/mpeg/id3v2
INCLUDEPATH += $$PWD/../taglib/taglib/riff
INCLUDEPATH += $$PWD/../taglib/taglib/riff/wav

# JUCE ------------
LIBS += -L$$JUCE_ROOT/Builds/MacOSX/build/$$JUCE_BUILD -l$$JUCE_LIB
LIBS += -framework QuartzCore
LIBS += -framework Security
LIBS += -framework Accelerate
LIBS += -framework WebKit
LIBS += -framework AudioToolbox
LIBS += -framework CoreAudioKit
LIBS += -framework iTunesLibrary

# KFR for filters -------------------------------
#   create-kfr-lib builds libkfr if it is not there yet.
QMAKE_EXTRA_TARGETS += libkfr
CONFIG(debug, debug|release) {
    KFR_BUILD_TYPE = debug
} else {
    KFR_BUILD_TYPE = release
}
KFR_DIR = $$OUT_PWD/../kfr
KFR_LIB = $$KFR_DIR/lib
message(KFR_LIB is $$KFR_LIB)

libkfr.target = $$KFR_LIB
libkfr.depends =
libkfr.commands = $$PWD/../kfr/create-kfr-lib $$KFR_DIR $$KFR_BUILD_TYPE

PRE_TARGETDEPS += $$libkfr.target

INCLUDEPATH += $$PWD/../kfr/include
LIBS += -L$$KFR_LIB -lkfr_dsp_neon64 -lkfr_io

# JUCE INSTALL ----------------------------------
#   ../juce-install fetches JUCE and builds libJUCEstatic if they are not there yet.
QMAKE_EXTRA_TARGETS += libJUCE JUCE
libJUCE.target = $$JUCE_ROOT
libJUCE.depends =
libJUCE.commands = zsh $$PWD/../juce-install
JUCE.target = $$JUCE_MODULES
JUCE.depends =
JUCE.commands = zsh $$PWD/../juce-install
PRE_TARGETDEPS += $$libJUCE.target

# MiniBPM for BPM detection -----------------------------------
INCLUDEPATH += $$PWD/miniBPM

# SoundTouch for pitch/tempo changing -----------------------------------
INCLUDEPATH += $$PWD/soundtouch/include

# SDLIB ------------------------------------------
LIBS += -L$$OUT_PWD/../sdlib -lsdlib
}
