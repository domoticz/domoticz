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
#include <json/json.h>
#include "McpService.hpp"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "../main/json_helper.h"
#include "../main/WebServer.h"
#include "../main/WebServerHelper.h"
#include "../webserver/Base64.h"

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

namespace http
{
	namespace server
	{
		void CWebServer::PostMcp(WebEmSession &session, const request &req, reply &rep)
		{
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
				_log.Debug(DEBUG_WEBSERVER, "MCP: Handling notification %s (do nothing).", sReqMethod.c_str());
				rep = reply::stock_reply(reply::no_content);
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

		// To-Do: rewrite to switch/case statement using djb2hash or similar
		if (sMethodName == "get_switch_state")
		{
			if(!mcp::getSwitchState(jsonRequest, jsonRPCRep))
			{
				jsonRPCRep["error"]["code"] = JSONRPC_INVALID_PARAMETER;
				jsonRPCRep["error"]["message"] = "Error getting switch state";
				return;
			}
		}
		else if (sMethodName == "toggle_switch_state")
		{
			if(!mcp::toggleSwitchState(jsonRequest, jsonRPCRep))
			{
				jsonRPCRep["error"]["code"] = JSONRPC_INVALID_PARAMETER;
				jsonRPCRep["error"]["message"] = "Error toggling switch state";
				return;
			}
		}
		else if (sMethodName == "get_sensor_value")
		{
			if(!mcp::getSensorValue(jsonRequest, jsonRPCRep))
			{
				jsonRPCRep["error"]["code"] = JSONRPC_INVALID_PARAMETER;
				jsonRPCRep["error"]["message"] = "Error getting sensor value";
				return;
			}
		}
		else if (sMethodName == "set_setpoint_value")
		{
			if (!mcp::setThermostatSetpoint(jsonRequest, jsonRPCRep))
			{
				jsonRPCRep["error"]["code"] = JSONRPC_INVALID_PARAMETER;
				jsonRPCRep["error"]["message"] = "Error setting thermostat setpoint";
				return;
			}
		}
		else if (sMethodName == "get_logging")
		{
			if(!mcp::getLogging(jsonRequest, jsonRPCRep))
			{
				jsonRPCRep["error"]["code"] = MCP_RESOURCE_NOT_FOUND;
				jsonRPCRep["error"]["message"] = "Error getting logging";
				return;
			}
		}
		else if (sMethodName == "get_floorplan")
		{
			if(!mcp::getFloorplan(jsonRequest, jsonRPCRep))
			{
				jsonRPCRep["error"]["code"] = JSONRPC_INVALID_PARAMETER;
				jsonRPCRep["error"]["message"] = "Error getting floorplan";
				return;
			}
		}
		else
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: Unsupported tool name: %s", sMethodName.c_str());
			jsonRPCRep["error"]["code"] = JSONRPC_METHOD_NOT_FOUND;
			jsonRPCRep["error"]["message"] = "Method not found";
			return;
		}
		//_log.Debug(DEBUG_WEBSERVER, "McpToolsCall: Returning result for method (%s): %s", sMethodName.c_str(), jsonRPCRep.toStyledString().c_str());
	}

	void McpResourcesList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		_log.Debug(DEBUG_WEBSERVER, "MCP: Handling resources/list request.");

		// Prepare the result for the resources/list method
		jsonRPCRep["result"]["resources"] = Json::Value(Json::arrayValue);

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
					//resource["uri"] = "dom:///dom.local:8080/" + device["Type"].asString() + "/" + device["SubType"].asString() + "/" + device["idx"].asString();
					std::string sType = device["Type"].asString();
					stdlower(sType);
					std::string sSubType = device["SubType"].asString();
					stdlower(sSubType);
					// Replace spaces and slashes to make it URL friendly
					stdreplace(sType, " ", "_");
					stdreplace(sType, "/", "-");
					stdreplace(sSubType, " ", "_");
					resource["uri"] = sType + ":///" + sSubType + "/" + device["idx"].asString();
					resource["name"] = device["Name"].asString();
					resource["title"] = device["Name"].asString() + " (" + device["HardwareName"].asString() + " - " + device["Type"].asString() + " - " + device["SubType"].asString() + ")";
					resource["description"] = "A Sensor from the " + device["HardwareName"].asString() + " hardware of Type " + device["Type"].asString() +
												" and subtype " + device["SubType"].asString() + " called " + device["Name"].asString() +
												" with ID " + device["ID"].asString() + " and IDX " + device["idx"].asString();
					resource["mimeType"] = "plain/text";
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
		// Currently we do not have any resource templates to offer
		_log.Debug(DEBUG_WEBSERVER, "MCP: ResourcesTemplatesList: Following resource templates offered:\n%s", jsonRPCRep.toStyledString().c_str());
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
		// Handle the tool call based on the name
		std::string sReadURI = jsonRequest["params"]["uri"].asString();

		_log.Debug(DEBUG_WEBSERVER, "MCP: Handling resources/read request for %s.", sReadURI.c_str());

		int nIdx = -1;
		try
		{
			nIdx = std::stoi(sReadURI.substr(sReadURI.find_last_of("/") + 1));
		}
		catch (const std::exception &e)
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: resources/read: Invalid resource URI, cannot extract IDX: %s", e.what());
			jsonRPCRep["error"]["code"] = MCP_SERVER_ERROR;
			jsonRPCRep["error"]["message"] = "Invalid resource URI, cannot extract IDX";
			return;
		}
		std::string sResourceType = sReadURI.substr(0, sReadURI.find(":///"));
		Json::Value resource;
		if (sResourceType == "floorplan")
		{
			// Read floorplan data from database
			auto result = m_sql.safe_query("SELECT Name, Scalefactor FROM Floorplans WHERE ID=%d", nIdx);
			if (result.empty() || result.size() != 1)
			{
				_log.Debug(DEBUG_WEBSERVER, "MCP: resources/read: No floorplan found with IDX %d", nIdx);
				jsonRPCRep["error"]["code"] = MCP_RESOURCE_NOT_FOUND;
				jsonRPCRep["error"]["message"] = "No floorplan found with the specified URI";
				return;
			}
			auto &row = result[0];
			jsonRPCRep["result"]["contents"] = Json::Value(Json::arrayValue);
			resource["uri"] = sReadURI;
			resource["name"] = row[0];
			resource["title"] = "Floorplan " + row[0] + " (Scale factor: " + row[1] + ")";
			Json::Value meta;
			meta["idx"] = nIdx;
			meta["type"] = "floorplan";
			resource["_meta"] = meta;
		}
		else
		{
			auto result = m_sql.safe_query("SELECT Name, HardwareID, DeviceID, Type, SubType, nValue, sValue, LastUpdate from DeviceStatus WHERE ID=%d", nIdx);
			if (result.empty() || result.size() != 1)
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
			jsonRPCRep["result"]["contents"] = Json::Value(Json::arrayValue);
			resource["uri"] = sReadURI;
			resource["name"] = sName;
			resource["title"] = sName + " (" + std::to_string(iHardwareID) + " - " + std::to_string(iType) + " - " + std::to_string(iSubType) + ")";
			resource["mimeType"] = "plain/text";
			resource["text"] = (sValue.empty() ? std::to_string(nValue) : sValue);
			Json::Value meta;
			meta["hardwareID"] = iHardwareID;
			meta["type"] = iType;
			meta["subtype"] = iSubType;
			meta["idx"] = nIdx;
			meta["id"] = sDeviceID;
			resource["_meta"] = meta;
			Json::Value annotations;
			stdreplace(sLastUpdate, " ", "T");
			annotations["lastModified"] = sLastUpdate + "Z";	// To-Do: Properly convert to ISO 8601 format (adjust for real timezone)
			resource["annotations"] = annotations;
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
			// To-do: add all known swtches and sensors to the prompt. Filter if needed by room if specified in the arguments
			message["content"]["text"] = sText;
			jsonRPCRep["result"]["messages"].append(message);
		}
		else if (sPromptName == "systemanalysis")
		{
			std::string sRoom = ((jsonRequest["params"].isMember("arguments") && jsonRequest["params"]["arguments"].isMember("room")) ? jsonRequest["params"]["arguments"]["room"].asString() : "");
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
			sThermostatState += (m_mainworker.SetSetPoint(device["idx"].asString(), fNewSetpoint) == false ? "Error setting the setpoint." : "Setpoint set successfully.");
		}
		Json::Value tool;
		tool["type"] = "text";
		tool["text"] = sThermostatState;
		jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
		jsonRPCRep["result"]["content"].append(tool);
		jsonRPCRep["result"]["isError"] = !bFound;
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

} // namespace mcp