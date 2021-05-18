#include "stdafx.h"

#include "XiaomiDeviceSupport.h"

int32_t XiaomiDeviceSupportHardcoded::GetXiaomiUnitCodeByID(std::string sid)
{
	int32_t unitcodefound = 0;

	if (sid == "158d00042490a5")
	{
		unitcodefound = 11;
	}
	else if (sid == "158d00045c25cf" || sid == "158d0003cbe6e8")
	{
		unitcodefound = 12;
	}

	return unitcodefound;
}

std::string XiaomiDeviceSupportHardcoded::GetXiaomiDeviceNameByID(std::string sid)
{
	// NAME_ACT_LIGHT "Light bulb"
	// NAME_ACT_RELAIS "Aqara Wireless Relay Controller"
	std::string name;

	if (sid == "158d00042490a5")
	{
		name = "Light bulb";
	}
	else if (sid == "158d00045c25cf" || sid == "158d0003cbe6e8")
	{
		name = "Aqara Wireless Relay Controller";
	}

	return name;
}

std::string XiaomiDeviceSupportHardcoded::GetXiaomiDeviceModelByID(std::string sid)
{
	// MODEL_ACT_LIGHT "light.aqcb02"
	// MODEL_ACT_RELAIS "relay.c2acn01"
	std::string model;

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