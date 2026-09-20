# =========================================================================================
# WHAT GETS COMPILED
#
#   Included by test123.pro.  $$PWD here is the test123 directory, the same as in the .pro.
# =========================================================================================

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
    third_party/miniBPM/MiniBpm.cpp \
    mytextedit.cpp \
    newdancedialog.cpp \
    playlists.cpp \
    applemusicfilter.cpp \
    preferencesdialog.cpp \
    choreosequencedialog.cpp \
    cuesheetmatchingdebugdialog.cpp \
    importdialog.cpp \
    exportdialog.cpp \
    songhistoryexportdialog.cpp \
    mytablewidget.cpp \
    mytreewidget.cpp \
    third_party/soundtouch/source/SoundTouch/AAFilter.cpp \
    third_party/soundtouch/source/SoundTouch/BPMDetect.cpp \
    third_party/soundtouch/source/SoundTouch/FIFOSampleBuffer.cpp \
    third_party/soundtouch/source/SoundTouch/FIRFilter.cpp \
    third_party/soundtouch/source/SoundTouch/InterpolateCubic.cpp \
    third_party/soundtouch/source/SoundTouch/InterpolateLinear.cpp \
    third_party/soundtouch/source/SoundTouch/InterpolateShannon.cpp \
    third_party/soundtouch/source/SoundTouch/PeakFinder.cpp \
    third_party/soundtouch/source/SoundTouch/RateTransposer.cpp \
    third_party/soundtouch/source/SoundTouch/SoundTouch.cpp \
    third_party/soundtouch/source/SoundTouch/TDStretch.cpp \
    third_party/soundtouch/source/SoundTouch/cpu_detect_x86.cpp \
    third_party/soundtouch/source/SoundTouch/mmx_optimized.cpp \
    third_party/soundtouch/source/SoundTouch/sse_optimized.cpp \
    splashscreen.cpp \
    svgClock.cpp \
    svgDial.cpp \
    svgSlider.cpp \
    svgVUmeter.cpp \
    svgWaveformSlider.cpp \
    tablenumberitem.cpp \
    playlistnumberdelegate.cpp \
    myslider.cpp \
    third_party/levelmeter.cpp \
    prefsmanager.cpp \
    clickablelabel.cpp \
    songsettings.cpp \
    typetracker.cpp \
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
    third_party/miniBPM/MiniBpm.h \
    third_party/minimp3.h \
    third_party/minimp3_ex.h \
    myslider.h \
    importdialog.h \
    exportdialog.h \
    newdancedialog.h \
    sessioninfo.h \
    songhistoryexportdialog.h \
    applemusicfilter.h \
    preferencesdialog.h \
    choreosequencedialog.h \
    cuesheetmatchingdebugdialog.h \
    third_party/soundtouch/include/BPMDetect.h \
    third_party/soundtouch/include/FIFOSampleBuffer.h \
    third_party/soundtouch/include/FIFOSamplePipe.h \
    third_party/soundtouch/include/STTypes.h \
    third_party/soundtouch/include/SoundTouch.h \
    third_party/soundtouch/include/soundtouch_config.h \
    third_party/soundtouch/source/SoundTouch/AAFilter.h \
    third_party/soundtouch/source/SoundTouch/FIRFilter.h \
    third_party/soundtouch/source/SoundTouch/InterpolateCubic.h \
    third_party/soundtouch/source/SoundTouch/InterpolateLinear.h \
    third_party/soundtouch/source/SoundTouch/InterpolateShannon.h \
    third_party/soundtouch/source/SoundTouch/PeakFinder.h \
    third_party/soundtouch/source/SoundTouch/RateTransposer.h \
    third_party/soundtouch/source/SoundTouch/TDStretch.h \
    third_party/soundtouch/source/SoundTouch/cpu_detect.h \
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
    third_party/levelmeter.h \
    common_enums.h \
    prefs_options.h \
    prefsmanager.h \
    default_colors.h \
    macUtils.h \
    mainwindow_applemusic.h \
    clickablelabel.h \
    typetracker.h \
    squaredancerscene.h \
    common.h \
    sdhighlighter.h \
    danceprograms.h \
    startupwizard.h \
    songsettings.h \
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
    wav_file.h \
    dragicon.h \
    globaleventfilter.h \
    playlistexport.h \
    sddancer.h \
    selectionretainer.h \
    xxhash64.h

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
    third_party/cuesheet2.css \
    fixAndSignSquareDesk.command \
    lyrics.template.2col.html \
    lyrics.template.html \
    makeDMG.command \
    makeInstallVersion.command \
    notarizeSquareDesk.command \
    patter.template.html \
    releaseSquareDesk.command \
    third_party/soundtouch/include/soundtouch_config.h.in \
    squareDanceLabelIDs.csv \
    themes/Themes.qss
