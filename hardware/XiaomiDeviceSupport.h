#pragma once

#include <string>
#include <vector>
#include "../main/SQLHelper.h"
#include "../main/Logger.h"

//-----------------------------------------------------------------------------------------------
// This class is for supporting Xiaomi devices that do not properly 'announce' themselves
// via the Xiaomi gateway/bridge API. i.e. they do not report their 'model'.
// Currently only the Aqara Wireless Relay Controller LLKZMK11LM (relay.c2acn01) is supported.
//-----------------------------------------------------------------------------------------------

/** XiaomiDeviceSupport interface */
class XiaomiDeviceSupport
{
      public:
	virtual ~XiaomiDeviceSupport() = default;
	/**
	 * Method to get 'model' corresponding to the ID of the device in case the Gateway API didn't provide it.
	 *
	 * @param sid is the id as a string of the device
	 * @return model as string or an empty string if the ID string was not found
	 */
	virtual std::string GetXiaomiDeviceModelByID(std::string sid) = 0;
};

/** XiaomiDeviceSupport hardcoded for testing purposes only */
class XiaomiDeviceSupportHardcoded : public XiaomiDeviceSupport
{
      public:
	~XiaomiDeviceSupportHardcoded(){};
	std::string GetXiaomiDeviceModelByID(std::string sid);
};

/** XiaomiDeviceSupport that uses a user variable from the database to get the SID's to map to a model (device type) */
class XiaomiDeviceSupportUserVariable : public XiaomiDeviceSupport
{
      private:
	std::vector<std::string> *m_SIDs;
	CSQLHelper &m_SQL;
	std::string m_Model;
	std::string m_UserVarName;
	CLogger &m_Log;

      public:
	XiaomiDeviceSupportUserVariable(CSQLHelper &sql, CLogger &log, std::string model);
	~XiaomiDeviceSupportUserVariable(){};
	std::string GetXiaomiDeviceModelByID(std::string sid);
	/** Method to load SIDs from the database user variable
	 * @return true if it succesfully loaded one or more ID strings
	 */
	bool LoadSIDsOk();
};
