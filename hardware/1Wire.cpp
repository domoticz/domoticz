
#include "stdafx.h"
#include "1Wire.h"
#include "hardwaretypes.h"
#include "1Wire/1WireByOWFS.h"
#include "1Wire/1WireByKernel.h"
#ifdef WIN32
#include "1Wire/1WireForWindows.h"
#endif // WIN32
#include "../main/Logger.h"
#include "../main/RFXtrx.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define round(a) ( int ) ( a + .5 )

extern CSQLHelper m_sql;

C1Wire::C1Wire(const int ID, const int sensorThreadPeriod, const int switchThreadPeriod, const std::string& path) :
	m_stoprequested(false),
	m_system(NULL),
	m_sensorThreadPeriod(sensorThreadPeriod),
	m_switchThreadPeriod(switchThreadPeriod),
	m_path(path)
{
	m_HwdID = ID;

	// Defaults for pre-existing 1wire instances
	if (0 == m_sensorThreadPeriod)
		m_sensorThreadPeriod = 5 * 60 * 1000;

	if (0 == m_switchThreadPeriod)
			m_switchThreadPeriod = 100;

	DetectSystem();
}

C1Wire::~C1Wire()
{
}

bool C1Wire::Have1WireSystem()
{
	LogSystem();

#ifdef WIN32
	return (C1WireForWindows::IsAvailable());
#else // WIN32
	return true;
#endif // WIN32
}

void C1Wire::DetectSystem()
{
	LogSystem();

#ifdef WIN32
	if (!m_system && C1WireForWindows::IsAvailable())
		m_system=new C1WireForWindows();
#else // WIN32

	// Using the both systems at same time results in conflicts,
	// see http://owfs.org/index.php?page=w1-project.
	if (C1WireByKernel::IsAvailable()) {
		m_system=new C1WireByKernel();
		_log.Log(LOG_STATUS,"1-Wire: Using Kernel...");
	} else {
		m_system=new C1WireByOWFS(m_path);
		_log.Log(LOG_STATUS,"1-Wire: Using OWFS...");
	}

#endif // WIN32
}

void C1Wire::LogSystem()
{
	static bool alreadyLogged=false;
	if (alreadyLogged)
		return;
	alreadyLogged=true;

#ifdef WIN32
	if (C1WireForWindows::IsAvailable())
		{ _log.Log(LOG_STATUS,"1-Wire support available..."); return; }
#else // WIN32

	if (C1WireByKernel::IsAvailable())
		{ _log.Log(LOG_STATUS,"1-Wire support available (By Kernel)..."); return; }

	_log.Log(LOG_STATUS,"1-Wire support available (By OWFS)..."); return;

#endif // WIN32
}

bool C1Wire::StartHardware()
{
	// Start worker thread
	if (0 != m_sensorThreadPeriod)
	{
		m_threadSensors = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&C1Wire::SensorThread, this)));
	}
	if (0 != m_switchThreadPeriod)
	{
		m_threadSwitches = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&C1Wire::SwitchThread, this)));
	}
	m_bIsStarted=true;
	sOnConnected(this);
	StartHeartbeatThread();
	return (m_threadSensors!=NULL && m_threadSwitches!=NULL);
}

bool C1Wire::StopHardware()
{
	if (m_threadSensors)
	{
		m_stoprequested = true;
		m_threadSensors->join();
	}

	if (m_threadSwitches)
	{
		m_stoprequested = true;
		m_threadSwitches->join();
	}

	m_bIsStarted=false;
	if (m_system)
	{
		delete m_system;
		m_system=NULL;
	}
	StopHeartbeatThread();
	return true;
}

void C1Wire::SensorThread()
{
	int pollPeriod = m_sensorThreadPeriod;
	int pollIterations = 1;

	if (pollPeriod > 1000)
	{
		pollIterations = pollPeriod / 1000;
		pollPeriod = 1000;
	}

	int iteration = 0;

	while (!m_stoprequested)
	{
		sleep_milliseconds(pollPeriod);
		if (0 == iteration++ % pollIterations) // may glitch on overflow, not disastrous
		{
			if (m_sql.m_bAcceptNewHardware || 1 == iteration) {
				BuildSensorList();
			}

			PollSensors();
		}
	}

	_log.Log(LOG_STATUS, "1-Wire: Sensor thread terminating");
}

void C1Wire::SwitchThread()
{
	int pollPeriod = m_switchThreadPeriod;

	// Rescan the bus once every 10 seconds if requested
#define HARDWARE_RESCAN_PERIOD 10000
	int rescanIterations = HARDWARE_RESCAN_PERIOD / pollPeriod;
	if (0 == rescanIterations)
		rescanIterations = 1;

	int iteration = 0;

	while (!m_stoprequested)
	{
		sleep_milliseconds(pollPeriod);

		if (0 == iteration++ % rescanIterations) // may glitch on overflow, not disastrous
		{
			if (m_sql.m_bAcceptNewHardware || 1 == iteration)
			{
				BuildSwitchList();
			}
		}

		PollSwitches();
	}

	_log.Log(LOG_STATUS, "1-Wire: Switch thread terminating");
}


bool C1Wire::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *pSen= reinterpret_cast<const tRBUF*>(pdata);

	if (!m_system)
		return false;//no 1-wire support

	if (pSen->ICMND.packettype==pTypeLighting2 && pSen->LIGHTING2.subtype==sTypeAC)
	{
		//light command
		unsigned char deviceIdByteArray[DEVICE_ID_SIZE]={0};
		deviceIdByteArray[0]=pSen->LIGHTING2.id1;
		deviceIdByteArray[1]=pSen->LIGHTING2.id2;
		deviceIdByteArray[2]=pSen->LIGHTING2.id3;
		deviceIdByteArray[3]=pSen->LIGHTING2.id4;

		m_system->SetLightState(ByteArrayToDeviceId(deviceIdByteArray),pSen->LIGHTING2.unitcode,pSen->LIGHTING2.cmnd==light2_sOn);
		return true;
	}
	return false;
}

bool IsTemperatureValid(_e1WireFamilyType deviceFamily, float temperature)
{
	if (temperature<=-300 || temperature>=381)
		return false;

	// Some devices has a power-on value at 85C and -127C, we have to filter it
	switch (deviceFamily)
	{
		case high_precision_digital_thermometer:
		case Econo_Digital_Thermometer:
		case programmable_resolution_digital_thermometer:
		case Temperature_memory:
		case Temperature_IO:
			if ((temperature == 85)|| (temperature == -127))
				return false;
	}

	return true;
}

void C1Wire::BuildSensorList() {
	if (!m_system)
		return;

	std::vector<_t1WireDevice> devices;
	_log.Log(LOG_STATUS, "1-Wire: Searching sensors");
	m_sensors.clear();
	m_system->GetDevices(devices);

	std::vector<_t1WireDevice>::const_iterator device;
	for (device=devices.begin(); device!=devices.end(); ++device)
	{
		switch((*device).family)
		{
		case high_precision_digital_thermometer:
		case Thermachron:
		case Econo_Digital_Thermometer:
		case Temperature_memory:
		case programmable_resolution_digital_thermometer:
		case Temperature_IO:
		case Environmental_Monitors:
		case _4k_ram_with_counter:
		case quad_ad_converter:
		case smart_battery_monitor:
			m_sensors.push_back(*device);
			break;

		default:
			break;
		}

	}
	devices.clear();
}

void C1Wire::PollSensors()
{
	if (!m_system)
		return;

	if (m_sensors.size() > 2)
	{
		m_system->StartSimultaneousTemperatureRead();
	}

	// Parse our devices (have to test m_stoprequested because it can take some time in case of big networks)
	std::vector<_t1WireDevice>::const_iterator itt;
	for (itt=m_sensors.begin(); itt!=m_sensors.end() && !m_stoprequested; ++itt)
	{
		const _t1WireDevice& device=*itt;

		// Manage families specificities
		switch(device.family)
		{
		case high_precision_digital_thermometer:
		case Thermachron:
		case Econo_Digital_Thermometer:
		case Temperature_memory:
		case programmable_resolution_digital_thermometer:
		case Temperature_IO:
			{
				float temperature=m_system->GetTemperature(device);
				if (IsTemperatureValid(device.family, temperature))
				{
					ReportTemperature(device.devid, temperature);
				}
				break;
			}

		case Environmental_Monitors:
			{
				float temperature = m_system->GetTemperature(device);
				if (IsTemperatureValid(device.family, temperature))
				{
					ReportTemperatureHumidity(device.devid, temperature, m_system->GetHumidity(device));
				}
				ReportPressure(device.devid,m_system->GetPressure(device));
				break;
			}

		case _4k_ram_with_counter:
			{
				ReportCounter(device.devid,0,m_system->GetCounter(device,0));
				ReportCounter(device.devid,1,m_system->GetCounter(device,1));
				break;
			}

		case quad_ad_converter:
			{
				ReportVoltage(device.devid, 0, m_system->GetVoltage(device, 0));
				ReportVoltage(device.devid, 1, m_system->GetVoltage(device, 1));
				ReportVoltage(device.devid, 2, m_system->GetVoltage(device, 2));
				ReportVoltage(device.devid, 3, m_system->GetVoltage(device, 3));
				break;
			}

		case smart_battery_monitor:
			{
				float temperature = m_system->GetTemperature(device);
				if (IsTemperatureValid(device.family, temperature))
				{
					ReportTemperature(device.devid, temperature);
				}
				ReportHumidity(device.devid,m_system->GetHumidity(device));
				ReportVoltage(device.devid, 0, m_system->GetVoltage(device, 0));   // VAD
				ReportVoltage(device.devid, 1, m_system->GetVoltage(device, 1));   // VDD
				ReportVoltage(device.devid, 2, m_system->GetVoltage(device, 2));   // vis
				ReportPressure(device.devid,m_system->GetPressure(device));
				// Commonly used as Illuminance sensor, see http://www.hobby-boards.com/store/products/Solar-Radiation-Detector.html
				ReportIlluminance(device.devid, m_system->GetIlluminance(device));
				break;
			}
		default: // not a supported sensor
			break;
		}
	}
}

void C1Wire::BuildSwitchList() {
	if (!m_system)
		return;

	std::vector<_t1WireDevice> devices;
	_log.Log(LOG_STATUS, "1-Wire: Searching switches");
	m_switches.clear();
	m_system->GetDevices(devices);

	std::vector<_t1WireDevice>::const_iterator device;
	for (device=devices.begin(); device!=devices.end(); ++device)
	{
		switch((*device).family)
		{
		case Addresable_Switch:
		case microlan_coupler:
		case dual_addressable_switch_plus_1k_memory:
		case _8_channel_addressable_switch:
		case Temperature_IO:
		case dual_channel_addressable_switch:
		case _4k_EEPROM_with_PIO:
			m_switches.push_back(*device);
			break;

		default:
			break;
		}

	}
	devices.clear();
}

void C1Wire::PollSwitches()
{
	if (!m_system)
		return;

	// Parse our devices (have to test m_stoprequested because it can take some time in case of big networks)
	std::vector<_t1WireDevice>::const_iterator itt;
	for (itt=m_switches.begin(); itt!=m_switches.end() && !m_stoprequested; ++itt)
	{
		const _t1WireDevice& device=*itt;

		// Manage families specificities
		switch(device.family)
		{
		case Addresable_Switch:
		case microlan_coupler:
			{
				ReportLightState(device.devid,0,m_system->GetLightState(device,0));
				break;
			}

		case dual_addressable_switch_plus_1k_memory:
			{
				ReportLightState(device.devid,0,m_system->GetLightState(device,0));
				if (m_system->GetNbChannels(device) == 2)
					ReportLightState(device.devid,1,m_system->GetLightState(device,1));
				break;
			}

		case _8_channel_addressable_switch:
			{
				ReportLightState(device.devid,0,m_system->GetLightState(device,0));
				ReportLightState(device.devid,1,m_system->GetLightState(device,1));
				ReportLightState(device.devid,2,m_system->GetLightState(device,2));
				ReportLightState(device.devid,3,m_system->GetLightState(device,3));
				ReportLightState(device.devid,4,m_system->GetLightState(device,4));
				ReportLightState(device.devid,5,m_system->GetLightState(device,5));
				ReportLightState(device.devid,6,m_system->GetLightState(device,6));
				ReportLightState(device.devid,7,m_system->GetLightState(device,7));
				break;
			}

		case Temperature_IO:
			{
				ReportLightState(device.devid, 0, m_system->GetLightState(device, 0));
				ReportLightState(device.devid, 1, m_system->GetLightState(device, 1));
				break;
			}

		case dual_channel_addressable_switch:
		case _4k_EEPROM_with_PIO:
			{
				ReportLightState(device.devid,0,m_system->GetLightState(device,0));
				ReportLightState(device.devid,1,m_system->GetLightState(device,1));
				break;
			}

		default: // Not a supported switch
			break;
		}
	}
}


void C1Wire::ReportTemperature(const std::string& deviceId, const float temperature)
{
	if (temperature == -1000.0)
		return;

	unsigned char deviceIdByteArray[DEVICE_ID_SIZE]={0};
	DeviceIdToByteArray(deviceId,deviceIdByteArray);

	RBUF tsen;
	memset(&tsen,0,sizeof(RBUF));
	tsen.TEMP.packetlength=sizeof(tsen.TEMP)-1;
	tsen.TEMP.packettype=pTypeTEMP;
	tsen.TEMP.subtype=sTypeTEMP10;
	tsen.TEMP.battery_level=9;
	tsen.TEMP.rssi=12;
	tsen.TEMP.id1=(BYTE)deviceIdByteArray[0];
	tsen.TEMP.id2=(BYTE)deviceIdByteArray[1];

	tsen.TEMP.tempsign=(temperature>=0)?0:1;
	int at10=round(abs(temperature*10.0f));
	tsen.TEMP.temperatureh=(BYTE)(at10/256);
	at10-=(tsen.TEMP.temperatureh*256);
	tsen.TEMP.temperaturel=(BYTE)(at10);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP, NULL, 255);
}

void C1Wire::ReportHumidity(const std::string& deviceId, const float humidity)
{
	if (humidity == -1000.0)
		return;

	unsigned char deviceIdByteArray[DEVICE_ID_SIZE]={0};
	DeviceIdToByteArray(deviceId,deviceIdByteArray);

	RBUF tsen;
	memset(&tsen,0,sizeof(RBUF));
	tsen.HUM.packetlength=sizeof(tsen.HUM)-1;
	tsen.HUM.packettype=pTypeHUM;
	tsen.HUM.subtype=sTypeHUM2;
	tsen.HUM.battery_level=9;
	tsen.HUM.rssi=12;
	tsen.TEMP.id1=(BYTE)deviceIdByteArray[0];
	tsen.TEMP.id2=(BYTE)deviceIdByteArray[1];

	tsen.HUM.humidity=(BYTE)round(humidity);
	tsen.HUM.humidity_status=Get_Humidity_Level(tsen.HUM.humidity);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.HUM, NULL, 255);
}

void C1Wire::ReportPressure(const std::string& deviceId, const float pressure)
{
	if (pressure == -1000.0)
		return;
	unsigned char deviceIdByteArray[DEVICE_ID_SIZE] = { 0 };
	DeviceIdToByteArray(deviceId, deviceIdByteArray);

	int lID = (deviceIdByteArray[0] << 24) + (deviceIdByteArray[1] << 16) + (deviceIdByteArray[2] << 8) + deviceIdByteArray[3];
	SendPressureSensor(0, lID, 255, pressure, "Pressure");
}

void C1Wire::ReportTemperatureHumidity(const std::string& deviceId, const float temperature, const float humidity)
{
	if ((temperature == -1000.0) || (humidity == -1000.0))
		return;
	unsigned char deviceIdByteArray[DEVICE_ID_SIZE]={0};
	DeviceIdToByteArray(deviceId,deviceIdByteArray);

	uint16_t NodeID = (deviceIdByteArray[0] << 8) | deviceIdByteArray[1];
	SendTempHumSensor(NodeID, 255, temperature, round(humidity), "TempHum");
}

void C1Wire::ReportLightState(const std::string& deviceId, const int unit, const bool state)
{
#if defined(_DEBUG)
	_log.Log(LOG_STATUS, "device '%s' unit %d state is %s", deviceId.c_str(), unit, (state) ? "on" : "off");
#endif

// check - is state changed ?
	char num[16];
	sprintf(num, "%s/%d", deviceId.c_str(), unit);
	const std::string id(num);

	std::map<std::string, bool>::iterator it;
	it = m_LastSwitchState.find(id);
	if (it != m_LastSwitchState.end())
	{
		if (m_LastSwitchState[id] == state)
		{
			return;
		}
	}

#if defined(_DEBUG)
	_log.Log(LOG_STATUS, "device '%s' unit %d changed state to %s", deviceId.c_str(), unit, (state) ? "on" : "off");
#endif

	m_LastSwitchState[id] = state;

	unsigned char deviceIdByteArray[DEVICE_ID_SIZE]={0};
	DeviceIdToByteArray(deviceId,deviceIdByteArray);

	RBUF tsen;
	memset(&tsen,0,sizeof(RBUF));
	tsen.LIGHTING2.packetlength=sizeof(tsen.LIGHTING2)-1;
	tsen.LIGHTING2.packettype=pTypeLighting2;
	tsen.LIGHTING2.subtype=sTypeAC;
	tsen.LIGHTING2.seqnbr=0;
	tsen.LIGHTING2.id1=(BYTE)deviceIdByteArray[0];
	tsen.LIGHTING2.id2=(BYTE)deviceIdByteArray[1];
	tsen.LIGHTING2.id3=(BYTE)deviceIdByteArray[2];
	tsen.LIGHTING2.id4=(BYTE)deviceIdByteArray[3];
	tsen.LIGHTING2.unitcode=unit;
	tsen.LIGHTING2.cmnd=state?light2_sOn:light2_sOff;
	tsen.LIGHTING2.level=0;
	tsen.LIGHTING2.rssi=12;
	sDecodeRXMessage(this, (const unsigned char *)&tsen.LIGHTING2, NULL, 255);
}

void C1Wire::ReportCounter(const std::string& deviceId, const int unit, const unsigned long counter)
{
	unsigned char deviceIdByteArray[DEVICE_ID_SIZE]={0};
	DeviceIdToByteArray(deviceId,deviceIdByteArray);

	RBUF tsen;
	memset(&tsen,0,sizeof(RBUF));
	tsen.RFXMETER.packetlength=sizeof(tsen.RFXMETER)-1;
	tsen.RFXMETER.packettype=pTypeRFXMeter;
	tsen.RFXMETER.subtype=sTypeRFXMeterCount;
	tsen.RFXMETER.rssi=12;
	tsen.RFXMETER.id1=(BYTE)deviceIdByteArray[0];
	tsen.RFXMETER.id2=(BYTE)deviceIdByteArray[1] + unit;

	tsen.RFXMETER.count1 = (BYTE)((counter & 0xFF000000) >> 24);
	tsen.RFXMETER.count2 = (BYTE)((counter & 0x00FF0000) >> 16);
	tsen.RFXMETER.count3 = (BYTE)((counter & 0x0000FF00) >> 8);
	tsen.RFXMETER.count4 = (BYTE)(counter & 0x000000FF);
	sDecodeRXMessage(this, (const unsigned char *)&tsen.RFXMETER, NULL, 255);
}

void C1Wire::ReportVoltage(const std::string& deviceId, const int unit, const int voltage)
{
	if (voltage == -1000.0)
		return;

	RBUF tsen;
	memset(&tsen,0,sizeof(RBUF));
	tsen.RFXSENSOR.packetlength=sizeof(tsen.RFXSENSOR)-1;
	tsen.RFXSENSOR.packettype=pTypeRFXSensor;
	tsen.RFXSENSOR.subtype=sTypeRFXSensorVolt;
	tsen.RFXSENSOR.rssi=12;
	tsen.RFXSENSOR.id=unit+1;

	tsen.RFXSENSOR.msg1 = (BYTE)(voltage/256);
	tsen.RFXSENSOR.msg2 = (BYTE)(voltage-(tsen.RFXSENSOR.msg1*256));
	sDecodeRXMessage(this, (const unsigned char *)&tsen.RFXSENSOR, NULL, 255);
}

void C1Wire::ReportIlluminance(const std::string& deviceId, const float illuminescence)
{
	if (illuminescence == -1000.0)
		return;

	unsigned char deviceIdByteArray[DEVICE_ID_SIZE] = { 0 };
	DeviceIdToByteArray(deviceId, deviceIdByteArray);

	uint8_t NodeID = deviceIdByteArray[0] ^ deviceIdByteArray[1];
	SendSolarRadiationSensor(NodeID, 255, illuminescence, "Solar Radiation");
}
