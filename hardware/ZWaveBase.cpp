#include "stdafx.h"
#include "ZWaveBase.h"
#include "ZWaveCommands.h"

#include <sstream>      // std::stringstream
#include <ctype.h>
#include <iomanip>

#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"
#include "hardwaretypes.h"

#include "../main/localtime_r.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"

#include "OpenZWave.h"

#define CONTROLLER_COMMAND_TIMEOUT 30

#pragma warning(disable: 4996)

#define round(a) ( int ) ( a + .5 )

ZWaveBase::ZWaveBase() :
	m_updateTime(0),
	m_ControllerCommandStartTime(0)
{
	m_LastIncludedNode = 0;
	m_bControllerCommandInProgress = false;
	m_bControllerCommandCanceled = false;
	m_bHaveLastIncludedNodeInfo = false;
	m_LastRemovedNode = -1;
	m_bInitState = true;
}


ZWaveBase::~ZWaveBase(void)
{
}

bool ZWaveBase::StartHardware()
{
	RequestStart();

	m_bInitState = true;
	m_updateTime = 0;
	m_LastIncludedNode = 0;
	m_bControllerCommandInProgress = false;
	m_bControllerCommandCanceled = false;
	m_bIsStarted = true;

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&ZWaveBase::Do_Work, this);
	SetThreadName(m_thread->native_handle(), "ZWaveBase");
	return (m_thread != nullptr);
}

bool ZWaveBase::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}

void ZWaveBase::Do_Work()
{
#ifdef WIN32
	//prevent OpenZWave locale from taking over
	_configthreadlocale(_ENABLE_PER_THREAD_LOCALE);
#endif
	int msec_counter = 0;
	int sec_counter = 0;
	while (!IsStopRequested(500))
	{
		msec_counter++;
		if (msec_counter == 2)
		{
			msec_counter = 0;
			sec_counter++;
			if (sec_counter % 12 == 0) {
				m_LastHeartbeat = mytime(NULL);
			}
		}


		if (m_bInitState)
		{
			if (GetInitialDevices())
			{
				m_bInitState = false;
				sOnConnected(this);
			}
		}
		else
		{
			GetUpdates();
			if (m_bControllerCommandInProgress == true)
			{
				time_t atime = mytime(NULL);
				double tdiff = difftime(atime, m_ControllerCommandStartTime);
				if (tdiff >= CONTROLLER_COMMAND_TIMEOUT)
				{
					_log.Log(LOG_STATUS, "ZWave: Stopping Controller command (Timeout!)");
					CancelControllerCommand();
				}
			}
		}
	}
}


std::string ZWaveBase::GenerateDeviceStringID(const _tZWaveDevice* pDevice)
{
	std::stringstream sstr;
	sstr << (int)pDevice->nodeID << ".instance." << (int)pDevice->orgInstanceID << ".index." << pDevice->orgIndexID << ".commandClasses." << (int)pDevice->commandClassID;
	return sstr.str();
}

void ZWaveBase::InsertDevice(_tZWaveDevice device)
{
	device.string_id = GenerateDeviceStringID(&device);
	device.lastreceived = mytime(NULL);
#ifdef _DEBUG
	bool bNewDevice = (m_devices.find(device.string_id) == m_devices.end());
	if (bNewDevice)
	{
		_log.Log(LOG_NORM, "New device: %s", device.string_id.c_str());
	}
	else
	{
		_log.Log(LOG_NORM, "Update device: %s", device.string_id.c_str());
	}
#endif
	//insert or update device in internal record
	device.sequence_number = 1;
	m_devices[device.string_id] = device;

	SendSwitchIfNotExists(&device);
}

void ZWaveBase::SendSwitchIfNotExists(const _tZWaveDevice* pDevice)
{
	if (
		(pDevice->devType != ZDTYPE_SWITCH_NORMAL) &&
		(pDevice->devType != ZDTYPE_SWITCH_DIMMER) &&
		(pDevice->devType != ZDTYPE_SWITCH_RGBW) &&
		(pDevice->devType != ZDTYPE_SWITCH_COLOR)
		)
		return; //only for switches

	//make device ID

	//To fix all problems it should be
	//ID1 = pDevice->nodeID;
	//ID2 = pDevice->instanceID;
	//ID3 = (pDevice->indexID & 0xFF00) >> 8;
	//ID4 = pDevice->indexID & 0x00FF;
	//but current users gets new devices in this case

	unsigned char ID1 = 0;
	unsigned char ID2 = 0;
	unsigned char ID3 = pDevice->nodeID;
	unsigned char ID4 = pDevice->instanceID;

	unsigned long lID = (ID1 << 24) + (ID2 << 16) + (ID3 << 8) + ID4;

	char szID[10];
	sprintf(szID, "%08lX", lID);
	unsigned char unitcode = 1;

	int devType = pTypeGeneralSwitch;
	int SubType = sSwitchGeneralSwitch;
	int SwitchType = (
		(pDevice->devType == ZDTYPE_SWITCH_DIMMER)
		|| (pDevice->devType == ZDTYPE_SWITCH_RGBW)
		|| (pDevice->devType == ZDTYPE_SWITCH_COLOR)
		) ? STYPE_Dimmer : STYPE_OnOff;

	if ((pDevice->devType == ZDTYPE_SWITCH_RGBW) || (pDevice->devType == ZDTYPE_SWITCH_COLOR))
	{
		devType = pTypeColorSwitch;
		// TODO: For devices of type ZDTYPE_SWITCH_COLOR the supported color channels is reported in response to command SWITCH_COLOR_SUPPORTED_GET
		// This is already done by OpenZWave::Color class and used internally by the class, the information is needed here.
		SubType = (pDevice->devType == ZDTYPE_SWITCH_RGBW) ? sTypeColor_RGB_W_Z : sTypeColor_RGB_CW_WW_Z;
	}

	//Check if we already exist
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d) AND (Type==%d) AND (SubType==%d) AND (DeviceID=='%q')",
		m_HwdID, int(unitcode), devType, SubType, szID);
	if (!result.empty())
	{
		// Find out if nodeID, instanceID, indexID, devType, SubType is ambiguous.
		// This test uses the same lookup procedure as ZWaveBase::WriteToHardware, calling FindDevice,
		// but instead of returning a single result, checks if there is more than one in memory
		// device satisfying those criteria

		std::map<std::string, _tZWaveDevice>::iterator itt;
		for (itt = m_devices.begin(); itt != m_devices.end(); ++itt)
		{
			if (
				// Node ID1 is not used when searching for switch-like devices, see ID1 = 0; at the start
				// of this routine
				(itt->second.nodeID == (ID2 << 8) + ID3) &&
				(itt->second.instanceID == ID4) &&
				(itt->second.string_id != pDevice->string_id))
			{
				// itt->second has the same szID as the one we would like to add,
				// but a different string_id. Example: "7.instance.1.index.256.commandClasses.113"
				// Now we have to check if devType, SubType makes it unique.
				if (devType == pTypeGeneralSwitch)
				{
					// Test for two different types, because ZWaveBase::WriteToHardware first tries
					// to find a dimmer, if that fails tries a switch. This means you cannot have
					// one database record with a dimmer and another with a switch, because trying
					// to control the switch will select the dimmer.
					if ((itt->second.devType == ZDTYPE_SWITCH_DIMMER) || (itt->second.devType == ZDTYPE_SWITCH_NORMAL))
						break;
				}
				else if (itt->second.devType == pDevice->devType)
				{
					// By elimination... devType = pTypeColorSwitch
					// Subtype depends
					// ZDTYPE_SWITCH_RGBW -> sTypeColor_RGB_W_Z
					// ZDTYPE_SWITCH_COLOR -> sTypeColor_RGB_CW_WW_Z
					// So all ZDTYPE_SWITCH_RGBW must be unique, and all ZDTYPE_SWITCH_COLOR devices must be unique
					break;
				}
			}
		}
		if (itt != m_devices.end())
		{
			_log.Log(
				LOG_STATUS,
				"SendSwitchIfNotExists: Device '%s' (%s) with DeviceID '%s' matches '%s' (%s). Domoticz will use the Dimmer (and hide the Switch).",
				pDevice->string_id.c_str(), pDevice->label.c_str(), szID, itt->second.string_id.c_str(), itt->second.label.c_str());
		}
		return; //Already in the system
	}

	if ((pDevice->devType == ZDTYPE_SWITCH_RGBW) || (pDevice->devType == ZDTYPE_SWITCH_COLOR))
	{
		//Send as ColorSwitch
		_tColorSwitch lcmd;
		lcmd.id = lID;
		lcmd.command = Color_LedOff;
		lcmd.subtype = SubType;
		lcmd.value = 0;
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char*)& lcmd, pDevice->label.c_str(), pDevice->batValue);
	}
	else
	{

		//Send as pTypeGeneralSwitch/sSwitchGeneralSwitch
		_tGeneralSwitch  gswitch;
		gswitch.subtype = sSwitchGeneralSwitch;
		gswitch.seqnbr = pDevice->sequence_number;
		gswitch.id = lID;
		gswitch.unitcode = unitcode;
		gswitch.battery_level = pDevice->batValue;

		// Get device level to set
		int level = pDevice->intvalue;

		// Simple on/off device, make sure we only have 0 or 255
		if ((pDevice->devType == ZDTYPE_SWITCH_NORMAL) || (pDevice->devType == ZDTYPE_CENTRAL_SCENE))
			level = (level == 0) ? 0 : 255;

		// Now check the values
		if (level == 0)
			gswitch.cmnd = gswitch_sOff;
		else if (level > 99)
		{
			if (pDevice->devType == ZDTYPE_SWITCH_DIMMER)
			{
				level = 100;
			}
			gswitch.cmnd = gswitch_sOn;
		}
		else
		{
			gswitch.cmnd = gswitch_sSetLevel;
		}

		// Update the lcmd with the correct level value.
		gswitch.level = level;

		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char*)& gswitch, pDevice->label.c_str(), pDevice->batValue);
	}

	//Set SwitchType
	m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d WHERE (HardwareID==%d) AND (Unit==%d) AND (Type==%d) AND (SubType==%d) AND (DeviceID=='%q')",
		SwitchType, m_HwdID, int(unitcode), devType, SubType, szID);
}

unsigned char ZWaveBase::Convert_Battery_To_PercInt(const unsigned char level)
{
	if (level == 255)
		return 9; //no battery
	int ret = (level / 10) - 1;
	if (ret < 0)
		ret = 0;
	return (unsigned char)ret;
}

void ZWaveBase::SendDevice2Domoticz(const _tZWaveDevice* pDevice)
{
	//make device ID

	//To fix all problems it should be
	//ID1 = pDevice->nodeID;
	//ID2 = pDevice->instanceID;
	//ID3 = (pDevice->indexID & 0xFF00) >> 8;
	//ID4 = pDevice->indexID & 0x00FF;
	//but current users gets new devices in this case

	uint8_t ID1 = 0;
	uint8_t ID2 = 0;
	uint8_t ID3 = pDevice->nodeID;
	uint8_t ID4 = pDevice->instanceID;

	char szID[10];
	sprintf(szID, "%X%02X%02X%02X", ID1, ID2, ID3, ID4);

	unsigned long lID = (ID1 << 24) + (ID2 << 16) + (ID3 << 8) + ID4;

	int BatLevel = pDevice->batValue;

	if ((pDevice->devType == ZDTYPE_SWITCH_NORMAL) || (pDevice->devType == ZDTYPE_SWITCH_DIMMER) || (pDevice->devType == ZDTYPE_CENTRAL_SCENE))
	{
		//Send as pTypeGeneralSwitch, sSwitchGeneralSwitch
		_tGeneralSwitch gswitch;
		gswitch.subtype = sSwitchGeneralSwitch;
		gswitch.seqnbr = pDevice->sequence_number;
		gswitch.battery_level = BatLevel;
		gswitch.id = lID;

		// Get device level to set
		int level = pDevice->intvalue;

		// Simple on/off device, make sure we only have 0 or 255
		if (pDevice->devType == ZDTYPE_SWITCH_NORMAL)
			level = (level == 0) ? 0 : 255;
		else if (pDevice->devType == ZDTYPE_CENTRAL_SCENE)
		{
			level = 255;
			gswitch.unitcode = pDevice->intvalue;
		}

		// Now check the values
		if (level == 0)
			gswitch.cmnd = gswitch_sOff;
		else if (level > 99)
		{
			if (pDevice->devType == ZDTYPE_SWITCH_DIMMER)
			{
				level = 100;
			}
			gswitch.cmnd = gswitch_sOn;
		}
		else
		{
			gswitch.cmnd = gswitch_sSetLevel;
		}

		// Update the lcmd with the correct level value.
		gswitch.level = level;
		gswitch.rssi = 12;
		sDecodeRXMessage(this, (const unsigned char*)& gswitch, NULL, BatLevel);
		return;
	}
	else if ((pDevice->devType == ZDTYPE_SWITCH_RGBW) || (pDevice->devType == ZDTYPE_SWITCH_COLOR))
	{
		//Send as ColorSwitch
		_tColorSwitch lcmd;
		lcmd.id = lID;

		// Get device level to set
		int level = pDevice->intvalue;

		if (level == 0)
			lcmd.command = Color_LedOff;
		else
			lcmd.command = Color_LedOn;
		lcmd.value = 0;
		sDecodeRXMessage(this, (const unsigned char*)& lcmd, NULL, BatLevel);
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_POWER)
	{
		_tUsageMeter umeter;
		umeter.id1 = ID1;
		umeter.id2 = ID2;
		umeter.id3 = ID3;
		umeter.id4 = ID4;
		umeter.dunit = 2;
		umeter.fusage = pDevice->floatValue;
		sDecodeRXMessage(this, (const unsigned char*)& umeter, NULL, BatLevel);

		//Search Energy Device
		const _tZWaveDevice* pEnergyDevice;
		pEnergyDevice = FindDeviceEx(pDevice->nodeID, pDevice->orgInstanceID, ZDTYPE_SENSOR_POWERENERGYMETER);
		if (pEnergyDevice == NULL)
		{
			pEnergyDevice = FindDevice(pDevice->nodeID, pDevice->instanceID, pDevice->indexID, COMMAND_CLASS_METER, ZDTYPE_SENSOR_POWERENERGYMETER);
			if (pEnergyDevice == NULL)
			{
				pEnergyDevice = FindDevice(pDevice->nodeID, pDevice->instanceID, pDevice->indexID, ZDTYPE_SENSOR_POWERENERGYMETER);
				if (pEnergyDevice == NULL)
				{
					if (
						(pDevice->Manufacturer_id == 0x010F) &&
						(
						((pDevice->Product_type == 0x0200) && (pDevice->Product_id == 0x1000))
							|| ((pDevice->Product_type == 0x0600) && (pDevice->Product_id == 0x1000))
							|| ((pDevice->Product_type == 0x0602) && (pDevice->Product_id == 0x1001))
							|| ((pDevice->Product_type == 0x0602) && (pDevice->Product_id == 0x1003))
							)
						)
					{
						//Fibaro Wallplug, find energy sensor
						pEnergyDevice = FindDevice(pDevice->nodeID, -1, -1, ZDTYPE_SENSOR_POWERENERGYMETER);
					}
				}
			}
		}
		if (pEnergyDevice)
		{
			if (pEnergyDevice->bValidValue)
			{
				SendKwhMeter(pEnergyDevice->nodeID, pEnergyDevice->instanceID, BatLevel, pDevice->floatValue, pEnergyDevice->floatValue / pEnergyDevice->scaleMultiply, "kWh Meter");
			}
		}
		else
		{
			//No kWh meter, send as normal Power device
			SendWattMeter(pDevice->nodeID, pDevice->instanceID, BatLevel, pDevice->floatValue, "Power Meter");
		}
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_POWERENERGYMETER)
	{
		RBUF tsen;
		memset(&tsen, 0, sizeof(RBUF));

		const _tZWaveDevice* pPowerDevice;
		if (
			(pDevice->Manufacturer_id == 0x010F) &&
			(
			((pDevice->Product_type == 0x0200) && (pDevice->Product_id == 0x1000))
				|| ((pDevice->Product_type == 0x0600) && (pDevice->Product_id == 0x1000))
				|| ((pDevice->Product_type == 0x0602) && (pDevice->Product_id == 0x1001))
				|| ((pDevice->Product_type == 0x0602) && (pDevice->Product_id == 0x1003))
				)
			)
		{
			//Fibaro Wallplug, find power sensor with idx 4 (idx 1 only reports when plug goes on/off, idx 4 is live power)
			pPowerDevice = FindDevice(pDevice->nodeID, 4, pDevice->indexID, COMMAND_CLASS_SENSOR_MULTILEVEL, ZDTYPE_SENSOR_POWER);
			if (pPowerDevice == NULL)
				pPowerDevice = FindDevice(pDevice->nodeID, pDevice->instanceID, pDevice->indexID, COMMAND_CLASS_METER, ZDTYPE_SENSOR_POWER);
		}
		else
		{
			pPowerDevice = FindDeviceEx(pDevice->nodeID, pDevice->orgInstanceID, ZDTYPE_SENSOR_POWER);
			if (pPowerDevice == NULL)
				pPowerDevice = FindDevice(pDevice->nodeID, pDevice->instanceID, pDevice->indexID, COMMAND_CLASS_METER, ZDTYPE_SENSOR_POWER);
		}
		if (pPowerDevice == NULL)
		{
			pPowerDevice = FindDevice(pDevice->nodeID, pDevice->instanceID, pDevice->indexID, ZDTYPE_SENSOR_POWER);
			if (pPowerDevice == NULL)
				pPowerDevice = FindDevice(pDevice->nodeID, -1, -1, ZDTYPE_SENSOR_POWER);
		}
		bool bHaveValidPowerDevice = false;
		if (pPowerDevice)
		{
			bHaveValidPowerDevice = pPowerDevice->bValidValue;
		}
		if (bHaveValidPowerDevice)
		{
			SendKwhMeter(pDevice->nodeID, pDevice->instanceID, BatLevel, pPowerDevice->floatValue, pDevice->floatValue / pDevice->scaleMultiply, "kWh Meter");
		}
		else
		{
			SendKwhMeter(pDevice->nodeID, pDevice->instanceID, BatLevel, 0, pDevice->floatValue / pDevice->scaleMultiply, "kWh Meter");
		}
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_VOLTAGE)
	{
		SendVoltageSensor(0, lID, BatLevel, pDevice->floatValue, "Voltage");
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_PERCENTAGE)
	{
		SendPercentageSensor(lID, 0, BatLevel, pDevice->floatValue, "Percentage");
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_AMPERE)
	{
		RBUF tsen;
		memset(&tsen, 0, sizeof(RBUF));
		tsen.CURRENT.packettype = pTypeCURRENT;
		tsen.CURRENT.subtype = sTypeELEC1;
		tsen.CURRENT.packetlength = sizeof(tsen.CURRENT) - 1;
		tsen.CURRENT.id1 = pDevice->nodeID;
		tsen.CURRENT.id2 = pDevice->instanceID;
		int amps = round(pDevice->floatValue * 10.0f);
		tsen.CURRENT.ch1h = amps / 256;
		amps -= (tsen.CURRENT.ch1h * 256);
		tsen.CURRENT.ch1l = (BYTE)amps;
		tsen.CURRENT.battery_level = Convert_Battery_To_PercInt(pDevice->batValue);
		tsen.CURRENT.rssi = 12;
		sDecodeRXMessage(this, (const unsigned char*)& tsen.CURRENT, NULL, BatLevel);
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_UV)
	{
		SendUVSensor(pDevice->nodeID, pDevice->instanceID, BatLevel, pDevice->floatValue, "UV");
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_TEMPERATURE)
	{
		if (!pDevice->bValidValue)
			return;
		RBUF tsen;
		memset(&tsen, 0, sizeof(RBUF));

		const _tZWaveDevice* pHumDevice = FindDevice(pDevice->nodeID, -1, -1, ZDTYPE_SENSOR_HUMIDITY);
		if (pHumDevice)
		{
			if (!pHumDevice->bValidValue)
				return;
			uint16_t NodeID = (pDevice->nodeID << 8) | pDevice->instanceID;
			SendTempHumSensor(NodeID, BatLevel, pDevice->floatValue, pHumDevice->intvalue, "TempHum");
		}
		else
		{
			uint16_t NodeID = (pDevice->nodeID << 8) | pDevice->instanceID;
			SendTempSensor(NodeID, BatLevel, pDevice->floatValue, "Temperature");
		}
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_HUMIDITY)
	{
		if (!pDevice->bValidValue)
			return;
		RBUF tsen;
		memset(&tsen, 0, sizeof(RBUF));

		const _tZWaveDevice* pTempDevice = FindDevice(pDevice->nodeID, -1, -1, ZDTYPE_SENSOR_TEMPERATURE);
		if (pTempDevice)
		{
			if (!pTempDevice->bValidValue)
				return;

			//report it with the ID of the temperature sensor, else we get two sensors with the same value
			uint16_t NodeID = (pTempDevice->nodeID << 8) | pTempDevice->instanceID;
			SendTempHumSensor(NodeID, BatLevel, pTempDevice->floatValue, pDevice->intvalue, "TempHum");
		}
		else
		{
			uint16_t NodeID = (pDevice->nodeID << 8) | pDevice->instanceID;
			SendHumiditySensor(NodeID, BatLevel, pDevice->intvalue, "Humidity");
		}
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_VELOCITY)
	{
		if (!pDevice->bValidValue)
			return;

		RBUF tsen;
		memset(&tsen, 0, sizeof(RBUF));
		tsen.WIND.packetlength = sizeof(tsen.WIND) - 1;
		tsen.WIND.packettype = pTypeWIND;
		tsen.WIND.subtype = sTypeWIND4;
		tsen.WIND.battery_level = Convert_Battery_To_PercInt(pDevice->batValue);
		tsen.WIND.rssi = 12;
		tsen.WIND.id1 = pDevice->nodeID;
		tsen.WIND.id2 = pDevice->instanceID;

		float winddir = 0;
		int aw = round(winddir);
		tsen.WIND.directionh = (BYTE)(aw / 256);
		aw -= (tsen.WIND.directionh * 256);
		tsen.WIND.directionl = (BYTE)(aw);

		int sw = round(pDevice->floatValue * 10.0f);
		tsen.WIND.av_speedh = (BYTE)(sw / 256);
		sw -= (tsen.WIND.av_speedh * 256);
		tsen.WIND.av_speedl = (BYTE)(sw);
		tsen.WIND.gusth = tsen.WIND.av_speedh;
		tsen.WIND.gustl = tsen.WIND.av_speedl;

		//this is not correct, why no wind temperature? and only chill?
		tsen.WIND.chillh = 0;
		tsen.WIND.chilll = 0;
		tsen.WIND.temperatureh = 0;
		tsen.WIND.temperaturel = 0;

		const _tZWaveDevice* pTempDevice = FindDevice(pDevice->nodeID, -1, -1, ZDTYPE_SENSOR_TEMPERATURE);
		if (pTempDevice)
		{
			if (!pTempDevice->bValidValue)
				return;
			tsen.WIND.tempsign = (pTempDevice->floatValue >= 0) ? 0 : 1;
			tsen.WIND.chillsign = (pTempDevice->floatValue >= 0) ? 0 : 1;
			int at10 = round(std::abs(pTempDevice->floatValue * 10.0f));
			tsen.WIND.temperatureh = (BYTE)(at10 / 256);
			tsen.WIND.chillh = (BYTE)(at10 / 256);
			at10 -= (tsen.WIND.chillh * 256);
			tsen.WIND.temperaturel = (BYTE)(at10);
			tsen.WIND.chilll = (BYTE)(at10);
		}
		sDecodeRXMessage(this, (const unsigned char*)& tsen.WIND, NULL, BatLevel);
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_BAROMETER)
	{
		if (!pDevice->bValidValue)
			return;
		const _tZWaveDevice* pTempDevice = FindDevice(pDevice->nodeID, -1, -1, ZDTYPE_SENSOR_TEMPERATURE);
		const _tZWaveDevice* pHumDevice = FindDevice(pDevice->nodeID, -1, -1, ZDTYPE_SENSOR_HUMIDITY);
		if (pTempDevice && pHumDevice)
		{
			int nforecast = wsbaroforcast_some_clouds;
			float pressure = pDevice->floatValue;
			if (pressure <= 980)
				nforecast = wsbaroforcast_heavy_rain;
			else if (pressure <= 995)
			{
				if (pDevice->floatValue > 1)
					nforecast = wsbaroforcast_rain;
				else
					nforecast = wsbaroforcast_snow;
			}
			else if (pressure >= 1029)
				nforecast = wsbaroforcast_sunny;
			SendTempHumBaroSensorFloat(pDevice->nodeID, BatLevel, pTempDevice->floatValue, pHumDevice->intvalue, pDevice->floatValue, nforecast, "TempHumBaro");
		}
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_LIGHT)
	{
		_tLightMeter lmeter;
		lmeter.id1 = ID1;
		lmeter.id2 = ID2;
		lmeter.id3 = ID3;
		lmeter.id4 = ID4;
		lmeter.dunit = 255;
		lmeter.fLux = pDevice->floatValue;
		lmeter.battery_level = BatLevel;
		sDecodeRXMessage(this, (const unsigned char*)& lmeter, (!pDevice->label.empty()) ? pDevice->label.c_str() : NULL, BatLevel);
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_GAS)
	{
		P1Gas	m_p1gas;
		m_p1gas.type = pTypeP1Gas;
		m_p1gas.subtype = sTypeP1Gas;
		m_p1gas.gasusage = (unsigned long)(pDevice->floatValue * 1000);
		m_p1gas.ID = lID;
		sDecodeRXMessage(this, (const unsigned char*)& m_p1gas, (!pDevice->label.empty()) ? pDevice->label.c_str() : NULL, BatLevel);
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_WATER)
	{
		uint16_t NodeID = (pDevice->nodeID << 8) | pDevice->instanceID;
		SendRainSensor(NodeID, BatLevel, pDevice->floatValue * 1000.0f, "Water");
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_CO2)
	{
		SendAirQualitySensor(pDevice->nodeID, (uint8_t)pDevice->orgInstanceID, BatLevel, int(pDevice->floatValue), "CO2 Sensor");
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_MOISTURE)
	{
		SendPercentageSensor(lID, 0, BatLevel, pDevice->floatValue, "Moisture");
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_TANK_CAPACITY)
	{
		SendCustomSensor(pDevice->nodeID, pDevice->instanceID, BatLevel, pDevice->floatValue, "Tank Capacity", "l");
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_SETPOINT)
	{
		_tThermostat tmeter;
		tmeter.subtype = sTypeThermSetpoint;
		tmeter.id1 = ID1;
		tmeter.id2 = ID2;
		tmeter.id3 = ID3;
		tmeter.id4 = ID4;
		tmeter.dunit = 1;
		tmeter.battery_level = BatLevel;
		tmeter.temp = pDevice->floatValue;
		sDecodeRXMessage(this, (const unsigned char*)& tmeter, (!pDevice->label.empty()) ? pDevice->label.c_str() : NULL, BatLevel);
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_THERMOSTAT_CLOCK)
	{
		_tGeneralDevice gDevice;
		gDevice.subtype = sTypeZWaveClock;
		gDevice.id = ID4;
		gDevice.intval1 = lID;
		gDevice.intval2 = pDevice->intvalue;
		sDecodeRXMessage(this, (const unsigned char*)& gDevice, "Thermostat Clock", BatLevel);
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_THERMOSTAT_MODE)
	{
		_tGeneralDevice gDevice;
		gDevice.subtype = sTypeZWaveThermostatMode;
		gDevice.id = ID4;
		gDevice.intval1 = lID;
		gDevice.intval2 = pDevice->intvalue;
		sDecodeRXMessage(this, (const unsigned char*)& gDevice, "Thermostat Mode", BatLevel);
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_THERMOSTAT_FAN_MODE)
	{
		_tGeneralDevice gDevice;
		gDevice.subtype = sTypeZWaveThermostatFanMode;
		gDevice.id = ID4;
		gDevice.intval1 = lID;
		gDevice.intval2 = pDevice->intvalue;
		sDecodeRXMessage(this, (const unsigned char*)& gDevice, "Thermostat Fan Mode", BatLevel);
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_THERMOSTAT_OPERATING_STATE)
	{
		_tGeneralDevice gDevice;
		gDevice.subtype = sTypeZWaveThermostatOperatingState;
		gDevice.id = ID4;
		gDevice.intval1 = lID;
		gDevice.intval2 = pDevice->intvalue;
		sDecodeRXMessage(this, (const unsigned char*)& gDevice, "Thermostat Operating State", BatLevel);
	}
	else if (pDevice->devType == ZDTYPE_ALARM)
	{
		_tGeneralDevice gDevice;
		gDevice.subtype = sTypeZWaveAlarm;
		gDevice.id = pDevice->Alarm_Type;
		gDevice.intval1 = lID;
		gDevice.intval2 = pDevice->intvalue;
		char szTmp[100];
		sprintf(szTmp, "Alarm Type: %s %d(0x%02X)", pDevice->label.c_str(), pDevice->Alarm_Type, pDevice->Alarm_Type);
		sDecodeRXMessage(this, (const unsigned char*)& gDevice, szTmp, BatLevel);
	}
	else if (pDevice->devType == ZDTYPE_SENSOR_CUSTOM)
	{
		SendCustomSensor(pDevice->nodeID, pDevice->instanceID, BatLevel, pDevice->floatValue, pDevice->label, pDevice->custom_label);
	}
}

ZWaveBase::_tZWaveDevice* ZWaveBase::FindDevice(const uint8_t nodeID, const int instanceID, const int indexID)
{
	for (auto& itt : m_devices)
	{
		if (
			(itt.second.nodeID == nodeID) &&
			(itt.second.instanceID == instanceID) &&
			(itt.second.indexID == indexID)
			)
			return &itt.second;
	}
	return NULL;
}

//Used for power/energy devices
ZWaveBase::_tZWaveDevice* ZWaveBase::FindDeviceEx(const uint8_t nodeID, const int instanceID, const _eZWaveDeviceType devType)
{
	for (auto& itt : m_devices)
	{
		if (
			(itt.second.nodeID == nodeID) &&
			(itt.second.instanceID == instanceID) &&
			(itt.second.devType == devType)
			)
			return &itt.second;
	}
	return NULL;
}

ZWaveBase::_tZWaveDevice* ZWaveBase::FindDevice(const uint8_t nodeID, const int instanceID, const int indexID, const _eZWaveDeviceType devType)
{
	for (auto& itt : m_devices)
	{
		if (
			(itt.second.nodeID == nodeID) &&
			((itt.second.instanceID == instanceID) || (instanceID == -1)) &&
			(itt.second.devType == devType)
			)
			return &itt.second;
	}
	return NULL;
}

ZWaveBase::_tZWaveDevice* ZWaveBase::FindDevice(const uint8_t nodeID, const int instanceID, const int indexID, const int CommandClassID, const _eZWaveDeviceType devType)
{
	for (auto& itt : m_devices)
	{
		if (
			(itt.second.nodeID == nodeID) &&
			((itt.second.instanceID == instanceID) || (instanceID == -1)) &&
			(itt.second.commandClassID == CommandClassID) &&
			(itt.second.devType == devType)
			)
			return &itt.second;
	}
	return NULL;
}

bool ZWaveBase::WriteToHardware(const char* pdata, const unsigned char length)
{
	std::lock_guard<std::mutex> l(m_NotificationMutex);

	_tZWaveDevice* pDevice = NULL;

	unsigned char packettype = pdata[1];
	unsigned char subtype = pdata[2];

	if ((packettype == pTypeGeneralSwitch) && (subtype == sSwitchGeneralSwitch))
	{
		//light command

		const _tGeneralSwitch* pSwitch = reinterpret_cast<const _tGeneralSwitch*>(pdata);

		uint8_t ID1 = (uint8_t)((pSwitch->id & 0xFF000000) >> 24);
		uint8_t ID2 = (uint8_t)((pSwitch->id & 0x00FF0000) >> 16);
		uint8_t ID3 = (uint8_t)((pSwitch->id & 0x0000FF00) >> 8);
		uint8_t ID4 = (uint8_t)((pSwitch->id & 0x000000FF));

		int level = pSwitch->level;
		int cmnd = pSwitch->cmnd;

		if (cmnd == gswitch_sSetLevel)
		{
			// Set command based on level value
			if (level == 0)
				cmnd = gswitch_sOff;
			else if (level == 255)
				cmnd = gswitch_sOn;
			else
			{
				// For dimmers we only allow level 0-99
				level = (level > 99) ? 99 : level;
			}
		}


		//find device

		uint8_t nodeID = ID3;
		uint8_t instanceID = ID4;
		int indexID = ID1;

		int svalue = 0;

		//First find dimmer
		pDevice = FindDevice(nodeID, instanceID, indexID, ZDTYPE_SWITCH_DIMMER);
		if (pDevice)
		{
			if ((cmnd == gswitch_sOff) || (cmnd == gswitch_sGroupOff))
				svalue = 0;
			else if ((cmnd == gswitch_sOn) || (cmnd == gswitch_sGroupOn))
				svalue = 255;
			else
			{
				svalue = (level < 99) ? level : 99;
			}
			return SwitchLight(pDevice, instanceID, svalue);
		}
		else
		{
			//find normal
			pDevice = FindDevice(nodeID, instanceID, indexID, ZDTYPE_SWITCH_NORMAL);
			if (pDevice)
			{
				if ((cmnd == gswitch_sOff) || (cmnd == gswitch_sGroupOff))
					svalue = 0;
				else
					svalue = 255;
				return SwitchLight(pDevice, instanceID, svalue);
			}
			_log.Log(LOG_ERROR, "ZWave: Node not found! (NodeID: %d, 0x%02x)", nodeID, nodeID);
			return false;
		}
	}
	else if ((packettype == pTypeThermostat) && (subtype == sTypeThermSetpoint))
	{
		//Set Point
		const _tThermostat* pMeter = reinterpret_cast<const _tThermostat*>(pdata);
		uint8_t nodeID = pMeter->id3;
		uint8_t instanceID = pMeter->id4;
		int indexID = pMeter->id1;

		//find normal
		pDevice = FindDevice(nodeID, instanceID, indexID, ZDTYPE_SENSOR_SETPOINT);
		if (pDevice)
		{
			SetThermostatSetPoint(nodeID, instanceID, pDevice->commandClassID, pMeter->temp);
			return true;
		}
		_log.Log(LOG_ERROR, "ZWave: Node not found! (NodeID: %d, 0x%02x)", nodeID, nodeID);
		return false;
	}
	else if ((packettype == pTypeGeneral) && (subtype == sTypeZWaveClock))
	{
		const _tGeneralDevice* pMeter = reinterpret_cast<const _tGeneralDevice*>(pdata);
		uint8_t ID1 = (uint8_t)((pMeter->intval1 & 0xFF000000) >> 24);
		uint8_t ID2 = (uint8_t)((pMeter->intval1 & 0x00FF0000) >> 16);
		uint8_t ID3 = (uint8_t)((pMeter->intval1 & 0x0000FF00) >> 8);
		uint8_t ID4 = (uint8_t)((pMeter->intval1 & 0x000000FF));

		uint8_t nodeID = ID3;
		uint8_t instanceID = ID4;
		int indexID = ID1;

		pDevice = FindDevice(nodeID, instanceID, indexID, ZDTYPE_SENSOR_THERMOSTAT_CLOCK);
		if (pDevice)
		{
			int tintval = pMeter->intval2;
			int day = tintval / (24 * 60); tintval -= (day * 24 * 60);
			int hour = tintval / (60); tintval -= (hour * 60);
			int minute = tintval;

			SetClock(nodeID, instanceID, pDevice->commandClassID, day, hour, minute);
			return true;
		}
		_log.Log(LOG_ERROR, "ZWave: Node not found! (NodeID: %d, 0x%02x)", nodeID, nodeID);
		return false;
	}
	else if ((packettype == pTypeGeneral) && (subtype == sTypeZWaveThermostatMode))
	{
		const _tGeneralDevice* pMeter = reinterpret_cast<const _tGeneralDevice*>(pdata);
		uint8_t ID1 = (uint8_t)((pMeter->intval1 & 0xFF000000) >> 24);
		uint8_t ID2 = (uint8_t)((pMeter->intval1 & 0x00FF0000) >> 16);
		uint8_t ID3 = (uint8_t)((pMeter->intval1 & 0x0000FF00) >> 8);
		uint8_t ID4 = (uint8_t)((pMeter->intval1 & 0x000000FF));

		uint8_t nodeID = ID3;
		uint8_t instanceID = ID4;
		int indexID = ID1;

		pDevice = FindDevice(nodeID, instanceID, indexID, ZDTYPE_SENSOR_THERMOSTAT_MODE);
		if (pDevice)
		{
			int tMode = pMeter->intval2;
			SetThermostatMode(nodeID, instanceID, pDevice->commandClassID, tMode);
			return true;
		}
		_log.Log(LOG_ERROR, "ZWave: Node not found! (NodeID: %d, 0x%02x)", nodeID, nodeID);
		return false;
	}
	else if ((packettype == pTypeGeneral) && (subtype == sTypeZWaveThermostatFanMode))
	{
		const _tGeneralDevice* pMeter = reinterpret_cast<const _tGeneralDevice*>(pdata);
		uint8_t ID1 = (uint8_t)((pMeter->intval1 & 0xFF000000) >> 24);
		uint8_t ID2 = (uint8_t)((pMeter->intval1 & 0x00FF0000) >> 16);
		uint8_t ID3 = (uint8_t)((pMeter->intval1 & 0x0000FF00) >> 8);
		uint8_t ID4 = (uint8_t)((pMeter->intval1 & 0x000000FF));

		uint8_t nodeID = ID3;
		uint8_t instanceID = ID4;
		int indexID = ID1;

		pDevice = FindDevice(nodeID, instanceID, indexID, ZDTYPE_SENSOR_THERMOSTAT_FAN_MODE);
		if (pDevice)
		{
			int tMode = pMeter->intval2;
			SetThermostatFanMode(nodeID, instanceID, pDevice->commandClassID, tMode);
			return true;
		}
		_log.Log(LOG_ERROR, "ZWave: Node not found! (NodeID: %d, 0x%02x)", nodeID, nodeID);
		return false;
	}
	else if (packettype == pTypeColorSwitch)
	{
		const _tColorSwitch* pLed = reinterpret_cast<const _tColorSwitch*>(pdata);
		uint8_t ID1 = (uint8_t)((pLed->id & 0xFF000000) >> 24);
		uint8_t ID2 = (uint8_t)((pLed->id & 0x00FF0000) >> 16);
		uint8_t ID3 = (uint8_t)((pLed->id & 0x0000FF00) >> 8);
		uint8_t ID4 = (uint8_t)pLed->id & 0x000000FF;
		uint8_t nodeID = ID3;
		uint8_t instanceID = ID4;
		int indexID = ID1;
		pDevice = FindDevice(nodeID, instanceID, indexID, ZDTYPE_SWITCH_RGBW);
		if (pDevice)
		{
			if (pLed->command == Color_LedOff)
			{
				instanceID = 2;
				return SwitchLight(pDevice, instanceID, 0);
			}
			else if (pLed->command == Color_LedOn)
			{
				instanceID = 2;
				return SwitchLight(pDevice, instanceID, 255);
			}
			else if (pLed->command == Color_SetBrightnessLevel)
			{
				instanceID = 2;
				int ivalue = pLed->value;
				if (ivalue > 99)
					ivalue = 99; //99 is fully on
				return SwitchLight(pDevice, instanceID, ivalue);
			}
			else if (pLed->command == Color_SetColorToWhite)
			{
				instanceID = 3;//red
				if (!SwitchLight(pDevice, instanceID, 0))
					return false;
				instanceID = 4;//green
				if (!SwitchLight(pDevice, instanceID, 0))
					return false;
				instanceID = 5;//blue
				if (!SwitchLight(pDevice, instanceID, 0))
					return false;
				instanceID = 6;//white
				if (!SwitchLight(pDevice, instanceID, 100))
					return false;
				return true;
			}
			else if (pLed->command == Color_SetColor)
			{
				int ivalue = pLed->value;
				if (ivalue > 99)
					ivalue = 99; //99 is fully on
				if (pLed->color.mode == ColorModeWhite)
				{
					instanceID = 3;//red
					if (!SwitchLight(pDevice, instanceID, 0))
						return false;
					instanceID = 4;//green
					if (!SwitchLight(pDevice, instanceID, 0))
						return false;
					instanceID = 5;//blue
					if (!SwitchLight(pDevice, instanceID, 0))
						return false;
					instanceID = 6;//white
					if (!SwitchLight(pDevice, instanceID, 100))
						return false;
				}
				else if (pLed->color.mode == ColorModeRGB)
				{
					instanceID = 6;//white
					if (!SwitchLight(pDevice, instanceID, 0))
						return false;

					instanceID = 3;//red
					if (!SwitchLight(pDevice, instanceID, pLed->color.r))
						return false;
					instanceID = 4;//green
					if (!SwitchLight(pDevice, instanceID, pLed->color.g))
						return false;
					instanceID = 5;//blue
					if (!SwitchLight(pDevice, instanceID, pLed->color.b))
						return false;
					_log.Log(LOG_NORM, "Red: %03d, Green:%03d, Blue:%03d", pLed->color.r, pLed->color.g, pLed->color.b);
				}
				else if (pLed->color.mode == ColorModeCustom)
				{
					instanceID = 3;//red
					if (!SwitchLight(pDevice, instanceID, pLed->color.r))
						return false;
					instanceID = 4;//green
					if (!SwitchLight(pDevice, instanceID, pLed->color.g))
						return false;
					instanceID = 5;//blue
					if (!SwitchLight(pDevice, instanceID, pLed->color.b))
						return false;
					instanceID = 6;//white
					if (!SwitchLight(pDevice, instanceID, pLed->color.ww))
						return false;
					_log.Log(LOG_NORM, "Red: %03d, Green:%03d, Blue:%03d", pLed->color.r, pLed->color.g, pLed->color.b);
				}
				else
				{
					_log.Log(LOG_STATUS, "ZWave: SetRGBColour - Color mode %d is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum", pLed->color.mode);
					return false;
				}
				instanceID = 2;//brightness
				if (!SwitchLight(pDevice, instanceID, ivalue))
					return false;
				return true;
			}
		}
		else
		{
			pDevice = FindDevice(nodeID, instanceID, indexID, ZDTYPE_SWITCH_COLOR);
			if (pDevice)
			{
				std::stringstream sstr;
				if (pLed->command == Color_LedOff)
				{
					instanceID = 1;
					return SwitchLight(pDevice, instanceID, 0);
				}
				else if (pLed->command == Color_LedOn)
				{
					instanceID = 1;
					return SwitchLight(pDevice, instanceID, 255);
				}
				else if (pLed->command == Color_SetBrightnessLevel)
				{
					instanceID = 1;
					int ivalue = pLed->value;
					if (ivalue > 99)
						ivalue = 99; //99 is fully on
					return SwitchLight(pDevice, instanceID, ivalue);
				}
				else if (pLed->command == Color_SetColorToWhite)
				{
					int Brightness = 100;
					int wWhite = round((255.0f / 100.0f) * float(Brightness));
					int cWhite = 0;
					sstr << "#000000"
						<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << wWhite
						<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << cWhite;
					instanceID = 1;
					return SwitchColor(nodeID, instanceID, sstr.str());
				}
				else if (pLed->command == Color_SetColor)
				{
					int red = 0, green = 0, blue = 0, wWhite = 0, cWhite = 0;

					instanceID = 1;
					int ivalue = pLed->value;
					if (ivalue > 99)
						ivalue = 99; //99 is fully on
					if (!SwitchLight(pDevice, instanceID, ivalue))
						return false;

					if (pLed->color.mode == ColorModeWhite)
					{
						wWhite = 255;
					}
					else if (pLed->color.mode == ColorModeTemp)
					{
						wWhite = pLed->color.t;
						cWhite = 255 - pLed->color.t;
					}
					else if (pLed->color.mode == ColorModeRGB)
					{
						red = pLed->color.r;
						green = pLed->color.g;
						blue = pLed->color.b;
					}
					else if (pLed->color.mode == ColorModeCustom)
					{
						red = pLed->color.r;
						green = pLed->color.g;
						blue = pLed->color.b;
						wWhite = pLed->color.ww;
						cWhite = pLed->color.cw;
					}
					else
					{
						_log.Log(LOG_STATUS, "ZWave: SetRGBColour - Color mode %d is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum", pLed->color.mode);
						return false;
					}

					instanceID = 1;
					sstr << "#"
						<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << red
						<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << green
						<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << blue
						<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << wWhite
						<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << cWhite;

					std::string sColor = sstr.str();

					if (!SwitchColor(nodeID, instanceID, sColor))
						return false;

					_log.Log(LOG_NORM, "Red: %03d, Green:%03d, Blue:%03d, wWhite:%03d, cWhite:%03d", red, green, blue, wWhite, cWhite);
					return true;
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "ZWave: Node not found! (NodeID: %d, 0x%02x)", nodeID, nodeID);
				return false;
			}
		}
	}
	return true;
}

