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

	} // namespace server
} // namespace http
