TEMPLATE = lib
CONFIG += qt warn_on
QT -= gui

# The ABI version.

!win32:VERSION = 1.0.0

# 1.0.0 is the first stable ABI.
# The next binary incompatible change will be 2.0.0 and so on.
# The existing QuaZIP policy on changing ABI requires to bump the
# major version of QuaZIP itself as well. Note that there may be
# other reasons for chaging the major version of QuaZIP, so
# in case where there is a QuaZIP major version bump but no ABI change,
# the VERSION variable will stay the same.

# For example:

# QuaZIP 1.0 is released after some 0.x, keeping binary compatibility.
# VERSION stays 1.0.0.
# Then some binary incompatible change is introduced. QuaZIP goes up to
# 2.0, VERSION to 2.0.0.
# And so on.


# This one handles dllimport/dllexport directives.
DEFINES += QUAZIP_BUILD

# You'll need to define this one manually if using a build system other
# than qmake or using QuaZIP sources directly in your project.
CONFIG(staticlib): DEFINES += QUAZIP_STATIC


macx {
# Do "brew install zlib" beforehand!  libz is no longer in /usr/lib
#    LIBS += /usr/lib/libz.dylib
    LIBS += /usr/local/opt/zlib/lib/libz.dylib
}

# Input
include(quazip.pri)


win32:CONFIG(debug, debug|release) {
TARGET = $$join(TARGET,,,d)
INCLUDEPATH += $$PWD/ $$PWD/../../local_win32/include
DEPENDPATH += $$PWD/ $$PWD/../../local_win32/include
LIBS += -L$$PWD/ -L$$PWD/../../local_win32/lib -lzlib
}

win32:CONFIG(release, debug|release) {
#     win32: TARGET = $$join(TARGET,,,d)
INCLUDEPATH += $$PWD/ $$PWD/../../local_win32/include
DEPENDPATH += $$PWD/ $$PWD/../../local_win32/include
LIBS += -L$$PWD/ -L$$PWD/../../local_win32/lib -lzlib

# copy out to the local_win32 location, where it will be picked up by test123.pro
    copydata3q.commands = xcopy /q /y $$shell_path($$OUT_PWD\release\quazip.dll) $$shell_path($$PWD/../../local_win32/bin)
    copydata3r.commands = xcopy /q /y $$shell_path($$PWD\*.h) $$shell_path($$PWD/../../local_win32/include)
    copydata3s.commands = xcopy /q /y $$shell_path($$OUT_PWD\release\quazip*.lib) $$shell_path($$PWD/../../local_win32/lib)
    first.depends = $(first) copydata3q copydata3r copydata3s
    export(copydata3q.commands)
    export(copydata3r.commands)
    export(copydata3s.commands)
    QMAKE_EXTRA_TARGETS += first copydata3q copydata3r copydata3s
}

unix:!symbian {
    headers.path=$$PREFIX/include/quazip
    headers.files=$$HEADERS
    target.path=$$PREFIX/lib/$${LIB_ARCH}
    INSTALLS += headers target

        OBJECTS_DIR=.obj
        MOC_DIR=.moc

}

win32 {
    headers.path=$$PREFIX/include/quazip
    headers.files=$$HEADERS
    target.path=$$PREFIX/lib
    INSTALLS += headers target
    # workaround for qdatetime.h macro bug
    DEFINES += NOMINMAX
}


symbian {

    # Note, on Symbian you may run into troubles with LGPL.
    # The point is, if your application uses some version of QuaZip,
    # and a newer binary compatible version of QuaZip is released, then
    # the users of your application must be able to relink it with the
    # new QuaZip version. For example, to take advantage of some QuaZip
    # bug fixes.

    # This is probably best achieved by building QuaZip as a static
    # library and providing linkable object files of your application,
    # so users can relink it.

    CONFIG += staticlib
    CONFIG += debug_and_release

    LIBS += -lezip

    #Export headers to SDK Epoc32/include directory
    exportheaders.sources = $$HEADERS
    exportheaders.path = quazip
    for(header, exportheaders.sources) {
        BLD_INF_RULES.prj_exports += "$$header $$exportheaders.path/$$basename(header)"
    }
}
