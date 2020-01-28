#include "shutterBox_fixtures.h"

const char *shutterBox_status_ok1 = R"(
{
    "device": {
        "deviceName": "My shutter 1",
        "type": "shutterBox",
        "fv": "0.147",
        "hv": "0.7",
        "apiLevel": "20180604",
        "id": "2bee34e750b8",
        "ip": "192.168.1.11"
    }
}
)";

const char *shutterBox_state_ok1 = R"(
{
    "shutter": {
        "state": 4,
        "currentPos": {
            "position": 34,
            "tilt": 3
        },
        "desiredPos": {
            "position": 78,
            "tilt": 97
        },
        "favPos": {
            "position": 13,
            "tilt": 17
        }
    }
}
)";

const char *shutterBox_state_after_set_21 = R"(
{
    "shutter": {
        "state": 4,
        "currentPos": {
            "position": 34,
            "tilt": 3
        },
        "desiredPos": {
            "position": 21,
            "tilt": 93
        },
        "favPos": {
            "position": 17,
            "tilt": 12
        }
    }
}
)";

const char *shutterBox_state_after_set_13 = R"(
{
    "shutter": {
        "state": 4,
        "currentPos": {
            "position": 72,
            "tilt": 3
        },
        "desiredPos": {
            "position": 13,
            "tilt": 93
        },
        "favPos": {
            "position": 17,
            "tilt": 12
        }
    }
}
)";
