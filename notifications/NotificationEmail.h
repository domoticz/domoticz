#pragma once
#include "NotificationBase.h"

class CNotificationEmail : public CNotificationBase {
public:
	CNotificationEmail();
	~CNotificationEmail();
	virtual bool IsConfigured();
protected:
	virtual bool SendMessageImplementation(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string &Sound, const bool bFromNotification);
private:
	std::string _EmailFrom;
	std::string _EmailTo;
	std::string _EmailServer;
	int _EmailPort;
	std::string _EmailUsername;
	std::string _EmailPassword;
	int _UseEmailInNotifications;
	int _EmailAsAttachment;
};
