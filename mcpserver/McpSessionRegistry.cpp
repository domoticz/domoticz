#include "stdafx.h"
#include "McpSessionRegistry.h"
#include "../main/Logger.h"

#include <ctime>
#include <openssl/rand.h>

CMcpSessionRegistry& CMcpSessionRegistry::Instance()
{
	static CMcpSessionRegistry instance;
	return instance;
}

std::string CMcpSessionRegistry::GenerateSessionId()
{
	unsigned char buf[16];
	if (RAND_bytes(buf, sizeof(buf)) != 1)
	{
		// fallback: use time + counter
		static std::atomic<uint64_t> counter{ 0 };
		uint64_t t = static_cast<uint64_t>(time(nullptr));
		uint64_t c = ++counter;
		memcpy(buf, &t, std::min(sizeof(t), sizeof(buf)));
		memcpy(buf + sizeof(t), &c, std::min(sizeof(c), sizeof(buf) - sizeof(t)));
	}
	std::string id;
	id.reserve(32);
	for (auto b : buf)
	{
		char hex[3];
		snprintf(hex, sizeof(hex), "%02x", b);
		id += hex;
	}
	return id;
}

std::string CMcpSessionRegistry::CreateSession(const http::server::WebEmSession& webSession)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	std::string id = GenerateSessionId();
	auto session = std::make_shared<CMcpSession>();
	session->sessionId = id;
	session->webSession = webSession;
	session->createdAt = time(nullptr);
	session->lastActivity = session->createdAt;
	m_sessions[id] = session;
	return id;
}

bool CMcpSessionRegistry::AttachSseStream(const std::string& sessionId,
                                           std::function<void(const std::string&)> writer)
{
	return WithSession(sessionId, [&writer](CMcpSession& s) {
		std::lock_guard<std::mutex> lock(s.m_sendMutex);
		s.sseWriter = writer;
		s.sseConnected = true;
	});
}

void CMcpSessionRegistry::DetachSseStream(const std::string& sessionId)
{
	WithSession(sessionId, [](CMcpSession& s) {
		std::lock_guard<std::mutex> lock(s.m_sendMutex);
		s.sseWriter = nullptr;
		s.sseConnected = false;
	});
}

void CMcpSessionRegistry::RemoveSession(const std::string& sessionId)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_sessions.erase(sessionId);
}

bool CMcpSessionRegistry::WithSession(const std::string& sessionId,
                                       std::function<void(CMcpSession&)> callback)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_sessions.find(sessionId);
	if (it == m_sessions.end())
		return false;
	callback(*it->second);
	return true;
}

void CMcpSessionRegistry::Broadcast(const std::string& notificationJson, int logLevel)
{
	std::vector<std::shared_ptr<CMcpSession>> active;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		for (auto& kv : m_sessions)
		{
			if (kv.second->sseConnected)
				active.push_back(kv.second);
		}
	}
	for (auto& s : active)
		s->SendSseEvent(notificationJson, logLevel);
}

void CMcpSessionRegistry::SendToSession(const std::string& sessionId,
                                         const std::string& notificationJson, int logLevel)
{
	WithSession(sessionId, [&notificationJson, logLevel](CMcpSession& s) {
		s.SendSseEvent(notificationJson, logLevel);
	});
}

void CMcpSessionRegistry::SendResourceUpdated(const std::string& uri,
                                               const std::string& notificationJson)
{
	std::vector<std::shared_ptr<CMcpSession>> active;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		for (auto& kv : m_sessions)
		{
			CMcpSession& s = *kv.second;
			if (!s.sseConnected)
				continue;
			if (!s.subscribedUris.empty() && s.subscribedUris.find(uri) == s.subscribedUris.end())
				continue;
			active.push_back(kv.second);
		}
	}
	for (auto& s : active)
		s->SendSseEvent(notificationJson);
}

void CMcpSessionRegistry::PruneExpiredSessions(int timeoutSeconds)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	time_t now = time(nullptr);
	for (auto it = m_sessions.begin(); it != m_sessions.end();)
	{
		if (it->second->lastActivity + timeoutSeconds < now)
			it = m_sessions.erase(it);
		else
			++it;
	}
}
