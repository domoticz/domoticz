#include <string>
#include <vector>
#include <sstream>
#include "XiaomiDeviceSupport.h"
#include "../main/SQLHelper.h"
#include "../main/Logger.h"

std::string XiaomiDeviceSupportHardcoded::GetXiaomiDeviceModelByID(std::string sid)
{
	// MODEL_ACT_LIGHT "light.aqcb02"
	// MODEL_ACT_RELAIS "relay.c2acn01"
	std::string model;

	// These are hardcoded IDs that need to be replaced by the devices that you want to support.
	if (sid == "158d00042490a5")
	{
		model = "light.aqcb02";
	}
	else if (sid == "158d00045c25cf" || sid == "158d0003cbe6e8")
	{
		model = "relay.c2acn01";
	}

	return model;
}

XiaomiDeviceSupportUserVariable::XiaomiDeviceSupportUserVariable(CSQLHelper &sql, CLogger &log, std::string model)
	: m_SQL(sql)
	, m_Log(log)
{
	m_SIDs = NULL;
	m_Model = model;
	if (model == "relay.c2acn0")
	{
		m_UserVarName = "XiaomiDeviceSupportRelais";
	}
	else if (model == "light.aqcb02")
	{
		m_UserVarName = "XiaomiDeviceSupportLight";
	}
	else
	{
		m_UserVarName = "XiaomiDeviceSupport";
	}
}

std::string XiaomiDeviceSupportUserVariable::GetXiaomiDeviceModelByID(std::string sid)
{
	if (LoadSIDsOk())
	{
		if (std::find(m_SIDs->begin(), m_SIDs->end(), sid) != m_SIDs->end())
		{
			_log.Log(LOG_DEBUG_INT, "Found m_model for: '%s'", sid);
			return m_Model;
		}
	}
	return "";
}

bool XiaomiDeviceSupportUserVariable::LoadSIDsOk()
{
	if (m_SIDs == NULL)
	{
		std::vector<std::vector<std::string>> result;
		m_SIDs = new std::vector<std::string>();

		// Check for relay devices to override
		result = m_sql.safe_query("SELECT Value FROM UserVariables WHERE (Name == '%s')", m_UserVarName);
		if (!result.empty())
		{
			std::stringstream ss(result[0][0]);

			while (ss.good())
			{
				std::string substr;
				getline(ss, substr, ',');
				m_SIDs->push_back(substr);
				m_Log.Log(LOG_DEBUG_INT, "XiaomiDeviceSupportUserVariable found: '%s'", substr.c_str());
			}
		}
	}
	if (m_SIDs->empty())
	{
		m_Log.Log(LOG_DEBUG_INT, "XiaomiDeviceSupportUserVariable SIDs empty");
		return false;
	}
	else
	{
		return true;
	}
}