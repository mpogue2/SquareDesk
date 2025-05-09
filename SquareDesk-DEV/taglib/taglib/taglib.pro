TEMPLATE = lib
CONFIG += staticlib

INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD/ape
INCLUDEPATH += $$PWD/asf
INCLUDEPATH += $$PWD/CMakeFiles
INCLUDEPATH += $$PWD/dsdiff
INCLUDEPATH += $$PWD/dsf
INCLUDEPATH += $$PWD/flac
INCLUDEPATH += $$PWD/it
INCLUDEPATH += $$PWD/mod
INCLUDEPATH += $$PWD/mp4
INCLUDEPATH += $$PWD/mpc
INCLUDEPATH += $$PWD/mpeg
INCLUDEPATH += $$PWD/ogg
INCLUDEPATH += $$PWD/riff
INCLUDEPATH += $$PWD/s3m
INCLUDEPATH += $$PWD/toolkit
INCLUDEPATH += $$PWD/trueaudio
INCLUDEPATH += $$PWD/wavpack
INCLUDEPATH += $$PWD/xm
INCLUDEPATH += $$PWD/mpeg/id3v1
INCLUDEPATH += $$PWD/mpeg/id3v2
INCLUDEPATH += $$PWD/mpeg/id3v2/frames
INCLUDEPATH += $$PWD/ogg/flac
INCLUDEPATH += $$PWD/ogg/opus
INCLUDEPATH += $$PWD/ogg/speex
INCLUDEPATH += $$PWD/ogg/vorbis
INCLUDEPATH += $$PWD/riff/aiff
INCLUDEPATH += $$PWD/riff/wav
INCLUDEPATH += $$PWD/../3rdparty/utfcpp

HEADERS += $$PWD/audioproperties.h
HEADERS += $$PWD/fileref.h
HEADERS += $$PWD/tag.h
HEADERS += $$PWD/taglib_export.h
HEADERS += $$PWD/tagunion.h
HEADERS += $$PWD/ape/apefile.h
HEADERS += $$PWD/ape/apefooter.h
HEADERS += $$PWD/ape/apeitem.h
HEADERS += $$PWD/ape/apeproperties.h
HEADERS += $$PWD/ape/apetag.h
HEADERS += $$PWD/asf/asfattribute.h
HEADERS += $$PWD/asf/asffile.h
HEADERS += $$PWD/asf/asfpicture.h
HEADERS += $$PWD/asf/asfproperties.h
HEADERS += $$PWD/asf/asftag.h
HEADERS += $$PWD/asf/asfutils.h
HEADERS += $$PWD/dsf/dsffile.h
HEADERS += $$PWD/dsf/dsfproperties.h
HEADERS += $$PWD/flac/flacfile.h
HEADERS += $$PWD/flac/flacmetadatablock.h
HEADERS += $$PWD/flac/flacpicture.h
HEADERS += $$PWD/flac/flacproperties.h
HEADERS += $$PWD/flac/flacunknownmetadatablock.h
HEADERS += $$PWD/it/itfile.h
HEADERS += $$PWD/it/itproperties.h
HEADERS += $$PWD/mod/modfile.h
HEADERS += $$PWD/mod/modfilebase.h
HEADERS += $$PWD/mod/modfileprivate.h
HEADERS += $$PWD/mod/modproperties.h
HEADERS += $$PWD/mod/modtag.h
HEADERS += $$PWD/mp4/mp4atom.h
HEADERS += $$PWD/mp4/mp4coverart.h
HEADERS += $$PWD/mp4/mp4file.h
HEADERS += $$PWD/mp4/mp4item.h
HEADERS += $$PWD/mp4/mp4properties.h
HEADERS += $$PWD/mp4/mp4tag.h
HEADERS += $$PWD/mpc/mpcfile.h
HEADERS += $$PWD/mpc/mpcproperties.h
HEADERS += $$PWD/mpeg/mpegfile.h
HEADERS += $$PWD/mpeg/mpegheader.h
HEADERS += $$PWD/mpeg/mpegproperties.h
HEADERS += $$PWD/mpeg/xingheader.h
HEADERS += $$PWD/mpeg/id3v1/id3v1genres.h
HEADERS += $$PWD/mpeg/id3v1/id3v1tag.h
HEADERS += $$PWD/mpeg/id3v2/id3v2extendedheader.h
HEADERS += $$PWD/mpeg/id3v2/id3v2footer.h
HEADERS += $$PWD/mpeg/id3v2/id3v2frame.h
HEADERS += $$PWD/mpeg/id3v2/id3v2framefactory.h
HEADERS += $$PWD/mpeg/id3v2/id3v2header.h
HEADERS += $$PWD/mpeg/id3v2/id3v2synchdata.h
HEADERS += $$PWD/mpeg/id3v2/id3v2tag.h
HEADERS += $$PWD/mpeg/id3v2/frames/attachedpictureframe.h
HEADERS += $$PWD/mpeg/id3v2/frames/chapterframe.h
HEADERS += $$PWD/mpeg/id3v2/frames/commentsframe.h
HEADERS += $$PWD/mpeg/id3v2/frames/eventtimingcodesframe.h
HEADERS += $$PWD/mpeg/id3v2/frames/generalencapsulatedobjectframe.h
HEADERS += $$PWD/mpeg/id3v2/frames/ownershipframe.h
HEADERS += $$PWD/mpeg/id3v2/frames/popularimeterframe.h
HEADERS += $$PWD/mpeg/id3v2/frames/privateframe.h
HEADERS += $$PWD/mpeg/id3v2/frames/relativevolumeframe.h
HEADERS += $$PWD/mpeg/id3v2/frames/synchronizedlyricsframe.h
HEADERS += $$PWD/mpeg/id3v2/frames/tableofcontentsframe.h
HEADERS += $$PWD/mpeg/id3v2/frames/textidentificationframe.h
HEADERS += $$PWD/mpeg/id3v2/frames/uniquefileidentifierframe.h
HEADERS += $$PWD/mpeg/id3v2/frames/unknownframe.h
HEADERS += $$PWD/mpeg/id3v2/frames/unsynchronizedlyricsframe.h
HEADERS += $$PWD/mpeg/id3v2/frames/urllinkframe.h
HEADERS += $$PWD/ogg/oggfile.h
HEADERS += $$PWD/ogg/oggpage.h
HEADERS += $$PWD/ogg/oggpageheader.h
HEADERS += $$PWD/ogg/xiphcomment.h
HEADERS += $$PWD/ogg/flac/oggflacfile.h
HEADERS += $$PWD/ogg/opus/opusfile.h
HEADERS += $$PWD/ogg/opus/opusproperties.h
HEADERS += $$PWD/ogg/speex/speexfile.h
HEADERS += $$PWD/ogg/speex/speexproperties.h
HEADERS += $$PWD/ogg/vorbis/vorbisfile.h
HEADERS += $$PWD/ogg/vorbis/vorbisproperties.h
HEADERS += $$PWD/riff/rifffile.h
HEADERS += $$PWD/riff/aiff/aifffile.h
HEADERS += $$PWD/riff/aiff/aiffproperties.h
HEADERS += $$PWD/riff/wav/infotag.h
HEADERS += $$PWD/riff/wav/wavfile.h
HEADERS += $$PWD/riff/wav/wavproperties.h
HEADERS += $$PWD/s3m/s3mfile.h
HEADERS += $$PWD/s3m/s3mproperties.h
HEADERS += $$PWD/toolkit/taglib.h
HEADERS += $$PWD/toolkit/tbytevector.h
HEADERS += $$PWD/toolkit/tbytevectorlist.h
HEADERS += $$PWD/toolkit/tbytevectorstream.h
HEADERS += $$PWD/toolkit/tdebug.h
HEADERS += $$PWD/toolkit/tdebuglistener.h
HEADERS += $$PWD/toolkit/tfile.h
HEADERS += $$PWD/toolkit/tfilestream.h
HEADERS += $$PWD/toolkit/tiostream.h
HEADERS += $$PWD/toolkit/tlist.h
HEADERS += $$PWD/toolkit/tmap.h
HEADERS += $$PWD/toolkit/tpropertymap.h
HEADERS += $$PWD/toolkit/tstring.h
HEADERS += $$PWD/toolkit/tstringlist.h
HEADERS += $$PWD/toolkit/tutils.h
#HEADERS += $$PWD/toolkit/unicode.h
HEADERS += $$PWD/trueaudio/trueaudiofile.h
HEADERS += $$PWD/trueaudio/trueaudioproperties.h
HEADERS += $$PWD/wavpack/wavpackfile.h
HEADERS += $$PWD/wavpack/wavpackproperties.h
HEADERS += $$PWD/xm/xmfile.h
HEADERS += $$PWD/xm/xmproperties.h

SOURCES += $$PWD/audioproperties.cpp \
    mpeg/id3v2/frames/podcastframe.cpp \
#    tagutils.cpp \
    toolkit/tzlib.cpp
SOURCES += $$PWD/fileref.cpp
SOURCES += $$PWD/tag.cpp
SOURCES += $$PWD/tagunion.cpp
SOURCES += $$PWD/tagutils.cpp
SOURCES += $$PWD/ape/apefile.cpp
SOURCES += $$PWD/ape/apefooter.cpp
SOURCES += $$PWD/ape/apeitem.cpp
SOURCES += $$PWD/ape/apeproperties.cpp
SOURCES += $$PWD/ape/apetag.cpp
SOURCES += $$PWD/asf/asfattribute.cpp
SOURCES += $$PWD/asf/asffile.cpp
SOURCES += $$PWD/asf/asfpicture.cpp
SOURCES += $$PWD/asf/asfproperties.cpp
SOURCES += $$PWD/asf/asftag.cpp
SOURCES += $$PWD/dsf/dsffile.cpp
SOURCES += $$PWD/dsf/dsfproperties.cpp
SOURCES += $$PWD/dsdiff/dsdiffdiintag.cpp
SOURCES += $$PWD/dsdiff/dsdifffile.cpp
SOURCES += $$PWD/dsdiff/dsdiffproperties.cpp
SOURCES += $$PWD/flac/flacfile.cpp
SOURCES += $$PWD/flac/flacmetadatablock.cpp
SOURCES += $$PWD/flac/flacpicture.cpp
SOURCES += $$PWD/flac/flacproperties.cpp
SOURCES += $$PWD/flac/flacunknownmetadatablock.cpp
SOURCES += $$PWD/it/itfile.cpp
SOURCES += $$PWD/it/itproperties.cpp
SOURCES += $$PWD/mod/modfile.cpp
SOURCES += $$PWD/mod/modfilebase.cpp
SOURCES += $$PWD/mod/modproperties.cpp
SOURCES += $$PWD/mod/modtag.cpp
SOURCES += $$PWD/mp4/mp4atom.cpp
SOURCES += $$PWD/mp4/mp4coverart.cpp
SOURCES += $$PWD/mp4/mp4file.cpp
SOURCES += $$PWD/mp4/mp4item.cpp
SOURCES += $$PWD/mp4/mp4itemfactory.cpp
SOURCES += $$PWD/mp4/mp4properties.cpp
SOURCES += $$PWD/mp4/mp4tag.cpp
SOURCES += $$PWD/mpc/mpcfile.cpp
SOURCES += $$PWD/mpc/mpcproperties.cpp
SOURCES += $$PWD/mpeg/mpegfile.cpp
SOURCES += $$PWD/mpeg/mpegheader.cpp
SOURCES += $$PWD/mpeg/mpegproperties.cpp
SOURCES += $$PWD/mpeg/xingheader.cpp
SOURCES += $$PWD/mpeg/id3v1/id3v1genres.cpp
SOURCES += $$PWD/mpeg/id3v1/id3v1tag.cpp
SOURCES += $$PWD/mpeg/id3v2/id3v2extendedheader.cpp
SOURCES += $$PWD/mpeg/id3v2/id3v2footer.cpp
SOURCES += $$PWD/mpeg/id3v2/id3v2frame.cpp
SOURCES += $$PWD/mpeg/id3v2/id3v2framefactory.cpp
SOURCES += $$PWD/mpeg/id3v2/id3v2header.cpp
SOURCES += $$PWD/mpeg/id3v2/id3v2synchdata.cpp
SOURCES += $$PWD/mpeg/id3v2/id3v2tag.cpp
SOURCES += $$PWD/mpeg/id3v2/frames/attachedpictureframe.cpp
SOURCES += $$PWD/mpeg/id3v2/frames/chapterframe.cpp
SOURCES += $$PWD/mpeg/id3v2/frames/commentsframe.cpp
SOURCES += $$PWD/mpeg/id3v2/frames/eventtimingcodesframe.cpp
SOURCES += $$PWD/mpeg/id3v2/frames/generalencapsulatedobjectframe.cpp
SOURCES += $$PWD/mpeg/id3v2/frames/ownershipframe.cpp
#SOURCES += $$PWD/mpeg/id3v2/frames/podcastframe.cpp
SOURCES += $$PWD/mpeg/id3v2/frames/popularimeterframe.cpp
SOURCES += $$PWD/mpeg/id3v2/frames/privateframe.cpp
SOURCES += $$PWD/mpeg/id3v2/frames/relativevolumeframe.cpp
SOURCES += $$PWD/mpeg/id3v2/frames/synchronizedlyricsframe.cpp
SOURCES += $$PWD/mpeg/id3v2/frames/tableofcontentsframe.cpp
SOURCES += $$PWD/mpeg/id3v2/frames/textidentificationframe.cpp
SOURCES += $$PWD/mpeg/id3v2/frames/uniquefileidentifierframe.cpp
SOURCES += $$PWD/mpeg/id3v2/frames/unknownframe.cpp
SOURCES += $$PWD/mpeg/id3v2/frames/unsynchronizedlyricsframe.cpp
SOURCES += $$PWD/mpeg/id3v2/frames/urllinkframe.cpp
SOURCES += $$PWD/ogg/oggfile.cpp
SOURCES += $$PWD/ogg/oggpage.cpp
SOURCES += $$PWD/ogg/oggpageheader.cpp
SOURCES += $$PWD/ogg/xiphcomment.cpp
SOURCES += $$PWD/ogg/flac/oggflacfile.cpp
SOURCES += $$PWD/ogg/opus/opusfile.cpp
SOURCES += $$PWD/ogg/opus/opusproperties.cpp
SOURCES += $$PWD/ogg/speex/speexfile.cpp
SOURCES += $$PWD/ogg/speex/speexproperties.cpp
SOURCES += $$PWD/ogg/vorbis/vorbisfile.cpp
SOURCES += $$PWD/ogg/vorbis/vorbisproperties.cpp
SOURCES += $$PWD/riff/rifffile.cpp
SOURCES += $$PWD/riff/aiff/aifffile.cpp
SOURCES += $$PWD/riff/aiff/aiffproperties.cpp
SOURCES += $$PWD/riff/wav/infotag.cpp
SOURCES += $$PWD/riff/wav/wavfile.cpp
SOURCES += $$PWD/riff/wav/wavproperties.cpp
SOURCES += $$PWD/s3m/s3mfile.cpp
SOURCES += $$PWD/s3m/s3mproperties.cpp
SOURCES += $$PWD/toolkit/tbytevector.cpp
SOURCES += $$PWD/toolkit/tbytevectorlist.cpp
SOURCES += $$PWD/toolkit/tbytevectorstream.cpp
SOURCES += $$PWD/toolkit/tdebug.cpp
SOURCES += $$PWD/toolkit/tdebuglistener.cpp
SOURCES += $$PWD/toolkit/tfile.cpp
SOURCES += $$PWD/toolkit/tfilestream.cpp
SOURCES += $$PWD/toolkit/tiostream.cpp
SOURCES += $$PWD/toolkit/tpicturetype.cpp
SOURCES += $$PWD/toolkit/tpropertymap.cpp
SOURCES += $$PWD/toolkit/tstring.cpp
SOURCES += $$PWD/toolkit/tstringlist.cpp
SOURCES += $$PWD/toolkit/tvariant.cpp
#SOURCES += $$PWD/toolkit/unicode.cpp
SOURCES += $$PWD/trueaudio/trueaudiofile.cpp
SOURCES += $$PWD/trueaudio/trueaudioproperties.cpp
SOURCES += $$PWD/wavpack/wavpackfile.cpp
SOURCES += $$PWD/wavpack/wavpackproperties.cpp
SOURCES += $$PWD/xm/xmfile.cpp
SOURCES += $$PWD/xm/xmproperties.cpp

#HEADERS += $$PWD/../config.h
HEADERS += $$PWD/../taglib_config.h


DEFINES += MAKE_TAGLIB_LIB
DEFINES += WITH_ASF
DEFINES += WITH_MP4

QT -= gui

CONFIG(debug, debug|release) {
    DEFINES += _DEBUG
#    DESTDIR = ../taglib-debug
    DESTDIR = ../
    OBJECTS_DIR = ./debug-o
} else {
#    DESTDIR = ../taglib-release
    DESTDIR = ../
    OBJECTS_DIR = ./release-o
}

win32 {
    CONFIG += dll
}
