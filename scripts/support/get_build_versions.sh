#!/bin/bash
: '
# Scriptname getcommitlog.sh     Ver: 20220123-02
# Created by JvdZ
# base info from https://www.domoticz.com/forum/viewtopic.php?f=47&t=30405#p230094

Script will list 20 commitlog entries preceding provided version, to easily list changes made around that version

usage:
sh getcommitlog.sh [-v version] [-m max_logEntries]
   -v domoticz version. default last
   -m number of records to show. default 20
'

MAXREC=20;
IVER=""
# process parameters
while getopts m:v: flag
do
	case "${flag}" in
        m) MAXREC=${OPTARG};;
        v) IVER=${OPTARG};;
    esac
done
# Get latest commit count
TCNT=`git rev-list HEAD --count`
# Version is Commitcount + 2107  use last when not provided as input
if [[ -z $IVER ]]; then
   let IVER=TCNT+2107;
fi
# Calculate start record
let SREC=TCNT+2107-IVER
let SCOM=TCNT-SREC
echo " *** Get $MAXREC logrecords from commit:$SCOM  version:$IVER ***"
echo "Build  Commit    Date       Commit Description"
echo "------ --------- ---------- ----------------------------------------------------------------------"
git log --date=human --pretty=format:"%h %as %s" --skip=$SREC --max-count=$MAXREC | awk '{print "V"'$IVER'-NR+1 " " $s}'
