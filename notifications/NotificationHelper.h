#pragma once
#include "NotificationBase.h"
#include "../webserver/cWebem.h"

#include <string>

#define NOTIFYALL std::string("")

struct _tNotification
{
	uint64_t ID;
	std::string Params;
	int Priority;
	time_t LastSend;
	time_t LastUpdate;
	std::string DeviceName;
	std::string CustomMessage;
	std::string ActiveSystems;
	bool SendAlways;
};

class CNotificationHelper {
public:
	CNotificationHelper();
	~CNotificationHelper();
	void Init();
	bool SendMessage(
		const uint64_t Idx,
		const std::string &Name,
		const std::string &Subsystems,
		const std::string &Subject,
		const std::string &Text,
		const std::string &ExtraData,
		const int Priority,
		const std::string &Sound,
		const bool bFromNotification);
	bool SendMessageEx(
		const uint64_t Idx,
		const std::string &Name,
		const std::string &Subsystems,
		const std::string &Subject,
		const std::string &Text,
		const std::string &ExtraData,
		int Priority,
		const std::string &Sound,
		const bool bFromNotification);
	void LoadConfig();
	void ConfigFromGetvars(const http::server::request& req, const bool save);
	bool IsInConfig(const std::string &Key);

	//notification functions
	void CheckAndHandleLastUpdateNotification();
	void ReloadNotifications();
	bool AddNotification(
		const std::string &DevIdx,
		const std::string &Param,
		const std::string &CustomMessage,
		const std::string &ActiveSystems,
		const int Priority,
		const bool SendAlways);
	bool RemoveDeviceNotifications(const std::string &DevIdx);
	bool RemoveNotification(const std::string &ID);
	std::vector<_tNotification> GetNotifications(const uint64_t DevIdx);
	std::vector<_tNotification> GetNotifications(const std::string &DevIdx);
	void TouchNotification(const uint64_t ID);
	void TouchLastUpdate(const uint64_t ID);
	bool CustomRecoveryMessage(const uint64_t ID, std::string &msg, const bool isRecovery);
	bool HasNotifications(const uint64_t DevIdx);
	bool HasNotifications(const std::string &DevIdx);

	bool CheckAndHandleNotification(
		const uint64_t DevRowIdx,
		const int HardwareID,
		const std::string &ID,
		const std::string &sName,
		const unsigned char unit,
		const unsigned char cType,
		const unsigned char cSubType,
		const int nValue);
	bool CheckAndHandleNotification(
		const uint64_t DevRowIdx,
		const int HardwareID,
		const std::string &ID,
		const std::string &sName,
		const unsigned char unit,
		const unsigned char cType,
		const unsigned char cSubType,
		const float fValue);
	bool CheckAndHandleNotification(
		const uint64_t DevRowIdx,
		const int HardwareID,
		const std::string &ID,
		const std::string &sName,
		const unsigned char unit,
		const unsigned char cType,
		const unsigned char cSubType,
		const std::string &sValue);
	bool CheckAndHandleNotification(
		const uint64_t DevRowIdx,
		const int HardwareID,
		const std::string &ID,
		const std::string &sName,
		const unsigned char unit,
		const unsigned char cType,
		const unsigned char cSubType,
		const int nValue,
		const std::string &sValue);

	bool CheckAndHandleNotification(
		const uint64_t Idx,
		const std::string &DeviceName,
		const unsigned char devType,
		const unsigned char subType,
		const _eNotificationTypes ntype,
		const float mvalue);
	bool CheckAndHandleNotification(
		const uint64_t Idx,
		const std::string &DeviceName,
		const _eNotificationTypes ntype,
		const std::string &message);

	bool CheckAndHandleSwitchNotification(
		const uint64_t Idx,
		const std::string &DeviceName,
		const _eNotificationTypes ntype);
	bool CheckAndHandleSwitchNotification(
		const uint64_t Idx,
		const std::string & DeviceName,
		const _eNotificationTypes ntype,
		const int llevel);

	std::map<std::string, CNotificationBase*> m_notifiers;
	void AddNotifier(CNotificationBase *notifier);
	void RemoveNotifier(CNotificationBase * notifier);
protected:
	void SetConfigValue(const std::string &key, const std::string &value);
private:
	bool CheckAndHandleNotification(
		const uint64_t DevRowIdx,
		const int HardwareID,
		const std::string &ID,
		const std::string &sName,
		const unsigned char unit,
		const unsigned char cType,
		const unsigned char cSubType,
		const int nValue,
		const std::string &sValue,
		const float fValue);

	bool CheckAndHandleTempHumidityNotification(
		const uint64_t Idx,
		const std::string &DeviceName,
		const float temp,
		const int humidity,
		const bool bHaveTemp,
		const bool bHaveHumidity);
	bool CheckAndHandleDewPointNotification(
		const uint64_t Idx,
		const std::string &DeviceName,
		const float temp,
		const float dewpoint);
	bool CheckAndHandleValueNotification(
		const uint64_t Idx,
		const std::string &DeviceName,
		const int value);
	bool CheckAndHandleRainNotification(
		const uint64_t Idx,
		const std::string &DeviceName,
		const unsigned char devType,
		const unsigned char subType,
		const _eNotificationTypes ntype,
		const float mvalue);
	bool CheckAndHandleAmpere123Notification(
		const uint64_t Idx,
		const std::string &DeviceName,
		const float Ampere1,
		const float Ampere2,
		const float Ampere3
		);

	std::string ParseCustomMessage(const std::string &cMessage, const std::string &sName, const std::string &sValue);
	bool ApplyRule(const std::string &rule, const bool equal, const bool less);
	std::mutex m_mutex;
	std::map<uint64_t, std::vector<_tNotification> > m_notifications;
	int m_NotificationSensorInterval;
	int m_NotificationSwitchInterval;
};

extern CNotificationHelper m_notifications;
