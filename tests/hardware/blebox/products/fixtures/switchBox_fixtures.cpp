#include "switchBox_fixtures.h"
// api/device/state

const char *switchBox_status_ok1 = R"(
{
  "device": {
    "deviceName": "My switchBox 1",
    "type": "switchBox",
    "fv": "0.247",
    "hv": "0.2",
    "id": "1afe34e750b8"
  },
  "network":  {
    "ip": "192.168.1.239",
    "ssid": "myWiFiNetwork",
    "station_status": 5,
    "apSSID": "switchBox-ap",
    "apPasswd": ""
  },
  "relays": [{
    "relay":  0,
    "state":  0,
    "stateAfterRestart":  0
  }]
}

)";

const char *switchBox_status_ok2 = R"(
{
  "device": {
    "deviceName": "My switchBox 2",
    "type": "switchBox",
    "fv": "0.247",
    "hv": "0.2",
    "id": "1afe34e750b8"
  },
  "network": {
    "ip": "192.168.1.239",
    "ssid": "myWiFiNetwork",
    "station_status": 5,
    "apSSID": "switchBox-ap",
    "apPasswd": ""
  },
  "relays": [{
    "relay": 0,
    "state": 1,
    "stateAfterRestart": 1
  }]
}

)";

const char *switchBox_state_after_relay_on = R"(
{
  "relays": [{
    "relay": 0,
    "state": 1,
    "stateAfterRestart": 1
  }]
}

)";
