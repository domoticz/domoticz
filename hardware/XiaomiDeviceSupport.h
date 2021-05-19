#pragma once

#include <string>

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
