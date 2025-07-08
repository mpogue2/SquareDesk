#!/bin/bash
# Build script for SquareDesk MCP DXT
# This script compiles the MCP server and creates a DXT file for Claude Desktop

# Exit on any error
set -e

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(dirname "$SCRIPT_DIR")"

# Define directories
SRC_DIR="$BASE_DIR/src"
BUILD_DIR="$BASE_DIR/build"
BIN_DIR="$BUILD_DIR/bin"

# Define file names
CPP_FILE="main_simple.cpp"
BINARY_NAME="sd_mcp"
DXT_NAME="squaredesk-mcp.dxt"
MANIFEST="manifest.json"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Building SquareDesk MCP DXT ===${NC}"

# Create build directories if they don't exist
echo -e "${YELLOW}Creating build directories...${NC}"
mkdir -p "$BUILD_DIR"
mkdir -p "$BIN_DIR"

# Check if source files exist
if [ ! -f "$SRC_DIR/$CPP_FILE" ]; then
    echo -e "${RED}Error: Source file $SRC_DIR/$CPP_FILE not found!${NC}"
    exit 1
fi

if [ ! -f "$SRC_DIR/$MANIFEST" ]; then
    echo -e "${RED}Error: Manifest file $SRC_DIR/$MANIFEST not found!${NC}"
    exit 1
fi

# Compile the C++ source
echo -e "${YELLOW}Compiling $CPP_FILE...${NC}"
g++ -std=c++17 -O2 -o "$BIN_DIR/$BINARY_NAME" "$SRC_DIR/$CPP_FILE"

if [ $? -eq 0 ]; then
    echo -e "${GREEN}Compilation successful!${NC}"
else
    echo -e "${RED}Compilation failed!${NC}"
    exit 1
fi

# Make the binary executable
chmod +x "$BIN_DIR/$BINARY_NAME"

# Create temporary directory for DXT contents
TEMP_DIR=$(mktemp -d)
trap "rm -rf $TEMP_DIR" EXIT

# Copy files to temp directory
echo -e "${YELLOW}Preparing DXT contents...${NC}"
cp "$BIN_DIR/$BINARY_NAME" "$TEMP_DIR/"
cp "$SRC_DIR/$MANIFEST" "$TEMP_DIR/"

# Create the DXT file
echo -e "${YELLOW}Creating DXT file...${NC}"
cd "$TEMP_DIR"
zip -r "$BUILD_DIR/$DXT_NAME" .

if [ $? -eq 0 ]; then
    echo -e "${GREEN}DXT creation successful!${NC}"
else
    echo -e "${RED}DXT creation failed!${NC}"
    exit 1
fi

# Display results
echo -e "${GREEN}=== Build Complete ===${NC}"
echo -e "Binary: $BIN_DIR/$BINARY_NAME"
echo -e "DXT file: $BUILD_DIR/$DXT_NAME"
echo -e ""
echo -e "${GREEN}The DXT file is ready to be installed in Claude Desktop!${NC}"
echo -e "Double-click on $BUILD_DIR/$DXT_NAME to install."