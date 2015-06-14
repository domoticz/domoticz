#include "stdafx.h"
#include <iostream>
#include "DomoticzHardware.h"
#include "../main/Logger.h"
#include "../main/localtime_r.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
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
	m_iLastSendNodeBatteryValue = 255;
	m_iHBCounter = 0;
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
			sDecodeRXMessage(this, (const unsigned char *)&m_rxbuffer);
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
void CDomoticzHardwareBase::SendTempSensor(const int NodeID, const int BatteryLevel, const float temperature, const std::string &defaultname)
{
	bool bDeviceExits = true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	char szTmp[30];
	sprintf(szTmp, "%d", (unsigned int)NodeID);

	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeTEMP) << ") AND (Subtype==" << int(sTypeTEMP5) << ")";
	result = m_sql.query(szQuery.str());
	if (result.size() < 1)
	{
		bDeviceExits = false;
	}

	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.TEMP.packetlength = sizeof(tsen.TEMP) - 1;
	tsen.TEMP.packettype = pTypeTEMP;
	tsen.TEMP.subtype = sTypeTEMP5;
	tsen.TEMP.battery_level = BatteryLevel;
	tsen.TEMP.rssi = 12;
	tsen.TEMP.id1 = (NodeID & 0xFF00) >> 8;
	tsen.TEMP.id2 = NodeID & 0xFF;
	tsen.TEMP.tempsign = (temperature >= 0) ? 0 : 1;
	int at10 = round(temperature*10.0f);
	tsen.TEMP.temperatureh = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP.temperatureh * 256);
	tsen.TEMP.temperaturel = (BYTE)(at10);
	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeTEMP) << ") AND (Subtype==" << int(sTypeTEMP5) << ")";
		m_sql.query(szQuery.str());
	}
}

void CDomoticzHardwareBase::SendHumiditySensor(const int NodeID, const int BatteryLevel, const int humidity)
{
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.HUM.packetlength = sizeof(tsen.HUM) - 1;
	tsen.HUM.packettype = pTypeHUM;
	tsen.HUM.subtype = sTypeHUM1;
	tsen.HUM.battery_level = BatteryLevel;
	tsen.HUM.rssi = 12;
	tsen.HUM.id1 = (NodeID & 0xFF00) >> 8;
	tsen.HUM.id2 = NodeID & 0xFF;
	tsen.HUM.humidity = (BYTE)humidity;
	tsen.HUM.humidity_status = Get_Humidity_Level(tsen.HUM.humidity);
	sDecodeRXMessage(this, (const unsigned char *)&tsen.HUM);
}

void CDomoticzHardwareBase::SendBaroSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float pressure, const int forecast)
{
	_tGeneralDevice gdevice;
	gdevice.intval1 = (NodeID << 8) | ChildID;
	gdevice.intval2 = forecast;
	gdevice.subtype = sTypeBaro;
	gdevice.floatval1 = pressure;
	sDecodeRXMessage(this, (const unsigned char *)&gdevice);
}

void CDomoticzHardwareBase::SendTempHumSensor(const int NodeID, const int BatteryLevel, const float temperature, const int humidity, const std::string &defaultname)
{
	bool bDeviceExits = true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	char szTmp[30];
	sprintf(szTmp, "%d", (unsigned int)NodeID);

	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeTEMP_HUM) << ") AND (Subtype==" << int(sTypeTH5) << ")";
	result = m_sql.query(szQuery.str());
	if (result.size() < 1)
	{
		bDeviceExits = false;
	}

	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.TEMP_HUM.packetlength = sizeof(tsen.TEMP_HUM) - 1;
	tsen.TEMP_HUM.packettype = pTypeTEMP_HUM;
	tsen.TEMP_HUM.subtype = sTypeTH5;
	tsen.TEMP_HUM.battery_level = BatteryLevel;
	tsen.TEMP_HUM.rssi = 12;
	tsen.TEMP_HUM.id1 = (NodeID&0xFF00)>>8;
	tsen.TEMP_HUM.id2 = NodeID & 0xFF;

	tsen.TEMP_HUM.tempsign = (temperature >= 0) ? 0 : 1;
	int at10 = round(abs(temperature*10.0f));
	tsen.TEMP_HUM.temperatureh = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP_HUM.temperatureh * 256);
	tsen.TEMP_HUM.temperaturel = (BYTE)(at10);
	tsen.TEMP_HUM.humidity = (BYTE)humidity;
	tsen.TEMP_HUM.humidity_status = Get_Humidity_Level(tsen.TEMP_HUM.humidity);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP_HUM);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeTEMP_HUM) << ") AND (Subtype==" << int(sTypeTH5) << ")";
		m_sql.query(szQuery.str());
	}
}

void CDomoticzHardwareBase::SendTempHumBaroSensor(const int NodeID, const int BatteryLevel, const float temperature, const int humidity, const float pressure, int forecast)
{
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.TEMP_HUM_BARO.packetlength = sizeof(tsen.TEMP_HUM_BARO) - 1;
	tsen.TEMP_HUM_BARO.packettype = pTypeTEMP_HUM_BARO;
	tsen.TEMP_HUM_BARO.subtype = sTypeTHB1;
	tsen.TEMP_HUM_BARO.battery_level = BatteryLevel;
	tsen.TEMP_HUM_BARO.rssi = 12;
	tsen.TEMP_HUM_BARO.id1 = (NodeID & 0xFF00) >> 8;
	tsen.TEMP_HUM_BARO.id2 = NodeID & 0xFF;

	tsen.TEMP_HUM_BARO.tempsign = (temperature >= 0) ? 0 : 1;
	int at10 = round(abs(temperature*10.0f));
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

	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP_HUM_BARO);
}

void CDomoticzHardwareBase::SendTempHumBaroSensorFloat(const int NodeID, const int BatteryLevel, const float temperature, const int humidity, const float pressure, int forecast)
{
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.TEMP_HUM_BARO.packetlength = sizeof(tsen.TEMP_HUM_BARO) - 1;
	tsen.TEMP_HUM_BARO.packettype = pTypeTEMP_HUM_BARO;
	tsen.TEMP_HUM_BARO.subtype = sTypeTHBFloat;
	tsen.TEMP_HUM_BARO.battery_level = 9;
	tsen.TEMP_HUM_BARO.rssi = 12;
	tsen.TEMP_HUM_BARO.id1 = (NodeID & 0xFF00) >> 8;
	tsen.TEMP_HUM_BARO.id2 = NodeID & 0xFF;

	tsen.TEMP_HUM_BARO.tempsign = (temperature >= 0) ? 0 : 1;
	int at10 = round(abs(temperature*10.0f));
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

	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP_HUM_BARO);//decode message
}

void CDomoticzHardwareBase::SendDistanceSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float distance)
{
	_tGeneralDevice gdevice;
	gdevice.intval1 = (NodeID << 8) | ChildID;
	gdevice.subtype = sTypeDistance;
	gdevice.floatval1 = distance;
	sDecodeRXMessage(this, (const unsigned char *)&gdevice);
}

void CDomoticzHardwareBase::SendRainSensor(const int NodeID, const int BatteryLevel, const int RainCounter)
{
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.RAIN.packetlength = sizeof(tsen.RAIN) - 1;
	tsen.RAIN.packettype = pTypeRAIN;
	tsen.RAIN.subtype = sTypeRAIN3;
	tsen.RAIN.battery_level = BatteryLevel;
	tsen.RAIN.rssi = 12;
	tsen.RAIN.id1 = (NodeID & 0xFF00) >> 8;
	tsen.RAIN.id2 = NodeID & 0xFF;

	tsen.RAIN.rainrateh = 0;
	tsen.RAIN.rainratel = 0;

	int tr10 = int((float(RainCounter)*10.0f)*0.7f);

	tsen.RAIN.raintotal1 = 0;
	tsen.RAIN.raintotal2 = (BYTE)(tr10 / 256);
	tr10 -= (tsen.RAIN.raintotal2 * 256);
	tsen.RAIN.raintotal3 = (BYTE)(tr10);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.RAIN);
}

void CDomoticzHardwareBase::SendKwhMeter(const int NodeID, const int ChildID, const int BatteryLevel, const double musage, const double mtotal, const std::string &defaultname)
{
	int Idx = (NodeID * 256) + ChildID;
	bool bDeviceExits = true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(Idx) << ") AND (Type==" << int(pTypeENERGY) << ") AND (Subtype==" << int(sTypeELEC2) << ")";
	result = m_sql.query(szQuery.str());
	if (result.size() < 1)
	{
		bDeviceExits = false;
	}

	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));

	tsen.ENERGY.packettype = pTypeENERGY;
	tsen.ENERGY.subtype = sTypeELEC2;
	tsen.ENERGY.packetlength = sizeof(tsen.ENERGY) - 1;
	tsen.ENERGY.id1 = NodeID;
	tsen.ENERGY.id2 = ChildID;
	tsen.ENERGY.count = 1;
	tsen.ENERGY.rssi = 12;

	tsen.ENERGY.battery_level = BatteryLevel;

	unsigned long long instant = (unsigned long long)(musage*1000.0);
	tsen.ENERGY.instant1 = (unsigned char)(instant / 0x1000000);
	instant -= tsen.ENERGY.instant1 * 0x1000000;
	tsen.ENERGY.instant2 = (unsigned char)(instant / 0x10000);
	instant -= tsen.ENERGY.instant2 * 0x10000;
	tsen.ENERGY.instant3 = (unsigned char)(instant / 0x100);
	instant -= tsen.ENERGY.instant3 * 0x100;
	tsen.ENERGY.instant4 = (unsigned char)(instant);

	double total = (mtotal*1000.0)*223.666;
	tsen.ENERGY.total1 = (unsigned char)(total / 0x10000000000ULL);
	total -= tsen.ENERGY.total1 * 0x10000000000ULL;
	tsen.ENERGY.total2 = (unsigned char)(total / 0x100000000ULL);
	total -= tsen.ENERGY.total2 * 0x100000000ULL;
	tsen.ENERGY.total3 = (unsigned char)(total / 0x1000000);
	total -= tsen.ENERGY.total3 * 0x1000000;
	tsen.ENERGY.total4 = (unsigned char)(total / 0x10000);
	total -= tsen.ENERGY.total4 * 0x10000;
	tsen.ENERGY.total5 = (unsigned char)(total / 0x100);
	total -= tsen.ENERGY.total5 * 0x100;
	tsen.ENERGY.total6 = (unsigned char)(total);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.ENERGY);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(Idx) << ") AND (Type==" << int(pTypeENERGY) << ") AND (Subtype==" << int(sTypeELEC2) << ")";
		m_sql.query(szQuery.str());
	}
}

void CDomoticzHardwareBase::SendMeterSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float metervalue)
{
	unsigned long counter = (unsigned long)(metervalue*1000.0f);
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.RFXMETER.packetlength = sizeof(tsen.RFXMETER) - 1;
	tsen.RFXMETER.packettype = pTypeRFXMeter;
	tsen.RFXMETER.subtype = sTypeRFXMeterCount;
	tsen.RFXMETER.rssi = 12;
	tsen.RFXMETER.id1 = (BYTE)NodeID;
	tsen.RFXMETER.id2 = (BYTE)ChildID;

	tsen.RFXMETER.count1 = (BYTE)((counter & 0xFF000000) >> 24);
	tsen.RFXMETER.count2 = (BYTE)((counter & 0x00FF0000) >> 16);
	tsen.RFXMETER.count3 = (BYTE)((counter & 0x0000FF00) >> 8);
	tsen.RFXMETER.count4 = (BYTE)(counter & 0x000000FF);
	sDecodeRXMessage(this, (const unsigned char *)&tsen.RFXMETER);
}


void CDomoticzHardwareBase::SendLuxSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float Lux)
{
	_tLightMeter lmeter;
	lmeter.id1 = 0;
	lmeter.id2 = 0;
	lmeter.id3 = 0;
	lmeter.id4 = NodeID;
	lmeter.dunit = ChildID;
	lmeter.fLux = Lux;
	lmeter.battery_level = BatteryLevel;
	sDecodeRXMessage(this, (const unsigned char *)&lmeter);
}

void CDomoticzHardwareBase::SendAirQualitySensor(const int NodeID, const int ChildID, const int BatteryLevel, const int AirQuality)
{
	_tAirQualityMeter meter;
	meter.len = sizeof(_tAirQualityMeter) - 1;
	meter.type = pTypeAirQuality;
	meter.subtype = sTypeVoltcraft;
	meter.airquality = AirQuality;
	meter.id1 = NodeID;
	meter.id2 = ChildID;
	sDecodeRXMessage(this, (const unsigned char *)&meter);
}

void CDomoticzHardwareBase::SendUsageSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float Usage)
{
	_tUsageMeter umeter;
	umeter.id1 = 0;
	umeter.id2 = 0;
	umeter.id3 = 0;
	umeter.id4 = NodeID;
	umeter.dunit = ChildID;
	umeter.fusage = Usage;
	sDecodeRXMessage(this, (const unsigned char *)&umeter);
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
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szIdx << "') AND (Unit == " << ChildID << ") AND (Type==" << int(pTypeLighting2) << ") AND (Subtype==" << int(sTypeAC) << ")";
	result = m_sql.query(szQuery.str()); //-V519
	if (result.size() < 1)
	{
		SendSwitch(NodeID, ChildID, BatteryLevel, bOn, Level, defaultname);
	}
}

void CDomoticzHardwareBase::SendSwitch(const int NodeID, const int ChildID, const int BatteryLevel, const bool bOn, const double Level, const std::string &defaultname)
{
	bool bDeviceExits = true;
	double rlevel = (15.0 / 100)*Level;
	int level = int(rlevel);

	//make device ID
	unsigned char ID1 = (unsigned char)((NodeID & 0xFF000000) >> 24);
	unsigned char ID2 = (unsigned char)((NodeID & 0xFF0000) >> 16);
	unsigned char ID3 = (unsigned char)((NodeID & 0xFF00) >> 8);
	unsigned char ID4 = (unsigned char)NodeID & 0xFF;

	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", ID1, ID2, ID3, ID4);
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szIdx << "') AND (Unit == " << ChildID << ") AND (Type==" << int(pTypeLighting2) << ") AND (Subtype==" << int(sTypeAC) << ")";
	result = m_sql.query(szQuery.str()); //-V519
	if (result.size() < 1)
	{
		bDeviceExits = false;
	}
	else
	{
		//check if we have a change, if not do not update it
		int nvalue = atoi(result[0][1].c_str());
		if ((!bOn) && (nvalue == 0))
			return;
		if (bOn && (nvalue == light2_sOn))
			return;
		if ((bOn && (nvalue != 0)))
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
	lcmd.LIGHTING2.rssi = 12;
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szIdx << "') AND (Unit == " << ChildID << ") AND (Type==" << int(pTypeLighting2) << ") AND (Subtype==" << int(sTypeAC) << ")";
		m_sql.query(szQuery.str());
	}
}

void CDomoticzHardwareBase::SendBlindSensor(const int NodeID, const int ChildID, const int BatteryLevel, const int Command, const std::string &defaultname)
{
	bool bDeviceExits = true;

	char szIdx[10];
	sprintf(szIdx, "%02X%02X%02X", 0,0,NodeID);
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szIdx << "') AND (Unit == " << ChildID << ") AND (Type==" << int(pTypeBlinds) << ") AND (Subtype==" << int(sTypeBlindsT0) << ")";
	result = m_sql.query(szQuery.str()); //-V519
	if (result.size() < 1)
	{
		bDeviceExits = false;
	}

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
	lcmd.BLINDS1.rssi = 12;
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.BLINDS1);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szIdx << "') AND (Unit == " << ChildID << ") AND (Type==" << int(pTypeBlinds) << ") AND (Subtype==" << int(sTypeBlindsT0) << ")";
		m_sql.query(szQuery.str());
	}
}

void CDomoticzHardwareBase::SendRGBWSwitch(const int NodeID, const int ChildID, const int BatteryLevel, const double Level, const bool bIsRGBW, const std::string &defaultname)
{
	bool bDeviceExits = true;
	int level = int(Level);

	int dID = (NodeID << 8) | ChildID;

	char szIdx[10];
	sprintf(szIdx, "%08X", dID);

	int subType = (bIsRGBW == true) ? sTypeLimitlessRGBW : sTypeLimitlessRGB;

	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szIdx << "') AND (Type==" << int(pTypeLimitlessLights) << ") AND (Subtype==" << subType << ")";
	result = m_sql.query(szQuery.str());
	if (result.size() < 1)
	{
		bDeviceExits = false;
	}
	//Send as LimitlessLight
	_tLimitlessLights lcmd;
	lcmd.id = dID;
	lcmd.subtype = subType;
	if (level == 0)
		lcmd.command = Limitless_LedOff;
	else
		lcmd.command = Limitless_LedOn;
	lcmd.value = level;
	sDecodeRXMessage(this, (const unsigned char *)&lcmd);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szIdx << "') AND (Type==" << int(pTypeLimitlessLights) << ") AND (Subtype==" << subType << ")";
		m_sql.query(szQuery.str());
	}
}

void CDomoticzHardwareBase::SendVoltageSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float Volt, const std::string &defaultname)
{
	bool bDeviceExits = true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	int dID = (NodeID << 8) | ChildID;

	char szTmp[30];
	sprintf(szTmp, "%08X", dID);

	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypeVoltage) << ")";
	result = m_sql.query(szQuery.str());
	if (result.size() < 1)
	{
		bDeviceExits = false;
	}

	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeVoltage;
	gDevice.id = ChildID;
	gDevice.floatval1 = Volt;
	gDevice.intval1 = dID;
	sDecodeRXMessage(this, (const unsigned char *)&gDevice);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypeVoltage) << ")";
		m_sql.query(szQuery.str());
	}
}

void CDomoticzHardwareBase::SendCurrentSensor(const int NodeID, const int BatteryLevel, const float Current1, const float Current2, const float Current3, const std::string &defaultname)
{
	bool bDeviceExits = true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	char szTmp[30];
	sprintf(szTmp, "%d", NodeID);

	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeCURRENT) << ") AND (Subtype==" << int(sTypeELEC1) << ")";
	result = m_sql.query(szQuery.str());
	if (result.size() < 1)
	{
		bDeviceExits = false;
	}

	tRBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.CURRENT.packetlength = sizeof(tsen.CURRENT) - 1;
	tsen.CURRENT.packettype = pTypeCURRENT;
	tsen.CURRENT.subtype = sTypeELEC1;
	tsen.CURRENT.rssi = 12;
	tsen.CURRENT.id1 = (NodeID & 0xFF00) >> 8;
	tsen.CURRENT.id2 = NodeID & 0xFF;
	tsen.CURRENT.battery_level = BatteryLevel;

	int at10 = round(abs(Current1*10.0f));
	tsen.CURRENT.ch1h = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP.temperatureh * 256);
	tsen.CURRENT.ch1l = (BYTE)(at10);

	at10 = round(abs(Current2*10.0f));
	tsen.CURRENT.ch2h = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP.temperatureh * 256);
	tsen.CURRENT.ch2l = (BYTE)(at10);

	at10 = round(abs(Current3*10.0f));
	tsen.CURRENT.ch3h = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP.temperatureh * 256);
	tsen.CURRENT.ch3l = (BYTE)(at10);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.CURRENT);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeCURRENT) << ") AND (Subtype==" << int(sTypeELEC1) << ")";
		m_sql.query(szQuery.str());
	}
}

void CDomoticzHardwareBase::SendPercentageSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float Percentage, const std::string &defaultname)
{
	bool bDeviceExits = true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	char szTmp[30];
	sprintf(szTmp, "%08X", (unsigned int)NodeID);

	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypePercentage) << ")";
	result = m_sql.query(szQuery.str());
	if (result.size() < 1)
	{
		bDeviceExits = false;
	}

	_tGeneralDevice gDevice;
	gDevice.subtype = sTypePercentage;
	gDevice.id = ChildID;
	gDevice.floatval1 = Percentage;
	gDevice.intval1 = static_cast<int>(NodeID);
	sDecodeRXMessage(this, (const unsigned char *)&gDevice);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypePercentage) << ")";
		m_sql.query(szQuery.str());
	}
}

bool CDomoticzHardwareBase::CheckPercentageSensorExists(const int NodeID, const int ChildID)
{
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	char szTmp[30];
	sprintf(szTmp, "%08X", (unsigned int)NodeID);
	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypePercentage) << ")";
	result = m_sql.query(szQuery.str());
	return (!result.empty());
}

void CDomoticzHardwareBase::SendWind(const int NodeID, const int BatteryLevel, const int WindDir, const float WindSpeed, const float WindGust, const float WindTemp, const float WindChill, const bool bHaveWindTemp)
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
	tsen.WIND.rssi = 12;
	tsen.WIND.id1 = (NodeID & 0xFF00) >> 8;
	tsen.WIND.id2 = NodeID & 0xFF;

	float winddir = float(WindDir)*22.5f;
	int aw = round(winddir);
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
		int at10 = round(abs(WindChill*10.0f));
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
		int at10 = round(abs(WindTemp*10.0f));
		tsen.WIND.temperatureh = (BYTE)(at10 / 256);
		at10 -= (tsen.WIND.temperatureh * 256);
		tsen.WIND.temperaturel = (BYTE)(at10);

		tsen.WIND.chillsign = (WindChill >= 0) ? 0 : 1;
		at10 = round(abs(WindChill*10.0f));
		tsen.WIND.chillh = (BYTE)(at10 / 256);
		at10 -= (tsen.WIND.chillh * 256);
		tsen.WIND.chilll = (BYTE)(at10);
	}

	sDecodeRXMessage(this, (const unsigned char *)&tsen.WIND);
}

void CDomoticzHardwareBase::SendPressureSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float pressure)
{
	_tGeneralDevice gdevice;
	gdevice.intval1 = (NodeID << 8) | ChildID;
	gdevice.subtype = sTypePressure;
	gdevice.floatval1 = pressure;
	sDecodeRXMessage(this, (const unsigned char *)&gdevice);
}

void CDomoticzHardwareBase::SenUVSensor(const int NodeID, const int ChildID, const int BatteryLevel, const float UVI)
{
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.UV.packetlength = sizeof(tsen.UV) - 1;
	tsen.UV.packettype = pTypeUV;
	tsen.UV.subtype = sTypeUV1;
	tsen.UV.battery_level = BatteryLevel;
	tsen.UV.rssi = 12;
	tsen.UV.id1 = (unsigned char)NodeID;
	tsen.UV.id2 = (unsigned char)ChildID;

	tsen.UV.uv = (BYTE)round(UVI * 10);
	sDecodeRXMessage(this, (const unsigned char *)&tsen.UV);
}
