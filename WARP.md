# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

## Project Overview

SquareDesk is a modern cross-platform application for Western Square Dance callers, built with Qt/C++. It combines a music player, cuesheet editor, and graphical version of the SD (Square Dance) sequence checker program. The application supports macOS (including M1/ARM), Linux, and Windows (deprecated).

## Build System & Commands

### Prerequisites
- **macOS**: Qt 6 (latest), Xcode (latest), macOS Big Sur or later
- **Linux**: Qt 6, build-essential, cmake, various dev packages (see `Linux.md`)

### Essential Build Commands

```bash
# Initial setup (macOS) - Install JUCE dependencies
cd SquareDesk-DEV
./juce-install

# Build SoundTouch library manually first
cd SquareDesk-DEV/test123/soundtouch
cmake .
make -j

# Main project build (using QtCreator)
# Open SquareDesk-DEV/SquareDesk.pro in QtCreator
# Configure project for desired architecture (ARM/x86)
# Build with Cmd+B or run with Cmd+R

# Linux build
cd SquareDesk-DEV
qmake SquareDesk.pro
make
cd sdlib && qmake && make
cd ../test123 && make
./SquareDesk
```

### Platform-Specific Build Notes

**macOS ARM (M1/M2):**
- Use `macosx15.5` SDK (adjust version in `test123.pro` line 47)
- Remove existing build directories before switching architectures
- M1 builds define `M1MAC=1` preprocessor flag

**macOS Intel:**
- Use same SDK but different architecture detection
- X86 builds work on both Intel and M1 (via Rosetta)

### Testing & Debugging

```bash
# Run from QtCreator for debugging (recommended)
# Or build and run directly:
cd SquareDesk-DEV/test123
./SquareDesk

# Debug logs location:
# macOS: ~/.squaredesk/debug.log (hidden folder in Music Directory)
# Use Ctrl+Cmd+. in Finder to show hidden files

# Database inspection:
# Use "DB Browser for SQLite" to examine ~/.squaredesk/SquareDesk.sqlite3
```

## Architecture Overview

### Main Components

**Core Application (`test123/` directory):**
- `main.cpp`: Application entry point with splash screen and theme initialization
- `mainwindow.*`: Central UI controller with modular functionality split across files:
  - `mainwindow_music.cpp`: Music playback and audio controls
  - `mainwindow_cuesheets.cpp`: Cuesheet editing functionality  
  - `mainwindow_sd.cpp`: Square Dance sequence designer integration
  - `mainwindow_taminations.cpp`: Taminations animation integration
  - Additional modules for themes, bulk operations, file management, etc.

**Audio System:**
- `flexible_audio.*`: Abstract audio backend (wraps platform-specific implementations)
- `bass_audio.*`: BASS library integration (non-M1 Macs)
- Built-in Qt Multimedia support (M1 Macs and Linux)
- SoundTouch integration for pitch/tempo modification
- VAMP plugins for beat/measure detection and audio segmentation

**Square Dance Integration:**
- `sdlib/`: Static library containing SD (Square Dance) sequence checker
- `sdinterface.*`: Interface layer between UI and SD engine
- `squaredancerscene.*`: Graphical representation of dance formations
- Custom language model and grammar files for voice recognition

**Third-Party Libraries:**
- **JUCE**: Audio processing and cross-platform utilities
- **TagLib**: MP3/audio metadata handling  
- **KFR**: Digital signal processing filters
- **QuaZip**: Archive handling for cuesheet downloads
- **QtWebEngine**: Embedded web views for PDFs and Taminations

### Key Architectural Patterns

**Modular MainWindow Design:**
The main application window is split across multiple source files (`mainwindow_*.cpp`) to separate concerns by feature area. Each module handles a specific tab or functional area while sharing the main UI object.

**Platform Abstraction:**
- Audio backend selection at compile time based on platform capabilities
- Conditional compilation blocks for macOS/Linux/Windows differences
- Resource bundling differs significantly between platforms

**Plugin Architecture:**
- VAMP plugin system for audio analysis
- Embedded web server for local content serving
- External tool integration (SD, VAMP tools) via process spawning

### Build System Architecture

**Qt Project Structure:**
- Top-level `SquareDesk.pro` defines subdirectories and dependencies
- Each major component has its own `.pro` file
- Complex platform-specific resource copying during build
- Extensive use of qmake's conditional compilation features

**Dependency Management:**
- Most dependencies are committed to repo (self-contained)
- JUCE installed via custom script (`juce-install`)
- Platform-specific libraries in `local_*` directories
- KFR library built from source during compilation

**Resource Bundling:**
macOS builds include extensive post-build steps to copy resources into the app bundle:
- Templates, themes, and configuration files
- VAMP plugins and dependencies
- Taminations web content
- Sound effects and PDF documentation

## Development Workflow

### Code Organization
- UI forms in `.ui` files (Qt Designer)
- Significant use of Qt's signal/slot system
- Custom widgets inherit from Qt base classes
- Preference management through `PreferencesManager` singleton

### Key Files to Understand
- `globaldefines.h`: Platform detection and configuration
- `common_enums.h`: Application-wide enumerations
- `prefsmanager.*`: Settings and preferences handling
- `utility.*`: Common utility functions
- `songsettings.*`: Per-song metadata and settings

### Platform-Specific Considerations
- macOS requires code signing and notarization for distribution
- SDK version must be kept current and matches Xcode installation
- M1 vs Intel builds require different library paths and compilation flags
- Linux builds require manual dependency installation and different Qt modules

### Audio Development Notes
- Beat detection requires specific VAMP plugin binaries
- Audio format support varies by platform (BASS vs Qt Multimedia)
- Real-time audio processing uses separate threads
- Waveform analysis and display is CPU-intensive

## Distribution

### macOS Distribution
- DMG creation scripts in `test123/` directory
- Separate builds required for M1 and Intel architectures  
- Code signing and notarization scripts included
- App bundle structure must include all dependencies

### Linux Distribution
- No formal packaging yet (manual build required)
- Desktop integration examples provided
- Requires runtime dependencies to be installed

### Windows (Deprecated)
- Windows support discontinued as of current version
- Legacy build files remain but are not maintained
- Users migrated to macOS platform
