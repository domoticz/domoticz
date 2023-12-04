#pragma once
#include "BasePush.h"

#include "../hardware/MQTT.h"

class CMQTTPush : public CBasePush, MQTT
{
public:
	CMQTTPush();
	bool Start();
	void Stop();
	void UpdateSettings();
private:
	struct _tPushItem
	{
		std::string skey;
		time_t stimestamp;
		std::string svalue;
	};
	void OnDeviceReceived(int m_HwdID, uint64_t DeviceRowIdx, const std::string& DeviceName, const unsigned char* pRXCommand);
	void DoMQTTPush(const uint64_t DeviceRowIdx, const bool bForced = false);

	std::shared_ptr<std::thread> m_thread;
	std::mutex m_background_task_mutex;
	void Do_Work();

	std::map<std::string, _tPushItem> m_PushedItems;
	std::vector<_tPushItem> m_background_task_queue;

	std::string m_szIPAddress;
	int m_usIPPort;
	std::string m_UserName;
	std::string m_Password;
	std::string m_CAFilename;
	int m_TLS_Version;
	std::string m_TopicOut;
};
extern CMQTTPush m_mqttpush;
