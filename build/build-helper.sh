#!/bin/sh

usage() { 
	echo "Usage: $0 open-zwave [ clean | check | test| updateIndexDefines]" 1>&2
	echo "Usage: $0 domoticz [ clean | cmake ]" 1>&2
	exit 1
}

if [ $# -eq 0 -o $# -gt 2 ]; then
	usage
fi

PROJECT=$1
CMD=$2
CPU_COUNT=$(lscpu -e | tail -n +2 | wc -l)

if [ $HOST_OS = "linux" ]; then
	copy_in() {
		ln -sf open-zwave open-zwave-read-only

		cd $PROJECT
	}
	copy_out() {
		cd ..
	}
else
	copy_in() {
		ln -sf open-zwave build/open-zwave-read-only

		echo "Syncing source tree..."
		rsync -a --delete $PROJECT build || exit 1
		echo

		cd build/$PROJECT
	}
	copy_out() {
		cd ../..
		echo

		echo "Syncing artifacts..."
		rsync -a --delete build/$PROJECT .
		echo
	}
fi

case $PROJECT in
domoticz)
	case $CMD in
	clean)
		copy_in
		cmake --build . --target clean || exit 1
		copy_out
		;;
	cmake)
		copy_in
		rm -f CMakeCache.txt
		cmake -DCMAKE_BUILD_TYPE=Release CMakeLists.txt || exit 1
		copy_out
		;;
    "")
		copy_in
    	make -j $CPU_COUNT || exit 1
		copy_out
    	;;
	*)        
		usage 
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
		usage 
		;;
	esac
	;;
*)
	usage 
	;;
esac
