TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

mac {
# Copy the sd_calls.dat to the same place as the executable
copydata.commands = $(COPY_DIR) $$PWD/sd_calls.dat $$OUT_PWD
first.depends = $(first) copydata
export(first.depends)
export(copydata.commands)
QMAKE_EXTRA_TARGETS += first copydata
}


win32:CONFIG(debug, debug|release): {
    # Copy the sd_calls.dat to the same place as the executable (DEBUG ONLY)
    copydata.commands = xcopy /q /y $$shell_path($$PWD/sd_calls.dat) $$shell_path($$OUT_PWD\debug)
    first.depends = $(first) copydata
    export(first.depends)
    export(copydata.commands)
    QMAKE_EXTRA_TARGETS += first copydata
}

#QMAKE_CXXFLAGS += -Wall -Wno-switch -Wno-uninitialized -Wno-char-subscripts -Wunused-parameter -Wunused-const-variable -Wunused-parameter
#QMAKE_CXXFLAGS += -Wmissing-field-initializers -Wswitch -Wsometimes-uninitialized

QMAKE_CXXFLAGS += -w

SOURCES += main.cpp \
    common.cpp \
    mapcachefile.cpp \
    sdbasic.cpp \
    sdconc.cpp \
    sdconcpt.cpp \
    sdctable.cpp \
    sdgetout.cpp \
    sdinit.cpp \
    sdistort.cpp \
    sdmain.cpp \
    sdmatch.cpp \
    sdmoves.cpp \
    sdpick.cpp \
    sdpreds.cpp \
    sdsi.cpp \
    sdtables.cpp \
    sdtand.cpp \
    sdtop.cpp \
    sdui-ttu.cpp \
    sdui-tty.cpp \
    sdutil.cpp

HEADERS += \
    database.h \
    mapcachefile.h \
    resource.h \
    sd.h \
    sdbase.h \
    sdui.h \
    sort.h \
    paths.h
