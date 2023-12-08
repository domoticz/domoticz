#pragma once
#include <string>
#include <cstdint>

class CDomoticzHardwareBase;

class CounterHelper
{
public:
	CounterHelper();
	~CounterHelper();
	void Init(const std::string &szUservariableName, CDomoticzHardwareBase *pHardwareBase);
	void SendKwhMeter(int NodeID, int ChildID, int BatteryLevel, double musage, double mtotal, const std::string& defaultname, int RssiLevel = 12);
	double SetCounterValue(const double nNewCounbterValue);
	double GetCounterValue() const { return m_nLastCounterValue; };
	void Reset();
private:
	std::string m_szUservariableName;
	CDomoticzHardwareBase* m_pHardwareBase = nullptr;

	double m_nLastCounterValue = 0;
	double m_CounterOffset = 0;
};

