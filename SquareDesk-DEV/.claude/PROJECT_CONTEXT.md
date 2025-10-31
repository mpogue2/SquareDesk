# SquareDesk Project Context

## Project Overview

**SquareDesk** is a comprehensive music player and choreography tool designed specifically for square dance callers. It provides advanced audio playback features, music library management, lyrics/cuesheet editing, and two integrated square dance choreography engines (SD & Taminations).

**Primary Authors**: Mike Pogue, Dan Lyke (2016-2025)
**License**: Dual GPL2/GPL3 and Commercial
**Current Status**: Active development, macOS ARM64 (M1/M2/M3/M4) primary platform

## Key Features

- Advanced audio playback with pitch/tempo control (independent adjustment)
- Music library management with SQLite database
- Lyrics and cuesheet editor with syntax highlighting
- Integrated square dance choreography engine (SD)
- Beat/bar detection and segmentation
- Playlist management
- Sound effects playback (whistles, applause, etc.)
- PDF reference viewing (SD documentation, manuals)
- Taminations integration (dance animations via embedded HTTP server)
- Dark/Light mode themes
- Parametric EQ and audio filters
- Song metadata editing (ID3 tags)

## Architecture Overview

### Technology Stack

- **Language**: C++11/C++17
- **Framework**: Qt 6.x (widgets, multimedia, webengine, network, svg, sql, httpserver)
- **Build System**: qmake (.pro files)
- **Platform**: macOS 12.0+ (primary), Linux (supported), Windows (now obsolete)

### Main Components

1. **Audio Engine** (`flexible_audio.cpp`, `bass_audio.cpp`)
   - New "flexible audio" classes mimic the now-obsolete BASS audio library integration
   - Qt Multimedia (ARM64/M1 builds, M1MAC define)
   - Real-time pitch/tempo adjustment via SoundTouch
   - Audio processing with KFR DSP library
   - Beat/bar detection via VAMP plugins
   - Music segmentation via VAMP plugins

2. **UI Layer** (`mainwindow.cpp` and split modules)
   - `mainwindow_init.cpp` - Initialization and startup
   - `mainwindow_audio.cpp` - Audio controls and playback UI
   - `mainwindow_music.cpp` - Music library and playlist UI
   - `mainwindow_cuesheets.cpp` - Lyrics/cuesheet editor
   - `mainwindow_sd.cpp` - Square dance choreography UI
   - `mainwindow_themes.cpp` - Theme and styling
   - Custom widgets: `svgClock`, `svgSlider`, `svgDial`, `svgVUmeter`, `svgWaveformSlider`

3. **Square Dance Choreography Engine** (`sdlib/`, `sdinterface.cpp`)
   - External SD choreography engine (separate library)
   - Call database (`sd_calls.dat`)
   - Formation visualization (`squaredancerscene.cpp`)
   - Syntax highlighting (`sdhighlighter.cpp`)
   - Undo/redo stack (`sdredostack.cpp`)

4. **Database Layer** (`prefsmanager.cpp`, `songsettings.cpp`)
   - prefsmanager: preferences local to a machine
   - songsettings: per-song settings that are the same on all the user's machines
   - SQLite3 database (`.squaredesk/SquareDesk.sqlite3` in Music Directory)
   - Song metadata, playlists, preferences
   - TagLib for ID3 tag reading/writing

5. **Web Components**
   - PDF viewer (`qpdfjs/` - embedded PDF.js)
   - Taminations server (`embeddedserver.cpp`, `taminationsinterface.cpp`)
   - Qt WebEngine widgets for embedded browser views

6. **Audio Processing**
   - SoundTouch (pitch/tempo) - `soundtouch/`
   - KFR (DSP filters) - `kfr/`
   - VAMP (beat detection and music segmentation) - external plugins
   - MiniBPM (BPM detection) - `miniBPM/`
   - minimp3 (MP3 decoding) - header-only library
   - JUCE library (compression using the LoudMax VST)

## Directory Structure

```
SquareDesk-DEV/
├── .claude/                    # Claude Code context files
├── test123/                    # Main application source
│   ├── main.cpp               # Entry point
│   ├── mainwindow.*           # Main window (split across multiple files)
│   ├── test123.pro            # Qt project file
│   ├── resources.qrc          # Qt resources
│   ├── soundtouch/            # SoundTouch library (pitch/tempo)
│   ├── miniBPM/               # BPM detection
│   ├── graphics/              # UI graphics (SVG, PNG)
│   ├── soundfx/               # Sound effects (MP3)
│   ├── themes/                # QSS stylesheets
│   ├── docs/                  # User manual
│   └── *.command              # macOS build/release scripts
├── sdlib/                     # Square Dance choreography engine (separate project)
├── html-tidy/                 # HTML Tidy library (cleanup lyrics HTML)
├── mp3gain/                   # MP3 normalization tool
├── taglib/                    # ID3 tag library
├── kfr/                       # KFR DSP library (filters)
├── qpdfjs/                    # PDF.js viewer wrapper
├── libJUCEstatic/             # JUCE audio library (static build)
├── Taminations/               # Dance animation web app (web.zip)
├── local_macosx/              # macOS-specific binaries (VAMP, etc.)
├── local_win32/               # Windows libraries (obsolete)
└── SquareDesk.pro             # Top-level project file
```

## Build System

### Qt Project Files

- **SquareDesk.pro** - Top-level project (TEMPLATE = subdirs)
- **test123/test123.pro** - Main application
- Platform-specific sections (macx, win32, unix:!macx)
- Extensive custom build steps (copy resources, deploy scripts)

### Key Build Settings

- **Deployment Target**: macOS 12.0+ (`QMAKE_MACOSX_DEPLOYMENT_TARGET`)
- **SDK Version**: Must match installed Xcode SDK (e.g., `macosx26.0`)
- **Architecture Detection**: x86_64 vs arm64 (M1MAC define)
- **Qt Modules**: core, gui, widgets, multimedia, webenginewidgets, sql, network, svg, httpserver, concurrent

### Dependencies (Included in Repo)

- SoundTouch - compiled manually before build
- JUCE - auto-installed via `juce-install` script
- KFR - auto-built via `kfr/create-kfr-lib`
- TagLib - built as part of project
- VAMP plugins - pre-compiled binaries in `local_macosx/vamp/`

## Common Development Tasks

### Building from Source

1. Install Qt 6.x with QtCreator and required modules
2. Install latest Xcode from App Store
3. Clone/unpack repository
4. Compile SoundTouch:
   ```bash
   cd SquareDesk-DEV/test123/soundtouch
   cmake .
   make -j
   ```
5. Update SDK version in `test123/test123.pro` if needed
6. Open `SquareDesk.pro` in QtCreator
7. Configure build directories (NOT default)
8. Build with Cmd-B or run with Cmd-R

### Debugging

- **Debug Builds**: Run from QtCreator, slower but better debugging
- **Release Builds**: Faster, use `qDebug()` statements for logging
- **Log File**: `~/<MusicDir>/.squaredesk/debug.log`
- **Database**: View with "DB Browser for SQLite"
- **Custom Message Handler**: `MainWindow::customMessageOutput()` (standalone) or `customMessageOutputQt()` (QtCreator)

### Version Updates

1. Update version in `mainwindow.cpp` (`VERSIONSTRING`)
2. Update version in `Info.plist`
3. Update scripts: `makeInstallVersion.command`, etc.

### Creating Release DMG

```bash
cd build-SquareDesk-Qt_6_9_3_for_macOS-Release/test123
./releaseSquareDesk.command --app-specific-pwd APP_SPECIFIC_PASSWORD_GOES_HERE
```

This will:
- Fix library paths
- Code sign application
- Notarize with Apple
- Create DMG installer
- Sign DMG

## Code Organization Patterns

### Main Window Split

The main window logic is split across multiple files for maintainability:
- `mainwindow.cpp` - Core UI logic, general event handling
- `mainwindow_init.cpp` - Initialization, setup, startup wizard
- `mainwindow_audio.cpp` - Audio playback controls
- `mainwindow_music.cpp` - Music library, playlists
- `mainwindow_cuesheets.cpp` - Lyrics editor
- `mainwindow_sd.cpp` - Square dance choreography
- `mainwindow_themes.cpp` - Theming and styles
- `mainwindow_fonts.cpp` - Font management
- `mainwindow_metadata.cpp` - ID3 tag editing
- `mainwindow_bulk.cpp` - Bulk operations
- `mainwindow_taminations.cpp` - Taminations integration
- `mainwindow_JUCE.cpp` - JUCE integration

### Key Classes

- `MainWindow` - Primary UI controller
- `PreferencesManager` - Settings and database access
- `SongSettings` - Per-song metadata
- `SDInterface` - Square dance engine interface
- `FlexibleAudio` / `BassAudio` - Audio backends
- `EmbeddedServer` - HTTP server for Taminations
- `LyricsEditor` - Cuesheet editor logic

### Custom Widgets

- `svgSlider`, `svgDial` - SVG-based UI controls
- `svgWaveformSlider` - Audio waveform display
- `svgClock` - Analog clock widget
- `svgVUmeter` - VU meter display
- `MyTableWidget` - Enhanced table widget
- `SongTitleLabel` - Clickable song title with context menu

## Important Files

### Configuration

- `test123.pro` - Main build configuration (SDK versions, dependencies)
- `Info.plist` - macOS bundle info (version, copyright)
- `sd.ini` - SD engine configuration

### Data Files

- `allcalls.csv` - Square dance call database
- `sd_calls.dat` - SD engine call definitions
- `squareDanceLabelIDs.csv` - Record label mappings
- `abbrevs.txt` - Abbreviation expansions
- `lyrics.template.html` - Lyrics HTML template
- `patter.template.html` - Patter call template
- `cuesheet2.css` - Cuesheet styling

### Build Scripts (macOS)

- `makeInstallVersion.command` - Prepare install version
- `fixAndSignSquareDesk.command` - Fix paths and code sign
- `notarizeSquareDesk.command` - Apple notarization
- `makeDMG.command` - Create DMG installer
- `releaseSquareDesk.command` - Complete release process

## Platform-Specific Notes

### macOS (Primary Platform)

- **M1/ARM64 Build**: Define `M1MAC=1`, uses Qt Multimedia
- **x86_64 Build**: Uses BASS audio library (obsolete)
- **Bundle Structure**: Resources in `SquareDesk.app/Contents/Resources/`
- **Frameworks**: CoreFoundation, AppKit, MediaPlayer, QuartzCore, Security, Accelerate, WebKit, AudioToolbox

### Linux

- Build system present but needs documentation
- Uses Qt Multimedia
- JUCE and KFR integration
- TagLib for metadata

### Windows (Obsolete)

- No longer maintained as of 2025
- Used BASS audio library
- Inno Setup for installers (`setup_win32.iss`)

## Known Issues and Quirks

1. **SDK Version**: Must manually update `QMAKE_MAC_SDK` in `.pro` file when Xcode updates
2. **Context Menu Bug**: First selection of context menu item doesn't work unless window focus is changed. Workaround: `setVisible(false)` then `setVisible(true)` in `main.cpp`
3. **SoundTouch**: Must be manually compiled before building SquareDesk
4. **Build Directories**: Cannot use Qt default build directories or deployment may fail
5. **Restart Feature**: `QProcess::startDetached()` restart doesn't work from within QtCreator
6. **Debug vs Release**: Debug more stable for development, Release faster for daily use

## Recent Changes

- Auto-select dance program from dance name suffix (`_MS`, `_PLUS`, etc.)
- Accept 'MS' as Mainstream program abbreviation
- Improved SD engine integration with UI updates

## External Resources

- **BASS Audio**: http://www.un4seen.com/ (obsolete)
- **SoundTouch**: https://www.surina.net/soundtouch/
- **KFR DSP**: https://www.kfrlib.com/
- **VAMP Plugins**: https://vamp-plugins.org/
- **Taminations**: https://taminations.com/
- **SD Engine**: https://challengedance.org/sd/
- **Github source control**: https://github.com/mpogue2/SquareDesk
- **JUCE audio plugin library**: https://juce.com
- **SquareDesk WordPress website for announcements**: https://squaredesk.net

## Development Environment

- **IDE**: Qt Creator (latest)
- **Compiler**: Clang (via Xcode, latest version)
- **Qt Version**: 6.x (latest stable, currently 6.9.3)
- **Graphics Tools**: Affinity Photo/Designer (for SVG/PNG assets)
- **Database Tool**: DB Browser for SQLite
- **Version Control**: Git

## Contact

- **Email**: mpogue @ zenstarstudio.com
- **Organization**: Zenstar Software
- **Website**: zenstarstudio.com

---

**Last Updated**: October 2025
**Current Version**: 0.9.6+
**Platform**: macOS Tahoe (15.0) / ARM64
