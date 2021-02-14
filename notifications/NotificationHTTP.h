#pragma once
#include "NotificationBase.h"

class CNotificationHTTP : public CNotificationBase {
public:
	CNotificationHTTP();
	~CNotificationHTTP() override = default;
	bool IsConfigured() override;

      protected:
	bool SendMessageImplementation(uint64_t Idx, const std::string &Name, const std::string &Subject, const std::string &Text, const std::string &ExtraData, int Priority, const std::string &Sound,
				       bool bFromNotification) override;

      private:
	std::string _HTTPField1;
	std::string _HTTPField2;
	std::string _HTTPField3;
	std::string _HTTPField4;
	std::string _HTTPTo;
	std::string _HTTPURL;
	std::string _HTTPPostData;
	std::string _HTTPPostContentType;
	std::string _HTTPPostHeaders;
};
