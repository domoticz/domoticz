#include "wLightBox_fixtures.h"

const char *wLightBox_status_ok1 = R"(
{
    "device": {
        "deviceName": "My light 1",
        "type": "wLightBox",
        "fv": "0.247",
        "hv": "0.2",
        "id": "1afe34e750b8"
    },
    "network": {
        "ip": "192.168.1.237",
        "ssid": "myWiFiNetwork",
        "station_status": 5,
        "apSSID": "wLightBox-ap",
        "apPasswd": ""
    },
    "rgbw": {
        "desiredColor": "ff000000",
        "currentColor": "ff000000",
        "fadeSpeed": 248,
        "effectSpeed": 2,
        "effectID": 3,
        "colorMode": 3
    }
}
)";

const char *wLightBox_state_ok2 = R"(
{
  "rgbw": {
    "colorMode": 1,
    "effectId": 2,
    "desiredColor": "ff000035",
    "currentColor": "ff003000",
    "lastOnColor": "ff003000",
    "durationsMs": {
      "colorFade": 1000,
      "effectFade": 1500,
      "effectStep": 2000
    }
  }
}
)";

const char *wLightBox_state_after_set = R"(
{
  "rgbw": {
    "colorMode": 1,
    "effectId": 2,
    "desiredColor": "fea16f70",
    "currentColor": "abcdef09",
    "lastOnColor": "c1d2e3ba",
    "durationsMs": {
      "colorFade": 1000,
      "effectFade": 1500,
      "effectStep": 2000
    }
  }
}
)";

const char *wLightBox_state_after_set_wm = R"(
{
  "rgbw": {
    "colorMode": 1,
    "effectId": 2,
    "desiredColor": "000000b5",
    "currentColor": "abcdef09",
    "lastOnColor": "c1d2e3ba",
    "durationsMs": {
      "colorFade": 1000,
      "effectFade": 1500,
      "effectStep": 2000
    }
  }
}
)";

const char *wLightBox_state_after_off = R"(
{
  "rgbw": {
    "colorMode": 1,
    "effectId": 2,
    "desiredColor": "00000000",
    "currentColor": "abcdef09",
    "lastOnColor": "ff000000",
    "durationsMs": {
      "colorFade": 1000,
      "effectFade": 1500,
      "effectStep": 2000
    }
  }
}
)";

const char *wLightBox_state_after_offon = R"(
{
  "rgbw": {
    "colorMode": 1,
    "effectId": 2,
    "desiredColor": "ff000000",
    "currentColor": "abcdef09",
    "lastOnColor": "ff000000",
    "durationsMs": {
      "colorFade": 1000,
      "effectFade": 1500,
      "effectStep": 2000
    }
  }
}
)";
