/*
 * McpService.cpp
 * The MCP Service of domoticz implements the Model Context Protocol (currently version 2025-06-18)
 * so domoticz can be used as a agent in a LLM (Large Language Model) AI Agent context.
 *
 *  Created on: 4 April 2025
 *      Author: kiddigital
 *
 *  Modified: 26 March 2026
 *      Author: GizMoCuz
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
#include <cassert>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
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
#include "../main/RFXtrx.h"
#include "../main/RFXNames.h"
#include "../hardware/hardwaretypes.h"
#include "../main/WebServerHandleGraphInternals.h"
#include "../mcpserver/McpSseSession.h"
#include "../mcpserver/McpSessionRegistry.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>


// MCP protocol version this server implements.
// ISO 8601 date strings (YYYY-MM-DD) compare correctly with < / > because
// lexicographic order matches chronological order for this format.
static constexpr const char *MCP_PROTOCOL_VERSION = "2025-06-18";
static constexpr int MCP_LIST_PAGE_SIZE = 50;

static const std::unordered_map<std::string, http::server::_eUserRights> s_toolMinRights = {
    { "set_switch_state",       http::server::URIGHTS_SWITCHER },
    { "toggle_switch_state",    http::server::URIGHTS_SWITCHER },
    { "set_dimmer_level",       http::server::URIGHTS_SWITCHER },
    { "control_blinds",         http::server::URIGHTS_SWITCHER },
    { "set_color_brightness",   http::server::URIGHTS_SWITCHER },
    { "set_color_temperature",  http::server::URIGHTS_SWITCHER },
    { "set_setpoint_value",     http::server::URIGHTS_SWITCHER },
    { "switch_scene",           http::server::URIGHTS_SWITCHER },
    { "add_log_message",        http::server::URIGHTS_SWITCHER },
    { "send_notification",      http::server::URIGHTS_SWITCHER },
    { "rename_device",          http::server::URIGHTS_ADMIN },
    { "delete_device",          http::server::URIGHTS_ADMIN },
    { "create_sensor",          http::server::URIGHTS_ADMIN },
    { "create_virtual_sensor",  http::server::URIGHTS_ADMIN },
    { "create_device",          http::server::URIGHTS_ADMIN },
    { "update_device_value",    http::server::URIGHTS_ADMIN },
    { "add_user_variable",      http::server::URIGHTS_ADMIN },
    { "update_user_variable",   http::server::URIGHTS_ADMIN },
    { "delete_user_variable",   http::server::URIGHTS_ADMIN },
    { "create_event",           http::server::URIGHTS_ADMIN },
    { "update_event",           http::server::URIGHTS_ADMIN },
    { "delete_event",           http::server::URIGHTS_ADMIN },
    { "set_security_status",    http::server::URIGHTS_ADMIN },
};

extern http::server::CWebServerHelper m_webservers;
extern CLogger _log;

static std::string McpGetSessionIdFromRequest(const http::server::request& req)
{
	const char* hdr = req.get_req_header(&req, "Mcp-Session-Id");
	return hdr ? std::string(hdr) : std::string{};
}

static bool McpIsValidSessionId(const std::string& sid)
{
	if (sid.empty() || sid.size() > 64)
		return false;
	return sid.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos;
}

static std::string McpGetSessionIdFromQuery(const http::server::request& req)
{
	// Parse ?sessionId=xxx or &sessionId=xxx from the URI query string
	const std::string& uri = req.uri;
	auto qpos = uri.find('?');
	if (qpos == std::string::npos)
		return {};
	std::string query = uri.substr(qpos + 1);
	const std::string key = "sessionId=";
	auto kpos = query.find(key);
	if (kpos == std::string::npos)
		return {};
	std::string value = query.substr(kpos + key.size());
	auto amp = value.find('&');
	if (amp != std::string::npos)
		value = value.substr(0, amp);
	if (!McpIsValidSessionId(value))
	{
		_log.Debug(DEBUG_WEBSERVER, "MCP: Rejecting malformed sessionId in query: %s", uri.c_str());
		return {};
	}
	return value;
}

static bool McpIsValidSubscriptionUri(const std::string& uri)
{
	if (uri == "domoticz://devices" || uri == "domoticz://scenes")
		return true;
	if (uri.size() > 19 && uri.substr(0, 19) == "domoticz://devices/")
	{
		const auto digits = uri.substr(19);
		return !digits.empty() && digits.find_first_not_of("0123456789") == std::string::npos;
	}
	if (uri.size() > 18 && uri.substr(0, 18) == "domoticz://scenes/")
	{
		const auto digits = uri.substr(18);
		return !digits.empty() && digits.find_first_not_of("0123456789") == std::string::npos;
	}
	return false;
}

static int McpLevelToPriority(const std::string& levelStr)
{
	if (levelStr == "debug")     return 0;
	if (levelStr == "info")      return 1;
	if (levelStr == "notice")    return 2;
	if (levelStr == "warning")   return 2;
	if (levelStr == "error")     return 3;
	if (levelStr == "critical")  return 3;
	if (levelStr == "alert")     return 3;
	if (levelStr == "emergency") return 3;
	return 2;
}
extern CNotificationHelper m_notifications;
extern std::string szAppVersion;
extern std::string szAppHash;
extern std::string szAppDate;
extern time_t m_StartTime;
extern bool g_bLlmMCPSupport;

namespace mcp
{
	struct McpProgressCtx
	{
		std::string sid;
		std::string progressToken;
	};
	extern thread_local McpProgressCtx tl_progressCtx;
}

namespace http
{
	namespace server
	{
		void CWebServer::OptionsMcp(WebEmSession& session, const request& req, reply& rep)
		{
			rep.status = http::server::reply::ok;
			http::server::reply::add_header(&rep, "Content-Length", "0");
			http::server::reply::add_header(&rep, "Access-Control-Allow-Origin", "*");
			http::server::reply::add_header(&rep, "Access-Control-Allow-Methods", "GET, HEAD, POST, DELETE, OPTIONS");
			http::server::reply::add_header(&rep, "Access-Control-Allow-Headers", "Content-Type, Accept, Authorization, Mcp-Session-Id, Mcp-Protocol-Version, Last-Event-ID");
			http::server::reply::add_header(&rep, "Access-Control-Max-Age", "86400");
		}

		static void ProcessSingleRequest(
			const request& req,
			const Json::Value& jsonRequest,
			Json::Value& jsonRPCRep,
			std::string& newSessionId,
			WebEmSession& session,
			bool isLegacySse,
			const std::string& querySid);

		void CWebServer::PostMcp(WebEmSession &session, const request &req, reply &rep)
		{
			if (g_bLlmMCPSupport == false)
			{
				_log.Log(LOG_ERROR, "MCP: MCP access requested (IP: %s), but service disabled with -nomcp !", session.remote_host.c_str());
				rep = reply::stock_reply(reply::service_unavailable);
				return;
			}
			if (session.rights == http::server::URIGHTS_NONE)
			{
				_log.Log(LOG_STATUS, "MCP: Unauthenticated access attempt from %s", session.remote_host.c_str());
				Json::Value errRep;
				errRep["jsonrpc"] = "2.0";
				errRep["id"] = Json::Value(Json::nullValue);
				errRep["error"]["code"] = mcp::MCP_PERMISSION_DENIED;
				errRep["error"]["message"] = "Unauthorized";
				rep.content = JSonToRawString(errRep);
				rep.status = reply::unauthorized;
				reply::add_header(&rep, "Content-Type", "application/json");
				// Wildcard CORS intentional: browser must be able to read the 401 error body.
				// The MCP endpoint does not use cookies/credentials for CORS purposes.
				reply::add_header(&rep, "Access-Control-Allow-Origin", "*");
				return;
			}
			if (req.method == "GET")
			{
				HandleMcpGet(session, req, rep);
				return;
			}
			if (req.method == "HEAD")
			{
				rep.status = reply::ok;
				reply::add_header(&rep, "Content-Type", "text/event-stream");
				reply::add_header(&rep, "Access-Control-Allow-Origin", "*");
				return;
			}
			if (req.method == "DELETE")
			{
				HandleMcpDelete(session, req, rep);
				return;
			}
			if (req.method != "POST")
			{
				_log.Debug(DEBUG_WEBSERVER, "MCP: Invalid method: %s", req.method.c_str());
				rep = reply::stock_reply(reply::method_not_allowed);
				reply::add_header(&rep, "Allow", "GET, HEAD, POST, DELETE, OPTIONS");
				reply::add_header(&rep, "Access-Control-Allow-Origin", "*");
				return;
			}

			_log.Debug(DEBUG_RECEIVED, "MCP: Post (%d): %s (%s)", req.content_length, req.content.c_str(), req.uri.c_str());

			// Empty-body POSTs are used as connection health checks by some MCP clients
			if (req.content.empty())
			{
				rep = reply::stock_reply(reply::accepted);
				reply::add_header(&rep, "Access-Control-Allow-Origin", "*");
				return;
			}

			// POST-specific header validation
			std::string sProtocolRequestHeader;
			if (req.get_req_header(&req, "Accept") != nullptr)
			{
				std::string accept = req.get_req_header(&req, "Accept");
				if (accept.find("text/event-stream") == std::string::npos &&
				    accept.find("application/json") == std::string::npos &&
				    accept.find("*/*") == std::string::npos)
				{
					_log.Debug(DEBUG_WEBSERVER, "MCP: Invalid Accept header: %s", accept.c_str());
					rep = reply::stock_reply(reply::bad_request);
					return;
				}
			}
			if (req.get_req_header(&req, "mcp-protocol-version") != nullptr)
			{
				sProtocolRequestHeader = req.get_req_header(&req, "mcp-protocol-version");
				if (sProtocolRequestHeader > MCP_PROTOCOL_VERSION)
				{
					_log.Debug(DEBUG_WEBSERVER, "MCP: MCP-PROTOCOL-VERSION newer than supported: %s (continuing)", sProtocolRequestHeader.c_str());
				}
				else if (sProtocolRequestHeader != MCP_PROTOCOL_VERSION)
				{
					_log.Debug(DEBUG_WEBSERVER, "MCP: MCP-PROTOCOL-VERSION not supported: %s", sProtocolRequestHeader.c_str());
					rep = reply::stock_reply(reply::bad_request);
					return;
				}
			}

			Json::Value jsonRoot;
			std::string sParseErr;
			if (!ParseJSon(req.content, jsonRoot, &sParseErr))
			{
				_log.Debug(DEBUG_WEBSERVER, "MCP: Invalid JSON-RPC request: %s", sParseErr.c_str());
				Json::Value errRep;
				errRep["jsonrpc"] = "2.0";
				errRep["id"] = Json::Value(Json::nullValue);
				errRep["error"]["code"] = mcp::JSONRPC_PARSE_ERROR;
				errRep["error"]["message"] = "Parse error: " + sParseErr;
				rep.content = JSonToRawString(errRep);
				rep.status = reply::ok;
				reply::add_header(&rep, "Content-Type", "application/json");
				return;
			}

			//_log.Debug(DEBUG_RECEIVED, "MCP: Parsed JSON Request content: %s", jsonRoot.toStyledString().c_str());

			// Detect legacy SSE mode: client uses ?sessionId= query param instead of Mcp-Session-Id header
			std::string querySid = McpGetSessionIdFromQuery(req);
			bool isLegacySse = !querySid.empty();

			auto reply202 = [&rep]() {
				rep = reply::stock_reply(reply::accepted);
				reply::add_header(&rep, "Access-Control-Allow-Origin", "*");
				reply::add_header(&rep, "Access-Control-Allow-Methods", "GET, HEAD, POST, DELETE, OPTIONS");
				reply::add_header(&rep, "Access-Control-Allow-Headers", "Content-Type, Accept, Authorization, Mcp-Session-Id, Mcp-Protocol-Version, Last-Event-ID");
			};

			if (jsonRoot.isArray())
			{
				if (jsonRoot.empty())
				{
					Json::Value errRep;
					errRep["jsonrpc"] = "2.0";
					errRep["id"] = Json::Value(Json::nullValue);
					errRep["error"]["code"] = mcp::JSONRPC_INVALID_REQUEST;
					errRep["error"]["message"] = "Empty batch";
					Json::Value errArray(Json::arrayValue);
					errArray.append(errRep);
					rep.content = JSonToRawString(errArray);
					rep.status = reply::bad_request;
					reply::add_header(&rep, "Content-Type", "application/json");
					reply::add_header(&rep, "Access-Control-Allow-Origin", "*");
					return;
				}

				Json::Value responses(Json::arrayValue);
				std::string newSessionId;

				for (Json::ArrayIndex i = 0; i < jsonRoot.size(); ++i)
				{
					const Json::Value& item = jsonRoot[i];
					if (!item.isObject())
					{
						Json::Value errRep;
						errRep["jsonrpc"] = "2.0";
						errRep["id"] = Json::Value(Json::nullValue);
						errRep["error"]["code"] = mcp::JSONRPC_INVALID_REQUEST;
						errRep["error"]["message"] = "Request item must be an object";
						if (isLegacySse)
							CMcpSessionRegistry::Instance().SendToSession(querySid, JSonToRawString(errRep));
						else
							responses.append(errRep);
						continue;
					}

					Json::Value itemRep;
					std::string itemNewSid;
					ProcessSingleRequest(req, item, itemRep, itemNewSid, session, isLegacySse, querySid);

					if (itemRep.isNull())
						continue;

					if (isLegacySse)
					{
						CMcpSessionRegistry::Instance().SendToSession(querySid, JSonToRawString(itemRep));
					}
					else
					{
						responses.append(itemRep);
						if (!itemNewSid.empty())
							newSessionId = itemNewSid;
					}
				}

				if (isLegacySse)
				{
					reply202();
					return;
				}

				if (responses.empty())
				{
					// All items were notifications
					reply202();
					return;
				}

				rep.content = JSonToRawString(responses);
				rep.status = reply::ok;
				reply::add_header(&rep, "Content-Type", "application/json");
				reply::add_header(&rep, "Access-Control-Allow-Origin", "*");
				reply::add_header(&rep, "Access-Control-Expose-Headers", "Mcp-Session-Id, Mcp-Protocol-Version");
				if (!newSessionId.empty())
					reply::add_header(&rep, "Mcp-Session-Id", newSessionId.c_str());
				return;
			}

			if (!jsonRoot.isObject())
			{
				Json::Value errRep;
				errRep["jsonrpc"] = "2.0";
				errRep["id"] = Json::Value(Json::nullValue);
				errRep["error"]["code"] = mcp::JSONRPC_INVALID_REQUEST;
				errRep["error"]["message"] = "Invalid Request";
				rep.content = JSonToRawString(errRep);
				rep.status = reply::bad_request;
				reply::add_header(&rep, "Content-Type", "application/json");
				reply::add_header(&rep, "Access-Control-Allow-Origin", "*");
				return;
			}

			// Single request object
			Json::Value jsonRPCRep;
			std::string newSessionId;
			ProcessSingleRequest(req, jsonRoot, jsonRPCRep, newSessionId, session, isLegacySse, querySid);

			if (jsonRPCRep.isNull())
			{
				// Notification: no response
				reply202();
				return;
			}

			if (isLegacySse)
			{
				// Legacy SSE: send result via the SSE stream and return 202 Accepted
				CMcpSessionRegistry::Instance().SendToSession(querySid, JSonToRawString(jsonRPCRep));
				reply202();
			}
			else
			{
				// Streamable HTTP: return result directly in the POST response body
				rep.content = JSonToRawString(jsonRPCRep);
				rep.status = reply::ok;
				reply::add_header(&rep, "Content-Type", "application/json");
				reply::add_header(&rep, "Access-Control-Allow-Origin", "*");
				reply::add_header(&rep, "Access-Control-Expose-Headers", "Mcp-Session-Id, Mcp-Protocol-Version");
				if (!newSessionId.empty())
					reply::add_header(&rep, "Mcp-Session-Id", newSessionId.c_str());
			}
			//reply::add_header(&rep, "Cache-Control", "no-cache");
			//reply::add_header(&rep, "Connection", "keep-alive");
		}

		static void ProcessSingleRequest(
			const request& req,
			const Json::Value& jsonRequest,
			Json::Value& jsonRPCRep,
			std::string& newSessionId,
			WebEmSession& session,
			bool isLegacySse,
			const std::string& querySid)
		{
			jsonRPCRep = Json::Value::null;
			newSessionId.clear();

			Json::Value requestId = Json::Value(Json::nullValue);
			if (jsonRequest.isMember("id"))
				requestId = jsonRequest["id"];

			if (!jsonRequest.isObject() || !jsonRequest.isMember("method") || !jsonRequest["method"].isString()
			    || !jsonRequest.isMember("jsonrpc") || jsonRequest["jsonrpc"].asString() != "2.0")
			{
				jsonRPCRep["jsonrpc"] = "2.0";
				jsonRPCRep["id"] = requestId;
				jsonRPCRep["error"]["code"] = mcp::JSONRPC_INVALID_REQUEST;
				jsonRPCRep["error"]["message"] = "Invalid Request";
				return;
			}

			std::string sReqMethod = jsonRequest["method"].asString();
			_log.Debug(DEBUG_WEBSERVER, "MCP: Request method: %s", sReqMethod.c_str());

			if (!jsonRequest.isMember("id"))
			{
				_log.Debug(DEBUG_WEBSERVER, "MCP: Handling notification %s (do nothing).", sReqMethod.c_str());
				// jsonRPCRep remains null — caller skips this entry
				return;
			}

			if (sReqMethod != "initialize")
			{
				std::string reqSid = isLegacySse ? querySid : McpGetSessionIdFromRequest(req);
				if (reqSid.empty())
				{
					jsonRPCRep["jsonrpc"] = "2.0";
					jsonRPCRep["id"] = requestId;
					jsonRPCRep["error"]["code"] = mcp::JSONRPC_INVALID_REQUEST;
					jsonRPCRep["error"]["message"] = "Missing session";
					return;
				}
				bool found = CMcpSessionRegistry::Instance().WithSession(
					reqSid, [](CMcpSession& s) { s.lastActivity = time(nullptr); });
				if (!found)
				{
					jsonRPCRep["jsonrpc"] = "2.0";
					jsonRPCRep["id"] = requestId;
					jsonRPCRep["error"]["code"] = mcp::MCP_RESOURCE_NOT_FOUND;
					jsonRPCRep["error"]["message"] = "Session not found";
					return;
				}
			}

			jsonRPCRep["jsonrpc"] = "2.0";

			if (requestId.isInt())
			{
				jsonRPCRep["id"] = requestId.asInt();
			}
			else if (requestId.isString())
			{
				jsonRPCRep["id"] = requestId.asString();
			}
			else if (!requestId.isNull())
			{
				_log.Debug(DEBUG_WEBSERVER, "MCP: Invalid ID type in request (must be number or string).");
				jsonRPCRep["id"] = Json::Value(Json::nullValue);
				jsonRPCRep["error"]["code"] = mcp::JSONRPC_INVALID_REQUEST;
				jsonRPCRep["error"]["message"] = "Invalid id type";
				return;
			}
			else
			{
				jsonRPCRep["id"] = Json::Value(Json::nullValue);
			}

			if (sReqMethod == "ping")
			{
				_log.Debug(DEBUG_WEBSERVER, "MCP: Handling ping request (return empty result).");
				jsonRPCRep["result"] = Json::Value(Json::objectValue);
			}
			else if (sReqMethod == "initialize")
			{
				mcp::McpInitialize(jsonRequest, jsonRPCRep);
				if (isLegacySse)
				{
					// Legacy SSE: session already created when GET established the SSE stream.
					// Signal caller to send via SSE by leaving jsonRPCRep populated; caller handles transport.
					// But for legacy SSE initialize, we need to send immediately and suppress the normal send path.
					// We repurpose newSessionId as a sentinel to indicate this was already handled.
					CMcpSessionRegistry::Instance().SendToSession(querySid, JSonToRawString(jsonRPCRep));
					jsonRPCRep = Json::Value::null;
					return;
				}
				newSessionId = CMcpSessionRegistry::Instance().CreateSession(session);
			}
			else if (sReqMethod == "tools/list")
			{
				mcp::McpToolsList(jsonRequest, jsonRPCRep);
			}
			else if (sReqMethod == "tools/call")
			{
				if (!jsonRequest.isMember("params") || !jsonRequest["params"].isMember("name"))
				{
					jsonRPCRep["error"]["code"] = mcp::JSONRPC_INVALID_PARAMETER;
					jsonRPCRep["error"]["message"] = "Missing required parameter: params.name";
				}
				else
				{
					std::string toolName = jsonRequest["params"]["name"].asString();
					auto rightsIt = s_toolMinRights.find(toolName);
					std::string sid = isLegacySse ? querySid : McpGetSessionIdFromRequest(req);
					if (rightsIt != s_toolMinRights.end())
					{
						http::server::_eUserRights sessionRights = http::server::URIGHTS_NONE;
						CMcpSessionRegistry::Instance().WithSession(sid, [&sessionRights](CMcpSession& s) {
							sessionRights = s.rights;
						});
						if (sessionRights < rightsIt->second)
						{
							jsonRPCRep["error"]["code"] = mcp::MCP_PERMISSION_DENIED;
							jsonRPCRep["error"]["message"] = "Insufficient rights for tool: " + toolName;
						}
						else
						{
							std::string progressToken;
							if (jsonRequest["params"].isMember("_meta") && jsonRequest["params"]["_meta"].isMember("progressToken"))
							{
								const Json::Value& tok = jsonRequest["params"]["_meta"]["progressToken"];
								if (tok.isString())
									progressToken = tok.asString();
								else if (tok.isIntegral())
									progressToken = std::to_string(tok.asInt64());
							}
							mcp::tl_progressCtx = { sid, progressToken };
							mcp::McpToolsCall(jsonRequest, jsonRPCRep, session);
							mcp::tl_progressCtx = {};
						}
					}
					else
					{
						std::string progressToken;
						if (jsonRequest["params"].isMember("_meta") && jsonRequest["params"]["_meta"].isMember("progressToken"))
						{
							const Json::Value& tok = jsonRequest["params"]["_meta"]["progressToken"];
							if (tok.isString())
								progressToken = tok.asString();
							else if (tok.isIntegral())
								progressToken = std::to_string(tok.asInt64());
						}
						mcp::tl_progressCtx = { sid, progressToken };
						mcp::McpToolsCall(jsonRequest, jsonRPCRep, session);
						mcp::tl_progressCtx = {};
					}
				}
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
			else if (sReqMethod == "completion/complete")
			{
				mcp::McpCompletionComplete(jsonRequest, jsonRPCRep);
			}
			else if (sReqMethod == "logging/setLevel")
			{
				std::string sid = isLegacySse ? querySid : McpGetSessionIdFromRequest(req);
				http::server::_eUserRights sessionRights = http::server::URIGHTS_NONE;
				CMcpSessionRegistry::Instance().WithSession(sid, [&sessionRights](CMcpSession& s) {
					sessionRights = s.rights;
				});
				if (sessionRights < http::server::URIGHTS_ADMIN)
				{
					jsonRPCRep["error"]["code"] = mcp::MCP_PERMISSION_DENIED;
					jsonRPCRep["error"]["message"] = "logging/setLevel requires admin rights";
				}
				else if (!jsonRequest.isMember("params") || !jsonRequest["params"].isMember("level"))
				{
					jsonRPCRep["error"]["code"] = mcp::JSONRPC_INVALID_PARAMETER;
					jsonRPCRep["error"]["message"] = "Missing required parameter: params.level";
				}
				else
				{
					std::string levelStr = jsonRequest["params"]["level"].asString();
					int priority = McpLevelToPriority(levelStr);
					if (!sid.empty())
					{
						CMcpSessionRegistry::Instance().WithSession(
							sid, [priority](CMcpSession& s) { s.minLogLevel = priority; });
					}
					jsonRPCRep["result"] = Json::Value(Json::objectValue);
				}
			}
			else if (sReqMethod == "resources/subscribe")
			{
				if (!jsonRequest.isMember("params") || !jsonRequest["params"].isMember("uri"))
				{
					jsonRPCRep["error"]["code"] = mcp::JSONRPC_INVALID_PARAMETER;
					jsonRPCRep["error"]["message"] = "Missing required parameter: params.uri";
				}
				else
				{
					std::string uri = jsonRequest["params"]["uri"].asString();
					if (!McpIsValidSubscriptionUri(uri))
					{
						jsonRPCRep["error"]["code"] = mcp::MCP_RESOURCE_NOT_FOUND;
						jsonRPCRep["error"]["message"] = "Unknown resource URI: " + uri;
					}
					else
					{
						std::string sid = isLegacySse ? querySid : McpGetSessionIdFromRequest(req);
						if (!sid.empty())
						{
							CMcpSessionRegistry::Instance().WithSession(
								sid, [&uri](CMcpSession& s) { s.subscribedUris.insert(uri); });
						}
						jsonRPCRep["result"] = Json::Value(Json::objectValue);
					}
				}
			}
			else if (sReqMethod == "resources/unsubscribe")
			{
				if (!jsonRequest.isMember("params") || !jsonRequest["params"].isMember("uri"))
				{
					jsonRPCRep["error"]["code"] = mcp::JSONRPC_INVALID_PARAMETER;
					jsonRPCRep["error"]["message"] = "Missing required parameter: params.uri";
				}
				else
				{
					std::string uri = jsonRequest["params"]["uri"].asString();
					std::string sid = isLegacySse ? querySid : McpGetSessionIdFromRequest(req);
					if (!sid.empty())
					{
						CMcpSessionRegistry::Instance().WithSession(
							sid, [&uri](CMcpSession& s) { s.subscribedUris.erase(uri); });
					}
					jsonRPCRep["result"] = Json::Value(Json::objectValue);
				}
			}
			else
			{
				_log.Debug(DEBUG_WEBSERVER, "MCP: Unsupported method: %s", sReqMethod.c_str());
				jsonRPCRep["error"]["code"] = mcp::JSONRPC_METHOD_NOT_FOUND;
				jsonRPCRep["error"]["message"] = "Method not found: " + sReqMethod;
			}
		}

		void CWebServer::HandleMcpGet(WebEmSession& session, const request& req, reply& rep)
		{
			// Validate Accept header: must include text/event-stream
			const char* acceptHdr = req.get_req_header(&req, "Accept");
			if (acceptHdr == nullptr ||
			    std::string(acceptHdr).find("text/event-stream") == std::string::npos)
			{
				rep = reply::stock_reply(reply::bad_request);
				return;
			}

			// Look up session ID from header
			const char* sessionIdHdr = req.get_req_header(&req, "Mcp-Session-Id");
			std::string mcpSessionId;

			if (sessionIdHdr != nullptr)
			{
				// Streamable HTTP: session already exists, look it up
				bool found = CMcpSessionRegistry::Instance().WithSession(
					sessionIdHdr, [](CMcpSession& s) { s.lastActivity = time(nullptr); });
				if (!found)
				{
					rep = reply::stock_reply(reply::not_found);
					return;
				}
				mcpSessionId = sessionIdHdr;

				// Build sse_context: "sessionId" or "sessionId:lastEventId"
				const char* lastEventId = req.get_req_header(&req, "Last-Event-ID");
				std::string sseContext = mcpSessionId;
				if (lastEventId != nullptr)
					sseContext = mcpSessionId + ":" + std::string(lastEventId);

				rep.status = reply::sse_stream;
				rep.sse_session = session;
				rep.sse_context = sseContext;
				reply::add_header(&rep, "Access-Control-Allow-Origin", "*");

				_log.Debug(DEBUG_WEBSERVER, "MCP: Opening SSE stream for client %s (session: %s)",
				           session.remote_host.c_str(), mcpSessionId.c_str());
			}
			else
			{
				// Legacy SSE transport: no session header — create new session and send endpoint event
				std::string newSid = CMcpSessionRegistry::Instance().CreateSession(session);

				// Build endpoint URL from the actual local socket address (not the Host header,
				// which is client-supplied and could be spoofed).
				std::string endpointUrl = "http://" + session.local_host + ":" + session.local_port + "/mcp?sessionId=" + newSid;

				// context format: "legacy|<sessionId>|<endpointUrl>"
				// Use '|' as separator — ':' cannot be used because the URL contains colons.
				std::string sseContext = "legacy|" + newSid + "|" + endpointUrl;

				rep.status = reply::sse_stream;
				rep.sse_session = session;
				rep.sse_context = sseContext;
				reply::add_header(&rep, "Access-Control-Allow-Origin", "*");

				_log.Debug(DEBUG_WEBSERVER, "MCP: Opening legacy SSE stream for client %s (new session: %s)",
				           session.remote_host.c_str(), newSid.c_str());
			}
		}

		void CWebServer::HandleMcpDelete(WebEmSession& /*session*/, const request& req, reply& rep)
		{
			const char* sessionIdHdr = req.get_req_header(&req, "Mcp-Session-Id");
			if (sessionIdHdr != nullptr)
			{
				std::string sid(sessionIdHdr);
				auto sessionPtr = CMcpSessionRegistry::Instance().GetSession(sid);
				CMcpSessionRegistry::Instance().RemoveSession(sid);
				if (sessionPtr && sessionPtr->sseConnected && sessionPtr->sseWriter)
				{
					Json::Value notif;
					notif["jsonrpc"] = "2.0";
					notif["method"] = "notifications/message";
					notif["params"]["level"] = "notice";
					notif["params"]["logger"] = "mcp";
					notif["params"]["data"]["message"] = "Session terminated by client";
					sessionPtr->SendSseEvent(JSonToRawString(notif));
				}
			}
			rep = reply::stock_reply(reply::ok);
			reply::add_header(&rep, "Access-Control-Allow-Origin", "*");
		}

	} // namespace server
} // namespace http

namespace mcp		// Model Context Protocol
{
	static const char* const kVarTypeNames[] = { "Integer", "Float", "String", "Date", "Time" };
	static const int kVarTypeCount = 5;

	thread_local McpProgressCtx tl_progressCtx;

	static void SendProgress(const std::string& sid, const std::string& token, int progress, int total, const std::string& message)
	{
		if (token.empty() || sid.empty()) return;
		Json::Value notif;
		notif["jsonrpc"] = "2.0";
		notif["method"] = "notifications/progress";
		notif["params"]["progressToken"] = token;
		notif["params"]["progress"] = progress;
		notif["params"]["total"] = total;
		notif["params"]["message"] = message;
		CMcpSessionRegistry::Instance().SendToSession(sid, JSonToRawString(notif));
	}

	static std::string buildSettingsText()
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
		return sResult;
	}

	void McpInitialize(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		_log.Debug(DEBUG_WEBSERVER, "MCP: Handling initialize request.");

		// Prepare the result for the initialize method
		jsonRPCRep["result"]["protocolVersion"] = MCP_PROTOCOL_VERSION;
		jsonRPCRep["result"]["capabilities"]["logging"] = Json::Value(Json::objectValue);
		jsonRPCRep["result"]["capabilities"]["completion"] = Json::Value(Json::objectValue);
		jsonRPCRep["result"]["capabilities"]["prompts"] = Json::Value(Json::objectValue);
		//jsonRPCRep["result"]["capabilities"]["prompts"]["listChanged"] = true;
		jsonRPCRep["result"]["capabilities"]["resources"] = Json::Value(Json::objectValue);
		jsonRPCRep["result"]["capabilities"]["resources"]["subscribe"] = true;
		jsonRPCRep["result"]["capabilities"]["resources"]["listChanged"] = true;
		jsonRPCRep["result"]["capabilities"]["tools"] = Json::Value(Json::objectValue);
		jsonRPCRep["result"]["capabilities"]["tools"]["listChanged"] = true;

		jsonRPCRep["result"]["serverInfo"]["name"] = "DomoticzMcp";
		jsonRPCRep["result"]["serverInfo"]["title"] = "Domoticz MCP Server";
		jsonRPCRep["result"]["serverInfo"]["version"] = "0.1.0";
		jsonRPCRep["result"]["serverInfo"]["description"] = "Domoticz is a home automation system that lets you monitor, configure and control various devices from different hardware in your home. Devices like switches (for example light switches or smart plugs) can be used to control (other) devices and devices like sensors (for example temperature sensors or contact sensors) can provide information about their state.";

		//jsonRPCRep["result"]["instructions"] = "Any additional instructions for the client can be provided here";
	}

	Json::Value buildToolSchema(const ToolDef &def)
	{
		Json::Value tool;
		tool["name"] = def.name;
		tool["title"] = def.title;
		tool["description"] = def.description;
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"] = Json::Value(Json::objectValue);
		Json::Value required(Json::arrayValue);
		for (const auto &p : def.params)
		{
			tool["inputSchema"]["properties"][p.name]["type"] = p.type;
			tool["inputSchema"]["properties"][p.name]["description"] = p.description;
			if (!p.enumValues.empty())
			{
				tool["inputSchema"]["properties"][p.name]["enum"] = Json::Value(Json::arrayValue);
				for (const auto &e : p.enumValues)
					tool["inputSchema"]["properties"][p.name]["enum"].append(e);
			}
			if (p.required)
				required.append(p.name);
		}
		tool["inputSchema"]["required"] = required;
		return tool;
	}

	static const std::vector<mcp::ToolDef> kToolDefinitions = {
		{
			"get_switch_state",
			"See the state of a switch in the system",
			"Get the current state of a given switch in the system",
			{
				{ "switchname", "string", "Name of the switch to query", false, {} },
				{ "idx", "integer", "Device IDX (use either this or switchname)", false, {} },
			}
		},
		{
			"toggle_switch_state",
			"Toggle the state of a switch in the system",
			"Toggle the state of a given switch in the system",
			{
				{ "switchname", "string", "Name of the switch to toggle", false, {} },
				{ "idx", "integer", "Device IDX (use either this or switchname)", false, {} },
			}
		},
		{
			"get_sensor_value",
			"Get the value of a sensor in the system",
			"Retrieve the current value of a specified sensor in the system",
			{
				{ "sensorname", "string", "Name of the sensor to query", false, {} },
				{ "idx", "integer", "Device IDX (use either this or sensorname)", false, {} },
			}
		},
		{
			"set_setpoint_value",
			"Set the target setpoint of a thermostat in the system",
			"Set the target setpoint of a given thermostat in the system",
			{
				{ "thermostatname", "string", "Name of the thermostat to set", false, {} },
				{ "idx", "integer", "Device IDX (use either this or thermostatname)", false, {} },
				{ "setpoint", "number", "Temperature setpoint as an number", true, {} },
			}
		},
		{
			"get_logging",
			"Get the logging information",
			"Retrieve the current logging information",
			{
				{ "logdate", "number", "The (Unixtimestamp) date and time from which to retrieve the logs (optional, default is 0, which means all logs)", false, {} },
			}
		},
		{
			"get_floorplan",
			"Get the floorplan",
			"Retrieve the specific floorplan within the system",
			{
				{ "floorplan", "string", "The name of the floorplan to retrieve", false, {} },
				{ "floorplan_id", "integer", "Floorplan IDX (use either this or floorplan name)", false, {} },
			}
		},
		{
			"get_status",
			"Get the system status of Domoticz",
			"Retrieve the current system status including version, uptime, sunrise/sunset times and device/hardware counts. Use this tool to check if the Domoticz instance is running and healthy.",
			{}
		},
		{
			"search_devices",
			"Search for devices",
			"Search for devices whose name, type or subtype contains the given query string (case-insensitive substring match). Use this tool to discover exact device names before using other tools like get_switch_state or toggle_switch_state that require exact names.",
			{
				{ "query", "string", "Search string to match against device name, type or subtype (case-insensitive substring match)", true, {} },
				{ "filter", "string", "Optional device type filter: light, temp, weather, utility (matches Domoticz device categories)", false, {} },
			}
		},
		{
			"get_all_devices",
			"List all devices",
			"Return a list of devices in the system, optionally filtered by category and/or hardware. Use this to discover all available devices.",
			{
				{ "filter", "string", "Optional category filter: light, temp, weather, utility (leave empty for all)", false, {} },
				{ "hw_idx", "integer", "Optional hardware IDX to return only devices belonging to that hardware adapter", false, {} },
				{ "include_unused", "boolean", "Set to true to also include unused/disabled devices (default: false, only used devices are returned)", false, {} },
			}
		},
		{
			"get_device",
			"Get device details",
			"Return full details for a single device by name or idx. At least one of name or idx must be provided.",
			{
				{ "name", "string", "Device name to look up", false, {} },
				{ "idx", "integer", "Device IDX (numeric identifier) to look up", false, {} },
			}
		},
		{
			"rename_device",
			"Rename a device",
			"Rename any device (switch, sensor, virtual, etc.) by its current name to a new name.",
			{
				{ "name", "string", "Current name of the device", false, {} },
				{ "idx", "integer", "Device IDX (use either this or name)", false, {} },
				{ "new_name", "string", "New name for the device", true, {} },
			}
		},
		{
			"delete_device",
			"Delete (hide) a device",
			"Hide any device (switch, sensor, virtual, etc.) by setting its Used flag to 0. The device is not permanently deleted; it can be re-enabled. Use with caution.",
			{
				{ "name", "string", "Name of the device to hide/delete", false, {} },
				{ "idx", "integer", "Device IDX (use either this or name)", false, {} },
			}
		},
		{
			"delete_event",
			"Delete an event script",
			"Permanently delete an event script by name or ID. This cannot be undone.",
			{
				{ "event_name", "string", "Name of the event script to delete (use either this or event_id)", false, {} },
				{ "event_id", "integer", "ID (idx) of the event script to delete (use either this or event_name)", false, {} },
			}
		},
		{
			"create_sensor",
			"Create a virtual sensor",
			"Create a new virtual sensor attached to a virtual hardware instance. sensortype is a sensor type name (e.g. 'Wind', 'Temperature', 'Switch') or its numeric mapped value. Read domoticz://sensor-types for all available types. Aliases: create_virtual_sensor, create_device.",
			{
				{ "hw_idx", "integer", "IDX of the virtual/dummy hardware to attach the sensor to", true, {} },
				{ "sensorname", "string", "Name for the new virtual sensor", true, {} },
				{ "sensortype", "string", "Sensor type name or numeric mapped value. Read domoticz://sensor-types for the full list.", true, {
					"Air Quality", "Alert", "Ampere (1 Phase)", "Ampere (3 Phase)", "Barometer",
					"Counter", "Counter Incremental", "Custom Sensor", "Distance",
					"Electric (Instant+Counter)", "Gas", "Humidity", "Leaf Wetness", "Lux",
					"Managed Counter", "P1 Smart Meter (Electric)", "Percentage", "Pressure (Bar)",
					"Rain", "RGB Switch", "RGBW Switch", "Scale", "Selector Switch",
					"Soil Moisture", "Solar Radiation", "Sound Level", "Switch",
					"Temp+Baro", "Temp+Hum", "Temp+Hum+Baro", "Temperature", "Text",
					"Thermostat (Temp/Baro/Setpoint)", "Thermostat (Temp/Hum/Baro/Setpoint)",
					"Thermostat (Temp/Hum/Setpoint)", "Thermostat (Temp/Setpoint)",
					"Thermostat Setpoint", "Usage (Electric)", "UV",
					"Visibility", "Voltage", "Waterflow", "Wind", "Wind+Temp+Chill"
				} },
			}
		},
		{
			"update_device_value",
			"Update a device value",
			"Directly update the nValue and/or sValue of a device (useful for virtual sensors).",
			{
				{ "name", "string", "Name of the device to update", false, {} },
				{ "idx", "integer", "Device IDX (use either this or name)", false, {} },
				{ "nvalue", "integer", "Numeric value to set", true, {} },
				{ "svalue", "string", "String value to set (optional)", false, {} },
			}
		},
		{
			"get_sensor_history",
			"Get daily-aggregated history for a sensor or event log for a switch",
			"Retrieve DAILY-AGGREGATED (calendar) data for sensors going back weeks or months, "
			"or the on/off/dim event log for switches and scenes. "
			"Use this for multi-day trends or long-term history. "
			"For intraday data (today, last few hours, last 24 h) use get_sensor_short_log instead. "
			"For sensors: specify 'days' or 'start_date'+'end_date'. "
			"For switches/scenes: specify 'days'/'start_date'/'end_date' for date range, "
			"or 'count' for last N events.",
			{
				{ "name", "string", "Name of the device or scene", false, {} },
				{ "idx", "integer", "Device IDX (use either this or name)", false, {} },
				{ "days", "integer", "Number of days of history to retrieve (1-366, default 7 for sensors). Ignored if start_date/end_date provided.", false, {} },
				{ "start_date", "string", "Start date in YYYY-MM-DD format (use with end_date for custom range)", false, {} },
				{ "end_date", "string", "End date in YYYY-MM-DD format (use with start_date for custom range)", false, {} },
				{ "count", "integer", "For switches/scenes: return last N log entries (1-500, default 50). When specified, date params are ignored.", false, {} },
			}
		},
		{
			"get_sensor_short_log",
			"Get recent/live sensor readings (last hours or N samples)",
			"Retrieve recent high-resolution measurements at ~5-minute intervals from the short-log tables. "
			"Use this for: today's data, last 24 hours, last N readings, live/recent sensor values. "
			"Kept for a configurable number of days (default 1 day). "
			"For multi-day or long-term history use get_sensor_history instead. "
			"Not applicable to switches/scenes.",
			{
				{ "name",  "string",  "Name of the device", false, {} },
				{ "idx",   "integer", "Device IDX (use either this or name)", false, {} },
				{ "hours", "integer", "Time window in hours (1-168, default 24). Ignored if count is provided.", false, {} },
				{ "count", "integer", "Return last N readings (1-1000). When specified, time window is ignored.", false, {} },
			}
		},
		{
			"get_user_variables",
			"List all user variables",
			"List all user-defined variables (name, type, value, last update).",
			{}
		},
		{
			"add_user_variable",
			"Create a user variable",
			"Create a new user variable. Types: 0=Integer, 1=Float, 2=String, 3=Date (DD/MM/YYYY), 4=Time (HH:MM).",
			{
				{ "name", "string", "Variable name (must be unique)", true, {} },
				{ "vtype", "integer", "Variable type: 0=Integer, 1=Float, 2=String, 3=Date, 4=Time", true, {} },
				{ "value", "string", "Initial value for the variable", true, {} },
			}
		},
		{
			"update_user_variable",
			"Update a user variable",
			"Update the value (and optionally type) of an existing user variable.",
			{
				{ "name", "string", "Variable name to update", false, {} },
				{ "variable_id", "integer", "Variable IDX (use either this or name)", false, {} },
				{ "value", "string", "New value", true, {} },
				{ "vtype", "integer", "New type (optional): 0=Integer, 1=Float, 2=String, 3=Date, 4=Time", false, {} },
			}
		},
		{
			"delete_user_variable",
			"Delete a user variable",
			"Delete a user variable by name.",
			{
				{ "name", "string", "Name of the variable to delete", false, {} },
				{ "variable_id", "integer", "Variable IDX (use either this or name)", false, {} },
			}
		},
		{
			"add_log_message",
			"Write a log message",
			"Write a message to the Domoticz system log. Messages are prefixed with 'MCP: ' to identify AI-originated entries.",
			{
				{ "message", "string", "The message text to log", true, {} },
				{ "level", "string", "Log level: normal (default), status, error", false, {} },
			}
		},
		{
			"send_notification",
			"Send a push notification",
			"Send a push notification via all configured Domoticz notification services (e.g. Telegram, Pushover, email). WARNING: This sends to real devices. Do not call repeatedly or for testing.",
			{
				{ "subject", "string", "Notification title/subject", true, {} },
				{ "body", "string", "Notification body text", true, {} },
				{ "priority", "integer", "Priority: -2 (very low) to 2 (emergency), default 0 (normal)", false, {} },
			}
		},
		{
			"get_events",
			"List event scripts",
			"List all event scripts (dzVents, Lua, Blockly, Python) with their name, interpreter, and enabled status.",
			{}
		},
		{
			"get_event",
			"Get event script source",
			"Get the full source code of a specific event script by name or ID.",
			{
				{ "event_name", "string", "Name of the event script (use either this or event_id)", false, {} },
				{ "event_id", "integer", "ID (idx) of the event script (use either this or event_name)", false, {} },
			}
		},
		{
			"create_event",
			"Create an event script",
			"Create a new event script. interpreter must be one of: Lua, dzVents, Python, Blockly.",
			{
				{ "name", "string", "Script name (must be unique)", true, {} },
				{ "interpreter", "string", "Script interpreter: Lua, dzVents, Python, or Blockly", true, {} },
				{ "code", "string", "Script source code", true, {} },
				{ "enabled", "boolean", "Whether to enable the script immediately (default true)", false, {} },
			}
		},
		{
			"update_event",
			"Update an event script",
			"Update an existing event script's code, enabled state, or name. Identify the script by event_name or event_id. At least one of code/enabled/new_name must be provided.",
			{
				{ "event_name", "string", "Current name of the event script (use either this or event_id)", false, {} },
				{ "event_id", "integer", "ID (idx) of the event script (use either this or event_name)", false, {} },
				{ "code", "string", "New script source code (optional)", false, {} },
				{ "enabled", "boolean", "Enable or disable the script (optional)", false, {} },
				{ "new_name", "string", "Rename the script to this name (optional)", false, {} },
			}
		},
		{
			"set_switch_state",
			"Set a switch On or Off",
			"Explicitly turn a switch On or Off by name (without toggling).",
			{
				{ "switchname", "string", "Name of the switch", false, {} },
				{ "idx", "integer", "Device IDX (use either this or switchname)", false, {} },
				{ "state", "string", "Desired state: On or Off", true, { "On", "Off" } },
			}
		},
		{
			"set_dimmer_level",
			"Set a dimmer level",
			"Set a dimmable light to a specific brightness level (0-100).",
			{
				{ "switchname", "string", "Name of the dimmable device", false, {} },
				{ "idx", "integer", "Device IDX (use either this or switchname)", false, {} },
				{ "level", "integer", "Brightness level 0-100", true, {} },
			}
		},
		{
			"control_blinds",
			"Control a blind or shutter",
			"Send an Open, Close, or Stop command to a blind or shutter device.",
			{
				{ "switchname", "string", "Name of the blind/shutter device", false, {} },
				{ "idx", "integer", "Device IDX (use either this or switchname)", false, {} },
				{ "command", "string", "Command to send: Open, Close, or Stop", true, { "Open", "Close", "Stop" } },
			}
		},
		{
			"set_color_brightness",
			"Set color and brightness on an RGB light",
			"Set the hue and brightness of an RGB or color light device.",
			{
				{ "switchname", "string", "Name of the color light device", false, {} },
				{ "idx", "integer", "Device IDX (use either this or switchname)", false, {} },
				{ "hue", "integer", "Hue angle 0-360", true, {} },
				{ "brightness", "integer", "Brightness level 0-100", true, {} },
				{ "iswhite", "boolean", "If true, use white mode instead of RGB color (default false)", false, {} },
			}
		},
		{
			"set_color_temperature",
			"Set color temperature on a tunable-white light",
			"Set the color temperature of a tunable-white light in Kelvin (e.g. 2700 for warm white, 6500 for cool white).",
			{
				{ "switchname", "string", "Name of the tunable-white light device", false, {} },
				{ "idx", "integer", "Device IDX (use either this or switchname)", false, {} },
				{ "kelvin", "integer", "Color temperature in Kelvin (2700=warm white, 6500=cool white)", true, {} },
			}
		},
		{
			"get_scenes",
			"List all scenes and groups",
			"Return a list of all scenes and groups configured in Domoticz.",
			{}
		},
		{
			"switch_scene",
			"Activate or deactivate a scene",
			"Activate (On) or deactivate (Off) a Domoticz scene or group by name.",
			{
				{ "scenename", "string", "Name of the scene or group", false, {} },
				{ "scene_id", "integer", "Scene IDX (use either this or scenename)", false, {} },
				{ "command", "string", "Command: On or Off", true, { "On", "Off" } },
			}
		},
		{
			"get_rooms",
			"List all rooms",
			"Return a list of all rooms (plans) configured in Domoticz.",
			{}
		},
		{
			"get_room_devices",
			"Get devices in a room",
			"Return all devices assigned to a specific room (plan) by room name.",
			{
				{ "roomname", "string", "Name of the room (plan)", false, {} },
				{ "room_id", "integer", "Room IDX (use either this or roomname)", false, {} },
			}
		},
		{
			"get_scene_devices",
			"Get devices in a scene",
			"Return all devices that belong to a specific scene or group.",
			{
				{ "scenename", "string", "Name of the scene or group", false, {} },
				{ "scene_id", "integer", "Scene IDX (use either this or scenename)", false, {} },
			}
		},
		{
			"get_hardware",
			"List all hardware",
			"Return a list of all configured hardware adapters in Domoticz.",
			{}
		},
		{
			"get_settings",
			"Get system settings",
			"Return key Domoticz system settings and preferences.",
			{}
		},
		{
			"get_sun_times",
			"Get sunrise and sunset times",
			"Return today's sunrise, sunset, dawn, dusk, solar noon and related solar data.",
			{}
		},
		{
			"get_cameras",
			"List all cameras",
			"Return a list of all configured cameras (name, address, port, enabled status). Passwords are never returned.",
			{}
		},
		{
			"get_floorplans",
			"List all floorplans",
			"Return a list of available floorplan names and IDs (no image data). Use get_floorplan to retrieve a specific floorplan image.",
			{}
		},
		{
			"get_users",
			"List all users",
			"Return a list of all system users with their username, rights and active status. Passwords are never returned.",
			{}
		},
		{
			"get_security_status",
			"Get security panel status",
			"Return the current Domoticz security panel status (Disarmed, Armed Home, or Armed Away).",
			{}
		},
		{
			"set_security_status",
			"Set security panel status",
			"Set the Domoticz security panel status. Requires the security PIN code. status: 0=Disarmed, 1=Armed Home, 2=Armed Away.",
			{
				{ "status", "integer", "New security status: 0=Disarmed, 1=Armed Home, 2=Armed Away", true, {} },
				{ "seccode", "string", "The security panel PIN code (plaintext)", true, {} },
			}
		},
	};

	void McpToolsList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		_log.Debug(DEBUG_WEBSERVER, "MCP: Handling tools/list request.");

		int offset = 0;
		if (jsonRequest.isMember("params") && jsonRequest["params"].isMember("cursor"))
		{
			std::string cursorStr = jsonRequest["params"]["cursor"].asString();
			if (!cursorStr.empty())
			{
				std::string decoded = base64_decode(cursorStr);
				Json::Value cursorObj;
				if (ParseJSon(decoded, cursorObj) && cursorObj.isMember("offset") && cursorObj["offset"].isInt())
					offset = cursorObj["offset"].asInt();
			}
		}
		if (offset < 0)
			offset = 0;

		Json::Value allTools(Json::arrayValue);
		for (const auto &def : kToolDefinitions)
			allTools.append(mcp::buildToolSchema(def));

		int total = (int)allTools.size();
		Json::Value page(Json::arrayValue);
		for (int i = offset; i < std::min(offset + MCP_LIST_PAGE_SIZE, total); i++)
			page.append(allTools[i]);
		jsonRPCRep["result"]["tools"] = page;

		if (offset + MCP_LIST_PAGE_SIZE < total)
		{
			Json::Value nextCursorObj;
			nextCursorObj["offset"] = offset + MCP_LIST_PAGE_SIZE;
			jsonRPCRep["result"]["nextCursor"] = base64_encode(JSonToRawString(nextCursorObj));
		}
	}

	void McpToolsCall(const Json::Value &jsonRequest, Json::Value &jsonRPCRep, const http::server::WebEmSession &session)
	{
		// Check if the required parameters are present
		if (!jsonRequest.isMember("params") || !jsonRequest["params"].isMember("name"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: Missing required tool parameter 'name' in tools/{tool} request.");
			jsonRPCRep["error"]["code"] = mcp::JSONRPC_INVALID_PARAMETER;
			jsonRPCRep["error"]["message"] = "Missing required parameter 'name'";
			return;
		}
		// Handle the tool call based on the name
		std::string sMethodName = jsonRequest["params"]["name"].asString();

		_log.Debug(DEBUG_WEBSERVER, "MCP: Handling tools/{%s} request.", sMethodName.c_str());

		static const std::unordered_set<std::string> kSwitcherTools = {
			"toggle_switch_state", "set_switch_state", "set_dimmer_level",
			"control_blinds", "set_color_brightness", "set_color_temperature",
			"set_setpoint_value", "switch_scene", "send_notification",
			"add_log_message"
		};
		static const std::unordered_set<std::string> kAdminTools = {
			"create_sensor", "create_virtual_sensor", "create_device",
			"rename_device", "delete_device",
			"add_user_variable", "update_user_variable", "delete_user_variable",
			"create_event", "update_event", "delete_event",
			"update_device_value", "set_security_status"
		};

		if (kAdminTools.count(sMethodName) && session.rights < http::server::URIGHTS_ADMIN)
		{
			jsonRPCRep["error"]["code"] = mcp::MCP_PERMISSION_DENIED;
			jsonRPCRep["error"]["message"] = "Admin rights required for: " + sMethodName;
			return;
		}
		if (kSwitcherTools.count(sMethodName) && session.rights < http::server::URIGHTS_SWITCHER)
		{
			jsonRPCRep["error"]["code"] = mcp::MCP_PERMISSION_DENIED;
			jsonRPCRep["error"]["message"] = "Switcher rights required for: " + sMethodName;
			return;
		}

		if (sMethodName == "set_security_status")
		{
			_log.Log(LOG_STATUS, "MCP: Security panel status change requested by user '%s' from %s",
				session.username.c_str(), session.remote_host.c_str());
		}

			// Tool function contract:
		// - Always return true (tool functions always produce a result content)
		// - Use mcp::setToolResult(rep, message, true) to signal logical errors (device not found, etc.)
		// - Return false only when required JSON parameters are missing (pre-flight validation)
		// - Never set jsonRPCRep["error"] directly — that is for JSON-RPC protocol errors only
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
			{ "create_sensor",          createVirtualSensor },
			{ "create_virtual_sensor",  createVirtualSensor },
			{ "create_device",          createVirtualSensor },
			{ "update_device_value",    updateDeviceValue },
			{ "get_sensor_history",     getSensorHistory },
			{ "get_sensor_short_log",   getSensorShortLog },
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
			{ "delete_event",           deleteEvent },
		};

#ifdef _DEBUG
		for (const auto &def : kToolDefinitions)
		{
			assert(kToolDispatch.count(def.name) > 0 &&
				   "Tool in definitions table has no dispatch entry");
		}
#endif

		auto it = kToolDispatch.find(sMethodName);
		if (it != kToolDispatch.end())
		{
			if (!it->second(jsonRequest, jsonRPCRep))
			{
				if (!jsonRPCRep.isMember("error"))
				{
					jsonRPCRep["error"]["code"] = mcp::JSONRPC_INVALID_PARAMETER;
					jsonRPCRep["error"]["message"] = "Error executing tool: " + sMethodName;
				}
			}
		}
		else
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: Unsupported tool name: %s", sMethodName.c_str());
			jsonRPCRep["error"]["code"] = mcp::JSONRPC_METHOD_NOT_FOUND;
			jsonRPCRep["error"]["message"] = "Method not found";
		}
		//_log.Debug(DEBUG_WEBSERVER, "McpToolsCall: Returning result for method (%s): %s", sMethodName.c_str(), jsonRPCRep.toStyledString().c_str());
	}

	void McpResourcesList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		_log.Debug(DEBUG_WEBSERVER, "MCP: Handling resources/list request.");

		int offset = 0;
		if (jsonRequest.isMember("params") && jsonRequest["params"].isMember("cursor"))
		{
			std::string cursorStr = jsonRequest["params"]["cursor"].asString();
			if (!cursorStr.empty())
			{
				std::string decoded = base64_decode(cursorStr);
				Json::Value cursorObj;
				if (ParseJSon(decoded, cursorObj) && cursorObj.isMember("offset") && cursorObj["offset"].isInt())
					offset = cursorObj["offset"].asInt();
			}
		}
		if (offset < 0)
			offset = 0;

		Json::Value allResources(Json::arrayValue);

		auto addResource = [&](const char* uri, const char* name, const char* title, const char* description, const char* mimeType) {
			Json::Value resource;
			resource["uri"] = uri;
			resource["name"] = name;
			resource["title"] = title;
			resource["description"] = description;
			resource["mimeType"] = mimeType;
			allResources.append(resource);
		};

		addResource("domoticz://devices",        "All Devices",    "All Devices",         "Summary of all used devices in the system",                                                                    "text/plain");
		addResource("domoticz://rooms",           "Rooms",          "Rooms",               "All configured rooms/plans",                                                                                   "text/plain");
		addResource("domoticz://scenes",          "Scenes",         "Scenes",              "All scenes and groups",                                                                                        "text/plain");
		addResource("domoticz://user-variables",  "User Variables", "User Variables",      "All user variables",                                                                                           "text/plain");
		addResource("domoticz://events",          "Event Scripts",  "Event Scripts",       "All automation event scripts",                                                                                 "text/plain");
		addResource("domoticz://security",        "Security",       "Security",            "Security panel status",                                                                                        "text/plain");
		addResource("domoticz://settings",        "Settings",       "Settings",            "System configuration (key subset)",                                                                            "text/plain");
		addResource("domoticz://log",             "System Log",     "System Log",          "Recent system log entries",                                                                                    "text/plain");
		addResource("domoticz://sensor-types",    "Sensor Types",   "Virtual Sensor Types","All sensor types available for create_sensor / create_virtual_sensor, with their names and numeric mapped values","text/plain");
		addResource("domoticz://hardware",        "Hardware",       "Hardware",            "All configured hardware instances with type, enabled status, and ID",                                          "text/plain");
		addResource("domoticz://notifications",   "Notifications",  "Notifications",       "All configured device notifications with trigger conditions and target systems",                               "text/plain");
		addResource("domoticz://timers",          "Timers",         "Timers",              "All device and scene timers with schedule and command details",                                                "text/plain");

		auto result = m_sql.safe_query("SELECT ID, Name FROM Floorplans");
		for (const auto &row : result)
		{
			std::string idx = row[0];
			std::string sName = row[1];
			Json::Value resource;
			resource["uri"] = "floorplan:///image/" + idx;
			resource["name"] = sName;
			resource["title"] = sName + " (Floorplan)";
			resource["description"] = "A Floorplan called " + sName + " with IDX " + idx;
			resource["mimeType"] = "image/*";
			Json::Value meta;
			meta["idx"] = atoi(idx.c_str());
			resource["_meta"] = meta;
			allResources.append(resource);
		}

		int total = (int)allResources.size();
		Json::Value page(Json::arrayValue);
		for (int i = offset; i < std::min(offset + MCP_LIST_PAGE_SIZE, total); i++)
			page.append(allResources[i]);
		jsonRPCRep["result"]["resources"] = page;

		if (offset + MCP_LIST_PAGE_SIZE < total)
		{
			Json::Value nextCursorObj;
			nextCursorObj["offset"] = offset + MCP_LIST_PAGE_SIZE;
			jsonRPCRep["result"]["nextCursor"] = base64_encode(JSonToRawString(nextCursorObj));
		}

		_log.Debug(DEBUG_WEBSERVER, "MCP: ResourcesList: Number of resources offered: %d", total);
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

	// Sensor type map — mirrors Dummy.cpp mappedsensorname exactly (44 entries).
	// mappedvalue: UI/API identifier (not sequential; assigned historically as types were added).
	// type/subtype: actual Domoticz device type constants used by CreateDevice().
	struct _tmapped { int mappedvalue; const char* name; int type; int subtype; };
	static const _tmapped kSensorMap[] = {
		{ 249,  "Air Quality",                                    249, 1  },  // pTypeAirQuality / sTypeVoc
		{ 7,    "Alert",                                          243, 22 },  // pTypeGeneral / sTypeAlert
		{ 9,    "Ampere (3 Phase)",                               89,  1  },  // pTypeCURRENT / sTypeELEC1
		{ 19,   "Ampere (1 Phase)",                               243, 23 },  // pTypeGeneral / sTypeCurrent
		{ 11,   "Barometer",                                      243, 26 },  // pTypeGeneral / sTypeBaro
		{ 113,  "Counter",                                        113, 0  },  // pTypeRFXMeter / sTypeRFXMeterCount
		{ 14,   "Counter Incremental",                            243, 28 },  // pTypeGeneral / sTypeCounterIncremental
		{ 1004, "Custom Sensor",                                  243, 31 },  // pTypeGeneral / sTypeCustom
		{ 13,   "Distance",                                       243, 27 },  // pTypeGeneral / sTypeDistance
		{ 18,   "Electric (Instant+Counter)",                     243, 29 },  // pTypeGeneral / sTypeKwh
		{ 3,    "Gas",                                            251, 2  },  // pTypeP1Gas / sTypeP1Gas
		{ 81,   "Humidity",                                       81,  1  },  // pTypeHUM / sTypeHUM1
		{ 16,   "Leaf Wetness",                                   243, 4  },  // pTypeGeneral / sTypeLeafWetness
		{ 246,  "Lux",                                            246, 1  },  // pTypeLux / sTypeLux
		{ 250,  "P1 Smart Meter (Electric)",                      250, 1  },  // pTypeP1Power / sTypeP1Power
		{ 1005, "Managed Counter",                                243, 33 },  // pTypeGeneral / sTypeManagedCounter
		{ 2,    "Percentage",                                     243, 6  },  // pTypeGeneral / sTypePercentage
		{ 1,    "Pressure (Bar)",                                 243, 9  },  // pTypeGeneral / sTypePressure
		{ 85,   "Rain",                                           85,  3  },  // pTypeRAIN / sTypeRAIN3
		{ 241,  "RGB Switch",                                     241, 2  },  // pTypeColorSwitch / sTypeColor_RGB
		{ 1003, "RGBW Switch",                                    241, 1  },  // pTypeColorSwitch / sTypeColor_RGB_W
		{ 93,   "Scale",                                          93,  1  },  // pTypeWEIGHT / sTypeWEIGHT1
		{ 1002, "Selector Switch",                                244, 62 },  // pTypeGeneralSwitch / sSwitchTypeSelector
		{ 15,   "Soil Moisture",                                  243, 3  },  // pTypeGeneral / sTypeSoilMoisture
		{ 20,   "Solar Radiation",                                243, 2  },  // pTypeGeneral / sTypeSolarRadiation
		{ 10,   "Sound Level",                                    243, 24 },  // pTypeGeneral / sTypeSoundLevel
		{ 6,    "Switch",                                         244, 73 },  // pTypeGeneralSwitch / sSwitchGeneralSwitch
		{ 80,   "Temperature",                                    80,  5  },  // pTypeTEMP / sTypeTEMP5
		{ 82,   "Temp+Hum",                                       82,  1  },  // pTypeTEMP_HUM / sTypeTH1
		{ 84,   "Temp+Hum+Baro",                                  84,  1  },  // pTypeTEMP_HUM_BARO / sTypeTHB1
		{ 247,  "Temp+Baro",                                      247, 1  },  // pTypeTEMP_BARO / sTypeBMP085
		{ 5,    "Text",                                           243, 19 },  // pTypeGeneral / sTypeTextStatus
		{ 8,    "Thermostat Setpoint",                            242, 1  },  // pTypeSetpoint / sTypeSetpoint
		{ 248,  "Usage (Electric)",                               248, 1  },  // pTypeUsage / sTypeElectric
		{ 87,   "UV",                                             87,  1  },  // pTypeUV / sTypeUV1
		{ 12,   "Visibility",                                     243, 1  },  // pTypeGeneral / sTypeVisibility
		{ 4,    "Voltage",                                        243, 8  },  // pTypeGeneral / sTypeVoltage
		{ 1000, "Waterflow",                                      243, 30 },  // pTypeGeneral / sTypeWaterflow
		{ 86,   "Wind",                                           86,  1  },  // pTypeWIND / sTypeWIND1
		{ 1001, "Wind+Temp+Chill",                                86,  4  },  // pTypeWIND / sTypeWIND4
		{ 1006, "Thermostat (Temp/Setpoint)",                     73,  0  },  // pTypeThermostat6 / sTypeThermostat6Temp
		{ 1007, "Thermostat (Temp/Hum/Setpoint)",                 73,  1  },  // pTypeThermostat6 / sTypeThermostat6TempHum
		{ 1008, "Thermostat (Temp/Baro/Setpoint)",                73,  2  },  // pTypeThermostat6 / sTypeThermostat6TempBaro
		{ 1009, "Thermostat (Temp/Hum/Baro/Setpoint)",            73,  3  },  // pTypeThermostat6 / sTypeThermostat6TempHumBaro
		{ 0,    nullptr,                                          0,   0  }   // sentinel
	};

	void McpResourcesRead(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		// Check if the required parameters are present
		if (!jsonRequest.isMember("params") || !jsonRequest["params"].isMember("uri"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: Missing required resource parameter 'uri' in resources/read request.");
			jsonRPCRep["error"]["code"] = mcp::JSONRPC_INVALID_PARAMETER;
			jsonRPCRep["error"]["message"] = "Missing required parameter 'uri'";
			return;
		}
		std::string sReadURI = jsonRequest["params"]["uri"].asString();
		_log.Debug(DEBUG_WEBSERVER, "MCP: Handling resources/read request for %s.", sReadURI.c_str());

		Json::Value resource;
		resource["uri"] = sReadURI;

		// --- domoticz:// scheme handler ---
		if (sReadURI.substr(0, 11) == "domoticz://")
		{
			std::string sPath = sReadURI.substr(11); // e.g. "device/42" or "devices"
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
					jsonRPCRep["error"]["code"] = mcp::JSONRPC_INVALID_PARAMETER;
					jsonRPCRep["error"]["message"] = "Invalid device idx in URI";
					return;
				}
				auto result = m_sql.safe_query(
					"SELECT Name, HardwareID, DeviceID, Type, SubType, nValue, sValue, LastUpdate FROM DeviceStatus WHERE ID=%d", nIdx);
				if (result.empty())
				{
					_log.Debug(DEBUG_WEBSERVER, "MCP: resources/read: No device found with IDX %d", nIdx);
					jsonRPCRep["error"]["code"] = mcp::MCP_RESOURCE_NOT_FOUND;
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
					jsonRPCRep["error"]["code"] = mcp::JSONRPC_INVALID_PARAMETER;
					jsonRPCRep["error"]["message"] = "Invalid room idx in URI";
					return;
				}
				auto planResult = m_sql.safe_query("SELECT Name FROM Plans WHERE ID=%d", nIdx);
				if (planResult.empty())
				{
					jsonRPCRep["error"]["code"] = mcp::MCP_RESOURCE_NOT_FOUND;
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
				auto result = m_sql.safe_query("SELECT ID, Name, SceneType, nValue FROM Scenes ORDER BY Name");
				std::string sText;
				if (result.empty())
					sText = "No scenes or groups configured.";
				else
				{
					sText = std::to_string(result.size()) + " scene(s)/group(s):\n";
					for (const auto &row : result)
					{
						std::string sType = (atoi(row[2].c_str()) == 1) ? "Group" : "Scene";
						std::string sStatus = (atoi(row[3].c_str()) == 1) ? "On" : "Off";
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
					jsonRPCRep["error"]["code"] = mcp::JSONRPC_INVALID_PARAMETER;
					jsonRPCRep["error"]["message"] = "Invalid scene idx in URI";
					return;
				}
				auto scResult = m_sql.safe_query("SELECT Name FROM Scenes WHERE ID=%d", nIdx);
				if (scResult.empty())
				{
					jsonRPCRep["error"]["code"] = mcp::MCP_RESOURCE_NOT_FOUND;
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
					jsonRPCRep["error"]["code"] = mcp::JSONRPC_INVALID_PARAMETER;
					jsonRPCRep["error"]["message"] = "Invalid user-variable idx in URI";
					return;
				}
				auto result = m_sql.safe_query("SELECT ID, Name, ValueType, Value, LastUpdate FROM UserVariables WHERE ID=%d", nIdx);
				if (result.empty())
				{
					jsonRPCRep["error"]["code"] = mcp::MCP_RESOURCE_NOT_FOUND;
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
					jsonRPCRep["error"]["code"] = mcp::JSONRPC_INVALID_PARAMETER;
					jsonRPCRep["error"]["message"] = "Invalid event idx in URI";
					return;
				}
				auto result = m_sql.safe_query(
					"SELECT ID, Name, XMLStatement, Status, Interpreter FROM EventMaster WHERE ID=%d", nIdx);
				if (result.empty())
				{
					jsonRPCRep["error"]["code"] = mcp::MCP_RESOURCE_NOT_FOUND;
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
				resource["name"] = "Settings";
				resource["mimeType"] = "text/plain";
				resource["text"] = buildSettingsText();
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
			else if (sResourceType == "sensor-types")
			{
				std::string sText = "Virtual Sensor Types for create_sensor / create_virtual_sensor\n";
				sText += "==============================================================\n";
				sText += "Use the 'Name' column as the sensortype value. Numeric mapped values are also accepted.\n\n";
				sText += "Name                        Mapped\n";
				sText += "--------------------------- ------\n";
				for (int i = 0; kSensorMap[i].name != nullptr; i++)
				{
					char line[64];
					std::snprintf(line, sizeof(line), "%-27s %d\n", kSensorMap[i].name, kSensorMap[i].mappedvalue);
					sText += line;
				}
				resource["name"] = "Virtual Sensor Types";
				resource["mimeType"] = "text/plain";
				resource["text"] = sText;
			}
			else if (sResourceType == "hardware")
			{
				auto result = m_sql.safe_query("SELECT ID, Name, Type, Enabled FROM Hardware ORDER BY Name");
				std::string sText;
				if (result.empty())
					sText = "No hardware configured.";
				else
				{
					sText = std::to_string(result.size()) + " hardware instance(s):\n";
					for (const auto &row : result)
					{
						int iType = atoi(row[2].c_str());
						bool bEnabled = (atoi(row[3].c_str()) != 0);
						const char *sTypeName = Hardware_Type_Desc(iType);
						sText += "- \"" + row[1] + "\" [" + (sTypeName ? sTypeName : "Unknown") + ", " + (bEnabled ? "enabled" : "disabled") + ", idx=" + row[0] + "]\n";
					}
				}
				resource["name"] = "Hardware";
				resource["mimeType"] = "text/plain";
				resource["text"] = sText;
			}
			else if (sResourceType == "notifications")
			{
				auto result = m_sql.safe_query(
					"SELECT N.ID, N.Params, N.CustomMessage, N.ActiveSystems, N.Priority, N.Active, D.Name "
					"FROM Notifications N LEFT JOIN DeviceStatus D ON D.ID=N.DeviceRowID "
					"ORDER BY D.Name, N.ID");
				std::string sText;
				if (result.empty())
					sText = "No notifications configured.";
				else
				{
					sText = std::to_string(result.size()) + " notification(s):\n";
					for (const auto &row : result)
					{
						std::string sDevice = row[6].empty() ? "(unknown device)" : row[6];
						std::string sParams = row[1];
						std::string sCustomMsg = row[2];
						std::string sSystems = row[3];
						int iPriority = atoi(row[4].c_str());
						bool bActive = (atoi(row[5].c_str()) != 0);
						sText += "- Device \"" + sDevice + "\": condition=" + sParams;
						if (!sCustomMsg.empty())
							sText += ", msg=\"" + sCustomMsg + "\"";
						if (!sSystems.empty())
							sText += ", systems=" + sSystems;
						if (iPriority != 0)
							sText += ", priority=" + std::to_string(iPriority);
						sText += " [" + std::string(bActive ? "active" : "inactive") + ", idx=" + row[0] + "]\n";
					}
				}
				resource["name"] = "Notifications";
				resource["mimeType"] = "text/plain";
				resource["text"] = sText;
			}
			else if (sResourceType == "timers")
			{
				static const struct { int bit; const char* name; } kDays[] = {
					{ 1, "Mon" }, { 2, "Tue" }, { 4, "Wed" }, { 8, "Thu" },
					{ 16, "Fri" }, { 32, "Sat" }, { 64, "Sun" }
				};
				auto buildDays = [&](int iDays) -> std::string {
					if (iDays & 128) return "Every day";
					std::string s;
					for (const auto &d : kDays)
						if (iDays & d.bit) { if (!s.empty()) s += ","; s += d.name; }
					return s.empty() ? "?" : s;
				};

				std::string sText;
				int iTotal = 0;

				auto devTimers = m_sql.safe_query(
					"SELECT T.ID, T.Active, T.Time, T.Type, T.Cmd, T.Level, T.Days, D.Name "
					"FROM Timers T LEFT JOIN DeviceStatus D ON D.ID=T.DeviceRowID "
					"ORDER BY D.Name, T.Time");
				for (const auto &row : devTimers)
				{
					iTotal++;
					std::string sDevice = row[7].empty() ? "(unknown)" : row[7];
					bool bActive = (atoi(row[1].c_str()) != 0);
					std::string sTime = row[2];
					int iType = atoi(row[3].c_str());
					int iCmd = atoi(row[4].c_str());
					int iLevel = atoi(row[5].c_str());
					int iDays = atoi(row[6].c_str());
					const char *sType = Timer_Type_Desc(iType);
					const char *sCmd = Timer_Cmd_Desc(iCmd);
					sText += "- Device \"" + sDevice + "\": " + sTime + " [" + (sType ? sType : "?") + ", " + buildDays(iDays) + ", cmd=" + (sCmd ? sCmd : "?");
					if (iCmd == 2 || iCmd == 13)
						sText += " level=" + std::to_string(iLevel);
					sText += ", " + std::string(bActive ? "active" : "inactive") + ", idx=" + row[0] + "]\n";
				}

				auto sceneTimers = m_sql.safe_query(
					"SELECT T.ID, T.Active, T.Time, T.Type, T.Cmd, T.Level, T.Days, S.Name "
					"FROM SceneTimers T LEFT JOIN Scenes S ON S.ID=T.SceneRowID "
					"ORDER BY S.Name, T.Time");
				for (const auto &row : sceneTimers)
				{
					iTotal++;
					std::string sScene = row[7].empty() ? "(unknown)" : row[7];
					bool bActive = (atoi(row[1].c_str()) != 0);
					std::string sTime = row[2];
					int iType = atoi(row[3].c_str());
					int iCmd = atoi(row[4].c_str());
					int iDays = atoi(row[6].c_str());
					const char *sType = Timer_Type_Desc(iType);
					const char *sCmd = Timer_Cmd_Desc(iCmd);
					sText += "- Scene \"" + sScene + "\": " + sTime + " [" + (sType ? sType : "?") + ", " + buildDays(iDays) + ", cmd=" + (sCmd ? sCmd : "?") + ", " + std::string(bActive ? "active" : "inactive") + ", idx=" + row[0] + "]\n";
				}

				if (iTotal == 0)
					sText = "No timers configured.";
				else
					sText = std::to_string(iTotal) + " timer(s):\n" + sText;

				resource["name"] = "Timers";
				resource["mimeType"] = "text/plain";
				resource["text"] = sText;
			}
			else
			{
				_log.Debug(DEBUG_WEBSERVER, "MCP: resources/read: Unknown domoticz:// resource type: %s", sResourceType.c_str());
				jsonRPCRep["error"]["code"] = mcp::MCP_RESOURCE_NOT_FOUND;
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
				jsonRPCRep["error"]["code"] = mcp::JSONRPC_INVALID_PARAMETER;
				jsonRPCRep["error"]["message"] = "Invalid floorplan idx in URI";
				return;
			}
			auto result = m_sql.safe_query("SELECT Name, Scalefactor FROM Floorplans WHERE ID=%d", nIdx);
			if (result.empty())
			{
				_log.Debug(DEBUG_WEBSERVER, "MCP: resources/read: No floorplan found with IDX %d", nIdx);
				jsonRPCRep["error"]["code"] = mcp::MCP_RESOURCE_NOT_FOUND;
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
			jsonRPCRep["error"]["code"] = mcp::MCP_RESOURCE_NOT_FOUND;
			jsonRPCRep["error"]["message"] = "Unsupported resource URI scheme";
			return;
		}

		jsonRPCRep["result"]["contents"].append(resource);

		_log.Debug(DEBUG_WEBSERVER, "MCP: Offering resources/read request result for: %s", resource["uri"].asString().c_str());
	}

	void McpPromptsList(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		_log.Debug(DEBUG_WEBSERVER, "MCP: Handling prompts/list request.");

		struct PromptArg { const char* name; const char* description; bool required; };
		struct PromptDef { const char* name; const char* title; const char* description; std::vector<PromptArg> args; };

		static const std::vector<PromptDef> kPrompts = {
			{ "housesummary",       "Get a status overview",
			  "Summarize the current status of all sensors and devices in the house (optionally limited to a specific room)",
			  { { "room", "The room to limit the summary to (optional)", false } } },
			{ "systemanalysis",     "Get a system analysis",
			  "Analyze the current status of the system and provide insights", {} },
			{ "troubleshoot_device","Troubleshoot a device",
			  "Diagnose issues with a specific device by analyzing its current state, recent history, and system logs",
			  { { "device", "Name or IDX of the device to troubleshoot", true } } },
			{ "analyze_automations","Analyze automation scripts",
			  "Review all event scripts for logic issues, inefficiencies, or improvement opportunities", {} },
			{ "analyze_event",      "Analyze an event script",
			  "Review a specific event script for logic issues, inefficiencies, or improvement opportunities",
			  { { "event", "Name of the event script to analyze", true } } },
			{ "energy_report",      "Energy consumption report",
			  "Summarize power and energy consumption across all electric sensors, identify high consumers, and compare to recent history", {} },
			{ "security_check",     "Security status check",
			  "Review security panel status, door/window sensors, cameras, and recent alerts", {} },
			{ "battery_status",     "Battery status report",
			  "List all battery-powered devices, flag low-battery ones, and suggest replacements", {} },
			{ "climate_overview",   "Climate overview",
			  "Summarize temperature, humidity, and thermostat setpoints per room and suggest comfort improvements", {} },
			{ "scene_optimizer",    "Scene optimizer",
			  "Review all scenes and groups, identify redundant or conflicting ones, and suggest consolidation", {} },
			{ "hardware_health",    "Hardware health check",
			  "Check all hardware instances for connectivity and errors, and flag anything offline or problematic", {} },
			{ "create_automation",  "Create an automation script",
			  "Guide through creating a new dzVents event script for a described automation rule",
			  { { "rule", "Description of the automation rule to implement", true } } },
			{ "daily_report",       "Daily digest report",
			  "Generate a daily digest covering overnight anomalies, battery warnings, offline devices, and energy highlights", {} },
		};

		int offset = 0;
		if (jsonRequest.isMember("params") && jsonRequest["params"].isMember("cursor"))
		{
			std::string cursorStr = jsonRequest["params"]["cursor"].asString();
			if (!cursorStr.empty())
			{
				std::string decoded = base64_decode(cursorStr);
				Json::Value cursorObj;
				if (ParseJSon(decoded, cursorObj) && cursorObj.isMember("offset") && cursorObj["offset"].isInt())
					offset = cursorObj["offset"].asInt();
			}
		}
		if (offset < 0)
			offset = 0;

		Json::Value allPrompts(Json::arrayValue);
		for (const auto &def : kPrompts)
		{
			Json::Value prompt;
			prompt["name"]        = def.name;
			prompt["title"]       = def.title;
			prompt["description"] = def.description;
			prompt["arguments"]   = Json::Value(Json::arrayValue);
			for (const auto &a : def.args)
			{
				Json::Value arg;
				arg["name"]        = a.name;
				arg["description"] = a.description;
				arg["required"]    = a.required;
				prompt["arguments"].append(arg);
			}
			allPrompts.append(prompt);
		}

		int total = (int)allPrompts.size();
		Json::Value page(Json::arrayValue);
		for (int i = offset; i < std::min(offset + MCP_LIST_PAGE_SIZE, total); i++)
			page.append(allPrompts[i]);
		jsonRPCRep["result"]["prompts"] = page;

		if (offset + MCP_LIST_PAGE_SIZE < total)
		{
			Json::Value nextCursorObj;
			nextCursorObj["offset"] = offset + MCP_LIST_PAGE_SIZE;
			jsonRPCRep["result"]["nextCursor"] = base64_encode(JSonToRawString(nextCursorObj));
		}
	}

	void McpPromptsGet(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		if (!jsonRequest.isMember("params") || !jsonRequest["params"].isMember("name"))
		{
			jsonRPCRep["error"]["code"] = mcp::JSONRPC_INVALID_PARAMETER;
			jsonRPCRep["error"]["message"] = "Missing required parameter 'name'";
			return;
		}
		std::string sPromptName = jsonRequest["params"]["name"].asString();
		_log.Debug(DEBUG_WEBSERVER, "MCP: Handling prompts/get request (%s).", sPromptName.c_str());

		auto getArg = [&](const char* argName) -> std::string {
			if (jsonRequest["params"].isMember("arguments") && jsonRequest["params"]["arguments"].isMember(argName))
				return jsonRequest["params"]["arguments"][argName].asString();
			return {};
		};

		auto escapeQuotes = [](const std::string &s) -> std::string {
			std::string result;
			result.reserve(s.size() * 2);
			for (unsigned char c : s) {
				if      (c == '"')  result += "\\\"";
				else if (c == '\\') result += "\\\\";
				else if (c == '\n') result += "\\n";
				else if (c == '\r') result += "\\r";
				else if (c == '\t') result += "\\t";
				else if (c < 0x20) { char buf[7]; snprintf(buf, sizeof(buf), "\\u%04x", c); result += buf; }
				else result += static_cast<char>(c);
			}
			return result;
		};

		auto makeUserMessage = [&](Json::Value &rep, const std::string &description, const std::string &text) {
			rep["result"]["description"] = description;
			rep["result"]["messages"] = Json::Value(Json::arrayValue);
			Json::Value msg;
			msg["role"] = "user";
			msg["content"]["type"] = "text";
			msg["content"]["text"] = text;
			rep["result"]["messages"].append(msg);
		};

		if (sPromptName == "housesummary")
		{
			std::string sRoom = getArg("room");
			std::string sText = "As the friendly butler of the house, please summarize the current status of all sensors and devices, preferably grouped by room.";
			sText += " Use get_all_devices to retrieve the full device list.";
			if (!sRoom.empty())
				sText += " Limit the summary to the room named \"" + escapeQuotes(sRoom) + "\".";
			sText += " Also use get_scenes to list available scenes/groups and their current state.";
			sText += " Present a concise, readable overview suitable for a quick status check.";
			makeUserMessage(jsonRPCRep,
				sRoom.empty() ? "Summarize the current status of the whole house"
				              : "Summarize the current status of room: " + sRoom,
				sText);
		}
		else if (sPromptName == "systemanalysis")
		{
			std::string sText = "As the friendly butler of the house, please make an analysis of the current status of the system by analyzing all available log information, and providing suggestions if needed. ";
			sText += "State the time window of the logging you have analyzed. If the latest log entries are older than 3 minutes, make sure to first retrieve the latest log entries before making your analysis.";
			makeUserMessage(jsonRPCRep, "Analyze the current status of the system and provide insights", sText);
		}
		else if (sPromptName == "troubleshoot_device")
		{
			std::string sDevice = getArg("device");
			if (sDevice.empty())
			{
				jsonRPCRep["error"]["code"] = mcp::JSONRPC_INVALID_PARAMETER;
				jsonRPCRep["error"]["message"] = "Missing required argument 'device'";
				return;
			}
			std::string sText = "Please troubleshoot the Domoticz device named \"" + escapeQuotes(sDevice) + "\". ";
			sText += "Use the available tools to: ";
			sText += "1) Find the device using search_devices or get_device if you know its IDX. ";
			sText += "2) Get the current state using get_switch_state or get_sensor_value. ";
			sText += "3) Check its recent history using get_sensor_history. ";
			sText += "4) Check the system log using get_logging for any errors related to this device. ";
			sText += "5) Check if its hardware is online using get_hardware. ";
			sText += "Summarize what you find and suggest any remediation steps.";
			makeUserMessage(jsonRPCRep, "Troubleshoot device: " + sDevice, sText);
		}
		else if (sPromptName == "analyze_automations")
		{
			std::string sText = "Please analyze all Domoticz automation event scripts. ";
			sText += "Use the available tools to: ";
			sText += "1) List all event scripts using get_events. ";
			sText += "2) Read each script's source code using get_event. ";
			sText += "3) Review each script for logic errors, inefficiencies, or improvement opportunities. ";
			sText += "4) Check the system log for any automation-related errors using get_logging. ";
			sText += "Provide a structured report with findings and suggestions for each script.";
			makeUserMessage(jsonRPCRep, "Review all event scripts for logic issues, inefficiencies, or improvement opportunities", sText);
		}
		else if (sPromptName == "analyze_event")
		{
			std::string sEvent = getArg("event");
			if (sEvent.empty())
			{
				jsonRPCRep["error"]["code"] = mcp::JSONRPC_INVALID_PARAMETER;
				jsonRPCRep["error"]["message"] = "Missing required argument 'event'";
				return;
			}
			std::string sText = "Please analyze the Domoticz automation event script named \"" + escapeQuotes(sEvent) + "\". ";
			sText += "Use the available tools to: ";
			sText += "1) Retrieve the script source code using get_event. ";
			sText += "2) Check the system log for any errors related to this script using get_logging. ";
			sText += "3) Review the script for logic errors, inefficiencies, or improvement opportunities. ";
			sText += "Provide a structured report with findings and concrete suggestions.";
			makeUserMessage(jsonRPCRep, "Analyze event script: " + sEvent, sText);
		}
		else if (sPromptName == "energy_report")
		{
			std::string sText = "Please generate an energy consumption report for this Domoticz system. ";
			sText += "Use the available tools to: ";
			sText += "1) List all devices using get_all_devices and identify electric, power, and energy sensors (kWh, Watt, P1 Smart Meter, Usage Electric types). ";
			sText += "2) Read current values using get_sensor_value for each energy device. ";
			sText += "3) Check recent history using get_sensor_history for the main consumers. ";
			sText += "4) Identify the highest consumers and any unusual patterns. ";
			sText += "Provide a structured report with current readings, estimated daily/monthly costs if possible, and recommendations for reducing consumption.";
			makeUserMessage(jsonRPCRep, "Summarize energy consumption and identify high consumers", sText);
		}
		else if (sPromptName == "security_check")
		{
			std::string sText = "Please perform a security check of this Domoticz system. ";
			sText += "Use the available tools to: ";
			sText += "1) Check the security panel status using get_security_status. ";
			sText += "2) List all devices using get_all_devices and identify door/window sensors, motion sensors, and smoke detectors. ";
			sText += "3) Check the current state of all security-related sensors using get_switch_state or get_sensor_value. ";
			sText += "4) List all cameras using get_cameras. ";
			sText += "5) Review recent alerts in the system log using get_logging. ";
			sText += "Provide a security status overview, flag any open doors/windows or triggered sensors, and suggest improvements.";
			makeUserMessage(jsonRPCRep, "Review security panel, sensors, cameras, and recent alerts", sText);
		}
		else if (sPromptName == "battery_status")
		{
			std::string sText = "Please check the battery status of all battery-powered devices in this Domoticz system. ";
			sText += "Use the available tools to: ";
			sText += "1) List all devices using get_all_devices and look for devices with battery level information (BatteryLevel field). ";
			sText += "2) Flag any devices with low battery (BatteryLevel below 20%). ";
			sText += "3) Also identify devices that have not reported recently based on their LastUpdate field. ";
			sText += "Provide a battery health report sorted by urgency, indicating which batteries need immediate replacement and which should be monitored.";
			makeUserMessage(jsonRPCRep, "List battery-powered devices and flag low-battery ones", sText);
		}
		else if (sPromptName == "climate_overview")
		{
			std::string sText = "Please provide a climate overview of this Domoticz system. ";
			sText += "Use the available tools to: ";
			sText += "1) List all devices using get_all_devices and identify temperature, humidity, and thermostat/setpoint sensors, grouped by room. ";
			sText += "2) Read current values using get_sensor_value for each climate sensor. ";
			sText += "3) Compare thermostat setpoints to actual measured temperatures. ";
			sText += "Provide a room-by-room climate summary. Flag rooms outside comfortable ranges (temperature below 18°C or above 26°C, humidity outside 40-60%). Suggest any adjustments.";
			makeUserMessage(jsonRPCRep, "Summarize temperature, humidity, and thermostat setpoints per room", sText);
		}
		else if (sPromptName == "scene_optimizer")
		{
			std::string sText = "Please analyze and optimize the scenes and groups in this Domoticz system. ";
			sText += "Use the available tools to: ";
			sText += "1) List all scenes using get_scenes. ";
			sText += "2) Get the devices in each scene using get_scene_devices. ";
			sText += "3) Review scene configurations for redundancies, conflicts, or missing useful combinations. ";
			sText += "Provide a structured analysis with suggestions for consolidation, renaming for clarity, or new scenes that would be useful based on the existing device setup.";
			makeUserMessage(jsonRPCRep, "Review scenes and groups, identify redundancies, and suggest improvements", sText);
		}
		else if (sPromptName == "hardware_health")
		{
			std::string sText = "Please check the health of all hardware instances in this Domoticz system. ";
			sText += "Use the available tools to: ";
			sText += "1) List all hardware using get_hardware and identify any that are disabled or show errors. ";
			sText += "2) Cross-reference with get_all_devices to find devices that haven't reported recently (stale LastUpdate). ";
			sText += "3) Check the system log using get_logging for any hardware-related errors or warnings. ";
			sText += "Provide a hardware health report, flag any offline or problematic hardware instances, and suggest remediation steps.";
			makeUserMessage(jsonRPCRep, "Check hardware connectivity, errors, and offline instances", sText);
		}
		else if (sPromptName == "create_automation")
		{
			std::string sRule = getArg("rule");
			if (sRule.empty())
			{
				jsonRPCRep["error"]["code"] = mcp::JSONRPC_INVALID_PARAMETER;
				jsonRPCRep["error"]["message"] = "Missing required argument 'rule'";
				return;
			}
			std::string sText = "Please create a new Domoticz automation event script for the following rule: \"" + escapeQuotes(sRule) + "\". ";
			sText += "Use the available tools to: ";
			sText += "1) List existing event scripts using get_events to avoid naming conflicts and understand existing patterns. ";
			sText += "2) Find relevant devices using search_devices or get_all_devices. ";
			sText += "3) Write the automation script in dzVents (preferred) following Domoticz best practices: use device names, handle edge cases, include meaningful log messages. ";
			sText += "4) Create the script using create_event with interpreter='dzVents'. ";
			sText += "Confirm the script was created successfully and explain what it does.";
			makeUserMessage(jsonRPCRep, "Create automation script for: " + sRule, sText);
		}
		else if (sPromptName == "daily_report")
		{
			std::string sText = "Please generate a daily digest report for this Domoticz system. ";
			sText += "Use the available tools to: ";
			sText += "1) Check the system log using get_logging for any warnings, errors, or anomalies. ";
			sText += "2) List all devices using get_all_devices and identify: devices with low battery (BatteryLevel < 20%), devices that haven't reported in over 24 hours, and any unusual states. ";
			sText += "3) Check energy consumption using get_sensor_value for power and energy devices. ";
			sText += "4) Review security status using get_security_status. ";
			sText += "Provide a concise daily digest with clearly separated sections: Alerts requiring attention, Battery warnings, Offline/stale devices, Energy highlights, and a General system health summary.";
			makeUserMessage(jsonRPCRep, "Daily digest: anomalies, battery warnings, offline devices, energy highlights", sText);
		}
		else
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: prompts/get: Unsupported prompt name: %s", sPromptName.c_str());
			jsonRPCRep["error"]["code"] = mcp::JSONRPC_INVALID_PARAMETER;
			jsonRPCRep["error"]["message"] = "Unknown prompt name: " + sPromptName;
		}
	}

	void McpCompletionComplete(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		_log.Debug(DEBUG_WEBSERVER, "MCP: Handling completion/complete request.");

		// Helper lambda: build empty completion result
		auto emptyResult = [&]() {
			jsonRPCRep["result"]["completion"]["values"] = Json::Value(Json::arrayValue);
			jsonRPCRep["result"]["completion"]["total"] = 0;
			jsonRPCRep["result"]["completion"]["hasMore"] = false;
		};

		if (!jsonRequest.isMember("params") ||
		    !jsonRequest["params"].isMember("ref") ||
		    !jsonRequest["params"].isMember("argument"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: completion/complete: Missing required params.");
			emptyResult();
			return;
		}

		const Json::Value &refObj  = jsonRequest["params"]["ref"];
		const Json::Value &argObj  = jsonRequest["params"]["argument"];

		if (!refObj.isMember("type") || !refObj.isMember("name") ||
		    !argObj.isMember("name") || !argObj.isMember("value"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: completion/complete: Malformed ref or argument object.");
			emptyResult();
			return;
		}

		const std::string sRefType  = refObj["type"].asString();
		const std::string sRefName  = refObj["name"].asString();
		const std::string sArgName  = argObj["name"].asString();
		const std::string sPartial  = argObj["value"].asString();

		_log.Debug(DEBUG_WEBSERVER, "MCP: completion/complete: refType=%s refName=%s argName=%s partial='%s'",
		           sRefType.c_str(), sRefName.c_str(), sArgName.c_str(), sPartial.c_str());

		// Build lowercase partial for case-insensitive prefix matching
		std::string sPartialLower = sPartial;
		stdlower(sPartialLower);

		// Query rows: vector of strings (display value)
		std::vector<std::string> candidates;
		// When true the candidates are already filtered (e.g. via SQL LIKE), skip the prefix pass.
		bool bAlreadyFiltered = false;

		if (sRefType == "ref/prompt")
		{
			if (sRefName == "housesummary" && sArgName == "room")
			{
				auto rows = m_sql.safe_query("SELECT Name FROM Plans WHERE Name!='' ORDER BY Name");
				for (const auto &row : rows)
					candidates.push_back(row[0]);
			}
			else if (sRefName == "troubleshoot_device" && sArgName == "device")
			{
				auto rows = m_sql.safe_query("SELECT Name FROM DeviceStatus ORDER BY Name");
				for (const auto &row : rows)
					candidates.push_back(row[0]);
			}
			else if (sRefName == "analyze_event" && sArgName == "event")
			{
				auto rows = m_sql.safe_query("SELECT Name FROM EventMaster ORDER BY Name");
				for (const auto &row : rows)
					candidates.push_back(row[0]);
			}
			else
			{
				_log.Debug(DEBUG_WEBSERVER, "MCP: completion/complete: Unrecognised prompt/arg combination.");
				emptyResult();
				return;
			}
		}
		else if (sRefType == "ref/resource")
		{
			if (sRefName.find("domoticz://device/") != std::string::npos)
			{
				auto rows = m_sql.safe_query("SELECT ID, Name FROM DeviceStatus ORDER BY Name");
				for (const auto &row : rows)
					candidates.push_back(row[0] + " - " + row[1]);
			}
			else if (sRefName.find("domoticz://room/") != std::string::npos)
			{
				auto rows = m_sql.safe_query("SELECT ID, Name FROM Plans WHERE Name!='' ORDER BY Name");
				for (const auto &row : rows)
					candidates.push_back(row[0] + " - " + row[1]);
			}
			else if (sRefName.find("domoticz://scene/") != std::string::npos)
			{
				auto rows = m_sql.safe_query("SELECT ID, Name FROM Scenes ORDER BY Name");
				for (const auto &row : rows)
					candidates.push_back(row[0] + " - " + row[1]);
			}
			else if (sRefName.find("domoticz://user-variable/") != std::string::npos)
			{
				auto rows = m_sql.safe_query("SELECT ID, Name FROM UserVariables ORDER BY Name");
				for (const auto &row : rows)
					candidates.push_back(row[0] + " - " + row[1]);
			}
			else if (sRefName.find("domoticz://event/") != std::string::npos)
			{
				auto rows = m_sql.safe_query("SELECT ID, Name FROM EventMaster ORDER BY Name");
				for (const auto &row : rows)
					candidates.push_back(row[0] + " - " + row[1]);
			}
			else
			{
				_log.Debug(DEBUG_WEBSERVER, "MCP: completion/complete: Unrecognised resource URI template.");
				emptyResult();
				return;
			}
		}
		else if (sRefType == "ref/tool")
		{
			int bToolReportedTotal = 0;
			auto loadLikeRows = [&](const std::vector<std::vector<std::string>> &rows) -> bool {
				bool overflow = (rows.size() > 20);
				int limit = overflow ? 20 : static_cast<int>(rows.size());
				for (int i = 0; i < limit; ++i)
					candidates.push_back(rows[i][0]);
				bAlreadyFiltered = true;
				bToolReportedTotal = overflow ? limit + 1 : limit;
				return overflow;
			};

			bool bToolHasMore = false;
			if (sArgName == "name")
			{
				auto rows = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE Name LIKE '%%%q%%' ORDER BY Name LIMIT 21", sPartial.c_str());
				bToolHasMore = loadLikeRows(rows);
			}
			else if (sArgName == "scene")
			{
				auto rows = m_sql.safe_query("SELECT Name FROM Scenes WHERE Name LIKE '%%%q%%' ORDER BY Name LIMIT 21", sPartial.c_str());
				bToolHasMore = loadLikeRows(rows);
			}
			else if (sArgName == "room")
			{
				auto rows = m_sql.safe_query("SELECT Name FROM Plans WHERE Name LIKE '%%%q%%' ORDER BY Name LIMIT 21", sPartial.c_str());
				bToolHasMore = loadLikeRows(rows);
			}
			else if (sArgName == "level")
			{
				static const std::vector<std::string> logLevels = { "debug", "info", "notice", "warning", "error", "critical" };
				for (const auto &lvl : logLevels)
					candidates.push_back(lvl);
			}
			else if (sArgName == "uri")
			{
				static const std::vector<std::string> uris = {
					"domoticz://devices",
					"domoticz://scenes",
					"domoticz://devices/",
					"domoticz://scenes/"
				};
				for (const auto &uri : uris)
					candidates.push_back(uri);
			}
			else
			{
				_log.Debug(DEBUG_WEBSERVER, "MCP: completion/complete: No completion available for tool arg '%s'.", sArgName.c_str());
				emptyResult();
				return;
			}

			if (bAlreadyFiltered)
			{
				jsonRPCRep["result"]["completion"]["values"] = Json::Value(Json::arrayValue);
				for (const auto &c : candidates)
					jsonRPCRep["result"]["completion"]["values"].append(c);
				jsonRPCRep["result"]["completion"]["total"] = bToolReportedTotal;
				jsonRPCRep["result"]["completion"]["hasMore"] = bToolHasMore;
				_log.Debug(DEBUG_WEBSERVER, "MCP: completion/complete: Returning %d matches (hasMore=%s).",
				           static_cast<int>(candidates.size()), bToolHasMore ? "true" : "false");
				return;
			}
		}
		else
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: completion/complete: Unknown ref type '%s'.", sRefType.c_str());
			emptyResult();
			return;
		}

		// Case-insensitive prefix filter
		std::vector<std::string> matches;
		for (const auto &candidate : candidates)
		{
			std::string candidateLower = candidate;
			stdlower(candidateLower);
			if (sPartialLower.empty() || (candidateLower.size() >= sPartialLower.size() && candidateLower.substr(0, sPartialLower.size()) == sPartialLower))
				matches.push_back(candidate);
		}

		constexpr int kMaxValues = 100;
		int total = static_cast<int>(matches.size());
		bool hasMore = (total > kMaxValues);

		jsonRPCRep["result"]["completion"]["values"] = Json::Value(Json::arrayValue);
		int count = std::min(total, kMaxValues);
		for (int i = 0; i < count; ++i)
			jsonRPCRep["result"]["completion"]["values"].append(matches[i]);
		jsonRPCRep["result"]["completion"]["total"] = total;
		jsonRPCRep["result"]["completion"]["hasMore"] = hasMore;

		_log.Debug(DEBUG_WEBSERVER, "MCP: completion/complete: Returning %d/%d matches (hasMore=%s).",
		           count, total, hasMore ? "true" : "false");
	}

	static int jsonAsIdx(const Json::Value &v)
	{
		if (v.isString())
		{
			try { return std::stoi(v.asString()); }
			catch (const std::exception&) { return -1; }
		}
		return v.asInt();
	}

	bool getSwitchState(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool bHasIdx = args.isMember("idx");
		bool bHasName = args.isMember("switchname") && !args["switchname"].asString().empty();
		if (!bHasIdx && !bHasName)
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: getSwitchState: Missing required parameter 'switchname' or 'idx'");
			return false;
		}
		Json::Value device;
		std::string sSwitchName;
		bool bFound;
		if (bHasIdx)
		{
			bFound = getDeviceByIdx(jsonAsIdx(args["idx"]), device);
			sSwitchName = bFound ? device["Name"].asString() : "idx=" + std::to_string(jsonAsIdx(args["idx"]));
		}
		else
		{
			sSwitchName = args["switchname"].asString();
			bFound = getDeviceByName(sSwitchName, device);
		}
		std::string sSwitchState = bFound
			? "The current state of switch \"" + sSwitchName + "\" is: " + device["Data"].asString()
			: "No switch exists with the name " + sSwitchName;
		mcp::setToolResult(jsonRPCRep, sSwitchState, !bFound);
		return true;
	}

	bool toggleSwitchState(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool bHasIdx = args.isMember("idx");
		bool bHasName = args.isMember("switchname") && !args["switchname"].asString().empty();
		if (!bHasIdx && !bHasName)
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: toggleSwitchState: Missing required parameter 'switchname' or 'idx'");
			return false;
		}
		Json::Value device;
		std::string sSwitchName;
		bool bFound;
		if (bHasIdx)
		{
			bFound = getDeviceByIdx(jsonAsIdx(args["idx"]), device);
			sSwitchName = bFound ? device["Name"].asString() : "idx=" + std::to_string(jsonAsIdx(args["idx"]));
		}
		else
		{
			sSwitchName = args["switchname"].asString();
			bFound = getDeviceByName(sSwitchName, device);
		}
		std::string sSwitchState = "No switch exists with the name " + sSwitchName;
		if (bFound)
		{
			sSwitchState = "The state of switch \"" + sSwitchName + "\" before toggle was: " + device["Data"].asString() + ". ";
			SendProgress(tl_progressCtx.sid, tl_progressCtx.progressToken, 0, 2, "Sending command to device");
			sSwitchState += (m_mainworker.SwitchLight(device["idx"].asString(), "Toggle", "", "", "", 0, "") == MainWorker::eSwitchLightReturnCode::SL_ERROR ? "Error toggling the switch." : "Switch toggled successfully.");
			SendProgress(tl_progressCtx.sid, tl_progressCtx.progressToken, 2, 2, "Command sent");
		}
		mcp::setToolResult(jsonRPCRep, sSwitchState, !bFound);
		return true;
	}

	bool getSensorValue(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool bHasIdx = args.isMember("idx");
		bool bHasName = args.isMember("sensorname") && !args["sensorname"].asString().empty();
		if (!bHasIdx && !bHasName)
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: getSensorValue: Missing required parameter 'sensorname' or 'idx'");
			return false;
		}
		Json::Value device;
		std::string sSensorName;
		bool bFound;
		if (bHasIdx)
		{
			bFound = getDeviceByIdx(jsonAsIdx(args["idx"]), device);
			sSensorName = bFound ? device["Name"].asString() : "idx=" + std::to_string(jsonAsIdx(args["idx"]));
		}
		else
		{
			sSensorName = args["sensorname"].asString();
			bFound = getDeviceByName(sSensorName, device);
		}
		std::string sSensorValue = bFound
			? "The current value for sensor \"" + sSensorName + "\" is: " + device["Data"].asString()
			: "No sensor exists with the name " + sSensorName;
		mcp::setToolResult(jsonRPCRep, sSensorValue, !bFound);
		return true;
	}

	bool getFloorplan(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool bHasId = args.isMember("floorplan_id");
		bool bHasName = args.isMember("floorplan") && !args["floorplan"].asString().empty();
		if (!bHasId && !bHasName)
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: getFloorplan: Missing required parameter 'floorplan' or 'floorplan_id'");
			return false;
		}
		std::string sFloorplan = bHasName ? args["floorplan"].asString() : "idx=" + std::to_string(args["floorplan_id"].asInt());
		std::string sFloorplanValue = "No floorplan exists with the name " + sFloorplan;
		std::string sMimeType;
		bool bFound = false;

		std::vector<std::vector<std::string>> result;
		if (bHasId)
			result = m_sql.safe_query("SELECT ID FROM Floorplans WHERE ID=%d", args["floorplan_id"].asInt());
		else
			result = m_sql.safe_query("SELECT ID FROM Floorplans WHERE Name='%q'", sFloorplan.c_str());
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
			mcp::setToolImageResult(jsonRPCRep, sFloorplanValue, sMimeType);
		else
			mcp::setToolResult(jsonRPCRep, sFloorplanValue, true);
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
		mcp::setToolResult(jsonRPCRep, sResult, !bFound);
		return true;
	}

	bool setThermostatSetpoint(const Json::Value& jsonRequest, Json::Value& jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool bHasIdx = args.isMember("idx");
		bool bHasName = args.isMember("thermostatname") && !args["thermostatname"].asString().empty();
		if ((!bHasIdx && !bHasName) || !args.isMember("setpoint"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: setThermostatSetpoint: Missing required parameter 'thermostatname/setpoint'");
			return false;
		}
		float fNewSetpoint = args["setpoint"].asFloat();
		Json::Value device;
		std::string sThermostatName;
		bool bFound;
		if (bHasIdx)
		{
			bFound = getDeviceByIdx(jsonAsIdx(args["idx"]), device);
			sThermostatName = bFound ? device["Name"].asString() : "idx=" + std::to_string(jsonAsIdx(args["idx"]));
		}
		else
		{
			sThermostatName = args["thermostatname"].asString();
			bFound = getDeviceByName(sThermostatName, device);
		}
		std::string sThermostatState = "No thermostat exists with the name " + sThermostatName;
		if (bFound)
		{
			sThermostatState = "The value of thermostat \"" + sThermostatName + "\" before setting was: " + device["Data"].asString() + ". ";
			bFound = true;
			SendProgress(tl_progressCtx.sid, tl_progressCtx.progressToken, 0, 2, "Sending command to device");
			sThermostatState += (m_mainworker.SetSetPoint(device["idx"].asString(), fNewSetpoint, "MCP") == false ? "Error setting the setpoint." : "Setpoint set successfully.");
			SendProgress(tl_progressCtx.sid, tl_progressCtx.progressToken, 2, 2, "Command sent");
		}
		mcp::setToolResult(jsonRPCRep, sThermostatState, !bFound);
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

		mcp::setToolResult(jsonRPCRep, sResult, false);
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
				if (device.isMember("BatteryLevel") && device["BatteryLevel"].isInt())
				{
					int iBatt = device["BatteryLevel"].asInt();
					if (iBatt != 255)
						sResult += " battery=" + std::to_string(iBatt) + "%";
				}
				if (device.isMember("SignalLevel") && device["SignalLevel"].isInt())
				{
					int iSignalLevel = device["SignalLevel"].asInt();
					if (iSignalLevel != 12)
						sResult += " rssi=" + std::to_string(iSignalLevel);
				}
				sResult += "\n";
			}
		}

		if (iMatchCount == 0)
			sResult = "No devices found matching \"" + sQuery + "\"" + (sFilter.empty() ? "" : " with filter \"" + sFilter + "\"");
		else
			sResult = std::to_string(iMatchCount) + " device(s) found matching \"" + sQuery + "\":\n" + sResult;

		mcp::setToolResult(jsonRPCRep, sResult, false);
		return true;
	}



	bool getDeviceByName(const std::string &sDeviceName, Json::Value &device)
	{
		Json::Value jsonDevices;
		m_webservers.GetJSonDevices(jsonDevices, "true", "", "", "", "", "", false, false, false, 0, "", "");

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

	bool getDeviceByIdx(int nIdx, Json::Value &device)
	{
		Json::Value jsonDevices;
		// bDisplayDisabled=true: include disabled devices so sensor history/short-log tools
		// can still read data for devices that are temporarily disabled.
		m_webservers.GetJSonDevices(jsonDevices, "true", "", "", "", "", "", false, true, false, 0, "", "");
		for (const auto &dev : jsonDevices["result"])
		{
			if (dev.isObject() && dev.isMember("idx") && dev["idx"].asString() == std::to_string(nIdx))
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
		// args access is safe: McpToolsCall guarantees params/arguments exists before dispatching
		const Json::Value &args = jsonRequest["params"]["arguments"];

		std::string sFilter;
		if (args.isMember("filter"))
			sFilter = args["filter"].asString();

		// Validate filter against whitelist
		if (!sFilter.empty() && sFilter != "light" && sFilter != "temp" && sFilter != "weather" && sFilter != "utility")
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: getAllDevices: Invalid filter value: %s", sFilter.c_str());
			mcp::setToolResult(jsonRPCRep, "Invalid filter value '" + sFilter + "'. Valid values are: light, temp, weather, utility.", true);
			return true;
		}

		// -1 means "not provided"; valid Hardware IDs start at 1 (AUTOINCREMENT primary key)
		int nHwIdx = -1;
		if (args.isMember("hw_idx"))
		{
			nHwIdx = args["hw_idx"].asInt();
			auto hwResult = m_sql.safe_query("SELECT ID FROM Hardware WHERE ID=%d", nHwIdx);
			if (hwResult.empty() || hwResult[0].empty())
			{
				mcp::setToolResult(jsonRPCRep, "No hardware found with hw_idx=" + std::to_string(nHwIdx), true);
				return true;
			}
		}

		bool bIncludeUnused = args.isMember("include_unused") && args["include_unused"].asBool();
		std::string sUsed = bIncludeUnused ? "" : "true";

		Json::Value jsonDevices;
		std::string sHwIdxFilter = (nHwIdx >= 0 ? std::to_string(nHwIdx) : "");
		// empty hardwareid = return devices from all hardware adapters
		m_webservers.GetJSonDevices(jsonDevices, sUsed, sFilter, "Name", "", "", "", false, false, false, 0, "", sHwIdxFilter);

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
				bool bHasType = device.isMember("Type");
				if (bHasType)
					sResult += " [" + device["Type"].asString();
				if (device.isMember("SubType"))
					sResult += "/" + device["SubType"].asString();
				if (bHasType)
					sResult += "]";
				if (device.isMember("Data"))
					sResult += " = " + device["Data"].asString();
				if (device.isMember("idx"))
					sResult += " (idx=" + device["idx"].asString() + ")";
				if (device.isMember("BatteryLevel") && device["BatteryLevel"].isInt())
				{
					int iBatt = device["BatteryLevel"].asInt();
					if (iBatt != 255) // 255 = not available
						sResult += " battery=" + std::to_string(iBatt) + "%";
				}
				if (device.isMember("SignalLevel") && device["SignalLevel"].isInt())
				{
					int iSignalLevel = device["SignalLevel"].asInt();
					if (iSignalLevel != 12) // 12 = not available
						sResult += " rssi=" + std::to_string(iSignalLevel);
				}
				sResult += "\n";
			}
		}

		if (iCount == 0)
		{
			sResult = "No devices found";
			if (!sFilter.empty())
				sResult += " with filter \"" + sFilter + "\"";
			if (nHwIdx >= 0)
				sResult += " for hw_idx=" + std::to_string(nHwIdx);
		}
		else
			sResult = std::to_string(iCount) + " device(s):\n" + sResult;

		mcp::setToolResult(jsonRPCRep, sResult, false);
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
				if (device.isMember("BatteryLevel") && device["BatteryLevel"].isInt())
				{
					int iBatt = device["BatteryLevel"].asInt();
					if (iBatt != 255)
						sResult += "  BatteryLevel: " + std::to_string(iBatt) + "%\n";
				}
				if (device.isMember("SignalLevel") && device["SignalLevel"].isInt())
				{
					int iSignalLevel = device["SignalLevel"].asInt();
					if (iSignalLevel != 12)
						sResult += "  SignalLevel: " + std::to_string(iSignalLevel) + "\n";
				}
			}
			else
			{
				sResult = "No device found with name \"" + args["name"].asString() + "\"";
			}
		}
		else
		{
			int nIdx = jsonAsIdx(args["idx"]);
			auto result = m_sql.safe_query(
				"SELECT DS.Name, DS.HardwareID, H.Name, DS.DeviceID, DS.Type, DS.SubType, DS.nValue, DS.sValue, DS.LastUpdate, DS.Used, DS.BatteryLevel, DS.SignalLevel "
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
				int iBatt = atoi(row[10].c_str());
				if (iBatt != 255)
					sResult += "  BatteryLevel: " + std::to_string(iBatt) + "%\n";
				int iSignalLevel = atoi(row[11].c_str());
				if (iSignalLevel != 12)
					sResult += "  SignalLevel: " + std::to_string(iSignalLevel) + "\n";
			}
			else
			{
				sResult = "No device found with idx=" + std::to_string(nIdx);
			}
		}

		mcp::setToolResult(jsonRPCRep, sResult, !bFound);
		return true;
	}

	bool renameDevice(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool bHasIdx = args.isMember("idx");
		bool bHasName = args.isMember("name") && !args["name"].asString().empty();
		if ((!bHasIdx && !bHasName) || !args.isMember("new_name"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: renameDevice: Missing required parameters 'name' or 'new_name'");
			return false;
		}
		std::string sNewName = args["new_name"].asString();
		Json::Value device;
		std::string sOldName;
		bool bFound;
		if (bHasIdx)
		{
			bFound = getDeviceByIdx(jsonAsIdx(args["idx"]), device);
			sOldName = bFound ? device["Name"].asString() : "idx=" + std::to_string(jsonAsIdx(args["idx"]));
		}
		else
		{
			sOldName = args["name"].asString();
			bFound = getDeviceByName(sOldName, device);
		}
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

		mcp::setToolResult(jsonRPCRep, sResult, !bFound);
		return true;
	}

	bool deleteDevice(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool bHasIdx = args.isMember("idx");
		bool bHasName = args.isMember("name") && !args["name"].asString().empty();
		if (!bHasIdx && !bHasName)
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: deleteDevice: Missing required parameter 'name' or 'idx'");
			return false;
		}
		Json::Value device;
		std::string sName;
		bool bFound;
		if (bHasIdx)
		{
			bFound = getDeviceByIdx(jsonAsIdx(args["idx"]), device);
			sName = bFound ? device["Name"].asString() : "idx=" + std::to_string(jsonAsIdx(args["idx"]));
		}
		else
		{
			sName = args["name"].asString();
			bFound = getDeviceByName(sName, device);
		}
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

		mcp::setToolResult(jsonRPCRep, sResult, !bFound);
		return true;
	}

	// Resolves a sensor type string (name or numeric) to type/subtype. Returns false on failure.
	static bool resolveSensorType(const std::string &sSensorType, int &iType, int &iSubType, std::string &sError)
	{
		iType = -1; iSubType = -1;
		std::string sLower = sSensorType;
		stdlower(sLower);

		for (int i = 0; kSensorMap[i].name != nullptr; i++)
		{
			std::string sMapLower = kSensorMap[i].name;
			stdlower(sMapLower);
			if (sMapLower == sLower)
			{
				iType    = kSensorMap[i].type;
				iSubType = kSensorMap[i].subtype;
				return true;
			}
		}

		if (!sSensorType.empty() && std::isdigit((unsigned char)sSensorType[0]))
		{
			try
			{
				int nMapped = std::stoi(sSensorType);
				for (int i = 0; kSensorMap[i].name != nullptr; i++)
				{
					if (kSensorMap[i].mappedvalue == nMapped)
					{
						iType    = kSensorMap[i].type;
						iSubType = kSensorMap[i].subtype;
						return true;
					}
				}
			}
			catch (const std::exception &) {}
		}

		static const std::string sValidNames = []() {
			std::string names;
			for (int i = 0; kSensorMap[i].name != nullptr; i++)
				names += std::string(kSensorMap[i].name) + ", ";
			if (!names.empty())
				names.resize(names.size() - 2);
			return names;
		}();
		sError = "Unknown sensor type: \"" + sSensorType + "\". Valid types: " + sValidNames;
		return false;
	}

	// Shared creation logic used by both createSensor and createVirtualSensor.
	static bool doCreateVirtualSensor(int nHwIdx, const std::string &sSensorName, const std::string &sSensorType, Json::Value &jsonRPCRep)
	{
		int iType, iSubType;
		std::string sError;
		if (!resolveSensorType(sSensorType, iType, iSubType, sError))
		{
			mcp::setToolResult(jsonRPCRep, sError, true);
			return true;
		}

		auto maxResult = m_sql.safe_query("SELECT MAX(ID) FROM DeviceStatus");
		unsigned long nid = 1;
		if (!maxResult.empty() && !maxResult[0][0].empty())
			nid = atol(maxResult[0][0].c_str()) + 1;
		nid += 82000;

		bool bPrevAccept = m_sql.m_bAcceptNewHardware;
		m_sql.m_bAcceptNewHardware = true;
		std::string soptions;
		std::string sName = sSensorName;
		uint64_t DeviceRowIdx = m_sql.CreateDevice(nHwIdx, iType, iSubType, sName, nid, soptions, "MCP");
		m_sql.m_bAcceptNewHardware = bPrevAccept;

		bool bOK = (DeviceRowIdx != (uint64_t)-1);
		std::string sResult = bOK
			? "Virtual sensor \"" + sSensorName + "\" created successfully with idx=" + std::to_string(DeviceRowIdx) + " on hw_idx=" + std::to_string(nHwIdx) + "."
			: "Failed to create virtual sensor \"" + sSensorName + "\".";
		mcp::setToolResult(jsonRPCRep, sResult, !bOK);
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

		int nHwIdx = jsonRequest["params"]["arguments"]["hw_idx"].asInt();
		std::string sSensorName = jsonRequest["params"]["arguments"]["sensorname"].asString();
		std::string sSensorType = jsonRequest["params"]["arguments"]["sensortype"].asString();

		auto hwResult = m_sql.safe_query("SELECT ID FROM Hardware WHERE ID=%d", nHwIdx);
		if (hwResult.empty())
		{
			mcp::setToolResult(jsonRPCRep, "No hardware found with idx=" + std::to_string(nHwIdx), true);
			return true;
		}

		return doCreateVirtualSensor(nHwIdx, sSensorName, sSensorType, jsonRPCRep);
	}

	bool updateDeviceValue(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool bHasIdx = args.isMember("idx");
		bool bHasName = args.isMember("name") && !args["name"].asString().empty();
		if ((!bHasIdx && !bHasName) || !args.isMember("nvalue"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: updateDeviceValue: Missing required parameters 'name' or 'nvalue'");
			return false;
		}

		int nValue = args["nvalue"].asInt();
		std::string sValue;
		if (args.isMember("svalue"))
			sValue = args["svalue"].asString();

		Json::Value device;
		std::string sName;
		bool bFound;
		if (bHasIdx)
		{
			bFound = getDeviceByIdx(jsonAsIdx(args["idx"]), device);
			sName = bFound ? device["Name"].asString() : "idx=" + std::to_string(jsonAsIdx(args["idx"]));
		}
		else
		{
			sName = args["name"].asString();
			bFound = getDeviceByName(sName, device);
		}
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

		mcp::setToolResult(jsonRPCRep, sResult, !bFound);
		return true;
	}

	bool getSensorHistory(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool bHasIdx = args.isMember("idx");
		bool bHasName = args.isMember("name") && !args["name"].asString().empty();
		if (!bHasIdx && !bHasName)
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: getSensorHistory: Missing required parameter 'name' or 'idx'");
			return false;
		}

		Json::Value device;
		std::string sName;
		bool bFound;
		if (bHasIdx)
		{
			bFound = getDeviceByIdx(jsonAsIdx(args["idx"]), device);
			sName = bFound ? device["Name"].asString() : "idx=" + std::to_string(jsonAsIdx(args["idx"]));
		}
		else
		{
			sName = args["name"].asString();
			bFound = getDeviceByName(sName, device);
		}
		if (!bFound)
		{
			mcp::setToolResult(jsonRPCRep, "No device found with name \"" + sName + "\"", true);
			return true;
		}
		const uint64_t nIdx = (uint64_t)atoll(device["idx"].asString().c_str());

		// Scenes/groups are not in DeviceStatus — detect them first via the device JSON Type field.
		bool bIsSwitch = false;
		if (device.isMember("Type"))
		{
			const std::string sDevType = device["Type"].asString();
			if (sDevType == "Scene" || sDevType == "Group")
				bIsSwitch = true;
		}

		// For non-scene devices fetch dType/dSubType, then use IsLightOrSwitch().
		unsigned char dType    = 0;
		unsigned char dSubType = 0;
		if (!bIsSwitch)
		{
			auto devResult = m_sql.safe_query(
				"SELECT Type, SubType FROM DeviceStatus WHERE ID=%" PRIu64, nIdx);
			if (devResult.empty())
			{
				mcp::setToolResult(jsonRPCRep, "Device \"" + sName + "\" not found in database.", true);
				return true;
			}
			dType    = (unsigned char)atoi(devResult[0][0].c_str());
			dSubType = (unsigned char)atoi(devResult[0][1].c_str());
			bIsSwitch = IsLightOrSwitch(dType, dSubType);
		}

		if (bIsSwitch)
		{
			// --- LightingLog branch: switches and scenes ---
			std::string sResult;

			if (args.isMember("count") && !args["count"].isNull())
			{
				// Last N entries regardless of date
				int iCount = std::max(1, std::min(500, args["count"].asInt()));
				auto result = m_sql.safe_query(
					"SELECT Date, nValue, sValue, User FROM LightingLog "
					"WHERE DeviceRowID=%" PRIu64 " ORDER BY Date DESC LIMIT %d",
					nIdx, iCount);
				if (result.empty())
				{
					sResult = "No log entries found for \"" + sName + "\"";
				}
				else
				{
					sResult = "Last " + std::to_string((int)result.size()) +
					          " log entries for switch \"" + sName + "\":\n";
					for (const auto &row : result)
					{
						int nValue = atoi(row[1].c_str());
						std::string sState;
						if (nValue == 0)       sState = "Off";
						else if (nValue == 1)  sState = "On";
						else if (nValue == 2)  sState = "Toggle";
						else if (nValue == 9)  sState = "Dim to level";
						else                   sState = std::to_string(nValue);
						if (!row[2].empty())   sState += ": " + row[2];
						std::string sLine = row[0] + "  " + sState;
						if (!row[3].empty())   sLine += "  (user: " + row[3] + ")";
						sResult += sLine + "\n";
					}
				}
			}
			else
			{
				// Date range filter
				std::string szDateStart, szDateEnd;
				if (args.isMember("start_date") && args.isMember("end_date") &&
				    !args["start_date"].asString().empty() && !args["end_date"].asString().empty())
				{
					szDateStart = args["start_date"].asString();
					szDateEnd   = args["end_date"].asString();
				}
				else
				{
					int iDays = 7;
					if (args.isMember("days"))
						iDays = std::max(1, std::min(366, args["days"].asInt()));
					time_t now = mytime(nullptr);
					struct tm tmNow, tmStart;
					localtime_r(&now, &tmNow);
					time_t tStart = now - (time_t)(iDays - 1) * 86400LL;
					localtime_r(&tStart, &tmStart);
					char buf[16];
					strftime(buf, sizeof(buf), "%Y-%m-%d", &tmNow);
					szDateEnd = buf;
					strftime(buf, sizeof(buf), "%Y-%m-%d", &tmStart);
					szDateStart = buf;
				}
				auto result = m_sql.safe_query(
					"SELECT Date, nValue, sValue, User FROM LightingLog "
					"WHERE DeviceRowID=%" PRIu64 " AND Date>='%q' AND Date<='%q 23:59:59' "
					"ORDER BY Date DESC",
					nIdx, szDateStart.c_str(), szDateEnd.c_str());
				if (result.empty())
				{
					sResult = "No log entries found for \"" + sName + "\" in the specified period.";
				}
				else
				{
					sResult = std::to_string((int)result.size()) +
					          " log entries for switch \"" + sName + "\" (" +
					          szDateStart + " to " + szDateEnd + "):\n";
					for (const auto &row : result)
					{
						int nValue = atoi(row[1].c_str());
						std::string sState;
						if (nValue == 0)       sState = "Off";
						else if (nValue == 1)  sState = "On";
						else if (nValue == 2)  sState = "Toggle";
						else if (nValue == 9)  sState = "Dim to level";
						else                   sState = std::to_string(nValue);
						if (!row[2].empty())   sState += ": " + row[2];
						std::string sLine = row[0] + "  " + sState;
						if (!row[3].empty())   sLine += "  (user: " + row[3] + ")";
						sResult += sLine + "\n";
					}
				}
			}
			mcp::setToolResult(jsonRPCRep, sResult, false);
			return true;
		}

		// Map device type to sensor string (what HandleGraphCustomRange expects)
		std::string sSensor;
		if (dType == pTypeTEMP || dType == pTypeTEMP_HUM || dType == pTypeTEMP_HUM_BARO ||
		    dType == pTypeTEMP_BARO || dType == pTypeHUM ||
		    dType == pTypeSetpoint || dType == pTypeRego6XXTemp ||
		    dType == pTypeRadiator1 || dType == pTypeThermostat6)
			sSensor = "temp";
		else if (dType == pTypeRAIN)
			sSensor = "rain";
		else if (dType == pTypeWIND)
			sSensor = "wind";
		else if (dType == pTypeUV)
			sSensor = "uv";
		else if (dType == pTypeGeneral && dSubType == sTypePercentage)
			sSensor = "Percentage";
		else if (dType == pTypeGeneral && dSubType == sTypeFan)
			sSensor = "fan";
		else
			sSensor = "counter";

		// Compute date range
		std::string szDateStart, szDateEnd;
		if (args.isMember("start_date") && args.isMember("end_date") &&
		    !args["start_date"].asString().empty() && !args["end_date"].asString().empty())
		{
			szDateStart = args["start_date"].asString();
			szDateEnd   = args["end_date"].asString();
		}
		else
		{
			int iDays = 7;
			if (args.isMember("days"))
				iDays = std::max(1, std::min(366, args["days"].asInt()));

			time_t now = mytime(nullptr);
			struct tm tmNow, tmStart;
			localtime_r(&now, &tmNow);
			time_t tStart = now - (time_t)(iDays - 1) * 86400LL;
			localtime_r(&tStart, &tmStart);

			char buf[16];
			strftime(buf, sizeof(buf), "%Y-%m-%d", &tmNow);
			szDateEnd = buf;
			strftime(buf, sizeof(buf), "%Y-%m-%d", &tmStart);
			szDateStart = buf;
		}

		// Build range string "YYYY-MM-DDTYYYY-MM-DD"
		std::string sRange = szDateStart + "T" + szDateEnd;

		// Prepare output
		const char tempsign = m_sql.m_tempsign[0];
		const char* sTempUnit = (tempsign == 'F') ? "F" : "C";
		auto cvtTemp = [&](const std::string& s) -> std::string {
			if (s.empty()) return "";
			double v = atof(s.c_str());
			if (tempsign == 'F') v = v * 1.8 + 32.0;
			char buf[32];
			snprintf(buf, sizeof(buf), "%.1f", v);
			return buf;
		};

		std::string sResult;

		// ---- All sensor types: delegate to HandleGraphCustomRange ----
		http::server::request req;
		req.parameters.emplace("idx",       std::to_string(nIdx));
		req.parameters.emplace("sensor",    sSensor);
		req.parameters.emplace("range",     sRange);
		// For temperature: enable all sub-series
		req.parameters.emplace("graphTemp",  "true");
		req.parameters.emplace("graphHum",   "true");
		req.parameters.emplace("graphBaro",  "true");
		req.parameters.emplace("graphDew",   "true");
		req.parameters.emplace("graphChill", "true");

		// Build GraphContext (reads device from DB using idx+sensor from req)
		http::server::GraphContext ctx;
		if (!http::server::BuildGraphContext(req, m_sql, ctx))
		{
			mcp::setToolResult(jsonRPCRep, "Could not build graph context for device \"" + sName + "\".", true);
			return true;
		}

		// Call HandleGraphCustomRange (CWebServer& param is unused in that function)
		Json::Value root;
		http::server::CWebServer* pWS = m_webservers.GetAnyServer();
		if (!pWS)
		{
			mcp::setToolResult(jsonRPCRep, "Web server not available.", true);
			return true;
		}
		http::server::HandleGraphCustomRange(ctx, req, root, m_sql, *pWS);

		if (!root.isMember("result") || root["result"].empty())
		{
			std::string sMsg = "There is no graph history available for the \"" + sName + "\" sensor ";
			sMsg += "between " + szDateStart + " and " + szDateEnd + ".\n";
			sMsg += "This may mean graph logging is disabled for this device or no data has been recorded.\n";
			sMsg += "Tip: try a wider date range or increase 'days'.";
			mcp::setToolResult(jsonRPCRep, sMsg, true);
			return true;
		}

		// For pTypeSetpoint: check if the unit is a custom (non-temperature) unit.
		std::string sSetpointUnit;
		bool bSetpointIsTemp = true;
		if (dType == pTypeSetpoint)
		{
			auto opts = m_sql.GetDeviceOptions(std::to_string(nIdx));
			auto it = opts.find("ValueUnit");
			if (it != opts.end())
				sSetpointUnit = it->second;
			bSetpointIsTemp = sSetpointUnit.empty()
			               || sSetpointUnit == "\xc2\xb0""C" || sSetpointUnit == "\xc2\xb0""F"
			               || sSetpointUnit == "C" || sSetpointUnit == "F"
			               || sSetpointUnit == "°C" || sSetpointUnit == "°F";
			if (bSetpointIsTemp && sSetpointUnit.empty())
				sSetpointUnit = std::string("\xc2\xb0") + sTempUnit;
		}

		// Format the JSON result as human-readable text
		if (sSensor == "temp")
		{
			if (dType == pTypeSetpoint && !bSetpointIsTemp)
				sResult = "Daily setpoint history for \"" + sName + "\" (" + sSetpointUnit + " min/avg/max):\n";
			else
				sResult = "Daily temperature history for \"" + sName + "\" (\xc2\xb0" + sTempUnit + " min/avg/max):\n";
		}
		else if (sSensor == "rain")
			sResult = "Daily rain history for \"" + sName + "\" (mm):\n";
		else if (sSensor == "wind")
			sResult = "Daily wind history for \"" + sName + "\":\n";
		else if (sSensor == "uv")
			sResult = "Daily UV index history for \"" + sName + "\":\n";
		else if (sSensor == "Percentage")
			sResult = "Daily percentage history for \"" + sName + "\" (% min/avg/max):\n";
		else if (sSensor == "fan")
			sResult = "Daily fan speed history for \"" + sName + "\" (rpm min/max):\n";
		else
			sResult = "Daily history for \"" + sName + "\":\n";

		sResult += "Date        | Data\n";
		sResult += "------------|--------------------------------------\n";

		const Json::Value& results = root["result"];
		for (const auto& row : results)
		{
			std::string sDate = row.isMember("d") ? row["d"].asString().substr(0, 10) : "?";

			if (sSensor == "temp")
			{
				std::string sLine = sDate + " |";
				if (row.isMember("tm") && row.isMember("ta") && row.isMember("te"))
				{
					if (dType == pTypeSetpoint && !bSetpointIsTemp)
						sLine += " setpoint: min=" + row["tm"].asString() +
						         " avg=" + row["ta"].asString() +
						         " max=" + row["te"].asString() +
						         " " + sSetpointUnit;
					else
						sLine += " temp: min=" + cvtTemp(row["tm"].asString()) +
						         " avg=" + cvtTemp(row["ta"].asString()) +
						         " max=" + cvtTemp(row["te"].asString()) +
						         "\xc2\xb0" + sTempUnit;
				}
				if (row.isMember("hu"))
					sLine += "  hum=" + row["hu"].asString() + "%";
				if (row.isMember("ba"))
					sLine += "  baro=" + row["ba"].asString() + " hPa";
				if (row.isMember("dp"))
					sLine += "  dew=" + cvtTemp(row["dp"].asString()) + "\xc2\xb0" + sTempUnit;
				sResult += sLine + "\n";
			}
			else if (sSensor == "rain")
			{
				sResult += sDate + " | " + (row.isMember("mm") ? row["mm"].asString() : "0") + " mm\n";
			}
			else if (sSensor == "wind")
			{
				std::string sLine = sDate + " |";
				if (row.isMember("di")) sLine += " dir=" + row["di"].asString() + "\xc2\xb0";
				if (row.isMember("sp")) sLine += " speed=" + row["sp"].asString();
				if (row.isMember("gu")) sLine += " gust=" + row["gu"].asString();
				sResult += sLine + "\n";
			}
			else if (sSensor == "uv")
			{
				sResult += sDate + " | uvi=" + (row.isMember("uvi") ? row["uvi"].asString() : "?") + "\n";
			}
			else if (sSensor == "Percentage" || sSensor == "fan")
			{
				std::string sLine = sDate + " |";
				if (row.isMember("v_min")) sLine += " min=" + row["v_min"].asString();
				if (row.isMember("v_avg")) sLine += " avg=" + row["v_avg"].asString();
				if (row.isMember("v_max")) sLine += " max=" + row["v_max"].asString();
				sResult += sLine + "\n";
			}
			else // counter (includes P1, energy, gas, water, generic counters)
			{
				std::string sLine = sDate + " |";
				if (row.isMember("v1")) sLine += " usage=" + row["v1"].asString();
				if (row.isMember("v2")) sLine += " delivery=" + row["v2"].asString();
				if (!row.isMember("v1") && row.isMember("v")) sLine += " value=" + row["v"].asString();
				sResult += sLine + "\n";
			}
		}

		mcp::setToolResult(jsonRPCRep, sResult, false);
		return true;
	}

	bool getSensorShortLog(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool bHasIdx  = args.isMember("idx");
		bool bHasName = args.isMember("name") && !args["name"].asString().empty();
		if (!bHasIdx && !bHasName)
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: getSensorShortLog: Missing required parameter 'name' or 'idx'");
			return false;
		}

		Json::Value device;
		std::string sName;
		bool bFound;
		if (bHasIdx)
		{
			bFound = getDeviceByIdx(jsonAsIdx(args["idx"]), device);
			sName  = bFound ? device["Name"].asString() : "idx=" + std::to_string(jsonAsIdx(args["idx"]));
		}
		else
		{
			sName  = args["name"].asString();
			bFound = getDeviceByName(sName, device);
		}
		if (!bFound)
		{
			mcp::setToolResult(jsonRPCRep, "No device found with name \"" + sName + "\"", true);
			return true;
		}
		const uint64_t nIdx = (uint64_t)atoll(device["idx"].asString().c_str());

		auto devResult = m_sql.safe_query(
			"SELECT Type, SubType FROM DeviceStatus WHERE ID=%" PRIu64, nIdx);
		if (devResult.empty())
		{
			mcp::setToolResult(jsonRPCRep, "Device \"" + sName + "\" not found in database.", true);
			return true;
		}
		const unsigned char dType    = (unsigned char)atoi(devResult[0][0].c_str());
		const unsigned char dSubType = (unsigned char)atoi(devResult[0][1].c_str());

		if (IsLightOrSwitch(dType, dSubType))
		{
			mcp::setToolResult(jsonRPCRep,
				"\"" + sName + "\" is a switch/light device. Use get_sensor_history to retrieve its log.", true);
			return true;
		}

		// Map device type to sensor string
		std::string sSensor;
		if (dType == pTypeTEMP || dType == pTypeTEMP_HUM || dType == pTypeTEMP_HUM_BARO ||
		    dType == pTypeTEMP_BARO || dType == pTypeHUM ||
		    dType == pTypeSetpoint || dType == pTypeRego6XXTemp ||
		    dType == pTypeRadiator1 || dType == pTypeThermostat6)
			sSensor = "temp";
		else if (dType == pTypeRAIN)
			sSensor = "rain";
		else if (dType == pTypeWIND)
			sSensor = "wind";
		else if (dType == pTypeUV)
			sSensor = "uv";
		else if (dType == pTypeGeneral && dSubType == sTypePercentage)
			sSensor = "Percentage";
		else if (dType == pTypeGeneral && dSubType == sTypeFan)
			sSensor = "fan";
		else
			sSensor = "counter";

		// Map sensor string to short-log table
		std::string dbasetable;
		if (sSensor == "temp")
			dbasetable = "Temperature";
		else if (sSensor == "rain")
			dbasetable = "Rain";
		else if (sSensor == "wind")
			dbasetable = "Wind";
		else if (sSensor == "uv")
			dbasetable = "UV";
		else if (sSensor == "Percentage")
			dbasetable = "Percentage";
		else if (sSensor == "fan")
			dbasetable = "Fan";
		else
		{
			// counter: P1Power / CURRENT / CURRENTENERGY use MultiMeter, others use Meter
			if (dType == pTypeP1Power || dType == pTypeCURRENT || dType == pTypeCURRENTENERGY)
				dbasetable = "MultiMeter";
			else
				dbasetable = "Meter";
		}

		const char tempsign   = m_sql.m_tempsign[0];
		const char *sTempUnit = (tempsign == 'F') ? "F" : "C";
		auto cvtTemp = [&](const std::string &s) -> std::string {
			if (s.empty()) return "";
			double v = atof(s.c_str());
			if (tempsign == 'F') v = v * 1.8 + 32.0;
			char buf[32];
			snprintf(buf, sizeof(buf), "%.1f", v);
			return buf;
		};

		// Build explicit column list per table — never use SELECT * so column order is guaranteed.
		// DeviceRowID is excluded; Date is always last.
		// Column layout:
		//   Temperature : Temperature, Chill, Humidity, Barometer, DewPoint, SetPoint, Date  (7 cols, indices 0-6)
		//   Rain        : Total, Rate, Date                                                   (3 cols, indices 0-2)
		//   Wind        : Direction, Speed, Gust, Date                                        (4 cols, indices 0-3)
		//   UV          : Level, Date                                                         (2 cols, indices 0-1)
		//   Percentage  : Percentage, Date                                                    (2 cols, indices 0-1)
		//   Fan         : Speed, Date                                                         (2 cols, indices 0-1)
		//   Meter       : Value, Usage, Price, Date                                           (4 cols, indices 0-3)
		//   MultiMeter  : Value1,Value2,Value3,Value4,Value5,Value6, Price, Date              (8 cols, indices 0-7)
		std::string sColumns;
		if      (dbasetable == "Temperature") sColumns = "Temperature, Chill, Humidity, Barometer, DewPoint, SetPoint, Date";
		else if (dbasetable == "Rain")        sColumns = "Total, Rate, Date";
		else if (dbasetable == "Wind")        sColumns = "Direction, Speed, Gust, Date";
		else if (dbasetable == "UV")          sColumns = "Level, Date";
		else if (dbasetable == "Percentage")  sColumns = "Percentage, Date";
		else if (dbasetable == "Fan")         sColumns = "Speed, Date";
		else if (dbasetable == "Meter")       sColumns = "Value, [Usage], Price, Date";
		else /* MultiMeter */                 sColumns = "Value1, Value2, Value3, Value4, Value5, Value6, Price, Date";

		std::vector<std::vector<std::string>> result;
		std::string sWindowDesc;

		// dbasetable and sColumns are derived from internal device-type logic, never user input.
		if (args.isMember("count") && !args["count"].isNull())
		{
			int iCount = std::max(1, std::min(1000, args["count"].asInt()));
			std::string sQ = "SELECT " + sColumns + " FROM [" + dbasetable + "] WHERE DeviceRowID=%" PRIu64 " ORDER BY Date DESC LIMIT %d";
			result = m_sql.safe_query(sQ.c_str(), nIdx, iCount);
			std::reverse(result.begin(), result.end());
			sWindowDesc = "last " + std::to_string(iCount) + " readings";
		}
		else
		{
			int iHours = 24;
			if (args.isMember("hours") && !args["hours"].isNull())
				iHours = std::max(1, std::min(168, args["hours"].asInt()));
			std::string sQ = "SELECT " + sColumns + " FROM [" + dbasetable + "] WHERE DeviceRowID=%" PRIu64
			                 " AND Date >= datetime('now','localtime','-%d hours') ORDER BY Date ASC";
			result = m_sql.safe_query(sQ.c_str(), nIdx, iHours);
			sWindowDesc = "last " + std::to_string(iHours) + " hour(s)";
		}

		_log.Debug(DEBUG_WEBSERVER, "MCP: getSensorShortLog: device='%s' table='%s' rows=%d",
			sName.c_str(), dbasetable.c_str(), (int)result.size());

		if (result.empty())
		{
			mcp::setToolResult(jsonRPCRep,
				"No short-log data found for \"" + sName + "\" in the " + sWindowDesc + ".\n"
				"Short-log data is only kept for a configurable number of days (default: 1 day).", false);
			return true;
		}

		// For pTypeSetpoint: read ValueUnit from device options to decide whether
		// the value is a temperature (apply conversion + °unit) or a custom unit (show raw).
		std::string sSetpointUnit;
		bool bSetpointIsTemp = true;
		if (dType == pTypeSetpoint)
		{
			auto opts = m_sql.GetDeviceOptions(std::to_string(nIdx));
			auto it = opts.find("ValueUnit");
			if (it != opts.end())
				sSetpointUnit = it->second;
			// Treat as temperature only if unit is empty or explicitly a temperature unit
			bSetpointIsTemp = sSetpointUnit.empty()
			               || sSetpointUnit == "\xc2\xb0""C" || sSetpointUnit == "\xc2\xb0""F"
			               || sSetpointUnit == "C" || sSetpointUnit == "F"
			               || sSetpointUnit == "°C" || sSetpointUnit == "°F";
			if (bSetpointIsTemp && sSetpointUnit.empty())
				sSetpointUnit = std::string("\xc2\xb0") + sTempUnit;
		}

		std::string sResult = std::to_string((int)result.size()) + " short-log readings for \"" +
		                      sName + "\" (" + sWindowDesc + "):\n";
		sResult += "Each line: timestamp (YYYY-MM-DD HH:MM:SS) followed by key=value pairs.\n";
		sResult += "Timestamp           | Data\n";
		sResult += "--------------------|--------------------------------------\n";

		for (const auto &row : result)
		{
			// Column layout per sColumns (DeviceRowID is NOT selected):
			// Temperature: 0=Temp, 1=Chill, 2=Hum, 3=Baro, 4=Dew, 5=SetPoint, 6=Date
			// Rain:        0=Total, 1=Rate, 2=Date
			// Wind:        0=Dir, 1=Speed, 2=Gust, 3=Date
			// UV:          0=Level, 1=Date
			// Percentage:  0=Percentage, 1=Date
			// Fan:         0=Speed, 1=Date
			// Meter:       0=Value, 1=Usage, 2=Price, 3=Date
			// MultiMeter:  0-5=Value1-6, 6=Price, 7=Date
			// row.back() is always the Date string.
			const std::string &sDate = row.back();
			std::string sLine = sDate + " |";

			if (dbasetable == "Temperature")
			{
				// row: 0=Temp, 1=Chill, 2=Hum, 3=Baro, 4=Dew, 5=SetPoint, 6=Date
				if (row.size() >= 7)
				{
					const bool bIsSetpoint = (dType == pTypeSetpoint || dType == pTypeRego6XXTemp ||
					                          dType == pTypeRadiator1 || dType == pTypeThermostat6);
					const std::string &sTempVal  = row[0];
					const std::string &sSetPtVal = row[5];
					const bool bHasTemp  = !sTempVal.empty()  && sTempVal  != "0" && sTempVal  != "0.00";
					const bool bHasSetPt = !sSetPtVal.empty() && sSetPtVal != "0" && sSetPtVal != "0.00";
					if (bIsSetpoint)
					{
						if (bHasTemp)
						{
							if (bSetpointIsTemp)
								sLine += " setpoint=" + cvtTemp(sTempVal) + sSetpointUnit;
							else
								sLine += " setpoint=" + sTempVal + " " + sSetpointUnit;
						}
					}
					else if (dType == pTypeTEMP || dType == pTypeTEMP_HUM || dType == pTypeTEMP_HUM_BARO || dType == pTypeTEMP_BARO)
					{
						if (bHasTemp)
							sLine += " temp=" + cvtTemp(sTempVal) + "\xc2\xb0" + sTempUnit;
					}
					else
					{
						if (bHasTemp)  sLine += " temp=" + cvtTemp(sTempVal) + "\xc2\xb0" + sTempUnit;
						if (bHasSetPt) sLine += "  setpoint=" + cvtTemp(sSetPtVal) + "\xc2\xb0" + sTempUnit;
					}
					if (dType == pTypeTEMP_HUM || dType == pTypeTEMP_HUM_BARO || dType == pTypeHUM)
						sLine += "  hum=" + row[2] + "%";
					if (dType == pTypeTEMP_HUM_BARO || dType == pTypeTEMP_BARO)
						sLine += "  baro=" + row[3] + " hPa";
					if (dType == pTypeTEMP || dType == pTypeTEMP_HUM || dType == pTypeTEMP_HUM_BARO || dType == pTypeTEMP_BARO)
						if (!row[4].empty() && row[4] != "0")
							sLine += "  dew=" + cvtTemp(row[4]) + "\xc2\xb0" + sTempUnit;
				}
			}
			else if (dbasetable == "Rain")
			{
				// row: 0=Total, 1=Rate, 2=Date
				if (row.size() >= 3)
					sLine += " total=" + row[0] + " mm  rate=" + row[1];
			}
			else if (dbasetable == "Wind")
			{
				// row: 0=Direction, 1=Speed, 2=Gust, 3=Date
				if (row.size() >= 4)
				{
					double spd  = atof(row[1].c_str()) / 10.0;
					double gust = atof(row[2].c_str()) / 10.0;
					char buf[64];
					snprintf(buf, sizeof(buf), " dir=%.0f\xc2\xb0  speed=%.1f m/s  gust=%.1f m/s",
						atof(row[0].c_str()), spd, gust);
					sLine += buf;
				}
			}
			else if (dbasetable == "UV")
			{
				// row: 0=Level, 1=Date
				if (row.size() >= 2)
					sLine += " uvi=" + row[0];
			}
			else if (dbasetable == "Percentage")
			{
				// row: 0=Percentage, 1=Date
				if (row.size() >= 2)
					sLine += " pct=" + row[0] + "%";
			}
			else if (dbasetable == "Fan")
			{
				// row: 0=Speed, 1=Date
				if (row.size() >= 2)
					sLine += " speed=" + row[0] + " rpm";
			}
			else if (dbasetable == "Meter")
			{
				// row: 0=Value, 1=Usage, 2=Price, 3=Date
				if (row.size() >= 4)
					sLine += " value=" + row[0];
			}
			else if (dbasetable == "MultiMeter")
			{
				// row: 0=Value1..5=Value6, 6=Price, 7=Date
				if (row.size() >= 8)
				{
					const char *labels[] = { "v1", "v2", "v3", "v4", "v5", "v6" };
					for (int i = 0; i < 6; ++i)
					{
						// Zero is a valid meter reading (e.g. no solar generation, no return feed).
						if (!row[i].empty())
							sLine += std::string("  ") + labels[i] + "=" + row[i];
					}
				}
			}

			sResult += sLine + "\n";
		}

		mcp::setToolResult(jsonRPCRep, sResult, false);
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

		mcp::setToolResult(jsonRPCRep, sResult, false);
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
			mcp::setToolResult(jsonRPCRep, "Invalid variable type " + std::to_string(vtype) + ". Must be 0-4 (Integer/Float/String/Date/Time).", true);
			return true;
		}

		// Check for duplicate name
		auto existing = m_sql.safe_query("SELECT ID FROM UserVariables WHERE Name='%q'", sName.c_str());
		if (!existing.empty())
		{
			mcp::setToolResult(jsonRPCRep, "A variable named \"" + sName + "\" already exists (idx=" + existing[0][0] + ").", true);
			return true;
		}

		std::string errorMessage;
		bool bOK = m_sql.AddUserVariable(sName, (_eUsrVariableType)vtype, sValue, errorMessage);
		if (bOK)
			m_mainworker.m_eventsystem.GetCurrentUserVariables();

		std::string sResult = bOK
			? "User variable \"" + sName + "\" created successfully."
			: "Failed to create variable \"" + sName + "\": " + errorMessage;
		mcp::setToolResult(jsonRPCRep, sResult, !bOK);
		return true;
	}

	bool updateUserVariable(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool bHasVarId = args.isMember("variable_id");
		bool bHasName = args.isMember("name") && !args["name"].asString().empty();
		if ((!bHasVarId && !bHasName) || !args.isMember("value"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: updateUserVariable: Missing required parameters 'name' or 'value'");
			return false;
		}

		std::string sValue = args["value"].asString();

		std::vector<std::vector<std::string>> result;
		std::string sIdentifier;
		if (bHasVarId)
		{
			int nVarId = args["variable_id"].asInt();
			sIdentifier = "idx=" + std::to_string(nVarId);
			result = m_sql.safe_query("SELECT ID, Name, ValueType FROM UserVariables WHERE ID=%d", nVarId);
		}
		else
		{
			std::string sName = args["name"].asString();
			sIdentifier = "\"" + sName + "\"";
			result = m_sql.safe_query("SELECT ID, Name, ValueType FROM UserVariables WHERE Name='%q'", sName.c_str());
		}

		if (result.empty())
		{
			mcp::setToolResult(jsonRPCRep, "No user variable found with " + sIdentifier, true);
			return true;
		}

		std::string sIdx = result[0][0];
		std::string sName = result[0][1];
		int currentType  = atoi(result[0][2].c_str());
		int newType      = currentType;
		if (args.isMember("vtype"))
			newType = args["vtype"].asInt();

		std::string errorMessage;
		bool bOK = m_sql.UpdateUserVariable(sIdx, sName, (_eUsrVariableType)newType, sValue, (newType == currentType), errorMessage);
		if (bOK)
			m_mainworker.m_eventsystem.GetCurrentUserVariables();

		std::string sResult = bOK
			? "User variable \"" + sName + "\" updated successfully."
			: "Failed to update variable \"" + sName + "\": " + errorMessage;
		mcp::setToolResult(jsonRPCRep, sResult, !bOK);
		return true;
	}

	bool deleteUserVariable(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool bHasVarId = args.isMember("variable_id");
		bool bHasName = args.isMember("name") && !args["name"].asString().empty();
		if (!bHasVarId && !bHasName)
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: deleteUserVariable: Missing required parameter 'name' or 'variable_id'");
			return false;
		}

		std::vector<std::vector<std::string>> result;
		std::string sIdentifier;
		if (bHasVarId)
		{
			int nVarId = args["variable_id"].asInt();
			sIdentifier = "idx=" + std::to_string(nVarId);
			result = m_sql.safe_query("SELECT ID, Name FROM UserVariables WHERE ID=%d", nVarId);
		}
		else
		{
			std::string sName = args["name"].asString();
			sIdentifier = "\"" + sName + "\"";
			result = m_sql.safe_query("SELECT ID, Name FROM UserVariables WHERE Name='%q'", sName.c_str());
		}

		if (result.empty())
		{
			mcp::setToolResult(jsonRPCRep, "No user variable found with " + sIdentifier, true);
			return true;
		}

		std::string sIdx = result[0][0];
		std::string sName = result[0][1];
		m_sql.DeleteUserVariable(sIdx);
		m_mainworker.m_eventsystem.GetCurrentUserVariables();

		mcp::setToolResult(jsonRPCRep, "User variable \"" + sName + "\" deleted successfully.", false);
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

		const size_t kMaxLogMessageLen = 2048;
		if (sMessage.size() > kMaxLogMessageLen)
		{
			sMessage.resize(kMaxLogMessageLen);
			sMessage += "... [truncated]";
		}

		std::string sLevel   = "normal";
		if (jsonRequest["params"]["arguments"].isMember("level"))
			sLevel = jsonRequest["params"]["arguments"]["level"].asString();

		if (sLevel == "error")
			_log.Log(LOG_ERROR, "MCP: %s", sMessage.c_str());
		else if (sLevel == "status")
			_log.Log(LOG_STATUS, "MCP: %s", sMessage.c_str());
		else
			_log.Log(LOG_NORM, "MCP: %s", sMessage.c_str());

		mcp::setToolResult(jsonRPCRep, "Log message written at level \"" + sLevel + "\": " + sMessage, false);
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
		mcp::setToolResult(jsonRPCRep, sResult, !bOK);
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

		mcp::setToolResult(jsonRPCRep, sResult, false);
		return true;
	}

	bool getEvent(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool hasName = args.isMember("event_name") && !args["event_name"].asString().empty();
		bool hasId   = args.isMember("event_id");

		if (!hasName && !hasId)
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: getEvent: Missing required parameter 'event_name' or 'event_id'");
			return false;
		}

		std::vector<std::vector<std::string>> result;
		std::string sIdentifier;
		if (hasId)
		{
			int nId = args["event_id"].asInt();
			sIdentifier = "idx=" + std::to_string(nId);
			result = m_sql.safe_query(
				"SELECT ID, Name, XMLStatement, Status, Interpreter FROM EventMaster WHERE ID=%d", nId);
		}
		else
		{
			sIdentifier = "\"" + args["event_name"].asString() + "\"";
			result = m_sql.safe_query(
				"SELECT ID, Name, XMLStatement, Status, Interpreter FROM EventMaster WHERE Name='%q'",
				args["event_name"].asString().c_str());
		}

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
			sResult = "No event script found with " + sIdentifier;
		}

		mcp::setToolResult(jsonRPCRep, sResult, !bFound);
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
			mcp::setToolResult(jsonRPCRep, "Invalid interpreter \"" + sInterpreter + "\". Must be one of: Lua, dzVents, Python, Blockly.", true);
			return true;
		}

		// Check for duplicate name
		auto existing = m_sql.safe_query("SELECT ID FROM EventMaster WHERE Name='%q'", sName.c_str());
		if (!existing.empty())
		{
			mcp::setToolResult(jsonRPCRep, "An event script named \"" + sName + "\" already exists (idx=" + existing[0][0] + ").", true);
			return true;
		}

		m_sql.safe_query(
			"INSERT INTO EventMaster (Name, Interpreter, Type, XMLStatement, Status) VALUES ('%q','%q','All','%q',%d)",
			sName.c_str(), sInterpreter.c_str(), sCode.c_str(), bEnabled ? 1 : 0);

		// Get the new idx
		auto newResult = m_sql.safe_query("SELECT ID FROM EventMaster WHERE Name='%q'", sName.c_str());
		std::string sNewIdx = newResult.empty() ? "?" : newResult[0][0];

		m_mainworker.m_eventsystem.LoadEvents();

		mcp::setToolResult(jsonRPCRep, "Event script \"" + sName + "\" created successfully (idx=" + sNewIdx + ", interpreter=" + sInterpreter + ", " + (bEnabled ? "enabled" : "disabled") + ").", false);
		return true;
	}

	bool updateEvent(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool hasName = args.isMember("event_name") && !args["event_name"].asString().empty();
		bool hasId   = args.isMember("event_id");

		if (!hasName && !hasId)
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: updateEvent: Missing required parameter 'event_name' or 'event_id'");
			return false;
		}

		std::vector<std::vector<std::string>> result;
		std::string sIdentifier;
		if (hasId)
		{
			int nId = args["event_id"].asInt();
			sIdentifier = "idx=" + std::to_string(nId);
			result = m_sql.safe_query("SELECT ID, Name FROM EventMaster WHERE ID=%d", nId);
		}
		else
		{
			sIdentifier = "\"" + args["event_name"].asString() + "\"";
			result = m_sql.safe_query("SELECT ID, Name FROM EventMaster WHERE Name='%q'", args["event_name"].asString().c_str());
		}

		if (result.empty())
		{
			mcp::setToolResult(jsonRPCRep, "No event script found with " + sIdentifier, true);
			return true;
		}

		int nIdx = atoi(result[0][0].c_str());
		std::string sEventName = result[0][1];
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
		mcp::setToolResult(jsonRPCRep, sResult, false);
		return true;
	}

	bool deleteEvent(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool hasName = args.isMember("event_name") && !args["event_name"].asString().empty();
		bool hasId   = args.isMember("event_id");

		if (!hasName && !hasId)
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: deleteEvent: Missing required parameter 'event_name' or 'event_id'");
			return false;
		}

		std::vector<std::vector<std::string>> result;
		std::string sIdentifier;
		if (hasId)
		{
			int nId = args["event_id"].asInt();
			sIdentifier = "idx=" + std::to_string(nId);
			result = m_sql.safe_query("SELECT ID, Name FROM EventMaster WHERE ID=%d", nId);
		}
		else
		{
			sIdentifier = "\"" + args["event_name"].asString() + "\"";
			result = m_sql.safe_query("SELECT ID, Name FROM EventMaster WHERE Name='%q'", args["event_name"].asString().c_str());
		}

		if (result.empty())
		{
			mcp::setToolResult(jsonRPCRep, "No event script found with " + sIdentifier, true);
			return true;
		}

		int nIdx = atoi(result[0][0].c_str());
		std::string sEventName = result[0][1];
		m_sql.safe_query("DELETE FROM EventMaster WHERE ID=%d", nIdx);
		m_mainworker.m_eventsystem.LoadEvents();

		mcp::setToolResult(jsonRPCRep, "Event script \"" + sEventName + "\" (idx=" + std::to_string(nIdx) + ") permanently deleted.", false);
		return true;
	}

	// =========================================================================
	// =========================================================================

	bool setSwitchState(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool bHasIdx = args.isMember("idx");
		bool bHasName = args.isMember("switchname") && !args["switchname"].asString().empty();
		if ((!bHasIdx && !bHasName) || !args.isMember("state"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: setSwitchState: Missing required parameter 'switchname' or 'state'");
			return false;
		}
		std::string sState = args["state"].asString();

		// Capitalize first letter, lowercase rest — matches "On"/"Off" format
		if (!sState.empty()) {
			sState[0] = toupper(sState[0]);
			for (size_t i = 1; i < sState.size(); i++)
				sState[i] = tolower(sState[i]);
		}

		if (sState != "On" && sState != "Off")
		{
			mcp::setToolResult(jsonRPCRep, "Invalid state '" + sState + "'. Must be 'On' or 'Off'.", true);
			return true;
		}

		Json::Value device;
		std::string sSwitchName;
		bool bFound;
		if (bHasIdx)
		{
			bFound = getDeviceByIdx(jsonAsIdx(args["idx"]), device);
			sSwitchName = bFound ? device["Name"].asString() : "idx=" + std::to_string(jsonAsIdx(args["idx"]));
		}
		else
		{
			sSwitchName = args["switchname"].asString();
			bFound = getDeviceByName(sSwitchName, device);
		}
		std::string sResult;
		if (!bFound)
		{
			sResult = "No switch exists with the name \"" + sSwitchName + "\".";
		}
		else
		{
			SendProgress(tl_progressCtx.sid, tl_progressCtx.progressToken, 0, 2, "Sending command to device");
			auto rc = m_mainworker.SwitchLight(device["idx"].asString(), sState, "", "", "", 0, "MCP");
			if (rc == MainWorker::eSwitchLightReturnCode::SL_ERROR)
				sResult = "Error setting switch \"" + sSwitchName + "\" to " + sState + ".";
			else if (rc == MainWorker::eSwitchLightReturnCode::SL_OK_NO_ACTION)
				sResult = "Switch \"" + sSwitchName + "\" was already " + sState + ". No action taken.";
			else
				sResult = "Switch \"" + sSwitchName + "\" set to " + sState + " successfully.";
			SendProgress(tl_progressCtx.sid, tl_progressCtx.progressToken, 2, 2, "Command sent");
		}
		mcp::setToolResult(jsonRPCRep, sResult, !bFound);
		return true;
	}

	bool setDimmerLevel(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool bHasIdx = args.isMember("idx");
		bool bHasName = args.isMember("switchname") && !args["switchname"].asString().empty();
		if ((!bHasIdx && !bHasName) || !args.isMember("level"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: setDimmerLevel: Missing required parameter 'switchname' or 'level'");
			return false;
		}
		int iLevel = args["level"].asInt();
		iLevel = std::max(0, std::min(100, iLevel));

		Json::Value device;
		std::string sSwitchName;
		bool bFound;
		if (bHasIdx)
		{
			bFound = getDeviceByIdx(jsonAsIdx(args["idx"]), device);
			sSwitchName = bFound ? device["Name"].asString() : "idx=" + std::to_string(jsonAsIdx(args["idx"]));
		}
		else
		{
			sSwitchName = args["switchname"].asString();
			bFound = getDeviceByName(sSwitchName, device);
		}
		std::string sResult;
		if (!bFound)
		{
			sResult = "No device exists with the name \"" + sSwitchName + "\".";
		}
		else
		{
			SendProgress(tl_progressCtx.sid, tl_progressCtx.progressToken, 0, 2, "Sending command to device");
			auto rc = m_mainworker.SwitchLight(device["idx"].asString(), "Set Level", std::to_string(iLevel), "", "", 0, "MCP");
			sResult = (rc == MainWorker::eSwitchLightReturnCode::SL_ERROR)
				? "Error setting dimmer level on \"" + sSwitchName + "\"."
				: "Dimmer \"" + sSwitchName + "\" set to level " + std::to_string(iLevel) + ".";
			SendProgress(tl_progressCtx.sid, tl_progressCtx.progressToken, 2, 2, "Command sent");
		}
		mcp::setToolResult(jsonRPCRep, sResult, !bFound);
		return true;
	}

	bool controlBlinds(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool bHasIdx = args.isMember("idx");
		bool bHasName = args.isMember("switchname") && !args["switchname"].asString().empty();
		if ((!bHasIdx && !bHasName) || !args.isMember("command"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: controlBlinds: Missing required parameter 'switchname' or 'command'");
			return false;
		}
		std::string sCommand = args["command"].asString();

		// Capitalize first letter, lowercase rest — matches "Open"/"Close"/"Stop" format
		if (!sCommand.empty()) {
			sCommand[0] = toupper(sCommand[0]);
			for (size_t i = 1; i < sCommand.size(); i++)
				sCommand[i] = tolower(sCommand[i]);
		}

		if (sCommand != "Open" && sCommand != "Close" && sCommand != "Stop")
		{
			mcp::setToolResult(jsonRPCRep, "Invalid command '" + sCommand + "'. Must be 'Open', 'Close', or 'Stop'.", true);
			return true;
		}

		Json::Value device;
		std::string sSwitchName;
		bool bFound;
		if (bHasIdx)
		{
			bFound = getDeviceByIdx(jsonAsIdx(args["idx"]), device);
			sSwitchName = bFound ? device["Name"].asString() : "idx=" + std::to_string(jsonAsIdx(args["idx"]));
		}
		else
		{
			sSwitchName = args["switchname"].asString();
			bFound = getDeviceByName(sSwitchName, device);
		}
		std::string sResult;
		if (!bFound)
		{
			sResult = "No device exists with the name \"" + sSwitchName + "\".";
		}
		else
		{
			SendProgress(tl_progressCtx.sid, tl_progressCtx.progressToken, 0, 2, "Sending command to device");
			auto rc = m_mainworker.SwitchLight(device["idx"].asString(), sCommand, "", "", "", 0, "MCP");
			sResult = (rc == MainWorker::eSwitchLightReturnCode::SL_ERROR)
				? "Error sending " + sCommand + " to \"" + sSwitchName + "\"."
				: "Command " + sCommand + " sent to \"" + sSwitchName + "\" successfully.";
			SendProgress(tl_progressCtx.sid, tl_progressCtx.progressToken, 2, 2, "Command sent");
		}
		mcp::setToolResult(jsonRPCRep, sResult, !bFound);
		return true;
	}

	bool setColorBrightness(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool bHasIdx = args.isMember("idx");
		bool bHasName = args.isMember("switchname") && !args["switchname"].asString().empty();
		if ((!bHasIdx && !bHasName) || !args.isMember("hue") || !args.isMember("brightness"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: setColorBrightness: Missing required parameters");
			return false;
		}
		float fHue = (float)args["hue"].asInt();
		int iBrightness = args["brightness"].asInt();
		bool bIsWhite = false;
		if (args.isMember("iswhite"))
			bIsWhite = args["iswhite"].asBool();

		iBrightness = std::max(0, std::min(100, iBrightness));
		fHue = std::max(0.0F, std::min(360.0F, fHue));

		Json::Value device;
		std::string sSwitchName;
		bool bFound;
		uint64_t uIdx = 0;
		if (bHasIdx)
		{
			bFound = getDeviceByIdx(jsonAsIdx(args["idx"]), device);
			sSwitchName = bFound ? device["Name"].asString() : "idx=" + std::to_string(jsonAsIdx(args["idx"]));
			uIdx = (uint64_t)jsonAsIdx(args["idx"]);
		}
		else
		{
			sSwitchName = args["switchname"].asString();
			bFound = getDeviceByName(sSwitchName, device);
			if (bFound)
			{
				try {
					uIdx = std::stoull(device["idx"].asString());
				} catch (const std::exception &e) {
					_log.Debug(DEBUG_WEBSERVER, "MCP: Invalid device idx: %s", e.what());
					mcp::setToolResult(jsonRPCRep, "Internal error: invalid device index.", true);
					return true;
				}
			}
		}
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

			SendProgress(tl_progressCtx.sid, tl_progressCtx.progressToken, 0, 2, "Sending command to device");
			auto rc = m_mainworker.SwitchLight(uIdx, "Set Color", (unsigned char)iBrightness, color, false, 0, "MCP");
			sResult = (rc == MainWorker::eSwitchLightReturnCode::SL_ERROR)
				? "Error setting color on \"" + sSwitchName + "\"."
				: "Color set on \"" + sSwitchName + "\": hue=" + std::to_string((int)fHue) + ", brightness=" + std::to_string(iBrightness) + ".";
			SendProgress(tl_progressCtx.sid, tl_progressCtx.progressToken, 2, 2, "Command sent");
		}
		mcp::setToolResult(jsonRPCRep, sResult, !bFound);
		return true;
	}

	bool setColorTemperature(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool bHasIdx = args.isMember("idx");
		bool bHasName = args.isMember("switchname") && !args["switchname"].asString().empty();
		if ((!bHasIdx && !bHasName) || !args.isMember("kelvin"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: setColorTemperature: Missing required parameters");
			return false;
		}
		int iKelvin = args["kelvin"].asInt();

		// Map Kelvin to Domoticz ColorModeTemp level (0-100):
		// 6500K (cool/daylight) maps to level 0, 2700K (warm/incandescent) maps to level 100.
		// Range is clamped to the typical tunable-white range supported by Domoticz (2700K-6500K).
		iKelvin = std::max(2700, std::min(6500, iKelvin));
		double dLevel = 100.0 * (double)(6500 - iKelvin) / (double)(6500 - 2700);
		int iLevel = (int)round(dLevel);
		iLevel = std::max(0, std::min(100, iLevel));

		Json::Value device;
		std::string sSwitchName;
		bool bFound;
		uint64_t uIdx = 0;
		if (bHasIdx)
		{
			bFound = getDeviceByIdx(jsonAsIdx(args["idx"]), device);
			sSwitchName = bFound ? device["Name"].asString() : "idx=" + std::to_string(jsonAsIdx(args["idx"]));
			uIdx = (uint64_t)jsonAsIdx(args["idx"]);
		}
		else
		{
			sSwitchName = args["switchname"].asString();
			bFound = getDeviceByName(sSwitchName, device);
			if (bFound)
			{
				try {
					uIdx = std::stoull(device["idx"].asString());
				} catch (const std::exception &e) {
					_log.Debug(DEBUG_WEBSERVER, "MCP: Invalid device idx: %s", e.what());
					mcp::setToolResult(jsonRPCRep, "Internal error: invalid device index.", true);
					return true;
				}
			}
		}
		std::string sResult;
		if (!bFound)
		{
			sResult = "No device exists with the name \"" + sSwitchName + "\".";
		}
		else
		{
			uint8_t tVal = (uint8_t)(int)round(iLevel * 255.0 / 100.0);
			_tColor color(tVal, ColorModeTemp);
			SendProgress(tl_progressCtx.sid, tl_progressCtx.progressToken, 0, 2, "Sending command to device");
			auto rc = m_mainworker.SwitchLight(uIdx, "Set Color", -1, color, false, 0, "MCP");
			sResult = (rc == MainWorker::eSwitchLightReturnCode::SL_ERROR)
				? "Error setting color temperature on \"" + sSwitchName + "\"."
				: "Color temperature on \"" + sSwitchName + "\" set to " + std::to_string(iKelvin) + "K.";
			SendProgress(tl_progressCtx.sid, tl_progressCtx.progressToken, 2, 2, "Command sent");
		}
		mcp::setToolResult(jsonRPCRep, sResult, !bFound);
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
		mcp::setToolResult(jsonRPCRep, sResult, false);
		return true;
	}

	bool switchScene(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool bHasId = args.isMember("scene_id");
		bool bHasName = args.isMember("scenename") && !args["scenename"].asString().empty();
		if ((!bHasId && !bHasName) || !args.isMember("command"))
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: switchScene: Missing required parameter 'scenename' or 'command'");
			return false;
		}
		std::string sCommand = args["command"].asString();

		// Capitalize first letter, lowercase rest — matches "On"/"Off" format
		if (!sCommand.empty()) {
			sCommand[0] = toupper(sCommand[0]);
			for (size_t i = 1; i < sCommand.size(); i++)
				sCommand[i] = tolower(sCommand[i]);
		}

		if (sCommand != "On" && sCommand != "Off")
		{
			mcp::setToolResult(jsonRPCRep, "Invalid command '" + sCommand + "'. Must be 'On' or 'Off'.", true);
			return true;
		}

		std::vector<std::vector<std::string>> result;
		std::string sIdentifier;
		if (bHasId)
		{
			int nSceneId = args["scene_id"].asInt();
			sIdentifier = "idx=" + std::to_string(nSceneId);
			result = m_sql.safe_query("SELECT ID, Name FROM Scenes WHERE ID=%d", nSceneId);
		}
		else
		{
			std::string sName = args["scenename"].asString();
			sIdentifier = "\"" + sName + "\"";
			result = m_sql.safe_query("SELECT ID, Name FROM Scenes WHERE Name='%q'", sName.c_str());
		}

		std::string sResult;
		bool bFound = !result.empty();
		std::string sSceneName = bFound ? result[0][1] : sIdentifier;
		if (!bFound)
		{
			sResult = "No scene or group exists with the name \"" + sSceneName + "\".";
		}
		else
		{
			std::string sIdx = result[0][0];
			SendProgress(mcp::tl_progressCtx.sid, mcp::tl_progressCtx.progressToken, 0, 2, "Sending command to device");
			bool bOk = m_mainworker.SwitchScene(sIdx, sCommand, "MCP");
			SendProgress(mcp::tl_progressCtx.sid, mcp::tl_progressCtx.progressToken, 2, 2, "Command sent");
			sResult = bOk
				? "Scene \"" + sSceneName + "\" switched " + sCommand + " successfully."
				: "Error switching scene \"" + sSceneName + "\".";
		}
		mcp::setToolResult(jsonRPCRep, sResult, !bFound);
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
		mcp::setToolResult(jsonRPCRep, sResult, false);
		return true;
	}

	bool getRoomDevices(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool bHasId = args.isMember("room_id");
		bool bHasName = args.isMember("roomname") && !args["roomname"].asString().empty();
		if (!bHasId && !bHasName)
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: getRoomDevices: Missing required parameter 'roomname' or 'room_id'");
			return false;
		}

		std::vector<std::vector<std::string>> planResult;
		std::string sIdentifier;
		if (bHasId)
		{
			int nRoomId = args["room_id"].asInt();
			sIdentifier = "idx=" + std::to_string(nRoomId);
			planResult = m_sql.safe_query("SELECT ID, Name FROM Plans WHERE ID=%d", nRoomId);
		}
		else
		{
			std::string sName = args["roomname"].asString();
			sIdentifier = "\"" + sName + "\"";
			planResult = m_sql.safe_query("SELECT ID, Name FROM Plans WHERE Name='%q'", sName.c_str());
		}

		std::string sResult;
		bool bFound = !planResult.empty();
		std::string sRoomName = bFound ? planResult[0][1] : sIdentifier;
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
		mcp::setToolResult(jsonRPCRep, sResult, !bFound);
		return true;
	}

	bool getSceneDevices(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		const Json::Value &args = jsonRequest["params"]["arguments"];
		bool bHasId = args.isMember("scene_id");
		bool bHasName = args.isMember("scenename") && !args["scenename"].asString().empty();
		if (!bHasId && !bHasName)
		{
			_log.Debug(DEBUG_WEBSERVER, "MCP: getSceneDevices: Missing required parameter 'scenename' or 'scene_id'");
			return false;
		}

		std::vector<std::vector<std::string>> scResult;
		std::string sIdentifier;
		if (bHasId)
		{
			int nSceneId = args["scene_id"].asInt();
			sIdentifier = "idx=" + std::to_string(nSceneId);
			scResult = m_sql.safe_query("SELECT ID, SceneType, Name FROM Scenes WHERE ID=%d", nSceneId);
		}
		else
		{
			std::string sName = args["scenename"].asString();
			sIdentifier = "\"" + sName + "\"";
			scResult = m_sql.safe_query("SELECT ID, SceneType, Name FROM Scenes WHERE Name='%q'", sName.c_str());
		}

		std::string sResult;
		bool bFound = !scResult.empty();
		std::string sSceneName = bFound ? scResult[0][2] : sIdentifier;
		if (!bFound)
		{
			sResult = "No scene or group exists with the name \"" + sSceneName + "\".";
		}
		else
		{
			std::string sSceneIdx = scResult[0][0];
			int nSceneIdx = atoi(sSceneIdx.c_str());
			auto devResult = m_sql.safe_query(
				"SELECT a.DeviceRowID, b.Name, a.Cmd, a.Level "
				"FROM SceneDevices a JOIN DeviceStatus b ON b.ID=a.DeviceRowID "
				"WHERE a.SceneRowID=%d ORDER BY b.Name", nSceneIdx);
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
		mcp::setToolResult(jsonRPCRep, sResult, !bFound);
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
		mcp::setToolResult(jsonRPCRep, sResult, false);
		return true;
	}

	bool getSystemSettings(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		mcp::setToolResult(jsonRPCRep, buildSettingsText(), false);
		return true;
	}

	bool getSunTimes(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		std::string sResult;
		if (m_mainworker.m_LastSunriseSet.empty())
		{
			sResult = "Sun rise/set data is not available yet. Domoticz needs a valid location configured.";
			mcp::setToolResult(jsonRPCRep, sResult, true);
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

		mcp::setToolResult(jsonRPCRep, sResult, false);
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
		mcp::setToolResult(jsonRPCRep, sResult, false);
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
		mcp::setToolResult(jsonRPCRep, sResult, false);
		return true;
	}

	bool getUsers(const Json::Value &jsonRequest, Json::Value &jsonRPCRep)
	{
		// Intentionally excludes Password, Secret, and MFAToken columns for security
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
		mcp::setToolResult(jsonRPCRep, sResult, false);
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

		mcp::setToolResult(jsonRPCRep, sResult, false);
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
			mcp::setToolResult(jsonRPCRep, "Invalid status value " + std::to_string(iNewStatus) + ". Must be 0 (Disarmed), 1 (Armed Home), or 2 (Armed Away).", true);
			return true;
		}

		// Read stored security code (stored as MD5 hash, 32 hex chars)
		std::string sStoredCode;
		int nValue = 1;
		m_sql.GetPreferencesVar("SecPassword", nValue, sStoredCode);

		if (sStoredCode.empty())
		{
			mcp::setToolResult(jsonRPCRep, "No security code is configured. Please set a security PIN in Domoticz settings before using this tool.", true);
			return true;
		}

		// The stored code is an MD5 hash. Always hash the provided plaintext PIN.
		std::string sHashedCode = GenerateMD5Hash(sProvidedCode);

		if (sHashedCode != sStoredCode)
		{
			_log.Log(LOG_STATUS, "MCP: setSecurityStatus: Invalid security code provided.");
			mcp::setToolResult(jsonRPCRep, "Invalid security code. Access denied.", true);
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

		mcp::setToolResult(jsonRPCRep, "Security panel status set to: " + sNewStatus + ".", false);
		return true;
	}

} // namespace mcp