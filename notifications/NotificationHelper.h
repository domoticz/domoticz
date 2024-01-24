#pragma once
#include "NotificationBase.h"
#include "../webserver/cWebem.h"

#include <string>

#define NOTIFYALL std::string("")

struct _tNotification
{
	uint64_t ID;
	bool Active;
	std::string Params;
	int Priority;
	time_t LastSend;
	time_t LastUpdate;
	std::string DeviceName;
	std::string CustomMessage;
	std::string CustomAction;
	std::string ActiveSystems;
	bool SendAlways;
};

class CNotificationHelper
{
      public:
	CNotificationHelper();
	~CNotificationHelper();
	void Init();
	bool SendMessage(uint64_t Idx, const std::string &Name, const std::string &Subsystems, const std::string& CustomAction, const std::string &Subject, const std::string &Text, const std::string &ExtraData, int Priority,
			 const std::string &Sound, bool bFromNotification);
	bool SendMessageEx(uint64_t Idx, const std::string &Name, const std::string &Subsystems, const std::string& CustomAction, const std::string &Subject, const std::string &Text, const std::string &ExtraData, int Priority,
			   const std::string &Sound, bool bFromNotification);
	void LoadConfig();
	void ConfigFromGetvars(const http::server::request &req, bool save);
	bool IsInConfig(const std::string &Key);

	// notification functions
	void CheckAndHandleLastUpdateNotification();
	void ReloadNotifications();
	bool AddNotification(const std::string &DevIdx, const bool Avtive, const std::string &Param, const std::string &CustomMessage, const std::string& CustomAction, const std::string &ActiveSystems, int Priority, bool SendAlways);
	bool RemoveDeviceNotifications(const std::string &DevIdx);
	bool RemoveNotification(const std::string &ID);
	std::vector<_tNotification> GetNotifications(uint64_t DevIdx, const bool bActiveOnly = true);
	void TouchNotification(uint64_t ID);
	void TouchLastUpdate(uint64_t ID);
	bool CustomRecoveryMessage(uint64_t ID, std::string &msg, bool isRecovery);
	bool HasNotifications(uint64_t DevIdx);
	bool HasNotifications(const std::string &DevIdx);

	bool CheckAndHandleNotification(uint64_t DevRowIdx, int HardwareID, const std::string &ID, const std::string &sName, unsigned char unit, unsigned char cType, unsigned char cSubType,
					int nValue);
	bool CheckAndHandleNotification(uint64_t DevRowIdx, int HardwareID, const std::string &ID, const std::string &sName, unsigned char unit, unsigned char cType, unsigned char cSubType,
					float fValue);
	bool CheckAndHandleNotification(uint64_t DevRowIdx, int HardwareID, const std::string &ID, const std::string &sName, unsigned char unit, unsigned char cType, unsigned char cSubType,
					const std::string &sValue);
	bool CheckAndHandleNotification(uint64_t DevRowIdx, int HardwareID, const std::string &ID, const std::string &sName, unsigned char unit, unsigned char cType, unsigned char cSubType,
					int nValue, const std::string &sValue);

	bool CheckAndHandleNotification(uint64_t Idx, const std::string &DeviceName, unsigned char devType, unsigned char subType, _eNotificationTypes ntype, float mvalue);
	bool CheckAndHandleNotification(uint64_t Idx, const std::string &DeviceName, _eNotificationTypes ntype, const std::string &message);

	bool CheckAndHandleSwitchNotification(uint64_t Idx, const std::string &DeviceName, _eNotificationTypes ntype);
	bool CheckAndHandleSwitchNotification(uint64_t Idx, const std::string &DeviceName, _eNotificationTypes ntype, int llevel);

	std::map<std::string, CNotificationBase *> m_notifiers;
	void AddNotifier(CNotificationBase *notifier);
	void RemoveNotifier(CNotificationBase *notifier);

      protected:
	void SetConfigValue(const std::string &key, const std::string &value);

      private:
	bool CheckAndHandleNotification(uint64_t DevRowIdx, int HardwareID, const std::string &ID, const std::string &sName, unsigned char unit, unsigned char cType, unsigned char cSubType,
					int nValue, const std::string &sValue, float fValue);

	bool CheckAndHandleTempHumidityNotification(uint64_t Idx, const std::string &DeviceName, float temp, int humidity, bool bHaveTemp, bool bHaveHumidity);
	bool CheckAndHandleDewPointNotification(uint64_t Idx, const std::string &DeviceName, float temp, float dewpoint);
	bool CheckAndHandleValueNotification(uint64_t Idx, const std::string &DeviceName, int value);
	bool CheckAndHandleRainNotification(uint64_t Idx, const std::string &DeviceName, unsigned char devType, unsigned char subType, _eNotificationTypes ntype, float mvalue);
	bool CheckAndHandleAmpere123Notification(uint64_t Idx, const std::string &DeviceName, float Ampere1, float Ampere2, float Ampere3);

	std::string ParseCustomMessage(const std::string &cMessage, const std::string &sName, const std::string &sValue);
	bool ApplyRule(const std::string &rule, bool equal, bool less);
	std::mutex m_mutex;
	std::map<uint64_t, std::vector<_tNotification>> m_notifications;
	int m_NotificationSensorInterval;
	int m_NotificationSwitchInterval;
};

extern CNotificationHelper m_notifications;
