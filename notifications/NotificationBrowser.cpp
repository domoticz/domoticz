#include "stdafx.h"
#include "NotificationBrowser.h"

boost::signals2::signal<void(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string & Sound, const bool bFromNotification)> sOnNotificationReceived;

CNotificationBrowser::CNotificationBrowser() : CNotificationBase(std::string("browser"), 0)
{
}

CNotificationBrowser::~CNotificationBrowser()
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
