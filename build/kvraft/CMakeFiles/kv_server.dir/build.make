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
include kvraft/CMakeFiles/kv_server.dir/depend.make

# Include the progress variables for this target.
include kvraft/CMakeFiles/kv_server.dir/progress.make

# Include the compile flags for this target's objects.
include kvraft/CMakeFiles/kv_server.dir/flags.make

kvraft/CMakeFiles/kv_server.dir/kv_server.cc.o: kvraft/CMakeFiles/kv_server.dir/flags.make
kvraft/CMakeFiles/kv_server.dir/kv_server.cc.o: ../kvraft/kv_server.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/wsl/project/raftcpp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object kvraft/CMakeFiles/kv_server.dir/kv_server.cc.o"
	cd /home/wsl/project/raftcpp/build/kvraft && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/kv_server.dir/kv_server.cc.o -c /home/wsl/project/raftcpp/kvraft/kv_server.cc

kvraft/CMakeFiles/kv_server.dir/kv_server.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/kv_server.dir/kv_server.cc.i"
	cd /home/wsl/project/raftcpp/build/kvraft && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/wsl/project/raftcpp/kvraft/kv_server.cc > CMakeFiles/kv_server.dir/kv_server.cc.i

kvraft/CMakeFiles/kv_server.dir/kv_server.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/kv_server.dir/kv_server.cc.s"
	cd /home/wsl/project/raftcpp/build/kvraft && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/wsl/project/raftcpp/kvraft/kv_server.cc -o CMakeFiles/kv_server.dir/kv_server.cc.s

# Object files for target kv_server
kv_server_OBJECTS = \
"CMakeFiles/kv_server.dir/kv_server.cc.o"

# External object files for target kv_server
kv_server_EXTERNAL_OBJECTS =

bin/kv_server: kvraft/CMakeFiles/kv_server.dir/kv_server.cc.o
bin/kv_server: kvraft/CMakeFiles/kv_server.dir/build.make
bin/kv_server: lib/libraftcpp.a
bin/kv_server: third_party/reyao/lib/libreyao.so
bin/kv_server: /usr/local/lib/libprotobuf.so
bin/kv_server: /usr/lib/x86_64-linux-gnu/libz.so
bin/kv_server: kvraft/CMakeFiles/kv_server.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/wsl/project/raftcpp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../bin/kv_server"
	cd /home/wsl/project/raftcpp/build/kvraft && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/kv_server.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
kvraft/CMakeFiles/kv_server.dir/build: bin/kv_server

.PHONY : kvraft/CMakeFiles/kv_server.dir/build

kvraft/CMakeFiles/kv_server.dir/clean:
	cd /home/wsl/project/raftcpp/build/kvraft && $(CMAKE_COMMAND) -P CMakeFiles/kv_server.dir/cmake_clean.cmake
.PHONY : kvraft/CMakeFiles/kv_server.dir/clean

kvraft/CMakeFiles/kv_server.dir/depend:
	cd /home/wsl/project/raftcpp/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/wsl/project/raftcpp /home/wsl/project/raftcpp/kvraft /home/wsl/project/raftcpp/build /home/wsl/project/raftcpp/build/kvraft /home/wsl/project/raftcpp/build/kvraft/CMakeFiles/kv_server.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : kvraft/CMakeFiles/kv_server.dir/depend

