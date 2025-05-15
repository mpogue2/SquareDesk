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
    VERSION="1.0.30"  # Default version if none provided
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
$QTDIR/${QTVERSION}/macos/bin/macdeployqt ${BUILDDIR}/test123/SquareDesk.app 2>&1 | grep -v "ERROR: Could not parse otool output line"
echo Mac Deploy Qt step done.
echo "---------------------------"
echo

# ----------------------------------------------------------------------------------------
echo Now making Install/SquareDesk_<version>.app ....

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

# Locate all the Qt Universal libs, and strip out the x86_64 part, since this is an M1-only executable right now
echo "----- Removing x86_64 stuff to make the M1-specific .app file much thinner...."
ls -al ${STAGING_DIR}/${APP_NAME}_${VERSION}.app/Contents/Frameworks
find ${STAGING_DIR}/${APP_NAME}_${VERSION}.app/Contents/Frameworks -type f -name "Qt*" -size +1k
find ${STAGING_DIR}/${APP_NAME}_${VERSION}.app/Contents/Frameworks -type f -name "Qt*" -size +1k -exec mv {} {}.backup \; -exec lipo -remove x86_64 {}.backup -output {} \; -exec ls -al {} \; -exec ls -al {}.backup \; -exec rm {}.backup \;
echo "----- DONE WITH LIPO ON QT FRAMEWORKS"
echo 

echo "---------------------------"
echo "Removing sd_calls.*cache files..."
rm -f ${STAGING_DIR}/${APP_NAME}_${VERSION}.app/Contents/MacOS/sd_calls.*cache

popd

echo "---------------------------"
echo 'DONE.'
echo "===================="

exit
