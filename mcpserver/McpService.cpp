/*
 * McpService.cpp
 * The MCP Service of domoticz implements the Model Context Protocol (currently version 2025-06-18)
 * so domoticz can be used as a agent in a LLM (Large Language Model) AI Agent context.
 *
 *  Created on: 4 April 2025
 *      Author: kiddigital
 * 
 * 
 * It contains the PostMCP routine that is part of the WebServer class, but for sourcecode management
 * reasons separated out into its own file so it is easier to maintain this MCP related function
 * of the WebServer. The definition of this method here is still in 'main/Webserver.h'
 * Also it contains the implementation of the other Model Context Protocol methods, which are defined
 * in 'mcpserver/McpService.hpp'.
 *  
*/

#include "stdafx.h"
#include <iostream>
#include <functional>
#include <unordered_map>
#include <json/json.h>
#include "McpService.hpp"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "../main/json_helper.h"
#include "../main/WebServer.h"
#include "../main/WebServerHelper.h"
#include "../notifications/NotificationHelper.h"
#include "../hardware/ColorSwitch.h"
#include <libwebem/Base64.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define JSONRPC_PARSE_ERROR -32700
#define JSONRPC_INVALID_REQUEST -32600
#define JSONRPC_METHOD_NOT_FOUND -32601
#define JSONRPC_INVALID_PARAMETER -32602
#define JSONRPC_INTERNAL_ERROR -32603
#define MCP_SERVER_ERROR -32000
#define MCP_TOOL_EXECUTION_FAILED -32000
#define MCP_RESOURCE_NOT_FOUND -32001
#define MCP_PERMISSION_DENIED -32002
#define MCP_RATE_LIMIT_EXCEEDED -32003
#define MCP_TIMEOUT_OCCURRED -32004

extern http::server::CWebServerHelper m_webservers;
extern CLogger _log;
extern CNotificationHelper m_notifications;
extern std::string szAppVersion;
extern std::string szAppHash;
extern std::string szAppDate;
extern time_t m_StartTime;
extern bool g_bLlmMCPSupport;

namespace http
{
	namespace server
	{
		void CWebServer::PostMcp(WebEmSession &session, const request &req, reply &rep)
		{
			if (g_bLlmMCPSupport == false)
			{
				_log.Log(LOG_ERROR, "MCP: MCP access requested (IP: %s), but service disabled with -nomcp !", session.remote_host.c_str());
				rep = reply::stock_reply(reply::service_unavailable);
				return;
			}
			_log.Debug(DEBUG_RECEIVED, "MCP: Post (%d): %s (%s)", req.content_length, req.content.c_str(), req.uri.c_str());
			// Check if the request is valid
			std::string sProtocolRequestHeader;
			if (req.get_req_header(&req, "Accept") != nullptr)
			{
				std::string accept = req.get_req_header(&req, "Accept");
				if (accept.find("text/event-stream") == std::string::npos && accept.find("application/json") == std::string::npos)
				{
					_log.Debug(DEBUG_WEBSERVER, "MCP: Invalid Accept header: %s", accept.c_str());
					rep = reply::stock_reply(reply::bad_request);
					return;
				}
			}
			// Check if the request has the MCP-PROTOCOL-VERSION header
			// If not, we assume the client is using the latest version
			// If it is present, we check if it matches the expected version
			if (req.get_req_header(&req, "mcp-protocol-version:") != nullptr)
			{
				sProtocolRequestHeader = req.get_req_header(&req, "mcp-protocol-version:");
				if (sProtocolRequestHeader != "2025-06-18")
				{
					_log.Debug(DEBUG_WEBSERVER, "MCP: MCP-PROTOCOL-VERSION not supported: %s", sProtocolRequestHeader.c_str());
					rep = reply::stock_reply(reply::bad_request);
					return;
				}
			}
			// Check if the request is a POST request
			if (req.method != "POST")
			{
				// VScode MCP client does sends GET's (maybe other do as well?)
				// It does this to look for asynchronous notifications support
				// but we don't support that yet, so we return bad request
				// And the MCP spec does not support GET for requests anyway
				_log.Debug(DEBUG_WEBSERVER, "MCP: Invalid method: %s", req.method.c_str());
				rep = reply::stock_reply(reply::bad_request);
				return;
			}

			Json::Value jsonRequest;
			std::string sParseErr;
			if (!mcp::validRPC(req.content, jsonRequest, sParseErr))
			{
				_log.Debug(DEBUG_WEBSERVER, "MCP: Invalid JSON-RPC request: %s", sParseErr.c_str());
				rep = reply::stock_reply(reply::bad_request);	// Or should we send a valid JSON-RPC response with error -32700 (Parse error)?
				return;
			}

			//_log.Debug(DEBUG_RECEIVED, "MCP: Parsed JSON Request content: %s", jsonRequest.toStyledString().c_str());

			// Check if the method is supported and handle it
			std::string sReqMethod = jsonRequest["method"].asString();
			_log.Debug(DEBUG_WEBSERVER, "MCP: Request method: %s", sReqMethod.c_str());

			if (sReqMethod.find("notifications/") != std::string::npos)
			{
				// Handle notifications, notifications don't have an ID and do not require a response
				// MCP HTTP transport expects 202 Accepted (not 204 No Content) for notifications
				_log.Debug(DEBUG_WEBSERVER, "MCP: Handling notification %s (do nothing).", sReqMethod.c_str());
				rep = reply::stock_reply(reply::accepted);
				return;
			}

			Json::Value jsonRPCRep;
			jsonRPCRep["jsonrpc"] = "2.0";

			// Check if the request has an ID
			if (jsonRequest.isMember("id"))
			{
				if (jsonRequest["id"].isInt())
				{
					jsonRPCRep["id"] = jsonRequest["id"].asInt();

				}
				else if (jsonRequest["id"].isString())
				{
					jsonRPCRep["id"] = jsonRequest["id"].asString();
				}
				else
				{
					_log.Debug(DEBUG_WEBSERVER, "MCP: Invalid ID type in request (must be number or string).");
					rep = reply::stock_reply(reply::bad_request);
					return;
				}
			}
			else
			{
				_log.Debug(DEBUG_WEBSERVER, "MCP: Missing ID in request!");
				rep = reply::stock_reply(reply::bad_request);
				return;
			};

			if (sReqMethod == "ping")
			{
				_log.Debug(DEBUG_WEBSERVER, "MCP: Handling ping request (return empty result).");
				jsonRPCRep["result"] = Json::Value(Json::objectValue);
			}
			else if (sReqMethod == "initialize")
			{
				mcp::McpInitialize(jsonRequest, jsonRPCRep);
			}
			else if (sReqMethod == "tools/list")
			{
				mcp::McpToolsList(jsonRequest, jsonRPCRep);
			}
			else if (sReqMethod == "tools/call")
			{
				mcp::McpToolsCall(jsonRequest, jsonRPCRep);
			}
			else if (sReqMethod == "resources/list")
			{
				mcp::McpResourcesList(jsonRequest, jsonRPCRep);
			}
			else if (sReqMethod == "resources/templates/list")
			{
				mcp::McpResourcesTemplatesList(jsonRequest, jsonRPCRep);
			}
			else if (sReqMethod == "resources/read")
			{
				mcp::McpResourcesRead(jsonRequest, jsonRPCRep);
			}
			else if (sReqMethod == "prompts/list")
			{
				mcp::McpPromptsList(jsonRequest, jsonRPCRep);
			}
			else if (sReqMethod == "prompts/get")
			{
				mcp::McpPromptsGet(jsonRequest, jsonRPCRep);
			}
			else
			{
				_log.Debug(DEBUG_WEBSERVER, "MCP: Unsupported method: %s", sReqMethod.c_str());
				rep = reply::stock_reply(reply::not_implemented);
				return;
			}
			// Set response content
			rep.content = jsonRPCRep.toStyledString();
			rep.status = reply::ok;

			// Set headers
			reply::add_header(&rep, "Content-Type", "application/json");	// "text/event-stream" is also an option if we want to support SSE
			//reply::add_header(&rep, "Cache-Control", "no-cache");
			//reply::add_header(&rep, "Connection", "keep-alive");
		}

	} // namespace server
} // namespace http

namespace mcp		// Model Context Protocol
{
	static const char* const kVarTypeNames[] = { "Integer", "Float", "String", "Date", "Time" };
	static const int kVarTypeCount = 5;

	void McpInitialize(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		_log.Debug(DEBUG_WEBSERVER, "MCP: Handling initialize request.");

		// Prepare the result for the initialize method
		jsonRPCRep["result"]["protocolVersion"] = "2025-06-18";
		//jsonRPCRep["result"]["capabilities"]["logging"] = Json::Value(Json::objectValue);
		//jsonRPCRep["result"]["capabilities"]["completion"] = Json::Value(Json::objectValue);
		jsonRPCRep["result"]["capabilities"]["prompts"] = Json::Value(Json::objectValue);
		//jsonRPCRep["result"]["capabilities"]["prompts"]["listChanged"] = true;
		jsonRPCRep["result"]["capabilities"]["resources"] = Json::Value(Json::objectValue);
		//jsonRPCRep["result"]["capabilities"]["resources"]["subscribe"] = true;
		//jsonRPCRep["result"]["capabilities"]["resources"]["listChanged"] = true;
		jsonRPCRep["result"]["capabilities"]["tools"] = Json::Value(Json::objectValue);
		//jsonRPCRep["result"]["capabilities"]["tools"]["listChanged"] = true;

		jsonRPCRep["result"]["serverInfo"]["name"] = "DomoticzMcp";
		jsonRPCRep["result"]["serverInfo"]["title"] = "Domoticz MCP Server";
		jsonRPCRep["result"]["serverInfo"]["version"] = "0.1.0";
		jsonRPCRep["result"]["serverInfo"]["description"] = "Domoticz is a home automation system that lets you monitor, configure and control various devices from different hardware in your home. Devices like switches (for example light switches or smart plugs) can be used to control (other) devices and devices like sensors (for example temperature sensors or contact sensors) can provide information about their state.";

		//jsonRPCRep["result"]["instructions"] = "Any additional instructions for the client can be provided here";
	}

	void McpToolsList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		_log.Debug(DEBUG_WEBSERVER, "MCP: Handling tools/list request.");

		// Prepare the result for the tools/list method
		jsonRPCRep["result"]["tools"] = Json::Value(Json::arrayValue);
		Json::Value tool;
		// Get Switch State tool
		tool["name"] = "get_switch_state";
		tool["title"] = "See the state of a switch in the system";
		tool["description"] = "Get the current state of a given switch in the system";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["switchname"]["type"] = "string";
		tool["inputSchema"]["properties"]["switchname"]["description"] = "Name of the switch to query";
		tool["inputSchema"]["required"].append("switchname");
		jsonRPCRep["result"]["tools"].append(tool);
		// Toggle switch state tool
		tool.clear();
		tool["name"] = "toggle_switch_state";
		tool["title"] = "Toggle the state of a switch in the system";
		tool["description"] = "Toggle the state of a given switch in the system";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["switchname"]["type"] = "string";
		tool["inputSchema"]["properties"]["switchname"]["description"] = "Name of the switch to toggle";
		tool["inputSchema"]["required"].append("switchname");
		jsonRPCRep["result"]["tools"].append(tool);
		// Get Sensor Value tool
		tool.clear();
		tool["name"] = "get_sensor_value";
		tool["title"] = "Get the value of a sensor in the system";
		tool["description"] = "Retrieve the current value of a specified sensor in the system";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["sensorname"]["type"] = "string";
		tool["inputSchema"]["properties"]["sensorname"]["description"] = "Name of the sensor to query";
		tool["inputSchema"]["required"].append("sensorname");
		jsonRPCRep["result"]["tools"].append(tool);
		// Set Setpoint Value tool
		tool.clear();
		tool["name"] = "set_setpoint_value";
		tool["title"] = "Set the target setpoint of a thermostat in the system";
		tool["description"] = "Set the target setpoint of a given thermostat in the system";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["thermostatname"]["type"] = "string";
		tool["inputSchema"]["properties"]["thermostatname"]["description"] = "Name of the thermostat to set";
		tool["inputSchema"]["properties"]["setpoint"]["type"] = "number";
		tool["inputSchema"]["properties"]["setpoint"]["description"] = "Temperature setpoint as an number";
		tool["inputSchema"]["required"].append("thermostatname");
		tool["inputSchema"]["required"].append("setpoint");
		jsonRPCRep["result"]["tools"].append(tool);
		// Get logging tool
		tool.clear();
		tool["name"] = "get_logging";
		tool["title"] = "Get the logging information";
		tool["description"] = "Retrieve the current logging information";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["logdate"]["type"] = "number";
		tool["inputSchema"]["properties"]["logdate"]["description"] = "The (Unixtimestamp) date and time from which to retrieve the logs (optional, default is 0, which means all logs)";
		jsonRPCRep["result"]["tools"].append(tool);
		// Get Floorplan(s) tool
		tool.clear();
		tool["name"] = "get_floorplan";
		tool["title"] = "Get the floorplan";
		tool["description"] = "Retrieve the specific floorplan within the system";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["floorplan"]["type"] = "string";
		tool["inputSchema"]["properties"]["floorplan"]["description"] = "The name of the floorplan to retrieve";
		jsonRPCRep["result"]["tools"].append(tool);
		// Get Status tool
		tool.clear();
		tool["name"] = "get_status";
		tool["title"] = "Get the system status of Domoticz";
		tool["description"] = "Retrieve the current system status including version, uptime, sunrise/sunset times and device/hardware counts. Use this tool to check if the Domoticz instance is running and healthy.";
		tool["inputSchema"]["type"] = "object";
		jsonRPCRep["result"]["tools"].append(tool);
		// Search Devices tool
		tool.clear();
		tool["name"] = "search_devices";
		tool["title"] = "Search for devices";
		tool["description"] = "Search for devices whose name, type or subtype contains the given query string (case-insensitive substring match). Use this tool to discover exact device names before using other tools like get_switch_state or toggle_switch_state that require exact names.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["query"]["type"] = "string";
		tool["inputSchema"]["properties"]["query"]["description"] = "Search string to match against device name, type or subtype (case-insensitive substring match)";
		tool["inputSchema"]["properties"]["filter"]["type"] = "string";
		tool["inputSchema"]["properties"]["filter"]["description"] = "Optional device type filter: light, temp, weather, utility (matches Domoticz device categories)";
		tool["inputSchema"]["required"].append("query");
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "get_all_devices";
		tool["title"] = "List all devices";
		tool["description"] = "Return a list of all used devices in the system, optionally filtered by category. Use this to discover all available devices.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["filter"]["type"] = "string";
		tool["inputSchema"]["properties"]["filter"]["description"] = "Optional category filter: light, temp, weather, utility (leave empty for all)";
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "get_device";
		tool["title"] = "Get device details";
		tool["description"] = "Return full details for a single device by name or idx. At least one of name or idx must be provided.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["name"]["type"] = "string";
		tool["inputSchema"]["properties"]["name"]["description"] = "Device name to look up";
		tool["inputSchema"]["properties"]["idx"]["type"] = "integer";
		tool["inputSchema"]["properties"]["idx"]["description"] = "Device IDX (numeric identifier) to look up";
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "rename_device";
		tool["title"] = "Rename a device";
		tool["description"] = "Rename a device by its current name to a new name.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["switchname"]["type"] = "string";
		tool["inputSchema"]["properties"]["switchname"]["description"] = "Current name of the device";
		tool["inputSchema"]["properties"]["new_name"]["type"] = "string";
		tool["inputSchema"]["properties"]["new_name"]["description"] = "New name for the device";
		tool["inputSchema"]["required"].append("switchname");
		tool["inputSchema"]["required"].append("new_name");
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "delete_device";
		tool["title"] = "Delete (hide) a device";
		tool["description"] = "Hide a device by setting its Used flag to 0. The device is not permanently deleted; it can be re-enabled. Use with caution.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["switchname"]["type"] = "string";
		tool["inputSchema"]["properties"]["switchname"]["description"] = "Name of the device to hide/delete";
		tool["inputSchema"]["required"].append("switchname");
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "create_virtual_sensor";
		tool["title"] = "Create a virtual sensor";
		tool["description"] = "Create a new virtual sensor attached to a virtual hardware instance. The hw_idx must refer to a virtual (dummy) hardware. sensortype is the mapped sensor type integer (e.g. 80=Temperature, 81=Humidity, 5=Text, 6=Switch).";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["hw_idx"]["type"] = "integer";
		tool["inputSchema"]["properties"]["hw_idx"]["description"] = "IDX of the virtual/dummy hardware to attach the sensor to";
		tool["inputSchema"]["properties"]["sensorname"]["type"] = "string";
		tool["inputSchema"]["properties"]["sensorname"]["description"] = "Name for the new virtual sensor";
		tool["inputSchema"]["properties"]["sensortype"]["type"] = "integer";
		tool["inputSchema"]["properties"]["sensortype"]["description"] = "Sensor type integer (80=Temperature, 81=Humidity, 5=Text, 6=Switch, 113=Counter, etc.)";
		tool["inputSchema"]["required"].append("hw_idx");
		tool["inputSchema"]["required"].append("sensorname");
		tool["inputSchema"]["required"].append("sensortype");
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "update_device_value";
		tool["title"] = "Update a device value";
		tool["description"] = "Directly update the nValue and/or sValue of a device (useful for virtual sensors).";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["switchname"]["type"] = "string";
		tool["inputSchema"]["properties"]["switchname"]["description"] = "Name of the device to update";
		tool["inputSchema"]["properties"]["nvalue"]["type"] = "integer";
		tool["inputSchema"]["properties"]["nvalue"]["description"] = "Numeric value to set";
		tool["inputSchema"]["properties"]["svalue"]["type"] = "string";
		tool["inputSchema"]["properties"]["svalue"]["description"] = "String value to set (optional)";
		tool["inputSchema"]["required"].append("switchname");
		tool["inputSchema"]["required"].append("nvalue");
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "get_device_history";
		tool["title"] = "Get device history";
		tool["description"] = "Retrieve the recent log history for a device. Returns up to 50 most recent entries.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["switchname"]["type"] = "string";
		tool["inputSchema"]["properties"]["switchname"]["description"] = "Name of the device";
		tool["inputSchema"]["properties"]["log_type"]["type"] = "string";
		tool["inputSchema"]["properties"]["log_type"]["description"] = "Log type: switch (default), text, or graph";
		tool["inputSchema"]["required"].append("switchname");
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "get_user_variables";
		tool["title"] = "List all user variables";
		tool["description"] = "List all user-defined variables (name, type, value, last update).";
		tool["inputSchema"]["type"] = "object";
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "add_user_variable";
		tool["title"] = "Create a user variable";
		tool["description"] = "Create a new user variable. Types: 0=Integer, 1=Float, 2=String, 3=Date (DD/MM/YYYY), 4=Time (HH:MM).";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["name"]["type"] = "string";
		tool["inputSchema"]["properties"]["name"]["description"] = "Variable name (must be unique)";
		tool["inputSchema"]["properties"]["vtype"]["type"] = "integer";
		tool["inputSchema"]["properties"]["vtype"]["description"] = "Variable type: 0=Integer, 1=Float, 2=String, 3=Date, 4=Time";
		tool["inputSchema"]["properties"]["value"]["type"] = "string";
		tool["inputSchema"]["properties"]["value"]["description"] = "Initial value for the variable";
		tool["inputSchema"]["required"].append("name");
		tool["inputSchema"]["required"].append("vtype");
		tool["inputSchema"]["required"].append("value");
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "update_user_variable";
		tool["title"] = "Update a user variable";
		tool["description"] = "Update the value (and optionally type) of an existing user variable.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["name"]["type"] = "string";
		tool["inputSchema"]["properties"]["name"]["description"] = "Variable name to update";
		tool["inputSchema"]["properties"]["value"]["type"] = "string";
		tool["inputSchema"]["properties"]["value"]["description"] = "New value";
		tool["inputSchema"]["properties"]["vtype"]["type"] = "integer";
		tool["inputSchema"]["properties"]["vtype"]["description"] = "New type (optional): 0=Integer, 1=Float, 2=String, 3=Date, 4=Time";
		tool["inputSchema"]["required"].append("name");
		tool["inputSchema"]["required"].append("value");
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "delete_user_variable";
		tool["title"] = "Delete a user variable";
		tool["description"] = "Delete a user variable by name.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["name"]["type"] = "string";
		tool["inputSchema"]["properties"]["name"]["description"] = "Name of the variable to delete";
		tool["inputSchema"]["required"].append("name");
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "add_log_message";
		tool["title"] = "Write a log message";
		tool["description"] = "Write a message to the Domoticz system log. Messages are prefixed with 'MCP: ' to identify AI-originated entries.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["message"]["type"] = "string";
		tool["inputSchema"]["properties"]["message"]["description"] = "The message text to log";
		tool["inputSchema"]["properties"]["level"]["type"] = "string";
		tool["inputSchema"]["properties"]["level"]["description"] = "Log level: normal (default), status, error";
		tool["inputSchema"]["required"].append("message");
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "send_notification";
		tool["title"] = "Send a push notification";
		tool["description"] = "Send a push notification via all configured Domoticz notification services (e.g. Telegram, Pushover, email). WARNING: This sends to real devices. Do not call repeatedly or for testing.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["subject"]["type"] = "string";
		tool["inputSchema"]["properties"]["subject"]["description"] = "Notification title/subject";
		tool["inputSchema"]["properties"]["body"]["type"] = "string";
		tool["inputSchema"]["properties"]["body"]["description"] = "Notification body text";
		tool["inputSchema"]["properties"]["priority"]["type"] = "integer";
		tool["inputSchema"]["properties"]["priority"]["description"] = "Priority: -2 (very low) to 2 (emergency), default 0 (normal)";
		tool["inputSchema"]["required"].append("subject");
		tool["inputSchema"]["required"].append("body");
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "get_events";
		tool["title"] = "List event scripts";
		tool["description"] = "List all event scripts (dzVents, Lua, Blockly, Python) with their name, interpreter, and enabled status.";
		tool["inputSchema"]["type"] = "object";
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "get_event";
		tool["title"] = "Get event script source";
		tool["description"] = "Get the full source code of a specific event script by name.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["event_name"]["type"] = "string";
		tool["inputSchema"]["properties"]["event_name"]["description"] = "Name of the event script to retrieve";
		tool["inputSchema"]["required"].append("event_name");
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "create_event";
		tool["title"] = "Create an event script";
		tool["description"] = "Create a new event script. interpreter must be one of: Lua, dzVents, Python, Blockly.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["name"]["type"] = "string";
		tool["inputSchema"]["properties"]["name"]["description"] = "Script name (must be unique)";
		tool["inputSchema"]["properties"]["interpreter"]["type"] = "string";
		tool["inputSchema"]["properties"]["interpreter"]["description"] = "Script interpreter: Lua, dzVents, Python, or Blockly";
		tool["inputSchema"]["properties"]["code"]["type"] = "string";
		tool["inputSchema"]["properties"]["code"]["description"] = "Script source code";
		tool["inputSchema"]["properties"]["enabled"]["type"] = "boolean";
		tool["inputSchema"]["properties"]["enabled"]["description"] = "Whether to enable the script immediately (default true)";
		tool["inputSchema"]["required"].append("name");
		tool["inputSchema"]["required"].append("interpreter");
		tool["inputSchema"]["required"].append("code");
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "update_event";
		tool["title"] = "Update an event script";
		tool["description"] = "Update an existing event script's code, enabled state, or name. At least one of code/enabled/new_name must be provided.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["event_name"]["type"] = "string";
		tool["inputSchema"]["properties"]["event_name"]["description"] = "Current name of the event script";
		tool["inputSchema"]["properties"]["code"]["type"] = "string";
		tool["inputSchema"]["properties"]["code"]["description"] = "New script source code (optional)";
		tool["inputSchema"]["properties"]["enabled"]["type"] = "boolean";
		tool["inputSchema"]["properties"]["enabled"]["description"] = "Enable or disable the script (optional)";
		tool["inputSchema"]["properties"]["new_name"]["type"] = "string";
		tool["inputSchema"]["properties"]["new_name"]["description"] = "Rename the script to this name (optional)";
		tool["inputSchema"]["required"].append("event_name");
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "set_switch_state";
		tool["title"] = "Set a switch On or Off";
		tool["description"] = "Explicitly turn a switch On or Off by name (without toggling).";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["switchname"]["type"] = "string";
		tool["inputSchema"]["properties"]["switchname"]["description"] = "Name of the switch";
		tool["inputSchema"]["properties"]["state"]["type"] = "string";
		tool["inputSchema"]["properties"]["state"]["description"] = "Desired state: On or Off";
		tool["inputSchema"]["properties"]["state"]["enum"].append("On");
		tool["inputSchema"]["properties"]["state"]["enum"].append("Off");
		tool["inputSchema"]["required"].append("switchname");
		tool["inputSchema"]["required"].append("state");
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "set_dimmer_level";
		tool["title"] = "Set a dimmer level";
		tool["description"] = "Set a dimmable light to a specific brightness level (0-100).";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["switchname"]["type"] = "string";
		tool["inputSchema"]["properties"]["switchname"]["description"] = "Name of the dimmable device";
		tool["inputSchema"]["properties"]["level"]["type"] = "integer";
		tool["inputSchema"]["properties"]["level"]["description"] = "Brightness level 0-100";
		tool["inputSchema"]["required"].append("switchname");
		tool["inputSchema"]["required"].append("level");
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "control_blinds";
		tool["title"] = "Control a blind or shutter";
		tool["description"] = "Send an Open, Close, or Stop command to a blind or shutter device.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["switchname"]["type"] = "string";
		tool["inputSchema"]["properties"]["switchname"]["description"] = "Name of the blind/shutter device";
		tool["inputSchema"]["properties"]["command"]["type"] = "string";
		tool["inputSchema"]["properties"]["command"]["description"] = "Command to send: Open, Close, or Stop";
		tool["inputSchema"]["properties"]["command"]["enum"].append("Open");
		tool["inputSchema"]["properties"]["command"]["enum"].append("Close");
		tool["inputSchema"]["properties"]["command"]["enum"].append("Stop");
		tool["inputSchema"]["required"].append("switchname");
		tool["inputSchema"]["required"].append("command");
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "set_color_brightness";
		tool["title"] = "Set color and brightness on an RGB light";
		tool["description"] = "Set the hue and brightness of an RGB or color light device.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["switchname"]["type"] = "string";
		tool["inputSchema"]["properties"]["switchname"]["description"] = "Name of the color light device";
		tool["inputSchema"]["properties"]["hue"]["type"] = "integer";
		tool["inputSchema"]["properties"]["hue"]["description"] = "Hue angle 0-360";
		tool["inputSchema"]["properties"]["brightness"]["type"] = "integer";
		tool["inputSchema"]["properties"]["brightness"]["description"] = "Brightness level 0-100";
		tool["inputSchema"]["properties"]["iswhite"]["type"] = "boolean";
		tool["inputSchema"]["properties"]["iswhite"]["description"] = "If true, use white mode instead of RGB color (default false)";
		tool["inputSchema"]["required"].append("switchname");
		tool["inputSchema"]["required"].append("hue");
		tool["inputSchema"]["required"].append("brightness");
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "set_color_temperature";
		tool["title"] = "Set color temperature on a tunable-white light";
		tool["description"] = "Set the color temperature of a tunable-white light in Kelvin (e.g. 2700 for warm white, 6500 for cool white).";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["switchname"]["type"] = "string";
		tool["inputSchema"]["properties"]["switchname"]["description"] = "Name of the tunable-white light device";
		tool["inputSchema"]["properties"]["kelvin"]["type"] = "integer";
		tool["inputSchema"]["properties"]["kelvin"]["description"] = "Color temperature in Kelvin (2700=warm white, 6500=cool white)";
		tool["inputSchema"]["required"].append("switchname");
		tool["inputSchema"]["required"].append("kelvin");
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "get_scenes";
		tool["title"] = "List all scenes and groups";
		tool["description"] = "Return a list of all scenes and groups configured in Domoticz.";
		tool["inputSchema"]["type"] = "object";
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "switch_scene";
		tool["title"] = "Activate or deactivate a scene";
		tool["description"] = "Activate (On) or deactivate (Off) a Domoticz scene or group by name.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["scenename"]["type"] = "string";
		tool["inputSchema"]["properties"]["scenename"]["description"] = "Name of the scene or group";
		tool["inputSchema"]["properties"]["command"]["type"] = "string";
		tool["inputSchema"]["properties"]["command"]["description"] = "Command: On or Off";
		tool["inputSchema"]["properties"]["command"]["enum"].append("On");
		tool["inputSchema"]["properties"]["command"]["enum"].append("Off");
		tool["inputSchema"]["required"].append("scenename");
		tool["inputSchema"]["required"].append("command");
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "get_rooms";
		tool["title"] = "List all rooms";
		tool["description"] = "Return a list of all rooms (plans) configured in Domoticz.";
		tool["inputSchema"]["type"] = "object";
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "get_room_devices";
		tool["title"] = "Get devices in a room";
		tool["description"] = "Return all devices assigned to a specific room (plan) by room name.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["roomname"]["type"] = "string";
		tool["inputSchema"]["properties"]["roomname"]["description"] = "Name of the room (plan)";
		tool["inputSchema"]["required"].append("roomname");
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "get_scene_devices";
		tool["title"] = "Get devices in a scene";
		tool["description"] = "Return all devices that belong to a specific scene or group.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["scenename"]["type"] = "string";
		tool["inputSchema"]["properties"]["scenename"]["description"] = "Name of the scene or group";
		tool["inputSchema"]["required"].append("scenename");
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "get_hardware";
		tool["title"] = "List all hardware";
		tool["description"] = "Return a list of all configured hardware adapters in Domoticz.";
		tool["inputSchema"]["type"] = "object";
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "get_settings";
		tool["title"] = "Get system settings";
		tool["description"] = "Return key Domoticz system settings and preferences.";
		tool["inputSchema"]["type"] = "object";
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "get_sun_times";
		tool["title"] = "Get sunrise and sunset times";
		tool["description"] = "Return today's sunrise, sunset, dawn, dusk, solar noon and related solar data.";
		tool["inputSchema"]["type"] = "object";
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "get_cameras";
		tool["title"] = "List all cameras";
		tool["description"] = "Return a list of all configured cameras (name, address, port, enabled status). Passwords are never returned.";
		tool["inputSchema"]["type"] = "object";
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "get_floorplans";
		tool["title"] = "List all floorplans";
		tool["description"] = "Return a list of available floorplan names and IDs (no image data). Use get_floorplan to retrieve a specific floorplan image.";
		tool["inputSchema"]["type"] = "object";
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "get_users";
		tool["title"] = "List all users";
		tool["description"] = "Return a list of all system users with their username, rights and active status. Passwords are never returned.";
		tool["inputSchema"]["type"] = "object";
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "get_security_status";
		tool["title"] = "Get security panel status";
		tool["description"] = "Return the current Domoticz security panel status (Disarmed, Armed Home, or Armed Away).";
		tool["inputSchema"]["type"] = "object";
		jsonRPCRep["result"]["tools"].append(tool);

		tool.clear();
		tool["name"] = "set_security_status";
		tool["title"] = "Set security panel status";
		tool["description"] = "Set the Domoticz security panel status. Requires the security PIN code. status: 0=Disarmed, 1=Armed Home, 2=Armed Away.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["status"]["type"] = "integer";
		tool["inputSchema"]["properties"]["status"]["description"] = "New security status: 0=Disarmed, 1=Armed Home, 2=Armed Away";
		tool["inputSchema"]["properties"]["seccode"]["type"] = "string";
		tool["inputSchema"]["properties"]["seccode"]["description"] = "The security panel PIN code (plaintext)";
		tool["inputSchema"]["required"].append("status");
		tool["inputSchema"]["required"].append("seccode");
		jsonRPCRep["result"]["tools"].append(tool);
	}

	void McpToolsCall(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		// Check if the required parameters are present
		if (!jsonRequest.isMember("params") || !jsonRequest["params"].isMember("name"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: Missing required tool parameter 'name' in tools/{tool} request.");
			jsonRPCRep["error"]["code"] = JSONRPC_INVALID_PARAMETER;
			jsonRPCRep["error"]["message"] = "Missing required parameter 'name'";
			return;
		}
		// Handle the tool call based on the name
		std::string sMethodName = jsonRequest["params"]["name"].asString();

		_log.Debug(DEBUG_WEBSERVER, "MCP: Handling tools/{%s} request.", sMethodName.c_str());

		static const std::unordered_map<std::string, std::function<bool(const Json::Value&, Json::Value&)>> kToolDispatch = {
			{ "get_switch_state",       getSwitchState },
			{ "toggle_switch_state",    toggleSwitchState },
			{ "get_sensor_value",       getSensorValue },
			{ "set_setpoint_value",     setThermostatSetpoint },
			{ "get_logging",            getLogging },
			{ "get_floorplan",          getFloorplan },
			{ "get_status",             getStatus },
			{ "search_devices",         searchDevices },
			{ "set_switch_state",       setSwitchState },
			{ "set_dimmer_level",       setDimmerLevel },
			{ "control_blinds",         controlBlinds },
			{ "set_color_brightness",   setColorBrightness },
			{ "set_color_temperature",  setColorTemperature },
			{ "get_all_devices",        getAllDevices },
			{ "get_device",             getDevice },
			{ "rename_device",          renameDevice },
			{ "delete_device",          deleteDevice },
			{ "create_virtual_sensor",  createVirtualSensor },
			{ "update_device_value",    updateDeviceValue },
			{ "get_device_history",     getDeviceHistory },
			{ "get_scenes",             getScenes },
			{ "switch_scene",           switchScene },
			{ "get_rooms",              getRooms },
			{ "get_room_devices",       getRoomDevices },
			{ "get_scene_devices",      getSceneDevices },
			{ "get_user_variables",     getUserVariables },
			{ "add_user_variable",      addUserVariable },
			{ "update_user_variable",   updateUserVariable },
			{ "delete_user_variable",   deleteUserVariable },
			{ "get_hardware",           getHardware },
			{ "get_settings",           getSystemSettings },
			{ "get_sun_times",          getSunTimes },
			{ "get_cameras",            getCameras },
			{ "get_floorplans",         getFloorplans },
			{ "get_users",              getUsers },
			{ "add_log_message",        addLogMessage },
			{ "send_notification",      sendNotification },
			{ "get_security_status",    getSecurityStatus },
			{ "set_security_status",    setSecurityStatus },
			{ "get_events",             getEvents },
			{ "get_event",              getEvent },
			{ "create_event",           createEvent },
			{ "update_event",           updateEvent },
		};

		auto it = kToolDispatch.find(sMethodName);
		if (it != kToolDispatch.end())
		{
			if (!it->second(jsonRequest, jsonRPCRep))
			{
				if (!jsonRPCRep.isMember("error"))
				{
					jsonRPCRep["error"]["code"] = JSONRPC_INVALID_PARAMETER;
					jsonRPCRep["error"]["message"] = "Error executing tool: " + sMethodName;
				}
			}
		}
		else
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: Unsupported tool name: %s", sMethodName.c_str());
			jsonRPCRep["error"]["code"] = JSONRPC_METHOD_NOT_FOUND;
			jsonRPCRep["error"]["message"] = "Method not found";
		}
		//_log.Debug(DEBUG_WEBSERVER, "McpToolsCall: Returning result for method (%s): %s", sMethodName.c_str(), jsonRPCRep.toStyledString().c_str());
	}

	void McpResourcesList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		_log.Debug(DEBUG_WEBSERVER, "MCP: Handling resources/list request.");

		// Prepare the result for the resources/list method
		jsonRPCRep["result"]["resources"] = Json::Value(Json::arrayValue);

		// --- Aggregate resources (fixed, always present) ---
		{
			Json::Value resource;
			resource["uri"] = "domoticz://devices";
			resource["name"] = "All Devices";
			resource["title"] = "All Devices";
			resource["description"] = "Summary of all used devices in the system";
			resource["mimeType"] = "text/plain";
			jsonRPCRep["result"]["resources"].append(resource);
		}
		{
			Json::Value resource;
			resource["uri"] = "domoticz://rooms";
			resource["name"] = "Rooms";
			resource["title"] = "Rooms";
			resource["description"] = "All configured rooms/plans";
			resource["mimeType"] = "text/plain";
			jsonRPCRep["result"]["resources"].append(resource);
		}
		{
			Json::Value resource;
			resource["uri"] = "domoticz://scenes";
			resource["name"] = "Scenes";
			resource["title"] = "Scenes";
			resource["description"] = "All scenes and groups";
			resource["mimeType"] = "text/plain";
			jsonRPCRep["result"]["resources"].append(resource);
		}
		{
			Json::Value resource;
			resource["uri"] = "domoticz://user-variables";
			resource["name"] = "User Variables";
			resource["title"] = "User Variables";
			resource["description"] = "All user variables";
			resource["mimeType"] = "text/plain";
			jsonRPCRep["result"]["resources"].append(resource);
		}
		{
			Json::Value resource;
			resource["uri"] = "domoticz://events";
			resource["name"] = "Event Scripts";
			resource["title"] = "Event Scripts";
			resource["description"] = "All automation event scripts";
			resource["mimeType"] = "text/plain";
			jsonRPCRep["result"]["resources"].append(resource);
		}
		{
			Json::Value resource;
			resource["uri"] = "domoticz://security";
			resource["name"] = "Security";
			resource["title"] = "Security";
			resource["description"] = "Security panel status";
			resource["mimeType"] = "text/plain";
			jsonRPCRep["result"]["resources"].append(resource);
		}
		{
			Json::Value resource;
			resource["uri"] = "domoticz://settings";
			resource["name"] = "Settings";
			resource["title"] = "Settings";
			resource["description"] = "System configuration (key subset)";
			resource["mimeType"] = "text/plain";
			jsonRPCRep["result"]["resources"].append(resource);
		}
		{
			Json::Value resource;
			resource["uri"] = "domoticz://log";
			resource["name"] = "System Log";
			resource["title"] = "System Log";
			resource["description"] = "Recent system log entries";
			resource["mimeType"] = "text/plain";
			jsonRPCRep["result"]["resources"].append(resource);
		}

		// --- Per-device resources using domoticz://device/{idx} ---
		Json::Value jsonDevices;
		m_webservers.GetJSonDevices(jsonDevices, "", "", "", "", "", "", false, false, false, 0, "", "");	// To-Do: Use Database instead of WebServerHelper
		if (jsonDevices.isObject() && jsonDevices.isMember("result"))
		{
			for (const auto &device : jsonDevices["result"])
			{
				//_log.Debug(DEBUG_WEBSERVER, "MCP: ResourcesList: Got device: %s", device.toStyledString().c_str());
				if (device.isObject() && device.isMember("idx") && device.isMember("HardwareName") && device.isMember("ID") &&
					device.isMember("Name") && device.isMember("Type") && device.isMember("SubType") && device.isMember("Data")	&&
					device.isMember("Used") && atoi(device["Used"].asString().c_str()) == 1)
				{
					Json::Value resource;
					resource["uri"] = "domoticz://device/" + device["idx"].asString();
					resource["name"] = device["Name"].asString();
					resource["title"] = device["Name"].asString() + " (" + device["HardwareName"].asString() + " - " + device["Type"].asString() + " - " + device["SubType"].asString() + ")";
					resource["description"] = "A Sensor from the " + device["HardwareName"].asString() + " hardware of Type " + device["Type"].asString() +
												" and subtype " + device["SubType"].asString() + " called " + device["Name"].asString() +
												" with ID " + device["ID"].asString() + " and IDX " + device["idx"].asString();
					resource["mimeType"] = "text/plain";
					Json::Value meta;
					meta["hardware"] = device["HardwareName"].asString();
					meta["type"] = device["Type"].asString();
					meta["subtype"] = device["SubType"].asString();
					meta["idx"] = atoi(device["idx"].asString().c_str());
					meta["id"] = device["ID"].asString();
					resource["_meta"] = meta;
					jsonRPCRep["result"]["resources"].append(resource);
				}
			}
		}
		// Add any available floorplans as resources too
		auto result = m_sql.safe_query("SELECT ID, Name FROM Floorplans");
		if (!result.empty())
		{
			for (const auto &row : result)
			{
				Json::Value resource;
				std::string idx = row[0];
				std::string sName = row[1];
				resource["uri"] = "floorplan:///image/" + idx;
				resource["name"] = sName;
				resource["title"] = sName + " (Floorplan)";
				resource["description"] = "A Floorplan called " + sName + " with IDX " + idx;
				resource["mimeType"] = "image/*"; // unknown image type
				Json::Value meta;
				meta["idx"] = atoi(idx.c_str());
				resource["_meta"] = meta;
				jsonRPCRep["result"]["resources"].append(resource);
			}
		}

		//_log.Debug(DEBUG_WEBSERVER, "MCP: ResourcesList: Following resources offered:\n%s", jsonRPCRep.toStyledString().c_str());
		_log.Debug(DEBUG_WEBSERVER, "MCP: ResourcesList: Number of resources offered: %d", jsonRPCRep["result"]["resources"].size());
	}

	void McpResourcesTemplatesList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		_log.Debug(DEBUG_WEBSERVER, "MCP: Handling resources/templates/list request.");

		// Prepare the result for the resources/templates/list method
		jsonRPCRep["result"]["resourceTemplates"] = Json::Value(Json::arrayValue);

		Json::Value tmpl;
		tmpl["uriTemplate"] = "domoticz://device/{idx}";
		tmpl["name"] = "Device by IDX";
		tmpl["mimeType"] = "text/plain";
		jsonRPCRep["result"]["resourceTemplates"].append(tmpl);

		tmpl.clear();
		tmpl["uriTemplate"] = "domoticz://room/{idx}";
		tmpl["name"] = "Room Devices by IDX";
		tmpl["mimeType"] = "text/plain";
		jsonRPCRep["result"]["resourceTemplates"].append(tmpl);

		tmpl.clear();
		tmpl["uriTemplate"] = "domoticz://scene/{idx}";
		tmpl["name"] = "Scene Devices by IDX";
		tmpl["mimeType"] = "text/plain";
		jsonRPCRep["result"]["resourceTemplates"].append(tmpl);

		tmpl.clear();
		tmpl["uriTemplate"] = "domoticz://user-variable/{idx}";
		tmpl["name"] = "User Variable by IDX";
		tmpl["mimeType"] = "text/plain";
		jsonRPCRep["result"]["resourceTemplates"].append(tmpl);

		tmpl.clear();
		tmpl["uriTemplate"] = "domoticz://event/{id}";
		tmpl["name"] = "Event Script by ID";
		tmpl["mimeType"] = "text/plain";
		jsonRPCRep["result"]["resourceTemplates"].append(tmpl);

		_log.Debug(DEBUG_WEBSERVER, "MCP: ResourcesTemplatesList: %d template(s) offered.", jsonRPCRep["result"]["resourceTemplates"].size());
	}

	void McpResourcesRead(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		// Check if the required parameters are present
		if (!jsonRequest.isMember("params") || !jsonRequest["params"].isMember("uri"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: Missing required resource parameter 'uri' in resources/read request.");
			jsonRPCRep["error"]["code"] = JSONRPC_INVALID_PARAMETER;
			jsonRPCRep["error"]["message"] = "Missing required parameter 'uri'";
			return;
		}
		std::string sReadURI = jsonRequest["params"]["uri"].asString();
		_log.Debug(DEBUG_WEBSERVER, "MCP: Handling resources/read request for %s.", sReadURI.c_str());

		jsonRPCRep["result"]["contents"] = Json::Value(Json::arrayValue);
		Json::Value resource;
		resource["uri"] = sReadURI;

		// --- domoticz:// scheme handler ---
		if (sReadURI.substr(0, 12) == "domoticz://")
		{
			std::string sPath = sReadURI.substr(12); // e.g. "device/42" or "devices"
			std::string sResourceType = sPath.substr(0, sPath.find('/'));
			std::string sResourceSuffix = (sPath.find('/') != std::string::npos) ? sPath.substr(sPath.find('/') + 1) : "";
			int nIdx = -1;
			if (!sResourceSuffix.empty())
			{
				try { nIdx = std::stoi(sResourceSuffix); } catch (...) {}
			}

			if (sResourceType == "devices")
			{
				// All devices summary
				Json::Value jsonDevices;
				m_webservers.GetJSonDevices(jsonDevices, "true", "", "Name", "", "", "", false, false, false, 0, "", "");
				std::string sText;
				int iCount = 0;
				if (jsonDevices.isObject() && jsonDevices.isMember("result"))
				{
					for (const auto &device : jsonDevices["result"])
					{
						if (!device.isObject() || !device.isMember("Name"))
							continue;
						iCount++;
						sText += "- \"" + device["Name"].asString() + "\"";
						if (device.isMember("Type"))
							sText += " [" + device["Type"].asString();
						if (device.isMember("SubType"))
							sText += "/" + device["SubType"].asString();
						if (device.isMember("Type"))
							sText += "]";
						if (device.isMember("Data"))
							sText += " = " + device["Data"].asString();
						if (device.isMember("idx"))
							sText += " (idx=" + device["idx"].asString() + ")";
						sText += "\n";
					}
				}
				if (iCount == 0)
					sText = "No used devices found.";
				else
					sText = std::to_string(iCount) + " device(s):\n" + sText;
				resource["name"] = "All Devices";
				resource["mimeType"] = "text/plain";
				resource["text"] = sText;
			}
			else if (sResourceType == "device")
			{
				// Single device by idx
				if (nIdx < 0)
				{
					jsonRPCRep["error"]["code"] = JSONRPC_INVALID_PARAMETER;
					jsonRPCRep["error"]["message"] = "Invalid device idx in URI";
					return;
				}
				auto result = m_sql.safe_query(
					"SELECT Name, HardwareID, DeviceID, Type, SubType, nValue, sValue, LastUpdate FROM DeviceStatus WHERE ID=%d", nIdx);
				if (result.empty())
				{
					_log.Debug(DEBUG_WEBSERVER, "MCP: resources/read: No device found with IDX %d", nIdx);
					jsonRPCRep["error"]["code"] = MCP_RESOURCE_NOT_FOUND;
					jsonRPCRep["error"]["message"] = "No device found with the specified URI";
					return;
				}
				auto &row = result[0];
				std::string sName = row[0];
				int iHardwareID = atoi(row[1].c_str());
				std::string sDeviceID = row[2];
				int iType = atoi(row[3].c_str());
				int iSubType = atoi(row[4].c_str());
				int nValue = atoi(row[5].c_str());
				std::string sValue = row[6];
				std::string sLastUpdate = row[7];
				resource["name"] = sName;
				resource["title"] = sName + " (" + std::to_string(iHardwareID) + " - " + std::to_string(iType) + " - " + std::to_string(iSubType) + ")";
				resource["mimeType"] = "text/plain";
				resource["text"] = (sValue.empty() ? std::to_string(nValue) : sValue);
				Json::Value meta;
				meta["hardwareID"] = iHardwareID;
				meta["type"] = iType;
				meta["subtype"] = iSubType;
				meta["idx"] = nIdx;
				meta["id"] = sDeviceID;
				resource["_meta"] = meta;
				Json::Value annotations;
				std::string sTmp = sLastUpdate;
				stdreplace(sTmp, " ", "T");
				annotations["lastModified"] = sTmp + "Z";
				resource["annotations"] = annotations;
			}
			else if (sResourceType == "rooms")
			{
				auto result = m_sql.safe_query("SELECT ID, Name FROM Plans WHERE (Name!='') ORDER BY Name");
				std::string sText;
				if (result.empty())
					sText = "No rooms (plans) configured.";
				else
				{
					sText = std::to_string(result.size()) + " room(s):\n";
					for (const auto &row : result)
						sText += "- \"" + row[1] + "\" (idx=" + row[0] + ")\n";
				}
				resource["name"] = "Rooms";
				resource["mimeType"] = "text/plain";
				resource["text"] = sText;
			}
			else if (sResourceType == "room")
			{
				// Devices in a room by plan idx
				if (nIdx < 0)
				{
					jsonRPCRep["error"]["code"] = JSONRPC_INVALID_PARAMETER;
					jsonRPCRep["error"]["message"] = "Invalid room idx in URI";
					return;
				}
				auto planResult = m_sql.safe_query("SELECT Name FROM Plans WHERE ID=%d", nIdx);
				if (planResult.empty())
				{
					jsonRPCRep["error"]["code"] = MCP_RESOURCE_NOT_FOUND;
					jsonRPCRep["error"]["message"] = "No room found with the specified idx";
					return;
				}
				std::string sRoomName = planResult[0][0];
				Json::Value jsonDevices;
				m_webservers.GetJSonDevices(jsonDevices, "true", "", "Name", "", std::to_string(nIdx), "", false, false, false, 0, "", "");
				std::string sText;
				int iCount = 0;
				if (jsonDevices.isObject() && jsonDevices.isMember("result"))
				{
					for (const auto &device : jsonDevices["result"])
					{
						if (!device.isObject() || !device.isMember("Name"))
							continue;
						iCount++;
						sText += "- \"" + device["Name"].asString() + "\"";
						if (device.isMember("Type"))
							sText += " [" + device["Type"].asString();
						if (device.isMember("SubType"))
							sText += "/" + device["SubType"].asString();
						if (device.isMember("Type"))
							sText += "]";
						if (device.isMember("Data"))
							sText += " = " + device["Data"].asString();
						sText += "\n";
					}
				}
				if (iCount == 0)
					sText = "Room \"" + sRoomName + "\" has no devices.";
				else
					sText = std::to_string(iCount) + " device(s) in room \"" + sRoomName + "\":\n" + sText;
				resource["name"] = "Room: " + sRoomName;
				resource["mimeType"] = "text/plain";
				resource["text"] = sText;
				Json::Value meta;
				meta["idx"] = nIdx;
				resource["_meta"] = meta;
			}
			else if (sResourceType == "scenes")
			{
				auto result = m_sql.safe_query("SELECT ID, Name, SceneType, Status FROM Scenes ORDER BY Name");
				std::string sText;
				if (result.empty())
					sText = "No scenes or groups configured.";
				else
				{
					sText = std::to_string(result.size()) + " scene(s)/group(s):\n";
					for (const auto &row : result)
					{
						std::string sType = (atoi(row[2].c_str()) == 1) ? "Group" : "Scene";
						std::string sStatus = (row[3] == "1") ? "On" : "Off";
						sText += "- \"" + row[1] + "\" [" + sType + ", " + sStatus + ", idx=" + row[0] + "]\n";
					}
				}
				resource["name"] = "Scenes";
				resource["mimeType"] = "text/plain";
				resource["text"] = sText;
			}
			else if (sResourceType == "scene")
			{
				// Devices in a scene by idx
				if (nIdx < 0)
				{
					jsonRPCRep["error"]["code"] = JSONRPC_INVALID_PARAMETER;
					jsonRPCRep["error"]["message"] = "Invalid scene idx in URI";
					return;
				}
				auto scResult = m_sql.safe_query("SELECT Name FROM Scenes WHERE ID=%d", nIdx);
				if (scResult.empty())
				{
					jsonRPCRep["error"]["code"] = MCP_RESOURCE_NOT_FOUND;
					jsonRPCRep["error"]["message"] = "No scene found with the specified idx";
					return;
				}
				std::string sSceneName = scResult[0][0];
				auto devResult = m_sql.safe_query(
					"SELECT a.DeviceRowID, b.Name, a.Cmd, a.Level "
					"FROM SceneDevices a JOIN DeviceStatus b ON b.ID=a.DeviceRowID "
					"WHERE a.SceneRowID=%d ORDER BY b.Name", nIdx);
				std::string sText;
				if (devResult.empty())
					sText = "Scene \"" + sSceneName + "\" has no devices.";
				else
				{
					sText = std::to_string(devResult.size()) + " device(s) in scene \"" + sSceneName + "\":\n";
					for (const auto &row : devResult)
					{
						sText += "- \"" + row[1] + "\" [idx=" + row[0] + ", cmd=" + row[2];
						if (!row[3].empty() && row[3] != "0")
							sText += ", level=" + row[3];
						sText += "]\n";
					}
				}
				resource["name"] = "Scene: " + sSceneName;
				resource["mimeType"] = "text/plain";
				resource["text"] = sText;
				Json::Value meta;
				meta["idx"] = nIdx;
				resource["_meta"] = meta;
			}
			else if (sResourceType == "user-variables")
			{
				auto result = m_sql.safe_query("SELECT ID, Name, ValueType, Value, LastUpdate FROM UserVariables ORDER BY Name");
				std::string sText;
				if (result.empty())
					sText = "No user variables defined.";
				else
				{
					sText = std::to_string(result.size()) + " variable(s):\n";
					for (const auto &row : result)
					{
						int vtype = atoi(row[2].c_str());
						const char *sTypeName = (vtype >= 0 && vtype <= 4) ? kVarTypeNames[vtype] : "Unknown";
						sText += "- \"" + row[1] + "\" [" + sTypeName + ", idx=" + row[0] + "] = " + row[3] + " (updated: " + row[4] + ")\n";
					}
				}
				resource["name"] = "User Variables";
				resource["mimeType"] = "text/plain";
				resource["text"] = sText;
			}
			else if (sResourceType == "user-variable")
			{
				// Single variable by idx
				if (nIdx < 0)
				{
					jsonRPCRep["error"]["code"] = JSONRPC_INVALID_PARAMETER;
					jsonRPCRep["error"]["message"] = "Invalid user-variable idx in URI";
					return;
				}
				auto result = m_sql.safe_query("SELECT ID, Name, ValueType, Value, LastUpdate FROM UserVariables WHERE ID=%d", nIdx);
				if (result.empty())
				{
					jsonRPCRep["error"]["code"] = MCP_RESOURCE_NOT_FOUND;
					jsonRPCRep["error"]["message"] = "No user variable found with the specified idx";
					return;
				}
				auto &row = result[0];
				int vtype = atoi(row[2].c_str());
				const char *sTypeName = (vtype >= 0 && vtype <= 4) ? kVarTypeNames[vtype] : "Unknown";
				std::string sText = "Variable \"" + row[1] + "\" [" + sTypeName + ", idx=" + row[0] + "] = " + row[3] + " (updated: " + row[4] + ")";
				resource["name"] = row[1];
				resource["mimeType"] = "text/plain";
				resource["text"] = sText;
				Json::Value meta;
				meta["idx"] = nIdx;
				resource["_meta"] = meta;
			}
			else if (sResourceType == "events")
			{
				auto result = m_sql.safe_query("SELECT ID, Name, Status, Interpreter FROM EventMaster ORDER BY Name");
				std::string sText;
				if (result.empty())
					sText = "No event scripts defined.";
				else
				{
					sText = std::to_string(result.size()) + " event script(s):\n";
					for (const auto &row : result)
					{
						bool bEnabled = (atoi(row[2].c_str()) == 1);
						sText += "- \"" + row[1] + "\" [" + row[3] + ", " + (bEnabled ? "enabled" : "disabled") + ", idx=" + row[0] + "]\n";
					}
				}
				resource["name"] = "Event Scripts";
				resource["mimeType"] = "text/plain";
				resource["text"] = sText;
			}
			else if (sResourceType == "event")
			{
				// Single event script by idx
				if (nIdx < 0)
				{
					jsonRPCRep["error"]["code"] = JSONRPC_INVALID_PARAMETER;
					jsonRPCRep["error"]["message"] = "Invalid event idx in URI";
					return;
				}
				auto result = m_sql.safe_query(
					"SELECT ID, Name, XMLStatement, Status, Interpreter FROM EventMaster WHERE ID=%d", nIdx);
				if (result.empty())
				{
					jsonRPCRep["error"]["code"] = MCP_RESOURCE_NOT_FOUND;
					jsonRPCRep["error"]["message"] = "No event script found with the specified idx";
					return;
				}
				auto &row = result[0];
				bool bEnabled = (atoi(row[3].c_str()) == 1);
				std::string sText = "Event script \"" + row[1] + "\" [" + row[4] + ", " + (bEnabled ? "enabled" : "disabled") + ", idx=" + row[0] + "]:\n";
				sText += "--- BEGIN SCRIPT ---\n";
				sText += row[2];
				sText += "\n--- END SCRIPT ---";
				resource["name"] = row[1];
				resource["mimeType"] = "text/plain";
				resource["text"] = sText;
				Json::Value meta;
				meta["idx"] = nIdx;
				resource["_meta"] = meta;
			}
			else if (sResourceType == "security")
			{
				int iSecStatus = 0;
				m_sql.GetPreferencesVar("SecStatus", iSecStatus);
				std::string sStatus;
				switch (iSecStatus)
				{
					case 0: sStatus = "Disarmed"; break;
					case 1: sStatus = "Armed Home"; break;
					case 2: sStatus = "Armed Away"; break;
					default: sStatus = "Unknown (" + std::to_string(iSecStatus) + ")"; break;
				}
				resource["name"] = "Security";
				resource["mimeType"] = "text/plain";
				resource["text"] = "Security panel status: " + sStatus + " (code: " + std::to_string(iSecStatus) + ")";
			}
			else if (sResourceType == "settings")
			{
				std::string sTitle, sLocation, sLanguage;
				int iTempScale = 0, iWindScale = 0, iSensorTimeout = 0, iBatterLow = 0, iActivePlan = 0;
				m_sql.GetPreferencesVar("Title", sTitle);
				m_sql.GetPreferencesVar("Location", sLocation);
				m_sql.GetPreferencesVar("Language", sLanguage);
				m_sql.GetPreferencesVar("TempScale", iTempScale);
				m_sql.GetPreferencesVar("WindScale", iWindScale);
				m_sql.GetPreferencesVar("SensorTimeout", iSensorTimeout);
				m_sql.GetPreferencesVar("BatterLowNotification", iBatterLow);
				m_sql.GetPreferencesVar("ActiveTimerPlan", iActivePlan);

				std::string sText = "Domoticz System Settings\n";
				sText += "========================\n";
				if (!sTitle.empty())
					sText += "Title: " + sTitle + "\n";
				if (!sLocation.empty())
					sText += "Location: " + sLocation + "\n";
				if (!sLanguage.empty())
					sText += "Language: " + sLanguage + "\n";
				sText += "Temperature scale: " + std::string(iTempScale == 1 ? "Fahrenheit" : "Celsius") + "\n";
				sText += "Wind scale: " + std::string(iWindScale == 1 ? "mph" : "m/s") + "\n";
				sText += "Sensor timeout (min): " + std::to_string(iSensorTimeout) + "\n";
				sText += "Battery low notification threshold: " + std::to_string(iBatterLow) + "%\n";
				sText += "Active timer plan: " + std::to_string(iActivePlan) + "\n";
				resource["name"] = "Settings";
				resource["mimeType"] = "text/plain";
				resource["text"] = sText;
			}
			else if (sResourceType == "log")
			{
				std::string sText = "Recent system log entries:\n";
				std::list<CLogger::_tLogLineStruct> logmessages = _log.GetLog(_eLogLevel::LOG_ALL, 0);
				if (logmessages.empty())
					sText += "(no log entries)\n";
				else
					for (const auto &msg : logmessages)
						sText += msg.logmessage + "\n";
				resource["name"] = "System Log";
				resource["mimeType"] = "text/plain";
				resource["text"] = sText;
			}
			else
			{
				_log.Debug(DEBUG_WEBSERVER, "MCP: resources/read: Unknown domoticz:// resource type: %s", sResourceType.c_str());
				jsonRPCRep["error"]["code"] = MCP_RESOURCE_NOT_FOUND;
				jsonRPCRep["error"]["message"] = "Unknown resource type: " + sResourceType;
				return;
			}
		}
		// --- Legacy floorplan:/// scheme handler ---
		else if (sReadURI.substr(0, 13) == "floorplan:///")
		{
			int nIdx = -1;
			try { nIdx = std::stoi(sReadURI.substr(sReadURI.find_last_of("/") + 1)); }
			catch (...) {}
			if (nIdx < 0)
			{
				jsonRPCRep["error"]["code"] = JSONRPC_INVALID_PARAMETER;
				jsonRPCRep["error"]["message"] = "Invalid floorplan idx in URI";
				return;
			}
			auto result = m_sql.safe_query("SELECT Name, Scalefactor FROM Floorplans WHERE ID=%d", nIdx);
			if (result.empty())
			{
				_log.Debug(DEBUG_WEBSERVER, "MCP: resources/read: No floorplan found with IDX %d", nIdx);
				jsonRPCRep["error"]["code"] = MCP_RESOURCE_NOT_FOUND;
				jsonRPCRep["error"]["message"] = "No floorplan found with the specified URI";
				return;
			}
			auto &row = result[0];
			resource["name"] = row[0];
			resource["title"] = "Floorplan " + row[0] + " (Scale factor: " + row[1] + ")";
			resource["mimeType"] = "image/*";
			resource["text"] = "Floorplan image - use the get_floorplan tool to retrieve the actual image data.";
			Json::Value meta;
			meta["idx"] = nIdx;
			meta["type"] = "floorplan";
			resource["_meta"] = meta;
		}
		else
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: resources/read: Unsupported URI scheme: %s", sReadURI.c_str());
			jsonRPCRep["error"]["code"] = MCP_RESOURCE_NOT_FOUND;
			jsonRPCRep["error"]["message"] = "Unsupported resource URI scheme";
			return;
		}

		jsonRPCRep["result"]["contents"].append(resource);

		_log.Debug(DEBUG_WEBSERVER, "MCP: Offering resources/read request result %s.", resource.toStyledString().c_str());
	}

	void McpPromptsList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		_log.Debug(DEBUG_WEBSERVER, "MCP: Handling prompts/list request.");

		// Prepare the result for the prompts/list method
		jsonRPCRep["result"]["prompts"] = Json::Value(Json::arrayValue);
		Json::Value prompt;
		// House Summary prompt
		prompt["name"] = "housesummary";
		prompt["title"] = "Get a status overview";
		prompt["description"] = "Summarize the current status of all sensors and devices in the house (optionally limited to a specific room)";
		prompt["arguments"] = Json::Value(Json::arrayValue);
		Json::Value arg;
		arg["name"] = "room";
		arg["description"] = "The room to limit the summary to (optional, if not provided the whole house is summarized)";
		arg["required"] = false;
		prompt["arguments"].append(arg);
		jsonRPCRep["result"]["prompts"].append(prompt);
		// System analysis prompt
		prompt.clear();
		prompt["name"] = "systemanalysis";
		prompt["title"] = "Get a system analysis";
		prompt["description"] = "Analyze the current status of the system and provide insights";
		prompt["arguments"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["prompts"].append(prompt);
		// Troubleshoot device prompt
		prompt.clear();
		prompt["name"] = "troubleshoot_device";
		prompt["title"] = "Troubleshoot a device";
		prompt["description"] = "Diagnose issues with a specific device by analyzing its current state, recent history, and system logs";
		prompt["arguments"] = Json::Value(Json::arrayValue);
		Json::Value argDev;
		argDev["name"] = "device";
		argDev["description"] = "Name or IDX of the device to troubleshoot";
		argDev["required"] = true;
		prompt["arguments"].append(argDev);
		jsonRPCRep["result"]["prompts"].append(prompt);
		// Analyze automations prompt
		prompt.clear();
		prompt["name"] = "analyze_automations";
		prompt["title"] = "Analyze automation scripts";
		prompt["description"] = "Review all event scripts for logic issues, inefficiencies, or improvement opportunities";
		prompt["arguments"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["prompts"].append(prompt);
	}

	void McpPromptsGet(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		std::string sPromptName = jsonRequest["params"]["name"].asString();
		_log.Debug(DEBUG_WEBSERVER, "MCP: Handling prompts/get request (%s).", sPromptName.c_str());

		if (sPromptName == "housesummary")
		{
			std::string sRoom = ((jsonRequest["params"].isMember("arguments") && jsonRequest["params"]["arguments"].isMember("room")) ? jsonRequest["params"]["arguments"]["room"].asString() : "");
			// Prepare the result for the prompts/get method
			jsonRPCRep["result"]["description"] = "Summarize the current status of all sensors and devices in the house (optionally limited to a specific room)";
			jsonRPCRep["result"]["messages"] = Json::Value(Json::arrayValue);
			Json::Value message;
			message["role"] = "user";
			message["content"] = Json::Value(Json::objectValue);
			message["content"]["type"] = "text";
			std::string sText = "As the friendly butler of the house, please summarize the current status of all sensors and devices preferably grouped by room.";
			Json::Value jsonDevices;
			m_webservers.GetJSonDevices(jsonDevices, "", "", "", "", "", "", false, false, false, 0, "", "");
			sText += " Include the following devices in your summary:";
			for(const auto &device : jsonDevices["result"])
			{
				if(device.isObject() && device.isMember("Name") && device.isMember("Data") && device.isMember("Type") && device.isMember("SubType"))
				{
					std::string sDevRoom = (device.isMember("Room") ? device["Room"].asString() : "");
					if(sRoom.empty() || (!sRoom.empty() && sRoom == sDevRoom))
					{
						sText += device["Name"].asString() + ", ";
					}
				}
			}
			// Append rooms
			auto roomResult = m_sql.safe_query("SELECT ID, Name FROM Plans WHERE Name!='' ORDER BY Name");
			if (!roomResult.empty())
			{
				sText += " The house has the following rooms: ";
				for (const auto &row : roomResult)
					sText += row[1] + " (idx=" + row[0] + "), ";
			}
			// Append scenes
			auto sceneResult = m_sql.safe_query("SELECT Name, Status FROM Scenes ORDER BY Name");
			if (!sceneResult.empty())
			{
				sText += " Available scenes/groups: ";
				for (const auto &row : sceneResult)
					sText += row[0] + " [" + (row[1] == "1" ? "On" : "Off") + "], ";
			}
			message["content"]["text"] = sText;
			jsonRPCRep["result"]["messages"].append(message);
		}
		else if (sPromptName == "systemanalysis")
		{
			// Prepare the result for the prompts/get method
			jsonRPCRep["result"]["description"] = "Analyze the current status of the system and provide insights";
			jsonRPCRep["result"]["messages"] = Json::Value(Json::arrayValue);
			Json::Value message;
			message["role"] = "user";
			message["content"] = Json::Value(Json::objectValue);
			message["content"]["type"] = "text";
			std::string sText = "As the friendly butler of the house, please make an analysis of the current status of the system by analyzing all available log information, and providing suggestions if needed.";
			sText += "State the time window of the logging you have analyzed. If the latest log entries are older than 3 minutes, make sure to first retrieve the latest log entries before making your analysis.";
			message["content"]["text"] = sText;
			jsonRPCRep["result"]["messages"].append(message);
		}
		else if (sPromptName == "troubleshoot_device")
		{
			std::string sDevice = ((jsonRequest["params"].isMember("arguments") && jsonRequest["params"]["arguments"].isMember("device")) ? jsonRequest["params"]["arguments"]["device"].asString() : "");
			jsonRPCRep["result"]["description"] = "Troubleshoot device: " + sDevice;
			jsonRPCRep["result"]["messages"] = Json::Value(Json::arrayValue);
			Json::Value message;
			message["role"] = "user";
			message["content"] = Json::Value(Json::objectValue);
			message["content"]["type"] = "text";
			std::string sText = "Please troubleshoot the Domoticz device named \"" + sDevice + "\". ";
			sText += "Use the available tools to: ";
			sText += "1) Get the current state of the device using get_switch_state or get_sensor_value. ";
			sText += "2) Check its recent history using get_device_history. ";
			sText += "3) Check the system log using get_logging for any errors related to this device. ";
			sText += "4) Check if its hardware is online using get_hardware. ";
			sText += "Summarize what you find and suggest any remediation steps.";
			message["content"]["text"] = sText;
			jsonRPCRep["result"]["messages"].append(message);
		}
		else if (sPromptName == "analyze_automations")
		{
			jsonRPCRep["result"]["description"] = "Review all event scripts for logic issues, inefficiencies, or improvement opportunities";
			jsonRPCRep["result"]["messages"] = Json::Value(Json::arrayValue);
			Json::Value message;
			message["role"] = "user";
			message["content"] = Json::Value(Json::objectValue);
			message["content"]["type"] = "text";
			std::string sText = "Please analyze all Domoticz automation event scripts. ";
			sText += "Use the available tools to: ";
			sText += "1) List all event scripts using get_events. ";
			sText += "2) Read each script's source code using get_event. ";
			sText += "3) Review each script for logic errors, inefficiencies, or improvement opportunities. ";
			sText += "4) Check the system log for any automation-related errors using get_logging. ";
			sText += "Provide a structured report with findings and suggestions for each script.";
			message["content"]["text"] = sText;
			jsonRPCRep["result"]["messages"].append(message);
		}
		else
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: prompts/get: Unsupported prompt name: %s", sPromptName.c_str());
			jsonRPCRep["error"]["code"] = JSONRPC_METHOD_NOT_FOUND;
			jsonRPCRep["error"]["message"] = "Method not found";
		}
	}

	bool getSwitchState(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") || !jsonRequest["params"]["arguments"].isMember("switchname"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: getSwitchState: Missing required parameter 'switchname'");
			return false;
		}
		std::string sSwitchName = jsonRequest["params"]["arguments"]["switchname"].asString();
		std::string sSwitchState = "No switch exists with the name " + sSwitchName;
		Json::Value device;
		bool bFound = getDeviceByName(sSwitchName, device);
		if (bFound)
		{
			sSwitchState = "The current state of switch \"" + sSwitchName + "\" is: " + device["Data"].asString();
		}
		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = sSwitchState;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = !bFound;
		return true;
	}

	bool toggleSwitchState(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") || !jsonRequest["params"]["arguments"].isMember("switchname"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: toggleSwitchState: Missing required parameter 'switchname'");
			return false;
		}
		std::string sSwitchName = jsonRequest["params"]["arguments"]["switchname"].asString();
		std::string sSwitchState = "No switch exists with the name " + sSwitchName;
		Json::Value device;
		bool bFound = getDeviceByName(sSwitchName, device);
		if (bFound)
		{
			sSwitchState = "The state of switch \"" + sSwitchName + "\" before toggle was: " + device["Data"].asString() + ". ";
			bFound = true;
			// const std::string& idx, const std::string& switchcmd, const std::string& level, const std::string& color, const std::string& ooc, const int ExtraDelay, const std::string& User)
			sSwitchState += (m_mainworker.SwitchLight(device["idx"].asString(), "Toggle", "", "", "", 0, "") == MainWorker::eSwitchLightReturnCode::SL_ERROR ? "Error toggling the switch." : "Switch toggled successfully.");
		}
		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = sSwitchState;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = !bFound;
		return true;
	}

	bool getSensorValue(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") || !jsonRequest["params"]["arguments"].isMember("sensorname"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: getSensorValue: Missing required parameter 'sensorname'");
			return false;
		}
		std::string sSensorName = jsonRequest["params"]["arguments"]["sensorname"].asString();
		std::string sSensorValue = "No sensor exists with the name " + sSensorName;
		Json::Value device;
		bool bFound = getDeviceByName(sSensorName, device);
		if (bFound)
		{
			sSensorValue = "The current value for sensor \"" + sSensorName + "\" is: " + device["Data"].asString();
		}
		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = sSensorValue;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = !bFound;
		return true;
	}

	bool getFloorplan(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") || !jsonRequest["params"]["arguments"].isMember("floorplan"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: getFloorplan: Missing required parameter 'floorplan'");
			return false;
		}
		std::string sFloorplan = jsonRequest["params"]["arguments"]["floorplan"].asString();
		std::string sFloorplanValue = "No floorplan exists with the name " + sFloorplan;
		std::string sMimeType;
		Json::Value tool;
		bool bFound = false;

		auto result = m_sql.safe_query("SELECT ID FROM Floorplans WHERE Name='%q'", sFloorplan.c_str());
		if (!result.empty() && result.size() == 1 )
		{
			std::string idx = result[0][0];
			std::vector<std::vector<std::string>> blob;
			blob = m_sql.safe_queryBlob("SELECT Image FROM Floorplans WHERE ID=%d", atol(idx.c_str()));
			if (!blob.empty())
			{
				// To-Do: refactor code together with CWebServer::GetFloorplanImage code (less duplication)
				bFound = true;
				sFloorplanValue = base64_encode(std::string(blob[0][0].begin(), blob[0][0].end()));
				sMimeType = "image/*"; // unknown image type
				if (blob[0][0].size() > 10)
				{
					// PNG
					const unsigned char png_signature[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

					// JPEG
					const unsigned char jpeg_signature[3] = {0xFF, 0xD8, 0xFF};

					// GIF87a
					const unsigned char gif87a_signature[6] = {'G', 'I', 'F', '8', '7', 'a'};
					// GIF89a
					const unsigned char gif89a_signature[6] = {'G', 'I', 'F', '8', '9', 'a'};

					// BMP
					const unsigned char bmp_signature[2] = {'B', 'M'};

					// WebP (RIFF....WEBP)
					const unsigned char webp_riff_signature[4] = {'R', 'I', 'F', 'F'};
					const unsigned char webp_webp_signature[4] = {'W', 'E', 'B', 'P'};

					if (std::equal(png_signature, png_signature + sizeof(png_signature), reinterpret_cast<const unsigned char*>(&*blob[0][0].begin())))
						sMimeType = "image/png";
					else if (std::equal(jpeg_signature, jpeg_signature + sizeof(jpeg_signature), reinterpret_cast<const unsigned char*>(&*blob[0][0].begin())))
						sMimeType = "image/jpeg";
					else if (std::equal(bmp_signature, bmp_signature + sizeof(bmp_signature), reinterpret_cast<const unsigned char*>(&*blob[0][0].begin())))
						sMimeType = "image/bmp";
					else if (std::equal(gif87a_signature, gif87a_signature + sizeof(gif87a_signature), reinterpret_cast<const unsigned char*>(&*blob[0][0].begin())) ||
							 std::equal(gif89a_signature, gif89a_signature + sizeof(gif89a_signature), reinterpret_cast<const unsigned char*>(&*blob[0][0].begin())))
						sMimeType = "image/gif";
					else if ((blob[0][0][0] == '<') && (blob[0][0][1] == 's') && (blob[0][0][2] == 'v') && (blob[0][0][3] == 'g'))
						sMimeType = "image/svg+xml";
					else if (blob[0][0].find("<svg") != std::string::npos) // some SVG's start with <xml
						sMimeType = "image/svg+xml";
					//_log.Debug(DEBUG_WEBSERVER, "MCP: getFloorplan: Detected floorplan image header: %.10s (%s)", std::string(blob[0][0].begin(), blob[0][0].begin() + 10).c_str(), sMimeType.c_str());
				}
			}
		}

		if (bFound)
		{
			tool["type"] = "image";
			tool["mimeType"] = sMimeType;
			tool["data"] = sFloorplanValue;
		}
		else
		{
			tool["type"] = "text";
			tool["text"] = sFloorplanValue;
		}
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = !bFound;
		return true;
	}

	bool getLogging(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		bool bFound = false;
		time_t iSinceUnixtime = 0;
		if (jsonRequest["params"].isMember("arguments") && jsonRequest["params"]["arguments"].isMember("logdate"))
		{
			iSinceUnixtime = (time_t)jsonRequest["params"]["arguments"]["logdate"].asUInt64();
			_log.Debug(DEBUG_WEBSERVER, "MCP: getLogging: Retrieving logs since Unixtime %ld", (uint64_t)iSinceUnixtime);
		}
		// Get the current log levels
		std::string sResult = "The following loglevel are currently enabled: ";
		if (_log.IsLogLevelEnabled(LOG_ALL))
		{
			bFound = true;
			sResult += "ALL ";
		}
		if (_log.IsLogLevelEnabled(LOG_ERROR))
		{
			bFound = true;
			sResult += "ERROR ";
		}
		if (_log.IsLogLevelEnabled(LOG_STATUS))
		{
			bFound = true;
			sResult += "STATUS ";
		}
		if (_log.IsLogLevelEnabled(LOG_NORM))
		{
			bFound = true;
			sResult += "NORM ";
		}
		if (_log.IsLogLevelEnabled(LOG_DEBUG_INT))
		{
			bFound = true;
			sResult += "DEBUG ";
		}
		if (bFound)
		{
			sResult += "\nThe last log messages are:\n";
			std::list<CLogger::_tLogLineStruct> logmessages = _log.GetLog(_eLogLevel::LOG_ALL, iSinceUnixtime);
			for (const auto& msg : logmessages)
			{
				sResult += msg.logmessage + "\n";
			}
		}
		else
			sResult = "No loglevels are currently enabled!";
		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = sResult;;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = !bFound;
		return bFound;
	}

	bool setThermostatSetpoint(const Json::Value& jsonRequest, Json::Value& jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") || !jsonRequest["params"]["arguments"].isMember("thermostatname") || !jsonRequest["params"]["arguments"].isMember("setpoint"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: setThermostatSetpoint: Missing required parameter 'thermostatname/setpoint'");
			return false;
		}
		std::string sThermostatName = jsonRequest["params"]["arguments"]["thermostatname"].asString();
		float fNewSetpoint = (float)atof(jsonRequest["params"]["arguments"]["setpoint"].asString().c_str());
		std::string sThermostatState = "No thermostat exists with the name " + sThermostatName;
		Json::Value device;
		bool bFound = getDeviceByName(sThermostatName, device);
		if (bFound)
		{
			sThermostatState = "The value of thermostat \"" + sThermostatName + "\" before setting was: " + device["Data"].asString() + ". ";
			bFound = true;
			sThermostatState += (m_mainworker.SetSetPoint(device["idx"].asString(), fNewSetpoint, "MCP") == false ? "Error setting the setpoint." : "Setpoint set successfully.");
		}
		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = sThermostatState;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = !bFound;
		return true;
	}

	bool getStatus(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		std::string sResult = "Domoticz System Status\n";
		sResult += "======================\n";

		// Version info
		sResult += "Version: " + szAppVersion + "\n";
		sResult += "Build hash: " + szAppHash + "\n";
		sResult += "Build time: " + szAppDate + "\n";

		// Server time
		time_t now = mytime(nullptr);
		sResult += "Server time: " + TimeToString(&now, TF_DateTime) + "\n";

		// Uptime
		time_t tuptime = now - m_StartTime;
		int days = (int)(tuptime / 86400);
		tuptime -= (days * 86400);
		int hours = (int)(tuptime / 3600);
		tuptime -= (hours * 3600);
		int minutes = (int)(tuptime / 60);
		tuptime -= (minutes * 60);
		int seconds = (int)tuptime;
		sResult += "Uptime: " + std::to_string(days) + "d " + std::to_string(hours) + "h " + std::to_string(minutes) + "m " + std::to_string(seconds) + "s\n";

		// Sunrise/Sunset
		if (!m_mainworker.m_LastSunriseSet.empty())
		{
			std::vector<std::string> strarray;
			StringSplit(m_mainworker.m_LastSunriseSet, ";", strarray);
			if (strarray.size() >= 10)
			{
				sResult += "Sunrise: " + strarray[0] + "\n";
				sResult += "Sunset: " + strarray[1] + "\n";
				sResult += "Day length: " + strarray[9] + "\n";
			}
		}

		// Device counts
		Json::Value jsonDevices;
		std::string sUsed = "true";
		m_webservers.GetJSonDevices(jsonDevices, sUsed, "", "", "", "", "", false, false, false, 0, "", "");
		int iDeviceCount = 0;
		if (jsonDevices.isObject() && jsonDevices.isMember("result"))
			iDeviceCount = jsonDevices["result"].size();
		sResult += "Active devices: " + std::to_string(iDeviceCount) + "\n";

		// Hardware count
		auto hwResult = m_sql.safe_query("SELECT COUNT(*) FROM Hardware WHERE Enabled=1");
		if (!hwResult.empty())
			sResult += "Active hardware: " + hwResult[0][0] + "\n";

		// Scenes count
		auto scResult = m_sql.safe_query("SELECT COUNT(*) FROM Scenes");
		if (!scResult.empty())
			sResult += "Scenes/groups: " + scResult[0][0] + "\n";

		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = false;
		return true;
	}

	bool searchDevices(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") || !jsonRequest["params"]["arguments"].isMember("query"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: searchDevices: Missing required parameter 'query'");
			return false;
		}
		std::string sQuery = jsonRequest["params"]["arguments"]["query"].asString();
		std::string sFilter;
		if (jsonRequest["params"]["arguments"].isMember("filter"))
			sFilter = jsonRequest["params"]["arguments"]["filter"].asString();

		// Lowercase the query for case-insensitive matching
		std::string sQueryLower = sQuery;
		stdlower(sQueryLower);

		Json::Value jsonDevices;
		std::string sUsed = "true";
		m_webservers.GetJSonDevices(jsonDevices, sUsed, sFilter, "", "", "", "", false, false, false, 0, "", "");

		std::string sResult;
		int iMatchCount = 0;
		if (jsonDevices.isObject() && jsonDevices.isMember("result"))
		{
			for (const auto &device : jsonDevices["result"])
			{
				if (!device.isObject() || !device.isMember("Name"))
					continue;
				std::string sName = device["Name"].asString();
				std::string sNameLower = sName;
				stdlower(sNameLower);
				bool bMatch = (sNameLower.find(sQueryLower) != std::string::npos);
				if (!bMatch && device.isMember("Type"))
				{
					std::string sTypeLower = device["Type"].asString();
					stdlower(sTypeLower);
					bMatch = (sTypeLower.find(sQueryLower) != std::string::npos);
				}
				if (!bMatch && device.isMember("SubType"))
				{
					std::string sSubTypeLower = device["SubType"].asString();
					stdlower(sSubTypeLower);
					bMatch = (sSubTypeLower.find(sQueryLower) != std::string::npos);
				}
				if (!bMatch)
					continue;
				iMatchCount++;
				sResult += "- \"" + sName + "\"";
				if (device.isMember("Type"))
					sResult += " [" + device["Type"].asString();
				if (device.isMember("SubType"))
					sResult += "/" + device["SubType"].asString();
				if (device.isMember("Type"))
					sResult += "]";
				if (device.isMember("Data"))
					sResult += " = " + device["Data"].asString();
				sResult += "\n";
			}
		}

		if (iMatchCount == 0)
			sResult = "No devices found matching \"" + sQuery + "\"" + (sFilter.empty() ? "" : " with filter \"" + sFilter + "\"");
		else
			sResult = std::to_string(iMatchCount) + " device(s) found matching \"" + sQuery + "\":\n" + sResult;

		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = false;
		return true;
	}

	bool getDeviceByName(const std::string &sDeviceName, Json::Value &device)
	{
		Json::Value jsonDevices;
		m_webservers.GetJSonDevices(jsonDevices, "", "", "", "", "", "", false, false, false, 0, "", "");

		for (const auto &dev : jsonDevices["result"])
		{
			if (dev.isObject() && dev.isMember("Name") && dev["Name"].asString() == sDeviceName)
			{
				device = dev;
				return true;
			}
		}
		return false;
	}

	bool validRPC(const std::string &sInput, Json::Value &jsonRequest, std::string &sError)
	{
		if (sInput.empty())
		{
			sError = "Empty input";
			return false;
		}
		std::string sParseErr;
		if (!ParseJSon(sInput, jsonRequest, &sParseErr))
		{
			sError = "Failed to parse JSON content: " + sParseErr;
			return false;
		}
		// Check if the parsed JSON is valid JSON-RPC 2.0 format
		if (!jsonRequest.isObject() || !jsonRequest.isMember("jsonrpc") || !jsonRequest.isMember("method"))
		{
			sError = "Invalid JSON-RPC request format";
			return false;
		}
		// Check if the JSON-RPC version is supported
		if (jsonRequest["jsonrpc"].asString() != "2.0")
		{
			sError = "Unsupported JSON-RPC version: " + jsonRequest["jsonrpc"].asString();
			return false;
		}
		return true;
	}

	// -------------------------------------------------------------------------
	// -------------------------------------------------------------------------

	bool getAllDevices(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		std::string sFilter;
		if (jsonRequest["params"].isMember("arguments") && jsonRequest["params"]["arguments"].isMember("filter"))
			sFilter = jsonRequest["params"]["arguments"]["filter"].asString();

		// Validate filter against whitelist
		if (!sFilter.empty() && sFilter != "light" && sFilter != "temp" && sFilter != "weather" && sFilter != "utility")
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: getAllDevices: Invalid filter value: %s", sFilter.c_str());
			Json::Value tool;
			tool["type"] = "text";
			tool["text"] = "Invalid filter value '" + sFilter + "'. Valid values are: light, temp, weather, utility.";
			jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
			jsonRPCRep["result"]["content"].append(tool);
			jsonRPCRep["result"]["isError"] = true;
			return true;
		}

		Json::Value jsonDevices;
		std::string sUsed = "true";
		m_webservers.GetJSonDevices(jsonDevices, sUsed, sFilter, "Name", "", "", "", false, false, false, 0, "", "");

		std::string sResult;
		int iCount = 0;
		if (jsonDevices.isObject() && jsonDevices.isMember("result"))
		{
			for (const auto &device : jsonDevices["result"])
			{
				if (!device.isObject() || !device.isMember("Name"))
					continue;
				iCount++;
				sResult += "- \"" + device["Name"].asString() + "\"";
				if (device.isMember("Type"))
					sResult += " [" + device["Type"].asString();
				if (device.isMember("SubType"))
					sResult += "/" + device["SubType"].asString();
				if (device.isMember("Type"))
					sResult += "]";
				if (device.isMember("Data"))
					sResult += " = " + device["Data"].asString();
				if (device.isMember("idx"))
					sResult += " (idx=" + device["idx"].asString() + ")";
				sResult += "\n";
			}
		}

		if (iCount == 0)
			sResult = "No devices found" + (sFilter.empty() ? "" : " with filter \"" + sFilter + "\"");
		else
			sResult = std::to_string(iCount) + " device(s):\n" + sResult;

		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = false;
		return true;
	}

	bool getDevice(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool bHasName = args.isMember("name") && !args["name"].asString().empty();
		bool bHasIdx = args.isMember("idx");

		if (!bHasName && !bHasIdx)
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: getDevice: Missing required parameter 'name' or 'idx'");
			return false;
		}

		std::string sResult;
		bool bFound = false;

		if (bHasName)
		{
			Json::Value device;
			bFound = getDeviceByName(args["name"].asString(), device);
			if (bFound)
			{
				sResult = "Device details for \"" + args["name"].asString() + "\":\n";
				sResult += "  idx:         " + device["idx"].asString() + "\n";
				if (device.isMember("HardwareName"))
					sResult += "  Hardware:    " + device["HardwareName"].asString() + "\n";
				if (device.isMember("Type"))
					sResult += "  Type:        " + device["Type"].asString() + "\n";
				if (device.isMember("SubType"))
					sResult += "  SubType:     " + device["SubType"].asString() + "\n";
				if (device.isMember("Data"))
					sResult += "  Data:        " + device["Data"].asString() + "\n";
				if (device.isMember("nValue"))
					sResult += "  nValue:      " + device["nValue"].asString() + "\n";
				if (device.isMember("sValue"))
					sResult += "  sValue:      " + device["sValue"].asString() + "\n";
				if (device.isMember("LastUpdate"))
					sResult += "  LastUpdate:  " + device["LastUpdate"].asString() + "\n";
				if (device.isMember("Used"))
					sResult += "  Used:        " + device["Used"].asString() + "\n";
			}
			else
			{
				sResult = "No device found with name \"" + args["name"].asString() + "\"";
			}
		}
		else
		{
			int nIdx = args["idx"].asInt();
			auto result = m_sql.safe_query(
				"SELECT DS.Name, DS.HardwareID, H.Name, DS.DeviceID, DS.Type, DS.SubType, DS.nValue, DS.sValue, DS.LastUpdate, DS.Used "
				"FROM DeviceStatus DS LEFT JOIN Hardware H ON DS.HardwareID=H.ID WHERE DS.ID=%d", nIdx);
			if (!result.empty())
			{
				bFound = true;
				auto &row = result[0];
				sResult = "Device details for idx=" + std::to_string(nIdx) + ":\n";
				sResult += "  Name:        " + row[0] + "\n";
				sResult += "  Hardware:    " + row[2] + " (id=" + row[1] + ")\n";
				sResult += "  DeviceID:    " + row[3] + "\n";
				sResult += "  Type:        " + row[4] + "\n";
				sResult += "  SubType:     " + row[5] + "\n";
				sResult += "  nValue:      " + row[6] + "\n";
				sResult += "  sValue:      " + row[7] + "\n";
				sResult += "  LastUpdate:  " + row[8] + "\n";
				sResult += "  Used:        " + row[9] + "\n";
			}
			else
			{
				sResult = "No device found with idx=" + std::to_string(nIdx);
			}
		}

		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = !bFound;
		return true;
	}

	bool renameDevice(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") ||
		    !jsonRequest["params"]["arguments"].isMember("switchname") ||
		    !jsonRequest["params"]["arguments"].isMember("new_name"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: renameDevice: Missing required parameters 'switchname' or 'new_name'");
			return false;
		}
		std::string sOldName = jsonRequest["params"]["arguments"]["switchname"].asString();
		std::string sNewName = jsonRequest["params"]["arguments"]["new_name"].asString();

		Json::Value device;
		bool bFound = getDeviceByName(sOldName, device);
		std::string sResult;
		if (bFound)
		{
			m_sql.UpdateDeviceName(device["idx"].asString(), sNewName);
			sResult = "Device \"" + sOldName + "\" successfully renamed to \"" + sNewName + "\".";
		}
		else
		{
			sResult = "No device found with name \"" + sOldName + "\"";
		}

		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = !bFound;
		return true;
	}

	bool deleteDevice(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") || !jsonRequest["params"]["arguments"].isMember("switchname"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: deleteDevice: Missing required parameter 'switchname'");
			return false;
		}
		std::string sName = jsonRequest["params"]["arguments"]["switchname"].asString();

		Json::Value device;
		bool bFound = getDeviceByName(sName, device);
		std::string sResult;
		if (bFound)
		{
			int nIdx = atoi(device["idx"].asString().c_str());
			m_sql.safe_query("UPDATE DeviceStatus SET Used=0 WHERE ID=%d", nIdx);
			sResult = "Device \"" + sName + "\" (idx=" + device["idx"].asString() + ") has been hidden (Used=0).";
		}
		else
		{
			sResult = "No device found with name \"" + sName + "\"";
		}

		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = !bFound;
		return true;
	}

	bool createVirtualSensor(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") ||
		    !jsonRequest["params"]["arguments"].isMember("hw_idx") ||
		    !jsonRequest["params"]["arguments"].isMember("sensorname") ||
		    !jsonRequest["params"]["arguments"].isMember("sensortype"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: createVirtualSensor: Missing required parameters");
			return false;
		}

		int nHwIdx     = jsonRequest["params"]["arguments"]["hw_idx"].asInt();
		std::string sSensorName = jsonRequest["params"]["arguments"]["sensorname"].asString();
		int nSensorType = jsonRequest["params"]["arguments"]["sensortype"].asInt();

		// Validate hardware exists
		auto hwResult = m_sql.safe_query("SELECT ID FROM Hardware WHERE ID=%d", nHwIdx);
		if (hwResult.empty())
		{
			Json::Value tool;
			tool["type"] = "text";
			tool["text"] = "No hardware found with idx=" + std::to_string(nHwIdx);
			jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
			jsonRPCRep["result"]["content"].append(tool);
			jsonRPCRep["result"]["isError"] = true;
			return true;
		}

		// Look up the type/subtype from the mapped sensor table
		// Sensor type map (subset of Dummy.cpp mappedsensorname)
		struct _tmapped { int mappedvalue; int type; int subtype; };
		static const _tmapped sensorMap[] = {
			{ 249, 18, 1 },	  // Air Quality (pTypeAirQuality/sTypeVoc)
			{ 7,   22, 1 },	  // Alert (pTypeGeneral/sTypeAlert)
			{ 9,   89, 1 },	  // Ampere 3-phase
			{ 19,  22, 23 },  // Ampere 1-phase
			{ 11,  22, 2 },	  // Barometer
			{ 113, 115, 1 },  // Counter (pTypeRFXMeter)
			{ 14,  22, 28 },  // Counter Incremental
			{ 1004,22, 31 },  // Custom Sensor
			{ 13,  22, 13 },  // Distance
			{ 18,  22, 29 },  // Electric Instant+Counter
			{ 3,   82, 1 },	  // Gas
			{ 81,  81, 1 },	  // Humidity
			{ 16,  22, 18 },  // Leaf Wetness
			{ 246, 246, 1 },  // Lux
			{ 250, 250, 1 },  // P1 Smart Meter
			{ 2,   22, 2  },  // Percentage (overlaps baro, use direct)
			{ 1,   22, 9 },	  // Pressure
			{ 85,  85, 3 },	  // Rain
			{ 241, 241, 1 },  // RGB Switch
			{ 93,  93, 1 },	  // Scale
			{ 1002,244, 2 },  // Selector Switch
			{ 15,  22, 17 },  // Soil Moisture
			{ 20,  22, 21 },  // Solar Radiation
			{ 10,  22, 12 },  // Sound Level
			{ 6,   244, 73 }, // Switch
			{ 80,  80, 5 },	  // Temperature
			{ 82,  82, 1 },	  // Temp+Hum
			{ 84,  84, 1 },	  // Temp+Hum+Baro
			{ 5,   22, 19 },  // Text
			{ 8,   242, 1 },  // Thermostat Setpoint
			{ 248, 250, 1 },  // Usage Electric (reuse P1)
			{ 87,  87, 1 },	  // UV
			{ 12,  22, 14 },  // Visibility
			{ 4,   22, 8 },	  // Voltage
			{ 86,  86, 1 },	  // Wind
			{ 0,   0,  0 }    // sentinel
		};

		int iType = -1, iSubType = -1;
		for (int i = 0; sensorMap[i].mappedvalue != 0 || i == 0; i++)
		{
			if (sensorMap[i].mappedvalue == 0 && i > 0)
				break;
			if (sensorMap[i].mappedvalue == nSensorType)
			{
				iType    = sensorMap[i].type;
				iSubType = sensorMap[i].subtype;
				break;
			}
		}

		if (iType == -1)
		{
			Json::Value tool;
			tool["type"] = "text";
			tool["text"] = "Unknown sensor type: " + std::to_string(nSensorType) + ". Common types: 80=Temperature, 81=Humidity, 5=Text, 6=Switch, 113=Counter, 18=Electric, 82=Temp+Hum";
			jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
			jsonRPCRep["result"]["content"].append(tool);
			jsonRPCRep["result"]["isError"] = true;
			return true;
		}

		// Generate a unique device ID (nid)
		auto maxResult = m_sql.safe_query("SELECT MAX(ID) FROM DeviceStatus");
		unsigned long nid = 1;
		if (!maxResult.empty() && !maxResult[0][0].empty())
			nid = atol(maxResult[0][0].c_str()) + 1;
		unsigned long vs_idx = nid;
		nid += 82000;

		bool bPrevAccept = m_sql.m_bAcceptNewHardware;
		m_sql.m_bAcceptNewHardware = true;
		std::string soptions;
		uint64_t DeviceRowIdx = m_sql.CreateDevice(nHwIdx, iType, iSubType, sSensorName, nid, soptions, "MCP");
		m_sql.m_bAcceptNewHardware = bPrevAccept;

		std::string sResult;
		bool bOK = (DeviceRowIdx != (uint64_t)-1);
		if (bOK)
			sResult = "Virtual sensor \"" + sSensorName + "\" created successfully with idx=" + std::to_string(DeviceRowIdx) + ".";
		else
			sResult = "Failed to create virtual sensor \"" + sSensorName + "\".";

		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = !bOK;
		return true;
	}

	bool updateDeviceValue(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") ||
		    !jsonRequest["params"]["arguments"].isMember("switchname") ||
		    !jsonRequest["params"]["arguments"].isMember("nvalue"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: updateDeviceValue: Missing required parameters 'switchname' or 'nvalue'");
			return false;
		}

		std::string sName  = jsonRequest["params"]["arguments"]["switchname"].asString();
		int nValue         = jsonRequest["params"]["arguments"]["nvalue"].asInt();
		std::string sValue;
		if (jsonRequest["params"]["arguments"].isMember("svalue"))
			sValue = jsonRequest["params"]["arguments"]["svalue"].asString();

		Json::Value device;
		bool bFound = getDeviceByName(sName, device);
		std::string sResult;
		if (bFound)
		{
			int nIdx = atoi(device["idx"].asString().c_str());
			bool bOK = m_mainworker.UpdateDevice(nIdx, nValue, sValue, "MCP");
			if (bOK)
				sResult = "Device \"" + sName + "\" updated: nValue=" + std::to_string(nValue) + (sValue.empty() ? "" : ", sValue=\"" + sValue + "\"");
			else
				sResult = "Failed to update device \"" + sName + "\".";
			bFound = bOK;
		}
		else
		{
			sResult = "No device found with name \"" + sName + "\"";
		}

		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = !bFound;
		return true;
	}

	bool getDeviceHistory(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") || !jsonRequest["params"]["arguments"].isMember("switchname"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: getDeviceHistory: Missing required parameter 'switchname'");
			return false;
		}

		std::string sName = jsonRequest["params"]["arguments"]["switchname"].asString();
		std::string sLogType = "switch";
		if (jsonRequest["params"]["arguments"].isMember("log_type"))
			sLogType = jsonRequest["params"]["arguments"]["log_type"].asString();

		Json::Value device;
		bool bFound = getDeviceByName(sName, device);
		std::string sResult;

		if (bFound)
		{
			int nIdx = atoi(device["idx"].asString().c_str());

			if (sLogType == "graph")
			{
				// Graph history from Temperature_Calendar or Meter_Calendar
				auto result = m_sql.safe_query(
					"SELECT Date, Max_Val, Min_Val, Avg_Val FROM Temperature_Calendar WHERE DeviceRowID=%d ORDER BY Date DESC LIMIT 50", nIdx);
				if (!result.empty())
				{
					sResult = std::to_string((int)result.size()) + " graph entries for \"" + sName + "\":\n";
					for (const auto &row : result)
						sResult += "  " + row[0] + " max=" + row[1] + " min=" + row[2] + " avg=" + row[3] + "\n";
				}
				else
				{
					// Try Meter_Calendar
					auto mResult = m_sql.safe_query(
						"SELECT Date, Value, Counter FROM Meter_Calendar WHERE DeviceRowID=%d ORDER BY Date DESC LIMIT 50", nIdx);
					if (!mResult.empty())
					{
						sResult = std::to_string((int)mResult.size()) + " meter entries for \"" + sName + "\":\n";
						for (const auto &row : mResult)
							sResult += "  " + row[0] + " value=" + row[1] + " counter=" + row[2] + "\n";
					}
					else
					{
						sResult = "No graph history found for \"" + sName + "\"";
					}
				}
			}
			else
			{
				// Switch/text log from LightingLog
				auto result = m_sql.safe_query(
					"SELECT Date, nValue, sValue, User FROM LightingLog WHERE DeviceRowID=%d ORDER BY Date DESC LIMIT 50", nIdx);
				if (!result.empty())
				{
					sResult = std::to_string((int)result.size()) + " log entries for \"" + sName + "\":\n";
					for (const auto &row : result)
					{
						sResult += "  " + row[0] + " nValue=" + row[1];
						if (!row[2].empty())
							sResult += " sValue=\"" + row[2] + "\"";
						if (!row[3].empty())
							sResult += " user=" + row[3];
						sResult += "\n";
					}
				}
				else
				{
					sResult = "No log history found for \"" + sName + "\"";
				}
			}
		}
		else
		{
			sResult = "No device found with name \"" + sName + "\"";
		}

		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = !bFound;
		return true;
	}

	// -------------------------------------------------------------------------
	// -------------------------------------------------------------------------

	bool getUserVariables(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		auto result = m_sql.safe_query("SELECT ID, Name, ValueType, Value, LastUpdate FROM UserVariables ORDER BY Name");
		std::string sResult;
		if (result.empty())
		{
			sResult = "No user variables defined.";
		}
		else
		{
			sResult = std::to_string((int)result.size()) + " variable(s):\n";
			for (const auto &row : result)
			{
				int vtype = atoi(row[2].c_str());
				const char *sTypeName = (vtype >= 0 && vtype <= 4) ? kVarTypeNames[vtype] : "Unknown";
				sResult += "- \"" + row[1] + "\" [" + sTypeName + ", idx=" + row[0] + "] = " + row[3] + " (updated: " + row[4] + ")\n";
			}
		}

		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = false;
		return true;
	}

	bool addUserVariable(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") ||
		    !jsonRequest["params"]["arguments"].isMember("name") ||
		    !jsonRequest["params"]["arguments"].isMember("vtype") ||
		    !jsonRequest["params"]["arguments"].isMember("value"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: addUserVariable: Missing required parameters");
			return false;
		}

		std::string sName  = jsonRequest["params"]["arguments"]["name"].asString();
		int vtype          = jsonRequest["params"]["arguments"]["vtype"].asInt();
		std::string sValue = jsonRequest["params"]["arguments"]["value"].asString();

		if (vtype < 0 || vtype > 4)
		{
			Json::Value tool;
			tool["type"] = "text";
			tool["text"] = "Invalid variable type " + std::to_string(vtype) + ". Must be 0-4 (Integer/Float/String/Date/Time).";
			jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
			jsonRPCRep["result"]["content"].append(tool);
			jsonRPCRep["result"]["isError"] = true;
			return true;
		}

		// Check for duplicate name
		auto existing = m_sql.safe_query("SELECT ID FROM UserVariables WHERE Name='%q'", sName.c_str());
		if (!existing.empty())
		{
			Json::Value tool;
			tool["type"] = "text";
			tool["text"] = "A variable named \"" + sName + "\" already exists (idx=" + existing[0][0] + ").";
			jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
			jsonRPCRep["result"]["content"].append(tool);
			jsonRPCRep["result"]["isError"] = true;
			return true;
		}

		std::string errorMessage;
		bool bOK = m_sql.AddUserVariable(sName, (_eUsrVariableType)vtype, sValue, errorMessage);
		if (bOK)
			m_mainworker.m_eventsystem.GetCurrentUserVariables();

		std::string sResult = bOK
			? "User variable \"" + sName + "\" created successfully."
			: "Failed to create variable \"" + sName + "\": " + errorMessage;

		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = !bOK;
		return true;
	}

	bool updateUserVariable(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") ||
		    !jsonRequest["params"]["arguments"].isMember("name") ||
		    !jsonRequest["params"]["arguments"].isMember("value"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: updateUserVariable: Missing required parameters 'name' or 'value'");
			return false;
		}

		std::string sName  = jsonRequest["params"]["arguments"]["name"].asString();
		std::string sValue = jsonRequest["params"]["arguments"]["value"].asString();

		auto result = m_sql.safe_query("SELECT ID, ValueType FROM UserVariables WHERE Name='%q'", sName.c_str());
		if (result.empty())
		{
			Json::Value tool;
			tool["type"] = "text";
			tool["text"] = "No user variable found with name \"" + sName + "\"";
			jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
			jsonRPCRep["result"]["content"].append(tool);
			jsonRPCRep["result"]["isError"] = true;
			return true;
		}

		std::string sIdx = result[0][0];
		int currentType  = atoi(result[0][1].c_str());
		int newType      = currentType;
		if (jsonRequest["params"]["arguments"].isMember("vtype"))
			newType = jsonRequest["params"]["arguments"]["vtype"].asInt();

		std::string errorMessage;
		bool bOK = m_sql.UpdateUserVariable(sIdx, sName, (_eUsrVariableType)newType, sValue, (newType == currentType), errorMessage);
		if (bOK)
			m_mainworker.m_eventsystem.GetCurrentUserVariables();

		std::string sResult = bOK
			? "User variable \"" + sName + "\" updated successfully."
			: "Failed to update variable \"" + sName + "\": " + errorMessage;

		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = !bOK;
		return true;
	}

	bool deleteUserVariable(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") || !jsonRequest["params"]["arguments"].isMember("name"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: deleteUserVariable: Missing required parameter 'name'");
			return false;
		}

		std::string sName = jsonRequest["params"]["arguments"]["name"].asString();

		auto result = m_sql.safe_query("SELECT ID FROM UserVariables WHERE Name='%q'", sName.c_str());
		if (result.empty())
		{
			Json::Value tool;
			tool["type"] = "text";
			tool["text"] = "No user variable found with name \"" + sName + "\"";
			jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
			jsonRPCRep["result"]["content"].append(tool);
			jsonRPCRep["result"]["isError"] = true;
			return true;
		}

		std::string sIdx = result[0][0];
		m_sql.DeleteUserVariable(sIdx);
		m_mainworker.m_eventsystem.GetCurrentUserVariables();

		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = "User variable \"" + sName + "\" deleted successfully.";
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = false;
		return true;
	}

	// -------------------------------------------------------------------------
	// -------------------------------------------------------------------------

	bool addLogMessage(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") || !jsonRequest["params"]["arguments"].isMember("message"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: addLogMessage: Missing required parameter 'message'");
			return false;
		}

		std::string sMessage = jsonRequest["params"]["arguments"]["message"].asString();
		std::string sLevel   = "normal";
		if (jsonRequest["params"]["arguments"].isMember("level"))
			sLevel = jsonRequest["params"]["arguments"]["level"].asString();

		if (sLevel == "error")
			_log.Log(LOG_ERROR, "MCP: %s", sMessage.c_str());
		else if (sLevel == "status")
			_log.Log(LOG_STATUS, "MCP: %s", sMessage.c_str());
		else
			_log.Log(LOG_NORM, "MCP: %s", sMessage.c_str());

		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = "Log message written at level \"" + sLevel + "\": " + sMessage;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = false;
		return true;
	}

	bool sendNotification(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") ||
		    !jsonRequest["params"]["arguments"].isMember("subject") ||
		    !jsonRequest["params"]["arguments"].isMember("body"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: sendNotification: Missing required parameters 'subject' or 'body'");
			return false;
		}

		std::string sSubject = jsonRequest["params"]["arguments"]["subject"].asString();
		std::string sBody    = jsonRequest["params"]["arguments"]["body"].asString();
		int iPriority = 0;
		if (jsonRequest["params"]["arguments"].isMember("priority"))
			iPriority = jsonRequest["params"]["arguments"]["priority"].asInt();

		// Clamp priority to valid range
		if (iPriority < -2) iPriority = -2;
		if (iPriority >  2) iPriority =  2;

		bool bOK = m_notifications.SendMessage(0, std::string(""), NOTIFYALL, std::string(""), sSubject, sBody, std::string(""), iPriority, std::string(""), false);

		std::string sResult = bOK
			? "Notification sent: \"" + sSubject + "\""
			: "Failed to send notification. No notification services may be configured.";

		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = !bOK;
		return true;
	}

	// -------------------------------------------------------------------------
	// -------------------------------------------------------------------------

	bool getEvents(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		auto result = m_sql.safe_query(
			"SELECT ID, Name, Status, Interpreter FROM EventMaster ORDER BY Name");

		std::string sResult;
		if (result.empty())
		{
			sResult = "No event scripts defined.";
		}
		else
		{
			sResult = std::to_string((int)result.size()) + " event script(s):\n";
			for (const auto &row : result)
			{
				bool bEnabled = (atoi(row[2].c_str()) == 1);
				sResult += "- \"" + row[1] + "\" [" + row[3] + ", " + (bEnabled ? "enabled" : "disabled") + ", idx=" + row[0] + "]\n";
			}
		}

		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = false;
		return true;
	}

	bool getEvent(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") || !jsonRequest["params"]["arguments"].isMember("event_name"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: getEvent: Missing required parameter 'event_name'");
			return false;
		}

		std::string sEventName = jsonRequest["params"]["arguments"]["event_name"].asString();

		auto result = m_sql.safe_query(
			"SELECT ID, Name, XMLStatement, Status, Interpreter FROM EventMaster WHERE Name='%q'",
			sEventName.c_str());

		std::string sResult;
		bool bFound = !result.empty();
		if (bFound)
		{
			auto &row = result[0];
			bool bEnabled = (atoi(row[3].c_str()) == 1);
			sResult = "Event script \"" + row[1] + "\" [" + row[4] + ", " + (bEnabled ? "enabled" : "disabled") + ", idx=" + row[0] + "]:\n";
			sResult += "--- BEGIN SCRIPT ---\n";
			sResult += row[2];
			sResult += "\n--- END SCRIPT ---";
		}
		else
		{
			sResult = "No event script found with name \"" + sEventName + "\"";
		}

		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = !bFound;
		return true;
	}

	bool createEvent(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") ||
		    !jsonRequest["params"]["arguments"].isMember("name") ||
		    !jsonRequest["params"]["arguments"].isMember("interpreter") ||
		    !jsonRequest["params"]["arguments"].isMember("code"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: createEvent: Missing required parameters");
			return false;
		}

		std::string sName        = jsonRequest["params"]["arguments"]["name"].asString();
		std::string sInterpreter = jsonRequest["params"]["arguments"]["interpreter"].asString();
		std::string sCode        = jsonRequest["params"]["arguments"]["code"].asString();
		bool bEnabled = true;
		if (jsonRequest["params"]["arguments"].isMember("enabled"))
			bEnabled = jsonRequest["params"]["arguments"]["enabled"].asBool();

		// Validate interpreter
		if (sInterpreter != "Lua" && sInterpreter != "dzVents" && sInterpreter != "Python" && sInterpreter != "Blockly")
		{
			Json::Value tool;
			tool["type"] = "text";
			tool["text"] = "Invalid interpreter \"" + sInterpreter + "\". Must be one of: Lua, dzVents, Python, Blockly.";
			jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
			jsonRPCRep["result"]["content"].append(tool);
			jsonRPCRep["result"]["isError"] = true;
			return true;
		}

		// Check for duplicate name
		auto existing = m_sql.safe_query("SELECT ID FROM EventMaster WHERE Name='%q'", sName.c_str());
		if (!existing.empty())
		{
			Json::Value tool;
			tool["type"] = "text";
			tool["text"] = "An event script named \"" + sName + "\" already exists (idx=" + existing[0][0] + ").";
			jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
			jsonRPCRep["result"]["content"].append(tool);
			jsonRPCRep["result"]["isError"] = true;
			return true;
		}

		m_sql.safe_query(
			"INSERT INTO EventMaster (Name, Interpreter, Type, XMLStatement, Status) VALUES ('%q','%q','All','%q',%d)",
			sName.c_str(), sInterpreter.c_str(), sCode.c_str(), bEnabled ? 1 : 0);

		// Get the new idx
		auto newResult = m_sql.safe_query("SELECT ID FROM EventMaster WHERE Name='%q'", sName.c_str());
		std::string sNewIdx = newResult.empty() ? "?" : newResult[0][0];

		m_mainworker.m_eventsystem.LoadEvents();

		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = "Event script \"" + sName + "\" created successfully (idx=" + sNewIdx + ", interpreter=" + sInterpreter + ", " + (bEnabled ? "enabled" : "disabled") + ").";
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = false;
		return true;
	}

	bool updateEvent(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") || !jsonRequest["params"]["arguments"].isMember("event_name"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: updateEvent: Missing required parameter 'event_name'");
			return false;
		}

		std::string sEventName = jsonRequest["params"]["arguments"]["event_name"].asString();
		const Json::Value &args = jsonRequest["params"]["arguments"];

		auto result = m_sql.safe_query("SELECT ID FROM EventMaster WHERE Name='%q'", sEventName.c_str());
		if (result.empty())
		{
			Json::Value tool;
			tool["type"] = "text";
			tool["text"] = "No event script found with name \"" + sEventName + "\"";
			jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
			jsonRPCRep["result"]["content"].append(tool);
			jsonRPCRep["result"]["isError"] = true;
			return true;
		}

		int nIdx = atoi(result[0][0].c_str());
		bool bAnyChange = false;

		if (args.isMember("code"))
		{
			m_sql.safe_query("UPDATE EventMaster SET XMLStatement='%q' WHERE ID=%d",
				args["code"].asString().c_str(), nIdx);
			bAnyChange = true;
		}
		if (args.isMember("enabled"))
		{
			m_sql.safe_query("UPDATE EventMaster SET Status=%d WHERE ID=%d",
				args["enabled"].asBool() ? 1 : 0, nIdx);
			bAnyChange = true;
		}
		if (args.isMember("new_name"))
		{
			std::string sNewName = args["new_name"].asString();
			m_sql.safe_query("UPDATE EventMaster SET Name='%q' WHERE ID=%d",
				sNewName.c_str(), nIdx);
			bAnyChange = true;
		}

		if (bAnyChange)
			m_mainworker.m_eventsystem.LoadEvents();

		std::string sResult = bAnyChange
			? "Event script \"" + sEventName + "\" updated successfully."
			: "No changes applied to event script \"" + sEventName + "\" (no update parameters provided).";

		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = false;
		return true;
	}

	// =========================================================================
	// =========================================================================

	bool setSwitchState(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") ||
			!jsonRequest["params"]["arguments"].isMember("switchname") ||
			!jsonRequest["params"]["arguments"].isMember("state"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: setSwitchState: Missing required parameter 'switchname' or 'state'");
			return false;
		}
		std::string sSwitchName = jsonRequest["params"]["arguments"]["switchname"].asString();
		std::string sState = jsonRequest["params"]["arguments"]["state"].asString();

		// Capitalize first letter, lowercase rest — matches "On"/"Off" format
		if (!sState.empty()) {
			sState[0] = toupper(sState[0]);
			for (size_t i = 1; i < sState.size(); i++)
				sState[i] = tolower(sState[i]);
		}

		if (sState != "On" && sState != "Off")
		{
			Json::Value toolContent;
			toolContent["type"] = "text";
			toolContent["text"] = "Invalid state '" + sState + "'. Must be 'On' or 'Off'.";
			jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
			jsonRPCRep["result"]["content"].append(toolContent);
			jsonRPCRep["result"]["isError"] = true;
			return true;
		}

		Json::Value device;
		bool bFound = getDeviceByName(sSwitchName, device);
		std::string sResult;
		if (!bFound)
		{
			sResult = "No switch exists with the name \"" + sSwitchName + "\".";
		}
		else
		{
			auto rc = m_mainworker.SwitchLight(device["idx"].asString(), sState, "", "", "", 0, "MCP");
			if (rc == MainWorker::eSwitchLightReturnCode::SL_ERROR)
				sResult = "Error setting switch \"" + sSwitchName + "\" to " + sState + ".";
			else if (rc == MainWorker::eSwitchLightReturnCode::SL_OK_NO_ACTION)
				sResult = "Switch \"" + sSwitchName + "\" was already " + sState + ". No action taken.";
			else
				sResult = "Switch \"" + sSwitchName + "\" set to " + sState + " successfully.";
		}
		Json::Value toolContent;
		toolContent["type"] = "text";
		toolContent["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(toolContent);
		jsonRPCRep["result"]["isError"] = !bFound;
		return true;
	}

	bool setDimmerLevel(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") ||
			!jsonRequest["params"]["arguments"].isMember("switchname") ||
			!jsonRequest["params"]["arguments"].isMember("level"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: setDimmerLevel: Missing required parameter 'switchname' or 'level'");
			return false;
		}
		std::string sSwitchName = jsonRequest["params"]["arguments"]["switchname"].asString();
		int iLevel = jsonRequest["params"]["arguments"]["level"].asInt();
		iLevel = std::max(0, std::min(100, iLevel));

		Json::Value device;
		bool bFound = getDeviceByName(sSwitchName, device);
		std::string sResult;
		if (!bFound)
		{
			sResult = "No device exists with the name \"" + sSwitchName + "\".";
		}
		else
		{
			auto rc = m_mainworker.SwitchLight(device["idx"].asString(), "Set Level", std::to_string(iLevel), "", "", 0, "MCP");
			sResult = (rc == MainWorker::eSwitchLightReturnCode::SL_ERROR)
				? "Error setting dimmer level on \"" + sSwitchName + "\"."
				: "Dimmer \"" + sSwitchName + "\" set to level " + std::to_string(iLevel) + ".";
		}
		Json::Value toolContent;
		toolContent["type"] = "text";
		toolContent["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(toolContent);
		jsonRPCRep["result"]["isError"] = !bFound;
		return true;
	}

	bool controlBlinds(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") ||
			!jsonRequest["params"]["arguments"].isMember("switchname") ||
			!jsonRequest["params"]["arguments"].isMember("command"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: controlBlinds: Missing required parameter 'switchname' or 'command'");
			return false;
		}
		std::string sSwitchName = jsonRequest["params"]["arguments"]["switchname"].asString();
		std::string sCommand = jsonRequest["params"]["arguments"]["command"].asString();

		// Capitalize first letter, lowercase rest — matches "Open"/"Close"/"Stop" format
		if (!sCommand.empty()) {
			sCommand[0] = toupper(sCommand[0]);
			for (size_t i = 1; i < sCommand.size(); i++)
				sCommand[i] = tolower(sCommand[i]);
		}

		if (sCommand != "Open" && sCommand != "Close" && sCommand != "Stop")
		{
			Json::Value toolContent;
			toolContent["type"] = "text";
			toolContent["text"] = "Invalid command '" + sCommand + "'. Must be 'Open', 'Close', or 'Stop'.";
			jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
			jsonRPCRep["result"]["content"].append(toolContent);
			jsonRPCRep["result"]["isError"] = true;
			return true;
		}

		Json::Value device;
		bool bFound = getDeviceByName(sSwitchName, device);
		std::string sResult;
		if (!bFound)
		{
			sResult = "No device exists with the name \"" + sSwitchName + "\".";
		}
		else
		{
			auto rc = m_mainworker.SwitchLight(device["idx"].asString(), sCommand, "", "", "", 0, "MCP");
			sResult = (rc == MainWorker::eSwitchLightReturnCode::SL_ERROR)
				? "Error sending " + sCommand + " to \"" + sSwitchName + "\"."
				: "Command " + sCommand + " sent to \"" + sSwitchName + "\" successfully.";
		}
		Json::Value toolContent;
		toolContent["type"] = "text";
		toolContent["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(toolContent);
		jsonRPCRep["result"]["isError"] = !bFound;
		return true;
	}

	bool setColorBrightness(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") ||
			!jsonRequest["params"]["arguments"].isMember("switchname") ||
			!jsonRequest["params"]["arguments"].isMember("hue") ||
			!jsonRequest["params"]["arguments"].isMember("brightness"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: setColorBrightness: Missing required parameters");
			return false;
		}
		std::string sSwitchName = jsonRequest["params"]["arguments"]["switchname"].asString();
		float fHue = (float)jsonRequest["params"]["arguments"]["hue"].asInt();
		int iBrightness = jsonRequest["params"]["arguments"]["brightness"].asInt();
		bool bIsWhite = false;
		if (jsonRequest["params"]["arguments"].isMember("iswhite"))
			bIsWhite = jsonRequest["params"]["arguments"]["iswhite"].asBool();

		iBrightness = std::max(0, std::min(100, iBrightness));
		fHue = std::max(0.0F, std::min(360.0F, fHue));

		Json::Value device;
		bool bFound = getDeviceByName(sSwitchName, device);
		std::string sResult;
		if (!bFound)
		{
			sResult = "No device exists with the name \"" + sSwitchName + "\".";
		}
		else
		{
			// Convert hue to RGB using full saturation, normalised to full brightness
			int r, g, b;
			hsb2rgb(fHue, 1.0F, 1.0F, r, g, b, 255);
			_tColor color((uint8_t)r, (uint8_t)g, (uint8_t)b, 0, 0, ColorModeRGB);
			if (bIsWhite)
				color.mode = ColorModeWhite;

			uint64_t uIdx;
			try {
				uIdx = std::stoull(device["idx"].asString());
			} catch (const std::exception &e) {
				_log.Debug(DEBUG_WEBSERVER, "MCP: Invalid device idx: %s", e.what());
				Json::Value tool;
				tool["type"] = "text";
				tool["text"] = "Internal error: invalid device index.";
				jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
				jsonRPCRep["result"]["content"].append(tool);
				jsonRPCRep["result"]["isError"] = true;
				return true;
			}
			auto rc = m_mainworker.SwitchLight(uIdx, "Set Color", (unsigned char)iBrightness, color, false, 0, "MCP");
			sResult = (rc == MainWorker::eSwitchLightReturnCode::SL_ERROR)
				? "Error setting color on \"" + sSwitchName + "\"."
				: "Color set on \"" + sSwitchName + "\": hue=" + std::to_string((int)fHue) + ", brightness=" + std::to_string(iBrightness) + ".";
		}
		Json::Value toolContent;
		toolContent["type"] = "text";
		toolContent["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(toolContent);
		jsonRPCRep["result"]["isError"] = !bFound;
		return true;
	}

	bool setColorTemperature(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") ||
			!jsonRequest["params"]["arguments"].isMember("switchname") ||
			!jsonRequest["params"]["arguments"].isMember("kelvin"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: setColorTemperature: Missing required parameters");
			return false;
		}
		std::string sSwitchName = jsonRequest["params"]["arguments"]["switchname"].asString();
		int iKelvin = jsonRequest["params"]["arguments"]["kelvin"].asInt();

		// Map Kelvin to Domoticz ColorModeTemp level (0-100):
		// 6500K (cool/daylight) maps to level 0, 2700K (warm/incandescent) maps to level 100.
		// Range is clamped to the typical tunable-white range supported by Domoticz (2700K-6500K).
		iKelvin = std::max(2700, std::min(6500, iKelvin));
		double dLevel = 100.0 * (double)(6500 - iKelvin) / (double)(6500 - 2700);
		int iLevel = (int)round(dLevel);
		iLevel = std::max(0, std::min(100, iLevel));

		Json::Value device;
		bool bFound = getDeviceByName(sSwitchName, device);
		std::string sResult;
		if (!bFound)
		{
			sResult = "No device exists with the name \"" + sSwitchName + "\".";
		}
		else
		{
			uint8_t tVal = (uint8_t)(int)round(iLevel * 255.0 / 100.0);
			_tColor color(tVal, ColorModeTemp);
			uint64_t uIdx;
			try {
				uIdx = std::stoull(device["idx"].asString());
			} catch (const std::exception &e) {
				_log.Debug(DEBUG_WEBSERVER, "MCP: Invalid device idx: %s", e.what());
				Json::Value tool;
				tool["type"] = "text";
				tool["text"] = "Internal error: invalid device index.";
				jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
				jsonRPCRep["result"]["content"].append(tool);
				jsonRPCRep["result"]["isError"] = true;
				return true;
			}
			auto rc = m_mainworker.SwitchLight(uIdx, "Set Color", -1, color, false, 0, "MCP");
			sResult = (rc == MainWorker::eSwitchLightReturnCode::SL_ERROR)
				? "Error setting color temperature on \"" + sSwitchName + "\"."
				: "Color temperature on \"" + sSwitchName + "\" set to " + std::to_string(iKelvin) + "K.";
		}
		Json::Value toolContent;
		toolContent["type"] = "text";
		toolContent["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(toolContent);
		jsonRPCRep["result"]["isError"] = !bFound;
		return true;
	}

	// =========================================================================
	// =========================================================================

	bool getScenes(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		auto result = m_sql.safe_query(
			"SELECT ID, Name, SceneType, LastUpdate, Status FROM Scenes ORDER BY Name");

		std::string sResult;
		if (result.empty())
		{
			sResult = "No scenes or groups found.";
		}
		else
		{
			sResult = std::to_string(result.size()) + " scene(s)/group(s) found:\n";
			for (const auto &row : result)
			{
				std::string sIdx = row[0];
				std::string sName = row[1];
				int iSceneType = atoi(row[2].c_str());
				std::string sLastUpdate = row[3];
				std::string sStatus = row[4];
				std::string sType = (iSceneType == 1) ? "Group" : "Scene";
				sResult += "- \"" + sName + "\" [" + sType + ", idx=" + sIdx;
				if (!sStatus.empty())
					sResult += ", Status=" + sStatus;
				if (!sLastUpdate.empty())
					sResult += ", Last: " + sLastUpdate;
				sResult += "]\n";
			}
		}
		Json::Value toolContent;
		toolContent["type"] = "text";
		toolContent["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(toolContent);
		jsonRPCRep["result"]["isError"] = false;
		return true;
	}

	bool switchScene(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") ||
			!jsonRequest["params"]["arguments"].isMember("scenename") ||
			!jsonRequest["params"]["arguments"].isMember("command"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: switchScene: Missing required parameter 'scenename' or 'command'");
			return false;
		}
		std::string sSceneName = jsonRequest["params"]["arguments"]["scenename"].asString();
		std::string sCommand = jsonRequest["params"]["arguments"]["command"].asString();

		// Capitalize first letter, lowercase rest — matches "On"/"Off" format
		if (!sCommand.empty()) {
			sCommand[0] = toupper(sCommand[0]);
			for (size_t i = 1; i < sCommand.size(); i++)
				sCommand[i] = tolower(sCommand[i]);
		}

		if (sCommand != "On" && sCommand != "Off")
		{
			Json::Value toolContent;
			toolContent["type"] = "text";
			toolContent["text"] = "Invalid command '" + sCommand + "'. Must be 'On' or 'Off'.";
			jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
			jsonRPCRep["result"]["content"].append(toolContent);
			jsonRPCRep["result"]["isError"] = true;
			return true;
		}

		auto result = m_sql.safe_query("SELECT ID FROM Scenes WHERE Name='%q'", sSceneName.c_str());
		std::string sResult;
		bool bFound = !result.empty();
		if (!bFound)
		{
			sResult = "No scene or group exists with the name \"" + sSceneName + "\".";
		}
		else
		{
			std::string sIdx = result[0][0];
			bool bOk = m_mainworker.SwitchScene(sIdx, sCommand, "MCP");
			sResult = bOk
				? "Scene \"" + sSceneName + "\" switched " + sCommand + " successfully."
				: "Error switching scene \"" + sSceneName + "\".";
		}
		Json::Value toolContent;
		toolContent["type"] = "text";
		toolContent["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(toolContent);
		jsonRPCRep["result"]["isError"] = !bFound;
		return true;
	}

	bool getRooms(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		auto result = m_sql.safe_query(
			"SELECT ID, Name, [Order] FROM Plans WHERE (Name!='') ORDER BY Name");

		std::string sResult;
		if (result.empty())
		{
			sResult = "No rooms (plans) found.";
		}
		else
		{
			sResult = std::to_string(result.size()) + " room(s) found:\n";
			for (const auto &row : result)
			{
				sResult += "- \"" + row[1] + "\" [idx=" + row[0] + "]\n";
			}
		}
		Json::Value toolContent;
		toolContent["type"] = "text";
		toolContent["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(toolContent);
		jsonRPCRep["result"]["isError"] = false;
		return true;
	}

	bool getRoomDevices(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") ||
			!jsonRequest["params"]["arguments"].isMember("roomname"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: getRoomDevices: Missing required parameter 'roomname'");
			return false;
		}
		std::string sRoomName = jsonRequest["params"]["arguments"]["roomname"].asString();

		auto planResult = m_sql.safe_query("SELECT ID FROM Plans WHERE Name='%q'", sRoomName.c_str());
		std::string sResult;
		bool bFound = !planResult.empty();
		if (!bFound)
		{
			sResult = "No room (plan) exists with the name \"" + sRoomName + "\".";
		}
		else
		{
			std::string sPlanID = planResult[0][0];
			Json::Value jsonDevices;
			m_webservers.GetJSonDevices(jsonDevices, "true", "", "Name", "", sPlanID, "", false, false, false, 0, "", "");
			int iCount = 0;
			std::string sDeviceList;
			if (jsonDevices.isObject() && jsonDevices.isMember("result"))
			{
				for (const auto &device : jsonDevices["result"])
				{
					if (!device.isObject() || !device.isMember("Name"))
						continue;
					iCount++;
					sDeviceList += "- \"" + device["Name"].asString() + "\"";
					if (device.isMember("Type"))
						sDeviceList += " [" + device["Type"].asString();
					if (device.isMember("SubType"))
						sDeviceList += "/" + device["SubType"].asString();
					if (device.isMember("Type"))
						sDeviceList += "]";
					if (device.isMember("Data"))
						sDeviceList += " = " + device["Data"].asString();
					sDeviceList += "\n";
				}
			}
			if (iCount == 0)
				sResult = "Room \"" + sRoomName + "\" has no devices assigned.";
			else
				sResult = std::to_string(iCount) + " device(s) in room \"" + sRoomName + "\":\n" + sDeviceList;
		}
		Json::Value toolContent;
		toolContent["type"] = "text";
		toolContent["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(toolContent);
		jsonRPCRep["result"]["isError"] = !bFound;
		return true;
	}

	bool getSceneDevices(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") ||
			!jsonRequest["params"]["arguments"].isMember("scenename"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: getSceneDevices: Missing required parameter 'scenename'");
			return false;
		}
		std::string sSceneName = jsonRequest["params"]["arguments"]["scenename"].asString();

		auto scResult = m_sql.safe_query("SELECT ID, SceneType FROM Scenes WHERE Name='%q'", sSceneName.c_str());
		std::string sResult;
		bool bFound = !scResult.empty();
		if (!bFound)
		{
			sResult = "No scene or group exists with the name \"" + sSceneName + "\".";
		}
		else
		{
			std::string sSceneIdx = scResult[0][0];
			auto devResult = m_sql.safe_query(
				"SELECT a.DeviceRowID, b.Name, a.Cmd, a.Level "
				"FROM SceneDevices a JOIN DeviceStatus b ON b.ID=a.DeviceRowID "
				"WHERE a.SceneRowID=%s ORDER BY b.Name", sSceneIdx.c_str());
			if (devResult.empty())
			{
				sResult = "Scene \"" + sSceneName + "\" has no devices assigned.";
			}
			else
			{
				sResult = std::to_string(devResult.size()) + " device(s) in scene \"" + sSceneName + "\":\n";
				for (const auto &row : devResult)
				{
					sResult += "- \"" + row[1] + "\" [idx=" + row[0] + ", cmd=" + row[2];
					if (!row[3].empty() && row[3] != "0")
						sResult += ", level=" + row[3];
					sResult += "]\n";
				}
			}
		}
		Json::Value toolContent;
		toolContent["type"] = "text";
		toolContent["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(toolContent);
		jsonRPCRep["result"]["isError"] = !bFound;
		return true;
	}

	// =========================================================================
	// =========================================================================

	bool getHardware(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		auto result = m_sql.safe_query(
			"SELECT ID, Name, Enabled, Type, Address, Port FROM Hardware ORDER BY Name");

		std::string sResult;
		if (result.empty())
		{
			sResult = "No hardware configured.";
		}
		else
		{
			sResult = std::to_string(result.size()) + " hardware adapter(s):\n";
			for (const auto &row : result)
			{
				std::string sEnabled = (atoi(row[2].c_str()) == 1) ? "Enabled" : "Disabled";
				sResult += "- \"" + row[1] + "\" [idx=" + row[0] + ", type=" + row[3] + ", " + sEnabled;
				if (!row[4].empty())
					sResult += ", addr=" + row[4];
				if (!row[5].empty() && row[5] != "0")
					sResult += ":" + row[5];
				sResult += "]\n";
			}
		}
		Json::Value toolContent;
		toolContent["type"] = "text";
		toolContent["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(toolContent);
		jsonRPCRep["result"]["isError"] = false;
		return true;
	}

	bool getSystemSettings(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		std::string sTitle, sLocation, sLanguage;
		int iTempScale = 0, iWindScale = 0, iSensorTimeout = 0, iBatterLow = 0, iActivePlan = 0;
		m_sql.GetPreferencesVar("Title", sTitle);
		m_sql.GetPreferencesVar("Location", sLocation);
		m_sql.GetPreferencesVar("Language", sLanguage);
		m_sql.GetPreferencesVar("TempScale", iTempScale);
		m_sql.GetPreferencesVar("WindScale", iWindScale);
		m_sql.GetPreferencesVar("SensorTimeout", iSensorTimeout);
		m_sql.GetPreferencesVar("BatterLowNotification", iBatterLow);
		m_sql.GetPreferencesVar("ActiveTimerPlan", iActivePlan);

		std::string sResult = "Domoticz System Settings\n";
		sResult += "========================\n";
		if (!sTitle.empty())
			sResult += "Title: " + sTitle + "\n";
		if (!sLocation.empty())
			sResult += "Location: " + sLocation + "\n";
		if (!sLanguage.empty())
			sResult += "Language: " + sLanguage + "\n";
		sResult += "Temperature scale: " + std::string(iTempScale == 1 ? "Fahrenheit" : "Celsius") + "\n";
		sResult += "Wind scale: " + std::string(iWindScale == 1 ? "mph" : "m/s") + "\n";
		sResult += "Sensor timeout (min): " + std::to_string(iSensorTimeout) + "\n";
		sResult += "Battery low notification threshold: " + std::to_string(iBatterLow) + "%\n";
		sResult += "Active timer plan: " + std::to_string(iActivePlan) + "\n";

		Json::Value toolContent;
		toolContent["type"] = "text";
		toolContent["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(toolContent);
		jsonRPCRep["result"]["isError"] = false;
		return true;
	}

	bool getSunTimes(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		std::string sResult;
		if (m_mainworker.m_LastSunriseSet.empty())
		{
			sResult = "Sun rise/set data is not available yet. Domoticz needs a valid location configured.";
			Json::Value toolContent;
			toolContent["type"] = "text";
			toolContent["text"] = sResult;
			jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
			jsonRPCRep["result"]["content"].append(toolContent);
			jsonRPCRep["result"]["isError"] = true;
			return true;
		}

		std::vector<std::string> strarray;
		StringSplit(m_mainworker.m_LastSunriseSet, ";", strarray);

		sResult = "Solar Times\n";
		sResult += "===========\n";
		if (strarray.size() > 0) sResult += "Sunrise:       " + strarray[0] + "\n";
		if (strarray.size() > 1) sResult += "Sunset:        " + strarray[1] + "\n";
		if (strarray.size() > 2) sResult += "Dawn:          " + strarray[2] + "\n";
		if (strarray.size() > 3) sResult += "Dusk:          " + strarray[3] + "\n";
		if (strarray.size() > 4) sResult += "Solar noon:    " + strarray[4] + "\n";
		if (strarray.size() > 5) sResult += "Civil dawn:    " + strarray[5] + "\n";
		if (strarray.size() > 6) sResult += "Civil dusk:    " + strarray[6] + "\n";
		if (strarray.size() > 7) sResult += "Nautical dawn: " + strarray[7] + "\n";
		if (strarray.size() > 8) sResult += "Nautical dusk: " + strarray[8] + "\n";
		if (strarray.size() > 9) sResult += "Day length:    " + strarray[9] + "\n";

		Json::Value toolContent;
		toolContent["type"] = "text";
		toolContent["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(toolContent);
		jsonRPCRep["result"]["isError"] = false;
		return true;
	}

	bool getCameras(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		// Do NOT include password or auth token columns
		auto result = m_sql.safe_query(
			"SELECT ID, Name, Enabled, Address, Port, Protocol FROM Cameras ORDER BY Name");

		std::string sResult;
		if (result.empty())
		{
			sResult = "No cameras configured.";
		}
		else
		{
			sResult = std::to_string(result.size()) + " camera(s) configured:\n";
			for (const auto &row : result)
			{
				std::string sEnabled = (atoi(row[2].c_str()) == 1) ? "Enabled" : "Disabled";
				std::string sProtocol;
				int iProtocol = atoi(row[5].c_str());
				switch (iProtocol)
				{
					case 0: sProtocol = "HTTP"; break;
					case 1: sProtocol = "HTTPS"; break;
					default: sProtocol = "Unknown"; break;
				}
				sResult += "- \"" + row[1] + "\" [idx=" + row[0] + ", " + sEnabled + ", " + sProtocol + "://" + row[3] + ":" + row[4] + "]\n";
			}
		}
		Json::Value toolContent;
		toolContent["type"] = "text";
		toolContent["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(toolContent);
		jsonRPCRep["result"]["isError"] = false;
		return true;
	}

	bool getFloorplans(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		auto result = m_sql.safe_query("SELECT ID, Name, ScaleFactor FROM Floorplans ORDER BY Name");

		std::string sResult;
		if (result.empty())
		{
			sResult = "No floorplans found.";
		}
		else
		{
			sResult = std::to_string(result.size()) + " floorplan(s) available:\n";
			for (const auto &row : result)
			{
				sResult += "- \"" + row[1] + "\" [idx=" + row[0] + ", scale=" + row[2] + "]\n";
			}
			sResult += "\nUse the get_floorplan tool with the floorplan name to retrieve the image.";
		}
		Json::Value toolContent;
		toolContent["type"] = "text";
		toolContent["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(toolContent);
		jsonRPCRep["result"]["isError"] = false;
		return true;
	}

	bool getUsers(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		// Do NOT include Password, Secret or MFAToken columns
		auto result = m_sql.safe_query(
			"SELECT ID, Active, Username, Rights FROM Users ORDER BY Username");

		std::string sResult;
		if (result.empty())
		{
			sResult = "No users found.";
		}
		else
		{
			sResult = std::to_string(result.size()) + " user(s):\n";
			for (const auto &row : result)
			{
				std::string sActive = (atoi(row[1].c_str()) == 1) ? "Active" : "Inactive";
				std::string sRights;
				int iRights = atoi(row[3].c_str());
				switch (iRights)
				{
					case 0: sRights = "Viewer"; break;
					case 1: sRights = "User"; break;
					case 2: sRights = "Admin"; break;
					default: sRights = "Unknown (" + row[3] + ")"; break;
				}
				sResult += "- \"" + row[2] + "\" [idx=" + row[0] + ", " + sRights + ", " + sActive + "]\n";
			}
		}
		Json::Value toolContent;
		toolContent["type"] = "text";
		toolContent["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(toolContent);
		jsonRPCRep["result"]["isError"] = false;
		return true;
	}

	// =========================================================================
	// =========================================================================

	bool getSecurityStatus(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		int iSecStatus = 0;
		m_sql.GetPreferencesVar("SecStatus", iSecStatus);

		std::string sStatus;
		switch (iSecStatus)
		{
			case 0: sStatus = "Disarmed"; break;
			case 1: sStatus = "Armed Home"; break;
			case 2: sStatus = "Armed Away"; break;
			default: sStatus = "Unknown (" + std::to_string(iSecStatus) + ")"; break;
		}
		std::string sResult = "Security panel status: " + sStatus + " (code: " + std::to_string(iSecStatus) + ")";

		Json::Value toolContent;
		toolContent["type"] = "text";
		toolContent["text"] = sResult;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(toolContent);
		jsonRPCRep["result"]["isError"] = false;
		return true;
	}

	bool setSecurityStatus(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest["params"].isMember("arguments") ||
			!jsonRequest["params"]["arguments"].isMember("status") ||
			!jsonRequest["params"]["arguments"].isMember("seccode"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: setSecurityStatus: Missing required parameter 'status' or 'seccode'");
			return false;
		}
		int iNewStatus = jsonRequest["params"]["arguments"]["status"].asInt();
		std::string sProvidedCode = jsonRequest["params"]["arguments"]["seccode"].asString();

		if (iNewStatus < 0 || iNewStatus > 2)
		{
			Json::Value toolContent;
			toolContent["type"] = "text";
			toolContent["text"] = "Invalid status value " + std::to_string(iNewStatus) + ". Must be 0 (Disarmed), 1 (Armed Home), or 2 (Armed Away).";
			jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
			jsonRPCRep["result"]["content"].append(toolContent);
			jsonRPCRep["result"]["isError"] = true;
			return true;
		}

		// Read stored security code (stored as MD5 hash, 32 hex chars)
		std::string sStoredCode;
		int nValue = 1;
		m_sql.GetPreferencesVar("SecPassword", nValue, sStoredCode);

		if (sStoredCode.empty())
		{
			Json::Value toolContent;
			toolContent["type"] = "text";
			toolContent["text"] = "No security code is configured. Please set a security PIN in Domoticz settings before using this tool.";
			jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
			jsonRPCRep["result"]["content"].append(toolContent);
			jsonRPCRep["result"]["isError"] = true;
			return true;
		}

		// The stored code is an MD5 hash. Always hash the provided plaintext PIN.
		std::string sHashedCode = GenerateMD5Hash(sProvidedCode);

		if (sHashedCode != sStoredCode)
		{
			_log.Log(LOG_STATUS, "MCP: setSecurityStatus: Invalid security code provided.");
			Json::Value toolContent;
			toolContent["type"] = "text";
			toolContent["text"] = "Invalid security code. Access denied.";
			jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
			jsonRPCRep["result"]["content"].append(toolContent);
			jsonRPCRep["result"]["isError"] = true;
			return true;
		}

		m_mainworker.UpdateDomoticzSecurityStatus(iNewStatus, "MCP");

		std::string sNewStatus;
		switch (iNewStatus)
		{
			case 0: sNewStatus = "Disarmed"; break;
			case 1: sNewStatus = "Armed Home"; break;
			case 2: sNewStatus = "Armed Away"; break;
			default: sNewStatus = "Unknown"; break;
		}
		_log.Log(LOG_STATUS, "MCP: Security panel status changed to %s.", sNewStatus.c_str());

		Json::Value toolContent;
		toolContent["type"] = "text";
		toolContent["text"] = "Security panel status set to: " + sNewStatus + ".";
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(toolContent);
		jsonRPCRep["result"]["isError"] = false;
		return true;
	}

} // namespace mcp