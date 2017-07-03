#!/bin/sh
cd $1

lowercase(){
    echo "$1" | sed "y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/"
}

OS=`lowercase \`uname -s\``
# KERNEL=`uname -r`
MACH=`uname -m`
if [ ${MACH} = "armv6l" ]
then
 MACH="armv7l"
fi

CHANNEL="stable"
if [ "$2" = "/beta" ]; then
  CHANNEL="beta"
fi

archive_file="http://www.domoticz.com/download.php?channel=${CHANNEL}&type=release&system=${OS}&machine=${MACH}"
checksum_file="http://www.domoticz.com/download.php?channel=${CHANNEL}&type=checksum&system=${OS}&machine=${MACH}"

# Download checksum
wget -q "${checksum_file}" -O update.tgz.sha256sum
if [ $? -ne 0 ]
then
        echo "Error downloading checksum file!...";
        exit 1
fi

# Download archive file
wget -q "${archive_file}" -O update.tgz
if [ $? -ne 0 ]
then
        echo "Error downloading archive file!...";
        exit 1
fi

# Check archive
if [ -f update.tgz.sha256sum ];
then
  #Check archive against checksum!
  #~ echo "LC_ALL=C cat update.tgz.sha256sum | awk '{print \$1\"  update.tgz\"}' | sha256sum -c -|  cut -d':' -f2 | tr -d ' '"
  valid=$(LC_ALL=C cat update.tgz.sha256sum | awk '{print $1"  update.tgz"}' | sha256sum -c -|  cut -d':' -f2 | tr -d ' ')
  if [ -z $valid ] || [ "$valid" != "OK" ]
  then
        echo "Archive checksum mismatch !";
        exit 1;
  fi
fi

#should never be here
exit 1;
