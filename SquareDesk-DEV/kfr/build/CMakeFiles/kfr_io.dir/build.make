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
include CMakeFiles/kfr_io.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/kfr_io.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/kfr_io.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/kfr_io.dir/flags.make

CMakeFiles/kfr_io.dir/include/kfr/io/impl/audiofile-impl.cpp.o: CMakeFiles/kfr_io.dir/flags.make
CMakeFiles/kfr_io.dir/include/kfr/io/impl/audiofile-impl.cpp.o: ../include/kfr/io/impl/audiofile-impl.cpp
CMakeFiles/kfr_io.dir/include/kfr/io/impl/audiofile-impl.cpp.o: CMakeFiles/kfr_io.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/kfr_io.dir/include/kfr/io/impl/audiofile-impl.cpp.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/kfr_io.dir/include/kfr/io/impl/audiofile-impl.cpp.o -MF CMakeFiles/kfr_io.dir/include/kfr/io/impl/audiofile-impl.cpp.o.d -o CMakeFiles/kfr_io.dir/include/kfr/io/impl/audiofile-impl.cpp.o -c /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/include/kfr/io/impl/audiofile-impl.cpp

CMakeFiles/kfr_io.dir/include/kfr/io/impl/audiofile-impl.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/kfr_io.dir/include/kfr/io/impl/audiofile-impl.cpp.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/include/kfr/io/impl/audiofile-impl.cpp > CMakeFiles/kfr_io.dir/include/kfr/io/impl/audiofile-impl.cpp.i

CMakeFiles/kfr_io.dir/include/kfr/io/impl/audiofile-impl.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/kfr_io.dir/include/kfr/io/impl/audiofile-impl.cpp.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/include/kfr/io/impl/audiofile-impl.cpp -o CMakeFiles/kfr_io.dir/include/kfr/io/impl/audiofile-impl.cpp.s

# Object files for target kfr_io
kfr_io_OBJECTS = \
"CMakeFiles/kfr_io.dir/include/kfr/io/impl/audiofile-impl.cpp.o"

# External object files for target kfr_io
kfr_io_EXTERNAL_OBJECTS =

libkfr_io.a: CMakeFiles/kfr_io.dir/include/kfr/io/impl/audiofile-impl.cpp.o
libkfr_io.a: CMakeFiles/kfr_io.dir/build.make
libkfr_io.a: CMakeFiles/kfr_io.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libkfr_io.a"
	$(CMAKE_COMMAND) -P CMakeFiles/kfr_io.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/kfr_io.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/kfr_io.dir/build: libkfr_io.a
.PHONY : CMakeFiles/kfr_io.dir/build

CMakeFiles/kfr_io.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/kfr_io.dir/cmake_clean.cmake
.PHONY : CMakeFiles/kfr_io.dir/clean

CMakeFiles/kfr_io.dir/depend:
	cd /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build /Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/kfr/build/CMakeFiles/kfr_io.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/kfr_io.dir/depend
