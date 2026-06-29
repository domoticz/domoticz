#include "stdafx.h"
#include "NotificationBrowser.h"
#include <boost/signals2.hpp>

boost::signals2::signal<void(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string & Sound, const bool bFromNotification)> sOnNotificationReceived;
// Emitted from the plugin worker thread; all connected slots are protected by
// each CDomoticzWebsocketHandler's handlerMutex and isStarted guard.
boost::signals2::signal<void(const std::string &pluginKey, int hwId, const std::string &jsonPayload)> sOnPluginWebSocketMessage;

CNotificationBrowser::CNotificationBrowser() : CNotificationBase(std::string("browser"), 0)
{
}

bool CNotificationBrowser::IsConfigured()
{
	return !sOnNotificationReceived.empty();
}

bool CNotificationBrowser::SendMessageImplementation(
	const uint64_t Idx,
	const std::string &Name,
	const std::string &Subject,
	const std::string &Text,
	const std::string &ExtraData,
	const int Priority,
	const std::string &Sound,
	const bool bFromNotification)
{
	sOnNotificationReceived(Subject, Text, ExtraData, Priority, Sound, bFromNotification);
	return true;
}
