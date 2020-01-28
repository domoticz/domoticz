#include "wLightBoxS_fixtures.h"

const char *wLightBoxS_status_ok1 = R"(
{
    "device": {
        "deviceName": "My light 1",
        "type": "wLightBoxS",
        "fv": "0.247",
        "hv": "0.2",
        "id": "eafe34e750b9"
    },
    "network": {
        "ip": "192.168.1.237",
        "ssid": "myWiFiNetwork",
        "station_status": 5,
        "apSSID": "wLightBoxS-ap",
        "apPasswd": ""
    },
    "light": {
        "desiredColor": "CE",
        "currentColor": "FC",
        "fadeSpeed": 248
    }
}
)";

const char *wLightBoxS_state_after_set_AD = R"(
{
  "light": {
      "desiredColor": "AD",
      "currentColor": "E9",
      "fadeSpeed": 248
  }
}
)";
