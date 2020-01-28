#include "airSensor_fixtures.h"

const char *airSensor_status_ok1 = R"(
{
  "deviceName": "My backyard air sensor",
  "type": "airSensor",
  "fv": "0.973",
  "hv": "0.6",
  "apiLevel": "20180403",
  "id": "1afe34db9437",
  "ip": "192.168.1.11"
}
)";

const char *airSensor_state_ok1 = R"(
{
  "air": {
    "sensors": [
      {
        "type": "pm1",
        "value": 111,
        "trend": 3,
        "state": 0,
        "qualityLevel": -1,
        "elaspedTimeS": -1
      },
      {
        "type": "pm2.5",
        "value": 222,
        "trend": 1,
        "state": 0,
        "qualityLevel": 1,
        "elaspedTimeS": -1
      },
      {
        "type": "pm10",
        "value": 333,
        "trend": 0,
        "state": 0,
        "qualityLevel": 6,
        "elaspedTimeS": -1
      }
    ]
  }
}
)";

// NOTE: one is "no data"
const char *airSensor_state_error_but_one_ok = R"(
{
  "air": {
    "sensors": [
      {
        "type": "pm1",
        "value": 111,
        "trend": 2,
        "state": 3,
        "qualityLevel": -1,
        "elaspedTimeS": -1
      },
      {
        "type": "pm2.5",
        "value": null,
        "trend": 0,
        "state": 0,
        "qualityLevel": 1,
        "elaspedTimeS": -1
      },
      {
        "type": "pm10",
        "value": 444,
        "trend": 1,
        "state": 0,
        "qualityLevel": 3,
        "elaspedTimeS": -1
      }
    ]
  }
}
)";

// NOTE: in reality, it's a 204 (no content)
const char *airSensor_state_after_kick = "";
