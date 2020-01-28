#include "switchBoxD_fixtures.h"
// api/device/state

const char *switchBoxD_status_ok1 = R"(
{
  "device": {
    "deviceName": "My switchBoxD 1",
    "type": "switchBoxD",
    "fv": "0.201",
    "hv": "0.7",
    "id": "7eaf1234dc79",
    "ip": "192.168.1.239"
  }
}
)";

const char *switchBoxD_status_ok1_with_old_apilevel = R"(
{
  "device" : {
    "deviceName" : "My lamp 1",
    "type" : "switchBoxD",
    "fv" : "0.603",
    "hv" : "9.1",
    "apiLevel" : "20170524",
    "id" : "bbe62d8d49f4",
    "ip" : "192.168.9.115"
  }
}
)";

const char *switchBoxD_state_ok1 = R"(
{
    "relays": [
        {
            "relay": 0,
            "state": 0,
            "stateAfterRestart": 0,
            "name": "Output no 0"
        },
        {
            "relay": 1,
            "state": 1,
            "stateAfterRestart": 1,
            "name": "Output no 1"
        }
    ]
}
)";

const char *switchBoxD_state_after_first_relay_on = R"(
{
    "relays": [
        {
            "relay": 0,
            "state": 1,
            "stateAfterRestart": 0,
            "name": "Output no 0"
        },
        {
            "relay": 1,
            "state": 1,
            "stateAfterRestart": 1,
            "name": "Output no 1"
        }
    ]
}
)";

const char *switchBoxD_state_after_second_relay_off = R"(
{
    "relays": [
        {
            "relay": 0,
            "state": 1,
            "stateAfterRestart": 0,
            "name": "Output no 0"
        },
        {
            "relay": 1,
            "state": 0,
            "stateAfterRestart": 1,
            "name": "Output no 1"
        }
    ]
}
)";
