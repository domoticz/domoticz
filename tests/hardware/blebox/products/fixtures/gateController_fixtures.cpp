#include "gateController_fixtures.h"

const char *gateController_status_ok1 = R"(
{
    "device": {
        "deviceName": "My gate controller 1",
        "type": "gateController",
        "apiLevel": "20180604",
        "fv": "1.390",
        "hv": "custom.2.6",
        "id": "aaa2ffaafe30db9437",
        "ip": "192.168.1.17"
    }
}
)";

const char *gateController_state_ok1 = R"(
{
  "gateController": {
    "state": 2,
    "safety": {
      "eventReason": 0,
      "triggered": [ 0 ]
    },
    "currentPos": [ 87 ],
    "desiredPos": [ 23 ]
  }
}
)";

const char *gateController_state_after_set_29 = R"(
{
  "gateController": {
    "state": 2,
    "safety": {
      "eventReason": 0,
      "triggered": [ 0 ]
    },
    "currentPos": [ 71 ],
    "desiredPos": [ 29 ]
  }
}
)";
