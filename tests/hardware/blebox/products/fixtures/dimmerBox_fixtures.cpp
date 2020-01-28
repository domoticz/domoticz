#include "dimmerBox_fixtures.h"
const char *dimmerBox_status_ok1 = R"(
{
    "device": {
        "deviceName": "My dimmer 1",
        "type": "dimmerBox",
        "fv": "0.247",
        "hv": "0.2",
        "id": "1afe34e750b8"
    },
    "network": {
        "ip": "192.168.1.239",
        "ssid": "myWiFiNetwork",
        "station_status": 5,
        "apSSID": "dimmerBox-ap",
        "apPasswd": ""
    },
    "dimmer": {
        "loadType": 7,
        "currentBrightness": 65,
        "desiredBrightness": 237,
        "temperature": 39,
        "overloaded": false,
        "overheated": false
    }
}
)";

const char *dimmerBox_state_after_set_53 = R"(
{
    "dimmer": {
        "loadType": 7,
        "currentBrightness": 11,
        "desiredBrightness": 53,
        "temperature": 29,
        "overloaded": false,
        "overheated": false
    }
}
)";

const char *dimmerBox_state_after_set_239 = R"(
{
    "dimmer": {
        "loadType": 7,
        "currentBrightness": 203,
        "desiredBrightness": 239,
        "temperature": 12,
        "overloaded": false,
        "overheated": false
    }
}
)";

const char *dimmerBox_state_after_set_0 = R"(
{
    "dimmer": {
        "loadType": 7,
        "currentBrightness": 27,
        "desiredBrightness": 0,
        "temperature": 29,
        "overloaded": false,
        "overheated": false
    }
}
)";

const char *dimmerBox_state_after_set_255 = R"(
{
    "dimmer": {
        "loadType": 7,
        "currentBrightness": 142,
        "desiredBrightness": 255,
        "temperature": 29,
        "overloaded": false,
        "overheated": false
    }
}
)";
