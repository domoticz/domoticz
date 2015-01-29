#include "stdafx.h"
#include "MySensorsBase.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "hardwaretypes.h"
#include <string>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>

#include <ctime>

#define round(a) ( int ) ( a + .5 )

MySensorsBase::MySensorsBase(void)
{
	m_bufferpos = 0;
}


MySensorsBase::~MySensorsBase(void)
{
}

void MySensorsBase::LoadDevicesFromDatabase()
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	m_nodes.clear();

	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;
	szQuery << "SELECT ID, SketchName, SketchVersion FROM MySensors WHERE (HardwareID=" << m_HwdID << ") ORDER BY ID ASC";
	result = m_sql.query(szQuery.str());
	if (result.size() > 0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;

			int ID = atoi(sd[0].c_str());
			std::string SkectName = sd[1];
			std::string SkectVersion = sd[2];

			_tMySensorNode mNode;
			mNode.nodeID = ID;
			mNode.SketchName = SkectName;
			mNode.SketchVersion = SkectVersion;
			mNode.lastreceived = mytime(NULL);
			m_nodes[ID] = mNode;
		}
	}
}

void MySensorsBase::Add2Database(const int nodeID, const std::string &SketchName, const std::string &SketchVersion)
{
	std::stringstream szQuery;
	szQuery << "INSERT INTO MySensors (HardwareID, ID, SketchName, SketchVersion) VALUES (" << m_HwdID << "," << nodeID << ", '" << SketchName << "', '" << SketchVersion << "')";
	m_sql.query(szQuery.str());
}

void MySensorsBase::DatabaseUpdateSketchName(const int nodeID, const std::string &SketchName)
{
	std::stringstream szQuery;
	szQuery << "UPDATE MySensors SET SketchName='" << SketchName << "' WHERE (HardwareID=" << m_HwdID << ") AND (ID=" << nodeID << ")";
	m_sql.query(szQuery.str());
}

void MySensorsBase::DatabaseUpdateSketchVersion(const int nodeID, const std::string &SketchVersion)
{
	std::stringstream szQuery;
	szQuery << "UPDATE MySensors SET SketchVersion='" << SketchVersion << "' WHERE (HardwareID=" << m_HwdID << ") AND (ID=" << nodeID << ")";
	m_sql.query(szQuery.str());
}

int MySensorsBase::FindNextNodeID()
{
	unsigned char _UsedValues[256];
	memset(_UsedValues, 0, sizeof(_UsedValues));
	std::map<int, _tMySensorNode>::const_iterator itt;
	for (itt = m_nodes.begin(); itt != m_nodes.end(); ++itt)
	{
		int ID = itt->first;
		if (ID < 255)
		{
			_UsedValues[ID] = 1;
		}
	}
	for (int ii = 1; ii < 255; ii++)
	{
		if (_UsedValues[ii] == 0)
			return ii;
	}
	return -1;
}

MySensorsBase::_tMySensorNode* MySensorsBase::FindNode(const int nodeID)
{
	std::map<int, _tMySensorNode>::iterator itt;
	itt = m_nodes.find(nodeID);
	return (itt == m_nodes.end()) ? NULL : &itt->second;
}

MySensorsBase::_tMySensorNode* MySensorsBase::InsertNode(const int nodeID)
{
	_tMySensorNode mNode;
	mNode.nodeID = nodeID;
	mNode.SketchName = "Unknown";
	mNode.SketchVersion = "1.0";
	mNode.lastreceived = time(NULL);
	m_nodes[mNode.nodeID] = mNode;
	Add2Database(mNode.nodeID, mNode.SketchName, mNode.SketchVersion);
	return FindNode(nodeID);
}

//Find sensor with childID+devType
MySensorsBase::_tMySensorSensor* MySensorsBase::FindSensor(_tMySensorNode *pNode, const int childID, _eSetType devType)
{
	std::vector<_tMySensorSensor>::iterator itt;
	for (itt = pNode->m_sensors.begin(); itt != pNode->m_sensors.end(); ++itt)
	{
		if (
			(itt->childID == childID)&&
			(itt->devType == devType)
			)
			return &*itt;
	}
	return NULL;
}

//Find any sensor with devType
MySensorsBase::_tMySensorSensor* MySensorsBase::FindSensor(const int nodeID, _eSetType devType)
{
	std::map<int, _tMySensorNode>::iterator ittNode;
	ittNode = m_nodes.find(nodeID);
	if (ittNode == m_nodes.end())
		return NULL;
	_tMySensorNode *pNode = &ittNode->second;
	std::vector<_tMySensorSensor>::iterator itt;
	for (itt = pNode->m_sensors.begin(); itt != pNode->m_sensors.end(); ++itt)
	{
		if (itt->devType == devType)
			return &*itt;
	}
	return NULL;
}

void MySensorsBase::SendTempSensor(const unsigned char NodeID, const int ChildID, const float temperature)
{
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.TEMP.packetlength = sizeof(tsen.TEMP) - 1;
	tsen.TEMP.packettype = pTypeTEMP;
	tsen.TEMP.subtype = sTypeTEMP5;
	tsen.TEMP.battery_level = 9;
	tsen.TEMP.rssi = 12;
	tsen.TEMP.id1 = (BYTE)0;
	tsen.TEMP.id2 = (BYTE)NodeID;
	tsen.TEMP.tempsign = (temperature >= 0) ? 0 : 1;
	int at10 = round(abs(temperature*10.0f));
	tsen.TEMP.temperatureh = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP.temperatureh * 256);
	tsen.TEMP.temperaturel = (BYTE)(at10);
	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP);
}

void MySensorsBase::SendHumiditySensor(const unsigned char NodeID, const int ChildID, const float humidity)
{
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.HUM.packetlength = sizeof(tsen.HUM) - 1;
	tsen.HUM.packettype = pTypeHUM;
	tsen.HUM.subtype = sTypeHUM1;
	tsen.HUM.battery_level = 9;
	tsen.HUM.rssi = 12;
	tsen.HUM.id1 = (BYTE)0;
	tsen.HUM.id2 = (BYTE)NodeID;
	tsen.HUM.humidity = (BYTE)round(humidity);
	tsen.HUM.humidity_status = Get_Humidity_Level(tsen.HUM.humidity);
	sDecodeRXMessage(this, (const unsigned char *)&tsen.HUM);
}

void MySensorsBase::SendBaroSensor(const unsigned char NodeID, const int ChildID, const float pressure)
{
	_tGeneralDevice gdevice;
	gdevice.intval1 = (NodeID << 8) | ChildID;
	gdevice.subtype = sTypePressure;
	gdevice.floatval1 = pressure;
	sDecodeRXMessage(this, (const unsigned char *)&gdevice);
}

void MySensorsBase::SendTempHumSensor(const unsigned char NodeID, const int ChildID, const float temperature, const float humidity)
{
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.TEMP_HUM.packetlength = sizeof(tsen.TEMP_HUM) - 1;
	tsen.TEMP_HUM.packettype = pTypeTEMP_HUM;
	tsen.TEMP_HUM.subtype = sTypeTH5;
	tsen.TEMP_HUM.battery_level = 9;
	tsen.TEMP_HUM.rssi = 12;
	tsen.TEMP_HUM.id1 = 0;
	tsen.TEMP_HUM.id2 = NodeID;

	tsen.TEMP_HUM.tempsign = (temperature >= 0) ? 0 : 1;
	int at10 = round(abs(temperature*10.0f));
	tsen.TEMP_HUM.temperatureh = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP_HUM.temperatureh * 256);
	tsen.TEMP_HUM.temperaturel = (BYTE)(at10);
	tsen.TEMP_HUM.humidity = (BYTE)humidity;
	tsen.TEMP_HUM.humidity_status = Get_Humidity_Level(tsen.TEMP_HUM.humidity);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP_HUM);
}

void MySensorsBase::SendTempHumBaroSensor(const unsigned char NodeID, const int ChildID, const float temperature, const float humidity, const float pressure, int forecast)
{
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.TEMP_HUM_BARO.packetlength = sizeof(tsen.TEMP_HUM_BARO) - 1;
	tsen.TEMP_HUM_BARO.packettype = pTypeTEMP_HUM_BARO;
	tsen.TEMP_HUM_BARO.subtype = sTypeTHB1;
	tsen.TEMP_HUM_BARO.battery_level = 9;
	tsen.TEMP_HUM_BARO.rssi = 12;
	tsen.TEMP_HUM_BARO.id1 = 0;
	tsen.TEMP_HUM_BARO.id2 = NodeID;

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

void MySensorsBase::SendSensor2Domoticz(const _tMySensorNode *pNode, const _tMySensorSensor *pSensor)
{
	switch (pSensor->devType)
	{
	case V_TEMP:
	{
		SendTempSensor(pSensor->nodeID, pSensor->childID, pSensor->floatValue);
		_tMySensorSensor *pSensorHum = FindSensor(pSensor->nodeID, V_HUM);
		_tMySensorSensor *pSensorBaro = FindSensor(pSensor->nodeID, V_PRESSURE);
		if (pSensorHum && pSensorBaro)
		{
			int forecast = baroForecastNoInfo;
			_tMySensorSensor *pSensorForecast = FindSensor(pSensor->nodeID, V_FORECAST);
			if (pSensorForecast)
				forecast = pSensorForecast->intvalue;
			SendTempHumBaroSensor(pSensor->nodeID, pSensor->childID, pSensor->floatValue, pSensorHum->floatValue, pSensorBaro->floatValue, forecast);
		}
		else if (pSensorHum) {
			SendTempHumSensor(pSensor->nodeID, pSensor->childID, pSensor->floatValue, pSensorHum->floatValue);
		}
	}
	break;
	case V_HUM:
	{
		SendHumiditySensor(pSensor->nodeID, pSensor->childID, pSensor->floatValue);
		_tMySensorSensor *pSensorTemp = FindSensor(pSensor->nodeID, V_TEMP);
		_tMySensorSensor *pSensorBaro = FindSensor(pSensor->nodeID, V_PRESSURE);
		if (pSensorTemp && pSensorBaro)
		{
			int forecast = baroForecastNoInfo;
			_tMySensorSensor *pSensorForecast = FindSensor(pSensor->nodeID, V_FORECAST);
			if (pSensorForecast)
				forecast = pSensorForecast->intvalue;
			SendTempHumBaroSensor(pSensor->nodeID, pSensor->childID, pSensorTemp->floatValue, pSensor->floatValue, pSensorBaro->floatValue, forecast);
		}
		else if (pSensorTemp) {
			SendTempHumSensor(pSensor->nodeID, pSensor->childID, pSensorTemp->floatValue, pSensor->floatValue);
		}
	}
	break;
	case V_PRESSURE:
	{
		SendBaroSensor(pSensor->nodeID, pSensor->childID, pSensor->floatValue);
		_tMySensorSensor *pSensorTemp = FindSensor(pSensor->nodeID, V_TEMP);
		_tMySensorSensor *pSensorHum = FindSensor(pSensor->nodeID, V_HUM);
		if (pSensorTemp && pSensorHum)
		{
			int forecast = baroForecastNoInfo;
			_tMySensorSensor *pSensorForecast = FindSensor(pSensor->nodeID, V_FORECAST);
			if (pSensorForecast)
				forecast = pSensorForecast->intvalue;
			SendTempHumBaroSensor(pSensor->nodeID, pSensor->childID, pSensorTemp->floatValue, pSensorHum->floatValue, pSensor->floatValue, forecast);
		}
	}
	break;
	case V_TRIPPED:
		//	Tripped status of a security sensor. 1 = Tripped, 0 = Untripped
		UpdateSwitch(pSensor->nodeID, pSensor->childID, (pSensor->intvalue == 1), 100, "Security Sensor");
		break;
	case V_LIGHT:
		//	Light status. 0 = off 1 = on
		UpdateSwitch(pSensor->nodeID, pSensor->childID, (pSensor->intvalue != 0), 100, "Light");
		break;
	case V_DIMMER:
		//	Dimmer value. 0 - 100 %
	{
		int level = pSensor->intvalue;
		UpdateSwitch(pSensor->nodeID, pSensor->childID, (level != 0), level, "Light");
	}
	break;
	case V_DUST_LEVEL:
	{
		_tAirQualityMeter meter;
		meter.len = sizeof(_tAirQualityMeter) - 1;
		meter.type = pTypeAirQuality;
		meter.subtype = sTypeVoltcraft;
		meter.airquality = pSensor->intvalue;
		meter.id1 = pSensor->nodeID;
		meter.id2 = pSensor->childID;
		sDecodeRXMessage(this, (const unsigned char *)&meter);//decode message
	}
	break;
	case V_WATT:
		while (1==0);
		break;
	case V_KWH:
		while (1 == 0);
		break;
	case V_FLOW:
		//Flow of water in meter
		while (1==0);
		break;
	case V_VOLUME:
		//Water Volume
		while (1==0);
		break;
	case V_FORECAST:
	{
		std::stringstream sstr;
		sstr << pSensor->nodeID;
		std::string devname;
		m_sql.UpdateValue(m_HwdID, sstr.str().c_str(), pSensor->childID, pTypeGeneral, sTypeTextStatus, 12, 255, 0, pSensor->stringValue.c_str(), devname);
	}
	break;
	}
}

void MySensorsBase::ParseData(const unsigned char *pData, int Len)
{
	int ii=0;
	while (ii<Len)
	{
		const unsigned char c = pData[ii];
		if(c == 0x0d)
		{
			ii++;
			continue;
		}

		if(c == 0x0a || m_bufferpos == sizeof(m_buffer) - 1)
		{
			// discard newline, close string, parse line and clear it.
			if(m_bufferpos > 0) m_buffer[m_bufferpos] = 0;
			ParseLine();
			m_bufferpos = 0;
		}
		else
		{
			m_buffer[m_bufferpos] = c;
			m_bufferpos++;
		}
		ii++;
	}
}

void MySensorsBase::UpdateSwitch(const unsigned char Idx, const int SubUnit, const bool bOn, const double Level, const std::string &defaultname)
{
	bool bDeviceExits = true;
	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", 0, 0, 0, Idx);
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szIdx << "')";
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
		if ((bOn && (nvalue != 0)))
			return;
	}

	//Send as Lighting 2
	tRBUF lcmd;
	memset(&lcmd, 0, sizeof(RBUF));
	lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
	lcmd.LIGHTING2.packettype = pTypeLighting2;
	lcmd.LIGHTING2.subtype = sTypeAC;
	lcmd.LIGHTING2.id1 = 0;
	lcmd.LIGHTING2.id2 = 0;
	lcmd.LIGHTING2.id3 = 0;
	lcmd.LIGHTING2.id4 = Idx;
	lcmd.LIGHTING2.unitcode = SubUnit;
	double rlevel = (15.0 / 100)*Level;
	int level = int(rlevel);
	if (!bOn)
	{
		level = 0;
		lcmd.LIGHTING2.cmnd = light2_sOff;
	}
	else
	{
		lcmd.LIGHTING2.cmnd = light2_sOn;
	}
	lcmd.LIGHTING2.level = level;
	lcmd.LIGHTING2.filler = 0;
	lcmd.LIGHTING2.rssi = 12;
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2);//decode message

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szIdx << "')";
		m_sql.query(szQuery.str());
	}
}

void MySensorsBase::SendCommand(const int NodeID, const int ChildID, const _eMessageType messageType, const int SubType, const std::string &Payload)
{
	std::stringstream sstr;
	sstr << NodeID << ";" << ChildID << ";" << int(messageType) << ";0;" << SubType << ";" << Payload << '\n';
	WriteInt(sstr.str());
}

void MySensorsBase::ParseLine()
{
	if (m_bufferpos<2)
		return;
	std::string sLine((char*)&m_buffer);

	//_log.Log(LOG_STATUS, sLine.c_str());

	std::vector<std::string> results;
	StringSplit(sLine, ";", results);
	if (results.size()<5)
		return; //invalid data

	int node_id = atoi(results[0].c_str());
	int child_sensor_id = atoi(results[1].c_str());
	_eMessageType message_type = (_eMessageType)atoi(results[2].c_str());
	int ack = atoi(results[3].c_str());
	int sub_type = atoi(results[4].c_str());
	std::string payload = "";
	if (results.size()>=6)
		payload=results[5];

	std::string retMessage;
	std::stringstream sstr;
//	_log.Log(LOG_NORM, "MySensors: NodeID: %d, ChildID: %d, MessageType: %d, Ack: %d, SubType: %d, Payload: %s",node_id,child_sensor_id,message_type,ack,sub_type,payload.c_str());

	if (message_type == MT_Internal)
	{
		switch (sub_type)
		{
		case I_ID_REQUEST:
			{
				//Set a unique node id from the controller
				int newID = FindNextNodeID();
				if (newID != -1)
				{
					sstr << newID;
					SendCommand(node_id, child_sensor_id, message_type, I_ID_RESPONSE, sstr.str());
				}
			}
			break;
		case I_CONFIG:
			// (M)etric or (I)mperal back to sensor.
			//Set a unique node id from the controller
			SendCommand(node_id, child_sensor_id, message_type, I_CONFIG, "M");
			break;
		case I_SKETCH_NAME:
			_log.Log(LOG_STATUS, "MySensors: Node: %d, Sketch Name: %s", node_id, payload.c_str());
			if (_tMySensorNode *pNode = FindNode(node_id))
			{
				DatabaseUpdateSketchName(node_id, payload);
			}
			else
			{
				//Unknown Node
				InsertNode(node_id);
				DatabaseUpdateSketchName(node_id, payload);
			}
			break;
		case I_SKETCH_VERSION:
			_log.Log(LOG_STATUS, "MySensors: Node: %d, Sketch Version: %s", node_id, payload.c_str());
			if (_tMySensorNode *pNode = FindNode(node_id))
			{
				DatabaseUpdateSketchVersion(node_id, payload);
			}
			else
			{
				//Unknown Node
				InsertNode(node_id);
				DatabaseUpdateSketchVersion(node_id, payload);
			}
			break;
		case I_LOG_MESSAGE:
			break;
		case I_GATEWAY_READY:
			_log.Log(LOG_NORM, "MySensors: Gateway Ready...");
			break;
		case I_TIME:
			//send time in seconds from 1970
			sstr << time(NULL);
			SendCommand(node_id, child_sensor_id, message_type, I_TIME, sstr.str());
			break;
		default:
			while (1==0);
			break;
		}
	}
	else if (message_type == MT_Set)
	{
		_tMySensorNode *pNode = FindNode(node_id);
		if (pNode == NULL)
		{
			pNode = InsertNode(node_id);
			if (pNode == NULL)
				return;
		}
		pNode->lastreceived = mytime(NULL);

		_tMySensorSensor *pSensor = FindSensor(pNode, child_sensor_id, (_eSetType)sub_type);
		if (pSensor == NULL)
		{
			//Unknown sensor, add it to the system
			_tMySensorSensor mSensor;
			mSensor.nodeID = node_id;
			mSensor.childID = child_sensor_id;
			mSensor.devType = (_eSetType)sub_type;
			pNode->m_sensors.push_back(mSensor);
			pSensor = FindSensor(pNode, child_sensor_id, mSensor.devType);
			if (pSensor == NULL)
				return;
		}
		pSensor->lastreceived = mytime(NULL);
		pSensor->devType = (_eSetType)sub_type;
		pSensor->bValidValue = true;
		bool bHaveValue = false;
		switch (sub_type)
		{
		case V_TEMP:
			pSensor->floatValue = (float)atof(payload.c_str());
			bHaveValue = true;
			break;
		case V_HUM:
			pSensor->floatValue = (float)atof(payload.c_str());
			bHaveValue = true;
			break;
		case V_PRESSURE:
			pSensor->floatValue = (float)atof(payload.c_str());
			bHaveValue = true;
			break;
		case V_VAR1: //Custom value
		case V_VAR2:
		case V_VAR3:
		case V_VAR4:
		case V_VAR5:
			//_log.Log(LOG_STATUS, "MySensors: Custom vars ignored! Node: %d, Child: %d, Payload: %s", node_id, child_sensor_id, payload.c_str());
			break;
		case V_TRIPPED:
			//	Tripped status of a security sensor. 1 = Tripped, 0 = Untripped
			pSensor->intvalue = atoi(payload.c_str());
			bHaveValue = true;
			break;
		case V_LIGHT:
			//	Light status. 0 = off 1 = on
			pSensor->intvalue = atoi(payload.c_str());
			bHaveValue = true;
			break;
		case V_DIMMER:
			//	Dimmer value. 0 - 100 %
			pSensor->intvalue = atoi(payload.c_str());
			bHaveValue = true;
			break;
		case V_DUST_LEVEL:
			pSensor->intvalue = atoi(payload.c_str());
			bHaveValue = true;
			break;
		case V_WATT:
			while (1==0);
			break;
		case V_FLOW:
			//Flow of water in meter
			while (1==0);
			break;
		case V_VOLUME:
			//Water Volume
			while (1==0);
			break;
		case V_WIND:
			while (1==0);
			break;
		case V_GUST:
			while (1==0);
			break;
		case V_DIRECTION:
			while (1==0);
			break;
		case V_FORECAST:
			pSensor->stringValue = payload;
			//	Whether forecast.One of "stable", "sunny", "cloudy", "unstable", "thunderstorm" or "unknown"
			pSensor->intvalue = baroForecastNoInfo;
			if (pSensor->stringValue == "sunny")
				pSensor->intvalue = baroForecastSunny;
			else if (pSensor->stringValue == "cloudy")
				pSensor->intvalue = baroForecastCloudy;
			else if (pSensor->stringValue == "thunderstorm")
				pSensor->intvalue = baroForecastRain;
			bHaveValue = true;
			break;
		default:
			if (sub_type > V_CURRENT)
			{
				_log.Log(LOG_ERROR, "MySensors: Unknown/Invalid sensor type (%d)",sub_type);
			}
			else
			{
				_log.Log(LOG_ERROR, "MySensors: Unhandled sensor (sub-type=%d), please report with log!",sub_type);
			}
			break;
		}
		if (bHaveValue)
		{
			SendSensor2Domoticz(pNode, pSensor);
		}
	}
	else if (message_type == MT_Presentation)
	{
		//Ignored
		while (1==0);
	}
	else if (message_type == MT_Req)
	{
		//Request a variable
		switch (sub_type)
		{
		case V_LIGHT:
			SendCommand(node_id, child_sensor_id, message_type, sub_type, "1");
			break;
		case V_VAR1:
		case V_VAR2:
		case V_VAR3:
		case V_VAR4:
		case V_VAR5:
			//cant send back a custom variable
			break;
		default:
			while (1==0);
			break;
		}
		while (1==0);
	}
	else {
		//Unhandled message type
		while (1==0);
	}
}

