#include "stdafx.h"
#include "RFXNames.h"
#include "RFXtrx.h"

typedef struct _STR_TABLE_SINGLE {
	unsigned long    id;
	const char   *str1;
	const char   *str2;
} STR_TABLE_SINGLE;

typedef struct _STR_TABLE_ID1_ID2 {
	unsigned long    id1;
	unsigned long    id2;
	const char   *str1;
} STR_TABLE_ID1_ID2;

const char *findTableIDSingle1 (STR_TABLE_SINGLE *t, unsigned long id)
{
	while (t->str1) {
		if (t->id == id)
			return t->str1;
		t++;
	}
	return "Unknown";
} 

const char *findTableIDSingle2 (STR_TABLE_SINGLE *t, unsigned long id)
{
	while (t->str2) {
		if (t->id == id)
			return t->str2;
		t++;
	}
	return "Unknown";
} 

const char *findTableID1ID2 (_STR_TABLE_ID1_ID2 *t, unsigned long id1, unsigned long id2)
{
	while (t->str1) {
		if ( (t->id1 == id1) && (t->id2 == id2) )
			return t->str1;
		t++;
	}
	return "Unknown";
}

const char *RFX_Humidity_Status_Desc(const unsigned char status)
{
	STR_TABLE_SINGLE	Table[] = 
	{
		{ humstat_normal, "Normal" },
		{ humstat_comfort, "Comfortable" },
		{ humstat_dry, "Dry" },
		{ humstat_wet, "Wet" },
		{  0,NULL,NULL }
	};
	return findTableIDSingle1 (Table, status);
}

const char *Security_Status_Desc(const unsigned char status)
{
	STR_TABLE_SINGLE	Table[] = 
	{
		{ sStatusNormal, "Normal" },
		{ sStatusNormalDelayed, "Normal Delayed" },
		{ sStatusAlarm, "Alarm" },
		{ sStatusAlarmDelayed, "Alarm Delayed" },
		{ sStatusMotion, "Motion" },
		{ sStatusNoMotion, "No Motion" },
		{ sStatusPanic, "Panic" },
		{ sStatusPanicOff, "Panic End" },
		{ sStatusArmAway, "Arm Away" },
		{ sStatusArmAwayDelayed, "Arm Away Delayed" },
		{ sStatusArmHome, "Arm Home" },
		{ sStatusArmHomeDelayed, "Arm Home Delayed" },
		{ sStatusDisarm, "Disarm" },
		{ sStatusLightOff, "Light Off" },
		{ sStatusLightOn, "Light On" },
		{ sStatusLight2Off, "Light 2 Off" },
		{ sStatusLight2On, "Light 2 On" },
		{ sStatusDark, "Dark detected" },
		{ sStatusLight, "Light Detected" },
		{ sStatusBatLow, "Battery low MS10 or XX18 sensor" },
		{ sStatusPairKD101, "Pair KD101" },
		{ sStatusNormalTamper, "Normal + Tamper" },
		{ sStatusNormalDelayedTamper, "Normal Delayed + Tamper" },
		{ sStatusAlarmTamper, "Alarm + Tamper" },
		{ sStatusAlarmDelayedTamper, "Alarm Delayed + Tamper" },
		{ sStatusMotionTamper, "Motion + Tamper" },
		{ sStatusNoMotionTamper, "No Motion + Tamper" },
		{ NULL, NULL }
	};
	return findTableIDSingle1 (Table, status);
}

const char *Timer_Type_Desc(int tType)
{
	STR_TABLE_SINGLE	Table[] = 
	{
		{ TTYPE_BEFORESUNRISE, "Before Sunrise" },
		{ TTYPE_AFTERSUNRISE, "After Sunrise" },
		{ TTYPE_ONTIME, "On Time" },
		{ TTYPE_BEFORESUNSET, "Before Sunset" },
		{ TTYPE_AFTERSUNSET, "After Sunset" },
		{  0,NULL,NULL }
	};
	return findTableIDSingle1 (Table, tType);
}

const char *Timer_Cmd_Desc(int tType)
{
	STR_TABLE_SINGLE	Table[] = 
	{
		{ TCMD_ON, "On" },
		{ TCMD_OFF, "Off" },
		{  0,NULL,NULL }
	};
	return findTableIDSingle1 (Table, tType);
}

const char *Hardware_Type_Desc(int hType)
{
	STR_TABLE_SINGLE	Table[] = 
	{
		{ HTYPE_RFXtrx315,	"RFXCom RFXtrx315 USB 315MHz Transceiver" },
		{ HTYPE_RFXtrx433,	"RFXCom RFXtrx433 USB 433.92MHz Transceiver" },
		{ HTYPE_RFXLAN,		"RFXCom RFXLAN Transceiver 433.92 MHz with LAN interface" },
		{ HTYPE_Domoticz,	"Remote Domoticz Server" },
		{  0,NULL,NULL }
	};
	return findTableIDSingle1 (Table, hType);
}

const char *Switch_Type_Desc(const unsigned char sType)
{
	STR_TABLE_SINGLE	Table[] = 
	{
		{ switchTypeOnOff, "On/Off" },
		{ switchTypeDoorbell, "Doorbell" },
		{ switchTypeContact, "Contact" },
		{  0,NULL,NULL }
	};
	return findTableIDSingle1 (Table, sType);
}

const char *RFX_Forecast_Desc(const unsigned char Forecast)
{
	STR_TABLE_SINGLE	Table[] = 
	{
		{ baroForecastNoInfo, "No Info" },
		{ baroForecastSunny, "Sunny" },
		{ baroForecastPartlyCloudy, "Partly Cloudy" },
		{ baroForecastCloudy, "Cloudy" },
		{ baroForecastRain, "Rain" },
		{  0,NULL,NULL }
	};
	return findTableIDSingle1 (Table, Forecast);
}

const char *RFX_Type_Desc(const unsigned char i, const unsigned char snum)
{
	STR_TABLE_SINGLE	Table[] = 
	{
		{ pTypeInterfaceControl, "Interface Control", "unknown" },
		{ pTypeInterfaceMessage, "Interface Message", "unknown" },
		{ pTypeRecXmitMessage, "Receiver/Transmitter Message", "unknown" },
		{ pTypeUndecoded, "Undecoded RF Message", "unknown" },
		{ pTypeLighting1, "Lighting 1" , "lightbulb", },
		{ pTypeLighting2, "Lighting 2" , "lightbulb", },
		{ pTypeLighting3, "Lighting 3" , "lightbulb", },
		{ pTypeLighting4, "Lighting 4" , "lightbulb", },
		{ pTypeLighting5, "Lighting 5" , "lightbulb", },
		{ pTypeLighting6, "Lighting 6" , "lightbulb", },
		{ pTypeCurtain, "Curtain" , "unknown" },
		{ pTypeBlinds, "Blinds" , "unknown" },
		{ pTypeSecurity1, "Security" , "security" },
		{ pTypeCamera, "Camera" , "unknown" },
		{ pTypeRemote, "Remote & IR" , "unknown" },
		{ pTypeThermostat1, "Thermostat 1" , "temperature" },
		{ pTypeThermostat2, "Thermostat 2" , "temperature" },
		{ pTypeThermostat3, "Thermostat 3" , "temperature" },
		{ pTypeTEMP, "Temp" , "temperature" },
		{ pTypeHUM, "Humidity" , "temperature" },
		{ pTypeTEMP_HUM, "Temp + Humidity" , "temperature" },
		{ pTypeBARO, "Barometric" , "temperature" },
		{ pTypeTEMP_HUM_BARO, "Temp + Humidity + Barometric" , "temperature" },
		{ pTypeRAIN, "Rain" , "rain" },
		{ pTypeWIND, "Wind" , "wind" },
		{ pTypeUV, "UV" , "uv" },
		{ pTypeDT, "Date/Time" , "unknown" },
		{ pTypeCURRENT, "Current" , "current" },
		{ pTypeENERGY, "Energy" , "current" },
		{ pTypeCURRENTENERGY, "Current/Energy" , "unknown" },
		{ pTypeGAS, "Gas" , "unknown" },
		{ pTypeWATER, "Water" , "unknown" },
		{ pTypeWEIGHT, "Weight" , "unknown" },
		{ pTypeRFXSensor, "RFXSensor" , "unknown" },
		{ pTypeRFXMeter, "RFXMeter" , "counter" },
		{ pTypeFS20, "FS20" , "unknown" },
		{  0,NULL,NULL }
	};
	if (snum==1)
		return findTableIDSingle1 (Table, i);

	return findTableIDSingle2 (Table, i);
}

const char *RFX_Type_SubType_Desc(const unsigned char dType, const unsigned char sType)
{
	STR_TABLE_ID1_ID2	Table[] = 
	{
		{ pTypeTEMP, sTypeTEMP1, "THR128/138, THC138" },
		{ pTypeTEMP, sTypeTEMP2, "THC238/268, THN132, THWR288, THRN122, THN122, AW129/131" },
		{ pTypeTEMP, sTypeTEMP3, "THWR800" },
		{ pTypeTEMP, sTypeTEMP4, "RTHN318" },
		{ pTypeTEMP, sTypeTEMP5, "LaCrosse TX3" },
		{ pTypeTEMP, sTypeTEMP6, "TS15C" },
		{ pTypeTEMP, sTypeTEMP7, "Viking 02811" },
		{ pTypeTEMP, sTypeTEMP8, "LaCrosse WS2300" },
		{ pTypeTEMP, sTypeTEMP9, "RUBiCSON" },
		{ pTypeTEMP, sTypeTEMP10, "TFA 30.3133" },

		{ pTypeHUM, sTypeHUM1, "LaCrosse TX3" },
		{ pTypeHUM, sTypeHUM2, "LaCrosse WS2300" },

		{ pTypeTEMP_HUM, sTypeTH1, "THGN122/123, THGN132, THGR122/228/238/268" },
		{ pTypeTEMP_HUM, sTypeTH2, "THGR810, THGN800" },
		{ pTypeTEMP_HUM, sTypeTH3, "RTGR328" },
		{ pTypeTEMP_HUM, sTypeTH4, "THGR328" },
		{ pTypeTEMP_HUM, sTypeTH5, "WTGR800" },
		{ pTypeTEMP_HUM, sTypeTH6, "THGR918, THGRN228, THGN500" },
		{ pTypeTEMP_HUM, sTypeTH7, "Cresta, TFA TS34C" },
		{ pTypeTEMP_HUM, sTypeTH8, "WT450H" },
		{ pTypeTEMP_HUM, sTypeTH9, "Viking 02035, 02038" },

		{ pTypeTEMP_HUM_BARO, sTypeTHB1, "THB1 - BTHR918" },
		{ pTypeTEMP_HUM_BARO, sTypeTHB2, "THB2 - BTHR918N, BTHR968" },

		{ pTypeRAIN, sTypeRAIN1, "RGR126/682/918/928" },
		{ pTypeRAIN, sTypeRAIN2, "PCR800" },
		{ pTypeRAIN, sTypeRAIN3, "TFA" },
		{ pTypeRAIN, sTypeRAIN4, "UPM RG700" },
		{ pTypeRAIN, sTypeRAIN5, "LaCrosse WS2300" },

		{ pTypeWIND, sTypeWIND1, "WTGR800" },
		{ pTypeWIND, sTypeWIND2, "WGR800" },
		{ pTypeWIND, sTypeWIND3, "STR918/928, WGR918" },
		{ pTypeWIND, sTypeWIND4, "TFA" },
		{ pTypeWIND, sTypeWIND5, "UPM WDS500" },
		{ pTypeWIND, sTypeWIND6, "LaCrosse WS2300" },

		{ pTypeUV, sTypeUV1, "UVN128,UV138" },
		{ pTypeUV, sTypeUV2, "UVN800" },
		{ pTypeUV, sTypeUV3, "TFA" },

		{ pTypeLighting1, sTypeX10, "X10" },
		{ pTypeLighting1, sTypeARC, "ARC" },
		{ pTypeLighting1, sTypeAB400D, "ELRO AB400" },
		{ pTypeLighting1, sTypeWaveman, "Waveman" },
		{ pTypeLighting1, sTypeEMW200, "EMW200" },
		{ pTypeLighting1, sTypeIMPULS, "Impuls" },
		{ pTypeLighting1, sTypeRisingSun, "RisingSun" },
		{ pTypeLighting1, sTypePhilips, "Philips" },

		{ pTypeLighting2, sTypeAC, "AC" },
		{ pTypeLighting2, sTypeHEU, "HomeEasy EU" },
		{ pTypeLighting2, sTypeANSLUT, "Anslut" },

		{ pTypeLighting3, sTypeKoppla, "Ikea Koppla" },

		{ pTypeLighting4, sTypePT2262, "PT2262" },

		{ pTypeLighting5, sTypeLightwaveRF, "LightwaveRF" },
		{ pTypeLighting5, sTypeEMW100, "EMW100" },
		{ pTypeLighting5, sTypeBBSB, "BBSB new" },

		{ pTypeLighting6, sTypeBlyss, "Blyss" },

		{ pTypeCurtain, sTypeHarrison, "Harrison" },

		{ pTypeBlinds, sTypeBlindsT0, "RollerTrol, Hasta new" },
		{ pTypeBlinds, sTypeBlindsT1, "Hasta old" },
		{ pTypeBlinds, sTypeBlindsT2, "A-OK RF01" },
		{ pTypeBlinds, sTypeBlindsT3, "A-OK AC114" },

		{ pTypeSecurity1, sTypeSecX10, "X10 security" },
		{ pTypeSecurity1, sTypeSecX10M, "X10 security motion" },
		{ pTypeSecurity1, sTypeSecX10R, "X10 security remote" },
		{ pTypeSecurity1, sTypeKD101, "KD101 smoke detector" },
		{ pTypeSecurity1, sTypePowercodeSensor, "Visonic PowerCode sensor - primary contact" },
		{ pTypeSecurity1, sTypePowercodeMotion, "Visonic PowerCode motion" },
		{ pTypeSecurity1, sTypeCodesecure, "Visonic CodeSecure" },
		{ pTypeSecurity1, sTypePowercodeAux, "Visonic PowerCode sensor - auxiliary contact" },
		{ pTypeSecurity1, sTypeMeiantech, "Meiantech" },
		
		{ pTypeCamera, sTypeNinja, "Meiantech" },

		{ pTypeRemote, sTypeATI, "ATI Remote Wonder" },
		{ pTypeRemote, sTypeATIplus, "ATI Remote Wonder Plus" },
		{ pTypeRemote, sTypeMedion, "Medion Remote" },
		{ pTypeRemote, sTypePCremote, "PC Remote" },
		{ pTypeRemote, sTypeATIrw2, "ATI Remote Wonder II" },
		
		{ pTypeThermostat1, sTypeDigimax, "Digimax" },
		{ pTypeThermostat1, sTypeDigimaxShort, "Digimax with short format" },

		{ pTypeThermostat2, sTypeHE105, "HE105" },
		{ pTypeThermostat2, sTypeRTS10, "RTS10" },

		{ pTypeThermostat3, sTypeMertikG6RH4T1, "Mertik G6R-H4T1" },
		{ pTypeThermostat3, sTypeMertikG6RH4TB, "Mertik G6R-H4TB" },

		{ pTypeDT, sTypeDT1, "RTGR328N" },

		{ pTypeCURRENT, sTypeELEC1, "CM113, Electrisave" },
		
		{ pTypeENERGY, sTypeELEC2, "CM119 / CM160" },
		{ pTypeENERGY, sTypeELEC3, "CM180" },
		
		{ pTypeCURRENTENERGY, sTypeELEC4, "CM180i" },
		
		{ pTypeWEIGHT, sTypeWEIGHT1, "BWR102" },
		{ pTypeWEIGHT, sTypeWEIGHT2, "GR101" },
		
		{ pTypeRFXSensor, sTypeRFXSensorTemp, "Temperature" },
		{ pTypeRFXSensor, sTypeRFXSensorAD, "A/D" },
		{ pTypeRFXSensor, sTypeRFXSensorVolt, "Voltage" },

		{ pTypeRFXMeter, sTypeRFXMeterCount, "RFXMeter counter" },
		

		{  0,0,NULL }
	};
	return findTableID1ID2(Table, dType, sType);
}

void GetLightStatus(
		const unsigned char dType, 
		const unsigned char dSubType, 
		const unsigned char nValue, 
		const std::string sValue, 
		std::string &lstatus, 
		int &llevel, 
		bool &bHaveDimmer,
		bool &bHaveGroupCmd)
{
	char szTmp[80];
	switch (dType)
	{
	case pTypeLighting1:
		switch (dSubType)
		{
		case sTypeX10:
			bHaveGroupCmd=true;
			switch (nValue)
			{
			case light1_sOff:
				lstatus="Off";
				break;
			case light1_sOn:
				lstatus="On";
				break;
			case light1_sDim:
				lstatus="Dim";
				break;
			case light1_sBright:
				lstatus="Bright";
				break;
			case light1_sAllOn:
				lstatus="All On";
				break;
			case light1_sAllOff:
				lstatus="All Off";
				break;
			}
			break;
		case sTypeARC:
			bHaveGroupCmd=true;
			switch (nValue)
			{
			case light1_sOff:
				lstatus="Off";
				break;
			case light1_sOn:
				lstatus="On";
				break;
			case light1_sAllOn:
				lstatus="All On";
				break;
			case light1_sAllOff:
				lstatus="All Off";
				break;
			case light1_sChime:
				lstatus="Chime";
				break;
			}
			break;
		case sTypeAB400D:
			switch (nValue)
			{
			case light1_sOff:
				lstatus="Off";
				break;
			case light1_sOn:
				lstatus="On";
				break;
			}
			break;
		}
		break;
	case pTypeLighting2:
		switch (dSubType)
		{
		case sTypeAC:
		case sTypeHEU:
		case sTypeANSLUT:
			bHaveDimmer=true;
			bHaveGroupCmd=true;
			switch (nValue)
			{
			case light2_sOff:
				lstatus="Off";
				break;
			case light2_sOn:
				lstatus="On";
				break;
			case light2_sSetLevel:
				sprintf(szTmp,"Set Level: %d", atoi(sValue.c_str()));
				if (sValue!="0")
					lstatus=szTmp;
				else
					lstatus="Off";
				break;
			case light2_sGroupOff:
				lstatus="Group Off";
				break;
			case light2_sGroupOn:
				lstatus="Group On";
				break;
			case light2_sSetGroupLevel:
				sprintf(szTmp,"Set Group Level: %d", atoi(sValue.c_str()));
				if (sValue!="0")
					lstatus=szTmp;
				else
					lstatus="Off";
				break;
			}
			break;
		}
		break;
	case pTypeLighting5:
		switch (dSubType)
		{
		case sTypeLightwaveRF:
			bHaveGroupCmd=true;
			bHaveDimmer=true;
			switch (nValue)
			{
			case light5_sOff:
				lstatus="Off";
				break;
			case light5_sOn:
				lstatus="On";
				break;
			case light5_sGroupOff:
				lstatus="Group Off";
				break;
			case light5_sMood1:
				lstatus="Group Mood 1";
				break;
			case light5_sMood2:
				lstatus="Group Mood 2";
				break;
			case light5_sMood3:
				lstatus="Group Mood 3";
				break;
			case light5_sMood4:
				lstatus="Group Mood 4";
				break;
			case light5_sMood5:
				lstatus="Group Mood 5";
				break;
			case light5_sUnlock:
				lstatus="Unlock";
				break;
			case light5_sLock:
				lstatus="Lock";
				break;
			case light5_sAllLock:
				lstatus="All lock";
				break;
			case light5_sClose:
				lstatus="Close inline relay";
				break;
			case light5_sStop:
				lstatus="Stop inline relay";
				break;
			case light5_sOpen:
				lstatus="Open inline relay";
				break;
			case light5_sSetLevel:
				sprintf(szTmp,"Set Level: %.2f %%" ,atof(sValue.c_str()));
				if (sValue!="0")
					lstatus=szTmp;
				else
					lstatus="Off";
				break;
			}
			break;
		case sTypeEMW100:
			switch (nValue)
			{
			case light5_sOff:
				lstatus="Off";
				break;
			case light5_sOn:
				lstatus="On";
				break;
			case light5_sLearn:
				lstatus="Learn";
				break;
			}
			break;
		case sTypeBBSB:
			bHaveGroupCmd=true;
			switch (nValue)
			{
			case light5_sOff:
				lstatus="Off";
				break;
			case light5_sOn:
				lstatus="On";
				break;
			case light5_sGroupOff:
				lstatus="Group Off";
				break;
			case light5_sGroupOn:
				lstatus="Group On";
				break;
			}
			break;
		}
		break;
	case pTypeLighting6:
		bHaveGroupCmd=true;
		switch (dSubType)
		{
		case sTypeBlyss:
			switch (nValue)
			{
			case light6_sOff:
				lstatus="Off";
				break;
			case light6_sOn:
				lstatus="On";
				break;
			case light6_sGroupOff:
				lstatus="Group Off";
				break;
			case light6_sGroupOn:
				lstatus="Group On";
				break;
			}
		}
		break;
	}
}

bool GetLightCommand(
	const unsigned char dType, 
	const unsigned char dSubType, 
	const unsigned char switchtype, 
	std::string switchcmd,
	unsigned char &cmd
	)
{
	if (switchtype==switchTypeContact)
		return false;	//we can not (or will not) switch this type

	switch (dType)
	{
	case pTypeLighting1:
		if (switchtype==switchTypeDoorbell)
		{
			if (switchcmd=="On")
			{
				cmd=light2_sGroupOn;
				return true;
			}
			else if (switchcmd=="Group On")
			{
				cmd=light2_sGroupOn;
				return true;
			}
			//no other combinations for the door switch
			return false;
		}
		if (switchcmd=="Off")
		{
			cmd=light1_sOff;
			return true;
		} else if (switchcmd=="On") {
			cmd=light1_sOn;
			return true;
		} else if (switchcmd=="Dim") {
			cmd=light1_sDim;
			return true;
		} else if (switchcmd=="Bright") {
			cmd=light1_sBright;
			return true;
		} else if (switchcmd=="All On") {
			cmd=light1_sAllOn;
			return true;
		} else if (switchcmd=="All Off") {
			cmd=light1_sAllOff;
			return true;
		} else if (switchcmd=="Chime") {
			cmd=light1_sChime;
			return true;
		} else
			return false;
		break;
	case pTypeLighting2:
		if (switchtype==switchTypeDoorbell)
		{
			if (switchcmd=="On")
			{
				cmd=light2_sGroupOn;
				return true;
			}
			else if (switchcmd=="Group On")
			{
				cmd=light2_sGroupOn;
				return true;
			}
			//no other combinations for the door switch
			return false;
		}
		if (switchcmd=="Off")
		{
			cmd=light2_sOff;
			return true;
		}
		else if (switchcmd=="On")
		{
			cmd=light2_sOn;
			return true;
		}
		else if (switchcmd=="Set Level")
		{
			cmd=light2_sSetLevel;
			return true;
		}
		else if (switchcmd=="Group Off")
		{
			cmd=light2_sGroupOff;
			return true;
		}
		else if (switchcmd=="Group On")
		{
			cmd=light2_sGroupOn;
			return true;
		}
		else if (switchcmd=="Set Group Level")
		{
			cmd=light2_sSetGroupLevel;
			return true;
		}
		else
			return false;
		break;
	}
	//unknown command
	return false;
}
