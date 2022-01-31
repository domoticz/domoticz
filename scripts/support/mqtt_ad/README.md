
# MQTT AutoDiscovery tools

## How to supply an MQTT AD messages dump to the developers

When you are asked to supply a copy of all MQTT messages for a device used by MQTT Autodiscovery, you can follow the following steps:

   1. go to subdirecory domoticz/scripts/support/mqtt_ad
   2. Run the script as shown in the examples below with the appropriate parameters. This will capture all MQTT packages in scope of the defined parametres.
   3. Provide us with the **mqtt_ad_record_*XXX*.log** file containing all records.
   4. Press Ctrl+c when you want to stop the capture process before the timer ends.

## mqtt_ad_record.sh

Script that helps capturing MQTT messages for Homeassistant AD into a file, so they can be shared easily with others for debugging.

### usage mqtt_ad_record.sh

```text
bash mqtt_ad_record.sh [-h hostname/ip] [-p port] [-s searchstring] [-t capturetime]
   -h Hostname or Ipaddres of MQTT deamon. default is 127.0.0.1
   -p port for the MQTT deamon. default is 1883
   -u Userid for MQTT deamon
   -P Password for MQTT deamon
   -s only records MQTT messages that contain this string in the TOPIC or PAYLOAD. default is all messages
   -t caputure time in seconds. default is 600
```

### examples mqtt_ad_record.sh

```bash
   # Records all MQTT Messages containing "/config", "_state" or "/state" for 10 minutes
   bash mqtt_ad_record.sh

   # specify userid and password to access the MQTT deamon.
   bash mqtt_ad_record.sh -u UserId -P Password

   #Records all MQTT Messages containing "/config", "_state" or "/state" for 30 Seconds
   bash mqtt_ad_record.sh -t 30

   #Records all MQTT Messages containing "TASMOTA_A1" for 10 minutes
   bash mqtt_ad_record.sh -s TASMOTA_A1

   #Records all MQTT Messages containing "TASMOTA_A1" for 30 seconds
   bash mqtt_ad_record.sh -s TASMOTA_A1 -t 30
```

The above command will create an **mqtt_ad_record_*XXX*.log** file in the current directory.

- where ***XXX*** will be ***all*** for the first 2 examples and will be ***TASMOTA_A1*** for the last 2 examples.

## mqtt_ad_send.sh

Script that sends the captured mqtt records again to a server recorded in HA_Discovery_mqtt.log in dezelfde directory.

### usage mqtt_ad_send.sh

```text
bash mqtt_ad_send.sh [-h hostname/ip] [-p port] [-s searchstring] [-i inputfile]
   -h Hostname or Ipaddres of MQTT deamon. default is 127.0.0.1
   -p port for the MQTT deamon. default is 1883
   -s only send MQTT messages that contain this string in the TOPIC or PAYLOAD. default is all messages
   -i inputfile. default is mqtt_ad_record_all.log
   -r override retain (y/n). defaults to the retain flag in the input records
   -d When Dry-run when switch is provided, no MQTT messages will be send, just the logging.
```

### examples mqtt_ad_send.sh

```bash
   #ALL records
   bash mqtt_ad_send.sh

   #ALL records from a specific file
   bash mqtt_ad_send.sh -i mqtt_ad_record_testDevice.log

   #Send selected records from log file:
   bash mqtt_ad_send.sh PARTIAL_DEVICE_NAME
```
