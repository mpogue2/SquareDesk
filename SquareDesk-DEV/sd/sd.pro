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
#win32 {
    SOURCES += main.cpp  # this is just a dummy file
    # Copy the sdtty.exe and sd_calls.dat to the test123 RELEASE dirs
    # sdtty.exe was pre-compiled externally with the makeSDforWin32 script
    copydata.commands = xcopy /q /y $$shell_path($$PWD/sdlib.dll) $$shell_path($$OUT_PWD\..\test123\debug)
    copydata2.commands = xcopy /q /y $$shell_path($$PWD/sd_calls.dat) $$shell_path($$OUT_PWD\..\test123\debug)
#    copydata3.commands = xcopy /q /y $$shell_path($$PWD/sdtty.exe) $$shell_path($$OUT_PWD\debug)
    copydata4.commands = xcopy /q /y $$shell_path($$PWD/sdtty.exe) $$shell_path($$OUT_PWD\..\test123\debug)
    first.depends = $(first) copydata copydata2 copydata4
    export(first.depends)
    export(copydata.commands)
    export(copydata2.commands)
#    export(copydata3.commands)
    export(copydata4.commands)
    QMAKE_EXTRA_TARGETS += first copydata copydata2 copydata4
}

win32:CONFIG(release, debug|release): {
    SOURCES += main.cpp  # this is just a dummy file
    # Copy the sdtty.exe and sd_calls.dat to the test123 RELEASE dirs
    # sdtty.exe was pre-compiled externally with the makeSDforWin32 script
    copydata.commands = xcopy /q /y $$shell_path($$PWD/sdlib.dll) $$shell_path($$OUT_PWD\..\test123\release)
    copydata2.commands = xcopy /q /y $$shell_path($$PWD/sd_calls.dat) $$shell_path($$OUT_PWD\..\test123\release)
#    copydata3.commands = xcopy /q /y $$shell_path($$PWD/sdtty.exe) $$shell_path($$OUT_PWD\debug)
    copydata4.commands = xcopy /q /y $$shell_path($$PWD/sdtty.exe) $$shell_path($$OUT_PWD\..\test123\release)
    first.depends = $(first) copydata copydata2 copydata4
    export(first.depends)
    export(copydata.commands)
    export(copydata2.commands)
#    export(copydata3.commands)
    export(copydata4.commands)
    QMAKE_EXTRA_TARGETS += first copydata copydata2 copydata4
}


#QMAKE_CXXFLAGS += -Wall -Wno-switch -Wno-uninitialized -Wno-char-subscripts -Wunused-parameter -Wunused-const-variable -Wunused-parameter
#QMAKE_CXXFLAGS += -Wmissing-field-initializers -Wswitch -Wsometimes-uninitialized

QMAKE_CXXFLAGS += -w

# NOTE: compilation for Win32 must be done with makeSDforWin32 script
#   in the minGW environment, NOT HERE.
#   I'm not sure about Linux.

mac {
SOURCES += common.cpp \
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
}

HEADERS += \
    database.h \
    mapcachefile.h \
    resource.h \
    sd.h \
    sdbase.h \
    sdui.h \
    sort.h \
    paths.h
