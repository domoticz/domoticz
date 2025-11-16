#include "stdafx.h"
#include "CounterHelper.h"

#include "DomoticzHardware.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "../main/Logger.h"
#include "../notifications/NotificationHelper.h"

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

void CounterHelper::SendKwhMeter(CDomoticzHardwareBase* pHardwareBase, const int NodeID, const int ChildID, const uint8_t Unit, const int BatteryLevel, const double musage, const double mtotal, const std::string& defaultname, const int RssiLevel)
{
	if (!m_bInitialized)
		Init(pHardwareBase, NodeID, ChildID, Unit);
	if (mtotal == 0)
		return;
	double rTotal = CheckTotalCounter(mtotal);
	pHardwareBase->SendKwhMeter(NodeID, ChildID, BatteryLevel, musage, static_cast<double>(rTotal), defaultname, RssiLevel);
}

uint64_t CounterHelper::UpdateValueInt(CDomoticzHardwareBase* pHardwareBase, const char* ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue,
	const char* sValue, std::string& devname, bool bUseOnOffAction, const std::string& user)
{
	uint64_t DeviceRowIdx = m_sql.UpdateValue(pHardwareBase->m_HwdID, 0, ID, unit, devType, subType, signallevel, batterylevel, nValue, sValue, devname, bUseOnOffAction, (!user.empty()) ? user.c_str() : pHardwareBase->m_Name.c_str());
	if (DeviceRowIdx == (uint64_t)-1)
		return -1;
	if (pHardwareBase->m_bOutputLog)
	{
		std::string szLogString = RFX_Type_Desc(devType, 1) + std::string("/") + std::string(RFX_Type_SubType_Desc(devType, subType)) + " (" + devname + ")";
		pHardwareBase->Log(LOG_NORM, szLogString);
	}
	m_mainworker.sOnDeviceReceived(m_HwdID, DeviceRowIdx, devname, nullptr);
	m_notifications.CheckAndHandleNotification(DeviceRowIdx, m_HwdID, ID, devname, unit, devType, subType, nValue, sValue);
	m_mainworker.CheckSceneCode(DeviceRowIdx, devType, subType, nValue, sValue, "MQTT Auto");
	return DeviceRowIdx;
}

void CounterHelper::SendKwhMeter(CDomoticzHardwareBase* pHardwareBase, const std::string& szDeviceID, const uint8_t Unit, const int BatteryLevel, const double musage, const double mtotal, std::string& defaultname, const int RssiLevel, const int iUsed)
{
	if (!m_bInitialized)
		Init(pHardwareBase, szDeviceID, Unit);
	if (mtotal == 0)
		return;

	std::string sValue = std_format("%.3f;%.3f", musage, CheckTotalCounter(mtotal));

	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)", m_HwdID,
		szDeviceID.c_str(), Unit, pTypeGeneral, sTypeKwh);
	if (result.empty())
	{
		//Insert
		if (!m_sql.m_bAcceptNewHardware)
		{
			pHardwareBase->Log(LOG_NORM, "Accept new hardware disabled. Ignoring new sensor %s", defaultname.c_str());
			return;
		}

		m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, OrgHardwareID, DeviceID, Unit, Type, SubType, SignalLevel, BatteryLevel, Name, Used, Options, nValue, sValue) "
			"VALUES (%d, %d, '%q', %d, %d, %d, %d, %d, %d, '%q', %d, '1;%q', %d, '%q')",
			m_HwdID, 0, szDeviceID, Unit, pTypeGeneral, sTypeKwh, RssiLevel, BatteryLevel, defaultname.c_str(), iUsed,
			"", 0, sValue.c_str());
	}
	else
	{
		UpdateValueInt(pHardwareBase, szDeviceID.c_str(), Unit, pTypeGeneral, sTypeKwh, RssiLevel, BatteryLevel, 0,
			sValue.c_str(), defaultname);
	}
}

double CounterHelper::CheckTotalCounter(const double mtotal)
{
	double rTotal = m_CounterOffset + mtotal;
	if (
		(rTotal < m_nLastCounterValue)
		&& (m_nLastCounterValue != 0)
		)
	{
		m_CounterOffset = m_nLastCounterValue;

		m_sql.safe_query("UPDATE DeviceStatus SET LastLevel=%lld, LastUpdate='%s' WHERE WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d) AND (Type=%d) AND (SubType=%d)",
			static_cast<long long int>(m_CounterOffset * 1000.0), TimeToString(nullptr, TF_DateTime).c_str(),
			m_HwdID, m_szID.c_str(), m_Unit,
			pTypeGeneral, sTypeKwh);

		rTotal = m_CounterOffset + mtotal;
	}
	m_nLastCounterValue = rTotal;
	return rTotal;
}