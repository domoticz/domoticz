#include "stdafx.h"
#include "../main/Logger.h"
#include "NotificationHelper.h"
#include "NotificationProwl.h"
#include "NotificationNma.h"
#include "NotificationPushover.h"
#include "NotificationPushalot.h"
#include "NotificationEmail.h"
#include "NotificationSMS.h"

#if defined WIN32
	#include "../msbuild/WindowsHelper.h"
#endif

typedef std::map<std::string, CNotificationBase*>::iterator it_noti_type;

CNotificationHelper::CNotificationHelper()
{
	AddNotifier(new CNotificationProwl());
	AddNotifier(new CNotificationNma());
	AddNotifier(new CNotificationPushover());
	AddNotifier(new CNotificationPushalot());
	AddNotifier(new CNotificationEmail());
	AddNotifier(new CNotificationSMS());
	/* more notifiers can be added here */
}

CNotificationHelper::~CNotificationHelper()
{
	for (it_noti_type iter = _notifiers.begin(); iter != _notifiers.end(); iter++) {
		delete iter->second;
	}
}

void CNotificationHelper::AddNotifier(CNotificationBase *notifier)
{
	_notifiers[notifier->GetSubsystemId()] = notifier;
}

bool CNotificationHelper::SendMessage(const std::string subsystem, const std::string Subject, const std::string Text, const bool bFromNotification)
{
	return SendMessageEx(subsystem, Subject, Text, 0, std::string(""),bFromNotification);
}

bool CNotificationHelper::SendMessageEx(const std::string subsystem, const std::string Subject, const std::string Text, int Priority, const std::string Sound, const bool bFromNotification)
{
	bool bRet = false;
#if defined WIN32
	//Make a system tray message
	ShowSystemTrayNotification(Subject.c_str());
#endif
	for (it_noti_type iter = _notifiers.begin(); iter != _notifiers.end(); iter++) {
		if ((subsystem == "" || subsystem == iter->first) && iter->second->IsConfigured()) {
			bRet |= iter->second->SendMessageEx(Subject, Text, Priority, Sound, bFromNotification);
		}
	}
	return bRet;
}

void CNotificationHelper::SetConfigValue(const std::string &key, const std::string &value)
{
	for (it_noti_type iter = _notifiers.begin(); iter != _notifiers.end(); iter++) {
		iter->second->SetConfigValue(key, value);
	}
}

void CNotificationHelper::ConfigFromGetvars(http::server::cWebem *webEm, const bool save)
{
	for (it_noti_type iter = _notifiers.begin(); iter != _notifiers.end(); iter++) {
		iter->second->ConfigFromGetvars(webEm, save);
	}
}

bool CNotificationHelper::IsInConfig(const std::string &Key)
{
	for (it_noti_type iter = _notifiers.begin(); iter != _notifiers.end(); iter++) {
		if (iter->second->IsInConfig(Key)) {
			return true;
		}
	}
	return false;
}

void CNotificationHelper::LoadConfig()
{
	int tot = 0, active = 0;
	std::stringstream logline;
	logline << "Active notification subsystems:";
	for (it_noti_type iter = _notifiers.begin(); iter != _notifiers.end(); iter++) {
		tot++;
		iter->second->LoadConfig();
		if (iter->second->IsConfigured()) {
			if (active==0)
				logline << " " << iter->first;
			else
				logline << ", " << iter->first;
			active++;
		}
	}
	logline << " (" << active << "/" << tot << ")";
	_log.Log(LOG_NORM, std::string(logline.str()).c_str());
}
