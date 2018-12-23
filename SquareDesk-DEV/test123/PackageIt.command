#!/bin/bash

# by Andy Maloney
# http://asmaloney.com/2013/07/howto/packaging-a-mac-os-x-application-using-a-dmg/

# make sure we are in the correct dir when we double-click a .command file
dir=${0%/*}
if [ -d "$dir" ]; then
  cd "$dir"
fi

echo $dir

# the macdeployqt step!
# cd build-test123-Desktop_Qt_5_7_0_clang_64bit-Debug/

# if errors about can't find libs, try:
# cd to the app/Contents/MacOS
# otool -L SquareDeskPlayer
# install_name_tool -change libquazip.1.dylib @executable_path/libquazip.1.dylib SquareDeskPlayer
# install_name_tool -change libtidy.5.dylib @executable_path/libtidy.5.dylib SquareDeskPlayer
# otool -L SquareDeskPlayer
#
# These errors can be ignored, they are false dependencies:
# ERROR: no file at "/opt/local/lib/mysql55/mysql/libmysqlclient.18.dylib"
# ERROR: no file at "/usr/local/lib/libpq.5.dylib"

#WHICH=Debug
WHICH=Release

# set up your app name, version number, and background image file name
APP_NAME="SquareDesk"
VERSION="0.9.2alpha12"
DMG_BACKGROUND_IMG="installer3.png"

QTVERSION="5.9.7"
QT_VERSION="5_9_7"   # same thing, but with underscores (yes, change both of them at the same time!)

#MANUAL="SquareDeskManual.pdf"

# ------------------------------------------------------------------------
echo Now running otool to fixup libraries...
pushd /Users/mpogue/clean3/SquareDesk/build-SquareDesk-Desktop_Qt_${QT_VERSION}_clang_64bit-${WHICH}/test123/SquareDesk.app/Contents/MacOS
otool -L SquareDesk | egrep "qua|tidy"
install_name_tool -change libquazip.1.dylib @executable_path/libquazip.1.dylib SquareDesk
install_name_tool -change libtidy.5.dylib @executable_path/libtidy.5.dylib SquareDesk
echo Those two lines should now start with executable_path...
otool -L SquareDesk | egrep "qua|tidy"
popd

# ---------------------------------------------------
echo Now running Mac Deploy Qt step...
 ~/Qt/${QTVERSION}/clang_64/bin/macdeployqt SquareDesk.app
echo Mac Deploy Qt step done.
echo

echo "--------------------------------------"
echo Building version $VERSION of $APP_NAME
echo "--------------------------------------"
echo

# you should not need to change these
APP_EXE="${APP_NAME}.app/Contents/MacOS/${APP_NAME}"

VOL_NAME="${APP_NAME} ${VERSION}"   # volume name will be "SuperCoolApp 1.0.0"
DMG_TMP="${VOL_NAME}-temp.dmg"
DMG_FINAL="${VOL_NAME}.dmg"         # final DMG name will be "SuperCoolApp 1.0.0.dmg"
STAGING_DIR="./Install"             # we copy all our stuff into this dir

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
mkdir -p "${STAGING_DIR}"
#cp -rpf "${APP_NAME}.app" "${STAGING_DIR}"
cp -Rpf "${APP_NAME}.app" "${STAGING_DIR}"

# copy in the SquareDesk Manual
#cp -rpf "${MANUAL}" "${STAGING_DIR}"

# ... cp anything else you want in the DMG - documentation, etc.

echo STAGING_DIR: ${STAGING_DIR}
pushd "${STAGING_DIR}"

# strip the executable
echo "Stripping ${APP_EXE}..."
strip -u -r "${APP_EXE}"

# compress the executable if we have upx in PATH
#  UPX: http://upx.sourceforge.net/
if hash upx 2>/dev/null; then
   echo "Compressing (UPX) ${APP_EXE}..."
   upx -9 "${APP_EXE}"
fi

# ... perform any other stripping/compressing of libs and executables

popd

# figure out how big our DMG needs to be
#  assumes our contents are at least 1M!
SIZE=`du -sh "${STAGING_DIR}" | sed 's/\([0-9\.]*\)M\(.*\)/\1/'`
SIZE=`echo "${SIZE} + 5.0" | bc | awk '{print int($1+0.5)}'`

if [ $? -ne 0 ]; then
   echo "Error: Cannot compute size of staging dir"
   exit
fi

echo SIZE: ${SIZE} MB
echo STAGING_DIR: ${STAGING_DIR}
echo VOL_NAME: ${VOL_NAME}
echo DMG_TMP: ${DMG_TMP}

# create the temp DMG file
hdiutil create -srcfolder "${STAGING_DIR}" -volname "${VOL_NAME}" -fs HFS+ \
      -fsargs "-c c=64,a=16,e=16" -format UDRW -size ${SIZE}M "${DMG_TMP}"

echo "Created DMG: ${DMG_TMP}"

# mount it and save the device
DEVICE=$(hdiutil attach -readwrite -noverify "${DMG_TMP}" | \
         egrep '^/dev/' | sed 1q | awk '{print $1}')

echo "DEVICE: ${DEVICE}"

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
           set background picture of viewOptions to file ".background:'${DMG_BACKGROUND_IMG}'"
           set position of item "'${APP_NAME}'.app" of container window to {160, 205}
           set position of item "Applications" of container window to {360, 205}
           close
           open
           update without registering applications
           delay 2
     end tell
   end tell
' | osascript

echo "Syncing..."
sync

# unmount it
echo "Unmounting..."
hdiutil detach "${DEVICE}"

# now make the final image a compressed disk image
echo "Creating compressed image"
hdiutil convert "${DMG_TMP}" -format UDZO -imagekey zlib-level=9 -o "${DMG_FINAL}"

# clean up
echo "Cleaning up..."
rm -rf "${DMG_TMP}"
#rm -rf "${STAGING_DIR}"     #  keep this one around, because it's useful for testing

echo 'DONE.'

exit
