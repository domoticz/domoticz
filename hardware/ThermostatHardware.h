#pragma once

#include <iostream>
#include "DomoticzHardware.h"

class CThermostatHardware : public CDomoticzHardwareBase
{
public:

	//thermostat available mode in string "OFF,ECO,COMFORT,AUTO," separated by comma, 
	// and in the order from 0 to N and 0 = OFF 1 = ECO, 2 = CONFOR 3 = AUTO
	std::string AvailableMode;

	//thermostat function
	//return the thermostat available mode in string "OFF,ECO,CONFOR,AUTO," separated by comma , and 0=OFF 1=ECO, 2=CONFOR 3=AUTO
	virtual std::string GetAvailableMode() { return AvailableMode; };
	virtual void SetAvailableMode(std::string pAvailableMode) { AvailableMode = pAvailableMode; };
	//return the thermostat mode 
	virtual std::string GetCurrentMode(std::string& devIdx) = 0;
	//return the thermostat room temperature 
	virtual std::string GetRoomTemperature(std::string& devIdx) = 0;
	//return the thermostat current setpoint 
	virtual std::string GetSetPoint(std::string& devIdx) = 0;
	//set the thermostat mode : newState = integer thermostat mode , in the order in AvailableMode
	virtual bool SetThermostatState(const std::string& devIdx, const int newState) = 0;

	//convert thermostat string state to int state : OFF-->0  ECO-->1
	int  ThermostatModeStringToInt(std::string& mode)
	{
		std::vector<std::string> ModeStr;
		StringSplit(AvailableMode, ",", ModeStr);

		for (int i = 0; i < (int)ModeStr.size(); i++)
			if (strcmp(mode.c_str(), ModeStr[i].c_str()) == 0)
				return  i;
		return 0;

	}
	//convert interger thermostat state to string state : 0--> OFF 1-->ECO
	std::string   ThermostatModeIntToString(int Mode)
	{
		std::vector<std::string> ModeStr;
		StringSplit(AvailableMode, ",", ModeStr);
		if (Mode < (int)ModeStr.size())
			return ModeStr[Mode];
		else
			return "UNKNOWN";

	}


};

