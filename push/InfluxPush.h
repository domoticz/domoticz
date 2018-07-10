#pragma once
#include "BasePush.h"

class CInfluxPush : public CBasePush
{
	struct _tPushItem
	{
		std::string skey;
		time_t stimestamp;
		std::string svalue;
	};
public:
	CInfluxPush();
	void Start();
	void Stop();
	void UpdateSettings();
private:
	void OnDeviceReceived(const int m_HwdID, const uint64_t DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand);
	void DoInfluxPush();

	std::shared_ptr<std::thread> m_thread;
	std::mutex m_background_task_mutex;
	bool m_stoprequested;
	bool StartThread();
	void StopThread();
	void Do_Work();

	std::map<std::string,_tPushItem> m_PushedItems;
	std::vector<_tPushItem> m_background_task_queue;
	std::string m_szURL;
	std::string m_InfluxIP;
	int m_InfluxPort;
	std::string m_InfluxDatabase;
	std::string m_InfluxUsername;
	std::string m_InfluxPassword;
	bool m_bInfluxDebugActive;
};
extern CInfluxPush m_influxpush;
