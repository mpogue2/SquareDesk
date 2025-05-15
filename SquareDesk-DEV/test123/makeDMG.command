#!/bin/bash

echo "==================================="
# make sure we are in the correct dir when we double-click a .command file
dir=${0%/*}
if [ -d "$dir" ]; then
    cd "$dir"
fi

# Parse command line arguments
CUSTOM_CERT_ID=""
while [[ $# -gt 0 ]]; do
    case $1 in
        --certid)
            CUSTOM_CERT_ID="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--certid <certificate_id>]"
            exit 1
            ;;
    esac
done

# check we are in a build directory
echo checking we are in suitable directory $PWD
echo $PWD | grep -E -q 'build.*Qt.*macOS-(Debug|Release)/test123$' || { echo invalid directory ; exit 1; }

# we need to be in the directory above the test123 directory
cd ..

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

# set up your app name, version number, and background image file name
APP_NAME="SquareDesk"

# Find the SquareDesk_*.app in the Install directory to get the version number
INSTALL_DIR="${BUILDDIR}/Install"
if [ ! -d "$INSTALL_DIR" ]; then
    echo "Error: Install directory not found at $INSTALL_DIR"
    exit 1
fi

# Find SquareDesk_*.app in the Install directory
FOUND_APPS=( $(find "$INSTALL_DIR" -maxdepth 1 -name "SquareDesk_*.app" -type d) )

# Check if we found exactly one app
if [ ${#FOUND_APPS[@]} -eq 0 ]; then
    echo "Error: No SquareDesk_*.app found in $INSTALL_DIR"
    exit 1
elif [ ${#FOUND_APPS[@]} -gt 1 ]; then
    echo "Error: Multiple SquareDesk_*.app files found in $INSTALL_DIR:"
    for app in "${FOUND_APPS[@]}"; do
        echo "  $(basename "$app")"
    done
    echo "Please clean up the Install directory and try again."
    exit 1
fi

# Set the app path
APP_PATH="${FOUND_APPS[0]}"
APP_INFO_PLIST="$APP_PATH/Contents/Info.plist"

# Extract version from Info.plist
if [ ! -f "$APP_INFO_PLIST" ]; then
    echo "Error: Info.plist not found at $APP_INFO_PLIST"
    exit 1
fi

VERSION=$(/usr/libexec/PlistBuddy -c "Print :CFBundleShortVersionString" "$APP_INFO_PLIST")
if [ $? -ne 0 ] || [ -z "$VERSION" ]; then
    echo "Error: Could not extract version number from Info.plist"
    exit 1
fi

echo "Found app: $(basename "$APP_PATH") with version: $VERSION"

QT_VERSION=$(echo $PWD | sed -e 's/.*Qt_//' -e 's/_for.*//')
QTVERSION=$(echo $QT_VERSION | sed -e 's/_/./g')

HOMEDIR=$HOME

SOURCEDIR=$(pwd | sed -e 's|/build.*||')/SquareDesk-DEV/test123 
BUILDDIR=$PWD
QTDIR="${HOMEDIR}/Qt"

DMG_BACKGROUND_IMG_SHORT="installer3.png"
DMG_BACKGROUND_IMG="${SOURCEDIR}/images/${DMG_BACKGROUND_IMG_SHORT}"

echo "---------------------------------------"
echo HOMEDIR: ${HOMEDIR}
echo SOURCEDIR: ${SOURCEDIR}
echo QTDIR: ${QTDIR}
echo BUILDDIR: ${BUILDDIR}
echo DMG_BACKGROUND_IMG: ${DMG_BACKGROUND_IMG}
echo DYLD_FRAMEWORK_PATH: ${DYLD_FRAMEWORK_PATH}
echo "---------------------------------------"
echo

# ----------------------------------------------------------------------------------------
echo "-------------------------------------------"
echo Building version $VERSION of $APP_NAME DMG
echo "-------------------------------------------"
echo

VOL_NAME="${APP_NAME}_${VERSION}"   # volume name will be "SquareDesk_0.9.6"
DMG_TMP="${VOL_NAME}-temp.dmg"
DMG_FINAL="${VOL_NAME}.dmg"         # final DMG name will be "SquareDesk_0.9.6.dmg"
STAGING_DIR="${BUILDDIR}/Install"             # we copy all our stuff into this dir

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
rm -rf "${DMG_TMP}" "${DMG_FINAL}"

# figure out how big our DMG needs to be
#  assumes our contents are at least 1M!
SIZE=`du -sh "${STAGING_DIR}" | sed 's/\([0-9\.]*\)M\(.*\)/\1/'`
SIZE=`echo "${SIZE} + 20.0" | bc | awk '{print int($1+0.5)}'`

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

echo "-------------------------"
echo creating the temp DMG file...
hdiutil create -srcfolder "${STAGING_DIR}" -volname "${VOL_NAME}" -fs HFS+ \
      -fsargs "-c c=64,a=16,e=16" -format UDRW -size ${SIZE}M "${DMG_TMP}"
echo "Created DMG: ${DMG_TMP}"

echo "-------------------------"
echo Mounting it and saving the device...
DEVICE=$(hdiutil attach -readwrite -noverify "${DMG_TMP}" | \
         egrep '^/dev/' | sed 1q | awk '{print $1}')
echo "DEVICE: ${DEVICE}"

echo "-------------------------"
echo "sleeping for 2 seconds..."
sleep 2

# add a link to the Applications dir
echo "-------------------------"
echo "Add link to /Applications"
pushd /Volumes/"${VOL_NAME}"
ln -s /Applications
popd

# add a background image
echo "-------------------------"
echo "Make the .background directory"
mkdir /Volumes/"${VOL_NAME}"/.background

echo "-------------------------"
echo "Copy in the background image: ${DMG_BACKGROUND_IMG}"
cp "${DMG_BACKGROUND_IMG}" /Volumes/"${VOL_NAME}"/.background/

echo "-------------------------"
echo "Sleeping 5 seconds to work around the -1728 error..."
sleep 5

# tell the Finder to resize the window, set the background,
#  change the icon size, place the icons in the right position, etc.
echo "-------------------------"
echo "Telling Finder to make it pretty: ${VOL_NAME} ${APP_NAME} ${DMG_BACKGROUND_IMG}"

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

echo "-------------------------"
echo "Syncing..."
sync

# unmount it
echo "-------------------------"
echo "Unmounting..."
hdiutil detach "${DEVICE}"

# now make the final image a compressed disk image
echo "-------------------------"
echo "Creating compressed image"
echo "DMG_FINAL: ${DMG_FINAL}"
hdiutil convert "${DMG_TMP}" -format UDZO -imagekey zlib-level=9 -o "${DMG_FINAL}"

# Now code sign the DMG file, too.
echo "-------------------------"
echo "Code signing the DMG file..."

# Set the certificate ID (use custom if provided, otherwise use default)
DEFAULT_CERT_ID="Developer ID Application: Michael Pogue (K63QFQ4P65)"
if [ -n "$CUSTOM_CERT_ID" ]; then
    CERT_ID="$CUSTOM_CERT_ID"
    echo "Using custom certificate ID: $CERT_ID"
else
    CERT_ID="$DEFAULT_CERT_ID"
    echo "Using default certificate ID: $CERT_ID"
fi

codesign --force --sign "$CERT_ID" ${DMG_FINAL}

# clean up
echo "-------------------------"
echo "Cleaning up..."
rm -rf "${DMG_TMP}"

echo "-------------------------"
echo 'DONE.'
echo "==================================="
echo

exit
