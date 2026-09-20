TEMPLATE = lib
CONFIG += staticlib

# Qt 6 requires C++17, so that is what SD is built as.
#   There was a "CONFIG += c++11" block here, justified by "SD makes extensive use of dynamic
#   exception specifications, which are disallowed in C++17".  Neither half of that still held:
#   SD has no dynamic exception specifications left, and the pin had never taken effect anyway,
#   because Qt 6 mandates C++17 and qmake appends its own -std=gnu++1z after ours, so the later
#   flag won.  SD has therefore been built as C++17 all along.  Issue #1755.
CONFIG += c++17

# SD is vendored third-party code that we are not going to fix, so silence the warning classes
#   it produces in bulk (5397 of them, 92% missing-field-initializers from the big static
#   tables).  Scoped to this .pro, so none of it reaches SquareDesk's own code: test123 links
#   libsdlib.a but never includes the SD headers.  Issue #1755.
#
#   These MUST go on QMAKE_CXXFLAGS_WARN_ON rather than QMAKE_CXXFLAGS.  qmake appends
#   -Wall -Wextra *after* QMAKE_CXXFLAGS, and the later flag wins, so -Wall would turn -Wswitch
#   back on and -Wextra would turn -Wmissing-field-initializers back on -- suppressing only 35
#   of the 5397.  QMAKE_CXXFLAGS_WARN_ON *is* the "-Wall -Wextra" variable, so appending here
#   puts the negations after them, where they stick.
QMAKE_CXXFLAGS_WARN_ON += -Wno-missing-field-initializers \
                          -Wno-switch \
                          -Wno-deprecated-declarations \
                          -Wno-unused-parameter \
                          -Wno-unused-const-variable

HEADERS = database.h  mapcachefile.h  resource.h  sd.h       sdui.h \
    deploy.h paths.h sdprint.h sort.h sdchars.h sdmatch.h
#deploy.h    paths.h         sdbase.h    sdprint.h  sort.h

SOURCES = sdmain.cpp sdutil.cpp sdbasic.cpp sdinit.cpp \
             sdtables.cpp sdctable.cpp sdtop.cpp sdconcpt.cpp sdpreds.cpp \
             sdgetout.cpp sdmoves.cpp sdtand.cpp sdconc.cpp sdistort.cpp \
             mapcachefile.cpp sdpick.cpp sdsi.cpp sdmatch.cpp common.cpp 
