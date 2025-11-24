#include "stdafx.h"
#include "CounterHelper.h"

#include "DomoticzHardware.h"
#include "../main/SQLHelper.h"
#include "../main/Logger.h"

bool CounterHelper::dummy_looped_boolean = false;

CounterHelper::CounterHelper()
{
}

CounterHelper::~CounterHelper()
{
}

void CounterHelper::Reset()
{
	m_nLastCounterValue = 0;
	m_CounterOffset = 0;
	if (!m_bInitialized)
		return;
	m_sql.safe_query("UPDATE DeviceStatus SET LastLevel=0, LastUpdate='%s' WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d) AND (Type=%d) AND (SubType=%d)",
		TimeToString(nullptr, TF_DateTime).c_str(),
		m_HwdID, m_szID.c_str(), m_Unit,
		pTypeGeneral, sTypeKwh
		);
}

void CounterHelper::Init(const CDomoticzHardwareBase* pHardwareBase, const int NodeID, const int ChildID, const uint8_t Unit)
{
	if (m_bInitialized)
		return; //Allready initialized

	m_HwdID = pHardwareBase->m_HwdID;
	m_Unit = Unit;
	m_szID = std_format("%08X", (unsigned int)(NodeID << 8) | ChildID);

	InitInt();
}

void CounterHelper::Init(const CDomoticzHardwareBase* pHardwareBase, const std::string& szDeviceID, const uint8_t Unit)
{
	if (m_bInitialized)
		return; //Allready initialized

	m_HwdID = pHardwareBase->m_HwdID;
	m_Unit = Unit;
	m_szID = szDeviceID;

	InitInt();
}

void CounterHelper::InitInt()
{
	auto result = m_sql.safe_query("SELECT sValue, LastLevel FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d) AND (Type=%d) AND (SubType=%d)",
		m_HwdID, m_szID.c_str(), m_Unit,
		pTypeGeneral, sTypeKwh);
	if (!result.empty())
	{
		std::string sValue = result[0][0];
		m_CounterOffset = std::stod(result[0][1]) / 1000.0;

		size_t pos = sValue.find(';');
		if (pos != std::string::npos)
		{
			m_nLastCounterValue = std::stod(sValue.substr(pos + 1)) / 1000.0;
		}
	}

	m_bInitialized = true;
}

double CounterHelper::CheckTotalCounter(CDomoticzHardwareBase* pHardwareBase, const int NodeID, const int ChildID, const uint8_t Unit, const double mtotal, const bool bDoReset, bool& bLooped)
{
	if (!m_bInitialized)
		Init(pHardwareBase, NodeID, ChildID, Unit);
	return CheckTotalCounter(mtotal, bDoReset, bLooped);
}

double CounterHelper::CheckTotalCounter(CDomoticzHardwareBase* pHardwareBase, const std::string& szDeviceID, const uint8_t Unit, const double mtotal, const bool bDoReset, bool& bLooped)
{
	if (!m_bInitialized)
		Init(pHardwareBase, szDeviceID, Unit);
	return CheckTotalCounter(mtotal, bDoReset, bLooped);
}

double CounterHelper::CheckTotalCounter(const double mtotal, const bool bDoReset, bool& bLooped)
{
	double rTotal = m_CounterOffset + mtotal;
	if (
		(rTotal < m_nLastCounterValue)
		&& (m_nLastCounterValue != 0)
		)
	{
		bLooped = true;
		if (bDoReset)
		{
			m_CounterOffset = m_nLastCounterValue;

			m_sql.safe_query("UPDATE DeviceStatus SET LastLevel=%lld, LastUpdate='%s' WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d) AND (Type=%d) AND (SubType=%d)",
				static_cast<long long int>(m_CounterOffset * 1000.0), TimeToString(nullptr, TF_DateTime).c_str(),
				m_HwdID, m_szID.c_str(), m_Unit,
				pTypeGeneral, sTypeKwh);

			rTotal = m_CounterOffset + mtotal;
		}
	}
	if (!bDoReset && bLooped)
		return rTotal;
	m_nLastCounterValue = rTotal;
	return rTotal;
}