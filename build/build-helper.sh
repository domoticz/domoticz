#!/bin/bash

if [ $# -ge 1 ]; then
	PROJECT=$1; shift 1
fi
if [ $# -ge 1 ]; then
	CMD=$1; shift 1
fi
CPU_COUNT=$(lscpu -e | tail -n +2 | wc -l)

if [ $HOST_OS = "linux" ]; then
	WORKING_DIR=$PROJECT
	copy_in() {
		ln -sf open-zwave open-zwave-read-only

		cd $WORKING_DIR
	}
	copy_out() {
		cd ..
	}
else
	WORKING_DIR=build/$PROJECT
	copy_in() {
		ln -sf open-zwave build/open-zwave-read-only

		echo "Syncing source tree..."
		rsync -a --delete $PROJECT build || exit 1
		echo

		cd $WORKING_DIR
	}
	copy_out() {
		cd ../..
		echo

		echo "Syncing artifacts..."
		rsync -a --delete build/$PROJECT .
		echo
	}
fi

do_cmake() {
	copy_in
	rm -f CMakeCache.txt
	args="-DCMAKE_BUILD_TYPE=Release"
	for arg in \
            USE_BUILTIN_JSONCPP \
            USE_BUILTIN_MINIZIP \
            USE_BUILTIN_MQTT \
            USE_BUILTIN_SQLITE \
            USE_PYTHON \
            INCLUDE_LINUX_I2C \
            INCLUDE_SPI \
            WITH_LIBUSB \
            USE_LUA_STATIC \
            USE_OPENSSL_STATIC \
            USE_STATIC_OPENZWAVE \
            USE_PRECOMPILED_HEADER \
            GIT_SUBMODULE
	do
		test -n "${!arg}" && args+=" -D${arg}=${!arg}"
	done
	cmake $args CMakeLists.txt || exit 1
	copy_out
}

case $PROJECT in
domoticz)
	case $CMD in
	clean)
		copy_in
		cmake --build . --target clean || exit 1
		copy_out
		;;
	cmake)
		do_cmake
		;;
	run)
		copy_in
		make -j $CPU_COUNT || exit 1
		copy_out

		cd $WORKING_DIR
		./domoticz -verbose 1
		;;
	"")
		copy_in
		make -j $CPU_COUNT || exit 1
		copy_out
		;;
	*)        
		echo "Wrong second argument"
		exit 1
		;;
	esac
	;;
open-zwave)
	case $CMD in
	clean|""|check|updateIndexDefines|test) 
		copy_in
		make ${CMD} || exit 1
		copy_out
		;;
	*)        
		echo "Wrong second argument"
		exit 1
		;;
	esac
	;;
shell)
	bash
	;;
*)
	echo "Wrong first argument"
	exit 1
	;;
esac
