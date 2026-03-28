#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <functional>
#include "McpSseSession.h"
#include <openssl/rand.h>

class CMcpSessionRegistry
{
public:
	static CMcpSessionRegistry& Instance();

	std::string CreateSession(const http::server::WebEmSession& webSession);

	bool AttachSseStream(const std::string& sessionId,
	                     std::function<void(const std::string&)> writer);

	void DetachSseStream(const std::string& sessionId);

	void RemoveSession(const std::string& sessionId);

	bool WithSession(const std::string& sessionId,
	                 std::function<void(CMcpSession&)> callback);

	void Broadcast(const std::string& notificationJson, int logLevel = -1);

	void SendToSession(const std::string& sessionId,
	                   const std::string& notificationJson, int logLevel = -1);

	void SendResourceUpdated(const std::string& uri,
	                         const std::string& notificationJson);

	void PruneExpiredSessions(int timeoutSeconds = 3600);

private:
	std::map<std::string, std::shared_ptr<CMcpSession>> m_sessions;
	mutable std::mutex m_mutex;

	static std::string GenerateSessionId();
};
