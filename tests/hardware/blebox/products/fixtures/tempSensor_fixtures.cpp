#include "tempSensor_fixtures.h"

const char *tempSensor_status_ok1 = R"(
{
    "device": {
        "deviceName": "My tempSensor",
        "type": "tempSensor",
        "fv": "0.176",
        "hv": "0.6",
        "apiLevel": "20180604",
        "id": "1afe34db9437",
        "ip": "192.168.1.11"
    }
}
)";

const char *tempSensor_state_error_status = R"(
{
  "tempSensor": {
    "sensors": [
      {
        "type": "temperature",
        "id": 0,
        "value": 2212,
        "trend": 3,
        "state": 3,
        "elapsedTimeS": 0
      }
    ]
  }
}
)";

const char *tempSensor_state_ok1 = R"(
{
  "tempSensor": {
    "sensors": [
      {
        "type": "temperature",
        "id": 0,
        "value": 2212,
        "trend": 3,
        "state": 2,
        "elapsedTimeS": 0
      }
    ]
  }
}

)";

const char *tempSensor_state_ok2_multisensor = R"(
{
  "tempSensor": {
    "sensors": [
      {
        "type": "temperature",
        "id": 9,
        "value": 2219,
        "trend": 3,
        "state": 2,
        "elapsedTimeS": 0
      },

      {
        "type": "temperature",
        "id": 7,
        "value": 1234,
        "trend": 2,
        "state": 2,
        "elapsedTimeS": 3
      }
    ]
  }
}
)";
