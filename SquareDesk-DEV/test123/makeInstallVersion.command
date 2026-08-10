#!/bin/bash

# This script takes the executable in build-*/test123 and copies
#   it to the Install directory, which is a staging folder.
#   It does liposuction of any X86 portions of code that it finds.
#
# The result is a version of SquareDesk that you should be able to double-click (on this system).
# This is the first step in the Release process.
# 
# After this is done, run 
#   signSquareDesk,
#   notarizeSquareDesk (requires the app-specific password!), and
#   makeDMG (which will also sign the DMG file).
#
# Then the resulting DMG file can be redistributed.

# make sure we are in the correct dir when we double-click a .command file
echo "===================="

dir=${0%/*}
if [ -d "$dir" ]; then
    cd "$dir"
fi

# check we are in a build directory
echo checking that we are in suitable directory, PWD = $PWD
echo $PWD | grep -E -q 'build.*Qt.*macOS-(Debug|Release)/test123$' || { echo invalid directory ; exit 1; }
APP_INFO_PLIST=$PWD/SquareDesk.app/Contents/Info.plist 
test -f $APP_INFO_PLIST || { echo missing $APP_INFO_PLIST ; exit 1; }

# we need to be in the directory above the test123 directory
cd ..

# echo $dir

case $PWD in
    *-Debug )
	WHICH=Debug;;
    *-Release )
	WHICH=Release;;
    * )
        echo >&2 "cannot determine WHICH (Debug or Release)"
	exit 1;;
esac
echo "WHICH =" $WHICH

# set up the app name and version number
APP_NAME="SquareDesk"
# Check if version was passed as a parameter, otherwise use default
if [ "$1" != "" ]; then
    VERSION="$1"
else
    # get default from Info.plist
    VERSION=$(/usr/libexec/PlistBuddy -c "Print :CFBundleShortVersionString" $APP_INFO_PLIST)
    echo "VERSION is $VERSION"
fi

QT_VERSION=$(echo $PWD | sed -e 's/.*Qt_//' -e 's/_for.*//')
QTVERSION=$(echo $QT_VERSION | sed -e 's/_/./g')

HOMEDIR=$HOME

SOURCEDIR=$(pwd | sed -e 's|/build.*||')/SquareDesk-DEV/test123 
BUILDDIR=$PWD
QTDIR="${HOMEDIR}/Qt"

echo "---------------------------"
echo HOMEDIR: ${HOMEDIR}
echo SOURCEDIR: ${SOURCEDIR}
echo QTDIR: ${QTDIR}
echo BUILDDIR: ${BUILDDIR}
echo DMG_BACKGROUND_IMG: ${DMG_BACKGROUND_IMG}
echo DYLD_FRAMEWORK_PATH: ${DYLD_FRAMEWORK_PATH}
echo "---------------------------"
echo

# ----------------------------------------------------------------------------------------
echo Now running Mac Deploy Qt step on the .app file in test123...
echo
echo "    NOTE: #1696 - the following 3 ERRORs from macdeployqt are NORMAL, please ignore them:"
echo "              ERROR: no file at \".../libiodbc.2.dylib\"       (needed only by libqsqlodbc)"
echo "              ERROR: no file at \".../libpq.5.dylib\"          (needed only by libqsqlpsql)"
echo "              ERROR: no file at \".../libmimerapi.dylib\"      (needed only by libqsqlmimer)"
echo "          SquareDesk only ever uses the QSQLITE driver, so those 3 SQL driver plugins are"
echo "          deleted from the staged .app a few steps below, and are NOT shipped."
echo "          The 2 'not Mac App store compliant' WARNINGs about those same plugins are also"
echo "          expected and harmless (we ship a notarized DMG, not via the App Store)."
echo
$QTDIR/${QTVERSION}/macos/bin/macdeployqt ${BUILDDIR}/test123/SquareDesk.app 2>&1 | grep -v "ERROR: Could not parse otool output line"
echo Mac Deploy Qt step done.
echo "---------------------------"
echo

# ----------------------------------------------------------------------------------------
echo "Now making Install/SquareDesk_<version>.app ...."

# you should not need to change these
APP_EXE="${APP_NAME}.app/Contents/MacOS/${APP_NAME}"
RENAMED_APP_EXE="${APP_NAME}_${VERSION}.app/Contents/MacOS/${APP_NAME}"

STAGING_DIR="${BUILDDIR}/Install"   # we copy all our stuff into this dir for staging

echo STAGING_DIR: ${STAGING_DIR}

# clear out any old data
rm -rf "${STAGING_DIR}" "${DMG_TMP}" "${DMG_FINAL}"

# copy over the stuff we want in the final disk image to our staging dir
echo
echo Making Staging Directory...
mkdir -p "${STAGING_DIR}"

echo "Copying .app to STAGING_DIR and renaming to ${APP_NAME}_${VERSION}.app..."
cp -Rpf "${BUILDDIR}/test123/${APP_NAME}.app" "${STAGING_DIR}/${APP_NAME}_${VERSION}.app"

# ----------------
# #1696: Remove the Qt SQL driver plugins we don't use. SquareDesk only ever opens QSQLITE
#   (see songsettings.cpp), and these three need libiodbc / libpq / libmimerapi, none of which are
#   installed here -- that's what the "ERROR: no file at ..." lines from macdeployqt above are about.
#   They can never load at runtime, so there's no reason to ship, sign, or notarize them.
#   NOTE: this has to happen AFTER macdeployqt, which re-copies them on every run. Also note that
#   macdeployqt's -appstore-compliant flag does NOT do this job: it does not remove plugins that are
#   already deployed, it never covers qsqlmimer, and it adds two more WARNING lines to the output.
echo "***** REMOVING UNUSED Qt SQL DRIVER PLUGINS (we only use QSQLITE)..."
for p in libqsqlodbc.dylib libqsqlpsql.dylib libqsqlmimer.dylib; do
    rm -f "${STAGING_DIR}/${APP_NAME}_${VERSION}.app/Contents/PlugIns/sqldrivers/$p"
done
ls -l "${STAGING_DIR}/${APP_NAME}_${VERSION}.app/Contents/PlugIns/sqldrivers/"
echo

# ----------------
# remove any SD cache files that were left over from testing...
echo "***** REMOVING SD CACHE FILES from: "
echo "     ${STAGING_DIR}/${APP_NAME}_${VERSION}.app/Contents/MacOS/sd_calls.*cache"
rm -f "${STAGING_DIR}/${APP_NAME}_${VERSION}.app/Contents/MacOS/sd_calls.*cache"
echo

# copy in the SquareDesk Manual
#cp -rpf "${MANUAL}" "${STAGING_DIR}"

# ... cp anything else you want in the DMG - documentation, etc.

echo "***** pushing to ${STAGING_DIR}..."
pushd "${STAGING_DIR}"
echo 

# strip the executable
echo "---------------------------"
echo
echo "Stripping ${RENAMED_APP_EXE}..."
strip -u -r "${RENAMED_APP_EXE}"
echo

# #1696: Strip the x86_64 slice out of EVERY fat binary in the bundle, since this is an arm64-only app.
#   The old version of this only looked at Contents/Frameworks/Qt*, which missed the Qt plugins in
#   Contents/PlugIns, the FFmpeg dylibs in Contents/Frameworks, and the vamp dylibs in Contents/MacOS.
#   This runs before any code signing happens (that's fixAndSignSquareDesk.command, the next step in
#   releaseSquareDesk.command), so everything thinned here gets re-signed afterwards.
echo "----- Removing x86_64 slices to make the arm64-only .app file much thinner...."
LIPO_APP="${STAGING_DIR}/${APP_NAME}_${VERSION}.app"
LIPO_COUNT=0
LIPO_FAILED=0
while IFS= read -r f; do
    # only touch files that actually have an x86_64 slice, so already-thin files aren't disturbed
    lipo -archs "$f" 2>/dev/null | grep -qw x86_64 || continue
    if lipo -remove x86_64 "$f" -output "$f.thin" 2>/dev/null; then
        mv "$f.thin" "$f"
        LIPO_COUNT=$((LIPO_COUNT + 1))
        echo "  thinned: ${f#$LIPO_APP/}"
    else
        rm -f "$f.thin"
        LIPO_FAILED=$((LIPO_FAILED + 1))
        echo "  WARNING: could not thin ${f#$LIPO_APP/}"
    fi
done < <(find "$LIPO_APP/Contents" -type f -size +1k)
echo "----- DONE WITH LIPO: ${LIPO_COUNT} thinned, ${LIPO_FAILED} failed"
if [ "$LIPO_FAILED" -ne 0 ]; then
    echo "Error: ${LIPO_FAILED} file(s) failed to thin. Aborting."
    exit 1
fi
echo

# #1696: Independently audit the whole bundle to prove no x86_64 slice survived anywhere.
#   Deliberately does NOT reuse the loop above's filters (no -size limit) so that this is a real
#   double-check rather than a restatement of what that loop thought it did.
echo "----- Verifying that no x86_64 slices remain...."
VERIFY_MACHO=0
VERIFY_FAT=0
while IFS= read -r f; do
    ARCHS=$(lipo -archs "$f" 2>/dev/null) || continue   # not a Mach-O file, skip it
    VERIFY_MACHO=$((VERIFY_MACHO + 1))
    if echo "$ARCHS" | grep -qw x86_64; then
        VERIFY_FAT=$((VERIFY_FAT + 1))
        echo "  STILL HAS x86_64 [${ARCHS}]: ${f#$LIPO_APP/}"
    fi
done < <(find "$LIPO_APP" -type f)
echo
if [ "$VERIFY_FAT" -ne 0 ]; then
    echo "***************************************************************************"
    echo "*** VERIFICATION FAILED: ${VERIFY_FAT} OF ${VERIFY_MACHO} BINARIES STILL CONTAIN X86_64 CODE! ***"
    echo "***************************************************************************"
    exit 1
fi
echo "***************************************************************************"
echo "*** VERIFIED: ALL ${VERIFY_MACHO} BINARIES ARE ARM64-ONLY, ZERO X86_64 SLICES REMAIN ***"
echo "***************************************************************************"
echo

echo "---------------------------"
echo "Removing sd_calls.*cache files..."
rm -f ${STAGING_DIR}/${APP_NAME}_${VERSION}.app/Contents/MacOS/sd_calls.*cache

popd

echo "---------------------------"
echo 'DONE.'
echo "===================="

exit
