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
include MQTT/CMakeFiles/mqtt.dir/depend.make

# Include the progress variables for this target.
include MQTT/CMakeFiles/mqtt.dir/progress.make

# Include the compile flags for this target's objects.
include MQTT/CMakeFiles/mqtt.dir/flags.make

MQTT/CMakeFiles/mqtt.dir/mosquittopp.cpp.o: MQTT/CMakeFiles/mqtt.dir/flags.make
MQTT/CMakeFiles/mqtt.dir/mosquittopp.cpp.o: MQTT/mosquittopp.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/reivax/dev-domoticz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object MQTT/CMakeFiles/mqtt.dir/mosquittopp.cpp.o"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/mqtt.dir/mosquittopp.cpp.o -c /home/reivax/dev-domoticz/MQTT/mosquittopp.cpp

MQTT/CMakeFiles/mqtt.dir/mosquittopp.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/mqtt.dir/mosquittopp.cpp.i"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/reivax/dev-domoticz/MQTT/mosquittopp.cpp > CMakeFiles/mqtt.dir/mosquittopp.cpp.i

MQTT/CMakeFiles/mqtt.dir/mosquittopp.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/mqtt.dir/mosquittopp.cpp.s"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/reivax/dev-domoticz/MQTT/mosquittopp.cpp -o CMakeFiles/mqtt.dir/mosquittopp.cpp.s

MQTT/CMakeFiles/mqtt.dir/mosquittopp.cpp.o.requires:

.PHONY : MQTT/CMakeFiles/mqtt.dir/mosquittopp.cpp.o.requires

MQTT/CMakeFiles/mqtt.dir/mosquittopp.cpp.o.provides: MQTT/CMakeFiles/mqtt.dir/mosquittopp.cpp.o.requires
	$(MAKE) -f MQTT/CMakeFiles/mqtt.dir/build.make MQTT/CMakeFiles/mqtt.dir/mosquittopp.cpp.o.provides.build
.PHONY : MQTT/CMakeFiles/mqtt.dir/mosquittopp.cpp.o.provides

MQTT/CMakeFiles/mqtt.dir/mosquittopp.cpp.o.provides.build: MQTT/CMakeFiles/mqtt.dir/mosquittopp.cpp.o


MQTT/CMakeFiles/mqtt.dir/mosquitto.c.o: MQTT/CMakeFiles/mqtt.dir/flags.make
MQTT/CMakeFiles/mqtt.dir/mosquitto.c.o: MQTT/mosquitto.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/reivax/dev-domoticz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object MQTT/CMakeFiles/mqtt.dir/mosquitto.c.o"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mqtt.dir/mosquitto.c.o   -c /home/reivax/dev-domoticz/MQTT/mosquitto.c

MQTT/CMakeFiles/mqtt.dir/mosquitto.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mqtt.dir/mosquitto.c.i"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/reivax/dev-domoticz/MQTT/mosquitto.c > CMakeFiles/mqtt.dir/mosquitto.c.i

MQTT/CMakeFiles/mqtt.dir/mosquitto.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mqtt.dir/mosquitto.c.s"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/reivax/dev-domoticz/MQTT/mosquitto.c -o CMakeFiles/mqtt.dir/mosquitto.c.s

MQTT/CMakeFiles/mqtt.dir/mosquitto.c.o.requires:

.PHONY : MQTT/CMakeFiles/mqtt.dir/mosquitto.c.o.requires

MQTT/CMakeFiles/mqtt.dir/mosquitto.c.o.provides: MQTT/CMakeFiles/mqtt.dir/mosquitto.c.o.requires
	$(MAKE) -f MQTT/CMakeFiles/mqtt.dir/build.make MQTT/CMakeFiles/mqtt.dir/mosquitto.c.o.provides.build
.PHONY : MQTT/CMakeFiles/mqtt.dir/mosquitto.c.o.provides

MQTT/CMakeFiles/mqtt.dir/mosquitto.c.o.provides.build: MQTT/CMakeFiles/mqtt.dir/mosquitto.c.o


MQTT/CMakeFiles/mqtt.dir/logging_mosq.c.o: MQTT/CMakeFiles/mqtt.dir/flags.make
MQTT/CMakeFiles/mqtt.dir/logging_mosq.c.o: MQTT/logging_mosq.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/reivax/dev-domoticz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object MQTT/CMakeFiles/mqtt.dir/logging_mosq.c.o"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mqtt.dir/logging_mosq.c.o   -c /home/reivax/dev-domoticz/MQTT/logging_mosq.c

MQTT/CMakeFiles/mqtt.dir/logging_mosq.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mqtt.dir/logging_mosq.c.i"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/reivax/dev-domoticz/MQTT/logging_mosq.c > CMakeFiles/mqtt.dir/logging_mosq.c.i

MQTT/CMakeFiles/mqtt.dir/logging_mosq.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mqtt.dir/logging_mosq.c.s"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/reivax/dev-domoticz/MQTT/logging_mosq.c -o CMakeFiles/mqtt.dir/logging_mosq.c.s

MQTT/CMakeFiles/mqtt.dir/logging_mosq.c.o.requires:

.PHONY : MQTT/CMakeFiles/mqtt.dir/logging_mosq.c.o.requires

MQTT/CMakeFiles/mqtt.dir/logging_mosq.c.o.provides: MQTT/CMakeFiles/mqtt.dir/logging_mosq.c.o.requires
	$(MAKE) -f MQTT/CMakeFiles/mqtt.dir/build.make MQTT/CMakeFiles/mqtt.dir/logging_mosq.c.o.provides.build
.PHONY : MQTT/CMakeFiles/mqtt.dir/logging_mosq.c.o.provides

MQTT/CMakeFiles/mqtt.dir/logging_mosq.c.o.provides.build: MQTT/CMakeFiles/mqtt.dir/logging_mosq.c.o


MQTT/CMakeFiles/mqtt.dir/memory_mosq.c.o: MQTT/CMakeFiles/mqtt.dir/flags.make
MQTT/CMakeFiles/mqtt.dir/memory_mosq.c.o: MQTT/memory_mosq.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/reivax/dev-domoticz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object MQTT/CMakeFiles/mqtt.dir/memory_mosq.c.o"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mqtt.dir/memory_mosq.c.o   -c /home/reivax/dev-domoticz/MQTT/memory_mosq.c

MQTT/CMakeFiles/mqtt.dir/memory_mosq.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mqtt.dir/memory_mosq.c.i"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/reivax/dev-domoticz/MQTT/memory_mosq.c > CMakeFiles/mqtt.dir/memory_mosq.c.i

MQTT/CMakeFiles/mqtt.dir/memory_mosq.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mqtt.dir/memory_mosq.c.s"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/reivax/dev-domoticz/MQTT/memory_mosq.c -o CMakeFiles/mqtt.dir/memory_mosq.c.s

MQTT/CMakeFiles/mqtt.dir/memory_mosq.c.o.requires:

.PHONY : MQTT/CMakeFiles/mqtt.dir/memory_mosq.c.o.requires

MQTT/CMakeFiles/mqtt.dir/memory_mosq.c.o.provides: MQTT/CMakeFiles/mqtt.dir/memory_mosq.c.o.requires
	$(MAKE) -f MQTT/CMakeFiles/mqtt.dir/build.make MQTT/CMakeFiles/mqtt.dir/memory_mosq.c.o.provides.build
.PHONY : MQTT/CMakeFiles/mqtt.dir/memory_mosq.c.o.provides

MQTT/CMakeFiles/mqtt.dir/memory_mosq.c.o.provides.build: MQTT/CMakeFiles/mqtt.dir/memory_mosq.c.o


MQTT/CMakeFiles/mqtt.dir/messages_mosq.c.o: MQTT/CMakeFiles/mqtt.dir/flags.make
MQTT/CMakeFiles/mqtt.dir/messages_mosq.c.o: MQTT/messages_mosq.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/reivax/dev-domoticz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object MQTT/CMakeFiles/mqtt.dir/messages_mosq.c.o"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mqtt.dir/messages_mosq.c.o   -c /home/reivax/dev-domoticz/MQTT/messages_mosq.c

MQTT/CMakeFiles/mqtt.dir/messages_mosq.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mqtt.dir/messages_mosq.c.i"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/reivax/dev-domoticz/MQTT/messages_mosq.c > CMakeFiles/mqtt.dir/messages_mosq.c.i

MQTT/CMakeFiles/mqtt.dir/messages_mosq.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mqtt.dir/messages_mosq.c.s"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/reivax/dev-domoticz/MQTT/messages_mosq.c -o CMakeFiles/mqtt.dir/messages_mosq.c.s

MQTT/CMakeFiles/mqtt.dir/messages_mosq.c.o.requires:

.PHONY : MQTT/CMakeFiles/mqtt.dir/messages_mosq.c.o.requires

MQTT/CMakeFiles/mqtt.dir/messages_mosq.c.o.provides: MQTT/CMakeFiles/mqtt.dir/messages_mosq.c.o.requires
	$(MAKE) -f MQTT/CMakeFiles/mqtt.dir/build.make MQTT/CMakeFiles/mqtt.dir/messages_mosq.c.o.provides.build
.PHONY : MQTT/CMakeFiles/mqtt.dir/messages_mosq.c.o.provides

MQTT/CMakeFiles/mqtt.dir/messages_mosq.c.o.provides.build: MQTT/CMakeFiles/mqtt.dir/messages_mosq.c.o


MQTT/CMakeFiles/mqtt.dir/net_mosq.c.o: MQTT/CMakeFiles/mqtt.dir/flags.make
MQTT/CMakeFiles/mqtt.dir/net_mosq.c.o: MQTT/net_mosq.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/reivax/dev-domoticz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object MQTT/CMakeFiles/mqtt.dir/net_mosq.c.o"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mqtt.dir/net_mosq.c.o   -c /home/reivax/dev-domoticz/MQTT/net_mosq.c

MQTT/CMakeFiles/mqtt.dir/net_mosq.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mqtt.dir/net_mosq.c.i"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/reivax/dev-domoticz/MQTT/net_mosq.c > CMakeFiles/mqtt.dir/net_mosq.c.i

MQTT/CMakeFiles/mqtt.dir/net_mosq.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mqtt.dir/net_mosq.c.s"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/reivax/dev-domoticz/MQTT/net_mosq.c -o CMakeFiles/mqtt.dir/net_mosq.c.s

MQTT/CMakeFiles/mqtt.dir/net_mosq.c.o.requires:

.PHONY : MQTT/CMakeFiles/mqtt.dir/net_mosq.c.o.requires

MQTT/CMakeFiles/mqtt.dir/net_mosq.c.o.provides: MQTT/CMakeFiles/mqtt.dir/net_mosq.c.o.requires
	$(MAKE) -f MQTT/CMakeFiles/mqtt.dir/build.make MQTT/CMakeFiles/mqtt.dir/net_mosq.c.o.provides.build
.PHONY : MQTT/CMakeFiles/mqtt.dir/net_mosq.c.o.provides

MQTT/CMakeFiles/mqtt.dir/net_mosq.c.o.provides.build: MQTT/CMakeFiles/mqtt.dir/net_mosq.c.o


MQTT/CMakeFiles/mqtt.dir/read_handle.c.o: MQTT/CMakeFiles/mqtt.dir/flags.make
MQTT/CMakeFiles/mqtt.dir/read_handle.c.o: MQTT/read_handle.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/reivax/dev-domoticz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building C object MQTT/CMakeFiles/mqtt.dir/read_handle.c.o"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mqtt.dir/read_handle.c.o   -c /home/reivax/dev-domoticz/MQTT/read_handle.c

MQTT/CMakeFiles/mqtt.dir/read_handle.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mqtt.dir/read_handle.c.i"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/reivax/dev-domoticz/MQTT/read_handle.c > CMakeFiles/mqtt.dir/read_handle.c.i

MQTT/CMakeFiles/mqtt.dir/read_handle.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mqtt.dir/read_handle.c.s"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/reivax/dev-domoticz/MQTT/read_handle.c -o CMakeFiles/mqtt.dir/read_handle.c.s

MQTT/CMakeFiles/mqtt.dir/read_handle.c.o.requires:

.PHONY : MQTT/CMakeFiles/mqtt.dir/read_handle.c.o.requires

MQTT/CMakeFiles/mqtt.dir/read_handle.c.o.provides: MQTT/CMakeFiles/mqtt.dir/read_handle.c.o.requires
	$(MAKE) -f MQTT/CMakeFiles/mqtt.dir/build.make MQTT/CMakeFiles/mqtt.dir/read_handle.c.o.provides.build
.PHONY : MQTT/CMakeFiles/mqtt.dir/read_handle.c.o.provides

MQTT/CMakeFiles/mqtt.dir/read_handle.c.o.provides.build: MQTT/CMakeFiles/mqtt.dir/read_handle.c.o


MQTT/CMakeFiles/mqtt.dir/read_handle_client.c.o: MQTT/CMakeFiles/mqtt.dir/flags.make
MQTT/CMakeFiles/mqtt.dir/read_handle_client.c.o: MQTT/read_handle_client.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/reivax/dev-domoticz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building C object MQTT/CMakeFiles/mqtt.dir/read_handle_client.c.o"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mqtt.dir/read_handle_client.c.o   -c /home/reivax/dev-domoticz/MQTT/read_handle_client.c

MQTT/CMakeFiles/mqtt.dir/read_handle_client.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mqtt.dir/read_handle_client.c.i"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/reivax/dev-domoticz/MQTT/read_handle_client.c > CMakeFiles/mqtt.dir/read_handle_client.c.i

MQTT/CMakeFiles/mqtt.dir/read_handle_client.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mqtt.dir/read_handle_client.c.s"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/reivax/dev-domoticz/MQTT/read_handle_client.c -o CMakeFiles/mqtt.dir/read_handle_client.c.s

MQTT/CMakeFiles/mqtt.dir/read_handle_client.c.o.requires:

.PHONY : MQTT/CMakeFiles/mqtt.dir/read_handle_client.c.o.requires

MQTT/CMakeFiles/mqtt.dir/read_handle_client.c.o.provides: MQTT/CMakeFiles/mqtt.dir/read_handle_client.c.o.requires
	$(MAKE) -f MQTT/CMakeFiles/mqtt.dir/build.make MQTT/CMakeFiles/mqtt.dir/read_handle_client.c.o.provides.build
.PHONY : MQTT/CMakeFiles/mqtt.dir/read_handle_client.c.o.provides

MQTT/CMakeFiles/mqtt.dir/read_handle_client.c.o.provides.build: MQTT/CMakeFiles/mqtt.dir/read_handle_client.c.o


MQTT/CMakeFiles/mqtt.dir/read_handle_shared.c.o: MQTT/CMakeFiles/mqtt.dir/flags.make
MQTT/CMakeFiles/mqtt.dir/read_handle_shared.c.o: MQTT/read_handle_shared.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/reivax/dev-domoticz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Building C object MQTT/CMakeFiles/mqtt.dir/read_handle_shared.c.o"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mqtt.dir/read_handle_shared.c.o   -c /home/reivax/dev-domoticz/MQTT/read_handle_shared.c

MQTT/CMakeFiles/mqtt.dir/read_handle_shared.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mqtt.dir/read_handle_shared.c.i"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/reivax/dev-domoticz/MQTT/read_handle_shared.c > CMakeFiles/mqtt.dir/read_handle_shared.c.i

MQTT/CMakeFiles/mqtt.dir/read_handle_shared.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mqtt.dir/read_handle_shared.c.s"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/reivax/dev-domoticz/MQTT/read_handle_shared.c -o CMakeFiles/mqtt.dir/read_handle_shared.c.s

MQTT/CMakeFiles/mqtt.dir/read_handle_shared.c.o.requires:

.PHONY : MQTT/CMakeFiles/mqtt.dir/read_handle_shared.c.o.requires

MQTT/CMakeFiles/mqtt.dir/read_handle_shared.c.o.provides: MQTT/CMakeFiles/mqtt.dir/read_handle_shared.c.o.requires
	$(MAKE) -f MQTT/CMakeFiles/mqtt.dir/build.make MQTT/CMakeFiles/mqtt.dir/read_handle_shared.c.o.provides.build
.PHONY : MQTT/CMakeFiles/mqtt.dir/read_handle_shared.c.o.provides

MQTT/CMakeFiles/mqtt.dir/read_handle_shared.c.o.provides.build: MQTT/CMakeFiles/mqtt.dir/read_handle_shared.c.o


MQTT/CMakeFiles/mqtt.dir/send_client_mosq.c.o: MQTT/CMakeFiles/mqtt.dir/flags.make
MQTT/CMakeFiles/mqtt.dir/send_client_mosq.c.o: MQTT/send_client_mosq.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/reivax/dev-domoticz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_10) "Building C object MQTT/CMakeFiles/mqtt.dir/send_client_mosq.c.o"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mqtt.dir/send_client_mosq.c.o   -c /home/reivax/dev-domoticz/MQTT/send_client_mosq.c

MQTT/CMakeFiles/mqtt.dir/send_client_mosq.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mqtt.dir/send_client_mosq.c.i"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/reivax/dev-domoticz/MQTT/send_client_mosq.c > CMakeFiles/mqtt.dir/send_client_mosq.c.i

MQTT/CMakeFiles/mqtt.dir/send_client_mosq.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mqtt.dir/send_client_mosq.c.s"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/reivax/dev-domoticz/MQTT/send_client_mosq.c -o CMakeFiles/mqtt.dir/send_client_mosq.c.s

MQTT/CMakeFiles/mqtt.dir/send_client_mosq.c.o.requires:

.PHONY : MQTT/CMakeFiles/mqtt.dir/send_client_mosq.c.o.requires

MQTT/CMakeFiles/mqtt.dir/send_client_mosq.c.o.provides: MQTT/CMakeFiles/mqtt.dir/send_client_mosq.c.o.requires
	$(MAKE) -f MQTT/CMakeFiles/mqtt.dir/build.make MQTT/CMakeFiles/mqtt.dir/send_client_mosq.c.o.provides.build
.PHONY : MQTT/CMakeFiles/mqtt.dir/send_client_mosq.c.o.provides

MQTT/CMakeFiles/mqtt.dir/send_client_mosq.c.o.provides.build: MQTT/CMakeFiles/mqtt.dir/send_client_mosq.c.o


MQTT/CMakeFiles/mqtt.dir/send_mosq.c.o: MQTT/CMakeFiles/mqtt.dir/flags.make
MQTT/CMakeFiles/mqtt.dir/send_mosq.c.o: MQTT/send_mosq.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/reivax/dev-domoticz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_11) "Building C object MQTT/CMakeFiles/mqtt.dir/send_mosq.c.o"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mqtt.dir/send_mosq.c.o   -c /home/reivax/dev-domoticz/MQTT/send_mosq.c

MQTT/CMakeFiles/mqtt.dir/send_mosq.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mqtt.dir/send_mosq.c.i"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/reivax/dev-domoticz/MQTT/send_mosq.c > CMakeFiles/mqtt.dir/send_mosq.c.i

MQTT/CMakeFiles/mqtt.dir/send_mosq.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mqtt.dir/send_mosq.c.s"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/reivax/dev-domoticz/MQTT/send_mosq.c -o CMakeFiles/mqtt.dir/send_mosq.c.s

MQTT/CMakeFiles/mqtt.dir/send_mosq.c.o.requires:

.PHONY : MQTT/CMakeFiles/mqtt.dir/send_mosq.c.o.requires

MQTT/CMakeFiles/mqtt.dir/send_mosq.c.o.provides: MQTT/CMakeFiles/mqtt.dir/send_mosq.c.o.requires
	$(MAKE) -f MQTT/CMakeFiles/mqtt.dir/build.make MQTT/CMakeFiles/mqtt.dir/send_mosq.c.o.provides.build
.PHONY : MQTT/CMakeFiles/mqtt.dir/send_mosq.c.o.provides

MQTT/CMakeFiles/mqtt.dir/send_mosq.c.o.provides.build: MQTT/CMakeFiles/mqtt.dir/send_mosq.c.o


MQTT/CMakeFiles/mqtt.dir/socks_mosq.c.o: MQTT/CMakeFiles/mqtt.dir/flags.make
MQTT/CMakeFiles/mqtt.dir/socks_mosq.c.o: MQTT/socks_mosq.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/reivax/dev-domoticz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_12) "Building C object MQTT/CMakeFiles/mqtt.dir/socks_mosq.c.o"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mqtt.dir/socks_mosq.c.o   -c /home/reivax/dev-domoticz/MQTT/socks_mosq.c

MQTT/CMakeFiles/mqtt.dir/socks_mosq.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mqtt.dir/socks_mosq.c.i"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/reivax/dev-domoticz/MQTT/socks_mosq.c > CMakeFiles/mqtt.dir/socks_mosq.c.i

MQTT/CMakeFiles/mqtt.dir/socks_mosq.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mqtt.dir/socks_mosq.c.s"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/reivax/dev-domoticz/MQTT/socks_mosq.c -o CMakeFiles/mqtt.dir/socks_mosq.c.s

MQTT/CMakeFiles/mqtt.dir/socks_mosq.c.o.requires:

.PHONY : MQTT/CMakeFiles/mqtt.dir/socks_mosq.c.o.requires

MQTT/CMakeFiles/mqtt.dir/socks_mosq.c.o.provides: MQTT/CMakeFiles/mqtt.dir/socks_mosq.c.o.requires
	$(MAKE) -f MQTT/CMakeFiles/mqtt.dir/build.make MQTT/CMakeFiles/mqtt.dir/socks_mosq.c.o.provides.build
.PHONY : MQTT/CMakeFiles/mqtt.dir/socks_mosq.c.o.provides

MQTT/CMakeFiles/mqtt.dir/socks_mosq.c.o.provides.build: MQTT/CMakeFiles/mqtt.dir/socks_mosq.c.o


MQTT/CMakeFiles/mqtt.dir/srv_mosq.c.o: MQTT/CMakeFiles/mqtt.dir/flags.make
MQTT/CMakeFiles/mqtt.dir/srv_mosq.c.o: MQTT/srv_mosq.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/reivax/dev-domoticz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_13) "Building C object MQTT/CMakeFiles/mqtt.dir/srv_mosq.c.o"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mqtt.dir/srv_mosq.c.o   -c /home/reivax/dev-domoticz/MQTT/srv_mosq.c

MQTT/CMakeFiles/mqtt.dir/srv_mosq.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mqtt.dir/srv_mosq.c.i"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/reivax/dev-domoticz/MQTT/srv_mosq.c > CMakeFiles/mqtt.dir/srv_mosq.c.i

MQTT/CMakeFiles/mqtt.dir/srv_mosq.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mqtt.dir/srv_mosq.c.s"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/reivax/dev-domoticz/MQTT/srv_mosq.c -o CMakeFiles/mqtt.dir/srv_mosq.c.s

MQTT/CMakeFiles/mqtt.dir/srv_mosq.c.o.requires:

.PHONY : MQTT/CMakeFiles/mqtt.dir/srv_mosq.c.o.requires

MQTT/CMakeFiles/mqtt.dir/srv_mosq.c.o.provides: MQTT/CMakeFiles/mqtt.dir/srv_mosq.c.o.requires
	$(MAKE) -f MQTT/CMakeFiles/mqtt.dir/build.make MQTT/CMakeFiles/mqtt.dir/srv_mosq.c.o.provides.build
.PHONY : MQTT/CMakeFiles/mqtt.dir/srv_mosq.c.o.provides

MQTT/CMakeFiles/mqtt.dir/srv_mosq.c.o.provides.build: MQTT/CMakeFiles/mqtt.dir/srv_mosq.c.o


MQTT/CMakeFiles/mqtt.dir/thread_mosq.c.o: MQTT/CMakeFiles/mqtt.dir/flags.make
MQTT/CMakeFiles/mqtt.dir/thread_mosq.c.o: MQTT/thread_mosq.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/reivax/dev-domoticz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_14) "Building C object MQTT/CMakeFiles/mqtt.dir/thread_mosq.c.o"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mqtt.dir/thread_mosq.c.o   -c /home/reivax/dev-domoticz/MQTT/thread_mosq.c

MQTT/CMakeFiles/mqtt.dir/thread_mosq.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mqtt.dir/thread_mosq.c.i"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/reivax/dev-domoticz/MQTT/thread_mosq.c > CMakeFiles/mqtt.dir/thread_mosq.c.i

MQTT/CMakeFiles/mqtt.dir/thread_mosq.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mqtt.dir/thread_mosq.c.s"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/reivax/dev-domoticz/MQTT/thread_mosq.c -o CMakeFiles/mqtt.dir/thread_mosq.c.s

MQTT/CMakeFiles/mqtt.dir/thread_mosq.c.o.requires:

.PHONY : MQTT/CMakeFiles/mqtt.dir/thread_mosq.c.o.requires

MQTT/CMakeFiles/mqtt.dir/thread_mosq.c.o.provides: MQTT/CMakeFiles/mqtt.dir/thread_mosq.c.o.requires
	$(MAKE) -f MQTT/CMakeFiles/mqtt.dir/build.make MQTT/CMakeFiles/mqtt.dir/thread_mosq.c.o.provides.build
.PHONY : MQTT/CMakeFiles/mqtt.dir/thread_mosq.c.o.provides

MQTT/CMakeFiles/mqtt.dir/thread_mosq.c.o.provides.build: MQTT/CMakeFiles/mqtt.dir/thread_mosq.c.o


MQTT/CMakeFiles/mqtt.dir/time_mosq.c.o: MQTT/CMakeFiles/mqtt.dir/flags.make
MQTT/CMakeFiles/mqtt.dir/time_mosq.c.o: MQTT/time_mosq.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/reivax/dev-domoticz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_15) "Building C object MQTT/CMakeFiles/mqtt.dir/time_mosq.c.o"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mqtt.dir/time_mosq.c.o   -c /home/reivax/dev-domoticz/MQTT/time_mosq.c

MQTT/CMakeFiles/mqtt.dir/time_mosq.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mqtt.dir/time_mosq.c.i"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/reivax/dev-domoticz/MQTT/time_mosq.c > CMakeFiles/mqtt.dir/time_mosq.c.i

MQTT/CMakeFiles/mqtt.dir/time_mosq.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mqtt.dir/time_mosq.c.s"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/reivax/dev-domoticz/MQTT/time_mosq.c -o CMakeFiles/mqtt.dir/time_mosq.c.s

MQTT/CMakeFiles/mqtt.dir/time_mosq.c.o.requires:

.PHONY : MQTT/CMakeFiles/mqtt.dir/time_mosq.c.o.requires

MQTT/CMakeFiles/mqtt.dir/time_mosq.c.o.provides: MQTT/CMakeFiles/mqtt.dir/time_mosq.c.o.requires
	$(MAKE) -f MQTT/CMakeFiles/mqtt.dir/build.make MQTT/CMakeFiles/mqtt.dir/time_mosq.c.o.provides.build
.PHONY : MQTT/CMakeFiles/mqtt.dir/time_mosq.c.o.provides

MQTT/CMakeFiles/mqtt.dir/time_mosq.c.o.provides.build: MQTT/CMakeFiles/mqtt.dir/time_mosq.c.o


MQTT/CMakeFiles/mqtt.dir/tls_mosq.c.o: MQTT/CMakeFiles/mqtt.dir/flags.make
MQTT/CMakeFiles/mqtt.dir/tls_mosq.c.o: MQTT/tls_mosq.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/reivax/dev-domoticz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_16) "Building C object MQTT/CMakeFiles/mqtt.dir/tls_mosq.c.o"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mqtt.dir/tls_mosq.c.o   -c /home/reivax/dev-domoticz/MQTT/tls_mosq.c

MQTT/CMakeFiles/mqtt.dir/tls_mosq.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mqtt.dir/tls_mosq.c.i"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/reivax/dev-domoticz/MQTT/tls_mosq.c > CMakeFiles/mqtt.dir/tls_mosq.c.i

MQTT/CMakeFiles/mqtt.dir/tls_mosq.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mqtt.dir/tls_mosq.c.s"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/reivax/dev-domoticz/MQTT/tls_mosq.c -o CMakeFiles/mqtt.dir/tls_mosq.c.s

MQTT/CMakeFiles/mqtt.dir/tls_mosq.c.o.requires:

.PHONY : MQTT/CMakeFiles/mqtt.dir/tls_mosq.c.o.requires

MQTT/CMakeFiles/mqtt.dir/tls_mosq.c.o.provides: MQTT/CMakeFiles/mqtt.dir/tls_mosq.c.o.requires
	$(MAKE) -f MQTT/CMakeFiles/mqtt.dir/build.make MQTT/CMakeFiles/mqtt.dir/tls_mosq.c.o.provides.build
.PHONY : MQTT/CMakeFiles/mqtt.dir/tls_mosq.c.o.provides

MQTT/CMakeFiles/mqtt.dir/tls_mosq.c.o.provides.build: MQTT/CMakeFiles/mqtt.dir/tls_mosq.c.o


MQTT/CMakeFiles/mqtt.dir/util_mosq.c.o: MQTT/CMakeFiles/mqtt.dir/flags.make
MQTT/CMakeFiles/mqtt.dir/util_mosq.c.o: MQTT/util_mosq.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/reivax/dev-domoticz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_17) "Building C object MQTT/CMakeFiles/mqtt.dir/util_mosq.c.o"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mqtt.dir/util_mosq.c.o   -c /home/reivax/dev-domoticz/MQTT/util_mosq.c

MQTT/CMakeFiles/mqtt.dir/util_mosq.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mqtt.dir/util_mosq.c.i"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/reivax/dev-domoticz/MQTT/util_mosq.c > CMakeFiles/mqtt.dir/util_mosq.c.i

MQTT/CMakeFiles/mqtt.dir/util_mosq.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mqtt.dir/util_mosq.c.s"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/reivax/dev-domoticz/MQTT/util_mosq.c -o CMakeFiles/mqtt.dir/util_mosq.c.s

MQTT/CMakeFiles/mqtt.dir/util_mosq.c.o.requires:

.PHONY : MQTT/CMakeFiles/mqtt.dir/util_mosq.c.o.requires

MQTT/CMakeFiles/mqtt.dir/util_mosq.c.o.provides: MQTT/CMakeFiles/mqtt.dir/util_mosq.c.o.requires
	$(MAKE) -f MQTT/CMakeFiles/mqtt.dir/build.make MQTT/CMakeFiles/mqtt.dir/util_mosq.c.o.provides.build
.PHONY : MQTT/CMakeFiles/mqtt.dir/util_mosq.c.o.provides

MQTT/CMakeFiles/mqtt.dir/util_mosq.c.o.provides.build: MQTT/CMakeFiles/mqtt.dir/util_mosq.c.o


MQTT/CMakeFiles/mqtt.dir/will_mosq.c.o: MQTT/CMakeFiles/mqtt.dir/flags.make
MQTT/CMakeFiles/mqtt.dir/will_mosq.c.o: MQTT/will_mosq.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/reivax/dev-domoticz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_18) "Building C object MQTT/CMakeFiles/mqtt.dir/will_mosq.c.o"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mqtt.dir/will_mosq.c.o   -c /home/reivax/dev-domoticz/MQTT/will_mosq.c

MQTT/CMakeFiles/mqtt.dir/will_mosq.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mqtt.dir/will_mosq.c.i"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/reivax/dev-domoticz/MQTT/will_mosq.c > CMakeFiles/mqtt.dir/will_mosq.c.i

MQTT/CMakeFiles/mqtt.dir/will_mosq.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mqtt.dir/will_mosq.c.s"
	cd /home/reivax/dev-domoticz/MQTT && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/reivax/dev-domoticz/MQTT/will_mosq.c -o CMakeFiles/mqtt.dir/will_mosq.c.s

MQTT/CMakeFiles/mqtt.dir/will_mosq.c.o.requires:

.PHONY : MQTT/CMakeFiles/mqtt.dir/will_mosq.c.o.requires

MQTT/CMakeFiles/mqtt.dir/will_mosq.c.o.provides: MQTT/CMakeFiles/mqtt.dir/will_mosq.c.o.requires
	$(MAKE) -f MQTT/CMakeFiles/mqtt.dir/build.make MQTT/CMakeFiles/mqtt.dir/will_mosq.c.o.provides.build
.PHONY : MQTT/CMakeFiles/mqtt.dir/will_mosq.c.o.provides

MQTT/CMakeFiles/mqtt.dir/will_mosq.c.o.provides.build: MQTT/CMakeFiles/mqtt.dir/will_mosq.c.o


# Object files for target mqtt
mqtt_OBJECTS = \
"CMakeFiles/mqtt.dir/mosquittopp.cpp.o" \
"CMakeFiles/mqtt.dir/mosquitto.c.o" \
"CMakeFiles/mqtt.dir/logging_mosq.c.o" \
"CMakeFiles/mqtt.dir/memory_mosq.c.o" \
"CMakeFiles/mqtt.dir/messages_mosq.c.o" \
"CMakeFiles/mqtt.dir/net_mosq.c.o" \
"CMakeFiles/mqtt.dir/read_handle.c.o" \
"CMakeFiles/mqtt.dir/read_handle_client.c.o" \
"CMakeFiles/mqtt.dir/read_handle_shared.c.o" \
"CMakeFiles/mqtt.dir/send_client_mosq.c.o" \
"CMakeFiles/mqtt.dir/send_mosq.c.o" \
"CMakeFiles/mqtt.dir/socks_mosq.c.o" \
"CMakeFiles/mqtt.dir/srv_mosq.c.o" \
"CMakeFiles/mqtt.dir/thread_mosq.c.o" \
"CMakeFiles/mqtt.dir/time_mosq.c.o" \
"CMakeFiles/mqtt.dir/tls_mosq.c.o" \
"CMakeFiles/mqtt.dir/util_mosq.c.o" \
"CMakeFiles/mqtt.dir/will_mosq.c.o"

# External object files for target mqtt
mqtt_EXTERNAL_OBJECTS =

MQTT/libmqtt.a: MQTT/CMakeFiles/mqtt.dir/mosquittopp.cpp.o
MQTT/libmqtt.a: MQTT/CMakeFiles/mqtt.dir/mosquitto.c.o
MQTT/libmqtt.a: MQTT/CMakeFiles/mqtt.dir/logging_mosq.c.o
MQTT/libmqtt.a: MQTT/CMakeFiles/mqtt.dir/memory_mosq.c.o
MQTT/libmqtt.a: MQTT/CMakeFiles/mqtt.dir/messages_mosq.c.o
MQTT/libmqtt.a: MQTT/CMakeFiles/mqtt.dir/net_mosq.c.o
MQTT/libmqtt.a: MQTT/CMakeFiles/mqtt.dir/read_handle.c.o
MQTT/libmqtt.a: MQTT/CMakeFiles/mqtt.dir/read_handle_client.c.o
MQTT/libmqtt.a: MQTT/CMakeFiles/mqtt.dir/read_handle_shared.c.o
MQTT/libmqtt.a: MQTT/CMakeFiles/mqtt.dir/send_client_mosq.c.o
MQTT/libmqtt.a: MQTT/CMakeFiles/mqtt.dir/send_mosq.c.o
MQTT/libmqtt.a: MQTT/CMakeFiles/mqtt.dir/socks_mosq.c.o
MQTT/libmqtt.a: MQTT/CMakeFiles/mqtt.dir/srv_mosq.c.o
MQTT/libmqtt.a: MQTT/CMakeFiles/mqtt.dir/thread_mosq.c.o
MQTT/libmqtt.a: MQTT/CMakeFiles/mqtt.dir/time_mosq.c.o
MQTT/libmqtt.a: MQTT/CMakeFiles/mqtt.dir/tls_mosq.c.o
MQTT/libmqtt.a: MQTT/CMakeFiles/mqtt.dir/util_mosq.c.o
MQTT/libmqtt.a: MQTT/CMakeFiles/mqtt.dir/will_mosq.c.o
MQTT/libmqtt.a: MQTT/CMakeFiles/mqtt.dir/build.make
MQTT/libmqtt.a: MQTT/CMakeFiles/mqtt.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/reivax/dev-domoticz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_19) "Linking CXX static library libmqtt.a"
	cd /home/reivax/dev-domoticz/MQTT && $(CMAKE_COMMAND) -P CMakeFiles/mqtt.dir/cmake_clean_target.cmake
	cd /home/reivax/dev-domoticz/MQTT && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/mqtt.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
MQTT/CMakeFiles/mqtt.dir/build: MQTT/libmqtt.a

.PHONY : MQTT/CMakeFiles/mqtt.dir/build

MQTT/CMakeFiles/mqtt.dir/requires: MQTT/CMakeFiles/mqtt.dir/mosquittopp.cpp.o.requires
MQTT/CMakeFiles/mqtt.dir/requires: MQTT/CMakeFiles/mqtt.dir/mosquitto.c.o.requires
MQTT/CMakeFiles/mqtt.dir/requires: MQTT/CMakeFiles/mqtt.dir/logging_mosq.c.o.requires
MQTT/CMakeFiles/mqtt.dir/requires: MQTT/CMakeFiles/mqtt.dir/memory_mosq.c.o.requires
MQTT/CMakeFiles/mqtt.dir/requires: MQTT/CMakeFiles/mqtt.dir/messages_mosq.c.o.requires
MQTT/CMakeFiles/mqtt.dir/requires: MQTT/CMakeFiles/mqtt.dir/net_mosq.c.o.requires
MQTT/CMakeFiles/mqtt.dir/requires: MQTT/CMakeFiles/mqtt.dir/read_handle.c.o.requires
MQTT/CMakeFiles/mqtt.dir/requires: MQTT/CMakeFiles/mqtt.dir/read_handle_client.c.o.requires
MQTT/CMakeFiles/mqtt.dir/requires: MQTT/CMakeFiles/mqtt.dir/read_handle_shared.c.o.requires
MQTT/CMakeFiles/mqtt.dir/requires: MQTT/CMakeFiles/mqtt.dir/send_client_mosq.c.o.requires
MQTT/CMakeFiles/mqtt.dir/requires: MQTT/CMakeFiles/mqtt.dir/send_mosq.c.o.requires
MQTT/CMakeFiles/mqtt.dir/requires: MQTT/CMakeFiles/mqtt.dir/socks_mosq.c.o.requires
MQTT/CMakeFiles/mqtt.dir/requires: MQTT/CMakeFiles/mqtt.dir/srv_mosq.c.o.requires
MQTT/CMakeFiles/mqtt.dir/requires: MQTT/CMakeFiles/mqtt.dir/thread_mosq.c.o.requires
MQTT/CMakeFiles/mqtt.dir/requires: MQTT/CMakeFiles/mqtt.dir/time_mosq.c.o.requires
MQTT/CMakeFiles/mqtt.dir/requires: MQTT/CMakeFiles/mqtt.dir/tls_mosq.c.o.requires
MQTT/CMakeFiles/mqtt.dir/requires: MQTT/CMakeFiles/mqtt.dir/util_mosq.c.o.requires
MQTT/CMakeFiles/mqtt.dir/requires: MQTT/CMakeFiles/mqtt.dir/will_mosq.c.o.requires

.PHONY : MQTT/CMakeFiles/mqtt.dir/requires

MQTT/CMakeFiles/mqtt.dir/clean:
	cd /home/reivax/dev-domoticz/MQTT && $(CMAKE_COMMAND) -P CMakeFiles/mqtt.dir/cmake_clean.cmake
.PHONY : MQTT/CMakeFiles/mqtt.dir/clean

MQTT/CMakeFiles/mqtt.dir/depend:
	cd /home/reivax/dev-domoticz && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/reivax/dev-domoticz /home/reivax/dev-domoticz/MQTT /home/reivax/dev-domoticz /home/reivax/dev-domoticz/MQTT /home/reivax/dev-domoticz/MQTT/CMakeFiles/mqtt.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : MQTT/CMakeFiles/mqtt.dir/depend

