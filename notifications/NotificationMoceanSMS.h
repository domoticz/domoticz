#pragma once
#include "NotificationBase.h"

class CNotificationMoceanSMS : public CNotificationBase {
public:
	CNotificationMoceanSMS();
	~CNotificationMoceanSMS();
	virtual bool IsConfigured();
protected:
	virtual bool SendMessageImplementation(
		const uint64_t Idx,
		const std::string& Name,
		const std::string& Subject,
		const std::string& Text,
		const std::string& ExtraData,
		const int Priority,
		const std::string& Sound,
		const bool bFromNotification);
private:
	/* config vars */
	std::string _moceanApiKey;
	std::string _moceanApiSecret;
	std::string _moceanTo;
	std::string _moceanFrom;
};
