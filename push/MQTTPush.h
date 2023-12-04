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
public:
	void on_message(const struct mosquitto_message* message) override;
	void on_connect(int rc) override;
	void on_disconnect(int rc) override;
	void on_going_down();
private:
	struct _tPushItem
	{
		std::string idx;
		std::string name;
		std::string stype;
		std::string svalue;
		time_t stimestamp;
	};
	void OnDeviceReceived(int m_HwdID, uint64_t DeviceRowIdx, const std::string& DeviceName, const unsigned char* pRXCommand);
	void DoMQTTPush(const uint64_t DeviceRowIdx, const bool bForced = false);

	std::shared_ptr<std::thread> m_thread;
	std::mutex m_background_task_mutex;
	void Do_Work();

	std::map<std::string, _tPushItem> m_PushedItems;
	std::vector<_tPushItem> m_background_task_queue;

	std::string m_TopicOut;
};
extern CMQTTPush m_mqttpush;
