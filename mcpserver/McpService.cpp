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

		void CWebServer::GetMcpSse(WebEmSession &session, const request &req, reply &rep)
		{
			_log.Debug(DEBUG_RECEIVED, "GetMcpSse (%d): %s (%s)", req.content_length, req.content.c_str(), req.uri.c_str());
			// Check if the request is valid
			if (req.get_req_header(&req, "Accept") != nullptr)
			{
				std::string accept = req.get_req_header(&req, "Accept");
				if (accept.find("text/event-stream") == std::string::npos)
				{
					rep = reply::stock_reply(reply::bad_request);
					return;
				}
			}
			
			// Set headers for SSE
			reply::add_header(&rep, "Content-Type", "text/event-stream");
			//reply::add_header(&rep, "Cache-Control", "no-cache");
			//reply::add_header(&rep, "Connection", "keep-alive");
		
			Json::Value jsonRPC;
			jsonRPC["jsonrpc"] = "2.0";
			jsonRPC["result"] = "Welcome to the Domoticz Model Context Protocol ServerSideEvents\n\n";
			jsonRPC["id"] = 1;

			// Set response content
			//rep.content = jsonRPC.toStyledString();
			rep.status = reply::ok;
		}

	} // namespace server
} // namespace http
