#pragma once
#include <string>
#include <cstdint>

class CDomoticzHardwareBase;

class CounterHelper
{
public:
	CounterHelper();
	~CounterHelper();
	void SendKwhMeter(CDomoticzHardwareBase* pHardwareBase, const int NodeID, const int ChildID, const uint8_t Unit, const int BatteryLevel, const double musage, const double mtotal, const std::string& defaultname, const int RssiLevel = 12);
	void SendKwhMeter(CDomoticzHardwareBase* pHardwareBase, const std::string& szDeviceID, const uint8_t Unit, const int BatteryLevel, const double musage, const double mtotal, std::string& defaultname, const int RssiLevel = 12, const int iUsed = 1);
	void Reset();
private:
	void Init(const CDomoticzHardwareBase* pHardwareBase, const int NodeID, const int ChildID, const uint8_t Unit = 1);
	void Init(const CDomoticzHardwareBase* pHardwareBase, const std::string& szDeviceID, const uint8_t Unit = 1);
	void InitInt();
	double CheckTotalCounter(const double mtotal);
	uint64_t CounterHelper::UpdateValueInt(CDomoticzHardwareBase* pHardwareBase, const char* ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue,
		const char* sValue, std::string& devname, bool bUseOnOffAction = true, const std::string& user = "");

	bool m_bInitialized = false;
	int m_HwdID = 0;
	uint8_t m_Unit = 1;

	std::string m_szID;

	double m_CounterOffset = 0;
	double m_nLastCounterValue = 0;
};

