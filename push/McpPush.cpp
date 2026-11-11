#include "stdafx.h"
#include "McpPush.h"
#include "../main/Logger.h"
#include "../main/mainworker.h"
#include "../main/json_helper.h"
#include "../main/WebServerHelper.h"
#include "../mcpserver/McpSessionRegistry.h"
#include <json/json.h>
#include <chrono>
#include <map>
#include <set>

extern boost::signals2::signal<void(const std::string& Subject, const std::string& Text, const std::string& ExtraData, const int Priority, const std::string& Sound, const bool bFromNotification)> sOnNotificationReceived;
extern http::server::CWebServerHelper m_webservers;

CMcpPush g_McpPush;

namespace
{
	// Per-user copy of the devices shared with that user, so that frequent device
	// updates do not each run a SharedDevices query for every connected non-admin
	// session. A share change takes effect within the time-to-live.
	constexpr auto kVisibilityCacheTtl = std::chrono::seconds(2);

	struct VisibleDevices
	{
		bool bAll = false;
		std::set<uint64_t> idxs;
		std::chrono::steady_clock::time_point fetchedAt;
	};

	std::mutex g_visibilityMutex;
	std::map<std::string, VisibleDevices> g_visibilityCache;

	bool IsDeviceVisibleCached(const std::string& username, const http::server::_eUserRights rights, const uint64_t idx)
	{
		if (rights == http::server::URIGHTS_ADMIN)
			return true;
		if (username.empty())
			return false;

		const auto now = std::chrono::steady_clock::now();
		{
			std::lock_guard<std::mutex> lock(g_visibilityMutex);
			auto it = g_visibilityCache.find(username);
			if ((it != g_visibilityCache.end()) && (now - it->second.fetchedAt < kVisibilityCacheTtl))
				return it->second.bAll || (it->second.idxs.count(idx) != 0);
		}

		// Queried without the cache lock held: the SQL layer may log, and logging feeds back into MCP
		VisibleDevices entry;
		entry.bAll = m_webservers.GetUserVisibleDevices(username, rights, entry.idxs);
		entry.fetchedAt = now;
		const bool bVisible = entry.bAll || (entry.idxs.count(idx) != 0);

		std::lock_guard<std::mutex> lock(g_visibilityMutex);
		for (auto it = g_visibilityCache.begin(); it != g_visibilityCache.end();)
		{
			if (now - it->second.fetchedAt >= kVisibilityCacheTtl)
				it = g_visibilityCache.erase(it);
			else
				++it;
		}
		g_visibilityCache[username] = std::move(entry);
		return bVisible;
	}
} // namespace

CMcpPush::CMcpPush()
{
	m_PushType = PushType::PUSHTYPE_UNKNOWN;
}

CMcpPush::~CMcpPush()
{
	Stop();
}

CMcpPush& CMcpPush::Instance()
{
	return g_McpPush;
}

void CMcpPush::Start()
{
	std::unique_lock<std::recursive_mutex> lock(m_mutex);
	if (m_isStarted)
		return;

	m_connDeviceReceived = m_mainworker.sOnDeviceReceived.connect(
		[this](auto id, auto idx, auto&& name, auto rx) { OnDeviceReceived(id, idx, name, rx); });
	m_connDeviceUpdate = m_mainworker.sOnDeviceUpdate.connect(
		[this](auto id, auto idx) { OnDeviceUpdate(id, idx); });
	m_connSceneChanged = m_mainworker.sOnSwitchScene.connect(
		[this](auto idx, auto&& name) { OnSceneChange(idx, name); });
	m_connNotification = sOnNotificationReceived.connect(
		[this](auto&& s, auto&& t, auto&& e, auto p, auto&& snd, auto n) { OnNotificationReceived(s, t, e, p, snd, n); });

	_log.sOnLogMessage.connect(this, &CMcpPush::OnLogMessage, &m_sLogMessage);

	m_isStarted = true;
}

void CMcpPush::Stop()
{
	std::unique_lock<std::recursive_mutex> lock(m_mutex);
	if (!m_isStarted)
		return;

	m_connDeviceReceived.disconnect();
	m_connDeviceUpdate.disconnect();
	m_connSceneChanged.disconnect();
	m_connNotification.disconnect();
	m_sLogMessage.disconnect();

	m_isStarted = false;
}

static Json::Value MakeResourceUpdatedNotification(const std::string& uri)
{
	Json::Value notif;
	notif["jsonrpc"] = "2.0";
	notif["method"] = "notifications/resources/updated";
	notif["params"]["uri"] = uri;
	return notif;
}

static Json::Value MakeResourceListChangedNotification()
{
	Json::Value notif;
	notif["jsonrpc"] = "2.0";
	notif["method"] = "notifications/resources/list_changed";
	return notif;
}

static Json::Value MakeToolsListChangedNotification()
{
	Json::Value notif;
	notif["jsonrpc"] = "2.0";
	notif["method"] = "notifications/tools/list_changed";
	return notif;
}

static Json::Value MakeLogNotification(const std::string& level, const std::string& message)
{
	Json::Value notif;
	notif["jsonrpc"] = "2.0";
	notif["method"] = "notifications/message";
	notif["params"]["level"] = level;
	notif["params"]["logger"] = "domoticz";
	notif["params"]["data"]["message"] = message;
	return notif;
}

static std::string LogLevelToMcpLevel(const _eLogLevel level)
{
	switch (level)
	{
		case LOG_ERROR:
			return "error";
		case LOG_STATUS:
			return "notice";
		case LOG_DEBUG_INT:
			return "debug";
		case LOG_NORM:
		default:
			return "info";
	}
}

void CMcpPush::SendDeviceUpdated(const uint64_t DeviceRowIdx)
{
	{
		std::unique_lock<std::recursive_mutex> lock(m_mutex);
		if (!m_isStarted)
			return;
	}

	// Sent without m_mutex held: the visibility check may run SQL, and a thread that logs
	// while holding the SQL lock enters OnLogMessage, which takes m_mutex
	std::string uri = "domoticz://devices/" + std::to_string(DeviceRowIdx);
	Json::Value notif = MakeResourceUpdatedNotification(uri);
	CMcpSessionRegistry::Instance().SendResourceUpdated(
		uri, JSonToRawString(notif),
		[DeviceRowIdx](const std::string& username, const http::server::_eUserRights rights) {
			return IsDeviceVisibleCached(username, rights, DeviceRowIdx);
		});
}

void CMcpPush::OnDeviceReceived(int /*hwdID*/, uint64_t DeviceRowIdx,
                                const std::string& /*DeviceName*/,
                                const unsigned char* /*pRXCommand*/)
{
	SendDeviceUpdated(DeviceRowIdx);
}

void CMcpPush::OnDeviceUpdate(int /*hwdID*/, uint64_t DeviceRowIdx)
{
	SendDeviceUpdated(DeviceRowIdx);
}

void CMcpPush::OnSceneChange(uint64_t SceneIdx, const std::string& /*SceneName*/)
{
	std::unique_lock<std::recursive_mutex> lock(m_mutex);
	if (!m_isStarted)
		return;

	std::string uri = "domoticz://scenes/" + std::to_string(SceneIdx);
	Json::Value notif = MakeResourceUpdatedNotification(uri);
	CMcpSessionRegistry::Instance().SendResourceUpdated(uri, JSonToRawString(notif));
}

void CMcpPush::OnNotificationReceived(const std::string& /*Subject*/, const std::string& /*Text*/,
                                      const std::string& /*ExtraData*/, int /*Priority*/,
                                      const std::string& /*Sound*/, bool /*bFromNotification*/)
{
	// Not mapped to MCP notifications.
}

void CMcpPush::OnLogMessage(const _eLogLevel level, const std::string& sLogline)
{
	std::unique_lock<std::recursive_mutex> lock(m_mutex);
	if (!m_isStarted)
		return;

	Json::Value notif = MakeLogNotification(LogLevelToMcpLevel(level), sLogline);
	CMcpSessionRegistry::Instance().Broadcast(JSonToRawString(notif), (int)level);
}

void CMcpPush::onDeviceTableChanged()
{
	std::unique_lock<std::recursive_mutex> lock(m_mutex);
	if (!m_isStarted)
		return;

	CMcpSessionRegistry::Instance().Broadcast(JSonToRawString(MakeToolsListChangedNotification()));
	CMcpSessionRegistry::Instance().Broadcast(JSonToRawString(MakeResourceListChangedNotification()));
}
