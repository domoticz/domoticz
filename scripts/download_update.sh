#!/bin/sh
cd $1

lowercase(){
    echo "$1" | sed "y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/"
}

OS=`lowercase \`uname -s\``
# KERNEL=`uname -r`
MACH=`uname -m`

ARCH=${MACH}
if [ ${ARCH} = "armv6l" ]
then
 ARCH="armv7l"
fi

base_url="http://domoticz.sourceforge.net$2"

archive_file="domoticz_${OS}_${ARCH}.tgz"
checksum_file="domoticz_${OS}_${ARCH}.tgz.sha256sum"

# Download checksum
wget -q ${base_url}/${checksum_file} -O update.tgz.sha256sum
if [ $? -ne 0 ]
then
	echo "Error downloading checksum file!...";
	exit 1
fi

# Download archive file
wget -q ${base_url}/${archive_file} -O update.tgz
if [ $? -ne 0 ]
then
        echo "Error downloading archive file!...";
        exit 1
fi

# Check archive
if [ -f update.tgz.sha256sum ];
then
  #Check archive against checksum!
  valid=$(LC_ALL=C sha256sum -c update.tgz.sha256sum | grep update.tgz | cut -d':' -f2 | tr -d ' ')
  if [ $valid != "OK" ]
  then
        echo "Archive checksum mismatch !";
        exit 1;
  fi
fi
