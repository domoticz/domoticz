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
	bool Start();
	void Stop();
	void UpdateSettings();

      private:
	void OnDeviceReceived(int m_HwdID, uint64_t DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand);
	void DoInfluxPush(const uint64_t DeviceRowIdx);

	std::shared_ptr<std::thread> m_thread;
	std::mutex m_background_task_mutex;
	void Do_Work();

	std::map<std::string, _tPushItem> m_PushedItems;
	std::vector<_tPushItem> m_background_task_queue;
	std::string m_szURL;
	std::string m_InfluxIP;
	int m_InfluxPort{ 8086 };
	std::string m_InfluxPath;
	std::string m_InfluxDatabase;
	std::string m_InfluxUsername;
	std::string m_InfluxPassword;
	bool m_bInfluxDebugActive{ false };
};
extern CInfluxPush m_influxpush;
