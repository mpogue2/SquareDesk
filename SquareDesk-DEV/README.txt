INTRODUCTION

	* The latest top-of-tree version of SquareDesk has been built and tested on
		Mac OS Tahoe.
	* The X86 version of SquareDesk is no longer supported (there were almost zero
		users, and it was painful to maintain).  If somebody wants to take this
		on again in the future, I'm all for it!
	* The ARM64 version of SquareDesk will run on any ARM64 Mac, e.g. M1/M2/M3/M4

REQUIREMENTS

	* Open Source versions of Qt 6 and QtCreator (generally latest versions, 
		download the Qt Online Installer here: https://www.qt.io/download-open-source) 
	* Using the Online Install tool, also download the following Qt optional packages: 
			Webengine, WebChannel, WebSockets, Positioning, and Multimedia
	* XCode (generally the latest version, from the Apple App Store)
	* macOS Tahoe or later
	* ARM64-based Mac (M4 compilation of everything is about a minute!)
	* If it complains about juce include file(s) missing, run script juce-install

OPTIONAL TOOLS

	* for modifying graphics source files: Affinity Photo and Affinity Design
		($55 each, https://affinity.serif.com/en-us/photo/)
	* for viewing and/or manually editing the sqlite3 database: DB Browser for SQLite
		(free, https://sqlitebrowser.org)

DEPENDENCIES

	* Dependent libraries are committed to the SquareDesk repo, so you don't
		have to spend time on version matching.  Therefore no manual source code downloads 
		are required for building SquareDesk.  JUCE is now installed automatically.
	* Some version numbers (e.g. XTools version) may be hard-coded in the main .pro file.  Updates
		to the build OS version may require changing these version numbers.

COMPILING AND BUILDING SQUAREDESK

	* Download and unpack the ZIP file from GitHub.
	* Compile SoundTouch manually (takes just a few seconds):
		cd SquareDesk-DEV/test123/soundtouch
		cmake .
		make -j
	* Inside QtCreator, open up the top-level SquareDesk project (SquareDesk.pro).
	* Configure the project in QtCreator to choose the tool chain.  Simplest is to 
		choose the default Desktop (arm-darwin-generic-mach_o-64bit) version.
	* Make sure that the build directories are NOT the default (if you use the default ones,
		the build may succeed, but the macdeployqt step may fail).  The build directories
		should look something like these:
			/Users/mpogue/clean3/SquareDesk/build-SquareDesk-Qt_6_9_2_for_macOS-Debug
			/Users/mpogue/clean3/SquareDesk/build-SquareDesk-Qt_6_9_2_for_macOS-Release
	* Figure out which SDK version you have, using:
		ls /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs
		For example, this might return: "MacOSX.sdk MacOSX26.0.sdk MacOSX26.sdk"
	* Edit the test123/test123.pro file to change this line:
		QMAKE_MAC_SDK = macosx26.0
		to match the XCode SDK version that you'd like to use.
	* In QtCreator, choose the version to build (Debug vs Release) -- lower left corner of QtCreator.
	* Remove any existing build directories, corresponding to the build version you chose
	* Press Cmd-B to build everything, or Cmd-R to both build and run.
	* There should be close to zero warnings against the SquareDesk part of the code base.  There are LOTS of warnings
		against the included open source code bases (SD, I'm looking at you in particular), 
		which I am unlikely to ever fix.

DEBUGGING SQUAREDESK

	* In the Music Directory, there is a hidden .squaredesk	folder, containing: 
		* a Sqlite3 database with song information
		* debug.log file (running non-debug apps can append to this file).  In the Release
			version of SquareDesk, only serious error messages are written to the log.  Debugging
			messages (qDebug()) are generally all commented out for Release.
		* lock.txt file (allows only one laptop at a time to access SquareDesk's database)
	* The .squaredesk folder can be made visible in Finder by pressing Ctrl-Cmd-<period> in the Finder.
	* The easiest way to debug SquareDesk (in my opinion) is to do so within QtCreator.  
		You can set breakpoints, or you can use qDebug() << "string: " << value; statements.  Many of the 
		qDebug() statements used for debugging are still there in the source code, just commented out.  This 
		might provide some hints as to which variables are interesting for debugging. 
	* NOTE: Execution time and CPU utilization is a LOT higher with the Debug build, but it also provides
		more information for debugging, especially when using breakpoints.  I generally do most of my 
		debugging directly in the Release version, with qDebug() statements, just because it runs faster, AND
		because most of my bugs are easily found with qDebug() statements.
	* Test your build setup by making a minor change to a .cpp file.  Then press Cmd-R, and the relevant
		files should be recompiled (watch the Compile Output pane in QtCreator), and SquareDesk should start up.
	* I use the free "DB Browser for SQLite" app for peeking inside the .squaredesk/SquareDesk.sqlite3 database.

EXECUTING SQUAREDESK

	* For testing, run from within QtCreator with Cmd-R
	* I have used Box.net and iCloud successfully for sharing the Music Directory across multiple
		laptops.  SquareDesk checks for attempts at simultaneous access from multiple laptops, 
		and it will warn you.

DISTRIBUTING SQUAREDESK FOR MAC

	* After the build is tested, create a DMG using "releaseSquareDesk.command" from command line
		where the current working directory is the RELEASE BUILD directory/test123.
		NOTE: you might have to change hard-coded pathnames in the .command files, for your particular installation.
		This will now digitally sign the SquareDesk application AND the DMG file, so that users won't have
		to do anything special when downloading and running.
	* The DMG will be ready for the usual drag-and-drop-style installation to the Applications folder.
		After the DMG is built, manually close the terminal window that shows the DMG build progress.

DISTRIBUTING SQUAREDESK FOR LINUX

	* This section needs to be written

DISTRIBUTING SQUAREDESK FOR WINDOWS (deprecated)

	* NOTE: top-of-tree Master branch does NOT compile or run on Windows at this time, 
		so the following info is only for historical purposes
	* After the build is tested, create a Windows installation package using 
		Inno Setup (https://jrsoftware.org/isinfo.php) and the .iss file located here:
		SquareDesk-DEV/test123/setup_win32.iss .
