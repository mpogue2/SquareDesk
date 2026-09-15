#-------------------------------------------------
#
# Project created by QtCreator 2016-07-07T21:58:24
#
#-------------------------------------------------

QT       += core gui sql network printsupport svg svgwidgets
unix {
    QT += webenginewidgets
    PRE_TARGETDEPS += $$OUT_PWD/../sdlib/libsdlib.a
}
macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 13.0

    # for BULK operations
    QT += concurrent

    # for Taminations HTTP server
    QT += httpserver
}

# =========================================================================================
# BUILD CONFIGURATION
#
#   Every value here can be overridden on the qmake command line, or in QtCreator under
#   Projects > Build & Run > Build > qmake step > "Additional arguments", e.g.
#       qmake ... CODESIGN_ID=- JUCE_ROOT=$HOME/JUCE/libJUCEstatic
#   so that nobody has to edit this file just to build on their own machine.
#
macx {
    # Apple SDK to build against.  See the NOTE in the next block before changing it.
    isEmpty(MAC_SDK):      MAC_SDK = macosx27.0

    # Where ../juce-install puts the static JUCE library, and where the JUCE modules live.
    isEmpty(JUCE_ROOT):    JUCE_ROOT = $$(HOME)/JUCEProjects/libJUCEstatic
    isEmpty(JUCE_MODULES): JUCE_MODULES = /Applications/JUCE

    # Which libJUCEstatic build to link against.
    #   NOTE: this is the Debug build even for Release builds of SquareDesk, which is what
    #   this project has always done.  See issue #1729 before changing it.
    isEmpty(JUCE_BUILD):   JUCE_BUILD = Debug
    isEmpty(JUCE_LIB):     JUCE_LIB = JUCE_debug

    # Code signing identity for the post-link re-sign (see the codesign section, far below).
    #   To find yours:  security find-identity -v -p codesigning | grep "Apple Development"
    #   Use CODESIGN_ID=- for ad-hoc signing if you have no Apple Developer certificate.
    isEmpty(CODESIGN_ID):  CODESIGN_ID = "Apple Development: Michael Pogue (6K9PD3928V)"
}

macx {
  # VARIABLE REFERENCE: https://doc.qt.io/qt-6/qmake-variable-reference.html
  #
  # NOTE: QMAKE_MAC_SDK MUST BE ALL LOWER CASE, and fully spelled out ("macosx27.0", not
  #   "macosx27").  Otherwise the command that <QtDir>/macos/mkspecs/features/macos/sdk.prf
  #   issues, "/usr/bin/xcrun --sdk macosx27.0 --show-sdk-version", fails with:
  #   "Could not resolve SDK SDKVersion for 'MacOSX27.0' using --show-sdk-version"
  #
  # When Apple ships a new SDK, make this match the latest version in:
  #   ls /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs
  #
  # NOTE: when this changes, you must delete the stale .qmake.stash in the BUILD directory by
  #   hand.  It lives one level above test123, so it is not regenerated.  See QTBUG-43015.
  QMAKE_MAC_SDK = $$MAC_SDK
}

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SquareDesk
TEMPLATE = app

#  turn off QML warning for Debug builds
DEFINES += QT_QML_DEBUG_NO_WARNING


SOURCES += main.cpp\
    addcommentdialog.cpp \
    audiodecoder.cpp \
    auditionbutton.cpp \
    flexible_audio.cpp \
    embeddedserver.cpp \
    lyricsEditor.cpp \
    lyricseditor_autoformat.cpp \
    mainwindow.cpp \
    mainwindow_audio.cpp \
    mainwindow_init.cpp \
    mainwindow_nowplaying.cpp \
    mainwindow_JUCE.cpp \
    mainwindow_bulk.cpp \
    mainwindow_choreo1.cpp \
    mainwindow_cuesheets.cpp \
    mainwindow_filemgmt.cpp \
    mainwindow_flashcalls.cpp \
    mainwindow_fonts.cpp \
    mainwindow_metadata.cpp \
    mainwindow_music.cpp \
    mainwindow_taminations.cpp \
    mainwindow_themes.cpp \
    miniBPM/MiniBpm.cpp \
    mytextedit.cpp \
    newdancedialog.cpp \
    playlists.cpp \
    preferencesdialog.cpp \
    choreosequencedialog.cpp \
    cuesheetmatchingdebugdialog.cpp \
    importdialog.cpp \
    exportdialog.cpp \
    songhistoryexportdialog.cpp \
    mytablewidget.cpp \
    mytreewidget.cpp \
    soundtouch/source/SoundTouch/AAFilter.cpp \
    soundtouch/source/SoundTouch/BPMDetect.cpp \
    soundtouch/source/SoundTouch/FIFOSampleBuffer.cpp \
    soundtouch/source/SoundTouch/FIRFilter.cpp \
    soundtouch/source/SoundTouch/InterpolateCubic.cpp \
    soundtouch/source/SoundTouch/InterpolateLinear.cpp \
    soundtouch/source/SoundTouch/InterpolateShannon.cpp \
    soundtouch/source/SoundTouch/PeakFinder.cpp \
    soundtouch/source/SoundTouch/RateTransposer.cpp \
    soundtouch/source/SoundTouch/SoundTouch.cpp \
    soundtouch/source/SoundTouch/TDStretch.cpp \
    soundtouch/source/SoundTouch/cpu_detect_x86.cpp \
    soundtouch/source/SoundTouch/mmx_optimized.cpp \
    soundtouch/source/SoundTouch/sse_optimized.cpp \
    splashscreen.cpp \
    svgClock.cpp \
    svgDial.cpp \
    svgSlider.cpp \
    svgVUmeter.cpp \
    svgWaveformSlider.cpp \
    tablenumberitem.cpp \
    playlistnumberdelegate.cpp \
    myslider.cpp \
    levelmeter.cpp \
    prefsmanager.cpp \
    clickablelabel.cpp \
    songsettings.cpp \
    typetracker.cpp \
    console.cpp \
    squaredancerscene.cpp \
    sdhighlighter.cpp \
    updateid3tagsdialog.cpp \
    updateid3tagsmanager.cpp \
    utility.cpp \
    danceprograms.cpp \
    startupwizard.cpp \
    keybindings.cpp \
    calllistcheckbox.cpp \
    sdlineedit.cpp \
    downloadmanager.cpp \
    sdinterface.cpp \
    sdformationutils.cpp \
    mainwindow_sd.cpp \
    songtitlelabel.cpp \
    sdsequencecalllabel.cpp \
    perftimer.cpp \
    tablewidgettimingitem.cpp \
    sdredostack.cpp \
    makeflashdrivewizard.cpp \
    songlistmodel.cpp \
    taminationsinterface.cpp \
    mydatetimeedit.cpp \
    tablelabelitem.cpp

unix {
SOURCES += ../qpdfjs/src/communicator.cpp

# this hint from: https://forum.qt.io/topic/74432/os-x-deployment-problem-rpath-framework/5
QMAKE_LFLAGS += -Wl,-rpath,@loader_path/../,-rpath,@executable_path/../ #,-rpath,@executable_path/../Frameworks
}

HEADERS  += mainwindow.h \
    addcommentdialog.h \
    audiodecoder.h \
    auditionbutton.h \
    embeddedserver.h \
    flexible_audio.h \
    globaldefines.h \
    mytextedit.h \
    palettetablebulkupdate.h \
    playlist_constants.h \
    miniBPM/MiniBpm.h \
    minimp3.h \
    minimp3_ex.h \
    myslider.h \
    importdialog.h \
    exportdialog.h \
    newdancedialog.h \
    sessioninfo.h \
    songhistoryexportdialog.h \
    preferencesdialog.h \
    choreosequencedialog.h \
    cuesheetmatchingdebugdialog.h \
    soundtouch/include/BPMDetect.h \
    soundtouch/include/FIFOSampleBuffer.h \
    soundtouch/include/FIFOSamplePipe.h \
    soundtouch/include/STTypes.h \
    soundtouch/include/SoundTouch.h \
    soundtouch/include/soundtouch_config.h \
    soundtouch/source/SoundTouch/AAFilter.h \
    soundtouch/source/SoundTouch/FIRFilter.h \
    soundtouch/source/SoundTouch/InterpolateCubic.h \
    soundtouch/source/SoundTouch/InterpolateLinear.h \
    soundtouch/source/SoundTouch/InterpolateShannon.h \
    soundtouch/source/SoundTouch/PeakFinder.h \
    soundtouch/source/SoundTouch/RateTransposer.h \
    soundtouch/source/SoundTouch/TDStretch.h \
    soundtouch/source/SoundTouch/cpu_detect.h \
    splashscreen.h \
    svgClock.h \
    svgDial.h \
    svgSlider.h \
    svgVUmeter.h \
    svgWaveformSlider.h \
    updateid3tagsdialog.h \
    updateid3tagsmanager.h \
    utility.h \
    mytablewidget.h \
    mytreewidget.h \
    songdraginfo.h \
    tablenumberitem.h \
    playlistnumberdelegate.h \
    levelmeter.h \
    common_enums.h \
    prefs_options.h \
    prefsmanager.h \
    default_colors.h \
    macUtils.h \
    mainwindow_applemusic.h \
    clickablelabel.h \
    typetracker.h \
    console.h \
    squaredancerscene.h \
    common.h \
    sdhighlighter.h \
    danceprograms.h \
    startupwizard.h \
    songsettings.h \
    platform.h \
    keybindings.h \
    calllistcheckbox.h \
    sdlineedit.h \
    downloadmanager.h \
    sdinterface.h \
    sdformationutils.h \
    songtitlelabel.h \
    sdsequencecalllabel.h \
    perftimer.h \
    tablewidgettimingitem.h \
    sdredostack.h \
    makeflashdrivewizard.h \
    songlistmodel.h \
    taminationsinterface.h \
    mydatetimeedit.h \
    keyactions.h \
    songsetting_attributes.h \
    tablelabelitem.h \
    wav_file.h

unix {
HEADERS += ../qpdfjs/src/communicator.h
INCLUDEPATH += $$PWD/../qpdfjs
}

FORMS    += mainwindow.ui \
    addcommentdialog.ui \
    importdialog.ui \
    exportdialog.ui \
    newdancedialog.ui \
    songhistoryexportdialog.ui \
    choreosequencedialog.ui \
    preferencesdialog.ui \
    updateID3TagsDialog.ui

macx {
  # to ensure that Info.plist gets copied over (build will not overwrite one that's already there)
  QMAKE_INFO_PLIST = $$PWD/Info.plist
  plist.target = Info.plist
  plist.depends = $$PWD/Info.plist # "$$OUT_PWD/SquareDesk.app/Contents/Info.plist"
  plist.commands = $(DEL_FILE) \"$$OUT_PWD/SquareDesk.app/Contents/Info.plist\" $$escape_expand(\n\t) \
                         $(COPY_FILE) $$PWD/Info.plist \"$$OUT_PWD/SquareDesk.app/Contents/Info.plist\"
  QMAKE_EXTRA_TARGETS += plist
  PRE_TARGETDEPS += $$plist.target
}

macx {
# This is just for libtidy at this point... (NOTE: libtidy no longer needed)
INCLUDEPATH += $$PWD/ $$PWD/../local_macosx/include
DEPENDPATH += $$PWD/ $$PWD/../local_macosx/include

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
# macx: see below...

# USE THIS ONE FOR ALL MACS **********************************************
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

# KFR for filters -----------------------------------

macx {
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
}

macx {
    QMAKE_EXTRA_TARGETS += libJUCE JUCE
    libJUCE.target = $$JUCE_ROOT
    libJUCE.depends =
    libJUCE.commands = zsh $$PWD/../juce-install
    JUCE.target = $$JUCE_MODULES
    JUCE.depends =
    JUCE.commands = zsh $$PWD/../juce-install
    PRE_TARGETDEPS += $$libJUCE.target
}


# MiniBPM for BPM detection -----------------------------------
INCLUDEPATH += $$PWD/miniBPM

# SoundTouch for pitch/tempo changing -----------------------------------
INCLUDEPATH += $$PWD/soundtouch/include

# SDLIB ------------------------------------------
LIBS += -L$$OUT_PWD/../sdlib -lsdlib

# ICONS, ALLCALLS.CSV ---------------------------
ICON = $$PWD/desk1d.icns
DISTFILES += desk1d.icns
DISTFILES += $$PWD/allcalls.csv  # RESOURCE: list of calls, and which level they are

# FILES COPIED INTO THE APP BUNDLE --------------------------------------------------------
#
#   QMAKE_BUNDLE_DATA emits one real Makefile rule per file, with the source file as a
#   prerequisite, so each file is re-copied only when it actually changes and the
#   destination directory is created automatically.  To deploy a new file, add it to the
#   right .files list below -- that is the only edit needed.
#
#   NOTE: sd_calls.dat and sd_doc.pdf must land in Resources, NOT MacOS, so that SDP can
#   find them and start up sd.

bundle_resources.path  = Contents/Resources
bundle_resources.files = \
    $$PWD/lyrics.template.html \
    $$PWD/lyrics.template.2col.html \
    $$PWD/cuesheet2.css \
    $$PWD/themes/Themes.qss \
    $$PWD/sd_calls.dat \
    $$PWD/allcalls.csv \
    $$PWD/abbrevs.txt \
    $$PWD/squareDanceLabelIDs.csv \
    $$PWD/../sdlib/sd_doc.pdf

# SVG resources for the knobs and sliders
bundle_knobs.path    = Contents/Resources/knobs
bundle_knobs.files   = $$files($$PWD/graphics/knobs/*.svg)

bundle_sliders.path  = Contents/Resources/sliders
bundle_sliders.files = $$files($$PWD/graphics/sliders/*.svg)

# Sound FX starter set
bundle_soundfx.path  = Contents/soundfx
bundle_soundfx.files = $$files($$PWD/soundfx/*.mp3)

# PDF viewer (qpdfjs): its "build" and "web" trees
bundle_pdfjs.path    = Contents/Resources/minified
bundle_pdfjs.files   = $$PWD/../qpdfjs/minified/build $$PWD/../qpdfjs/minified/web

# VAMP, for beat/measure detection and segmentation.
#   NOTE: the dylibs and the vamp-simple-host executable are ARM64 binaries; segmentino and
#   the QM plugins are universal binaries.  Listed by pattern rather than as one glob so a
#   stray file in that directory cannot silently end up inside a signed bundle.
VAMP_DIR = $$PWD/../local_macosx/vamp/cleanVAMPfiles/vamp-standalone
bundle_vamp.path     = Contents/MacOS
bundle_vamp.files    = $$files($$VAMP_DIR/*.dylib) $$VAMP_DIR/vamp-simple-host

QMAKE_BUNDLE_DATA += bundle_resources bundle_knobs bundle_sliders bundle_soundfx \
                     bundle_pdfjs bundle_vamp

# ----------------------------------------------------------------------------------------
# For the Mac OS X DMG installer build, we need these files stuck into the results directory
installer1.commands = $(COPY) $$PWD/images/Installer3.png      $$OUT_PWD/Installer3.png             # DMG BACKGROUND IMAGE

installer2.commands = $(COPY) $$PWD/makeInstallVersion.command    $$OUT_PWD/makeInstallVersion.command    # NEW RELEASE METHOD
installer3.commands = $(COPY) $$PWD/fixAndSignSquareDesk.command  $$OUT_PWD/fixAndSignSquareDesk.command  # NEW RELEASE METHOD
installer4.commands = $(COPY) $$PWD/notarizeSquareDesk.command    $$OUT_PWD/notarizeSquareDesk.command    # NEW RELEASE METHOD
installer5.commands = $(COPY) $$PWD/makeDMG.command               $$OUT_PWD/makeDMG.command               # NEW RELEASE METHOD
installer6.commands = $(COPY) $$PWD/releaseSquareDesk.command     $$OUT_PWD/releaseSquareDesk.command     # NEW RELEASE METHOD

first.depends += installer1 installer2 installer3 installer4 installer5 installer6

QMAKE_EXTRA_TARGETS += first installer1 installer2 installer3 installer4 installer5 installer6
}

# ************************************************************************************
# USE THIS ONE FOR STUFF THAT IS FOR M1 MACS ONLY *************
macx {
    # M1MAC: comment this section out on X86 Mac builds
    DEFINES += M1MAC=1
    # TEMPORARY: turn off complaining when result of file.open() is not looked at
    DEFINES += QT_NO_USE_NODISCARD_FILE_OPEN
    QT += multimedia

    # POST-PROCESSING OF THE FINISHED BUNDLE ----------------------------------------------
    #
    #   The deployment steps that are not plain file copies, and so cannot be expressed as
    #   QMAKE_BUNDLE_DATA.  These used to be ordered by "sleep 1" / "sleep 2" / "sleep 3" /
    #   "sleep 5" inside the recipes, which guarantees nothing under a parallel build
    #   (make -j) and cost ~16 seconds on every build.  They are now chained with real
    #   dependencies, and the head of the chain depends on "all", so the whole chain runs
    #   after every bundle file has been copied and the app has been linked.

    # pdf.js ships a large sample PDF that we do not need.
    stripPDFJS.target   = stripPDFJS
    stripPDFJS.depends  = all
    stripPDFJS.commands = $(RM) $$OUT_PWD/SquareDesk.app/Contents/Resources/minified/web/compressed.*.pdf

    # Taminations: unzip web.zip into Resources/Taminations ("unzip -d" creates the folder).
    taminations.target   = taminations
    taminations.depends  = stripPDFJS
    taminations.commands = unzip -o -q $$PWD/../Taminations/web.zip -d $$OUT_PWD/SquareDesk.app/Contents/Resources/Taminations

    # Strip AppleDouble files and build cruft, which otherwise break signing and notarizing.
    bundleCleanup.target   = bundleCleanup
    bundleCleanup.depends  = taminations
    bundleCleanup.commands = dot_clean $$OUT_PWD/SquareDesk.app $$escape_expand(\n\t) \
                             find $$OUT_PWD/SquareDesk.app/Contents -name \".last_build_id\" -type f -delete $$escape_expand(\n\t) \
                             find $$OUT_PWD/SquareDesk.app/Contents -name \".DS_Store\" -type f -delete

    QMAKE_EXTRA_TARGETS += stripPDFJS taminations bundleCleanup
    first.depends += bundleCleanup

    # Re-sign the app bundle with the Apple Music entitlement after each build --------
    # Required so that macOS grants, and then remembers, Media & Apple Music permission.
    # Signing with a named Developer certificate keeps the TCC grant stable across rebuilds;
    #   ad-hoc signing (CODESIGN_ID=-) also works, but macOS treats every ad-hoc signature as
    #   a new app and re-prompts on every build.  Set CODESIGN_ID at the top of this file.
    #
    # After your first build: System Settings > Privacy & Security > Media & Apple Music,
    #   remove any old SquareDesk entry, run SquareDesk, and grant permission once.
    QMAKE_POST_LINK += codesign --force --sign \'$$CODESIGN_ID\' --entitlements $$PWD/SquareDesk.entitlements $$OUT_PWD/SquareDesk.app ;
}

RESOURCES += resources.qrc
RESOURCES += startupwizard.qrc

OBJECTIVE_SOURCES += \
    macUtils.mm \
    mainwindow_applemusic.mm

DISTFILES += \
    SquareDesk.entitlements \
    ../../README.md \
    ../juce-install \
    CurrentKeyAssignments.txt \
    Info.plist \
    LICENSE.GPL3 \
    LICENSE.GPL2 \
    abbrevs.txt \
    cuesheet2.css \
    fixAndSignSquareDesk.command \
    lyrics.template.2col.html \
    lyrics.template.html \
    makeDMG.command \
    makeInstallVersion.command \
    notarizeSquareDesk.command \
    patter.template.html \
    releaseSquareDesk.command \
    soundtouch/include/soundtouch_config.h.in \
    squareDanceLabelIDs.csv \
    themes/Themes.qss

CONFIG += c++17

# =========================================================================================
# ADDRESS SANITIZER BUILD (Issue #1686)
#
#   Off by default. Nothing below affects a normal Debug or Release build -- it is all
#   inside the asan{} block, which is only entered when qmake is given CONFIG+=asan.
#
#   In QtCreator:
#     Projects -> Build & Run -> (your Qt kit) -> Build -> clone the Debug configuration,
#     name the clone "ASan", give it its own build directory (e.g.
#     build-SquareDesk-Qt_6_10_3_for_macOS-ASan), and add
#         CONFIG+=asan
#     to "Additional arguments" on the qmake build step. Build it, then just Run.
#
#   From the command line:
#     mkdir build-asan && cd build-asan
#     <QtDir>/macos/bin/qmake ../SquareDesk-DEV/SquareDesk.pro CONFIG+=asan CONFIG+=debug
#     make -j8
#
#   ASan finds the bug at the moment it happens (the second free), and prints BOTH the
#   stack that freed it the first time and the stack that allocated it -- which is exactly
#   what the quit-time crash reports cannot tell us. Run it, use the app normally, then
#   quit: if the double-free is real, ASan reports it instead of crashing in _free.
#
#   Recommended environment when running (full instructions are in ../ASAN.md):
#     ASAN_OPTIONS=abort_on_error=1:malloc_context_size=50:detect_leaks=0
#
asan {
    message("*** ADDRESS SANITIZER BUILD ENABLED (CONFIG+=asan) ***")

    # -O1 and -fno-omit-frame-pointer keep the stack traces readable; -g gives us line
    #   numbers. These come after qmake's own flags, so they win.
    ASAN_FLAGS = -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls -O1 -g

    QMAKE_CFLAGS   += $$ASAN_FLAGS
    QMAKE_CXXFLAGS += $$ASAN_FLAGS
    QMAKE_LFLAGS   += -fsanitize=address

    # so code can tell (e.g. to skip a known-noisy path) that it is an ASan build
    DEFINES += SQUAREDESK_ASAN
}
