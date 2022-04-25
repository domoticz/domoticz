
# Domoticz support scripts

## Index available support tools

- [Get Domoticz version list](./README.md#get_build_versionssh)
- [MQTT AutoDiscovery tools](./mqtt_ad/README.md)

## get_build_versions.sh

**This script ONLY works when you run it from a subdirectory within the GITHUB clone of DOMOTICZ/DOMOTICZ.**\
Script will list 20 commitlog entries preceding provided version, to easily list changes made around that version.

### usage get_build_versions.sh

```text
sh getcommitlog.sh [-v version] [-m max_logEntries]
   -v domoticz version. default last
   -m number of records to show. default 20
```

### examples get_build_versions.sh

```bash
#List last 20 versions & commit logrecords
sh getcommitlog.sh

#List last 20 versions & commit logrecords for v41080
sh getcommitlog.sh -v 41080
*** Get 20 logrecords from commit:11974  version:14080 ***
Build  Commit    Date       Commit Description
------ --------- ---------- ----------------------------------------------------------------------
V14080 41e11f829 2022-01-17 Fix compile warning (and use internal reader)
V14079 6350f3544 2022-01-17 MQTT-AD: Escaping special characters
V14078 b120b0041 2022-01-17 Merge pull request #5113 from guillaumeclair/development
V14077 67c349a22 2022-01-17 Fixed compile warnings under windows
V14076 0f31858e1 2022-01-17 MQTT-AD: making sure correct discovery topic is matched (Fixes #5114)
V14075 7259c5139 2022-01-17 Fix a bug in variables name in alertlevel function
V14074 f64304efb 2022-01-16 Allow hardware to declare manual switches they are able to handle. (#4923)
V14073 4fcda55f3 2022-01-16 Partly fix for Meter Counter report
V14072 63e2f7b97 2022-01-16 MQTT/AD: Fixes for Fibaro FGRGBW dimmer (Issue #5069) (#5100)
V14071 f47572654 2022-01-16 MQTT-AD: Making sure JSON object is not a object
V14070 0bf7a176f 2022-01-15 MQTT-AD: putting unknown sensor types back into general type
V14069 b2a1ebf50 2022-01-15 MQTT-AD: Added more exception information
V14068 16801a0a4 2022-01-15 MQTT-AD: Added detection of CT nodes (needs more work from other pR)
V14067 b36f2bc65 2022-01-15 Replace light/switch now only transfers new HardwareID/DeviceID/Unit to the old device
V14066 13523bbe6 2022-01-15 Making sure a new sensor is created (could already exist a device with same type/id)
V14065 b66a52e13 2022-01-15 Updated History
V14064 650d91643 2022-01-15 Merge pull request #5111 from jgaalen/add-annathermostat-metrics
V14063 a3b03f315 2022-01-14 Add AnnaThermostat metrics
V14062 fce96f2a0 2022-01-14 Add AnnaThermostat metrics
V14061 d43fd6b3b 2022-01-14 Add Daikin yearly energy usage for kwh meter
```
