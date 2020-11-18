#pragma once
#include "NotificationBase.h"

class CNotificationEmail : public CNotificationBase {
public:
	CNotificationEmail();
	~CNotificationEmail() override = default;
	bool IsConfigured() override;

      protected:
	bool SendMessageImplementation(uint64_t Idx, const std::string &Name, const std::string &Subject, const std::string &Text, const std::string &ExtraData, int Priority, const std::string &Sound,
				       bool bFromNotification) override;

      private:
	std::string _EmailFrom;
	std::string _EmailTo;
	std::string _EmailServer;
	int			_EmailPort;
	std::string _EmailUsername;
	std::string _EmailPassword;
	int			_UseEmailInNotifications;
	int			_EmailAsAttachment;
};
