INTRODUCTION

	* The latest top-of-tree version of SquareDesk has been built and tested on
		Mac OS Big Sur and Monterey.  Both M1 and X86 versions have been built and they work.
	* The M1 version of SquareDesk will run only on M1 Macs.
	  The X86 version will run on either X86 Macs (Native) or M1 Macs (with Rosetta).
	  Performance on M1 Macs is best with the M1 version of SquareDesk.
	* The Windows version of SquareDesk is now deprecated, since our Windows users have all 
		moved to Macbooks.

REQUIREMENTS

	* Open Source versions of Qt 6 and QtCreator (generally latest versions, 
		download the Qt Online Installer here: https://www.qt.io/download-open-source) 
	* Using the Online Install tool, also download the following Qt optional packages: 
			Webengine, WebChannel, WebSockets, Positioning, and Multimedia
	* XCode (generally the latest version, from the Apple App Store)
	* MacOS Big Sur or Monterey (NOTE: not tested on Ventura yet)
	* M1/ARM Mac or X86/Intel Mac (NOTE: Windows is no longer supported)

OPTIONAL TOOLS
	* for modifying graphics source files: Affinity Photo and Affinity Design
		($55 each, https://affinity.serif.com/en-us/photo/)
	* for viewing and/or manually editing the sqlite3 database: DB Browser for SQLite
		(free, https://sqlitebrowser.org)

DEPENDENCIES

	* Dependent libraries are committed to the SquareDesk repo, so you don't
		have to spend time on version matching.  Therefore no manual source code downloads 
		are required for building SquareDesk.
	* Some version numbers (e.g. XTools version) may be hard-coded in the main .pro file.  Updates
		to the build OS version (e.g. moving to Ventura) may require changing these version numbers.

COMPILING AND BUILDING SQUAREDESK

	* Download and unpack the ZIP file from GitHub.
	* Compile SoundTouch manually (takes just a few seconds):
		cd SquareDesk-DEV/test123/soundtouch
		cmake .
		make -j
	* Inside QtCreator, open up the SquareDesk project.
		Press Cmd-B to build everything, or Cmd-R to both build and run.
	* There should be close to zero warnings against the SquareDesk code base.  There are LOTS of warnings
		against the included open source code bases (SD, I'm looking at you in particular), 
		which I am unlikely to ever fix.

DEBUGGING SQUAREDESK

	* In the Music Directory, there is a hidden .squaredesk	folder, containing: 
		* a Sqlite3 database with song information
		* debug.log file (running non-debug apps can append to this file).  In the Release
			version of SquareDesk, only serious error messages are written to the log.  Debugging
			messages (qDebug()) are generally all commented out for Release.
		* lock.txt file (allows only one laptop at a time to access SquareDesk's database)
	* the .squaredesk folder can be made visible in Finder by pressing Ctrl-Cmd-<period> .

EXECUTING SQUAREDESK

	* For testing, run from within QtCreator with Cmd-R
	* I have used Box.net successfully to share the Music Directory across multiple
		laptops.  SquareDesk checks for attempts at simultaneous access from multiple laptops, 
		and it will warn you.

DISTRIBUTING SQUAREDESK FOR MAC

	* After the build is tested, create a DMG with a self-contained version
		of SquareDesk by double clicking on one of PackageIt_M1.command (for M1-based Macs)
		or PackageIt.command (for X86 Macs).  NOTE: you might have to change hard-coded pathnames
		in the .command files, for your particular installation.
	* After running one of the PackageIt command files, the DMG will be ready for
		the usual drag-and-drop-style installation to the Applications folder.
	* The .app executable files have different names for M1 and X86, so that they can coexist on a single
		M1-based Mac in the same folder.  Be sure to run only one at a time, or they might both try to
		modify the database.  There is no protection against two separate instances 
		running on a single machine.

DISTRIBUTING SQUAREDESK FOR WINDOWS

	* NOTE: top-of-tree Master branch does NOT compile or run on Windows at this time, 
		so the following info is only for historical purposes
	* After the build is tested, create a Windows installation package using 
		Inno Setup (https://jrsoftware.org/isinfo.php) and the .iss file located here:
		SquareDesk-DEV/test123/setup_win32.iss .
