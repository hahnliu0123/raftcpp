# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/wsl/project/raftcpp

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/wsl/project/raftcpp/build

# Include any dependencies generated for this target.
include src/CMakeFiles/raftcpp.dir/depend.make

# Include the progress variables for this target.
include src/CMakeFiles/raftcpp.dir/progress.make

# Include the compile flags for this target's objects.
include src/CMakeFiles/raftcpp.dir/flags.make

src/CMakeFiles/raftcpp.dir/args.pb.cc.o: src/CMakeFiles/raftcpp.dir/flags.make
src/CMakeFiles/raftcpp.dir/args.pb.cc.o: ../src/args.pb.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/wsl/project/raftcpp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/CMakeFiles/raftcpp.dir/args.pb.cc.o"
	cd /home/wsl/project/raftcpp/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/raftcpp.dir/args.pb.cc.o -c /home/wsl/project/raftcpp/src/args.pb.cc

src/CMakeFiles/raftcpp.dir/args.pb.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/raftcpp.dir/args.pb.cc.i"
	cd /home/wsl/project/raftcpp/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/wsl/project/raftcpp/src/args.pb.cc > CMakeFiles/raftcpp.dir/args.pb.cc.i

src/CMakeFiles/raftcpp.dir/args.pb.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/raftcpp.dir/args.pb.cc.s"
	cd /home/wsl/project/raftcpp/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/wsl/project/raftcpp/src/args.pb.cc -o CMakeFiles/raftcpp.dir/args.pb.cc.s

src/CMakeFiles/raftcpp.dir/raft.cc.o: src/CMakeFiles/raftcpp.dir/flags.make
src/CMakeFiles/raftcpp.dir/raft.cc.o: ../src/raft.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/wsl/project/raftcpp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object src/CMakeFiles/raftcpp.dir/raft.cc.o"
	cd /home/wsl/project/raftcpp/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/raftcpp.dir/raft.cc.o -c /home/wsl/project/raftcpp/src/raft.cc

src/CMakeFiles/raftcpp.dir/raft.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/raftcpp.dir/raft.cc.i"
	cd /home/wsl/project/raftcpp/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/wsl/project/raftcpp/src/raft.cc > CMakeFiles/raftcpp.dir/raft.cc.i

src/CMakeFiles/raftcpp.dir/raft.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/raftcpp.dir/raft.cc.s"
	cd /home/wsl/project/raftcpp/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/wsl/project/raftcpp/src/raft.cc -o CMakeFiles/raftcpp.dir/raft.cc.s

# Object files for target raftcpp
raftcpp_OBJECTS = \
"CMakeFiles/raftcpp.dir/args.pb.cc.o" \
"CMakeFiles/raftcpp.dir/raft.cc.o"

# External object files for target raftcpp
raftcpp_EXTERNAL_OBJECTS =

lib/libraftcpp.a: src/CMakeFiles/raftcpp.dir/args.pb.cc.o
lib/libraftcpp.a: src/CMakeFiles/raftcpp.dir/raft.cc.o
lib/libraftcpp.a: src/CMakeFiles/raftcpp.dir/build.make
lib/libraftcpp.a: src/CMakeFiles/raftcpp.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/wsl/project/raftcpp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX static library ../lib/libraftcpp.a"
	cd /home/wsl/project/raftcpp/build/src && $(CMAKE_COMMAND) -P CMakeFiles/raftcpp.dir/cmake_clean_target.cmake
	cd /home/wsl/project/raftcpp/build/src && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/raftcpp.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/CMakeFiles/raftcpp.dir/build: lib/libraftcpp.a

.PHONY : src/CMakeFiles/raftcpp.dir/build

src/CMakeFiles/raftcpp.dir/clean:
	cd /home/wsl/project/raftcpp/build/src && $(CMAKE_COMMAND) -P CMakeFiles/raftcpp.dir/cmake_clean.cmake
.PHONY : src/CMakeFiles/raftcpp.dir/clean

src/CMakeFiles/raftcpp.dir/depend:
	cd /home/wsl/project/raftcpp/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/wsl/project/raftcpp /home/wsl/project/raftcpp/src /home/wsl/project/raftcpp/build /home/wsl/project/raftcpp/build/src /home/wsl/project/raftcpp/build/src/CMakeFiles/raftcpp.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/CMakeFiles/raftcpp.dir/depend

