
# MQTT AutoDiscovery tools

Two Python scripts to capture the MQTT Auto Discovery messages of a broker into a file, and to replay such a file
to another (test) broker. They run on Windows, Linux, macOS and a Raspberry Pi.

Requirements: Python 3.7 or newer and the paho-mqtt package:

```text
pip install paho-mqtt
# or on Debian / Raspberry Pi OS:
sudo apt-get install python3-paho-mqtt
```

On Windows use `python` or `py` instead of `python3` in the examples below.

## How to supply an MQTT AD capture to the developers

When you are asked for a copy of the MQTT messages of your devices:

1. Go to the directory `domoticz/scripts/support/mqtt_ad`.
2. Run `mqtt_ad_record.py` with the address and login of your broker, for example
   `python3 mqtt_ad_record.py -h 192.168.1.10 -u UserId -P -t 300 -l -x` (`-P` without a value asks for the
   password)
3. While it records, operate the devices that have the problem, so their state messages are included.
4. Attach the **mqtt_ad_record_*XXX*.log** file to the GitHub issue (or zip it when it is large).

Use `-x` (and `-X` with your room names) when you do not want to share IP addresses, MAC addresses, device ids
or names. The password is never shown or written to the file. To keep it out of the process list and the shell
history, give `-P` without a value (the script asks for it) or set the environment variable `MQTT_PASSWORD`.

## mqtt_ad_record.py

Captures the discovery configs and every message on a topic that one of those configs points to (state_topic,
brightness_state_topic, rgb_state_topic, availability, json_attributes_topic and so on), whatever the topic is
called. So Zigbee2MQTT, Z-Wave JS UI (`.../currentValue`, `.../currentColor`), Tasmota, Shelly, ESPHome and others
are all captured completely, and messages of other applications on the same broker are left out. The `~` base
topic and abbreviated keys (`stat_t`, `bri_stat_t`, ...) are understood. Ctrl+C, `kill` (SIGTERM) or closing the
terminal (SIGHUP) stops earlier and still writes the file.

### usage mqtt_ad_record.py

```text
python3 mqtt_ad_record.py [-h host] [-p port] [-u user] [-P password] [--tls] [--cafile file] [--insecure]
                          [-d prefix] [-s searchstring] [-t seconds] [-a] [-l] [-x] [-X "name1,name2"] [-o file]
   -h  Hostname or IP address of the MQTT broker. default is 127.0.0.1
   -p  Port of the MQTT broker. default is 1883, or 8883 with TLS
   -u  User for the MQTT broker
   -P  Password for the MQTT broker. -P without a value asks for it; without -P the environment variable
       MQTT_PASSWORD is used
   --tls       Connect with TLS (checks the certificate against the system certificates)
   --cafile    CA certificate file for TLS (implies --tls)
   --insecure  TLS without checking the certificate, for a self signed certificate (implies --tls)
   -d  Discovery prefix. default is homeassistant
   -s  Only keep devices whose discovery config contains this string (case insensitive), together with the
       topics their configs point to. With -a: only messages that contain the string in the topic or payload.
   -t  Capture time in seconds. default is 600
   -a  Keep all messages on the broker, not only the discovery related ones
   -l  Keep only the last message of each topic. A topic that had a retained message stays retained.
   -x  Anonymize IP addresses, MAC addresses, Z-Wave home ids and Zigbee IEEE addresses. The same value always
       gets the same placeholder, so the topics in the configs still match their messages.
   -X  Anonymize these names as well, comma separated (implies -x). Only whole words are replaced, longest name
       first; the name as given matches with spaces, "_", "-" or no spaces, and in lowercase.
   -o  Output file. default is mqtt_ad_record_all.log, or mqtt_ad_record_<searchstring>.log with -s
   --help  Show the help
```

### examples mqtt_ad_record.py

```bash
# Record all discovery configs and their state messages for 10 minutes
python3 mqtt_ad_record.py

# Broker with a login and a self signed TLS certificate, 5 minutes, latest message per topic, anonymized
python3 mqtt_ad_record.py -h 192.168.1.10 -p 8883 -u UserId -P Password --insecure -t 300 -l -x

# Also hide room names
python3 mqtt_ad_record.py -x -X "Living Room,Kitchen,Garage"

# Only the devices with "TASMOTA_A1" in their discovery config, for 30 seconds
python3 mqtt_ad_record.py -s TASMOTA_A1 -t 30

# Everything on the broker
python3 mqtt_ad_record.py -a
```

At the end it shows how many configs (per component) and state messages were recorded, and how many topics the
configs point to that had no message during the capture. States that the device does not publish retained only
arrive when the device reports, so record longer or operate the devices when that number is high.

### File format

One message per line: `<time><TAB><retain 0/1><TAB><topic><TAB><payload>`, the discovery configs first. The time
is local time in milliseconds with the UTC offset.
Lines starting with `#` are comments with a summary. Multi line JSON payloads are written on one line.
Files made with the older mqtt_ad_record.sh have the same format.

## mqtt_ad_send.py

Sends the records of a capture file to a broker, with the retain flag of each record. Use a separate test broker
with a test Domoticz on it: replaying to the broker the capture came from overwrites the retained configs and
states of the real devices.

### usage mqtt_ad_send.py

```text
python3 mqtt_ad_send.py [-h host] [-p port] [-u user] [-P password] [--tls] [--cafile file] [--insecure]
                        [-i inputfile] [-s searchstring] [-r y|n] [-w milliseconds] [-d]
   -h, -p, -u, -P, --tls, --cafile, --insecure  as for mqtt_ad_record.py (also MQTT_PASSWORD)
   -i  Input file. default is mqtt_ad_record_all.log
   -s  Only send records that contain this string in the topic or payload (case insensitive)
   -r  Override the retain flag of the records (y/n)
   -w  Wait this many milliseconds between messages. default is 0
   -d  Dry run: only show the selected records, send nothing
```

### examples mqtt_ad_send.py

```bash
# All records of mqtt_ad_record_all.log to the local broker
python3 mqtt_ad_send.py

# All records from a specific file
python3 mqtt_ad_send.py -i mqtt_ad_record_testDevice.log

# Only the records of one device, without sending
python3 mqtt_ad_send.py -s PARTIAL_DEVICE_NAME -d
```
