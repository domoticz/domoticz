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
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/json_helper.h"
#include "../httpclient/UrlEncode.h"
#include "../main/WebServer.h"
#include "../webserver/Base64.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

namespace http
{
	namespace server
	{

		void CWebServer::PostMcp(WebEmSession &session, const request &req, reply &rep)
		{
			_log.Debug(DEBUG_RECEIVED, "PostMcp (%d): %s (%s)", req.content_length, req.content.c_str(), req.uri.c_str());
			// Check if the request is valid
			std::string sProtocolRequestHeader;
			if (req.get_req_header(&req, "Accept") != nullptr)
			{
				std::string accept = req.get_req_header(&req, "Accept");
				if (accept.find("text/event-stream") == std::string::npos && accept.find("application/json") == std::string::npos)
				{
					_log.Debug(DEBUG_WEBSERVER, "PostMcp: Invalid Accept header: %s", accept.c_str());
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
					_log.Debug(DEBUG_WEBSERVER, "PostMcp: MCP-PROTOCOL-VERSION not supported: %s", sProtocolRequestHeader.c_str());
					rep = reply::stock_reply(reply::bad_request);
					return;
				}
			}
			// Check if the request is a POST request
			if (req.method != "POST")
			{
				_log.Debug(DEBUG_WEBSERVER, "PostMcp: Invalid method: %s", req.method.c_str());
				rep = reply::stock_reply(reply::bad_request);
				return;
			}

			// Check if the request is a proper JSON-RPC request
			if (req.content_length <= 0 || req.content.empty())
			{
				_log.Debug(DEBUG_WEBSERVER, "PostMcp: Invalid content length or empty content");
				rep = reply::stock_reply(reply::bad_request);
				return;
			}
			Json::Value jsonRequest;
			std::string sParseErr;
			if (!ParseJSon(req.content, jsonRequest, &sParseErr))
			{
				_log.Debug(DEBUG_WEBSERVER, "PostMcp: Failed to parse JSON content: %s", sParseErr.c_str());
				rep = reply::stock_reply(reply::bad_request);
				return;
			}
			_log.Debug(DEBUG_RECEIVED, "PostMcp: Parsed JSON Request content: %s", jsonRequest.toStyledString().c_str());
			// Check if the parsed JSON is valid JSON-RPC 2.0 format
			if (!jsonRequest.isObject() || !jsonRequest.isMember("jsonrpc") || !jsonRequest.isMember("method"))	//  || !jsonRequest.isMember("params") || !jsonRequest.isMember("id")
			{
				_log.Debug(DEBUG_WEBSERVER, "PostMcp: Invalid JSON-RPC request format");
				rep = reply::stock_reply(reply::bad_request);
				return;
			}
			// Check if the JSON-RPC version is supported
			if (jsonRequest["jsonrpc"].asString() != "2.0")
			{
				_log.Debug(DEBUG_WEBSERVER, "PostMcp: Unsupported JSON-RPC version: %s", jsonRequest["jsonrpc"].asString().c_str());
				rep = reply::stock_reply(reply::bad_request);
				return;
			}
			// Check if the method is supported and handle it
			std::string sReqMethod = jsonRequest["method"].asString();
			_log.Debug(DEBUG_WEBSERVER, "PostMcp: Request method: %s", sReqMethod.c_str());

			if (sReqMethod.find("notifications/") != std::string::npos)
			{
				// Handle notifications, notifications do not require a response
				_log.Debug(DEBUG_WEBSERVER, "PostMcp: Notification: %s", sReqMethod.c_str());
				rep = reply::stock_reply(reply::no_content);
				return;
			}
			// Check if the request has an ID
			std::string sReqID = (jsonRequest.isMember("id") ? jsonRequest["id"].asString() : "");

			Json::Value jsonRPCRep;
			jsonRPCRep["jsonrpc"] = "2.0";
			jsonRPCRep["id"] = sReqID;

			if (sReqMethod == "ping")
			{
				_log.Debug(DEBUG_WEBSERVER, "PostMcp: Handling ping request");
				jsonRPCRep["result"] = Json::Value(Json::objectValue);
			}
			else if (sReqMethod == "initialize")
			{
				McpInitialize(jsonRequest, jsonRPCRep);
			}
			else if (sReqMethod == "tools/list")
			{
				McpToolsList(jsonRequest, jsonRPCRep);
			}
			else if (sReqMethod == "tools/call")
			{
				McpToolsCall(jsonRequest, jsonRPCRep);
			}
			else if (sReqMethod == "resources/list")
			{
				McpResourcesList(jsonRequest, jsonRPCRep);
			}
			else if (sReqMethod == "resources/read")
			{
				McpResourcesRead(jsonRequest, jsonRPCRep);
			}
			else if (sReqMethod == "prompts/list")
			{
				McpPromptsList(jsonRequest, jsonRPCRep);
			}
			else if (sReqMethod == "prompts/get")
			{
				McpPromptsGet(jsonRequest, jsonRPCRep);
			}
			else
			{
				_log.Debug(DEBUG_WEBSERVER, "PostMcp: Unsupported method: %s", sReqMethod.c_str());
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

		void CWebServer::McpInitialize(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
		{
/*
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "protocolVersion": "2024-11-05",
    "capabilities": {
      "logging": {},
      "prompts": {
        "listChanged": true
      },
      "resources": {
        "subscribe": true,
        "listChanged": true
      },
      "tools": {
        "listChanged": true
      }
    },
    "serverInfo": {
      "name": "ExampleServer",
      "title": "Example Server Display Name",
      "version": "1.0.0"
    },
    "instructions": "Optional instructions for the client"
  }
}
*/
			_log.Debug(DEBUG_WEBSERVER, "HandleMcpInitialize: Handling MCP initialization request");

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

			if (jsonRequest.isMember("params") && jsonRequest["params"].isMember("instructions"))
			{
				jsonRPCRep["result"]["instructions"] = jsonRequest["params"]["instructions"];
			}
		}

		void CWebServer::McpToolsList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
		{
/*

  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "tools": [
      {
        "name": "get_weather",
        "title": "Weather Information Provider",
        "description": "Get current weather information for a location",
        "inputSchema": {
          "type": "object",
          "properties": {
            "location": {
              "type": "string",
              "description": "City name or zip code"
            }
          },
          "required": ["location"]
        }
      }
    ],
    "nextCursor": "next-page-cursor"
  }
}

{
  "type": "resource_link",
  "uri": "file:///project/src/main.rs",
  "name": "main.rs",
  "description": "Primary application entry point",
  "mimeType": "text/x-rust",
  "annotations": {
    "audience": ["assistant"],
    "priority": 0.9
  }
}
*/
			// Prepare the result for the tools/list method
			jsonRPCRep["result"]["tools"] = Json::Value(Json::arrayValue);
			Json::Value tool;
			tool["name"] = "get_switch_state";
			tool["title"] = "See the state of a switch in the system";
			tool["description"] = "Get the current state of a given switch in the system";
			tool["inputSchema"]["type"] = "object";
			tool["inputSchema"]["properties"]["switchname"]["type"] = "string";
			tool["inputSchema"]["properties"]["switchname"]["description"] = "Name of the switch to query";
			tool["inputSchema"]["required"].append("switchname");
			jsonRPCRep["result"]["tools"].append(tool);
		}

		void CWebServer::McpToolsCall(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
		{
			if (!jsonRequest.isMember("params") || !jsonRequest["params"].isMember("name"))
			{
				_log.Debug(DEBUG_WEBSERVER, "McpToolsCall: Missing required parameter 'name'");
				jsonRPCRep["error"]["code"] = -32602; // Invalid params
				jsonRPCRep["error"]["message"] = "Missing required parameter 'name'";
				return;
			}
			std::string sMethodName = jsonRequest["params"]["name"].asString();
			if (sMethodName != "get_switch_state")
			{
				_log.Debug(DEBUG_WEBSERVER, "McpToolsCall: Unsupported tool name: %s", sMethodName.c_str());
				jsonRPCRep["error"]["code"] = -32601; // Method not found
				jsonRPCRep["error"]["message"] = "Method not found";
				return;
			}
			if (!jsonRequest["params"].isMember("arguments") || !jsonRequest["params"]["arguments"].isMember("switchname"))
			{
				_log.Debug(DEBUG_WEBSERVER, "McpToolsCall: Missing required parameter 'switchname'");
				jsonRPCRep["error"]["code"] = -32602; // Invalid params
				jsonRPCRep["error"]["message"] = "Missing required parameter 'switchname'";
				return;
			}
			std::string sSwitchName = jsonRequest["params"]["arguments"]["switchname"].asString();
			std::string sSwitchState = "No switch exists with the name " + sSwitchName;
			// Here you would typically query your system/database for the actual switch state using sSwitchName.
			// For demonstration, let's assume the switch is "Off" (replace with actual lookup logic as needed).
			Json::Value jsonDevices;
			GetJSonDevices(jsonDevices, "", "", "", "", "", "", false, false, false, 0, "", "");
			_log.Debug(DEBUG_WEBSERVER, "McpToolsCall: Looking for switch with name: %s in (%s)", sSwitchName.c_str(), jsonDevices.toStyledString().c_str());
			//GetLightStatus(0, 0, STYPE_OnOff, 0, "", sSwitchState, 0, false, 0, false);

/*
{
  "jsonrpc": "2.0",
  "id": 2,
  "result": {
    "content": [
      {
        "type": "text",
        "text": "Current weather in New York:\nTemperature: 72°F\nConditions: Partly cloudy"
      }
    ],
    "isError": false
  }
}*/
			// Prepare the result for the tools/list method
			jsonRPCRep["result"]["content"] = Json::Value(Json::arrayValue);
			Json::Value tool;
			tool["type"] = "text";
			tool["text"] = sSwitchState;
			jsonRPCRep["result"]["content"].append(tool);
			jsonRPCRep["result"]["isError"] = false;
		}

		void CWebServer::McpResourcesList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
		{
/*
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "resources": [
      {
        "uri": "file:///project/src/main.rs",
        "name": "main.rs",
        "title": "Rust Software Application Main File",
        "description": "Primary application entry point",
        "mimeType": "text/x-rust"
      }
    ],
    "nextCursor": "next-page-cursor"
  }
}
idx	hardwareID	uniqueID	ena	name		type			subtype			value
2	testmb		0000044C	1	Memory Usage	General			Percentage		57.96%
3	testmb		000000DC	1	Process Usage	General			Custom Sensor	43.87 MB
5	testmb		0000044D	1	CPU_Usage		General			Percentage		0.13%
4	testmb		0000044E	1	HDD /			General			Percentage		26.31%
6	testmb		0000044F	1	HDD /boot		General			Percentage		11.83%
1	testdummy	00014051	1	dumswitch		Light/Switch	Switch			Off

                {
                        "AddjMulti" : 1.0,
                        "AddjMulti2" : 1.0,
                        "AddjValue" : 0.0,
                        "AddjValue2" : 0.0,
                        "BatteryLevel" : 255,
                        "CustomImage" : 0,
                        "Data" : "Off",
                        "Description" : "",
                        "DimmerType" : "none",
                        "Favorite" : 1,
                        "HardwareDisabled" : false,
                        "HardwareID" : 2,
                        "HardwareName" : "testdummy",
                        "HardwareType" : "Dummy (Does nothing, use for virtual switches only)",
                        "HardwareTypeVal" : 15,
                        "HaveDimmer" : true,
                        "HaveGroupCmd" : true,
                        "HaveTimeout" : false,
                        "ID" : "00014051",
                        "Image" : "Light",
                        "IsSubDevice" : false,
                        "LastUpdate" : "2025-04-27 15:45:22",
                        "Level" : 0,
                        "LevelInt" : 0,
                        "MaxDimLevel" : 100,
                        "Name" : "dumswitch",
                        "Notifications" : "false",
                        "PlanID" : "0",
                        "PlanIDs" :
                        [
                                0
                        ],
                        "Protected" : false,
                        "ShowNotifications" : true,
                        "SignalLevel" : "-",
                        "Status" : "Off",
                        "StrParam1" : "",
                        "StrParam2" : "",
                        "SubType" : "Switch",
                        "SwitchType" : "On/Off",
                        "SwitchTypeVal" : 0,
                        "Timers" : "false",
                        "Type" : "Light/Switch",
                        "TypeImg" : "lightbulb",
                        "Unit" : 1,
                        "Used" : 1,
                        "UsedByCamera" : false,
                        "XOffset" : "0",
                        "YOffset" : "0",
                        "idx" : "1"
                },
*/
			// Prepare the result for the resources/list method
			jsonRPCRep["result"]["resources"] = Json::Value(Json::arrayValue);

			Json::Value jsonDevices;
			GetJSonDevices(jsonDevices, "", "", "", "", "", "", false, false, false, 0, "", "");
			if (jsonDevices.isObject() && jsonDevices.isMember("result"))
			{
				for (const auto &device : jsonDevices["result"])
				{
					_log.Debug(DEBUG_WEBSERVER, "McpResourcesList: Got device: %s", device.toStyledString().c_str());
					if (device.isObject() && device.isMember("idx") && device.isMember("HardwareName") && device.isMember("ID") &&
						device.isMember("Name") && device.isMember("Type") && device.isMember("SubType") && device.isMember("Data"))
					{
						Json::Value resource;
						resource["uri"] = "dom:///dom.local:8080/" + device["Type"].asString() + "/" + device["SubType"].asString() + "/" + device["idx"].asString();
						resource["name"] = device["Name"].asString();
						resource["title"] = device["Name"].asString() + " (" + device["HardwareName"].asString() + " - " + device["Type"].asString() + " - " + device["SubType"].asString() + ")";
						resource["description"] = "A Sensor from the " + device["HardwareName"].asString() + " hardware of Type " + device["Type"].asString() +
												  " and subtype " + device["SubType"].asString() + " called " + device["Name"].asString() +
												  " with ID " + device["ID"].asString() + " and IDX " + device["idx"].asString();
						resource["mimeType"] = "plain/text";
						jsonRPCRep["result"]["resources"].append(resource);
					}
				}
			}
			_log.Debug(DEBUG_WEBSERVER, "McpResourcesList: Following resources offered:\n%s", jsonRPCRep.toStyledString().c_str());
		}

		void CWebServer::McpResourcesRead(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
		{
/*
{
  "jsonrpc": "2.0",
  "id": 2,
  "result": {
    "contents": [
      {
        "uri": "file:///project/src/main.rs",
        "name": "main.rs",
        "title": "Rust Software Application Main File",
        "mimeType": "text/x-rust",
        "text": "fn main() {\n    println!(\"Hello world!\");\n}"
      }
    ]
  }
}*/
			// Prepare the result for the resources/read method
			jsonRPCRep["result"]["contents"] = Json::Value(Json::arrayValue);
			Json::Value resource;
			resource["uri"] = "dom:///dom.local:8080/Light_Switch/Switch/1";
			resource["name"] = "dumswitch";
			resource["title"] = "dumswitch (testdummy - light switch - switch)";
			resource["mimeType"] = "plain/text";
			resource["text"] = "off";
			jsonRPCRep["result"]["contents"].append(resource);
		}

		void CWebServer::McpPromptsList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
		{
/*
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "prompts": [
      {
        "name": "code_review",
        "title": "Request Code Review",
        "description": "Asks the LLM to analyze code quality and suggest improvements",
        "arguments": [
          {
            "name": "code",
            "description": "The code to review",
            "required": true
          }
        ]
      }
    ],
    "nextCursor": "next-page-cursor"
  }
}*/
			// Prepare the result for the prompts/list method
			jsonRPCRep["result"]["prompts"] = Json::Value(Json::arrayValue);
			Json::Value prompt;
			prompt["name"] = "housesummary";
			prompt["title"] = "Get a status overview";
			prompt["description"] = "Asks the LLM to summarize the current status of all sensors and devices (either grouped by hardware, type or room)";
			prompt["arguments"] = Json::Value(Json::arrayValue);
			Json::Value arg;
			arg["name"] = "hardware";
			arg["description"] = "The hardware to summarize";
			arg["required"] = false;
			prompt["arguments"].append(arg);
			arg["name"] = "type";
			arg["description"] = "The type to summarize";
			arg["required"] = false;
			prompt["arguments"].append(arg);
			arg["name"] = "room";
			arg["description"] = "The room to summarize";
			arg["required"] = false;
			prompt["arguments"].append(arg);
			jsonRPCRep["result"]["prompts"].append(prompt);
		}

		void CWebServer::McpPromptsGet(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
		{
/*
{
  "jsonrpc": "2.0",
  "id": 2,
  "result": {
    "description": "Code review prompt",
    "messages": [
      {
        "role": "user",
        "content": {
          "type": "text",
          "text": "Please review this Python code:\ndef hello():\n    print('world')"
        }
      }
    ]
  }
}*/
			// Prepare the result for the prompts/get method
			jsonRPCRep["result"]["description"] = "Asks the LLM to summarize the current status of all sensors and devices (either grouped by hardware, type or room)";
			jsonRPCRep["result"]["messages"] = Json::Value(Json::arrayValue);
			Json::Value message;
			message["role"] = "user";
			message["content"] = Json::Value(Json::objectValue);
			message["content"]["type"] = "text";
			message["content"]["text"] = "As the friendly butler of the house, please summarize the current status of all sensors and devices preferably grouped by room. Please also include the current state of the switches and sensors in your summary.";
			jsonRPCRep["result"]["messages"].append(message);
		}

	} // namespace server
} // namespace http
