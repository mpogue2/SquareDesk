#!/bin/bash

# THIS FILE IS FOR X86 ONLY -----------------

# by Andy Maloney
# http://asmaloney.com/2013/07/howto/packaging-a-mac-os-x-application-using-a-dmg/

# make sure we are in the correct dir when we double-click a .command file
# dir=${0%/*}
# if [ -d "$dir" ]; then
#   cd "$dir"
# fi

# echo $dir

# the macdeployqt step!
# cd build-test123-Desktop_Qt_5_7_0_clang_64bit-Debug/

# No longer needed:
## install_name_tool -change libquazip.1.dylib @executable_path/libquazip.1.dylib SquareDeskPlayer
## install_name_tool -change libtidy.5.dylib @executable_path/libtidy.5.dylib SquareDeskPlayer
#
# These errors can be ignored, they are false dependencies:
# ERROR: no file at "/opt/local/lib/mysql55/mysql/libmysqlclient.18.dylib"
# ERROR: no file at "/usr/local/lib/libpq.5.dylib"

#WHICH=Debug-X86
WHICH=Release-X86

# set up your app name, version number, and background image file name
APP_NAME="SquareDesk"
VERSION="1.0.4X"  # <-- THIS IS THE ONE TO CHANGE (X = X86)

QTVERSION="6.3.1"
QT_VERSION="6_3_1"   # same thing, but with underscores (yes, change both of them at the same time!)

HOMEDIR="/Users/mpogue"
SOURCEDIR="${HOMEDIR}/clean3/SquareDesk/SquareDesk-DEV/test123"
MIKEBUILDDIR="${HOMEDIR}/clean3/SquareDesk/build-SquareDesk-Qt_${QT_VERSION}_for_macOS-${WHICH}"
QTDIR="${HOMEDIR}/Qt"

DMG_BACKGROUND_IMG_SHORT="installer3.png"
DMG_BACKGROUND_IMG="${SOURCEDIR}/images/${DMG_BACKGROUND_IMG_SHORT}"

echo
echo HOMEDIR: ${HOMEDIR}
echo SOURCEDIR: ${SOURCEDIR}
echo QTDIR: ${QTDIR}
echo MIKEBUILDDIR: ${MIKEBUILDDIR}
echo DMG_BACKGROUND_IMG: ${DMG_BACKGROUND_IMG}
echo DYLD_FRAMEWORK_PATH: ${DYLD_FRAMEWORK_PATH}
echo

# ------------------------------------------------------------------------
echo WARNING: libquazip not present in X86 build yet

echo Now running otool to fixup libraries...
# # Note: The 64bit5 may be specific to my machine, since I have a bunch of Qt installations...
pushd ${MIKEBUILDDIR}/test123/SquareDesk.app/Contents/MacOS
otool -L SquareDesk | egrep "qua"
# install_name_tool -change libquazip.1.dylib @executable_path/libquazip.1.dylib SquareDesk
echo Those two lines should now start with executable_path...
otool -L SquareDesk | egrep "qua"
popd

# ----------------------------------------------------------------------------------------
echo Now running Mac Deploy Qt step...  NOTE: MUST BE UNCOMMENTED OUT AND RUN ONCE
# NOT THIS ONE: ~/Qt6.2.3/${QTVERSION}/macos/bin/macdeployqt ${MIKEBUILDDIR}/test123/SquareDesk.app 2>&1 | grep -v "ERROR: Could not parse otool output line"
#~/Qt/${QTVERSION}/macos/bin/macdeployqt ${MIKEBUILDDIR}/test123/SquareDesk.app 2>&1 | grep -v "ERROR: Could not parse otool output line"
~/Qt${QTVERSION}/${QTVERSION}/macos/bin/macdeployqt ${MIKEBUILDDIR}/test123/SquareDesk.app 2>&1 | grep -v "ERROR: Could not parse otool output line"

echo Mac Deploy Qt step done.
echo

# ----------------------------------------------------------------------------------------
echo "-------------------------------------------"
echo Building version $VERSION of $APP_NAME DMG
echo "-------------------------------------------"
echo

# you should not need to change these
APP_EXE="${APP_NAME}.app/Contents/MacOS/${APP_NAME}"
RENAMED_APP_EXE="${APP_NAME}_${VERSION}.app/Contents/MacOS/${APP_NAME}"

VOL_NAME="${APP_NAME}_${VERSION}_X86"   # volume name will be "SquareDesk_0.9.6_X86"
DMG_TMP="${VOL_NAME}-temp.dmg"
DMG_FINAL="${VOL_NAME}.dmg"         # final DMG name will be "SquareDesk_0.9.6.dmg"
STAGING_DIR="${MIKEBUILDDIR}/Install"             # we copy all our stuff into this dir

echo STAGING_DIR: ${STAGING_DIR}

# ----------------------------------------------------------------------------------------
# Check the background image DPI and convert it if it isn't 72x72
_BACKGROUND_IMAGE_DPI_H=`sips -g dpiHeight ${DMG_BACKGROUND_IMG} | grep -Eo '[0-9]+\.[0-9]+'`
_BACKGROUND_IMAGE_DPI_W=`sips -g dpiWidth ${DMG_BACKGROUND_IMG} | grep -Eo '[0-9]+\.[0-9]+'`

if [ $(echo " $_BACKGROUND_IMAGE_DPI_H != 72.0 " | bc) -eq 1 -o $(echo " $_BACKGROUND_IMAGE_DPI_W != 72.0 " | bc) -eq 1 ]; then
   echo "WARNING: The background image's DPI is not 72.  This will result in distorted backgrounds on Mac OS X 10.7+."
   echo "         I will convert it to 72 DPI for you."

   _DMG_BACKGROUND_TMP="${DMG_BACKGROUND_IMG%.*}"_dpifix."${DMG_BACKGROUND_IMG##*.}"

   sips -s dpiWidth 72 -s dpiHeight 72 ${DMG_BACKGROUND_IMG} --out ${_DMG_BACKGROUND_TMP}

   DMG_BACKGROUND_IMG="${_DMG_BACKGROUND_TMP}"
fi

# clear out any old data
rm -rf "${STAGING_DIR}" "${DMG_TMP}" "${DMG_FINAL}"

# copy over the stuff we want in the final disk image to our staging dir
echo
echo Making Staging Directory...
mkdir -p "${STAGING_DIR}"

# ----------------------------------------------------------------------------------------
echo "================================================================"
echo "Running xattr to clean up permissions... (no need for sudo here)"
xattr -r "${MIKEBUILDDIR}/test123/${APP_NAME}.app"
xattr -cr "${MIKEBUILDDIR}/test123/${APP_NAME}.app"
xattr -r "${MIKEBUILDDIR}/test123/${APP_NAME}.app"

echo "Copying .app to STAGING_DIR and renaming to ${APP_NAME}_${VERSION}.app..."
cp -Rpf "${MIKEBUILDDIR}/test123/${APP_NAME}.app" "${STAGING_DIR}/${APP_NAME}_${VERSION}.app"

# ----------------------------------------------------------------------------------------
echo "============================================================================"
echo Now copying in the libdarwinmediaplugin.dylib to avoid the error: QtMultimedia is not currently supported on this platform or compiler
echo which is described here: https://forum.qt.io/topic/139680/qt6-4-compiled-app-asserts-qtmultimedia-is-not-currently-supported-on-this-platform-or-compiler-and-crashes-qt6-3-2-runs-fine/2
echo This is probably a bug in macdeployqt, starting with Qt6.4.1, and this step might be removable in the future.
echo
echo Making the directory....
mkdir -p "${STAGING_DIR}/${APP_NAME}_${VERSION}.app/Contents/PlugIns/multimedia"
echo Doing the copy of the one file we need...
#ls -al "${QTDIR}/${QTVERSION}/macos/plugins/multimedia/libdarwinmediaplugin.dylib"
cp -pfv "${QTDIR}/${QTVERSION}/macos/plugins/multimedia/libdarwinmediaplugin.dylib" "${STAGING_DIR}/${APP_NAME}_${VERSION}.app/Contents/PlugIns/multimedia/libdarwinmediaplugin.dylib"
echo COPY DONE.
echo
echo "============================================================================"

echo "Running xattr to clean up permissions, belt and suspenders...(no need for sudo here)"
xattr -r "${STAGING_DIR}/${APP_NAME}_${VERSION}.app"
xattr -cr "${STAGING_DIR}/${APP_NAME}_${VERSION}.app"
xattr -r "${STAGING_DIR}/${APP_NAME}_${VERSION}.app"
echo "================================================================"

# ----------------
# I am not sure why the .plist file is not getting copied into the executable.
#  Do that here to be sure it's in.
echo "**** COPYING IN PLIST FILE...."
cp ${SOURCEDIR}/Info.plist "${STAGING_DIR}/${APP_NAME}_${VERSION}.app/Contents/Info.plist"
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
echo "Stripping ${RENAMED_APP_EXE}..."
strip -u -r "${RENAMED_APP_EXE}"
echo

# compress the executable if we have upx in PATH
#  UPX: http://upx.sourceforge.net/
# if hash upx 2>/dev/null; then
#    echo "Compressing (UPX) ${APP_EXE}..."
#    upx -9 "${APP_EXE}"
# fi

# ... perform any other stripping/compressing of libs and executables

# Locate all the Qt Universal libs, and strip out the x86_64 part, since this is an M1-only executable right now
echo "----- Removing M1 stuff to make the X86-specific .app file much thinner...."
ls -al ${STAGING_DIR}/${APP_NAME}_${VERSION}.app/Contents/Frameworks
find ${STAGING_DIR}/${APP_NAME}_${VERSION}.app/Contents/Frameworks -type f -name "Qt*" -size +1k
find ${STAGING_DIR}/${APP_NAME}_${VERSION}.app/Contents/Frameworks -type f -name "Qt*" -size +1k -exec mv {} {}.backup \; -exec lipo -remove arm64 {}.backup -output {} \; -exec ls -al {} \; -exec ls -al {}.backup \; -exec rm {}.backup \;
echo "----- DONE WITH LIPO ON QT FRAMEWORKS"
echo 

popd

# figure out how big our DMG needs to be
#  assumes our contents are at least 1M!
SIZE=`du -sh "${STAGING_DIR}" | sed 's/\([0-9\.]*\)M\(.*\)/\1/'`
SIZE=`echo "${SIZE} + 5.0" | bc | awk '{print int($1+0.5)}'`

if [ $? -ne 0 ]; then
   echo "Error: Cannot compute size of staging dir"
   exit
fi

echo "-------------------------"
echo SIZE: ${SIZE} MB
echo STAGING_DIR: ${STAGING_DIR}
echo VOL_NAME: ${VOL_NAME}
echo DMG_TMP: ${DMG_TMP}

echo
echo creating the temp DMG file...
hdiutil create -srcfolder "${STAGING_DIR}" -volname "${VOL_NAME}" -fs HFS+ \
      -fsargs "-c c=64,a=16,e=16" -format UDRW -size ${SIZE}M "${DMG_TMP}"

echo "Created DMG: ${DMG_TMP}"
echo

echo Mounting it and saving the device...
DEVICE=$(hdiutil attach -readwrite -noverify "${DMG_TMP}" | \
         egrep '^/dev/' | sed 1q | awk '{print $1}')
echo "DEVICE: ${DEVICE}"
echo

sleep 2

# add a link to the Applications dir
echo "Add link to /Applications"
pushd /Volumes/"${VOL_NAME}"
ln -s /Applications
popd

# add a background image
echo "Make the .background directory"
mkdir /Volumes/"${VOL_NAME}"/.background

echo "Copy in the background image: ${DMG_BACKGROUND_IMG}"
cp "${DMG_BACKGROUND_IMG}" /Volumes/"${VOL_NAME}"/.background/

# tell the Finder to resize the window, set the background,
#  change the icon size, place the icons in the right position, etc.
echo "Telling Finder to make it pretty: ${VOL_NAME} ${APP_NAME} ${DMG_BACKGROUND_IMG}"

echo "Sleeping 5 seconds to work around the -1728 error..."
sleep 5

echo '
   tell application "Finder"
     #list disks
     tell disk "'${VOL_NAME}'"
           open
           set current view of container window to icon view
           set toolbar visible of container window to false
           set statusbar visible of container window to false
           set the bounds of container window to {400, 100, 920, 440}
           set viewOptions to the icon view options of container window
           set arrangement of viewOptions to not arranged
           set icon size of viewOptions to 72
           set background picture of viewOptions to file ".background:'${DMG_BACKGROUND_IMG_SHORT}'"
           set position of item "'${APP_NAME}_${VERSION}'.app" of container window to {160, 205}
           set position of item "Applications" of container window to {360, 205}
           close
           open
           update without registering applications
           delay 2
     end tell
   end tell
' | osascript

echo
echo "Syncing..."
sync

# unmount it
echo
echo "Unmounting..."
hdiutil detach "${DEVICE}"

# now make the final image a compressed disk image
echo
echo "Creating compressed image"
echo "DMG_FINAL: ${DMG_FINAL}"
hdiutil convert "${DMG_TMP}" -format UDZO -imagekey zlib-level=9 -o "${DMG_FINAL}"

# clean up
echo
echo "Cleaning up..."
rm -rf "${DMG_TMP}"
#rm -rf "${STAGING_DIR}"     #  keep this one around, because it's useful for testing

echo 'DONE.'
echo "-------------------------"

exit
