#pragma once

#include <string>

class XiaomiDeviceSupport
{
      public:
	virtual int32_t GetXiaomiUnitCodeByID();
	virtual std::string GetXiaomiDeviceNameByID();
	virtual std::string GetXiaomiDeviceModelByID();
};

class XiaomiDeviceSupportHardcoded : public XiaomiDeviceSupport
{
	int32_t GetXiaomiUnitCodeByID();
	std::string GetXiaomiDeviceNameByID();
	std::string GetXiaomiDeviceModelByID();
};
