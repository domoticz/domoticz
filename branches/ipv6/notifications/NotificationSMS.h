#pragma once
#include "NotificationBase.h"

class CNotificationSMS : public CNotificationBase {
public:
	CNotificationSMS();
	~CNotificationSMS();
	virtual bool IsConfigured();
protected:
	virtual bool SendMessageImplementation(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string &Sound, const bool bFromNotification);
private:
	/* config vars */
	std::string _clickatellApi;
	std::string _clickatellUser;
	std::string _clickatellPassword;
	std::string _clickatellFrom;
	std::string _clickatellTo;
	/* default vars */
	int _maxCredits;
	int _escalate;
	int _features;
	int _concat;
};
