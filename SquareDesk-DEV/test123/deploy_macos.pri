# =========================================================================================
# BUILDING THE SquareDesk.app BUNDLE
#
#   Included by test123.pro, which defines CODESIGN_ID.  macOS only.
# =========================================================================================

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
    $$PWD/third_party/cuesheet2.css \
    $$PWD/themes/Themes.qss \
    $$PWD/sd_calls.dat \
    $$PWD/allcalls.csv \
    $$PWD/abbrevs.txt \
    $$PWD/squareDanceLabelIDs.csv \
    $$PWD/../third_party/sdlib/sd_doc.pdf

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
bundle_pdfjs.files   = $$PWD/../third_party/qpdfjs/minified/build $$PWD/../third_party/qpdfjs/minified/web

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

macx {
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
    taminations.commands = unzip -o -q $$PWD/../third_party/Taminations/web.zip -d $$OUT_PWD/SquareDesk.app/Contents/Resources/Taminations

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
    #   a new app and re-prompts on every build.  Set CODESIGN_ID in test123.pro.
    #
    # After your first build: System Settings > Privacy & Security > Media & Apple Music,
    #   remove any old SquareDesk entry, run SquareDesk, and grant permission once.
    QMAKE_POST_LINK += codesign --force --sign \'$$CODESIGN_ID\' --entitlements $$PWD/SquareDesk.entitlements $$OUT_PWD/SquareDesk.app ;
}
