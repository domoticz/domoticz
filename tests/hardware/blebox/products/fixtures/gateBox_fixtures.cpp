#include "gateBox_fixtures.h"

// GET /api/device/state
const char *gateBox_status_ok1 = R"(
{
    "deviceName": "My gate 1",
    "type": "gateBox",
    "fv": "0.176",
    "hv": "0.6",
    "id": "1afe34db9437",
    "ip": "192.168.1.11"
}
)";

// GET /api/gate/state
const char *gateBox_state_ok1 = R"(
{
    "currentPos": 87,
    "desiredPos": 23,
    "extraButtonType": 1,
    "extraButtonRelayNumber": 1,
    "extraButtonPulseTimeMs": 800,
    "extraButtonInvert": 1,
    "gateType": 0,
    "gateRelayNumber": 0,
    "gatePulseTimeMs": 800,
    "gateInvert": 0,
    "inputsType": 1,
    "openLimitSwitchInputNumber": 0,
    "closeLimitSwitchInputNumber": 1
}
)";

const char *gateBox_state_after_primary = R"(
{
  "gate": {
    "currentPos": 91,
    "desiredPos": 100,
    "extraButtonType": 1,
    "extraButtonRelayNumber": 1,
    "extraButtonPulseTimeMs": 800,
    "extraButtonInvert": 1,
    "gateType": 0,
    "gateRelayNumber": 0,
    "gatePulseTimeMs": 800,
    "gateInvert": 0,
    "inputsType": 1,
    "openLimitSwitchInputNumber": 0,
    "closeLimitSwitchInputNumber": 1
  }
}
)";
