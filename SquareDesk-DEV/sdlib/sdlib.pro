TEMPLATE = lib
CONFIG += staticlib

HEADERS = database.h  mapcachefile.h  resource.h  sd.h       sdui.h \
 deploy.h    paths.h         sdbase.h    sdprint.h  sort.h

SOURCES = sdmain.cpp sdutil.cpp sdbasic.cpp sdinit.cpp \
             sdtables.cpp sdctable.cpp sdtop.cpp sdconcpt.cpp sdpreds.cpp \
             sdgetout.cpp sdmoves.cpp sdtand.cpp sdconc.cpp sdistort.cpp \
             mapcachefile.cpp sdpick.cpp sdsi.cpp sdmatch.cpp common.cpp 
