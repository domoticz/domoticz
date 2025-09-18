/*
 * McpService.cpp
 *
 *  Created on: 4 April 2025
 *      Author: kiddigital
 * 
 * 
 * It contains routines that are part of the WebServer class, but for sourcecode management
 * reasons separated out into its own file so it is easier to maintain the MCP related functions
 * of the WebServer. The definitions of the methods here are still in 'main/Webserver.h'
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

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

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
				_log.Debug(DEBUG_WEBSERVER, "MCP: Invalid method: %s", req.method.c_str());
				rep = reply::stock_reply(reply::bad_request);
				// VScode MCP client does sends GET's
				// It does this to look for asynchronous notifications support
				// but we don't support that yet, so we return bad request
				// And the MCP spec does not support GET for requests anyway
				return;
			}

			Json::Value jsonRequest;
			std::string sParseErr;
			if (!mcp::validRPC(req.content, jsonRequest, sParseErr))
			{
				_log.Debug(DEBUG_WEBSERVER, "MCP: Invalid JSON-RPC request: %s", sParseErr.c_str());
				rep = reply::stock_reply(reply::bad_request);
				return;
			}

			_log.Debug(DEBUG_RECEIVED, "MCP: Parsed JSON Request content: %s", jsonRequest.toStyledString().c_str());

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
			std::string sReqID;
			int iReqID;
			if (jsonRequest.isMember("id"))
			{
				if (jsonRequest["id"].isInt())
				{
					iReqID = jsonRequest["id"].asInt();
					jsonRPCRep["id"] = iReqID;

				}
				else if (jsonRequest["id"].isString())
				{
					sReqID = jsonRequest["id"].asString();
					jsonRPCRep["id"] = sReqID;
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

namespace mcp
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
	}

	void McpToolsCall(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		// Check if the required parameters are present
		if (!jsonRequest.isMember("params") || !jsonRequest["params"].isMember("name"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: Missing required tool parameter 'name' in tools/{tool} request.");
			jsonRPCRep["error"]["code"] = -32602; // Invalid params
			jsonRPCRep["error"]["message"] = "Missing required parameter 'name'";
			return;
		}
		// Handle the tool call based on the name
		std::string sMethodName = jsonRequest["params"]["name"].asString();

		_log.Debug(DEBUG_WEBSERVER, "MCP: Handling tools/{%s} request.", sMethodName.c_str());

		Json::Value jsonDevices;
		m_webservers.GetJSonDevices(jsonDevices, "", "", "", "", "", "", false, false, false, 0, "", "");
		if (sMethodName == "get_switch_state")
		{
			if(!mcp::getSwitchState(jsonRequest, jsonRPCRep, jsonDevices))
			{
				jsonRPCRep["error"]["code"] = -32602; // Invalid params
				jsonRPCRep["error"]["message"] = "Error getting switch state";
				return;
			}
		}
		else if (sMethodName == "toggle_switch_state")
		{
			if(!mcp::toggleSwitchState(jsonRequest, jsonRPCRep, jsonDevices))
			{
				jsonRPCRep["error"]["code"] = -32602; // Invalid params
				jsonRPCRep["error"]["message"] = "Error toggling switch state";
				return;
			}
		}
		else if (sMethodName == "get_sensor_value")
		{
			if(!mcp::getSensorValue(jsonRequest, jsonRPCRep, jsonDevices))
			{
				jsonRPCRep["error"]["code"] = -32602; // Invalid params
				jsonRPCRep["error"]["message"] = "Error getting sensor value";
				return;
			}
		}
		else
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: Unsupported tool name: %s", sMethodName.c_str());
			jsonRPCRep["error"]["code"] = -32601; // Method not found
			jsonRPCRep["error"]["message"] = "Method not found";
			return;
		}
		// std::string sLastMod = (device.isMember("LastUpdate") ? device["LastUpdate"].asString() : "");
		// if (!sLastMod.empty())
		// 	stdreplace(sLastMod, " ", "T");
		// 	annotations["lastModified"] = sLastMod + "Z";	// To-Do: Properly convert to ISO 8601 format
		// break;
		// if (annotations.isMember("lastModified"))
		// {
		// 	tool["annotations"] = annotations;
		// }
		_log.Debug(DEBUG_WEBSERVER, "McpToolsCall: Returning result for method (%s): %s", sMethodName.c_str(), jsonRPCRep.toStyledString().c_str());
	}

	void McpResourcesList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		_log.Debug(DEBUG_WEBSERVER, "MCP: Handling resources/list request.");

		// Prepare the result for the resources/list method
		jsonRPCRep["result"]["resources"] = Json::Value(Json::arrayValue);

		Json::Value jsonDevices;
		m_webservers.GetJSonDevices(jsonDevices, "", "", "", "", "", "", false, false, false, 0, "", "");
		if (jsonDevices.isObject() && jsonDevices.isMember("result"))
		{
			for (const auto &device : jsonDevices["result"])
			{
				//_log.Debug(DEBUG_WEBSERVER, "MCP: ResourcesList: Got device: %s", device.toStyledString().c_str());
				if (device.isObject() && device.isMember("idx") && device.isMember("HardwareName") && device.isMember("ID") &&
					device.isMember("Name") && device.isMember("Type") && device.isMember("SubType") && device.isMember("Data"))
				{
					Json::Value resource;
					//resource["uri"] = "dom:///dom.local:8080/" + device["Type"].asString() + "/" + device["SubType"].asString() + "/" + device["idx"].asString();
					std::string sType = device["Type"].asString();
					stdlower(sType);
					// Replace spaces and slashes in sType to make it URL friendly
					stdreplace(sType, " ", "-");
					stdreplace(sType, "/", "-");
					resource["uri"] = sType + ":///" + device["SubType"].asString() + "/" + device["idx"].asString();
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
		_log.Debug(DEBUG_WEBSERVER, "McpResourcesList: Following resources offered:\n%s", jsonRPCRep.toStyledString().c_str());
	}

	void McpResourcesRead(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		// Check if the required parameters are present
		if (!jsonRequest.isMember("params") || !jsonRequest["params"].isMember("uri"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: Missing required resource parameter 'uri' in resources/read request.");
			jsonRPCRep["error"]["code"] = -32602; // Invalid params
			jsonRPCRep["error"]["message"] = "Missing required parameter 'uri'";
			return;
		}
		// Handle the tool call based on the name
		std::string sReadURI = jsonRequest["params"]["uri"].asString();

		_log.Debug(DEBUG_WEBSERVER, "MCP: Handling resources/read request for %s.", sReadURI.c_str());

		// Prepare the result for the resources/read method
		// TODO : Actually read the resource specified in the request
		jsonRPCRep["result"]["contents"] = Json::Value(Json::arrayValue);
		Json::Value resource;
		resource["uri"] = "dom:///dom.local:8080/Light_Switch/Switch/1";
		resource["name"] = "dumswitch";
		resource["title"] = "dumswitch (testdummy - light switch - switch)";
		resource["mimeType"] = "plain/text";
		resource["text"] = "off";
		jsonRPCRep["result"]["contents"].append(resource);
	}

	void McpPromptsList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		_log.Debug(DEBUG_WEBSERVER, "MCP: Handling prompts/list request.");

		// Prepare the result for the prompts/list method
		jsonRPCRep["result"]["prompts"] = Json::Value(Json::arrayValue);
		Json::Value prompt;
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
			std::string sText = "As the friendly butler of the house, please summarize the current status of all sensors and devices preferably grouped by room. Please also include the current state of the switches and sensors in your summary.";
			Json::Value jsonDevices;
			m_webservers.GetJSonDevices(jsonDevices, "", "", "", "", "", "", false, false, false, 0, "", "");
			sText += "Include the following devices in your summary:";
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
		else
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: prompts/get: Unsupported prompt name: %s", sPromptName.c_str());
			jsonRPCRep["error"]["code"] = -32601; // Method not found
			jsonRPCRep["error"]["message"] = "Method not found";
		}
	}

	bool getSwitchState(const Json::Value &jsonRequest, Json::Value &jsonRPCRep, const Json::Value &jsonDevices)
	{
		if (!jsonRequest["params"].isMember("arguments") || !jsonRequest["params"]["arguments"].isMember("switchname"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: getSwitchState: Missing required parameter 'switchname'");
			return false;
		}
		std::string sSwitchName = jsonRequest["params"]["arguments"]["switchname"].asString();
		std::string sSwitchState = "No switch exists with the name " + sSwitchName;
		Json::Value device;
		bool bFound = getDeviceByName(sSwitchName, device, jsonDevices);
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

	bool toggleSwitchState(const Json::Value &jsonRequest, Json::Value &jsonRPCRep, const Json::Value &jsonDevices)
	{
		if (!jsonRequest["params"].isMember("arguments") || !jsonRequest["params"]["arguments"].isMember("switchname"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: toggleSwitchState: Missing required parameter 'switchname'");
			return false;
		}
		std::string sSwitchName = jsonRequest["params"]["arguments"]["switchname"].asString();
		std::string sSwitchState = "No switch exists with the name " + sSwitchName;
		Json::Value device;
		bool bFound = getDeviceByName(sSwitchName, device, jsonDevices);
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

	bool getSensorValue(const Json::Value &jsonRequest, Json::Value &jsonRPCRep, const Json::Value &jsonDevices)
	{
		if (!jsonRequest["params"].isMember("arguments") || !jsonRequest["params"]["arguments"].isMember("sensorname"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: getSensorValue: Missing required parameter 'sensorname'");
			return false;
		}
		std::string sSensorName = jsonRequest["params"]["arguments"]["sensorname"].asString();
		std::string sSensorValue = "No sensor exists with the name " + sSensorName;
		Json::Value device;
		bool bFound = getDeviceByName(sSensorName, device, jsonDevices);
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

	bool getDeviceByName(const std::string &sDeviceName, Json::Value &device, const Json::Value &jsonDevices)
	{
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