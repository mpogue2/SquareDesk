TEMPLATE = aux
OBJECTS_DIR = ./
DESTDIR = ./

macx: {
# Mac OS X:
#   1) Install brew: ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)" < /dev/null 2> /dev/null
#   2) Let brew install cmake: brew install cmake
#   3) Check to make sure it's there: which cmake
#
# NOTE: we will rename the versioned libtidy.5.5.31.dylib to just libtidy.5.dylib, so as to avoid symbolic links when we copy it to the final .app .
#       the installed symlinks are deleted (they are not needed by the .app).
# NOTE: the version number is currently hard-coded here.
# NOTE: the compiled libs go into SquareDesk-DEV/local/lib
#       the includes go into SquareDesk-DEV/local/include

html-tidy.target = html-tidy
html-tidy.commands = \
   cd $$PWD/build/cmake && \
#   rm CMakeCache.txt && \
   /usr/local/bin/cmake ../.. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PWD/../../../../local && \
   make && make install && \
   cd $$PWD/../local/lib && \
   rm libtidy.dylib && \
   rm libtidy.5.dylib && \
   mv libtidy.5.5.31.dylib libtidy.5.dylib

QMAKE_EXTRA_TARGETS += html-tidy
}

win32: {
# Windows: install tidy by hand to local_win32
#
#html-tidy.target = html-tidy
#html-tidy.commands = cd $$PWD/../html-tidy/build/cmake && \
#   del CMakeCache.txt && \
#   cmake ../.. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$$PWD/../local_win32 && \
#   cmake --build . --config Release && \
#   cmake --build . --config Release --target INSTALL

#QMAKE_EXTRA_TARGETS += html-tidy
#PRE_TARGETDEPS += html-tidy
}

unix:!macx: {
# Linux
html-tidy.target = html-tidy
html-tidy.commands = mkdir -p $$PWD/../local/lib $$PWD/../local/include $$PWD/../local/bin && \
   cd $$PWD/../html-tidy/build/cmake && \
   cmake ../.. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$$PWD/../local && \
   make && make install

QMAKE_EXTRA_TARGETS += html-tidy
PRE_TARGETDEPS += html-tidy
}

HEADERS += \
    include/tidy.h

SOURCES +=
