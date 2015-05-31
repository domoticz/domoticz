#include "stdafx.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "NotificationHelper.h"
#include "NotificationProwl.h"
#include "NotificationNma.h"
#include "NotificationPushover.h"
#include "NotificationPushalot.h"
#include "NotificationEmail.h"
#include "NotificationSMS.h"
#include "NotificationHTTP.h"
#include "NotificationKodi.h"
#include <map>

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
	AddNotifier(new CNotificationHTTP());
	AddNotifier(new CNotificationKodi());
	/* more notifiers can be added here */
}

CNotificationHelper::~CNotificationHelper()
{
	for (it_noti_type iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter) {
		delete iter->second;
	}
}

void CNotificationHelper::AddNotifier(CNotificationBase *notifier)
{
	m_notifiers[notifier->GetSubsystemId()] = notifier;
}

bool CNotificationHelper::SendMessage(const std::string &subsystems, const std::string &Subject, const std::string &Text, const std::string &ExtraData, const bool bFromNotification)
{
	return SendMessageEx(subsystems, Subject, Text, ExtraData, 0, std::string(""), bFromNotification);
}

bool CNotificationHelper::SendMessageEx(const std::string &subsystems, const std::string &Subject, const std::string &Text, const std::string &ExtraData, int Priority, const std::string &Sound, const bool bFromNotification)
{
	bool bRet = false;
#if defined WIN32
	//Make a system tray message
	ShowSystemTrayNotification(Subject.c_str());
#endif
	std::vector<std::string> sResult;
	StringSplit(subsystems, ";", sResult);

	std::map<std::string, int> ActiveSystems;

	std::vector<std::string>::const_iterator itt;
	for (itt = sResult.begin(); itt != sResult.end(); ++itt)
	{
		ActiveSystems[*itt] = 1;
	}

	for (it_noti_type iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter) {
		std::map<std::string, int>::const_iterator ittSystem = ActiveSystems.find(iter->first);
		if (ActiveSystems.empty() || (ittSystem!=ActiveSystems.end() && iter->second->IsConfigured())) 
		{
			bRet |= iter->second->SendMessageEx(Subject, Text, ExtraData, Priority, Sound, bFromNotification);
		}
	}
	return bRet;
}

void CNotificationHelper::SetConfigValue(const std::string &key, const std::string &value)
{
	for (it_noti_type iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter) {
		iter->second->SetConfigValue(key, value);
	}
}

void CNotificationHelper::ConfigFromGetvars(http::server::cWebem *webEm, const bool save)
{
	for (it_noti_type iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter) {
		iter->second->ConfigFromGetvars(webEm, save);
	}
}

bool CNotificationHelper::IsInConfig(const std::string &Key)
{
	for (it_noti_type iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter) {
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
	for (it_noti_type iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter) {
		tot++;
		iter->second->LoadConfig();
		if (iter->second->IsConfigured()) {
			if (iter->second->m_IsEnabled)
			{
				if (active == 0)
					logline << " " << iter->first;
				else
					logline << ", " << iter->first;
				active++;
			}
		}
	}
	logline << " (" << active << "/" << tot << ")";
	_log.Log(LOG_NORM, std::string(logline.str()).c_str());
}
