TEMPLATE = lib
CONFIG += staticlib

# SD makes extensive use of dynamic exception specifications, which are disallowed in C++17
CONFIG += c++11
QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS -= -std=gnu++1z

HEADERS = database.h  mapcachefile.h  resource.h  sd.h       sdui.h \
    deploy.h paths.h sdprint.h sort.h
#deploy.h    paths.h         sdbase.h    sdprint.h  sort.h

SOURCES = sdmain.cpp sdutil.cpp sdbasic.cpp sdinit.cpp \
             sdtables.cpp sdctable.cpp sdtop.cpp sdconcpt.cpp sdpreds.cpp \
             sdgetout.cpp sdmoves.cpp sdtand.cpp sdconc.cpp sdistort.cpp \
             mapcachefile.cpp sdpick.cpp sdsi.cpp sdmatch.cpp common.cpp 
