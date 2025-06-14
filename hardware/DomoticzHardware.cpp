#include "stdafx.h"
#include <iostream>
#include "DomoticzHardware.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"
#include "hardwaretypes.h"

CDomoticzHardwareBase::CDomoticzHardwareBase()
{
	mytime(&m_LastHeartbeat);
	mytime(&m_LastHeartbeatReceive);
};

bool CDomoticzHardwareBase::CustomCommand(const uint64_t /*idx*/, const std::string& /*sCommand*/)
{
	return false;
}

std::string CDomoticzHardwareBase::GetManualSwitchesJsonConfiguration() const
{
	return std::string("");
}

void CDomoticzHardwareBase::GetManualSwitchParameters(const std::multimap<std::string, std::string> & /*Parameters*/, _eSwitchType & /*SwitchTypeInOut*/, int & /*LightTypeInOut*/,
	 int & /*dTypeOut*/, int &dSubTypeOut, std::string & /*devIDOut*/, std::string & /*sUnitOut*/) const
{
}

bool CDomoticzHardwareBase::Start()
{
	m_iHBCounter = 0;
	m_bIsStarted = StartHardware();
	return m_bIsStarted;
}

bool CDomoticzHardwareBase::Stop()
{
	m_bIsStarted = (!StopHardware());
	return (!m_bIsStarted);
}

bool CDomoticzHardwareBase::Restart()
{
	if (StopHardware())
	{
		m_bIsStarted = StartHardware();
		return m_bIsStarted;
	}
	return false;
}

bool CDomoticzHardwareBase::RestartWithDelay(const long seconds)
{
	if (StopHardware())
	{
		sleep_seconds(seconds);
		m_bIsStarted = StartHardware();
		return m_bIsStarted;
	}
	return false;
}

void CDomoticzHardwareBase::EnableOutputLog(const bool bEnableLog)
{
	m_bOutputLog = bEnableLog;
}

void CDomoticzHardwareBase::StartHeartbeatThread()
{
	StartHeartbeatThread("Domoticz_HBWork");
}

void CDomoticzHardwareBase::StartHeartbeatThread(const char* ThreadName)
{
	m_Heartbeatthread = std::make_shared<std::thread>([this] { Do_Heartbeat_Work(); });
	SetThreadName(m_Heartbeatthread->native_handle(), ThreadName);
}


void CDomoticzHardwareBase::StopHeartbeatThread()
{
	if (m_Heartbeatthread)
	{
		RequestStop();
		m_Heartbeatthread->join();
		// Wait a while. The read thread might be reading. Adding this prevents a pointer error in the async serial class.
		sleep_milliseconds(10);
		m_Heartbeatthread.reset();
	}
}

void CDomoticzHardwareBase::Do_Heartbeat_Work()
{
	int secCounter = 0;
	int hbCounter = 0;
	while (!IsStopRequested(200))
	{
		secCounter++;
		if (secCounter == 5)
		{
			secCounter = 0;
			hbCounter++;
			if (hbCounter % 12 == 0) {
				mytime(&m_LastHeartbeat);
			}
		}
	}
}

void CDomoticzHardwareBase::SetHeartbeatReceived()
{
	mytime(&m_LastHeartbeatReceive);
}

int CDomoticzHardwareBase::SetThreadNameInt(const std::thread::native_handle_type& thread)
{
	return SetThreadName(thread, m_Name.c_str());
}

//Log Helper functions
#define MAX_LOG_LINE_LENGTH (2048*3)
void CDomoticzHardwareBase::Log(const _eLogLevel level, const std::string& sLogline)
{
	if (!(m_LogLevelEnabled & (uint32_t)level))
		return; //this type of log is disabled

	_log.Log(level, "%s: %s", m_Name.c_str(), sLogline.c_str());
}

void CDomoticzHardwareBase::Log(const _eLogLevel level, const char* logline, ...)
{
	if (!(m_LogLevelEnabled & (uint32_t)level))
		return; // this type of log is disabled

	va_list argList;
	char cbuffer[MAX_LOG_LINE_LENGTH];
	va_start(argList, logline);
	vsnprintf(cbuffer, sizeof(cbuffer), logline, argList);
	va_end(argList);
	_log.Log(level, "%s: %s", m_Name.c_str(), cbuffer);
}

void CDomoticzHardwareBase::Debug(const _eDebugLevel level, const std::string& sLogline)
{
	_log.Debug(level, "%s: %s", m_Name.c_str(), sLogline.c_str());
}

void CDomoticzHardwareBase::Debug(const _eDebugLevel level, const char* logline, ...)
{
	va_list argList;
	char cbuffer[MAX_LOG_LINE_LENGTH];
	va_start(argList, logline);
	vsnprintf(cbuffer, sizeof(cbuffer), logline, argList);
	va_end(argList);
	_log.Debug(level, "%s: %s", m_Name.c_str(), cbuffer);
}

//Sensor Helpers
void CDomoticzHardwareBase::SendTempSensor(const int NodeID, const int BatteryLevel, const float temperature, const std::string& defaultname, const int RssiLevel /* =12 */)
{
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.TEMP.packetlength = sizeof(tsen.TEMP) - 1;
	tsen.TEMP.packettype = pTypeTEMP;
	tsen.TEMP.subtype = sTypeTEMP5;
	tsen.TEMP.battery_level = BatteryLevel;
	tsen.TEMP.rssi = RssiLevel;
	tsen.TEMP.id1 = (NodeID & 0xFF00) >> 8;
	tsen.TEMP.id2 = NodeID & 0xFF;
	tsen.TEMP.tempsign = (temperature >= 0) ? 0 : 1;
	int at10 = ground(std::abs(temperature * 10.0F));
	tsen.TEMP.temperatureh = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP.temperatureh * 256);
	tsen.TEMP.temperaturel = (BYTE)(at10);
	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendHumiditySensor(const int NodeID, const int BatteryLevel, const int humidity, const std::string& defaultname, const int RssiLevel /* =12 */)
{
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.HUM.packetlength = sizeof(tsen.HUM) - 1;
	tsen.HUM.packettype = pTypeHUM;
	tsen.HUM.subtype = sTypeHUM1;
	tsen.HUM.battery_level = BatteryLevel;
	tsen.HUM.rssi = RssiLevel;
	tsen.HUM.id1 = (NodeID & 0xFF00) >> 8;
	tsen.HUM.id2 = NodeID & 0xFF;
	tsen.HUM.humidity = (BYTE)humidity;
	tsen.HUM.humidity_status = Get_Humidity_Level(tsen.HUM.humidity);
	sDecodeRXMessage(this, (const unsigned char *)&tsen.HUM, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendBaroSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float pressure, const int forecast, const std::string& defaultname)
{
	_tGeneralDevice gdevice;
	gdevice.subtype = sTypeBaro;
	gdevice.intval1 = (NodeID << 8) | ChildID;
	gdevice.intval2 = forecast;
	gdevice.floatval1 = pressure;
	sDecodeRXMessage(this, (const unsigned char *)&gdevice, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendTempHumSensor(const int NodeID, const int BatteryLevel, const float temperature, const int humidity, const std::string& defaultname, const int RssiLevel /* =12 */)
{
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.TEMP_HUM.packetlength = sizeof(tsen.TEMP_HUM) - 1;
	tsen.TEMP_HUM.packettype = pTypeTEMP_HUM;
	tsen.TEMP_HUM.subtype = sTypeTH5;
	tsen.TEMP_HUM.battery_level = BatteryLevel;
	tsen.TEMP_HUM.rssi = RssiLevel;
	tsen.TEMP_HUM.id1 = (NodeID & 0xFF00) >> 8;
	tsen.TEMP_HUM.id2 = NodeID & 0xFF;

	tsen.TEMP_HUM.tempsign = (temperature >= 0) ? 0 : 1;
	int at10 = ground(std::abs(temperature * 10.0F));
	tsen.TEMP_HUM.temperatureh = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP_HUM.temperatureh * 256);
	tsen.TEMP_HUM.temperaturel = (BYTE)(at10);
	tsen.TEMP_HUM.humidity = (BYTE)humidity;
	tsen.TEMP_HUM.humidity_status = Get_Humidity_Level(tsen.TEMP_HUM.humidity);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP_HUM, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendTempHumBaroSensor(const int NodeID, const int BatteryLevel, const float temperature, const int humidity, const float pressure, int forecast, const std::string& defaultname, const int RssiLevel /* =12 */)
{
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.TEMP_HUM_BARO.packetlength = sizeof(tsen.TEMP_HUM_BARO) - 1;
	tsen.TEMP_HUM_BARO.packettype = pTypeTEMP_HUM_BARO;
	tsen.TEMP_HUM_BARO.subtype = sTypeTHB1;
	tsen.TEMP_HUM_BARO.battery_level = BatteryLevel;
	tsen.TEMP_HUM_BARO.rssi = RssiLevel;
	tsen.TEMP_HUM_BARO.id1 = (NodeID & 0xFF00) >> 8;
	tsen.TEMP_HUM_BARO.id2 = NodeID & 0xFF;

	tsen.TEMP_HUM_BARO.tempsign = (temperature >= 0) ? 0 : 1;
	int at10 = ground(std::abs(temperature * 10.0F));
	tsen.TEMP_HUM_BARO.temperatureh = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP_HUM_BARO.temperatureh * 256);
	tsen.TEMP_HUM_BARO.temperaturel = (BYTE)(at10);
	tsen.TEMP_HUM_BARO.humidity = (BYTE)humidity;
	tsen.TEMP_HUM_BARO.humidity_status = Get_Humidity_Level(tsen.TEMP_HUM.humidity);

	int ab10 = ground(pressure);
	tsen.TEMP_HUM_BARO.baroh = (BYTE)(ab10 / 256);
	ab10 -= (tsen.TEMP_HUM_BARO.baroh * 256);
	tsen.TEMP_HUM_BARO.barol = (BYTE)(ab10);

	tsen.TEMP_HUM_BARO.forecast = (BYTE)forecast;

	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP_HUM_BARO, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendTempHumBaroSensorFloat(const int NodeID, const int BatteryLevel, const float temperature, const int humidity, const float pressure, const uint8_t forecast, const std::string& defaultname, const int RssiLevel /* =12 */)
{
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.TEMP_HUM_BARO.packetlength = sizeof(tsen.TEMP_HUM_BARO) - 1;
	tsen.TEMP_HUM_BARO.packettype = pTypeTEMP_HUM_BARO;
	tsen.TEMP_HUM_BARO.subtype = sTypeTHBFloat;
	tsen.TEMP_HUM_BARO.battery_level = 9;
	tsen.TEMP_HUM_BARO.rssi = RssiLevel;
	tsen.TEMP_HUM_BARO.id1 = (NodeID & 0xFF00) >> 8;
	tsen.TEMP_HUM_BARO.id2 = NodeID & 0xFF;

	tsen.TEMP_HUM_BARO.tempsign = (temperature >= 0) ? 0 : 1;
	int at10 = ground(std::abs(temperature * 10.0F));
	tsen.TEMP_HUM_BARO.temperatureh = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP_HUM_BARO.temperatureh * 256);
	tsen.TEMP_HUM_BARO.temperaturel = (BYTE)(at10);
	tsen.TEMP_HUM_BARO.humidity = (BYTE)humidity;
	tsen.TEMP_HUM_BARO.humidity_status = Get_Humidity_Level(tsen.TEMP_HUM.humidity);

	int ab10 = ground(pressure * 10.0F);
	tsen.TEMP_HUM_BARO.baroh = (BYTE)(ab10 / 256);
	ab10 -= (tsen.TEMP_HUM_BARO.baroh * 256);
	tsen.TEMP_HUM_BARO.barol = (BYTE)(ab10);
	tsen.TEMP_HUM_BARO.forecast = forecast;

	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP_HUM_BARO, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendTempBaroSensor(const uint8_t NodeID, const int BatteryLevel, const float temperature, const float pressure, const std::string& defaultname)
{
	_tTempBaro tsensor;
	tsensor.id1 = NodeID;
	tsensor.temp = temperature;
	tsensor.baro = pressure;
	tsensor.altitude = 188;

	//this is probably not good, need to take the rising/falling of the pressure into account?
	//any help would be welcome!

	tsensor.forecast = baroForecastNoInfo;
	if (tsensor.baro < 1000)
		tsensor.forecast = baroForecastRain;
	else if (tsensor.baro < 1020)
		tsensor.forecast = baroForecastCloudy;
	else if (tsensor.baro < 1030)
		tsensor.forecast = baroForecastPartlyCloudy;
	else
		tsensor.forecast = baroForecastSunny;
	sDecodeRXMessage(this, (const unsigned char *)&tsensor, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendSetPointSensor(const uint8_t ID1, const uint8_t ID2, const uint8_t ID3, const uint8_t ID4, const uint8_t Unit, const int BatteryLevel, const float Value, const std::string& defaultname)
{
	_tSetpoint setpoint;
	setpoint.subtype = sTypeSetpoint;
	setpoint.id1 = ID1;
	setpoint.id2 = ID2;
	setpoint.id3 = ID3;
	setpoint.id4 = ID4;
	setpoint.dunit = Unit;
	setpoint.value = Value;
	setpoint.battery_level = BatteryLevel;
	sDecodeRXMessage(this, (const unsigned char *)&setpoint, defaultname.c_str(), -1, nullptr);
}


void CDomoticzHardwareBase::SendDistanceSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float distance, const std::string& defaultname, const int RssiLevel /* =12 */)
{
	_tGeneralDevice gdevice;
	gdevice.subtype = sTypeDistance;
	gdevice.intval1 = (NodeID << 8) | ChildID;
	gdevice.floatval1 = distance;
	gdevice.rssi = RssiLevel;
	sDecodeRXMessage(this, (const unsigned char *)&gdevice, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendTextSensor(const int NodeID, const int ChildID, const int BatteryLevel, const std::string& textMessage, const std::string& defaultname)
{
	_tGeneralDevice gdevice;
	gdevice.subtype = sTypeTextStatus;
	gdevice.intval1 = (NodeID << 8) | ChildID;
	std::string sstatus = textMessage;
	if (sstatus.size() > 63)
		sstatus = sstatus.substr(0, 63);
	strcpy(gdevice.text, sstatus.c_str());
	sDecodeRXMessage(this, (const unsigned char *)&gdevice, defaultname.c_str(), BatteryLevel, nullptr);
}

std::string CDomoticzHardwareBase::GetTextSensorText(const int NodeID, const int ChildID, bool& bExists)
{
	bExists = false;
	std::string sTmp = std_format("%08X", (NodeID << 8) | ChildID);

	std::string ret;
	std::vector<std::vector<std::string>> result;

	result = m_sql.safe_query("SELECT sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d)", m_HwdID, sTmp.c_str(), int(pTypeGeneral),
				  int(sTypeTextStatus));
	if (!result.empty())
	{
		bExists = true;
		ret = result[0][0];
	}
	return ret;
}

void CDomoticzHardwareBase::SendRainSensor(const int NodeID, const int BatteryLevel, const float RainCounter, const std::string& defaultname, const int RssiLevel /* =12 */)
{
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.RAIN.packetlength = sizeof(tsen.RAIN) - 1;
	tsen.RAIN.packettype = pTypeRAIN;
	tsen.RAIN.subtype = sTypeRAIN3;
	tsen.RAIN.battery_level = BatteryLevel;
	tsen.RAIN.rssi = RssiLevel;
	tsen.RAIN.id1 = (NodeID & 0xFF00) >> 8;
	tsen.RAIN.id2 = NodeID & 0xFF;

	tsen.RAIN.rainrateh = 0;
	tsen.RAIN.rainratel = 0;

	uint64_t tr10 = int((float(RainCounter) * 10.0F));

	tsen.RAIN.raintotal1 = (BYTE)(tr10 / 65535);
	tr10 -= (tsen.RAIN.raintotal1 * 65535);
	tsen.RAIN.raintotal2 = (BYTE)(tr10 / 256);
	tr10 -= (tsen.RAIN.raintotal2 * 256);
	tsen.RAIN.raintotal3 = (BYTE)(tr10);
	sDecodeRXMessage(this, (const unsigned char *)&tsen.RAIN, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendRainSensorWU(const int NodeID, const int BatteryLevel, const float RainCounter, const float LastHour, const std::string& defaultname, const int RssiLevel)
{
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.RAIN.packetlength = sizeof(tsen.RAIN) - 1;
	tsen.RAIN.packettype = pTypeRAIN;
	tsen.RAIN.subtype = sTypeRAINWU;
	tsen.RAIN.battery_level = 9;
	tsen.RAIN.rssi = 12;
	tsen.RAIN.id1 = 0;
	tsen.RAIN.id2 = 1;

	tsen.RAIN.rainrateh = 0;
	tsen.RAIN.rainratel = 0;

	if (LastHour != 0)
	{
		uint64_t at10 = int((float(LastHour) * 10.0F));
		tsen.RAIN.rainrateh = (BYTE)(at10 / 256);
		at10 -= (tsen.RAIN.rainrateh * 256);
		tsen.RAIN.rainratel = (BYTE)(at10);
	}

	int tr10 = int((float(RainCounter) * 10.0F));
	tsen.RAIN.raintotal1 = 0;
	tsen.RAIN.raintotal2 = (BYTE)(tr10 / 256);
	tr10 -= (tsen.RAIN.raintotal2 * 256);
	tsen.RAIN.raintotal3 = (BYTE)(tr10);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.RAIN, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendRainRateSensor(const int NodeID, const int BatteryLevel, const float RainRate, const std::string& defaultname, const int RssiLevel /* =12 */)
{
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.RAIN.packetlength = sizeof(tsen.RAIN) - 1;
	tsen.RAIN.packettype = pTypeRAIN;
	tsen.RAIN.subtype = sTypeRAINByRate;
	tsen.RAIN.battery_level = BatteryLevel;
	tsen.RAIN.rssi = RssiLevel;
	tsen.RAIN.id1 = (NodeID & 0xFF00) >> 8;
	tsen.RAIN.id2 = NodeID & 0xFF;

	int at10 = ground(std::abs(RainRate * 10000.0F));
	tsen.RAIN.rainrateh = (BYTE)(at10 / 256);
	at10 -= (tsen.RAIN.rainrateh * 256);
	tsen.RAIN.rainratel = (BYTE)(at10);

	tsen.RAIN.raintotal1 = 0;
	tsen.RAIN.raintotal2 = 0;
	tsen.RAIN.raintotal3 = 0;

	sDecodeRXMessage(this, (const unsigned char *)&tsen.RAIN, defaultname.c_str(), BatteryLevel, nullptr);
}

float CDomoticzHardwareBase::GetRainSensorValue(const int NodeID, bool& bExists)
{
	std::string sIdx = std_format("%d", NodeID & 0xFFFF);
	int Unit = 0;

	std::vector<std::vector<std::string> > results;
	results = m_sql.safe_query("SELECT ID,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)", m_HwdID, sIdx.c_str(), Unit, int(pTypeRAIN), int(sTypeRAIN3));
	if (results.empty())
	{
		bExists = false;
		return 0.0F;
	}
	std::vector<std::string> splitresults;
	StringSplit(results[0][1], ";", splitresults);
	if (splitresults.size() != 2)
	{
		bExists = false;
		return 0.0F;
	}
	bExists = true;
	return (float)atof(splitresults[1].c_str());
}

bool CDomoticzHardwareBase::GetWindSensorValue(const int NodeID, int& WindDir, float& WindSpeed, float& WindGust, float& WindTemp, float& WindChill, bool bHaveWindTemp, bool& bExists)
{
	std::string sIdx = std_format("%d", NodeID & 0xFFFF);
	int Unit = 0;

	std::vector<std::vector<std::string> > results;
	if (!bHaveWindTemp)
		results = m_sql.safe_query("SELECT ID,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)", m_HwdID, sIdx.c_str(), Unit,
					   int(pTypeWIND), int(sTypeWINDNoTemp));
	else
		results = m_sql.safe_query("SELECT ID,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)", m_HwdID, sIdx.c_str(), Unit,
					   int(pTypeWIND), int(sTypeWIND4));
	if (results.empty())
	{
		bExists = false;
		return 0.0F;
	}
	std::vector<std::string> splitresults;
	StringSplit(results[0][1], ";", splitresults);

	if (splitresults.size() != 6)
	{
		bExists = false;
		return 0.0F;
	}
	bExists = true;
	WindDir = (int)atof(splitresults[0].c_str());
	WindSpeed = (float)atof(splitresults[2].c_str());
	WindGust = (float)atof(splitresults[3].c_str());
	WindTemp = (float)atof(splitresults[4].c_str());
	WindChill = (float)atof(splitresults[5].c_str());
	return bExists;
}

void CDomoticzHardwareBase::SendWattMeter(const int NodeID, const uint8_t ChildID, const int BatteryLevel, const float musage, const std::string& defaultname, const int RssiLevel /* =12 */)
{
	if ((musage < -m_sql.m_max_kwh_usage) || (musage > m_sql.m_max_kwh_usage))
	{
		Log(LOG_ERROR, "Power usage to high/low! Usage: %g Watt. Max Usage configured: %g. (NodeID: 0x%04X, ChildID: 0x%04X, SID: %s)", musage, m_sql.m_max_kwh_usage, NodeID, ChildID,
		    defaultname.c_str());
		return;
	}
	_tUsageMeter umeter;

	umeter.id1 = (uint8_t)((NodeID & 0xFF000000) >> 24);
	umeter.id2 = (uint8_t)((NodeID & 0xFF0000) >> 16);
	umeter.id3 = (uint8_t)((NodeID & 0xFF00) >> 8);
	umeter.id4 = (uint8_t)NodeID & 0xFF;
	umeter.dunit = ChildID;
	umeter.rssi = RssiLevel;
	umeter.fusage = musage;
	sDecodeRXMessage(this, (const unsigned char *)&umeter, defaultname.c_str(), BatteryLevel, nullptr);
}


//Obsolete, we should not call this anymore
//when all calls are removed, we should delete this function
void CDomoticzHardwareBase::SendKwhMeterOldWay(const int NodeID, const int ChildID, const int BatteryLevel, const double musage, const double mtotal, const std::string& defaultname, const int RssiLevel /* =12 */)
{
	SendKwhMeter(NodeID, ChildID, BatteryLevel, musage * 1000, mtotal, defaultname);
}

void CDomoticzHardwareBase::SendKwhMeter(const int NodeID, const int ChildID, const int BatteryLevel, const double musage, const double mtotal, const std::string& defaultname, const int RssiLevel /* =12 */)
{
	if ((musage < -m_sql.m_max_kwh_usage) || (musage > m_sql.m_max_kwh_usage))
	{
		Log(LOG_ERROR, "Power usage to high/low! Usage: %g Watt. Max Usage configured: %g. (NodeID: 0x%04X, ChildID: 0x%04X, SID: %s)", musage, m_sql.m_max_kwh_usage, NodeID, ChildID,
		    defaultname.c_str());
		return;
	}
	_tGeneralDevice gdevice;
	gdevice.subtype = sTypeKwh;
	gdevice.intval1 = (NodeID << 8) | ChildID;
	gdevice.floatval1 = (float)musage;
	gdevice.floatval2 = (float)(mtotal * 1000.0);
	gdevice.rssi = RssiLevel;
	sDecodeRXMessage(this, (const unsigned char *)&gdevice, defaultname.c_str(), BatteryLevel, nullptr);
}

double CDomoticzHardwareBase::GetKwhMeter(const int NodeID, const int ChildID, bool& bExists)
{
	int dID = (NodeID << 8) | ChildID;
	std::string sTmp = std_format("%08X", dID);

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d)", m_HwdID, sTmp.c_str(), int(pTypeGeneral), int(sTypeKwh));
	if (result.empty())
	{
		bExists = false;
		return 0;
	}
	std::vector<std::string> splitresults;
	StringSplit(result[0][0], ";", splitresults);
	if (splitresults.size() != 2)
	{
		bExists = false;
		return 0.0F;
	}
	bExists = true;
	return atof(splitresults[1].c_str());
}

void CDomoticzHardwareBase::SendMeterSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float metervalue, const std::string& defaultname, const int RssiLevel /* =12 */)
{
	unsigned long counter = (unsigned long)(metervalue * 1000.0F);
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.RFXMETER.packetlength = sizeof(tsen.RFXMETER) - 1;
	tsen.RFXMETER.packettype = pTypeRFXMeter;
	tsen.RFXMETER.subtype = sTypeRFXMeterCount;
	tsen.RFXMETER.rssi = RssiLevel;
	tsen.RFXMETER.id1 = (BYTE)NodeID;
	tsen.RFXMETER.id2 = (BYTE)ChildID;

	tsen.RFXMETER.count1 = (BYTE)((counter & 0xFF000000) >> 24);
	tsen.RFXMETER.count2 = (BYTE)((counter & 0x00FF0000) >> 16);
	tsen.RFXMETER.count3 = (BYTE)((counter & 0x0000FF00) >> 8);
	tsen.RFXMETER.count4 = (BYTE)(counter & 0x000000FF);
	sDecodeRXMessage(this, (const unsigned char *)&tsen.RFXMETER, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendLuxSensor(const uint8_t NodeID, const uint8_t ChildID, const uint8_t BatteryLevel, const float Lux, const std::string& defaultname)
{
	_tLightMeter lmeter;
	lmeter.id1 = 0;
	lmeter.id2 = 0;
	lmeter.id3 = 0;
	lmeter.id4 = NodeID;
	lmeter.dunit = ChildID;
	lmeter.fLux = Lux;
	lmeter.battery_level = BatteryLevel;
	sDecodeRXMessage(this, (const unsigned char *)&lmeter, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendAirQualitySensor(const uint8_t NodeID, const uint8_t ChildID, const int BatteryLevel, const int AirQuality, const std::string& defaultname)
{
	_tAirQualityMeter meter;
	meter.len = sizeof(_tAirQualityMeter) - 1;
	meter.type = pTypeAirQuality;
	meter.subtype = sTypeVoc;
	meter.airquality = AirQuality;
	meter.id1 = NodeID;
	meter.id2 = ChildID;
	sDecodeRXMessage(this, (const unsigned char *)&meter, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendSwitchIfNotExists(const int NodeID, const uint8_t ChildID, const int BatteryLevel, const bool bOn, const double Level, const std::string &defaultname,
						  const std::string &userName)
{
	//make device ID
	unsigned char ID1 = (unsigned char)((NodeID & 0xFF000000) >> 24);
	unsigned char ID2 = (unsigned char)((NodeID & 0xFF0000) >> 16);
	unsigned char ID3 = (unsigned char)((NodeID & 0xFF00) >> 8);
	unsigned char ID4 = (unsigned char)NodeID & 0xFF;

	std::string sIdx = std_format("%X%02X%02X%02X", ID1, ID2, ID3, ID4);

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)", m_HwdID, sIdx.c_str(), ChildID,
				  int(pTypeLighting2), int(sTypeAC));
	if (result.empty())
	{
		SendSwitch(NodeID, ChildID, BatteryLevel, bOn, Level, defaultname, userName);
	}
}

void CDomoticzHardwareBase::SendSwitchUnchecked(int NodeID, uint8_t ChildID, int BatteryLevel, bool bOn, double Level, const std::string &defaultname, const std::string &userName,
						int RssiLevel /* =12 */)
{
	double rlevel = (16.0 / 100.0) * Level;
	uint8_t level = (uint8_t)(rlevel);

	//make device ID
	unsigned char ID1 = (unsigned char)((NodeID & 0xFF000000) >> 24);
	unsigned char ID2 = (unsigned char)((NodeID & 0xFF0000) >> 16);
	unsigned char ID3 = (unsigned char)((NodeID & 0xFF00) >> 8);
	unsigned char ID4 = (unsigned char)NodeID & 0xFF;

	//Send as Lighting 2
	tRBUF lcmd;
	memset(&lcmd, 0, sizeof(RBUF));
	lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
	lcmd.LIGHTING2.packettype = pTypeLighting2;
	lcmd.LIGHTING2.subtype = sTypeAC;
	lcmd.LIGHTING2.id1 = ID1;
	lcmd.LIGHTING2.id2 = ID2;
	lcmd.LIGHTING2.id3 = ID3;
	lcmd.LIGHTING2.id4 = ID4;
	lcmd.LIGHTING2.unitcode = ChildID;
	if (!bOn)
	{
		lcmd.LIGHTING2.cmnd = light2_sOff;
	}
	else
	{
		if (level != 0)
			lcmd.LIGHTING2.cmnd = light2_sSetLevel;
		else
			lcmd.LIGHTING2.cmnd = light2_sOn;
	}
	lcmd.LIGHTING2.level = level;
	lcmd.LIGHTING2.filler = 0;
	lcmd.LIGHTING2.rssi = RssiLevel;
	sDecodeRXMessage(this, (const unsigned char*)& lcmd.LIGHTING2, defaultname.c_str(), BatteryLevel, userName.c_str());
}


void CDomoticzHardwareBase::SendSwitch(const int NodeID, const uint8_t ChildID, const int BatteryLevel, const bool bOn, const double Level, const std::string &defaultname, const std::string &userName,
				       const int RssiLevel /* =12 */)
{
	double rlevel = (16.0 / 100.0) * Level;
	int level = int(rlevel);

	//make device ID
	unsigned char ID1 = (unsigned char)((NodeID & 0xFF000000) >> 24);
	unsigned char ID2 = (unsigned char)((NodeID & 0xFF0000) >> 16);
	unsigned char ID3 = (unsigned char)((NodeID & 0xFF00) >> 8);
	unsigned char ID4 = (unsigned char)NodeID & 0xFF;

	std::string sIdx = std_format("%X%02X%02X%02X", ID1, ID2, ID3, ID4);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)", m_HwdID, sIdx.c_str(), ChildID,
				  int(pTypeLighting2), int(sTypeAC));
	if (!result.empty())
	{
		//check if we have a change, if not do not update it
		int nvalue = atoi(result[0][1].c_str());
		if ((!bOn) && (nvalue == light2_sOff))
			return;
		if ((bOn && (nvalue != light2_sOff)))
		{
			//Check Level
			int slevel = atoi(result[0][2].c_str());
			if (slevel == level)
				return;
		}
	}
	SendSwitchUnchecked(NodeID, ChildID, BatteryLevel, bOn, Level, defaultname, userName, RssiLevel);
}

void CDomoticzHardwareBase::SendBlindSensor(const uint8_t NodeID, const uint8_t ChildID, const int BatteryLevel, const uint8_t Command, const std::string &defaultname, const std::string &userName,
					    const int RssiLevel /* =12 */)
{
	//Send as Blinds
	tRBUF lcmd;
	memset(&lcmd, 0, sizeof(RBUF));
	lcmd.BLINDS1.packetlength = sizeof(lcmd.LIGHTING2) - 1;
	lcmd.BLINDS1.packettype = pTypeBlinds;
	lcmd.BLINDS1.subtype = sTypeBlindsT0;
	lcmd.BLINDS1.id1 = 0;
	lcmd.BLINDS1.id2 = 0;
	lcmd.BLINDS1.id3 = NodeID;
	lcmd.BLINDS1.id4 = 0;
	lcmd.BLINDS1.unitcode = ChildID;
	lcmd.BLINDS1.cmnd = Command;
	lcmd.BLINDS1.filler = 0;
	lcmd.BLINDS1.rssi = RssiLevel;
	sDecodeRXMessage(this, (const unsigned char*)& lcmd.BLINDS1, defaultname.c_str(), BatteryLevel, userName.c_str());
}

void CDomoticzHardwareBase::SendRGBWSwitch(const int NodeID, const uint8_t ChildID, const int BatteryLevel, const int Level, const bool bIsRGBW, const std::string &defaultname,
					   const std::string &userName)
{
	int level = int(Level);
	uint8_t subType = (bIsRGBW == true) ? sTypeColor_RGB_W : sTypeColor_RGB;
	if (defaultname == "LIVCOL")
		subType = sTypeColor_LivCol;
	//Send as ColorSwitch
	_tColorSwitch lcmd;
	lcmd.id = NodeID;
	lcmd.subtype = subType;
	if (level == 0)
		lcmd.command = Color_LedOff;
	else
		lcmd.command = Color_LedOn;
	lcmd.dunit = ChildID;
	lcmd.value = (uint32_t)level;
	sDecodeRXMessage(this, (const unsigned char*)& lcmd, defaultname.c_str(), BatteryLevel, userName.c_str());
}

void CDomoticzHardwareBase::SendVoltageSensor(const int NodeID, const uint32_t ChildID, const int BatteryLevel, const float Volt, const std::string& defaultname)
{
	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeVoltage;
	gDevice.id = ChildID;
	gDevice.intval1 = (NodeID << 8) | ChildID;
	gDevice.floatval1 = Volt;
	sDecodeRXMessage(this, (const unsigned char *)&gDevice, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendCurrentSensor(const int NodeID, const int BatteryLevel, const float Current1, const float Current2, const float Current3, const std::string& defaultname, const int RssiLevel /* =12 */)
{
	tRBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.CURRENT.packetlength = sizeof(tsen.CURRENT) - 1;
	tsen.CURRENT.packettype = pTypeCURRENT;
	tsen.CURRENT.subtype = sTypeELEC1;
	tsen.CURRENT.rssi = RssiLevel;
	tsen.CURRENT.id1 = (NodeID & 0xFF00) >> 8;
	tsen.CURRENT.id2 = NodeID & 0xFF;
	tsen.CURRENT.battery_level = BatteryLevel;

	int at10 = ground(std::abs(Current1 * 10.0F));
	tsen.CURRENT.ch1h = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP.temperatureh * 256);
	tsen.CURRENT.ch1l = (BYTE)(at10);

	at10 = ground(std::abs(Current2 * 10.0F));
	tsen.CURRENT.ch2h = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP.temperatureh * 256);
	tsen.CURRENT.ch2l = (BYTE)(at10);

	at10 = ground(std::abs(Current3 * 10.0F));
	tsen.CURRENT.ch3h = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP.temperatureh * 256);
	tsen.CURRENT.ch3l = (BYTE)(at10);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.CURRENT, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendPercentageSensor(const int NodeID, const uint8_t ChildID, const int BatteryLevel, const float Percentage, const std::string& defaultname)
{
	_tGeneralDevice gDevice;
	gDevice.subtype = sTypePercentage;
	gDevice.id = ChildID;
	gDevice.intval1 = NodeID;
	gDevice.floatval1 = Percentage;
	sDecodeRXMessage(this, (const unsigned char *)&gDevice, defaultname.c_str(), BatteryLevel, nullptr);
}

bool CDomoticzHardwareBase::CheckPercentageSensorExists(const int NodeID, const int /*ChildID*/)
{
	std::string sTmp = std_format("%08X", NodeID);

	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d)",
		m_HwdID, sTmp.c_str(), int(pTypeGeneral), int(sTypePercentage));
	return (!result.empty());
}

void CDomoticzHardwareBase::SendWaterflowSensor(const int NodeID, const uint8_t ChildID, const int BatteryLevel, const float LPM, const std::string& defaultname)
{
	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeWaterflow;
	gDevice.id = ChildID;
	gDevice.intval1 = (NodeID << 8) | ChildID;
	gDevice.floatval1 = LPM;
	sDecodeRXMessage(this, (const unsigned char *)&gDevice, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendVisibilitySensor(const int NodeID, const int ChildID, const int BatteryLevel, const float Visibility, const std::string& defaultname)
{
	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeVisibility;
	gDevice.id = ChildID;
	gDevice.intval1 = (NodeID << 8) | ChildID;
	gDevice.floatval1 = Visibility;
	sDecodeRXMessage(this, (const unsigned char *)&gDevice, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendCustomSensor(const int NodeID, const uint8_t ChildID, const int BatteryLevel, const float CustomValue, const std::string& defaultname, const std::string& defaultLabel, const int RssiLevel /* =12 */)
{

	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeCustom;
	gDevice.id = ChildID;
	gDevice.battery_level = BatteryLevel;
	gDevice.rssi = RssiLevel;
	gDevice.intval1 = (NodeID << 8) | ChildID;
	gDevice.floatval1 = CustomValue;

	std::string sTmp = std_format("%08X", gDevice.intval1);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d)",
		m_HwdID, sTmp.c_str(), int(pTypeGeneral), int(sTypeCustom));
	bool bDoesExists = !result.empty();

	if (bDoesExists)
		sDecodeRXMessage(this, (const unsigned char *)&gDevice, defaultname.c_str(), BatteryLevel, nullptr);
	else
	{
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char*)& gDevice, defaultname.c_str(), BatteryLevel, nullptr);
		//Set the Label
		std::string soptions = "1;" + defaultLabel;
		m_sql.safe_query("UPDATE DeviceStatus SET Options='%q' WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d)",
			soptions.c_str(), m_HwdID, sTmp.c_str(), int(pTypeGeneral), int(sTypeCustom));
	}
}

//wind direction is in steps of 22.5 degrees (360/16)
void CDomoticzHardwareBase::SendWind(const int NodeID, const int BatteryLevel, const int WindDir, const float WindSpeed, const float WindGust, const float WindTemp, const float WindChill, const bool bHaveWindTemp, const bool bHaveWindChill, const std::string& defaultname, const int RssiLevel /* =12 */)
{
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.WIND.packetlength = sizeof(tsen.WIND) - 1;
	tsen.WIND.packettype = pTypeWIND;

	if ((!bHaveWindTemp) && (!bHaveWindChill))
		tsen.WIND.subtype = sTypeWINDNoTempNoChill;
	else if (!bHaveWindTemp)
		tsen.WIND.subtype = sTypeWINDNoTemp;
	else
		tsen.WIND.subtype = sTypeWIND4;

	tsen.WIND.battery_level = BatteryLevel;
	tsen.WIND.rssi = RssiLevel;
	tsen.WIND.id1 = (NodeID & 0xFF00) >> 8;
	tsen.WIND.id2 = NodeID & 0xFF;

	int aw = WindDir;
	tsen.WIND.directionh = (BYTE)(aw / 256);
	aw -= (tsen.WIND.directionh * 256);
	tsen.WIND.directionl = (BYTE)(aw);

	int sw = ground(WindSpeed * 10.0F);
	tsen.WIND.av_speedh = (BYTE)(sw / 256);
	sw -= (tsen.WIND.av_speedh * 256);
	tsen.WIND.av_speedl = (BYTE)(sw);

	int gw = ground(WindGust * 10.0F);
	tsen.WIND.gusth = (BYTE)(gw / 256);
	gw -= (tsen.WIND.gusth * 256);
	tsen.WIND.gustl = (BYTE)(gw);

	if (!bHaveWindTemp)
	{
		//No temp, only chill
		tsen.WIND.tempsign = (WindChill >= 0) ? 0 : 1;
		tsen.WIND.chillsign = (WindChill >= 0) ? 0 : 1;
		int at10 = ground(std::abs(WindChill * 10.0F));
		tsen.WIND.temperatureh = (BYTE)(at10 / 256);
		tsen.WIND.chillh = (BYTE)(at10 / 256);
		at10 -= (tsen.WIND.chillh * 256);
		tsen.WIND.temperaturel = (BYTE)(at10);
		tsen.WIND.chilll = (BYTE)(at10);
	}
	else
	{
		//temp+chill
		tsen.WIND.tempsign = (WindTemp >= 0) ? 0 : 1;
		int at10 = ground(std::abs(WindTemp * 10.0F));
		tsen.WIND.temperatureh = (BYTE)(at10 / 256);
		at10 -= (tsen.WIND.temperatureh * 256);
		tsen.WIND.temperaturel = (BYTE)(at10);

		tsen.WIND.chillsign = (WindChill >= 0) ? 0 : 1;
		at10 = ground(std::abs(WindChill * 10.0F));
		tsen.WIND.chillh = (BYTE)(at10 / 256);
		at10 -= (tsen.WIND.chillh * 256);
		tsen.WIND.chilll = (BYTE)(at10);
	}

	sDecodeRXMessage(this, (const unsigned char *)&tsen.WIND, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendPressureSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float pressure, const std::string& defaultname)
{
	_tGeneralDevice gdevice;
	gdevice.subtype = sTypePressure;
	gdevice.intval1 = (NodeID << 8) | ChildID;
	gdevice.floatval1 = pressure;
	sDecodeRXMessage(this, (const unsigned char *)&gdevice, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendSolarRadiationSensor(const unsigned char NodeID, const int BatteryLevel, const float radiation, const std::string& defaultname)
{
	if (radiation > 1361)
		return; //https://en.wikipedia.org/wiki/Solar_irradiance

	_tGeneralDevice gdevice;
	gdevice.subtype = sTypeSolarRadiation;
	gdevice.id = NodeID;
	gdevice.floatval1 = radiation;
	sDecodeRXMessage(this, (const unsigned char *)&gdevice, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendSoundSensor(const int NodeID, const int BatteryLevel, const int sLevel, const std::string& defaultname)
{
	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeSoundLevel;
	gDevice.id = 1;
	gDevice.intval1 = NodeID;
	gDevice.intval2 = sLevel;
	sDecodeRXMessage(this, (const unsigned char *)&gDevice, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendAlertSensor(const int NodeID, const int BatteryLevel, const int alertLevel, const std::string& message, const std::string& defaultname)
{
	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeAlert;
	gDevice.id = (unsigned char)NodeID;
	gDevice.intval1 = alertLevel;
	sprintf(gDevice.text, "%.63s", message.c_str());
	sDecodeRXMessage(this, (const unsigned char *)&gDevice, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendGeneralSwitch(const int NodeID, const int ChildID, const int BatteryLevel, const uint8_t SwitchState, const uint8_t Level, const std::string &defaultname,
					      const std::string &userName, const int RssiLevel)
{
	_tGeneralSwitch gSwitch;
	gSwitch.id = NodeID;
	gSwitch.unitcode = ChildID;
	gSwitch.cmnd = SwitchState;
	gSwitch.level = Level;
	gSwitch.rssi = (uint8_t)RssiLevel;
	sDecodeRXMessage(this, (const unsigned char*)& gSwitch, defaultname.c_str(), BatteryLevel, userName.c_str());
}

void CDomoticzHardwareBase::SendMoistureSensor(const int NodeID, const int BatteryLevel, const int mLevel, const std::string& defaultname, const int RssiLevel /* =12 */)
{
	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeSoilMoisture;
	gDevice.id = 1;
	gDevice.intval1 = NodeID;
	gDevice.intval2 = mLevel;
	gDevice.rssi = RssiLevel;
	sDecodeRXMessage(this, (const unsigned char *)&gDevice, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendUVSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float UVI, const std::string& defaultname, const int RssiLevel /* =12 */)
{
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.UV.packetlength = sizeof(tsen.UV) - 1;
	tsen.UV.packettype = pTypeUV;
	tsen.UV.subtype = sTypeUV1;
	tsen.UV.battery_level = BatteryLevel;
	tsen.UV.rssi = RssiLevel;
	tsen.UV.id1 = (unsigned char)NodeID;
	tsen.UV.id2 = (unsigned char)ChildID;

	tsen.UV.uv = (BYTE)ground(UVI * 10);
	sDecodeRXMessage(this, (const unsigned char *)&tsen.UV, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendFanSensor(const int Idx, const int BatteryLevel, const int FanSpeed, const std::string& defaultname)
{
	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeFan;
	gDevice.id = 1;
	gDevice.intval1 = Idx;
	gDevice.intval2 = FanSpeed;
	sDecodeRXMessage(this, (const unsigned char *)&gDevice, defaultname.c_str(), BatteryLevel, nullptr);
}

void CDomoticzHardwareBase::SendSecurity1Sensor(const int NodeID, const int DeviceSubType, const int BatteryLevel, const int Status, const std::string &defaultname, const std::string &userName,
						const int RssiLevel /* = 12 */)
{
	RBUF m_sec1;
	memset(&m_sec1, 0, sizeof(RBUF));

	m_sec1.SECURITY1.packetlength = sizeof(m_sec1) -1;
	m_sec1.SECURITY1.packettype = pTypeSecurity1;
	m_sec1.SECURITY1.subtype = DeviceSubType;
	m_sec1.SECURITY1.id1 = (NodeID & 0xFF0000) >> 16;
	m_sec1.SECURITY1.id2 = (NodeID & 0xFF00) >> 8;
	m_sec1.SECURITY1.id3 = (NodeID & 0xFF);
	m_sec1.SECURITY1.status = Status;
	m_sec1.SECURITY1.rssi = RssiLevel;
	m_sec1.SECURITY1.battery_level = BatteryLevel;

	sDecodeRXMessage(this, (const unsigned char*)& m_sec1.SECURITY1, defaultname.c_str(), BatteryLevel, userName.c_str());
}
/**
 * SendSelectorSwitch
 * 
 * Helper function to create/update selector switch, validates sValue exist in the list of actions
 *  
 * @param  {int} NodeID               : As normal
 * @param  {uint8_t} ChildID          : As normal
 * @param  {int} sValue               : Current Level (Level a 0 / 10 / 20 / 30 etc... 0 = Off, 10 = Level 1, 20 = Level 2 etc...)
 * @param  {std::string} defaultname  : As normal
 * @param  {int} customImage          : Custom image ID to use for the selector
 * @param  {bool} bDropdown           : boolean: true will show a drop down, false will show a row of buttons
 * @param  {std::string} LevelNames   : String with the labels to show for each level, seperated with |. Example: "Off|Level 1|Level 2|Level 3"
 * @param  {std::string} LevelActions : String with action to be carried for each level (http:// https:// script://), separated with |, can be an empty string. Example "http://www.dosomething.com|http://www.doanotherthing.com"
 * @param  {bool} bHideOff            : Boolean: true will hide the off level, false will enable it.
  */
void CDomoticzHardwareBase::SendSelectorSwitch(const int NodeID, const uint8_t ChildID, const std::string &sValue, const std::string &defaultname, const int customImage, const bool bDropdown,
					       const std::string &LevelNames, const std::string &LevelActions, const bool bHideOff, const std::string &userName)
{
	/*if (std::size_t index = LevelActions.find(sValue) == std::string::npos)
	{ 
	   Log(LOG_ERROR,"Value %s not supported by Selector Switch %s, it needs %s",sValue.c_str() , defaultname.c_str(), LevelActions.c_str() ); 
	   return; // did not find sValue in LevelAction string so exit with warning
	}*/

	_tGeneralSwitch xcmd;
	xcmd.len = sizeof(_tGeneralSwitch) - 1;
	xcmd.type = pTypeGeneralSwitch;
	xcmd.subtype = sSwitchTypeSelector;
	xcmd.id = NodeID;
	xcmd.unitcode = ChildID; //Do we support multiple copies of the same selector switch ??
	xcmd.level = std::stoi(sValue);

	_eSwitchType switchtype;
	switchtype = STYPE_Selector;

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID, sValue, Options FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%08X') AND (Unit == '%d')", m_HwdID, NodeID, xcmd.unitcode);
	bool bDoesExists = !result.empty();
    
	m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&xcmd,  defaultname.c_str(), 255, userName.c_str());  // will create the base switch if not exist

	if (!bDoesExists)//Switch is new so update it with all relevant info
	{
		std::stringstream build_str; //building up selector option string
		build_str << "SelectorStyle:";
		if (bDropdown)
			build_str << "1";
		else
			build_str << "0";
		build_str << ";LevelNames:" << LevelNames.c_str() << ";LevelOffHidden:";
		if (bHideOff)
			build_str <<  "true";
		else
			build_str << "false";
		build_str << ";LevelActions:" << LevelActions.c_str();
		std::string options_str = m_sql.FormatDeviceOptions(m_sql.BuildDeviceOptions( build_str.str(), false));
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', sValue=%i, SwitchType=%d, CustomImage=%i,options='%q' WHERE (HardwareID == %d) AND (DeviceID=='%08X') AND (Unit == '%d')", defaultname.c_str(), xcmd.level, (switchtype), customImage, options_str.c_str(), m_HwdID, NodeID, xcmd.unitcode);
        // The Selector switch has been created
	}
	else
	{ 
		//Check Level (sValue in SQL Query)
		if (xcmd.level == std::stoi(result[0][1]))
			return; // no need to uodate
		result = m_sql.safe_query("UPDATE DeviceStatus SET sValue=%i WHERE (HardwareID==%d) AND (DeviceID=='%08X')", xcmd.level, m_HwdID, NodeID);
	}
}
                            
/**
 * MigrateSelectorSwitch
 *
 * Helper function to migrate selector switch,  validates if existing LevelActions match existing in the DB, if not migrate the switch based on bMigrate setting
 *   
 * @param  {int} NodeID               : As Usual
 * @param  {uint8_t} ChildID          : As Usual
 * @param  {std::string} LevelNames   : Will be updated together with LevelNames
 * @param  {std::string} LevelActions : Checks if the selector switch in the db is in synch with these value, then trie4s to migrat if BMigrate is true
 * @param  {bool} bMigrate            : true if  the selector may be upgraded if incorrect, false if you only want an  error if migration is needed
 * @return {int}                      : 0 No need for migration /not yet exist| 1 Migration completed| -1 Need Migration
 */
int CDomoticzHardwareBase::MigrateSelectorSwitch(const int NodeID, const uint8_t ChildID,  const std::string& LevelNames, const std::string& LevelActions, const bool bMigrate )
{
	std::string options;
	std::vector<std::vector<std::string> > result;
	bool bUpdated = false;
		
	result = m_sql.safe_query("SELECT Options FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%08X') AND (Unit == '%d')", m_HwdID, NodeID, ChildID);
	if (result.empty())
	    return 0;  // switch does not exist yet
	std::map<std::string, std::string> optionsMap;
	optionsMap = m_sql.BuildDeviceOptions(result[0][0]);
	int count = static_cast<int>(optionsMap.size());
	if (count > 0) 
	{
		int i = 0;
		std::stringstream ssoptions;
		for (const auto &option : optionsMap)
		{
			std::string optionName = option.first;
			std::string optionValue = option.second;
			if (strcmp(option.first.c_str(), "LevelActions") == 0)
			{
				if (strcmp(option.second.c_str(), LevelActions.c_str()) != 0)
				{
					bUpdated = true;  // the list of actions is not what we expected. flag that Migration is required
					optionValue = LevelActions;
				}
			}
			else if (strcmp(option.first.c_str(), "LevelNames") == 0)
			{
				optionValue = LevelNames;
			}
			ssoptions << optionName << ":" << optionValue;
			if (i < count) {
				ssoptions << ";";
			}
			options = ssoptions.str();
		}
	}
    if( bUpdated ) // the options map has been migrated do we migrate to warn? 
	{
		if(!bMigrate)
			return -1;  // Signnal  selector switch is not latest version
		std::string options_str = m_sql.FormatDeviceOptions(m_sql.BuildDeviceOptions(options, false));
		m_sql.safe_query("UPDATE DeviceStatus SET options='%q' WHERE (HardwareID==%d) AND (DeviceID=='%08X')", options_str.c_str(), m_HwdID, NodeID);
	   return 1; // signal migration completed
	}
    return 0;	// signal no need for migration
}

/**
 * CreateBlindSwitch
 *
 * Helper function to create/update blind control switch & associated widget
 *
 * @param  {int} NodeID              : As normal, device ID
 * @param  {uint8_t} ChildID         : As normal, device unit code
 * @param  {_eSwitchType} switchtype : Blind switch type (STYPE_Blinds, STYPE_BlindsPercentage, STYPE_VenetianBlindsUS, STYPE_VenetianBlindsEU or STYPE_BlindsPercentageWithStop)
 * @param  {bool} bDeviceUsed        : true : device appeard on switches screen
 * @param  {bool} bReversePosition   : true : reverse slider position
 * @param  {bool} bReverseState      : true : reverse Open/Closed state
 * @param  {uint8_t} cmnd            : general switch command (gswitch_sOpen, gswitch_sClose, gswitch_sLevel or gswitch_sStop)
 * @param  {uint8_t} level           : general switch level 0..100
 * @param  {std::string} defaultName : As normal, device default name, updated if empty or unknown
 * @param  {std::string} userName    : As normal, name of the hardware using the device
 * @param  {int32_t} batteryLevel    : As normal, 0 (mini) .. 100 (maxi), 255 (not available), -1 (don't set)
 * @param  {uint8_t} rssiLevel       : As normal, 0 (mini) .. 11 (maxi), 12 (not available)
 */
void CDomoticzHardwareBase::CreateBlindSwitch(int NodeID, uint8_t ChildID, _eSwitchType switchtype, bool bDeviceUsed, bool bReversePosition, bool bReverseState, uint8_t cmnd, uint8_t level, const std::string &defaultName, const std::string &userName, int32_t batteryLevel, uint8_t rssiLevel)
{
	if (switchtype != STYPE_Blinds && switchtype != STYPE_BlindsPercentage && switchtype != STYPE_VenetianBlindsUS && switchtype != STYPE_VenetianBlindsEU && switchtype != STYPE_BlindsPercentageWithStop)
	{
	   Log(LOG_ERROR, "Node %08X (%s), invalid switch type %u", NodeID, defaultName.c_str(), uint32_t(switchtype));
	   return;
	}
	// Create (or update) blind control switch

	_tGeneralSwitch xcmd;

	xcmd.id = NodeID;
	xcmd.type = pTypeGeneralSwitch;
	xcmd.subtype = sSwitchGeneralSwitch;
	xcmd.unitcode = ChildID;
	xcmd.cmnd = cmnd;
	xcmd.level = level;
	if (batteryLevel != -1)
	   xcmd.battery_level = uint8_t(batteryLevel);
	xcmd.rssi = rssiLevel;

	m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&xcmd, defaultName.c_str(), batteryLevel, userName.c_str()); // will create the base switch if not exist

	// Set (or reset) blind control switch widget options

	std::stringstream build_str;
	build_str << "ReversePosition:" << (bReversePosition ? "true" : "false");
	build_str << ";ReverseState:" << (bReverseState ? "true" : "false");

	std::string options_str = m_sql.FormatDeviceOptions(m_sql.BuildDeviceOptions(build_str.str(), false));

	m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', SwitchType=%d, Used=%d, Options='%q' WHERE (HardwareID == %d) AND (DeviceID=='%08X') AND (Unit == '%d')", defaultName.c_str(), switchtype, (bDeviceUsed ? 1 : 0), options_str.c_str(), m_HwdID, NodeID, ChildID);
}

/**
 * SendBlindSwitch
 *
 * Helper function to create/update blind control switch
 *
 * @param  {int} NodeID              : As normal, device ID
 * @param  {uint8_t} ChildID         : As normal, device unit code
 * @param  {uint8_t} cmnd            : general switch command (gswitch_sOpen, gswitch_sClose, gswitch_sLevel or gswitch_sStop)
 * @param  {uint8_t} level           : general switch level 0..100
 * @param  {std::string} defaultName : As normal, device default name, updated if empty or unknown
 * @param  {std::string} userName    : As normal, name of the hardware using the device
 * @param  {int32_t} batteryLevel    : As normal, 0 (mini) .. 100 (maxi), 255 (not available), -1 (don't set)
 * @param  {uint8_t} rssiLevel       : As normal, 0 (mini) .. 11 (maxi), 12 (not available)
 */
void CDomoticzHardwareBase::SendBlindSwitch(int NodeID, uint8_t ChildID, uint8_t cmnd, uint8_t level, const std::string &defaultName, const std::string &userName, int32_t batteryLevel, uint8_t rssiLevel)
{
	_tGeneralSwitch xcmd;

	xcmd.type = pTypeGeneralSwitch;
	xcmd.subtype = sSwitchGeneralSwitch;
	xcmd.id = NodeID;
	xcmd.unitcode = ChildID;
	xcmd.cmnd = cmnd;
	xcmd.level = level;
	if (batteryLevel != -1)
	   xcmd.battery_level = uint8_t(batteryLevel);
	xcmd.rssi = rssiLevel;

	sDecodeRXMessage(this, (const unsigned char *)&xcmd, defaultName.c_str(), batteryLevel, userName.c_str());
}

#ifdef WITH_OPENZWAVE
void CDomoticzHardwareBase::SendZWaveAlarmSensor(const int NodeID, const uint8_t InstanceID, const int BatteryLevel, const uint8_t aType, const int aValue, const std::string& alarmLabel, const std::string& defaultname)
{
	uint8_t ID1 = 0;
	uint8_t ID2 = (unsigned char)((NodeID & 0xFF00) >> 8);
	uint8_t ID3 = (unsigned char)NodeID & 0xFF;
	uint8_t ID4 = InstanceID;

	unsigned long lID = (ID1 << 24) + (ID2 << 16) + (ID3 << 8) + ID4;

	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeZWaveAlarm;
	gDevice.id = aType;
	gDevice.intval1 = (int)(lID);
	gDevice.intval2 = aValue;

	size_t maxChars = (alarmLabel.size() < sizeof(_tGeneralDevice::text) - 1) ? alarmLabel.size() : sizeof(_tGeneralDevice::text) - 1;
	strncpy(gDevice.text, alarmLabel.c_str(), maxChars);
	gDevice.text[maxChars] = 0;

	sDecodeRXMessage(this, (const unsigned char*)&gDevice, defaultname.c_str(), BatteryLevel, nullptr);
}
#endif
