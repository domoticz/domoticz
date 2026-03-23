#include "stdafx.h"
#include "CounterHelper.h"

#include "DomoticzHardware.h"
#include "../main/SQLHelper.h"
#include "../main/Logger.h"
#include <ctime>

// Maximum time (in seconds) a pending reset confirmation remains valid.
// If no confirming reading arrives within this window, the pending reset is discarded.
constexpr int PENDING_RESET_TIMEOUT_SECONDS = 300;

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
	m_bPendingReset = false;
	m_pendingOffset = 0;
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
		return; // Already initialized

	if (pHardwareBase == nullptr)
	{
		_log.Log(LOG_ERROR, "CounterHelper: Init called with null hardware base pointer");
		return;
	}

	m_HwdID = pHardwareBase->m_HwdID;
	m_Unit = Unit;
	m_szID = std_format("%08X", (unsigned int)(NodeID << 8) | ChildID);

	InitInt();
}

void CounterHelper::Init(const CDomoticzHardwareBase* pHardwareBase, const std::string& szDeviceID, const uint8_t Unit)
{
	if (m_bInitialized)
		return; // Already initialized

	if (pHardwareBase == nullptr)
	{
		_log.Log(LOG_ERROR, "CounterHelper: Init called with null hardware base pointer");
		return;
	}

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

		try
		{
			m_CounterOffset = std::stod(result[0][1]) / 1000.0;
		}
		catch (const std::exception& e)
		{
			_log.Log(LOG_ERROR, "CounterHelper: Failed to parse counter offset from DB value '%s': %s", result[0][1].c_str(), e.what());
			m_CounterOffset = 0;
		}

		size_t pos = sValue.find(';');
		if (pos != std::string::npos)
		{
			try
			{
				m_nLastCounterValue = std::stod(sValue.substr(pos + 1)) / 1000.0;
			}
			catch (const std::exception& e)
			{
				_log.Log(LOG_ERROR, "CounterHelper: Failed to parse last counter value from DB value '%s': %s", sValue.c_str(), e.what());
				m_nLastCounterValue = 0;
			}
		}

		// Sanity check: The offset should always be less than the combined total.
		// If offset >= total, the data is corrupted (e.g., LastLevel was incorrectly
		// set to the total value instead of the offset). In this case, reset the offset
		// to 0 to prevent counter values from doubling after restart.
		if ((m_CounterOffset > 0) && (m_nLastCounterValue > 0) && (m_CounterOffset >= m_nLastCounterValue))
		{
			_log.Log(LOG_ERROR, "CounterHelper: Detected corrupted counter data (offset %.3f >= total %.3f). Resetting offset to 0.",
				m_CounterOffset, m_nLastCounterValue);
			m_CounterOffset = 0;
			m_sql.safe_query("UPDATE DeviceStatus SET LastLevel=0, LastUpdate='%s' WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d) AND (Type=%d) AND (SubType=%d)",
				TimeToString(nullptr, TF_DateTime).c_str(),
				m_HwdID, m_szID.c_str(), m_Unit,
				pTypeGeneral, sTypeKwh);
		}
	}

	m_bInitialized = true;
}

double CounterHelper::CheckTotalCounter(CDomoticzHardwareBase* pHardwareBase, const int NodeID, const int ChildID, const uint8_t Unit, const double mtotal, bool& bLooped)
{
	if (pHardwareBase == nullptr)
		return 0;
	if (!m_bInitialized)
		Init(pHardwareBase, NodeID, ChildID, Unit);
	return CheckTotalCounter(mtotal, bLooped);
}

double CounterHelper::CheckTotalCounter(CDomoticzHardwareBase* pHardwareBase, const std::string& szDeviceID, const uint8_t Unit, const double mtotal, bool& bLooped)
{
	if (pHardwareBase == nullptr)
		return 0;
	if (!m_bInitialized)
		Init(pHardwareBase, szDeviceID, Unit);
	return CheckTotalCounter(mtotal, bLooped);
}

double CounterHelper::CheckTotalCounter(CDomoticzHardwareBase* pHardwareBase, const int NodeID, const int ChildID, const uint8_t Unit, const double mtotal)
{
	bool bDummyLooped = false;
	return CheckTotalCounter(pHardwareBase, NodeID, ChildID, Unit, mtotal, bDummyLooped);
}

double CounterHelper::CheckTotalCounter(CDomoticzHardwareBase* pHardwareBase, const std::string& szDeviceID, const uint8_t Unit, const double mtotal)
{
	bool bDummyLooped = false;
	return CheckTotalCounter(pHardwareBase, szDeviceID, Unit, mtotal, bDummyLooped);
}

double CounterHelper::CheckTotalCounter(const double mtotal, bool& bLooped)
{
	if (mtotal == 0)
	{
		_log.Log(LOG_STATUS, "CounterHelper: Received 0 reading, returning cached value (%.3f) to avoid DB corruption", m_nLastCounterValue);
		return m_nLastCounterValue; //ignore 0 readings, return last known value to avoid corrupting the DB
	}

	double rTotal = m_CounterOffset + mtotal;

	if (m_bPendingReset)
	{
		m_bPendingReset = false;

		// Check if the pending reset has expired
		time_t now = time(nullptr);
		bool bExpired = (now - m_pendingResetTime) > PENDING_RESET_TIMEOUT_SECONDS;

		if (bExpired)
		{
			// Pending reset timed out, discard it and treat current reading normally
			_log.Log(LOG_STATUS, "CounterHelper: Pending counter reset expired after %d seconds, discarding", static_cast<int>(now - m_pendingResetTime));
		}
		else if ((rTotal < m_nLastCounterValue) && (m_nLastCounterValue != 0))
		{
			// Counter is still below the expected value on second consecutive reading
			// This confirms a genuine counter reset
			bLooped = true;
			m_CounterOffset = m_pendingOffset;

			m_sql.safe_query("UPDATE DeviceStatus SET LastLevel=%lld, LastUpdate='%s' WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d) AND (Type=%d) AND (SubType=%d)",
				static_cast<long long int>(m_CounterOffset * 1000.0), TimeToString(nullptr, TF_DateTime).c_str(),
				m_HwdID, m_szID.c_str(), m_Unit,
				pTypeGeneral, sTypeKwh);

			rTotal = m_CounterOffset + mtotal;
		}
		// else: counter recovered to expected value, the drop was temporary - no offset needed
	}
	else if ((rTotal < m_nLastCounterValue) && (m_nLastCounterValue != 0))
	{
		// Counter value decreased - potential reset, but wait for next reading to confirm
		m_bPendingReset = true;
		m_pendingOffset = m_nLastCounterValue;
		m_pendingResetTime = time(nullptr);

		// Return last known good value while waiting for confirmation
		return m_nLastCounterValue;
	}

	m_nLastCounterValue = rTotal;
	return rTotal;
}
