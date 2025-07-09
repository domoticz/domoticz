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

			// Check if the request is a POST request
			if (req.method != "POST")
			{
				_log.Debug(DEBUG_WEBSERVER, "PostMcp: Invalid method: %s", req.method.c_str());
				rep = reply::stock_reply(reply::bad_request);
				return;
			}

			// Check if the request if a proper JSON-RPC request
			if (req.content_length <= 0 || req.content.empty())
			{
				_log.Debug(DEBUG_WEBSERVER, "PostMcp: Invalid content length or empty content");
				rep = reply::stock_reply(reply::bad_request);
				return;
			}
			Json::Value jsonRequest;
			Json::CharReaderBuilder readerBuilder;	// Use ParseJSON instead of Json::Reader
			std::string errs;
			std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
			if (!reader->parse(req.content.c_str(), req.content.c_str() + req.content_length, &jsonRequest, &errs))
			{
				_log.Debug(DEBUG_WEBSERVER, "PostMcp: Invalid JSON content: %s", errs.c_str());
				rep = reply::stock_reply(reply::bad_request);
				return;
			}

			// Set headers for SSE
			reply::add_header(&rep, "Content-Type", "application/json");	// "text/event-stream" is also an option if we want to support SSE
			//reply::add_header(&rep, "Cache-Control", "no-cache");
			//reply::add_header(&rep, "Connection", "keep-alive");
		
			Json::Value jsonRPC;
			jsonRPC["jsonrpc"] = "2.0";
			jsonRPC["result"] = "Welcome to the Domoticz Model Context Protocol over StreamableHTTP\n\n";
			jsonRPC["id"] = 1;
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
			// Set response content
			//rep.content = jsonRPC.toStyledString();
			rep.status = reply::ok;
		}

	} // namespace server
} // namespace http
