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
include examples/CMakeFiles/fir.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include examples/CMakeFiles/fir.dir/compiler_depend.make

# Include the progress variables for this target.
include examples/CMakeFiles/fir.dir/progress.make

# Include the compile flags for this target's objects.
include examples/CMakeFiles/fir.dir/flags.make

examples/CMakeFiles/fir.dir/fir.cpp.o: examples/CMakeFiles/fir.dir/flags.make
examples/CMakeFiles/fir.dir/fir.cpp.o: ../examples/fir.cpp
examples/CMakeFiles/fir.dir/fir.cpp.o: examples/CMakeFiles/fir.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object examples/CMakeFiles/fir.dir/fir.cpp.o"
	cd /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build/examples && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT examples/CMakeFiles/fir.dir/fir.cpp.o -MF CMakeFiles/fir.dir/fir.cpp.o.d -o CMakeFiles/fir.dir/fir.cpp.o -c /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/examples/fir.cpp

examples/CMakeFiles/fir.dir/fir.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/fir.dir/fir.cpp.i"
	cd /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build/examples && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/examples/fir.cpp > CMakeFiles/fir.dir/fir.cpp.i

examples/CMakeFiles/fir.dir/fir.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/fir.dir/fir.cpp.s"
	cd /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build/examples && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/examples/fir.cpp -o CMakeFiles/fir.dir/fir.cpp.s

# Object files for target fir
fir_OBJECTS = \
"CMakeFiles/fir.dir/fir.cpp.o"

# External object files for target fir
fir_EXTERNAL_OBJECTS =

bin/fir: examples/CMakeFiles/fir.dir/fir.cpp.o
bin/fir: examples/CMakeFiles/fir.dir/build.make
bin/fir: libkfr_dft.a
bin/fir: examples/CMakeFiles/fir.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../bin/fir"
	cd /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build/examples && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/fir.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
examples/CMakeFiles/fir.dir/build: bin/fir
.PHONY : examples/CMakeFiles/fir.dir/build

examples/CMakeFiles/fir.dir/clean:
	cd /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build/examples && $(CMAKE_COMMAND) -P CMakeFiles/fir.dir/cmake_clean.cmake
.PHONY : examples/CMakeFiles/fir.dir/clean

examples/CMakeFiles/fir.dir/depend:
	cd /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/examples /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build/examples /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build/examples/CMakeFiles/fir.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : examples/CMakeFiles/fir.dir/depend

