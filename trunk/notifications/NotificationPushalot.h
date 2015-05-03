#pragma once
#include "NotificationBase.h"

class CNotificationPushalot : public CNotificationBase {
public:
	CNotificationPushalot();
	~CNotificationPushalot();
	virtual bool IsConfigured();
protected:
	virtual bool SendMessageImplementation(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string &Sound, const bool bFromNotification);
private:
	std::string _apikey;
};
