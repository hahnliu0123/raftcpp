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
include third_party/reyao/reyao/tests/CMakeFiles/asynclog_test.dir/depend.make

# Include the progress variables for this target.
include third_party/reyao/reyao/tests/CMakeFiles/asynclog_test.dir/progress.make

# Include the compile flags for this target's objects.
include third_party/reyao/reyao/tests/CMakeFiles/asynclog_test.dir/flags.make

third_party/reyao/reyao/tests/CMakeFiles/asynclog_test.dir/asynclog_test.cc.o: third_party/reyao/reyao/tests/CMakeFiles/asynclog_test.dir/flags.make
third_party/reyao/reyao/tests/CMakeFiles/asynclog_test.dir/asynclog_test.cc.o: ../third_party/reyao/reyao/tests/asynclog_test.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/wsl/project/raftcpp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object third_party/reyao/reyao/tests/CMakeFiles/asynclog_test.dir/asynclog_test.cc.o"
	cd /home/wsl/project/raftcpp/build/third_party/reyao/reyao/tests && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/asynclog_test.dir/asynclog_test.cc.o -c /home/wsl/project/raftcpp/third_party/reyao/reyao/tests/asynclog_test.cc

third_party/reyao/reyao/tests/CMakeFiles/asynclog_test.dir/asynclog_test.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/asynclog_test.dir/asynclog_test.cc.i"
	cd /home/wsl/project/raftcpp/build/third_party/reyao/reyao/tests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/wsl/project/raftcpp/third_party/reyao/reyao/tests/asynclog_test.cc > CMakeFiles/asynclog_test.dir/asynclog_test.cc.i

third_party/reyao/reyao/tests/CMakeFiles/asynclog_test.dir/asynclog_test.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/asynclog_test.dir/asynclog_test.cc.s"
	cd /home/wsl/project/raftcpp/build/third_party/reyao/reyao/tests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/wsl/project/raftcpp/third_party/reyao/reyao/tests/asynclog_test.cc -o CMakeFiles/asynclog_test.dir/asynclog_test.cc.s

# Object files for target asynclog_test
asynclog_test_OBJECTS = \
"CMakeFiles/asynclog_test.dir/asynclog_test.cc.o"

# External object files for target asynclog_test
asynclog_test_EXTERNAL_OBJECTS =

third_party/reyao/bin/asynclog_test: third_party/reyao/reyao/tests/CMakeFiles/asynclog_test.dir/asynclog_test.cc.o
third_party/reyao/bin/asynclog_test: third_party/reyao/reyao/tests/CMakeFiles/asynclog_test.dir/build.make
third_party/reyao/bin/asynclog_test: third_party/reyao/lib/libreyao.so
third_party/reyao/bin/asynclog_test: /usr/local/lib/libprotobuf.so
third_party/reyao/bin/asynclog_test: /usr/lib/x86_64-linux-gnu/libz.so
third_party/reyao/bin/asynclog_test: third_party/reyao/reyao/tests/CMakeFiles/asynclog_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/wsl/project/raftcpp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../../bin/asynclog_test"
	cd /home/wsl/project/raftcpp/build/third_party/reyao/reyao/tests && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/asynclog_test.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
third_party/reyao/reyao/tests/CMakeFiles/asynclog_test.dir/build: third_party/reyao/bin/asynclog_test

.PHONY : third_party/reyao/reyao/tests/CMakeFiles/asynclog_test.dir/build

third_party/reyao/reyao/tests/CMakeFiles/asynclog_test.dir/clean:
	cd /home/wsl/project/raftcpp/build/third_party/reyao/reyao/tests && $(CMAKE_COMMAND) -P CMakeFiles/asynclog_test.dir/cmake_clean.cmake
.PHONY : third_party/reyao/reyao/tests/CMakeFiles/asynclog_test.dir/clean

third_party/reyao/reyao/tests/CMakeFiles/asynclog_test.dir/depend:
	cd /home/wsl/project/raftcpp/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/wsl/project/raftcpp /home/wsl/project/raftcpp/third_party/reyao/reyao/tests /home/wsl/project/raftcpp/build /home/wsl/project/raftcpp/build/third_party/reyao/reyao/tests /home/wsl/project/raftcpp/build/third_party/reyao/reyao/tests/CMakeFiles/asynclog_test.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : third_party/reyao/reyao/tests/CMakeFiles/asynclog_test.dir/depend

