#include "stdafx.h"
#include "CounterHelper.h"

#include "DomoticzHardware.h"
#include "../main/SQLHelper.h"

CounterHelper::CounterHelper()
{
}

CounterHelper::~CounterHelper()
{
}

void CounterHelper::Init(const std::string& szUservariableName, CDomoticzHardwareBase* pHardwareBase)
{
	m_szUservariableName = szUservariableName;
	m_pHardwareBase = pHardwareBase;
	auto result = m_sql.safe_query("SELECT ID, Value FROM UserVariables WHERE (Name=='%q')", m_szUservariableName.c_str());
	if (result.empty())
	{
		m_sql.safe_query("INSERT INTO UserVariables (Name, ValueType, Value) VALUES ('%q',%d,'%q')", m_szUservariableName.c_str(), USERVARTYPE_STRING, "0");
		result = m_sql.safe_query("SELECT ID, Value FROM UserVariables WHERE (Name=='%q')", m_szUservariableName.c_str());
	}
	if (!result.empty())
	{
		m_CounterOffset = std::stod(result[0][1]);
	}
}

void CounterHelper::SendKwhMeter(int NodeID, int ChildID, int BatteryLevel, double musage, double mtotal, const std::string& defaultname, int RssiLevel)
{
	if (mtotal != 0)
	{
		double rTotal = m_CounterOffset + mtotal;
		if (
			(rTotal < m_nLastCounterValue)
			&& (m_nLastCounterValue != 0)
			)
		{
			m_CounterOffset = m_nLastCounterValue;

			m_sql.safe_query("UPDATE UserVariables SET Value='%q', LastUpdate='%s' WHERE (Name=='%q')", std::to_string(m_CounterOffset).c_str(), TimeToString(nullptr, TF_DateTime).c_str(), m_szUservariableName.c_str());

			rTotal = m_CounterOffset + mtotal;
		}
		m_pHardwareBase->SendKwhMeter(NodeID, ChildID, BatteryLevel, musage, static_cast<double>(rTotal), defaultname, RssiLevel);
		m_nLastCounterValue = rTotal;
	}

}