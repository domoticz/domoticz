#include "saunaBox_fixtures.h"
const char *saunaBox_status_ok1 = R"(
{
    "device": {
        "deviceName": "My saunaBox 1",
        "type": "saunaBox",
        "fv": "0.176",
        "hv": "0.6",
        "apiLevel": "20180604",
        "id": "1afe34db9437",
        "ip": "192.168.1.11"
    }
}
)";

const char *saunaBox_state_ok1 = R"(
{
    "heat": {
        "state": 0,
        "desiredTemp": 7126,
        "sensors": [
            {
                "type": "temperature",
                "id": 0,
                "value": 6789,
                "trend": 0,
                "state": 2,
                "elapsedTimeS": 0
            }
        ]
    }
}
)";

const char *saunaBox_state_error = R"(

{
    "heat": {
        "state": 0,
        "desiredTemp": 7126,
        "sensors": [
            {
                "type": "temperature",
                "id": 0,
                "value": 4567,
                "trend": 0,
                "state": 3,
                "elapsedTimeS": 0
            }
        ]
    }
}

)";

const char *saunaBox_state_ok2 = R"(
{
    "heat": {
        "state": 0,
        "desiredTemp": 4560,
        "sensors": [
            {
                "type": "temperature",
                "id": 0,
                "value": 6567,
                "trend": 0,
                "state": 2,
                "elapsedTimeS": 0
            }
        ]
    }
}
)";

const char *saunaBox_state_after_setpoint_3260 = R"(
{
    "heat": {
        "state": 0,
        "desiredTemp": 3260,
        "sensors": [
            {
                "type": "temperature",
                "id": 0,
                "value": 5432,
                "trend": 0,
                "state": 2,
                "elapsedTimeS": 0
            }
        ]
    }
}
)";

const char *saunaBox_state_after_set_on = R"(
{
    "heat": {
        "state": 1,
        "desiredTemp": 3890,
        "sensors": [
            {
                "type": "temperature",
                "id": 0,
                "value": 2876,
                "trend": 0,
                "state": 2,
                "elapsedTimeS": 0
            }
        ]
    }
}
)";

const char *saunaBox_state_after_set_off = R"(
{
    "heat": {
        "state": 0,
        "desiredTemp": 3770,
        "sensors": [
            {
                "type": "temperature",
                "id": 0,
                "value": 1234,
                "trend": 0,
                "state": 2,
                "elapsedTimeS": 0
            }
        ]
    }
}
)";
