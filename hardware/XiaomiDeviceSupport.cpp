#include "stdafx.h"

#include "XiaomiDeviceSupport.h"

int32_t XiaomiDeviceSupportHardcoded::GetXiaomiUnitCodeByID(std::string sid)
{
	int32_t unitcodefound = 0;

	if (sid == "")
	{
		unitcodefound = 11;
	}

	return unitcodefound;
}

std::string GetXiaomiDeviceNameByID(std::string sid)
{
	return "";
}

std::string GetXiaomiDeviceModelByID(std::string sid)
{
	return "";
}