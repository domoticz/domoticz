#pragma once

#include <string>
#include <json/json.h>
#include "../main/Helper.h"
#include "../main/mainworker.h"

namespace mcp
{
	void McpInitialize(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	void McpToolsList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	void McpToolsCall(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	void McpResourcesList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	void McpResourcesRead(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	void McpPromptsList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	void McpPromptsGet(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);

	bool getSensorValue(const Json::Value &jsonRequest, Json::Value &jsonRPCRep, const Json::Value &jsonDevices);
	bool toggleSwitchState(const Json::Value &jsonRequest, Json::Value &jsonRPCRep, const Json::Value &jsonDevices);
	bool getSwitchState(const Json::Value &jsonRequest, Json::Value &jsonRPCRep, const Json::Value &jsonDevices);
	bool getDeviceByName(const std::string &sDeviceName, Json::Value &device, const Json::Value &jsonDevices);

	bool validRPC(const std::string &sInput, Json::Value &jsonRequest, std::string &sError);
}
