#!/bin/bash

# SquareDesk Notarization Script
# This script handles the notarization process for SquareDesk.

# Parse command line arguments
APPLE_ID="mpogue@zenstarstudio.com"  # Default Apple ID
APP_PASSWORD=""
TEAM_ID="K63QFQ4P65"  # Default Team ID
CHECK_INTERVAL=300  # Default check interval (5 minutes)

# Parse command line options
while [[ $# -gt 0 ]]; do
    case $1 in
        --apple_id)
            APPLE_ID="$2"
            shift 2
            ;;
        --app_specific_pwd)
            APP_PASSWORD="$2"
            shift 2
            ;;
        --team_id)
            TEAM_ID="$2"
            shift 2
            ;;
        --check_interval_sec)
            CHECK_INTERVAL="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--apple_id <apple_id>] --app_specific_pwd <password> [--team_id <team_id>] [--check_interval_sec <seconds>]"
            exit 1
            ;;
    esac
done

# Make sure we're in the test123 directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Check if we're in test123 directory
if [[ "$(basename "$PWD")" != "test123" ]]; then
    echo "Error: Script must be run from the test123 directory"
    exit 1
fi

# Validate required parameters
if [ -z "$APP_PASSWORD" ]; then
    echo "Error: --app_specific_pwd parameter is required"
    echo "Usage: $0 [--apple_id <apple_id>] --app_specific_pwd <password> [--team_id <team_id>] [--check_interval_sec <seconds>]"
    exit 1
fi

# Look for the SquareDesk app in the Install directory
INSTALL_DIR="../Install"
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

echo "====== SquareDesk Notarization Script ======"
echo "Starting at $(date)"
echo "App to notarize: $(basename "$APP_PATH")"
echo "Apple ID: $APPLE_ID"
echo "Team ID: $TEAM_ID"
echo "Check interval: $CHECK_INTERVAL seconds"
echo ""

# Step 1: Create ZIP archive
APP_NAME=$(basename "$APP_PATH")
ZIP_NAME="${APP_NAME%.app}.zip"

echo "Creating ZIP archive: $ZIP_NAME"
if [ -f "$ZIP_NAME" ]; then
    rm "$ZIP_NAME"
    echo "Removed existing ZIP file"
fi

ditto -c -k --keepParent "$APP_PATH" "$ZIP_NAME"
if [ $? -ne 0 ]; then
    echo "Error: Failed to create ZIP archive"
    exit 1
fi
echo "Successfully created ZIP archive: $ZIP_NAME"
echo ""

# Step 2: Submit for notarization
echo "Submitting for notarization..."
echo ""

# Submit to Apple's notarization service
SUBMIT_OUTPUT=$(xcrun notarytool submit "$ZIP_NAME" --apple-id "$APPLE_ID" --password "$APP_PASSWORD" --team-id "$TEAM_ID" 2>&1)
SUBMIT_STATUS=$?

if [ $SUBMIT_STATUS -ne 0 ]; then
    echo "Error: Notarization submission failed with status $SUBMIT_STATUS"
    echo "Output: $SUBMIT_OUTPUT"
    exit 1
fi

# Save the output to a file for debugging
echo "$SUBMIT_OUTPUT" > submit_output.txt

# Extract the submission ID with a regexp for UUIDs
REQUEST_ID=$(grep -o -E "[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}" submit_output.txt | head -1)

if [ -z "$REQUEST_ID" ]; then
    echo "Error: Could not extract submission ID from output. Please check submit_output.txt"
    cat submit_output.txt
    exit 1
fi

echo "Submission successful. Request ID: $REQUEST_ID"
echo ""

# Step 3: Wait for notarization to complete
echo "Waiting for notarization to complete..."
echo "Checking status every $(($CHECK_INTERVAL / 60)) minutes"
echo ""

while true; do
    echo "Checking status at $(date)..."
    
    # Save the status output to a file for debugging
    STATUS_OUTPUT=$(xcrun notarytool info "$REQUEST_ID" --apple-id "$APPLE_ID" --password "$APP_PASSWORD" --team-id "$TEAM_ID" 2>&1)
    echo "$STATUS_OUTPUT" > status_output.txt
    
    if [ $? -ne 0 ]; then
        echo "Error checking status. Will retry in $(($CHECK_INTERVAL / 60)) minutes..."
        cat status_output.txt
        sleep $CHECK_INTERVAL
        continue
    fi
    
    # Extract status using grep - match exactly the line format from the observed output
    STATUS=$(grep -E "^  status: " status_output.txt | sed 's/^  status: //g')
    
    if [ -z "$STATUS" ]; then
        echo "Warning: Could not parse status from output. Will retry in $(($CHECK_INTERVAL / 60)) minutes..."
        cat status_output.txt
        sleep $CHECK_INTERVAL
        continue
    fi
    
    echo "Current status: $STATUS"
    
    if [ "$STATUS" == "Accepted" ]; then
        echo "✅ Notarization successful!"
        break
    elif [ "$STATUS" == "Invalid" ] || [ "$STATUS" == "Rejected" ]; then
        echo "❌ Notarization rejected"
        
        # Get and save the logs
        LOG_FILE="notarization_log_${REQUEST_ID}.json"
        echo "Saving detailed logs to $LOG_FILE"
        xcrun notarytool log "$REQUEST_ID" --apple-id "$APPLE_ID" --password "$APP_PASSWORD" --team-id "$TEAM_ID" > "$LOG_FILE" 2>&1
        
        echo "Please check $LOG_FILE for details on why notarization failed"
        exit 1
    elif [ "$STATUS" == "In Progress" ]; then
        echo "Notarization still in progress. Waiting $(($CHECK_INTERVAL / 60)) minutes..."
        sleep $CHECK_INTERVAL
    else
        echo "Unknown status: $STATUS"
        echo "Will retry in $(($CHECK_INTERVAL / 60)) minutes..."
        sleep $CHECK_INTERVAL
    fi
done

# Step 4: Staple the notarization ticket
echo ""
echo "Stapling notarization ticket to $APP_PATH..."
xcrun stapler staple "$APP_PATH"

if [ $? -ne 0 ]; then
    echo "⚠️ Warning: Failed to staple notarization ticket"
    echo "The app is notarized but users may still see security warnings"
else
    echo "✅ Successfully stapled notarization ticket"
    
    # Verify the stapling
    echo "Verifying stapling..."
    xcrun stapler validate "$APP_PATH"
    
    if [ $? -ne 0 ]; then
        echo "⚠️ Warning: Stapling verification failed"
    else
        echo "✅ Stapling verification successful"
    fi
fi

echo ""
echo "✅ SUCCESS: SquareDesk has been successfully notarized!"
echo "The app is now ready for distribution."
echo ""
echo "Completed at $(date)"
echo "====== End of Notarization Process ======"

# Clean up temporary files, including the ZIP file that was uploaded for notarization
rm -f submit_output.txt status_output.txt $ZIP_NAME
