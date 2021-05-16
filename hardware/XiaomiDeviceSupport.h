#pragma once

#include <string>

class XiaomiDeviceSupport
{
      public:
	virtual int32_t GetXiaomiUnitCodeByID(std::string sid);
	virtual std::string GetXiaomiDeviceNameByID(std::string sid);
	virtual std::string GetXiaomiDeviceModelByID(std::string sid);
};

class XiaomiDeviceSupportHardcoded : public XiaomiDeviceSupport
{
      public:
	int32_t GetXiaomiUnitCodeByID(std::string sid);
	std::string GetXiaomiDeviceNameByID(std::string sid);
	std::string GetXiaomiDeviceModelByID(std::string sid);
};
