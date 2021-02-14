#pragma once
#include "NotificationBase.h"

class CNotificationSMS : public CNotificationBase {
public:
	CNotificationSMS();
	~CNotificationSMS() override = default;
	bool IsConfigured() override;

      protected:
	bool SendMessageImplementation(uint64_t Idx, const std::string &Name, const std::string &Subject, const std::string &Text, const std::string &ExtraData, int Priority, const std::string &Sound,
				       bool bFromNotification) override;

      private:
	/* config vars */
	std::string _clickatellApi;
	std::string _clickatellTo;
	std::string _clickatellFrom;

};
