#!/bin/bash

# releaseSquareDesk.command
# Top-level script to execute all SquareDesk release steps in order
# Created on: May 14, 2025

# Parse command line arguments
CERT_ID=""
APP_SPECIFIC_PWD=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --certid)
            CERT_ID="$2"
            shift 2
            ;;
        --app-specific-pwd)
            APP_SPECIFIC_PWD="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 --app-specific-pwd <app_specific_password> [--certid <certificate_id>]"
            exit 1
            ;;
    esac
done

# Validate required parameters
if [ -z "$APP_SPECIFIC_PWD" ]; then
    echo "Error: --app-specific-pwd parameter is required"
    echo "Usage: $0 --app-specific-pwd <app_specific_password> [--certid <certificate_id>]"
    exit 1
fi

echo "Starting SquareDesk Release Process..."
echo "-------------------------------------"

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Extract the version number from Info.plist
VERSION=$(/usr/libexec/PlistBuddy -c "Print :CFBundleShortVersionString" "./SquareDesk.app/Contents/Info.plist")
if [ $? -ne 0 ] || [ -z "$VERSION" ]; then
    echo "Error: Could not extract version number from Info.plist. Aborting release process."
    exit 1
fi
echo "Extracted version number: $VERSION"
echo

# Step 1: Make Installation Version
echo "Step 1: Creating installation version for version $VERSION..."
./makeInstallVersion.command "$VERSION"
if [ $? -ne 0 ]; then
    echo "Error: makeInstallVersion.command failed. Aborting release process."
    exit 1
fi
echo "Installation version created successfully."
echo

# Step 2: Fix and Sign SquareDesk
echo "Step 2: Fixing and signing SquareDesk..."
if [ -n "$CERT_ID" ]; then
    echo "Using custom certificate ID: $CERT_ID"
    ./fixAndSignSquareDesk.command --certid "$CERT_ID"
else
    echo "Using default certificate ID"
    ./fixAndSignSquareDesk.command
fi
if [ $? -ne 0 ]; then
    echo "Error: fixAndSignSquareDesk.command failed. Aborting release process."
    exit 1
fi
echo "SquareDesk fixed and signed successfully."
echo

# Step 3: Notarize SquareDesk
echo "Step 3: Notarizing SquareDesk..."
./notarizeSquareDesk.command --app_specific_pwd "$APP_SPECIFIC_PWD"
if [ $? -ne 0 ]; then
    echo "Error: notarizeSquareDesk.command failed. Aborting release process."
    exit 1
fi
echo "SquareDesk notarized successfully."
echo

# Step 4: Create DMG
echo "Step 4: Creating DMG package..."
if [ -n "$CERT_ID" ]; then
    echo "Using custom certificate ID for DMG signing: $CERT_ID"
    ./makeDMG.command --certid "$CERT_ID"
else
    echo "Using default certificate ID for DMG signing"
    ./makeDMG.command
fi
if [ $? -ne 0 ]; then
    echo "Error: makeDMG.command failed. Aborting release process."
    exit 1
fi
echo "DMG package created successfully."
echo

echo "-------------------------------------"
echo "SquareDesk Release Process Complete!"
echo "Version: $VERSION"
echo "-------------------------------------"

exit 0
