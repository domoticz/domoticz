#pragma once

#include <string>

class XiaomiDeviceSupport
{
      public:
	virtual int32_t GetXiaomiUnitCodeByID(std::string sid) = 0;
	virtual std::string GetXiaomiDeviceNameByID(std::string sid) = 0;
	virtual std::string GetXiaomiDeviceModelByID(std::string sid) = 0;
};

class XiaomiDeviceSupportHardcoded : public XiaomiDeviceSupport
{
      public:
	~XiaomiDeviceSupportHardcoded(){};
	int32_t GetXiaomiUnitCodeByID(std::string sid);
	std::string GetXiaomiDeviceNameByID(std::string sid);
	std::string GetXiaomiDeviceModelByID(std::string sid);
};
