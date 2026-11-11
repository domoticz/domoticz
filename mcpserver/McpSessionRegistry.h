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

	std::shared_ptr<CMcpSession> GetSession(const std::string& sessionId);

	bool WithSession(const std::string& sessionId,
	                 std::function<void(CMcpSession&)> callback);

	void Broadcast(const std::string& notificationJson, int logLevel = -1);

	void SendToSession(const std::string& sessionId,
	                   const std::string& notificationJson, int logLevel = -1);

	// Decides whether a non-admin user may receive an update for a resource. It is
	// called without the registry lock held, so it may query the database.
	using ResourceVisibleFn = std::function<bool(const std::string& username, http::server::_eUserRights rights)>;

	// Sends to every SSE session that has no subscriptions or is subscribed to the uri
	// (or to a parent collection of it). When isVisible is set, non-admin sessions only
	// receive the update when it returns true for the session's user.
	void SendResourceUpdated(const std::string& uri,
	                         const std::string& notificationJson,
	                         const ResourceVisibleFn& isVisible = nullptr);

	void PruneExpiredSessions(int timeoutSeconds = 3600);

private:
	std::map<std::string, std::shared_ptr<CMcpSession>> m_sessions;
	mutable std::mutex m_mutex;

	static std::string GenerateSessionId();
};
