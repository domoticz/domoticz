#pragma once

#include <string>
#include <vector>

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
	std::vector<std::string> m_SIDs;
	CSQLHelper m_sql;

      public:
	XiaomiDeviceSupportInjected(std::string devicesCommaSeparated);
	~XiaomiDeviceSupportInjected(){};
	std::string GetXiaomiDeviceModelByID(std::string sid);
};
