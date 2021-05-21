#pragma once

#include <string>
#include <vector>
#include "../main/SQLHelper.h"

class XiaomiDeviceSupport
{
      public:
	virtual std::string GetXiaomiDeviceModelByID(std::string sid) = 0;
};

class XiaomiDeviceSupportHardcoded : public XiaomiDeviceSupport
{
      public:
	~XiaomiDeviceSupportHardcoded(){};
	std::string GetXiaomiDeviceModelByID(std::string sid);
};

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
	bool LoadSIDsOk();
};
