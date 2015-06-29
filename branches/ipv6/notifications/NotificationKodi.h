#pragma once
#include "NotificationBase.h"

class CNotificationKodi : public CNotificationBase {
public:
	CNotificationKodi();
	~CNotificationKodi();
	virtual bool IsConfigured();
protected:
	virtual bool SendMessageImplementation(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string &Sound, const bool bFromNotification);
private:
	std::string _IPAddress;
	int			_Port;
	int			_TTL;
		
	std::string	 GetIconFile(const std::string &ExtraData);
	std::string	 GetCustomIcon(std::string &szCustom);

};
