#include "stdafx.h"

#include "XiaomiDeviceSupport.h"

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