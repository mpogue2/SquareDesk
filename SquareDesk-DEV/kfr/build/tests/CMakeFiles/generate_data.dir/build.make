# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
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
RM = /Applications/CMake.app/Contents/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build

# Include any dependencies generated for this target.
include tests/CMakeFiles/generate_data.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include tests/CMakeFiles/generate_data.dir/compiler_depend.make

# Include the progress variables for this target.
include tests/CMakeFiles/generate_data.dir/progress.make

# Include the compile flags for this target's objects.
include tests/CMakeFiles/generate_data.dir/flags.make

tests/CMakeFiles/generate_data.dir/generate_data.cpp.o: tests/CMakeFiles/generate_data.dir/flags.make
tests/CMakeFiles/generate_data.dir/generate_data.cpp.o: ../tests/generate_data.cpp
tests/CMakeFiles/generate_data.dir/generate_data.cpp.o: tests/CMakeFiles/generate_data.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object tests/CMakeFiles/generate_data.dir/generate_data.cpp.o"
	cd /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build/tests && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT tests/CMakeFiles/generate_data.dir/generate_data.cpp.o -MF CMakeFiles/generate_data.dir/generate_data.cpp.o.d -o CMakeFiles/generate_data.dir/generate_data.cpp.o -c /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/tests/generate_data.cpp

tests/CMakeFiles/generate_data.dir/generate_data.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/generate_data.dir/generate_data.cpp.i"
	cd /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build/tests && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/tests/generate_data.cpp > CMakeFiles/generate_data.dir/generate_data.cpp.i

tests/CMakeFiles/generate_data.dir/generate_data.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/generate_data.dir/generate_data.cpp.s"
	cd /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build/tests && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/tests/generate_data.cpp -o CMakeFiles/generate_data.dir/generate_data.cpp.s

# Object files for target generate_data
generate_data_OBJECTS = \
"CMakeFiles/generate_data.dir/generate_data.cpp.o"

# External object files for target generate_data
generate_data_EXTERNAL_OBJECTS =

bin/generate_data: tests/CMakeFiles/generate_data.dir/generate_data.cpp.o
bin/generate_data: tests/CMakeFiles/generate_data.dir/build.make
bin/generate_data: /usr/local/lib/libmpfr.dylib
bin/generate_data: /usr/local/lib/libgmp.dylib
bin/generate_data: tests/CMakeFiles/generate_data.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../bin/generate_data"
	cd /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build/tests && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/generate_data.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
tests/CMakeFiles/generate_data.dir/build: bin/generate_data
.PHONY : tests/CMakeFiles/generate_data.dir/build

tests/CMakeFiles/generate_data.dir/clean:
	cd /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build/tests && $(CMAKE_COMMAND) -P CMakeFiles/generate_data.dir/cmake_clean.cmake
.PHONY : tests/CMakeFiles/generate_data.dir/clean

tests/CMakeFiles/generate_data.dir/depend:
	cd /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/tests /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build/tests /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build/tests/CMakeFiles/generate_data.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tests/CMakeFiles/generate_data.dir/depend

