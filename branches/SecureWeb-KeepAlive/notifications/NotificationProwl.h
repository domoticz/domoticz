#pragma once
#include "NotificationBase.h"

class CNotificationProwl : public CNotificationBase {
public:
	CNotificationProwl();
	~CNotificationProwl();
	virtual bool IsConfigured();
protected:
	virtual bool SendMessageImplementation(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string &Sound, const bool bFromNotification);
private:
	std::string _apikey;
};
