#pragma once
#include "NotificationBase.h"

class CNotificationFCM : public CNotificationBase {
public:
	CNotificationFCM();
	~CNotificationFCM();
	virtual bool IsConfigured();
protected:
	virtual bool SendMessageImplementation(
		const uint64_t Idx,
		const std::string &Name,
		const std::string &Subject,
		const std::string &Text,
		const std::string &ExtraData,
		const int Priority,
		const std::string &Sound,
		const bool bFromNotification);
};
