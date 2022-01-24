#!/bin/bash
sver="20220124-01"
: '
# Scriptname mqtt_ad_send.sh
# Created by JvdZ

Script that sends the captured mqtt records again to a server recorded in HA_Discovery_mqtt.log in dezelfde directory.

usage:
bash mqtt_ad_send.sh [-h hostname/ip] [-p port] [-s searchstring] [-i inputfile]
   -h Hostname or Ipaddres of MQTT deamon. default is 127.0.0.1
   -p port for the MQTT deamon. default is 1883
   -u Userid for MQTT deamon
   -P Password for MQTT deamon
   -s only send MQTT messages that contain this string in the TOPIC or PAYLOAD. default is all messages
   -i inputfile. default is mqtt_ad_record_all.log
   -r override retain (y/n). defaults to the retain flag in the record
   -d When Dry-run when switch is provided, no MQTT messages will be send, just the logging.

example:
   ALL records     :  bash mqtt_ad_send.sh
   Selected records:  bash mqtt_ad_send.sh PARTIAL_DEVICE_NAME
'

MQTT_IP="127.0.0.1"
MQTT_PORT="1883"
MQTT_Param=""
input="mqtt_ad_record_all.log"
retain=""
dryrun=""

# process parameters
while getopts h:p:u:P:s:i:r:d flag
do
   case "${flag}" in
      h) MQTT_IP=${OPTARG};;
      p) MQTT_PORT=${OPTARG};;
      u) MQTT_Param=${MQTT_Param}' -u '${OPTARG};;
      P) MQTT_Param=${MQTT_Param}' -P '${OPTARG};;
      s) sdev=${OPTARG};;
      i) input=${OPTARG};;
      r) retain=${OPTARG};;
      d) dryrun='y';;
   esac
done

# Set case insensitive flag for the PARTIAL_DEVICE_NAME tests
shopt -s nocasematch

echo "== $sver =============================================================================================================="
echo "MQTT_IP   : '$MQTT_IP'";
echo "MQTT_PORT : '$MQTT_PORT'";
echo "MQTT_Param: '$MQTT_Param'";
echo "inputfile : '$input'";
if [[ ! -z "$retain" ]] ; then
   echo "Retain override with '$retain'";
fi
if [[ ! -z "$dryrun" ]] ; then
   echo "### Dry-Run so no MQTT messages are send, just showing the log of the selected messages.###";
fi

srec=0;

#Get rid of CR and only leave LF and read all records
#   -r option read is to leave backslaches as in the file
cat "$input" | tr -d "\r" | while IFS=$'\t' read -r itime iretain itopic ipayload;
do
   #echo -e "###\nT=$itopic\nP=$ipayload"
   #skip Comment lines starting with #
   [[ itime =~ ^#.* ]] && { continue; }

   #skip empty lines or only one param is provided
   [[ -z "$ipayload" ]] && { continue; }

   #skip lines that do not contain the selected dev
   [[ ! $itopic =~ $sdev ]] && {
      [[ ! $ipayload =~ $sdev ]] && { continue; }
   }

   ## process the selected record
   srec=$((srec+1))
   # Set MQTT Options
   MQTT_Opts=""
   # Add Retain option when 1 is specified in input or in the override param -r
   if [[ -z "$retain" ]] ; then
      [[ "$iretain" == "1" ]] && { MQTT_Opts=" -r";}
   else
      [[ "$retain" == "y" ]] && { MQTT_Opts=" -r";}
   fi
   if [[ -z "$dryrun" ]] ; then
      echo -e "== $srec > $MQTT_Opts\nT=$itopic\nP=$ipayload"
      mosquitto_pub $MQTT_Param $MQTT_Opts -h $MQTT_IP -p $MQTT_PORT -t "$itopic" -m "$ipayload"
   else
      echo -e "-- $srec: $MQTT_Opts\nT=$itopic\nP=$ipayload"
   fi
done

if [[ ! -z "$dryrun" ]] ; then
   echo "###============================================###";
   echo "### Dry-Run ended,so no MQTT messages are send.###";
   echo "###============================================###";
fi
