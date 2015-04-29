#pragma once
#include "NotificationBase.h"
#include "../webserver/cWebem.h"

#define NOTIFYALL std::string("")

class CNotificationHelper {
public:
	CNotificationHelper();
	~CNotificationHelper();
	bool SendMessage(const std::string subsystem, const std::string Subject, const std::string Text, const bool bFromNotification);
	bool SendMessageEx(const std::string subsystem, const std::string Subject, const std::string Text, int Priority, const std::string Sound, const bool bFromNotification);
	void LoadConfig();
	void ConfigFromGetvars(http::server::cWebem *m_pWebEm, const bool save);
	bool IsInConfig(const std::string &Key);
protected:
	void SetConfigValue(const std::string &key, const std::string &value);
private:
	void AddNotifier(CNotificationBase *notifier);
	std::map<std::string, CNotificationBase*> _notifiers;
};

extern CNotificationHelper m_notifications;
