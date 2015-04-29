#pragma once
#include "NotificationBase.h"

class CNotificationSMS : public CNotificationBase {
public:
	CNotificationSMS();
	~CNotificationSMS();
	virtual bool IsConfigured();
protected:
	virtual bool SendMessageImplementation(const std::string &Subject, const std::string &Text, const int Priority, const std::string &Sound, const bool bFromNotification);
private:
	std::string _apikey;
};
