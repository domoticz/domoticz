#pragma once
#include <string>
#include <cstdint>

class CDomoticzHardwareBase;

class CounterHelper
{
public:
	CounterHelper();
	~CounterHelper();
	double CheckTotalCounter(CDomoticzHardwareBase* pHardwareBase, const int NodeID, const int ChildID, const uint8_t Unit, const double mtotal);
	double CheckTotalCounter(CDomoticzHardwareBase* pHardwareBase, const std::string& szDeviceID, const uint8_t Unit, const double mtotal);
	void Reset();
	double GetOffset() const { return m_CounterOffset; }
private:
	void Init(const CDomoticzHardwareBase* pHardwareBase, const int NodeID, const int ChildID, const uint8_t Unit = 1);
	void Init(const CDomoticzHardwareBase* pHardwareBase, const std::string& szDeviceID, const uint8_t Unit = 1);
	void InitInt();
	double CheckTotalCounter(const double mtotal, bool &bLooped);

	bool m_bInitialized = false;
	int m_HwdID = 0;
	uint8_t m_Unit = 1;

	std::string m_szID;

	double m_CounterOffset = 0;
	double m_nLastCounterValue = 0;
};

