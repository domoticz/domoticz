#include "stdafx.h"
#include "McpSseSession.h"
#include "McpSessionRegistry.h"
#include "../main/Logger.h"
#include "../main/json_helper.h"
#include <json/json.h>

#include <ctime>

// Maps _eLogLevel bitmask values to a 0-3 priority scale (higher = more important).
// LOG_NORM=1→1, LOG_STATUS=2→2, LOG_ERROR=4→3, LOG_DEBUG_INT=8→0
static int LogLevelToPriority(int level)
{
	switch (level)
	{
		case 0x8: return 0; // LOG_DEBUG_INT
		case 0x1: return 1; // LOG_NORM
		case 0x2: return 2; // LOG_STATUS
		case 0x4: return 3; // LOG_ERROR
		default:  return 1;
	}
}

void CMcpSession::SendSseEvent(const std::string& notificationJson, int logLevel)
{
	if (logLevel >= 0 && LogLevelToPriority(logLevel) < minLogLevel)
		return;

	std::lock_guard<std::mutex> lock(m_sendMutex);

	uint64_t id = nextEventId++;

	std::string event = "id: ";
	event += std::to_string(id);
	event += "\ndata: ";
	event += notificationJson;
	event += "\n\n";

	time_t now = time(nullptr);
	SentEvent sent;
	sent.id = id;
	sent.json = notificationJson;
	sent.timestamp = now;
	recentEvents.push_back(std::move(sent));

	while (recentEvents.size() > 100
	       || (!recentEvents.empty() && now - recentEvents.front().timestamp > 300))
	{
		recentEvents.pop_front();
	}

	if (sseConnected && sseWriter)
		sseWriter(event);
}

void CMcpSession::ReplayEventsAfter(uint64_t lastId, std::function<void(const std::string&)> writer)
{
	std::lock_guard<std::mutex> lock(m_sendMutex);
	for (const auto& ev : recentEvents)
	{
		if (ev.id > lastId)
		{
			std::string event = "id: ";
			event += std::to_string(ev.id);
			event += "\ndata: ";
			event += ev.json;
			event += "\n\n";
			writer(event);
		}
	}
}

CMcpSseHandler::CMcpSseHandler(std::function<void(const std::string&)> writer,
                                const http::server::WebEmSession& webSession,
                                const std::string& context)
	: m_writer(std::move(writer))
	, m_webSession(webSession)
{
	// context format: "mcpSessionId" or "mcpSessionId:lastEventId"
	auto colon = context.find(':');
	if (colon != std::string::npos)
	{
		m_mcpSessionId = context.substr(0, colon);
		m_lastEventId = context.substr(colon + 1);
	}
	else
	{
		m_mcpSessionId = context;
	}
}

CMcpSseHandler::~CMcpSseHandler()
{
	Stop();
}

void CMcpSseHandler::Start()
{
	if (!m_mcpSessionId.empty())
	{
		CMcpSessionRegistry::Instance().AttachSseStream(
			m_mcpSessionId,
			[this](const std::string& event) {
				if (m_alive.load())
					m_writer(event);
			});

		if (!m_lastEventId.empty())
		{
			uint64_t lastId = 0;
			try { lastId = std::stoull(m_lastEventId); } catch (...) {}
			if (lastId > 0)
			{
				CMcpSessionRegistry::Instance().WithSession(
					m_mcpSessionId,
					[&](CMcpSession& s) { s.ReplayEventsAfter(lastId, m_writer); });
			}
		}
	}
	_log.Debug(DEBUG_WEBSERVER, "MCP: SSE handler started for session %s",
	           m_mcpSessionId.empty() ? "(anon)" : m_mcpSessionId.c_str());

	// Send MCP-specific "connected" notification unconditionally (logLevel=-1 bypasses filter).
	Json::Value notif;
	notif["jsonrpc"] = "2.0";
	notif["method"] = "notifications/message";
	notif["params"]["level"] = "notice";
	notif["params"]["logger"] = "mcp";
	notif["params"]["data"]["message"] = "SSE stream connected for session " + m_mcpSessionId;
	CMcpSessionRegistry::Instance().SendToSession(m_mcpSessionId, JSonToRawString(notif));
}

void CMcpSseHandler::Stop()
{
	if (m_alive.exchange(false))
	{
		if (!m_mcpSessionId.empty())
			CMcpSessionRegistry::Instance().DetachSseStream(m_mcpSessionId);
		_log.Debug(DEBUG_WEBSERVER, "MCP: SSE handler stopped for session %s",
		           m_mcpSessionId.c_str());
	}
}

bool CMcpSseHandler::IsAlive() const
{
	return m_alive.load();
}
