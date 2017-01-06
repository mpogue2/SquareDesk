***** THIS IS CURRENTLY TAGLIB-1.10 *****

***** NOTE -- config.h and taglib_config.h were copied over from a working taglib build
*****   after running cmake on it.  Also, binaries/include is copied over, too.

1) http://stellarium.org/wiki/index.php/Compilation_on_Mac_OS_X
2) set PATH
3) sudo brew install cmake
4) https://gist.github.com/trevorsheridan/1948299
   run that command to build the library
5) Mac OS X
https://github.com/taglib/taglib/blob/master/INSTALL
6) mutagen for Pythong
--------

On Mac OS X, you might want to build a framework that can be easily integrated
into your application. If you set the BUILD_FRAMEWORK option on, it will compile
TagLib as a framework. For example, the following command can be used to build
an Universal Binary framework with Mac OS X 10.4 as the deployment target:

  cmake -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_FRAMEWORK=ON \
    -DCMAKE_C_COMPILER=/usr/bin/gcc-4.0 \
    -DCMAKE_CXX_COMPILER=/usr/bin/c++-4.0 \
    -DCMAKE_OSX_SYSROOT=/Developer/SDKs/MacOSX10.4u.sdk/ \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.4 \
    -DCMAKE_OSX_ARCHITECTURES="ppc;i386;x86_64"

For a 10.6 Snow Leopard static library with both 32-bit and 64-bit code, use:

  cmake -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.6 \
    -DCMAKE_OSX_ARCHITECTURES="i386;x86_64" \
    -DENABLE_STATIC=ON \
    -DCMAKE_INSTALL_PREFIX="<folder you want to build to>"

After 'make', and 'make install', add libtag.a to your XCode project, and add
the include folder to the project's User Header Search Paths.

6)

0
down vote
accepted
Don't waste your time to use QMediaPlayer to read tags (this solution works good, but difficult for many files in list), simply use TagLib or another open-source library.

For example, 3 simple steps to use TagLib in Qt:

1.Compile taglib from source:

$ pwd
/home/user/taglib-1.9.1
$ cmake .
$ make
It's enough to deploy working instance of tag library. Really :)

2.Include headers and library in your project, simply add this or your custom path to project file:

unix:!macx: LIBS += -L$$PWD/3rdparty/taglib-1.9.1/taglib/ -ltag
INCLUDEPATH += $$PWD/3rdparty/taglib-1.9.1/taglib/Headers
DEPENDPATH += $$PWD/3rdparty/taglib-1.9.1/taglib/Headers
3.Use it, very simple function to get media tags from files, in this example artist and title of track:

    #include <fileref.h>
    #include <tag.h>
    QString gettags(QString mediafile){
        QString string;
        TagLib::FileRef file(mediafile.toUtf8());
        TagLib::String artist_string = file.tag()->artist();
        TagLib::String title_string = file.tag()->title();
        QString artist = QString::fromStdWString(artist_string.toWString());
        QString title = QString::fromStdWString(title_string.toWString());
        string = artist + " - " + title;
        return string;
    }
