#pragma once

#include <string>
#include <set>
#include <deque>
#include <functional>
#include <mutex>
#include <atomic>
#include <ctime>
#include "../main/Logger.h"
#include "../extern/libwebem/include/libwebem/session.h"
#include "../extern/libwebem/include/libwebem/ISseHandler.h"

struct CMcpSession
{
	std::string sessionId;
	http::server::WebEmSession webSession;
	http::server::_eUserRights rights = http::server::URIGHTS_NONE;
	time_t createdAt;
	time_t lastActivity;

	std::function<void(const std::string&)> sseWriter;
	bool sseConnected = false;

	std::set<std::string> subscribedUris;

	// MCP log priority: 0=debug, 1=info/norm, 2=notice/status, 3=error
	// (cannot use _eLogLevel values directly — they are non-sequential bitmasks)
	// MCP_LOG_LEVEL_OFF is a sentinel above all real priorities.
	// Default is 3 (error only): Domoticz status/info/debug logs are not forwarded.
	// MCP-specific messages (logger="mcp") are sent unconditionally via logLevel=-1.
	static constexpr int MCP_LOG_LEVEL_OFF = 4;
	int minLogLevel = 3;

	uint64_t nextEventId = 1;

	struct SentEvent
	{
		uint64_t id;
		std::string json;
		time_t timestamp;
	};
	std::deque<SentEvent> recentEvents;

	mutable std::mutex m_sendMutex;

	void SendSseEvent(const std::string& notificationJson, int logLevel = -1);
	void ReplayEventsAfter(uint64_t lastId, std::function<void(const std::string&)> writer);
};

class CMcpSseHandler : public http::server::ISseHandler
{
public:
	// context format: "mcpSessionId" or "mcpSessionId:lastEventId"
	CMcpSseHandler(std::function<void(const std::string&)> writer,
	               const http::server::WebEmSession& webSession,
	               const std::string& context);
	~CMcpSseHandler() override;

	void Start() override;
	void Stop() override;
	bool IsAlive() const override;

private:
	std::function<void(const std::string&)> m_writer;
	http::server::WebEmSession m_webSession;
	std::string m_mcpSessionId;
	std::string m_lastEventId;
	bool m_isLegacy = false;
	std::string m_legacyEndpointUrl;
	std::atomic<bool> m_alive{ true };
};
