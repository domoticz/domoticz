#include "stdafx.h"
#include <iostream>
#include "DomoticzHardware.h"
#include "../main/Logger.h"
#include "../main/localtime_r.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"
#include "hardwaretypes.h"

#define round(a) ( int ) ( a + .5 )

CDomoticzHardwareBase::CDomoticzHardwareBase()
{
	m_HwdID=0; //should be uniquely assigned
	m_bEnableReceive=false;
	m_rxbufferpos=0;
	m_SeqNr=0;
	m_pUserData=NULL;
	m_bIsStarted=false;
	m_stopHeartbeatrequested = false;
	mytime(&m_LastHeartbeat);
	mytime(&m_LastHeartbeatReceive);
	m_DataTimeout = 0;
	m_bSkipReceiveCheck = false;
	m_bOutputLog = true;
	m_iHBCounter = 0;

	m_baro_minuteCount = 0;
	m_last_forecast = wsbaroforcast_unknown;
	mytime(&m_BaroCalcLastTime);
};

CDomoticzHardwareBase::~CDomoticzHardwareBase()
{
}

bool CDomoticzHardwareBase::Start()
{
	m_iHBCounter = 0;
	return StartHardware();
}

bool CDomoticzHardwareBase::Stop()
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	return StopHardware();
}

void CDomoticzHardwareBase::EnableOutputLog(const bool bEnableLog)
{
	m_bOutputLog = bEnableLog;
}

bool CDomoticzHardwareBase::onRFXMessage(const unsigned char *pBuffer, const size_t Len)
{
	if (!m_bEnableReceive)
		return true; //receiving not enabled

	size_t ii=0;
	while (ii<Len)
	{
		if (m_rxbufferpos == 0)	//1st char of a packet received
		{
			if (pBuffer[ii]==0) //ignore first char if 00
				return true;
		}
		m_rxbuffer[m_rxbufferpos]=pBuffer[ii];
		m_rxbufferpos++;
		if (m_rxbufferpos>=sizeof(m_rxbuffer))
		{
			//something is out of sync here!!
			//restart
			_log.Log(LOG_ERROR,"input buffer out of sync, going to restart!....");
			m_rxbufferpos=0;
			return false;
		}
		if (m_rxbufferpos > m_rxbuffer[0])
		{
			sDecodeRXMessage(this, (const unsigned char *)&m_rxbuffer, NULL, -1);
			m_rxbufferpos = 0;    //set to zero to receive next message
		}
		ii++;
	}
	return true;
}

void CDomoticzHardwareBase::StartHeartbeatThread()
{
	m_stopHeartbeatrequested = false;
	m_Heartbeatthread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CDomoticzHardwareBase::Do_Heartbeat_Work, this)));
}

void CDomoticzHardwareBase::StopHeartbeatThread()
{
	m_stopHeartbeatrequested = true;
	if (m_Heartbeatthread)
	{
		m_Heartbeatthread->join();
		// Wait a while. The read thread might be reading. Adding this prevents a pointer error in the async serial class.
		sleep_milliseconds(10);
	}
}

void CDomoticzHardwareBase::Do_Heartbeat_Work()
{
	int secCounter = 0;
	int hbCounter = 0;
	while (!m_stopHeartbeatrequested)
	{
		sleep_milliseconds(200);
		if (m_stopHeartbeatrequested)
			break;
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

void CDomoticzHardwareBase::HandleHBCounter(const int iInterval)
{
	m_iHBCounter++;
	if (m_iHBCounter % iInterval == 0)
	{
		SetHeartbeatReceived();
	}
}

//Sensor Helpers
void CDomoticzHardwareBase::SendTempSensor(const int NodeID, const int BatteryLevel, const float temperature, const std::string &defaultname, const int RssiLevel /* =12 */)
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
	int at10 = round(std::abs(temperature*10.0f));
	tsen.TEMP.temperatureh = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP.temperatureh * 256);
	tsen.TEMP.temperaturel = (BYTE)(at10);
	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendHumiditySensor(const int NodeID, const int BatteryLevel, const int humidity, const std::string &defaultname, const int RssiLevel /* =12 */)
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
	sDecodeRXMessage(this, (const unsigned char *)&tsen.HUM, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendBaroSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float pressure, const int forecast, const std::string &defaultname)
{
	_tGeneralDevice gdevice;
	gdevice.subtype = sTypeBaro;
	gdevice.intval1 = (NodeID << 8) | ChildID;
	gdevice.intval2 = forecast;
	gdevice.floatval1 = pressure;
	sDecodeRXMessage(this, (const unsigned char *)&gdevice, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendTempHumSensor(const int NodeID, const int BatteryLevel, const float temperature, const int humidity, const std::string &defaultname, const int RssiLevel /* =12 */)
{
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.TEMP_HUM.packetlength = sizeof(tsen.TEMP_HUM) - 1;
	tsen.TEMP_HUM.packettype = pTypeTEMP_HUM;
	tsen.TEMP_HUM.subtype = sTypeTH5;
	tsen.TEMP_HUM.battery_level = BatteryLevel;
	tsen.TEMP_HUM.rssi = RssiLevel;
	tsen.TEMP_HUM.id1 = (NodeID&0xFF00)>>8;
	tsen.TEMP_HUM.id2 = NodeID & 0xFF;

	tsen.TEMP_HUM.tempsign = (temperature >= 0) ? 0 : 1;
	int at10 = round(std::abs(temperature*10.0f));
	tsen.TEMP_HUM.temperatureh = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP_HUM.temperatureh * 256);
	tsen.TEMP_HUM.temperaturel = (BYTE)(at10);
	tsen.TEMP_HUM.humidity = (BYTE)humidity;
	tsen.TEMP_HUM.humidity_status = Get_Humidity_Level(tsen.TEMP_HUM.humidity);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP_HUM, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendTempHumBaroSensor(const int NodeID, const int BatteryLevel, const float temperature, const int humidity, const float pressure, int forecast, const std::string &defaultname, const int RssiLevel /* =12 */)
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
	int at10 = round(std::abs(temperature*10.0f));
	tsen.TEMP_HUM_BARO.temperatureh = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP_HUM_BARO.temperatureh * 256);
	tsen.TEMP_HUM_BARO.temperaturel = (BYTE)(at10);
	tsen.TEMP_HUM_BARO.humidity = (BYTE)humidity;
	tsen.TEMP_HUM_BARO.humidity_status = Get_Humidity_Level(tsen.TEMP_HUM.humidity);

	int ab10 = round(pressure);
	tsen.TEMP_HUM_BARO.baroh = (BYTE)(ab10 / 256);
	ab10 -= (tsen.TEMP_HUM_BARO.baroh * 256);
	tsen.TEMP_HUM_BARO.barol = (BYTE)(ab10);

	tsen.TEMP_HUM_BARO.forecast = (BYTE)forecast;

	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP_HUM_BARO, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendTempHumBaroSensorFloat(const int NodeID, const int BatteryLevel, const float temperature, const int humidity, const float pressure, int forecast, const std::string &defaultname, const int RssiLevel /* =12 */)
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
	int at10 = round(std::abs(temperature*10.0f));
	tsen.TEMP_HUM_BARO.temperatureh = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP_HUM_BARO.temperatureh * 256);
	tsen.TEMP_HUM_BARO.temperaturel = (BYTE)(at10);
	tsen.TEMP_HUM_BARO.humidity = (BYTE)humidity;
	tsen.TEMP_HUM_BARO.humidity_status = Get_Humidity_Level(tsen.TEMP_HUM.humidity);

	int ab10 = round(pressure*10.0f);
	tsen.TEMP_HUM_BARO.baroh = (BYTE)(ab10 / 256);
	ab10 -= (tsen.TEMP_HUM_BARO.baroh * 256);
	tsen.TEMP_HUM_BARO.barol = (BYTE)(ab10);
	tsen.TEMP_HUM_BARO.forecast = forecast;

	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP_HUM_BARO, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendTempBaroSensor(const int NodeID, const int BatteryLevel, const float temperature, const float pressure, const std::string &defaultname)
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
	sDecodeRXMessage(this, (const unsigned char *)&tsensor, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendSetPointSensor(const int NodeID, const int ChildID, const unsigned char SensorID, const float Temp, const std::string &defaultname)
{
	_tThermostat thermos;
	thermos.subtype = sTypeThermSetpoint;
	thermos.id1 = 0;
	thermos.id2 = NodeID;
	thermos.id3 = ChildID;
	thermos.id4 = SensorID;
	thermos.dunit = 1;

	thermos.temp = Temp;

	sDecodeRXMessage(this, (const unsigned char *)&thermos, defaultname.c_str(), -1);
}


void CDomoticzHardwareBase::SendDistanceSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float distance, const std::string &defaultname)
{
	_tGeneralDevice gdevice;
	gdevice.subtype = sTypeDistance;
	gdevice.intval1 = (NodeID << 8) | ChildID;
	gdevice.floatval1 = distance;
	sDecodeRXMessage(this, (const unsigned char *)&gdevice, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendTextSensor(const int NodeID, const int ChildID, const int BatteryLevel, const std::string &textMessage, const std::string &defaultname)
{
	_tGeneralDevice gdevice;
	gdevice.subtype = sTypeTextStatus;
	gdevice.intval1 = (NodeID << 8) | ChildID;
	std::string sstatus = textMessage;
	if (sstatus.size() > 63)
		sstatus = sstatus.substr(0, 63);
	strcpy(gdevice.text, sstatus.c_str());
	sDecodeRXMessage(this, (const unsigned char *)&gdevice, defaultname.c_str(), BatteryLevel);
}

std::string CDomoticzHardwareBase::GetTextSensorText(const int NodeID, const int ChildID, bool &bExists)
{
	bExists = false;
	std::string ret = "";

	std::vector<std::vector<std::string> > result;

	char szTmp[30];
	sprintf(szTmp, "%08X", (NodeID << 8) | ChildID);

	result = m_sql.safe_query("SELECT sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d)",
		m_HwdID, szTmp, int(pTypeGeneral), int(sTypeTextStatus));
	if (!result.empty())
	{
		bExists = true;
		ret = result[0][0];
	}
	return ret;
}

void CDomoticzHardwareBase::SendRainSensor(const int NodeID, const int BatteryLevel, const float RainCounter, const std::string &defaultname, const int RssiLevel /* =12 */)
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

	uint64_t tr10 = int((float(RainCounter)*10.0f));

	tsen.RAIN.raintotal1 = (BYTE)(tr10 / 65535);
	tr10 -= (tsen.RAIN.raintotal1 * 65535);
	tsen.RAIN.raintotal2 = (BYTE)(tr10 / 256);
	tr10 -= (tsen.RAIN.raintotal2 * 256);
	tsen.RAIN.raintotal3 = (BYTE)(tr10);
	sDecodeRXMessage(this, (const unsigned char *)&tsen.RAIN, defaultname.c_str(), BatteryLevel);
}

float CDomoticzHardwareBase::GetRainSensorValue(const int NodeID, bool &bExists)
{
	char szIdx[10];
	sprintf(szIdx, "%d", NodeID & 0xFFFF);
	int Unit = 0;

	std::vector<std::vector<std::string> > results;
	results = m_sql.safe_query("SELECT ID,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)", m_HwdID, szIdx, Unit, int(pTypeRAIN), int(sTypeRAIN3));
	if (results.size() < 1)
	{
		bExists = false;
		return 0.0f;
	}
	std::vector<std::string> splitresults;
	StringSplit(results[0][1], ";", splitresults);
	if (splitresults.size() != 2)
	{
		bExists = false;
		return 0.0f;
	}
	bExists = true;
	return (float)atof(splitresults[1].c_str());
}

bool CDomoticzHardwareBase::GetWindSensorValue(const int NodeID, int &WindDir, float &WindSpeed, float &WindGust, float &WindTemp, float &WindChill, bool bHaveWindTemp,  bool &bExists)
{
	char szIdx[10];
	sprintf(szIdx, "%d", NodeID & 0xFFFF);
	int Unit = 0;

	std::vector<std::vector<std::string> > results;
	if (!bHaveWindTemp)
		results = m_sql.safe_query("SELECT ID,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)", m_HwdID, szIdx, Unit, int(pTypeWIND), int(sTypeWINDNoTemp));
	else
		results = m_sql.safe_query("SELECT ID,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)", m_HwdID, szIdx, Unit, int(pTypeWIND), int(sTypeWIND4));
	if (results.size() < 1)
	{
		bExists = false;
		return 0.0f;
	}
	std::vector<std::string> splitresults;
	StringSplit(results[0][1], ";", splitresults);

	if (splitresults.size() != 6)
	{
		bExists = false;
		return 0.0f;
	}
	bExists = true;
	WindDir = (int)atof(splitresults[0].c_str());
	WindSpeed = (float)atof(splitresults[2].c_str());
	WindGust = (float)atof(splitresults[3].c_str());
	WindTemp = (float)atof(splitresults[4].c_str());
	WindChill = (float)atof(splitresults[5].c_str());
	return bExists;
}

void CDomoticzHardwareBase::SendWattMeter(const int NodeID, const int ChildID, const int BatteryLevel, const float musage, const std::string &defaultname)
{
	_tUsageMeter umeter;
	umeter.id1 = 0;
	umeter.id2 = 0;
	umeter.id3 = 0;
	umeter.id4 = NodeID;
	umeter.dunit = ChildID;
	umeter.fusage = musage;
	sDecodeRXMessage(this, (const unsigned char *)&umeter, defaultname.c_str(), BatteryLevel);
}

//Obsolete, we should not call this anymore
//when all calls are removed, we should delete this function
void CDomoticzHardwareBase::SendKwhMeterOldWay(const int NodeID, const int ChildID, const int BatteryLevel, const double musage, const double mtotal, const std::string &defaultname)
{
	SendKwhMeter(NodeID, ChildID, BatteryLevel, musage * 1000, mtotal, defaultname);
}

void CDomoticzHardwareBase::SendKwhMeter(const int NodeID, const int ChildID, const int BatteryLevel, const double musage, const double mtotal, const std::string &defaultname)
{
	_tGeneralDevice gdevice;
	gdevice.subtype = sTypeKwh;
	gdevice.intval1 = (NodeID << 8) | ChildID;
	gdevice.floatval1 = (float)musage;
	gdevice.floatval2 = (float)(mtotal*1000.0);
	sDecodeRXMessage(this, (const unsigned char *)&gdevice, defaultname.c_str(), BatteryLevel);
}

double CDomoticzHardwareBase::GetKwhMeter(const int NodeID, const int ChildID, bool &bExists)
{
	int dID = (NodeID << 8) | ChildID;
	char szTmp[30];
	sprintf(szTmp, "%08X", dID);

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d)",
		m_HwdID, szTmp, int(pTypeGeneral), int(sTypeKwh));
	if (result.size() < 1)
	{
		bExists = false;
		return 0;
	}
	result = m_sql.safe_query("SELECT MAX(Counter) FROM Meter_Calendar WHERE (DeviceRowID=='%q')", result[0][0].c_str());
	if (result.size() < 1)
	{
		bExists = false;
		return 0.0f;
	}
	bExists = true;
	return (float)atof(result[0][0].c_str());
}

void CDomoticzHardwareBase::SendMeterSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float metervalue, const std::string &defaultname, const int RssiLevel /* =12 */)
{
	unsigned long counter = (unsigned long)(metervalue*1000.0f);
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
	sDecodeRXMessage(this, (const unsigned char *)&tsen.RFXMETER, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendLuxSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float Lux, const std::string &defaultname)
{
	_tLightMeter lmeter;
	lmeter.id1 = 0;
	lmeter.id2 = 0;
	lmeter.id3 = 0;
	lmeter.id4 = NodeID;
	lmeter.dunit = ChildID;
	lmeter.fLux = Lux;
	lmeter.battery_level = BatteryLevel;
	sDecodeRXMessage(this, (const unsigned char *)&lmeter, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendAirQualitySensor(const int NodeID, const int ChildID, const int BatteryLevel, const int AirQuality, const std::string &defaultname)
{
	_tAirQualityMeter meter;
	meter.len = sizeof(_tAirQualityMeter) - 1;
	meter.type = pTypeAirQuality;
	meter.subtype = sTypeVoltcraft;
	meter.airquality = AirQuality;
	meter.id1 = NodeID;
	meter.id2 = ChildID;
	sDecodeRXMessage(this, (const unsigned char *)&meter, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendUsageSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float Usage, const std::string &defaultname)
{
	_tUsageMeter umeter;
	umeter.id1 = 0;
	umeter.id2 = 0;
	umeter.id3 = 0;
	umeter.id4 = NodeID;
	umeter.dunit = ChildID;
	umeter.fusage = Usage;
	sDecodeRXMessage(this, (const unsigned char *)&umeter, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendSwitchIfNotExists(const int NodeID, const int ChildID, const int BatteryLevel, const bool bOn, const double Level, const std::string &defaultname)
{
	//make device ID
	unsigned char ID1 = (unsigned char)((NodeID & 0xFF000000) >> 24);
	unsigned char ID2 = (unsigned char)((NodeID & 0xFF0000) >> 16);
	unsigned char ID3 = (unsigned char)((NodeID & 0xFF00) >> 8);
	unsigned char ID4 = (unsigned char)NodeID & 0xFF;

	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", ID1, ID2, ID3, ID4);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)",
		m_HwdID, szIdx, ChildID, int(pTypeLighting2), int(sTypeAC));
	if (result.size() < 1)
	{
		SendSwitch(NodeID, ChildID, BatteryLevel, bOn, Level, defaultname);
	}
}

void CDomoticzHardwareBase::SendSwitch(const int NodeID, const int ChildID, const int BatteryLevel, const bool bOn, const double Level, const std::string &defaultname, const int RssiLevel /* =12 */)
{
	double rlevel = (16.0 / 100.0)*Level;
	int level = int(rlevel);

	//make device ID
	unsigned char ID1 = (unsigned char)((NodeID & 0xFF000000) >> 24);
	unsigned char ID2 = (unsigned char)((NodeID & 0xFF0000) >> 16);
	unsigned char ID3 = (unsigned char)((NodeID & 0xFF00) >> 8);
	unsigned char ID4 = (unsigned char)NodeID & 0xFF;

	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", ID1, ID2, ID3, ID4);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)",
		m_HwdID, szIdx, ChildID, int(pTypeLighting2), int(sTypeAC));
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
		if (level!=0)
			lcmd.LIGHTING2.cmnd = light2_sSetLevel;
		else
			lcmd.LIGHTING2.cmnd = light2_sOn;
	}
	lcmd.LIGHTING2.level = level;
	lcmd.LIGHTING2.filler = 0;
	lcmd.LIGHTING2.rssi = RssiLevel;
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendBlindSensor(const int NodeID, const int ChildID, const int BatteryLevel, const int Command, const std::string &defaultname, const int RssiLevel /* =12 */)
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
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.BLINDS1, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendRGBWSwitch(const int NodeID, const int ChildID, const int BatteryLevel, const double Level, const bool bIsRGBW, const std::string &defaultname)
{
	int level = int(Level);
	int subType = (bIsRGBW == true) ? sTypeLimitlessRGBW : sTypeLimitlessRGB;
	//Send as LimitlessLight
	_tLimitlessLights lcmd;
	lcmd.id = NodeID;
	lcmd.subtype = subType;
	if (level == 0)
		lcmd.command = Limitless_LedOff;
	else
		lcmd.command = Limitless_LedOn;
	lcmd.dunit = ChildID;
	lcmd.value = level;
	sDecodeRXMessage(this, (const unsigned char *)&lcmd, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendVoltageSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float Volt, const std::string &defaultname)
{
	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeVoltage;
	gDevice.id = ChildID;
	gDevice.intval1 = (NodeID << 8) | ChildID;
	gDevice.floatval1 = Volt;
	sDecodeRXMessage(this, (const unsigned char *)&gDevice, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendCurrentSensor(const int NodeID, const int BatteryLevel, const float Current1, const float Current2, const float Current3, const std::string &defaultname, const int RssiLevel /* =12 */)
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

	int at10 = round(std::abs(Current1*10.0f));
	tsen.CURRENT.ch1h = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP.temperatureh * 256);
	tsen.CURRENT.ch1l = (BYTE)(at10);

	at10 = round(std::abs(Current2*10.0f));
	tsen.CURRENT.ch2h = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP.temperatureh * 256);
	tsen.CURRENT.ch2l = (BYTE)(at10);

	at10 = round(std::abs(Current3*10.0f));
	tsen.CURRENT.ch3h = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP.temperatureh * 256);
	tsen.CURRENT.ch3l = (BYTE)(at10);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.CURRENT, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendPercentageSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float Percentage, const std::string &defaultname)
{
	_tGeneralDevice gDevice;
	gDevice.subtype = sTypePercentage;
	gDevice.id = ChildID;
	gDevice.intval1 = NodeID;
	gDevice.floatval1 = Percentage;
	sDecodeRXMessage(this, (const unsigned char *)&gDevice, defaultname.c_str(), BatteryLevel);
}

bool CDomoticzHardwareBase::CheckPercentageSensorExists(const int NodeID, const int ChildID)
{
	std::vector<std::vector<std::string> > result;
	char szTmp[30];
	sprintf(szTmp, "%08X", (unsigned int)NodeID);
	result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d)",
		m_HwdID, szTmp, int(pTypeGeneral), int(sTypePercentage));
	return (!result.empty());
}

void CDomoticzHardwareBase::SendWaterflowSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float LPM, const std::string &defaultname)
{
	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeWaterflow;
	gDevice.id = ChildID;
	gDevice.intval1 = (NodeID << 8) | ChildID;
	gDevice.floatval1 = LPM;
	sDecodeRXMessage(this, (const unsigned char *)&gDevice, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendCustomSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float Dust, const std::string &defaultname, const std::string &defaultLabel)
{
	std::vector<std::vector<std::string> > result;
	char szTmp[30];
	sprintf(szTmp, "0000%02X%02X", NodeID, ChildID);
	result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d)",
		m_HwdID, szTmp, int(pTypeGeneral), int(sTypeCustom));
	bool bDoesExists = !result.empty();

	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeCustom;
	gDevice.id = ChildID;
	gDevice.intval1 = (NodeID << 8) | ChildID;
	gDevice.floatval1 = Dust;

	if (bDoesExists)
		sDecodeRXMessage(this, (const unsigned char *)&gDevice, defaultname.c_str(), BatteryLevel);
	else
	{
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&gDevice, defaultname.c_str(), BatteryLevel);
		//Set the Label
		std::string soptions = "1;" + defaultLabel;
		m_sql.safe_query("UPDATE DeviceStatus SET Options='%q' WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d)",
			soptions.c_str(), m_HwdID, szTmp, int(pTypeGeneral), int(sTypeCustom));
	}
}

//wind direction is in steps of 22.5 degrees (360/16)
void CDomoticzHardwareBase::SendWind(const int NodeID, const int BatteryLevel, const int WindDir, const float WindSpeed, const float WindGust, const float WindTemp, const float WindChill, const bool bHaveWindTemp, const std::string &defaultname, const int RssiLevel /* =12 */)
{
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.WIND.packetlength = sizeof(tsen.WIND) - 1;
	tsen.WIND.packettype = pTypeWIND;
	if (!bHaveWindTemp)
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

	int sw = round(WindSpeed*10.0f);
	tsen.WIND.av_speedh = (BYTE)(sw / 256);
	sw -= (tsen.WIND.av_speedh * 256);
	tsen.WIND.av_speedl = (BYTE)(sw);

	int gw = round(WindGust*10.0f);
	tsen.WIND.gusth = (BYTE)(gw / 256);
	gw -= (tsen.WIND.gusth * 256);
	tsen.WIND.gustl = (BYTE)(gw);

	if (!bHaveWindTemp)
	{
		//No temp, only chill
		tsen.WIND.tempsign = (WindChill >= 0) ? 0 : 1;
		tsen.WIND.chillsign = (WindChill >= 0) ? 0 : 1;
		int at10 = round(std::abs(WindChill*10.0f));
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
		int at10 = round(std::abs(WindTemp*10.0f));
		tsen.WIND.temperatureh = (BYTE)(at10 / 256);
		at10 -= (tsen.WIND.temperatureh * 256);
		tsen.WIND.temperaturel = (BYTE)(at10);

		tsen.WIND.chillsign = (WindChill >= 0) ? 0 : 1;
		at10 = round(std::abs(WindChill*10.0f));
		tsen.WIND.chillh = (BYTE)(at10 / 256);
		at10 -= (tsen.WIND.chillh * 256);
		tsen.WIND.chilll = (BYTE)(at10);
	}

	sDecodeRXMessage(this, (const unsigned char *)&tsen.WIND, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendPressureSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float pressure, const std::string &defaultname)
{
	_tGeneralDevice gdevice;
	gdevice.subtype = sTypePressure;
	gdevice.intval1 = (NodeID << 8) | ChildID;
	gdevice.floatval1 = pressure;
	sDecodeRXMessage(this, (const unsigned char *)&gdevice, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendSolarRadiationSensor(const unsigned char NodeID, const int BatteryLevel, const float radiation, const std::string &defaultname)
{
	_tGeneralDevice gdevice;
	gdevice.subtype = sTypeSolarRadiation;
	gdevice.id = NodeID;
	gdevice.floatval1 = radiation;
	sDecodeRXMessage(this, (const unsigned char *)&gdevice, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendSoundSensor(const int NodeID, const int BatteryLevel, const int sLevel, const std::string &defaultname)
{
	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeSoundLevel;
	gDevice.id = 1;
	gDevice.intval1 = NodeID;
	gDevice.intval2 = sLevel;
	sDecodeRXMessage(this, (const unsigned char *)&gDevice, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendAlertSensor(const int NodeID, const int BatteryLevel, const int alertLevel, const std::string &message, const std::string &defaultname)
{
	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeAlert;
	gDevice.id = (unsigned char)NodeID;
	gDevice.intval1 = alertLevel;
	sprintf(gDevice.text, "%.63s", message.c_str());
	sDecodeRXMessage(this, (const unsigned char *)&gDevice, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendGeneralSwitchSensor(const int NodeID, const int BatteryLevel, const int switchState, const char* defaultname, const int unitCode)
{
	_tGeneralSwitch gSwitch;
	gSwitch.id = NodeID;
	gSwitch.unitcode = unitCode;
	gSwitch.cmnd = switchState;
	sDecodeRXMessage(this, (const unsigned char *)&gSwitch, defaultname, BatteryLevel);
}

void CDomoticzHardwareBase::SendMoistureSensor(const int NodeID, const int BatteryLevel, const int mLevel, const std::string &defaultname)
{
	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeSoilMoisture;
	gDevice.id = 1;
	gDevice.intval1 = NodeID;
	gDevice.intval2 = mLevel;
	sDecodeRXMessage(this, (const unsigned char *)&gDevice, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendUVSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float UVI, const std::string &defaultname, const int RssiLevel /* =12 */)
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

	tsen.UV.uv = (BYTE)round(UVI * 10);
	sDecodeRXMessage(this, (const unsigned char *)&tsen.UV, defaultname.c_str(), BatteryLevel);
}

void CDomoticzHardwareBase::SendZWaveAlarmSensor(const int NodeID, const int InstanceID, const int BatteryLevel, const int aType, const int aValue, const std::string &defaultname)
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
	sDecodeRXMessage(this, (const unsigned char *)&gDevice, defaultname.c_str(), BatteryLevel);
}

int CDomoticzHardwareBase::CalculateBaroForecast(const double pressure)
{
	//From 0 to 5 min.
	if (m_baro_minuteCount <= 5){
		m_pressureSamples[0][m_baro_minuteCount] = pressure;
	}
	//From 30 to 35 min.
	else if ((m_baro_minuteCount >= 30) && (m_baro_minuteCount <= 35)){
		m_pressureSamples[1][m_baro_minuteCount - 30] = pressure;
	}
	//From 60 to 65 min.
	else if ((m_baro_minuteCount >= 60) && (m_baro_minuteCount <= 65)){
		m_pressureSamples[2][m_baro_minuteCount - 60] = pressure;
	}
	//From 90 to 95 min.
	else if ((m_baro_minuteCount >= 90) && (m_baro_minuteCount <= 95)){
		m_pressureSamples[3][m_baro_minuteCount - 90] = pressure;
	}
	//From 120 to 125 min.
	else if ((m_baro_minuteCount >= 120) && (m_baro_minuteCount <= 125)){
		m_pressureSamples[4][m_baro_minuteCount - 120] = pressure;
	}
	//From 150 to 155 min.
	else if ((m_baro_minuteCount >= 150) && (m_baro_minuteCount <= 155)){
		m_pressureSamples[5][m_baro_minuteCount - 150] = pressure;
	}
	//From 180 to 185 min.
	else if ((m_baro_minuteCount >= 180) && (m_baro_minuteCount <= 185)){
		m_pressureSamples[6][m_baro_minuteCount - 180] = pressure;
	}
	//From 210 to 215 min.
	else if ((m_baro_minuteCount >= 210) && (m_baro_minuteCount <= 215)){
		m_pressureSamples[7][m_baro_minuteCount - 210] = pressure;
	}
	//From 240 to 245 min.
	else if ((m_baro_minuteCount >= 240) && (m_baro_minuteCount <= 245)){
		m_pressureSamples[8][m_baro_minuteCount - 240] = pressure;
	}


	if (m_baro_minuteCount == 5) {
		// Avg pressure in first 5 min, value averaged from 0 to 5 min.
		m_pressureAvg[0] = ((m_pressureSamples[0][0] + m_pressureSamples[0][1]
			+ m_pressureSamples[0][2] + m_pressureSamples[0][3]
			+ m_pressureSamples[0][4] + m_pressureSamples[0][5]) / 6);
	}
	else if (m_baro_minuteCount == 35) {
		// Avg pressure in 30 min, value averaged from 0 to 5 min.
		m_pressureAvg[1] = ((m_pressureSamples[1][0] + m_pressureSamples[1][1]
			+ m_pressureSamples[1][2] + m_pressureSamples[1][3]
			+ m_pressureSamples[1][4] + m_pressureSamples[1][5]) / 6);
		double change = (m_pressureAvg[1] - m_pressureAvg[0]);
		m_dP_dt = change / 5;
	}
	else if (m_baro_minuteCount == 65) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		m_pressureAvg[2] = ((m_pressureSamples[2][0] + m_pressureSamples[2][1]
			+ m_pressureSamples[2][2] + m_pressureSamples[2][3]
			+ m_pressureSamples[2][4] + m_pressureSamples[2][5]) / 6);
		double change = (m_pressureAvg[2] - m_pressureAvg[0]);
		m_dP_dt = change / 10;
	}
	else if (m_baro_minuteCount == 95) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		m_pressureAvg[3] = ((m_pressureSamples[3][0] + m_pressureSamples[3][1]
			+ m_pressureSamples[3][2] + m_pressureSamples[3][3]
			+ m_pressureSamples[3][4] + m_pressureSamples[3][5]) / 6);
		double change = (m_pressureAvg[3] - m_pressureAvg[0]);
		m_dP_dt = change / 15;
	}
	else if (m_baro_minuteCount == 125) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		m_pressureAvg[4] = ((m_pressureSamples[4][0] + m_pressureSamples[4][1]
			+ m_pressureSamples[4][2] + m_pressureSamples[4][3]
			+ m_pressureSamples[4][4] + m_pressureSamples[4][5]) / 6);
		double change = (m_pressureAvg[4] - m_pressureAvg[0]);
		m_dP_dt = change / 20;
	}
	else if (m_baro_minuteCount == 155) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		m_pressureAvg[5] = ((m_pressureSamples[5][0] + m_pressureSamples[5][1]
			+ m_pressureSamples[5][2] + m_pressureSamples[5][3]
			+ m_pressureSamples[5][4] + m_pressureSamples[5][5]) / 6);
		double change = (m_pressureAvg[5] - m_pressureAvg[0]);
		m_dP_dt = change / 25;
	}
	else if (m_baro_minuteCount == 185) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		m_pressureAvg[6] = ((m_pressureSamples[6][0] + m_pressureSamples[6][1]
			+ m_pressureSamples[6][2] + m_pressureSamples[6][3]
			+ m_pressureSamples[6][4] + m_pressureSamples[6][5]) / 6);
		double change = (m_pressureAvg[6] - m_pressureAvg[0]);
		m_dP_dt = change / 30;
	}
	else if (m_baro_minuteCount == 215) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		m_pressureAvg[7] = ((m_pressureSamples[7][0] + m_pressureSamples[7][1]
			+ m_pressureSamples[7][2] + m_pressureSamples[7][3]
			+ m_pressureSamples[7][4] + m_pressureSamples[7][5]) / 6);
		double change = (m_pressureAvg[7] - m_pressureAvg[0]);
		m_dP_dt = change / 35;
	}
	else if (m_baro_minuteCount == 245) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		m_pressureAvg[8] = ((m_pressureSamples[8][0] + m_pressureSamples[8][1]
			+ m_pressureSamples[8][2] + m_pressureSamples[8][3]
			+ m_pressureSamples[8][4] + m_pressureSamples[8][5]) / 6);
		double change = (m_pressureAvg[8] - m_pressureAvg[0]);
		m_dP_dt = change / 40; // note this is for t = 4 hour

		m_baro_minuteCount -= 30;
		m_pressureAvg[0] = m_pressureAvg[1];
		m_pressureAvg[1] = m_pressureAvg[2];
		m_pressureAvg[2] = m_pressureAvg[3];
		m_pressureAvg[3] = m_pressureAvg[4];
		m_pressureAvg[4] = m_pressureAvg[5];
		m_pressureAvg[5] = m_pressureAvg[6];
		m_pressureAvg[6] = m_pressureAvg[7];
		m_pressureAvg[7] = m_pressureAvg[8];
	}

	m_baro_minuteCount++;

	if (m_baro_minuteCount < 36) //if time is less than 35 min 
		return wsbaroforcast_unknown; // Unknown, more time needed
	else if (m_dP_dt < (-0.25))
		return wsbaroforcast_heavy_rain; // Quickly falling LP, Thunderstorm, not stable
	else if (m_dP_dt > 0.25)
		return wsbaroforcast_unstable; // Quickly rising HP, not stable weather
	else if ((m_dP_dt > (-0.25)) && (m_dP_dt < (-0.05)))
		return wsbaroforcast_rain; // Slowly falling Low Pressure System, stable rainy weather
	else if ((m_dP_dt > 0.05) && (m_dP_dt < 0.25))
		return wsbaroforcast_sunny; // Slowly rising HP stable good weather
	else if ((m_dP_dt >(-0.05)) && (m_dP_dt < 0.05))
		return wsbaroforcast_stable; // Stable weather
	else
		return wsbaroforcast_unknown; // Unknown
}
