#!/bin/bash

while getopts 'o:p:' c; do
  case $c in
    o) 
    	HOST_OS=$OPTARG
    	;;
    p)
    	PROJECT=$OPTARG
    	if [ $PROJECT = openzwave ]; then
    		PROJECT=open-zwave
    	elif [ $PROJECT != domoticz ]; then
    		echo Unknown project $PROJECT
    		exit 1
    	fi
    	;;
  esac
done
shift $(($OPTIND - 1))
CMD=$1

CPU_COUNT=$(lscpu -e | tail -n +2 | wc -l)

if [ $HOST_OS = "linux" ]; then
	DOMOTICZ_ROOT=domoticz
	ln -sf open-zwave open-zwave-read-only

	copy_in() {
		cd $1
	}
	copy_out() {
		cd ..
	}
else
	DOMOTICZ_ROOT=build/domoticz
	ln -sf open-zwave build/open-zwave-read-only

	copy_in() {
		echo "Syncing source tree $1..."
		rsync -a --delete $1 build || exit 1
		echo

		cd build/$1
	}
	copy_out() {
		cd ../..
		echo

		echo "Syncing artifacts $1..."
		rsync -a --delete build/$1 .
		echo
	}
fi

do_cmake() {
	if [ $1 = domoticz ]; then
		copy_in $1
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
		copy_out $1
	else
		echo "No CMake for $1"
		exit 1
	fi
}

do_clean() {
	copy_in $1
	if [ $1 = domoticz ]; then
		cmake --build . --target clean || exit 1
	else
		make clean || exit 1
	fi
	copy_out $1
}

do_compile() {
	copy_in $1
	if [ $1 = domoticz ]; then
		make -j $CPU_COUNT || exit 1
	else
		make || exit 1
	fi
	copy_out $1
}

do_run() {
	do_compile $1
	if [ $1 = domoticz ]; then
		cd $DOMOTICZ_ROOT
		./domoticz -verbose 1
	fi
}

do_make() {
	if [ $1 = domoticz ]; then
		echo "No make for $1"
		exit 1
	else
		copy_in $1 
		make $2 || exit 1
		copy_out $1
	fi
}

case $CMD in
clean)
	if [ -n "$PROJECT" ]; then
		do_clean $PROJECT
	else
		do_clean open-zwave
		do_clean domoticz
	fi
	;;
compile)
	if [ -n "$PROJECT" ]; then
		do_compile $PROJECT
	else
		do_compile open-zwave
		do_compile domoticz
	fi
	;;
cmake)
	if [ -n "$PROJECT" ]; then
		do_cmake $PROJECT
	else
		do_cmake domoticz
	fi
	;;
run)
	if [ -n "$PROJECT" ]; then
		do_run $PROJECT
	else
		do_run domoticz
	fi
	;;
check|updateIndexDefines|test) 
	if [ -n "$PROJECT" ]; then
		do_make $PROJECT $CMD
	else
		do_make open-zwave $CMD
	fi
	;;
shell)
	bash
	;;
*)        
	echo "Wrong invocation"
	exit 1
	;;
esac
