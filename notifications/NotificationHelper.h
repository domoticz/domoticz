#pragma once
#include "NotificationBase.h"
#include "../webserver/cWebem.h"

#include <vector>
#include <string>

#define NOTIFYALL std::string("")

struct _tNotification
{
	unsigned long long ID;
	std::string Params;
	int Priority;
	time_t LastSend;
	std::string CustomMessage;
	std::string ActiveSystems;
};

class CNotificationHelper {
public:
	CNotificationHelper();
	~CNotificationHelper();
	bool SendMessage(const std::string &subsystems, const std::string &Subject, const std::string &Text, const std::string &ExtraData, const bool bFromNotification);
	bool SendMessageEx(const std::string &subsystems, const std::string &Subject, const std::string &Text, const std::string &ExtraData, int Priority, const std::string &Sound, const bool bFromNotification);
	void LoadConfig();
	void ConfigFromGetvars(http::server::cWebem *m_pWebEm, const bool save);
	bool IsInConfig(const std::string &Key);

	//Notificaton functions
	//notification functions
	bool AddNotification(const std::string &DevIdx, const std::string &Param, const std::string &CustomMessage, const std::string &ActiveSystems, const int Priority);
	bool RemoveDeviceNotifications(const std::string &DevIdx);
	bool RemoveNotification(const std::string &ID);
	std::vector<_tNotification> GetNotifications(const unsigned long long DevIdx);
	std::vector<_tNotification> GetNotifications(const std::string &DevIdx);
	void TouchNotification(const unsigned long long ID);
	bool HasNotifications(const unsigned long long DevIdx);
	bool HasNotifications(const std::string &DevIdx);

	bool CheckAndHandleTempHumidityNotification(
		const int HardwareID,
		const std::string &ID,
		const unsigned char unit,
		const unsigned char devType,
		const unsigned char subType,
		const float temp,
		const int humidity,
		const bool bHaveTemp,
		const bool bHaveHumidity);
	bool CheckAndHandleDewPointNotification(
		const int HardwareID,
		const std::string &ID,
		const unsigned char unit,
		const unsigned char devType,
		const unsigned char subType,
		const float temp,
		const float dewpoint);
	bool CheckAndHandleNotification(
		const int HardwareID,
		const std::string &ID,
		const unsigned char unit,
		const unsigned char devType,
		const unsigned char subType,
		const _eNotificationTypes ntype,
		const float mvalue);
	bool CheckAndHandleSwitchNotification(
		const int HardwareID,
		const std::string &ID,
		const unsigned char unit,
		const unsigned char devType,
		const unsigned char subType,
		const _eNotificationTypes ntype);
	bool CheckAndHandleRainNotification(
		const int HardwareID,
		const std::string &ID,
		const unsigned char unit,
		const unsigned char devType,
		const unsigned char subType,
		const _eNotificationTypes ntype,
		const float mvalue);
	bool CheckAndHandleTotalNotification(
		const int HardwareID,
		const std::string &ID,
		const unsigned char unit,
		const unsigned char devType,
		const unsigned char subType,
		const _eNotificationTypes ntype,
		const float mvalue);
	bool CheckAndHandleUsageNotification(
		const int HardwareID,
		const std::string &ID,
		const unsigned char unit,
		const unsigned char devType,
		const unsigned char subType,
		const _eNotificationTypes ntype,
		const float mvalue);
	bool CheckAndHandleAmpere123Notification(
		const int HardwareID,
		const std::string &ID,
		const unsigned char unit,
		const unsigned char devType,
		const unsigned char subType,
		const float Ampere1,
		const float Ampere2,
		const float Ampere3
		);


	std::map<std::string, CNotificationBase*> m_notifiers;
protected:
	void SetConfigValue(const std::string &key, const std::string &value);
private:
	void AddNotifier(CNotificationBase *notifier);
};

extern CNotificationHelper m_notifications;
