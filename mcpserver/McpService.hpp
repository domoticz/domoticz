#pragma once

#include <string>
#include <json/json.h>
#include "../main/Helper.h"
#include "../main/mainworker.h"

namespace mcp
{
    bool getSensorValue(const Json::Value &jsonRequest, Json::Value &jsonRPCRep, const Json::Value &jsonDevices);
    bool toggleSwitchState(const Json::Value &jsonRequest, Json::Value &jsonRPCRep, const Json::Value &jsonDevices);
    bool getSwitchState(const Json::Value &jsonRequest, Json::Value &jsonRPCRep, const Json::Value &jsonDevices);
    bool getDeviceByName(const std::string &sDeviceName, Json::Value &device, const Json::Value &jsonDevices);
}