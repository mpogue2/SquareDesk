# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.9

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /Applications/CMake.app/Contents/bin/cmake

# The command to remove a file.
RM = /Applications/CMake.app/Contents/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/bin

# Include any dependencies generated for this target.
include CMakeFiles/tidy.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/tidy.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/tidy.dir/flags.make

CMakeFiles/tidy.dir/console/tidy.c.o: CMakeFiles/tidy.dir/flags.make
CMakeFiles/tidy.dir/console/tidy.c.o: ../console/tidy.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/bin/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/tidy.dir/console/tidy.c.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/tidy.dir/console/tidy.c.o   -c /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/console/tidy.c

CMakeFiles/tidy.dir/console/tidy.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/tidy.dir/console/tidy.c.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/console/tidy.c > CMakeFiles/tidy.dir/console/tidy.c.i

CMakeFiles/tidy.dir/console/tidy.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/tidy.dir/console/tidy.c.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/console/tidy.c -o CMakeFiles/tidy.dir/console/tidy.c.s

CMakeFiles/tidy.dir/console/tidy.c.o.requires:

.PHONY : CMakeFiles/tidy.dir/console/tidy.c.o.requires

CMakeFiles/tidy.dir/console/tidy.c.o.provides: CMakeFiles/tidy.dir/console/tidy.c.o.requires
	$(MAKE) -f CMakeFiles/tidy.dir/build.make CMakeFiles/tidy.dir/console/tidy.c.o.provides.build
.PHONY : CMakeFiles/tidy.dir/console/tidy.c.o.provides

CMakeFiles/tidy.dir/console/tidy.c.o.provides.build: CMakeFiles/tidy.dir/console/tidy.c.o


# Object files for target tidy
tidy_OBJECTS = \
"CMakeFiles/tidy.dir/console/tidy.c.o"

# External object files for target tidy
tidy_EXTERNAL_OBJECTS =

tidy: CMakeFiles/tidy.dir/console/tidy.c.o
tidy: CMakeFiles/tidy.dir/build.make
tidy: libtidys.a
tidy: CMakeFiles/tidy.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/bin/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable tidy"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/tidy.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/tidy.dir/build: tidy

.PHONY : CMakeFiles/tidy.dir/build

CMakeFiles/tidy.dir/requires: CMakeFiles/tidy.dir/console/tidy.c.o.requires

.PHONY : CMakeFiles/tidy.dir/requires

CMakeFiles/tidy.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/tidy.dir/cmake_clean.cmake
.PHONY : CMakeFiles/tidy.dir/clean

CMakeFiles/tidy.dir/depend:
	cd /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/bin && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/bin /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/bin /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/bin/CMakeFiles/tidy.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/tidy.dir/depend

