#pragma once
#include "NotificationBase.h"

class CNotificationHTTP : public CNotificationBase {
public:
	CNotificationHTTP();
	~CNotificationHTTP();
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
