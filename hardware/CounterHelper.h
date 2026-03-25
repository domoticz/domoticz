#pragma once
#include <string>
#include <cstdint>

class CDomoticzHardwareBase;

class CounterHelper
{
public:
	CounterHelper();
	~CounterHelper();
	double CheckTotalCounter(CDomoticzHardwareBase* pHardwareBase, const int NodeID, const int ChildID, const uint8_t Unit, const double mtotal, bool &bLooped);
	double CheckTotalCounter(CDomoticzHardwareBase* pHardwareBase, const std::string& szDeviceID, const uint8_t Unit, const double mtotal, bool& bLooped);
	double CheckTotalCounter(CDomoticzHardwareBase* pHardwareBase, const int NodeID, const int ChildID, const uint8_t Unit, const double mtotal);
	double CheckTotalCounter(CDomoticzHardwareBase* pHardwareBase, const std::string& szDeviceID, const uint8_t Unit, const double mtotal);
	void Reset();
	double GetCounterOffset() const { return m_CounterOffset; }
	double GetLastCounterValue() const { return m_nLastCounterValue; }
private:
	void Init(const CDomoticzHardwareBase* pHardwareBase, const int NodeID, const int ChildID, const uint8_t Unit = 1);
	void Init(const CDomoticzHardwareBase* pHardwareBase, const std::string& szDeviceID, const uint8_t Unit = 1);
	void InitInt();
	double CheckTotalCounter(const double mtotal, bool &bLooped);

	bool m_bInitialized = false;
	int m_HwdID = 0;
	uint8_t m_Unit = 1;

	std::string m_szID;

	bool m_bWarnedAbout0Received = false;

	double m_CounterOffset = 0;
	double m_nLastCounterValue = 0;

	bool m_bPendingReset = false;
	double m_pendingOffset = 0;
	time_t m_pendingResetTime = 0;
};

