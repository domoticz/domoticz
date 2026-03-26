#pragma once

#include <string>
#include <vector>
#include <json/json.h>
#include "../main/Helper.h"
#include "../main/mainworker.h"
#include <libwebem/session.h>

namespace mcp
{
	// JSON-RPC 2.0 standard error codes
	constexpr int JSONRPC_PARSE_ERROR       = -32700;
	constexpr int JSONRPC_INVALID_REQUEST   = -32600;
	constexpr int JSONRPC_METHOD_NOT_FOUND  = -32601;
	constexpr int JSONRPC_INVALID_PARAMETER = -32602;
	constexpr int JSONRPC_INTERNAL_ERROR    = -32603;

	// MCP-specific error codes (reserved range: -32000 to -32099)
	constexpr int MCP_SERVER_ERROR          = -32000;
	constexpr int MCP_RESOURCE_NOT_FOUND    = -32001;
	constexpr int MCP_PERMISSION_DENIED     = -32002;
	constexpr int MCP_RATE_LIMIT_EXCEEDED   = -32003;
	constexpr int MCP_TIMEOUT_OCCURRED      = -32004;
	// Note: MCP_TOOL_EXECUTION_FAILED (-32000) was an alias for MCP_SERVER_ERROR — removed as duplicate

	void McpInitialize(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	void McpToolsList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	void McpToolsCall(const Json::Value &jsonRequest, Json::Value &jsonRPCRep, const http::server::WebEmSession &session);
	void McpResourcesList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	void McpResourcesTemplatesList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	void McpResourcesRead(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	void McpPromptsList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	void McpPromptsGet(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	void McpCompletionComplete(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);

	bool getSensorValue(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getSwitchState(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getLogging(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getFloorplan(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);

	bool toggleSwitchState(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool setThermostatSetpoint(const Json::Value& jsonRequest, Json::Value& jsonRPCRep);
	bool getStatus(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool searchDevices(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);

	bool setSwitchState(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool setDimmerLevel(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool controlBlinds(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool setColorBrightness(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool setColorTemperature(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);

	bool getScenes(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool switchScene(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getRooms(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getRoomDevices(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getSceneDevices(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);

	bool getHardware(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getSystemSettings(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getSunTimes(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getCameras(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getFloorplans(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getUsers(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);

	bool getSecurityStatus(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool setSecurityStatus(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);

	bool getDeviceByName(const std::string &sDeviceName, Json::Value &device);

	bool validRPC(const std::string &sInput, Json::Value &jsonRequest, std::string &sError);

	bool getAllDevices(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getDevice(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool renameDevice(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool deleteDevice(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool createVirtualSensor(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool updateDeviceValue(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getSensorHistory(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);

	bool getUserVariables(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool addUserVariable(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool updateUserVariable(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool deleteUserVariable(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);

	bool addLogMessage(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool sendNotification(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);

	bool getEvents(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool getEvent(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool createEvent(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool updateEvent(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);
	bool deleteEvent(const Json::Value &jsonRequest, Json::Value &jsonRPCRep);

	struct ToolParam
	{
		std::string name;
		std::string type;        // "string", "integer", "number", "boolean"
		std::string description;
		bool required = false;
		std::vector<std::string> enumValues;  // optional, empty means no enum constraint
	};

	struct ToolDef
	{
		std::string name;
		std::string title;
		std::string description;
		std::vector<ToolParam> params;
	};

	Json::Value buildToolSchema(const ToolDef &def);

	// Sets a text tool result on a JSON-RPC reply object.
	// isError: true if the tool reports a logical error (device not found, etc.)
	inline void setToolResult(Json::Value &jsonRPCRep, const std::string &text, bool isError = false)
	{
		Json::Value content;
		content["type"] = "text";
		content["text"] = text;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(content);
		jsonRPCRep["result"]["isError"] = isError;
	}

	// Overload for image content (used by getFloorplan)
	inline void setToolImageResult(Json::Value &jsonRPCRep, const std::string &base64Data, const std::string &mimeType)
	{
		Json::Value content;
		content["type"] = "image";
		content["mimeType"] = mimeType;
		content["data"] = base64Data;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(content);
		jsonRPCRep["result"]["isError"] = false;
	}
}
