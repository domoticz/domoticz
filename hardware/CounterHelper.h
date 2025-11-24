#pragma once
#include <string>
#include <cstdint>

class CDomoticzHardwareBase;

class CounterHelper
{
public:
	CounterHelper();
	~CounterHelper();
	static bool dummy_looped_boolean;
	double CheckTotalCounter(CDomoticzHardwareBase* pHardwareBase, const int NodeID, const int ChildID, const uint8_t Unit, const double mtotal, const bool bDoReset = true, bool &bLooped = dummy_looped_boolean);
	double CheckTotalCounter(CDomoticzHardwareBase* pHardwareBase, const std::string& szDeviceID, const uint8_t Unit, const double mtotal, const bool bDoReset = true, bool& bLooped = dummy_looped_boolean);
	void Reset();
	double GetCounterOffset() const { return m_CounterOffset; }
	double GetLastCounterValue() const { return m_nLastCounterValue; }
private:
	void Init(const CDomoticzHardwareBase* pHardwareBase, const int NodeID, const int ChildID, const uint8_t Unit = 1);
	void Init(const CDomoticzHardwareBase* pHardwareBase, const std::string& szDeviceID, const uint8_t Unit = 1);
	void InitInt();
	double CheckTotalCounter(const double mtotal, const bool bDoReset, bool &bLooped);

	bool m_bInitialized = false;
	int m_HwdID = 0;
	uint8_t m_Unit = 1;

	std::string m_szID;

	double m_CounterOffset = 0;
	double m_nLastCounterValue = 0;
};

