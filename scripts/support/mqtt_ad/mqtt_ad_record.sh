#!/bin/bash
sver="20220124-01"
: '
# Scriptname mqtt_ad_record.sh
# Created by JvdZ

Script that helps capturing MQTT messages for Homeassistant AD into a file so they can be shared easily with others for debugging.

usage:
bash mqtt_ad_record.sh [-h hostname/ip] [-p port] [-s searchstring] [-t*max capture time seconds*]
   -h Hostname or Ipaddres of MQTT deamon. default is 127.0.0.1
   -p port for the MQTT deamon. default is 1883
   -u Userid for MQTT deamon
   -P Password for MQTT deamon
   -s only records MQTT messages that contain this string in the TOPIC or PAYLOAD. default is all messages
   -t caputure time in seconds. default is 600

examples:
   # Records all MQTT Messages containing "/config", "_state" or "/state" for 10 minutes.
   bash mqtt_ad_record.sh

   # specify userid and password to access the MQTT deamon.
   bash mqtt_ad_record.sh -u UserId -P Password

   #Records all MQTT Messages containing "/config", "_state" or "/state" for 30 Seconds.
   bash mqtt_ad_record.sh -t 30

   #Records all MQTT Messages containing "TASMOTA_A1" for 10 minutes.
   bash mqtt_ad_record.sh -s TASMOTA_A1

   #Records all MQTT Messages containing "TASMOTA_A1" for 30 seconds.
   bash mqtt_ad_record.sh -s TASMOTA_A1 -t 30

Output file name:
The above commands will create file mqtt_ad_record_XXX.log in the current directory,
   where XXX will be all for the first 2 examples and will be TASMOTA_A1 for the last 2 examples.
'

MQTT_IP="127.0.0.1"  # Define MQTT Server
MQTT_PORT="1883"     # Define port
rtime=600            # Define default Capture time for MQTT messages in seconds.
                     # You can always interrupt the capture process at anytime withCtrl+Break pr Ctrl+C
MQTT_Param=""        # used for extram mosquitto_sub parameters

# Check if mosquitto_sub is installed
if ! command -v mosquitto_sub &> /dev/null; then
   echo "================================================================================================================="
   echo "This script can be used to capture MQTT messages for a particular device."
   echo "Current MQT Server $MQTT_IP port $MQTT_PORT, edit script to change these."
   echo "!!!! program mosquitto_sub not found, please install that first."
   echo "RPI install:sudo apt-get install mosquitto-clients"
   echo "================================================================================================================="
   exit
fi
# process parameters
while getopts h:p:u:P:s:t: flag
do
   case "${flag}" in
      h) MQTT_IP=${OPTARG};;
      p) MQTT_PORT=${OPTARG};;
      u) MQTT_Param=${MQTT_Param}' -u '${OPTARG}'';;
      P) MQTT_Param=${MQTT_Param}' -P '${OPTARG}'';;
      s) sdev=${OPTARG};;
      t) rtime=${OPTARG};;
   esac
done

# Set case insensitive flag for the PARTIAL_DEVICE_NAME tests
shopt -s nocasematch

# trap ctrl-c and call ctrl_c()
trap message INT

function message() {
   echo "** CTRL-C pressed."
   # command for clean up e.g. rm and so on goes below
}

echo "== $sver =============================================================================================================="
echo "MQTT_IP   : '$MQTT_IP'";
echo "MQTT_PORT : '$MQTT_PORT'";
echo "MQTT_Param: '$MQTT_Param'";
echo "Recordtime: '$rtime'";
echo "Search For: '$sdev'";

logfile="mqtt_ad_record_all.log"
# Start Capture
if [[ -z $sdev ]]; then
   echo "Start Capture for $rtime seconds of all MQTT messages to Console and file: $logfile"
   mosquitto_sub $MQTT_Param -h $MQTT_IP -p $MQTT_PORT -t "#" -v -W $rtime -F "%I\t%r\t%t\t%p"| stdbuf -i0 -o0 grep -i -e "\/config\|[_\/]state" | stdbuf -i0 -o0 tee "$logfile"
else
   # remove characters in filename that aren't allowed
   logfile="mqtt_ad_record_"${sdev//[^A-Za-z0-9-_]/}".log"
   echo "Start Capture for $rtime seconds of MQTT messages containing $sdev to Console and file: $logfile"
   mosquitto_sub $MQTT_Param -h $MQTT_IP -p $MQTT_PORT -t "#" -v -W $rtime -F "%I\t%r\t%t\t%p"| stdbuf -i0 -o0 grep -i "$sdev" | stdbuf -i0 -o0 tee "$logfile"
fi
# Capture Ended
if [ "$?" -eq "0" ] ; then
    echo "Capture ended, check file: $logfile"
else
    echo "Capture interrupted, check file: $logfile"
fi