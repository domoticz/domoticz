# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.8

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
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/reivax/dev-domoticz

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/reivax/dev-domoticz

# Include any dependencies generated for this target.
include zip/CMakeFiles/minizip.dir/depend.make

# Include the progress variables for this target.
include zip/CMakeFiles/minizip.dir/progress.make

# Include the compile flags for this target's objects.
include zip/CMakeFiles/minizip.dir/flags.make

zip/CMakeFiles/minizip.dir/unzip.c.o: zip/CMakeFiles/minizip.dir/flags.make
zip/CMakeFiles/minizip.dir/unzip.c.o: zip/unzip.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/reivax/dev-domoticz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object zip/CMakeFiles/minizip.dir/unzip.c.o"
	cd /home/reivax/dev-domoticz/zip && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/minizip.dir/unzip.c.o   -c /home/reivax/dev-domoticz/zip/unzip.c

zip/CMakeFiles/minizip.dir/unzip.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/minizip.dir/unzip.c.i"
	cd /home/reivax/dev-domoticz/zip && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/reivax/dev-domoticz/zip/unzip.c > CMakeFiles/minizip.dir/unzip.c.i

zip/CMakeFiles/minizip.dir/unzip.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/minizip.dir/unzip.c.s"
	cd /home/reivax/dev-domoticz/zip && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/reivax/dev-domoticz/zip/unzip.c -o CMakeFiles/minizip.dir/unzip.c.s

zip/CMakeFiles/minizip.dir/unzip.c.o.requires:

.PHONY : zip/CMakeFiles/minizip.dir/unzip.c.o.requires

zip/CMakeFiles/minizip.dir/unzip.c.o.provides: zip/CMakeFiles/minizip.dir/unzip.c.o.requires
	$(MAKE) -f zip/CMakeFiles/minizip.dir/build.make zip/CMakeFiles/minizip.dir/unzip.c.o.provides.build
.PHONY : zip/CMakeFiles/minizip.dir/unzip.c.o.provides

zip/CMakeFiles/minizip.dir/unzip.c.o.provides.build: zip/CMakeFiles/minizip.dir/unzip.c.o


zip/CMakeFiles/minizip.dir/ioapi.c.o: zip/CMakeFiles/minizip.dir/flags.make
zip/CMakeFiles/minizip.dir/ioapi.c.o: zip/ioapi.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/reivax/dev-domoticz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object zip/CMakeFiles/minizip.dir/ioapi.c.o"
	cd /home/reivax/dev-domoticz/zip && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/minizip.dir/ioapi.c.o   -c /home/reivax/dev-domoticz/zip/ioapi.c

zip/CMakeFiles/minizip.dir/ioapi.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/minizip.dir/ioapi.c.i"
	cd /home/reivax/dev-domoticz/zip && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/reivax/dev-domoticz/zip/ioapi.c > CMakeFiles/minizip.dir/ioapi.c.i

zip/CMakeFiles/minizip.dir/ioapi.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/minizip.dir/ioapi.c.s"
	cd /home/reivax/dev-domoticz/zip && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/reivax/dev-domoticz/zip/ioapi.c -o CMakeFiles/minizip.dir/ioapi.c.s

zip/CMakeFiles/minizip.dir/ioapi.c.o.requires:

.PHONY : zip/CMakeFiles/minizip.dir/ioapi.c.o.requires

zip/CMakeFiles/minizip.dir/ioapi.c.o.provides: zip/CMakeFiles/minizip.dir/ioapi.c.o.requires
	$(MAKE) -f zip/CMakeFiles/minizip.dir/build.make zip/CMakeFiles/minizip.dir/ioapi.c.o.provides.build
.PHONY : zip/CMakeFiles/minizip.dir/ioapi.c.o.provides

zip/CMakeFiles/minizip.dir/ioapi.c.o.provides.build: zip/CMakeFiles/minizip.dir/ioapi.c.o


# Object files for target minizip
minizip_OBJECTS = \
"CMakeFiles/minizip.dir/unzip.c.o" \
"CMakeFiles/minizip.dir/ioapi.c.o"

# External object files for target minizip
minizip_EXTERNAL_OBJECTS =

zip/libminizip.a: zip/CMakeFiles/minizip.dir/unzip.c.o
zip/libminizip.a: zip/CMakeFiles/minizip.dir/ioapi.c.o
zip/libminizip.a: zip/CMakeFiles/minizip.dir/build.make
zip/libminizip.a: zip/CMakeFiles/minizip.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/reivax/dev-domoticz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking C static library libminizip.a"
	cd /home/reivax/dev-domoticz/zip && $(CMAKE_COMMAND) -P CMakeFiles/minizip.dir/cmake_clean_target.cmake
	cd /home/reivax/dev-domoticz/zip && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/minizip.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
zip/CMakeFiles/minizip.dir/build: zip/libminizip.a

.PHONY : zip/CMakeFiles/minizip.dir/build

zip/CMakeFiles/minizip.dir/requires: zip/CMakeFiles/minizip.dir/unzip.c.o.requires
zip/CMakeFiles/minizip.dir/requires: zip/CMakeFiles/minizip.dir/ioapi.c.o.requires

.PHONY : zip/CMakeFiles/minizip.dir/requires

zip/CMakeFiles/minizip.dir/clean:
	cd /home/reivax/dev-domoticz/zip && $(CMAKE_COMMAND) -P CMakeFiles/minizip.dir/cmake_clean.cmake
.PHONY : zip/CMakeFiles/minizip.dir/clean

zip/CMakeFiles/minizip.dir/depend:
	cd /home/reivax/dev-domoticz && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/reivax/dev-domoticz /home/reivax/dev-domoticz/zip /home/reivax/dev-domoticz /home/reivax/dev-domoticz/zip /home/reivax/dev-domoticz/zip/CMakeFiles/minizip.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : zip/CMakeFiles/minizip.dir/depend

