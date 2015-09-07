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
#include <boost/lexical_cast.hpp>

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
	result = m_sql.safe_query("SELECT ID, SketchName, SketchVersion FROM MySensors WHERE (HardwareID=%d) ORDER BY ID ASC", m_HwdID);
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
	m_sql.safe_query("INSERT INTO MySensors (HardwareID, ID, SketchName, SketchVersion) VALUES (%d,%d, '%q', '%q')", m_HwdID, nodeID, SketchName.c_str(), SketchVersion.c_str());
}

void MySensorsBase::DatabaseUpdateSketchName(const int nodeID, const std::string &SketchName)
{
	m_sql.safe_query("UPDATE MySensors SET SketchName='%q' WHERE (HardwareID=%d) AND (ID=%d)", SketchName.c_str(), m_HwdID, nodeID);
}

void MySensorsBase::DatabaseUpdateSketchVersion(const int nodeID, const std::string &SketchVersion)
{
	m_sql.safe_query("UPDATE MySensors SET SketchVersion='%q' WHERE (HardwareID=%d) AND (ID=%d)", SketchVersion.c_str(), m_HwdID, nodeID);
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

//Find any sensor with presentation type
MySensorsBase::_tMySensorChild* MySensorsBase::FindSensorWithPresentationType(const int nodeID, const _ePresentationType presType)
{
	std::map<int, _tMySensorNode>::iterator ittNode;
	ittNode = m_nodes.find(nodeID);
	if (ittNode == m_nodes.end())
		return NULL;
	_tMySensorNode *pNode = &ittNode->second;
	std::vector<_tMySensorChild>::iterator itt;
	for (itt = pNode->m_childs.begin(); itt != pNode->m_childs.end(); ++itt)
	{
		if (itt->presType == presType)
			return &*itt;
	}
	return NULL;
}

//Find any sensor with value type
MySensorsBase::_tMySensorChild* MySensorsBase::FindChildWithValueType(const int nodeID, const _eSetType valType)
{
	std::map<int, _tMySensorNode>::iterator ittNode;
	ittNode = m_nodes.find(nodeID);
	if (ittNode == m_nodes.end())
		return NULL;
	_tMySensorNode *pNode = &ittNode->second;
	std::vector<_tMySensorChild>::iterator itt;
	for (itt = pNode->m_childs.begin(); itt != pNode->m_childs.end(); ++itt)
	{
		std::map<_eSetType, _tMySensorValue>::const_iterator itt2;
		for (itt2 = itt->values.begin(); itt2 != itt->values.end(); ++itt2)
		{
			if (itt2->first == valType)
			{
				if (!itt2->second.bValidValue)
					return NULL;
				return &*itt;
			}
		}
	}
	return NULL;
}

void MySensorsBase::UpdateNodeBatteryLevel(const int nodeID, const int Level)
{
	std::map<int, _tMySensorNode>::iterator ittNode = m_nodes.find(nodeID);
	if (ittNode == m_nodes.end())
		return; //Not found
	_tMySensorNode *pNode = &ittNode->second;
	std::vector<_tMySensorChild>::iterator itt;
	for (itt = pNode->m_childs.begin(); itt != pNode->m_childs.end(); ++itt)
	{
		itt->hasBattery = true;
		itt->batValue = Level;
	}
}

void MySensorsBase::MakeAndSendWindSensor(const int nodeID, const std::string &sname)
{
	bool bHaveTemp = false;
	int ChildID = 0;

	float fWind = 0;
	float fGust = 0;
	float fTemp = 0;
	float fChill = 0;
	int iDirection = 0;
	int iBatteryLevel = 255;

	_tMySensorChild *pChild;
	pChild = FindChildWithValueType(nodeID, V_WIND);
	if (!pChild)
		return;
	if (!pChild->GetValue(V_WIND, fWind))
		return;
	fGust = fWind;
	ChildID = pChild->childID;
	iBatteryLevel = pChild->batValue;

	pChild = FindChildWithValueType(nodeID, V_DIRECTION);
	if (!pChild)
		return;
	if (!pChild->GetValue(V_DIRECTION, iDirection))
		return;

	pChild = FindChildWithValueType(nodeID, V_GUST);
	if (pChild)
	{
		if (!pChild->GetValue(V_GUST, fGust))
			return;
	}

	pChild = FindChildWithValueType(nodeID, V_TEMP);
	if (pChild)
	{
		if (!pChild->GetValue(V_TEMP, fTemp))
			return;
		bHaveTemp = true;
		fChill = fTemp;
		if ((fTemp < 10.0) && (fWind >= 1.4))
		{
			fChill = 13.12f + 0.6215f*fTemp - 11.37f*pow(fWind*3.6f, 0.16f) + 0.3965f*fTemp*pow(fWind*3.6f, 0.16f);
		}
	}
	int cNode = (nodeID << 8) | ChildID;
	SendWind(cNode, iBatteryLevel, float(iDirection), fWind, fGust, fTemp, fChill, bHaveTemp, sname);
}

void MySensorsBase::SendSensor2Domoticz(_tMySensorNode *pNode, _tMySensorChild *pChild, const _eSetType vType)
{
	m_iLastSendNodeBatteryValue = 255;
	if (pChild->hasBattery)
	{
		m_iLastSendNodeBatteryValue = pChild->batValue;
	}
	int cNode = (pChild->nodeID << 8) | pChild->childID;
	int intValue;
	float floatValue;
	std::string stringValue;

	switch (vType)
	{
	case V_TEMP:
	{
		float Temp = 0;
		pChild->GetValue(V_TEMP, Temp);
		_tMySensorChild *pChildHum = FindChildWithValueType(pChild->nodeID, V_HUM);
		_tMySensorChild *pChildBaro = FindChildWithValueType(pChild->nodeID, V_PRESSURE);
		if (pChildHum && pChildBaro)
		{
			int Humidity;
			float Baro;
			bool bHaveHumidity = pChildHum->GetValue(V_HUM, Humidity);
			bool bHaveBaro = pChildBaro->GetValue(V_PRESSURE, Baro);
			if (bHaveHumidity && bHaveBaro)
			{
				int forecast = bmpbaroforecast_unknown;
				_tMySensorChild *pSensorForecast = FindChildWithValueType(pChild->nodeID, V_FORECAST);
				if (pSensorForecast)
				{
					pSensorForecast->GetValue(V_FORECAST, forecast);
				}
				if (forecast == bmpbaroforecast_cloudy)
				{
					if (Baro < 1010)
						forecast = bmpbaroforecast_rain;
				}

				//We are using the TempHumBaro Float type now, convert the forecast
				int nforecast = wsbaroforcast_some_clouds;
				if (Baro <= 980)
					nforecast = wsbaroforcast_heavy_rain;
				else if (Baro <= 995)
				{
					if (Temp > 1)
						nforecast = wsbaroforcast_rain;
					else
						nforecast = wsbaroforcast_snow;
					break;
				}
				else if (Baro >= 1029)
					nforecast = wsbaroforcast_sunny;
				switch (forecast)
				{
				case bmpbaroforecast_sunny:
					nforecast = wsbaroforcast_sunny;
					break;
				case bmpbaroforecast_cloudy:
					nforecast = wsbaroforcast_cloudy;
					break;
				case bmpbaroforecast_thunderstorm:
					nforecast = wsbaroforcast_heavy_rain;
					break;
				case bmpbaroforecast_rain:
					if (Temp>1)
						nforecast = wsbaroforcast_rain;
					else
						nforecast = wsbaroforcast_snow;
					break;
				}
				SendTempHumBaroSensorFloat(cNode, pChild->batValue, Temp, Humidity, Baro, nforecast, (!pChild->childName.empty()) ? pChild->childName : "TempHumBaro");
			}
		}
		else if (pChildHum) {
			int Humidity;
			bool bHaveHumidity = pChildHum->GetValue(V_HUM, Humidity);
			if (bHaveHumidity)
			{
				SendTempHumSensor(cNode, pChild->batValue, Temp, Humidity, (!pChild->childName.empty()) ? pChild->childName : "TempHum");
			}
		}
		else
		{
			SendTempSensor(cNode, pChild->batValue, Temp, (!pChild->childName.empty()) ? pChild->childName : "Temp");
		}
	}
	break;
	case V_HUM:
	{
		_tMySensorChild *pChildTemp = FindChildWithValueType(pChild->nodeID, V_TEMP);
		_tMySensorChild *pChildBaro = FindChildWithValueType(pChild->nodeID, V_PRESSURE);
		int forecast = bmpbaroforecast_unknown;
		_tMySensorChild *pSensorForecast = FindChildWithValueType(pChild->nodeID, V_FORECAST);
		if (pSensorForecast)
		{
			pSensorForecast->GetValue(V_FORECAST, forecast);
		}
		if (forecast == bmpbaroforecast_cloudy)
		{
			if (pChildBaro)
			{
				float Baro;
				if (pChildBaro->GetValue(V_PRESSURE, Baro))
				{
					if (Baro < 1010)
						forecast = bmpbaroforecast_rain;
				}
			}
		}
		float Temp;
		float Baro;
		int Humidity;
		pChild->GetValue(V_HUM, Humidity);
		if (pChildTemp && pChildBaro)
		{
			bool bHaveTemp = pChildTemp->GetValue(V_TEMP, Temp);
			bool bHaveBaro = pChildBaro->GetValue(V_PRESSURE, Baro);
			if (bHaveTemp && bHaveBaro)
			{
				cNode = (pChildTemp->nodeID << 8) | pChildTemp->childID;

				//We are using the TempHumBaro Float type now, convert the forecast
				int nforecast = wsbaroforcast_some_clouds;
				if (Baro <= 980)
					nforecast = wsbaroforcast_heavy_rain;
				else if (Baro <= 995)
				{
					if (Temp > 1)
						nforecast = wsbaroforcast_rain;
					else
						nforecast = wsbaroforcast_snow;
					break;
				}
				else if (Baro >= 1029)
					nforecast = wsbaroforcast_sunny;
				switch (forecast)
				{
				case bmpbaroforecast_sunny:
					nforecast = wsbaroforcast_sunny;
					break;
				case bmpbaroforecast_cloudy:
					nforecast = wsbaroforcast_cloudy;
					break;
				case bmpbaroforecast_thunderstorm:
					nforecast = wsbaroforcast_heavy_rain;
					break;
				case bmpbaroforecast_rain:
					if (Temp > 1)
						nforecast = wsbaroforcast_rain;
					else
						nforecast = wsbaroforcast_snow;
					break;
				}

				SendTempHumBaroSensorFloat(cNode, pChildTemp->batValue, Temp, Humidity, Baro, nforecast, (!pChild->childName.empty()) ? pChild->childName : "TempHumBaro");
			}
		}
		else if (pChildTemp) {
			bool bHaveTemp = pChildTemp->GetValue(V_TEMP, Temp);
			if (bHaveTemp)
			{
				cNode = (pChildTemp->nodeID << 8) | pChildTemp->childID;
				SendTempHumSensor(cNode, pChildTemp->batValue, Temp, Humidity, (!pChild->childName.empty()) ? pChild->childName : "TempHum");
			}
		}
		else
		{
			SendHumiditySensor(cNode, pChild->batValue, Humidity);
		}
	}
	break;
	case V_PRESSURE:
	{
		float Baro;
		pChild->GetValue(V_PRESSURE, Baro);
		_tMySensorChild *pSensorTemp = FindChildWithValueType(pChild->nodeID, V_TEMP);
		_tMySensorChild *pSensorHum = FindChildWithValueType(pChild->nodeID, V_HUM);
		int forecast = bmpbaroforecast_unknown;
		_tMySensorChild *pSensorForecast = FindChildWithValueType(pChild->nodeID, V_FORECAST);
		if (pSensorForecast)
		{
			pSensorForecast->GetValue(V_FORECAST, forecast);
		}
		if (forecast == bmpbaroforecast_cloudy)
		{
			if (Baro < 1010)
				forecast = bmpbaroforecast_rain;
		}
		if (pSensorTemp && pSensorHum)
		{
			float Temp;
			int Humidity;
			bool bHaveTemp = pSensorTemp->GetValue(V_TEMP, Temp);
			bool bHaveHumidity = pSensorHum->GetValue(V_HUM, Humidity);

			if (bHaveTemp && bHaveHumidity)
			{
				cNode = (pSensorTemp->nodeID << 8) | pSensorTemp->childID;
				//We are using the TempHumBaro Float type now, convert the forecast
				int nforecast = wsbaroforcast_some_clouds;
				if (Baro <= 980)
					nforecast = wsbaroforcast_heavy_rain;
				else if (Baro <= 995)
				{
					if (Temp > 1)
						nforecast = wsbaroforcast_rain;
					else
						nforecast = wsbaroforcast_snow;
					break;
				}
				else if (Baro >= 1029)
					nforecast = wsbaroforcast_sunny;
				switch (forecast)
				{
				case bmpbaroforecast_sunny:
					nforecast = wsbaroforcast_sunny;
					break;
				case bmpbaroforecast_cloudy:
					nforecast = wsbaroforcast_cloudy;
					break;
				case bmpbaroforecast_thunderstorm:
					nforecast = wsbaroforcast_heavy_rain;
					break;
				case bmpbaroforecast_rain:
					if (Temp > 1)
						nforecast = wsbaroforcast_rain;
					else
						nforecast = wsbaroforcast_snow;
					break;
				}
				SendTempHumBaroSensorFloat(cNode, pSensorTemp->batValue, Temp, Humidity, Baro, nforecast, (!pChild->childName.empty()) ? pChild->childName : "TempHumBaro");
			}
		}
		else
			SendBaroSensor(pChild->nodeID, pChild->childID, pChild->batValue, Baro, forecast);
	}
	break;
	case V_TRIPPED:
		//	Tripped status of a security sensor. 1 = Tripped, 0 = Untripped
		if (pChild->GetValue(vType, intValue))
			UpdateSwitch(pChild->nodeID, pChild->childID, (intValue == 1), 100, "Security Sensor");
		break;
	case V_ARMED:
		//Armed status of a security sensor. 1 = Armed, 0 = Bypassed
		if (pChild->GetValue(vType, intValue))
			UpdateSwitch(pChild->nodeID, pChild->childID, (intValue == 1), 100, "Security Sensor");
		break;
	case V_LOCK_STATUS:
		//Lock status. 1 = Locked, 0 = Unlocked
		if (pChild->GetValue(vType, intValue))
			UpdateSwitch(pChild->nodeID, pChild->childID, (intValue == 1), 100, "Lock Sensor");
		break;
	case V_STATUS:
		//	Light status. 0 = off 1 = on
		if (pChild->GetValue(vType, intValue))
			UpdateSwitch(pChild->nodeID, pChild->childID, (intValue != 0), 100, "Light");
		break;
	case V_SCENE_ON:
		if (pChild->GetValue(vType, intValue))
			UpdateSwitch(pChild->nodeID, pChild->childID + intValue, true, 100, "Scene");
		break;
	case V_SCENE_OFF:
		if (pChild->GetValue(vType, intValue))
			UpdateSwitch(pChild->nodeID, pChild->childID + intValue, false, 100, "Scene");
		break;
	case V_PERCENTAGE:
		//	Dimmer value. 0 - 100 %
		if (pChild->GetValue(vType, intValue))
		{
			int level = intValue;
			UpdateSwitch(pChild->nodeID, pChild->childID, (level != 0), level, "Light");
		}
		break;
	case V_RGB:
		//RRGGBB
		if (pChild->GetValue(vType, intValue))
			SendRGBWSwitch(pChild->nodeID, pChild->childID, pChild->batValue, intValue, false, (!pChild->childName.empty()) ? pChild->childName : "RGB Light");
		break;
	case V_RGBW:
		//RRGGBBWW
		if (pChild->GetValue(vType, intValue))
			SendRGBWSwitch(pChild->nodeID, pChild->childID, pChild->batValue, intValue, true, (!pChild->childName.empty()) ? pChild->childName : "RGBW Light");
		break;
	case V_UP:
	case V_DOWN:
	case V_STOP:
		if (pChild->GetValue(vType, intValue))
			SendBlindSensor(pChild->nodeID, pChild->childID, pChild->batValue, intValue, (!pChild->childName.empty()) ? pChild->childName : "Blinds/Window");
		break;
	case V_LIGHT_LEVEL:
		if (pChild->GetValue(vType, floatValue))
		{
			_tLightMeter lmeter;
			lmeter.id1 = 0;
			lmeter.id2 = 0;
			lmeter.id3 = 0;
			lmeter.id4 = pChild->nodeID;
			lmeter.dunit = pChild->childID;
			lmeter.fLux = floatValue;
			lmeter.battery_level = pChild->batValue;
			if (pChild->hasBattery)
				lmeter.battery_level = pChild->batValue;
			sDecodeRXMessage(this, (const unsigned char *)&lmeter);
		}
		break;
	case V_LEVEL:
		if ((pChild->presType == S_DUST)|| (pChild->presType == S_AIR_QUALITY))
		{
			if (pChild->GetValue(vType, intValue))
			{
				_tAirQualityMeter meter;
				meter.len = sizeof(_tAirQualityMeter) - 1;
				meter.type = pTypeAirQuality;
				meter.subtype = sTypeVoltcraft;
				meter.airquality = intValue;
				meter.id1 = pChild->nodeID;
				meter.id2 = pChild->childID;
				sDecodeRXMessage(this, (const unsigned char *)&meter);
			}
		}
		else if (pChild->presType == S_LIGHT_LEVEL)
		{
			if (pChild->GetValue(vType, intValue))
			{
				_tLightMeter lmeter;
				lmeter.id1 = 0;
				lmeter.id2 = 0;
				lmeter.id3 = 0;
				lmeter.id4 = pChild->nodeID;
				lmeter.dunit = pChild->childID;
				lmeter.fLux = (float)intValue;
				lmeter.battery_level = pChild->batValue;
				if (pChild->hasBattery)
					lmeter.battery_level = pChild->batValue;
				sDecodeRXMessage(this, (const unsigned char *)&lmeter);
			}
		}
		else if (pChild->presType == S_SOUND)
		{
			if (pChild->GetValue(vType, intValue))
				SendSoundSensor(cNode, pChild->batValue, intValue, (!pChild->childName.empty()) ? pChild->childName : "Sound Level");
		}
		else if (pChild->presType == S_MOISTURE)
		{
			if (pChild->GetValue(vType, intValue))
			{
				SendMoistureSensor(cNode, pChild->batValue, intValue, "Moisture");
			}
		}
		break;
	case V_RAIN:
		if (pChild->GetValue(vType, floatValue))
			SendRainSensor(cNode, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Rain");
		break;
	case V_WATT:
		{
			if (pChild->GetValue(vType, floatValue))
			{
				_tMySensorChild *pSensorKwh = pNode->FindChildWithValueType(pChild->childID, V_KWH);// FindChildWithValueType(pChild->nodeID, V_KWH);
				if (pSensorKwh) {
					float Kwh;
					if (pSensorKwh->GetValue(V_KWH, Kwh))
						SendKwhMeter(pSensorKwh->nodeID, pSensorKwh->childID, pSensorKwh->batValue, floatValue / 1000.0f, Kwh, (!pChild->childName.empty()) ? pChild->childName : "Meter");
				}
				else {
					SendWattMeter(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Usage");
				}
			}
		}
		break;
	case V_KWH:
		if (pChild->GetValue(vType, floatValue))
		{
			_tMySensorChild *pSensorWatt = pNode->FindChildWithValueType(pChild->childID, V_WATT);// FindChildWithValueType(pChild->nodeID, V_WATT);
			if (pSensorWatt) {
				float Watt;
				if (pSensorWatt->GetValue(V_WATT, Watt))
					SendKwhMeter(pChild->nodeID, pChild->childID, pChild->batValue, Watt / 1000.0f, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Meter");
			}
			else {
				SendKwhMeter(pChild->nodeID, pChild->childID, pChild->batValue, 0, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Meter");
			}
		}
		break;
	case V_DISTANCE:
		if (pChild->GetValue(vType, floatValue))
			SendDistanceSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue);
		break;
	case V_FLOW:
		//Flow of water/gas in meter (for now send as a percentage sensor)
		if (pChild->GetValue(vType, floatValue))
			SendPercentageSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Water Flow");
		break;
	case V_VOLUME:
		//Water or Gas Volume
		if (pChild->GetValue(vType, floatValue))
		{
			if (pChild->presType == S_WATER)
				SendMeterSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Water");
			else
				SendMeterSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Water");
		}
		break;
	case V_VOLTAGE:
		if (pChild->GetValue(vType, floatValue))
			SendVoltageSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Voltage");
		break;
	case V_UV:
		if (pChild->GetValue(vType, floatValue))
			SendUVSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue);
		break;
	case V_IMPEDANCE:
		if (pChild->GetValue(vType, floatValue))
			SendPercentageSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Impedance");
		break;
	case V_WEIGHT:
		if (pChild->GetValue(vType, floatValue))
		{
			while (1 == 0);
		}
		break;
	case V_CURRENT:
		if (pChild->GetValue(vType, floatValue))
			SendCurrentSensor(cNode, pChild->batValue, floatValue, 0, 0, (!pChild->childName.empty()) ? pChild->childName : "Current");
		break;
	case V_FORECAST:
		if (pChild->GetValue(vType, intValue))
		{
			_tMySensorChild *pSensorBaro = FindChildWithValueType(pChild->nodeID, V_PRESSURE);
			if (pSensorBaro)
			{
				float Baro;
				if (pSensorBaro->GetValue(V_PRESSURE, Baro))
				{
					int forecast = intValue;
					if (forecast == bmpbaroforecast_cloudy)
					{
						if (Baro < 1010)
							forecast = bmpbaroforecast_rain;
					}
					SendBaroSensor(pSensorBaro->nodeID, pSensorBaro->childID, pSensorBaro->batValue, Baro, forecast);
				}
			}
			else
			{
				if (pChild->GetValue(V_FORECAST, stringValue))
				{
					std::stringstream sstr;
					sstr << pChild->nodeID;
					std::string devname = (!pChild->childName.empty()) ? pChild->childName : "Forecast";
					m_sql.UpdateValue(m_HwdID, sstr.str().c_str(), pChild->childID, pTypeGeneral, sTypeTextStatus, 12, pChild->batValue, 0, stringValue.c_str(), devname);
				}
			}
		}
		break;
	case V_WIND:
	case V_GUST:
	case V_DIRECTION:
		MakeAndSendWindSensor(pChild->nodeID, (!pChild->childName.empty()) ? pChild->childName : "Wind");
		break;
	case V_HVAC_SETPOINT_HEAT:
		if (pChild->GetValue(vType, floatValue))
			SendSetPointSensor(pNode->nodeID, pChild->childID, (unsigned char)vType, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Setpoint Heat");
		break;
	case V_HVAC_SETPOINT_COOL:
		if (pChild->GetValue(vType, floatValue))
			SendSetPointSensor(pNode->nodeID, pChild->childID, (unsigned char)vType, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Setpoint Cool");
		break;
	case V_TEXT:
		if (pChild->GetValue(vType, stringValue))
		{
			std::stringstream sstr;
			sstr << cNode;
			std::string devname = (!pChild->childName.empty()) ? pChild->childName : "Text";
			m_sql.UpdateValue(m_HwdID, sstr.str().c_str(), pChild->childID, pTypeGeneral, sTypeTextStatus, 12, pChild->batValue, 0, stringValue.c_str(), devname);
		}
		break;
	case V_IR_RECEIVE:
		if (pChild->GetValue(vType, intValue))
		{
			_tGeneralSwitch gswitch;
			gswitch.subtype = sSwitchTypeMDREMOTE;
			gswitch.id = intValue;
			gswitch.unitcode = pNode->nodeID;
			gswitch.cmnd = gswitch_sOn;
			gswitch.level = 100;
			gswitch.battery_level = pChild->batValue;
			gswitch.rssi = 12;
			gswitch.seqnbr = 0;
			sDecodeRXMessage(this, (const unsigned char *)&gswitch);
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
	double rlevel = (15.0 / 100)*Level;
	int level = int(rlevel);

	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", 0, 0, 0, Idx);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d) AND (Type==%d) AND (Subtype==%d)", m_HwdID, szIdx, SubUnit, int(pTypeLighting2), int(sTypeAC));
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
		{
			//Check Level
			int slevel = atoi(result[0][2].c_str());
			if (slevel==level)
				return;
		}
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
	if (!bOn)
	{
		lcmd.LIGHTING2.cmnd = light2_sOff;
	}
	else
	{
		lcmd.LIGHTING2.cmnd = light2_sOn;
	}
	lcmd.LIGHTING2.level = level;
	lcmd.LIGHTING2.filler = 0;
	lcmd.LIGHTING2.rssi = 12;
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2);

	if (!bDeviceExits)
	{
		//Assign default name for device
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q' WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d) AND (Type==%d) AND (Subtype==%d)", defaultname.c_str(), m_HwdID, szIdx, SubUnit, int(pTypeLighting2), int(sTypeAC));
	}
}

bool MySensorsBase::GetBlindsValue(const int NodeID, const int ChildID, int &blind_value)
{
	char szIdx[10];
	sprintf(szIdx, "%02X%02X%02X", 0, 0, NodeID);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)", m_HwdID, szIdx, ChildID);
	if (result.size() < 1)
		return false;
	blind_value = atoi(result[0][0].c_str());
	return true;
}

bool MySensorsBase::GetSwitchValue(const unsigned char Idx, const int SubUnit, const int sub_type, std::string &sSwitchValue)
{
	char szIdx[10];
	if ((sub_type == V_RGB) || (sub_type == V_RGBW))
	{
		if (Idx==1)
			sprintf(szIdx, "%d", 1);
		else
			sprintf(szIdx, "%08x", Idx);
	}
	else
	{
		sprintf(szIdx, "%X%02X%02X%02X", 0, 0, 0, Idx);
	}
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)", m_HwdID, szIdx, SubUnit);
	if (result.size() < 1)
		return false;
	int nvalue = atoi(result[0][1].c_str());
	if ((sub_type == V_STATUS) || (sub_type == V_TRIPPED))
	{
		sSwitchValue = (nvalue == light2_sOn) ? "1" : "0";
		return true;
	}
	else if ((sub_type == V_RGB) || (sub_type == V_RGBW))
	{
		sSwitchValue = (nvalue == Limitless_LedOn) ? "1" : "0";
		return true;
	}

	int slevel = atoi(result[0][2].c_str());
	std::stringstream sstr;
	sstr << int(slevel * 100 / 15);
	sSwitchValue = sstr.str();
	return true;
}

void MySensorsBase::SendCommand(const int NodeID, const int ChildID, const _eMessageType messageType, const int SubType, const std::string &Payload)
{
	std::stringstream sstr;
	sstr << NodeID << ";" << ChildID << ";" << int(messageType) << ";0;" << SubType << ";" << Payload << '\n';
	WriteInt(sstr.str());
}

bool MySensorsBase::WriteToHardware(const char *pdata, const unsigned char length)
{
	tRBUF *pCmd = (tRBUF *)pdata;
	unsigned char packettype = pCmd->ICMND.packettype;
	unsigned char subtype = pCmd->ICMND.subtype;

	if (packettype == pTypeLighting2)
	{
		//Light command

		int node_id = pCmd->LIGHTING2.id4;
		int child_sensor_id = pCmd->LIGHTING2.unitcode;

		if (_tMySensorNode *pNode = FindNode(node_id))
		{
			int light_command = pCmd->LIGHTING2.cmnd;
			if ((pCmd->LIGHTING2.cmnd == light2_sSetLevel) && (pCmd->LIGHTING2.level == 0))
			{
				light_command = light2_sOff;
			}
			else if ((pCmd->LIGHTING2.cmnd == light2_sSetLevel) && (pCmd->LIGHTING2.level == 255))
			{
				light_command = light2_sOn;
			}

			if ((light_command == light2_sOn) || (light_command == light2_sOff))
			{
				std::string lState = (light_command == light2_sOn) ? "1" : "0";
				if (FindChildWithValueType(node_id, V_LOCK_STATUS) != NULL)
				{
					//Door lock
					SendCommand(node_id, child_sensor_id, MT_Set, V_LOCK_STATUS, lState);
				}
				else if ((FindChildWithValueType(node_id, V_SCENE_ON) != NULL) || (FindChildWithValueType(node_id, V_SCENE_OFF) != NULL))
				{
					//Scene Controller
					SendCommand(node_id, child_sensor_id, MT_Set, (light_command == light2_sOn) ? V_SCENE_ON : V_SCENE_OFF, lState);
				}
				else
				{
					//normal
					SendCommand(node_id, child_sensor_id, MT_Set, V_STATUS, lState);
				}
			}
			else if (light_command == light2_sSetLevel)
			{
				float fvalue = (100.0f / 15.0f)*float(pCmd->LIGHTING2.level);
				if (fvalue > 100.0f)
					fvalue = 100.0f; //99 is fully on
				int svalue = round(fvalue);

				std::stringstream sstr;
				sstr << svalue;
				SendCommand(node_id, child_sensor_id, MT_Set, V_PERCENTAGE, sstr.str());
			}
		}
		else {
			_log.Log(LOG_ERROR, "MySensors: Light command received for unknown node_id: %d", node_id);
			return false;
		}
	}
	else if (packettype == pTypeLimitlessLights)
	{
		//RGW/RGBW command
		_tLimitlessLights *pLed = (_tLimitlessLights *)pdata;
		unsigned char ID1 = (unsigned char)((pLed->id & 0xFF000000) >> 24);
		unsigned char ID2 = (unsigned char)((pLed->id & 0x00FF0000) >> 16);
		unsigned char ID3 = (unsigned char)((pLed->id & 0x0000FF00) >> 8);
		unsigned char ID4 = (unsigned char)pLed->id & 0x000000FF;

		int node_id = (ID3 << 8) | ID4;
		int child_sensor_id = pLed->dunit;

		if (_tMySensorNode *pNode = FindNode(node_id))
		{
			bool bIsRGBW = (pNode->FindChildWithPresentationType(child_sensor_id, S_RGBW_LIGHT) != NULL);
			if (pLed->command == Limitless_SetRGBColour)
			{
				int red, green, blue;

				float cHue = (360.0f / 255.0f)*float(pLed->value);//hue given was in range of 0-255
				int Brightness = 100;
				int dMax = round((255.0f / 100.0f)*float(Brightness));
				hue2rgb(cHue, red, green, blue, dMax);
				std::stringstream sstr;
				sstr << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << red
					<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << green
					<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << blue;
				SendCommand(node_id, child_sensor_id, MT_Set, (bIsRGBW == true) ? V_RGBW : V_RGB, sstr.str());
			}
			else if (pLed->command == Limitless_SetColorToWhite)
			{
				std::stringstream sstr;
				int Brightness = 100;
				int wWhite = round((255.0f / 100.0f)*float(Brightness));
				if (!bIsRGBW)
				{
					sstr << std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << wWhite
						<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << wWhite
						<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << wWhite;
				}
				else
				{
					sstr << "#000000"
						<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << wWhite;
				}
				SendCommand(node_id, child_sensor_id, MT_Set, (bIsRGBW == true) ? V_RGBW : V_RGB, sstr.str());
			}
			else if (pLed->command == Limitless_SetBrightnessLevel)
			{
				float fvalue = pLed->value;
				int svalue = round(fvalue);
				if (svalue > 100)
					svalue = 100;
				std::stringstream sstr;
				sstr << svalue;
				SendCommand(node_id, child_sensor_id, MT_Set, V_PERCENTAGE, sstr.str());
			}
			else if ((pLed->command == Limitless_LedOff) || (pLed->command == Limitless_LedOn))
			{
				std::string lState = (pLed->command == Limitless_LedOn) ? "1" : "0";
				SendCommand(node_id, child_sensor_id, MT_Set, V_STATUS, lState);
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "MySensors: Light command received for unknown node_id: %d", node_id);
			return false;
		}
	}
	else if (packettype == pTypeBlinds)
	{
		//Blinds/Window command
		int node_id = pCmd->BLINDS1.id3;
		int child_sensor_id = pCmd->BLINDS1.unitcode;

		if (_tMySensorNode *pNode = FindNode(node_id))
		{
			if (pCmd->BLINDS1.cmnd == blinds_sOpen)
			{
				SendCommand(node_id, child_sensor_id, MT_Set, V_UP, "");
			}
			else if (pCmd->BLINDS1.cmnd == blinds_sClose)
			{
				SendCommand(node_id, child_sensor_id, MT_Set, V_DOWN, "");
			}
			else if (pCmd->BLINDS1.cmnd == blinds_sStop)
			{
				SendCommand(node_id, child_sensor_id, MT_Set, V_STOP, "");
			}
		}
		else {
			_log.Log(LOG_ERROR, "MySensors: Blinds/Window command received for unknown node_id: %d", node_id);
			return false;
		}
	}
	else if ((packettype == pTypeThermostat) && (subtype == sTypeThermSetpoint))
	{
		//Set Point
		_tThermostat *pMeter = (_tThermostat *)pCmd;

		int node_id = pMeter->id2;
		int child_sensor_id = pMeter->id3;
		int vtype_id = pMeter->id4;

		//Seems MySensors setpoints are integers?

		std::stringstream sstr;
		sstr << round(pMeter->temp);

		SendCommand(node_id, child_sensor_id, MT_Set, vtype_id, sstr.str());
	}
	else if (packettype == pTypeGeneralSwitch)
	{
		//Used to store IR codes
		_tGeneralSwitch *pSwitch=(_tGeneralSwitch *)pCmd;

		int node_id = pSwitch->unitcode;
		unsigned int ir_code = pSwitch->id;

		if (_tMySensorNode *pNode = FindNode(node_id))
		{
			_tMySensorChild* pChild = pNode->FindChildByValueType(V_IR_RECEIVE);
			if (pChild)
			{
				std::stringstream sstr;
				sstr << ir_code;
				SendCommand(node_id, pChild->childID, MT_Set, V_IR_SEND, sstr.str());
			}
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "MySensors: Unknown action received");
		return false;
	}
	return true;
}

void MySensorsBase::UpdateVar(const int NodeID, const int ChildID, const int VarID, const std::string &svalue)
{
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ROWID FROM MySensorsVars WHERE (HardwareID=%d) AND (NodeID=%d) AND (ChildID=%d) AND (VarID=%d)", m_HwdID, NodeID, ChildID, VarID);
	if (result.size() == 0)
	{
		//Insert
		m_sql.safe_query("INSERT INTO MySensorsVars (HardwareID, NodeID, ChildID, VarID, [Value]) VALUES (%d, %d, %d, %d,'%q')", m_HwdID, NodeID, ChildID, VarID, svalue.c_str());
	}
	else
	{
		//Update
		m_sql.safe_query("UPDATE MySensorsVars SET [Value]='%q' WHERE (ROWID = '%q')", svalue.c_str(), result[0][0].c_str());
	}

}

bool MySensorsBase::GetVar(const int NodeID, const int ChildID, const int VarID, std::string &sValue)
{
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT [Value] FROM MySensorsVars WHERE (HardwareID=%d) AND (NodeID=%d) AND (ChildID=%d) AND (VarID=%d)", m_HwdID, NodeID, ChildID, VarID);
	if (result.size() < 1)
		return false;
	std::vector<std::string> sd = result[0];
	sValue = sd[0];
	return true;
}

void MySensorsBase::UpdateChildDBInfo(const int NodeID, const int ChildID, const _ePresentationType pType, const std::string &Name, const bool UseAck)
{
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ROWID FROM MySensorsChilds WHERE (HardwareID=%d) AND (NodeID=%d) AND (ChildID=%d)", m_HwdID, NodeID, ChildID);
	if (result.size() < 1)
	{
		//Insert
		m_sql.safe_query("INSERT INTO MySensorsChilds (HardwareID, NodeID, ChildID, [Type], [Name], UseAck) VALUES (%d, %d, %d, %d, '%q', %d)", m_HwdID, NodeID, ChildID, pType, Name.c_str(), (UseAck) ? 1 : 0);
	}
	else
	{
		//Update
		m_sql.safe_query("UPDATE MySensorsChilds SET [Type]='%d', [Name]='%q', [UseAck]='%d' WHERE (ROWID = '%q')", pType, Name.c_str(), (UseAck) ? 1 : 0, result[0][0].c_str());
	}
}

bool MySensorsBase::GetChildDBInfo(const int NodeID, const int ChildID, _ePresentationType &pType, std::string &Name, bool &UseAck)
{
	pType = S_UNKNOWN;
	Name = "";
	UseAck = false;
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT [Type], [Name], [UseAck] FROM MySensorsChilds WHERE (HardwareID=%d) AND (NodeID=%d) AND (ChildID=%d)", m_HwdID, NodeID, ChildID);
	if (result.size() < 1)
		return false;
	pType = (_ePresentationType)atoi(result[0][0].c_str());
	Name = result[0][1];
	UseAck = (atoi(result[0][2].c_str()) != 0);
	return true;
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

	std::stringstream sstr;
#ifdef _DEBUG
	_log.Log(LOG_NORM, "MySensors: NodeID: %d, ChildID: %d, MessageType: %d, Ack: %d, SubType: %d, Payload: %s",node_id,child_sensor_id,message_type,ack,sub_type,payload.c_str());
#endif

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
		case I_BATTERY_LEVEL:
			UpdateNodeBatteryLevel(node_id, atoi(payload.c_str()));
			break;
		case I_LOG_MESSAGE:
			//_log.Log(LOG_NORM, "MySensors: 'Log': %s", payload.c_str());
			break;
		case I_GATEWAY_READY:
			_log.Log(LOG_NORM, "MySensors: Gateway Ready...");
			break;
		case I_TIME:
			//send time in seconds from 1970 with timezone offset
			{
				boost::posix_time::ptime tlocal(boost::posix_time::second_clock::local_time());
				boost::posix_time::time_duration dur = tlocal - boost::posix_time::ptime(boost::gregorian::date(1970, 1, 1));
				time_t fltime(dur.total_seconds());
				sstr << fltime;
				SendCommand(node_id, child_sensor_id, message_type, I_TIME, sstr.str());
			}
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

		_tMySensorChild *pChild = pNode->FindChild(child_sensor_id);
		if (pChild == NULL)
		{
			//Unknown sensor, add it to the system
			_tMySensorChild mSensor;
			mSensor.nodeID = node_id;
			mSensor.childID = child_sensor_id;
			//Get Info from database if child already existed
			GetChildDBInfo(node_id, child_sensor_id, mSensor.presType, mSensor.childName, mSensor.useAck);
			pNode->m_childs.push_back(mSensor);
			pChild = pNode->FindChild(child_sensor_id);
			if (pChild == NULL)
				return;
		}
		pChild->lastreceived = mytime(NULL);

		_eSetType vType = (_eSetType)sub_type;
		bool bHaveValue = false;
		switch (vType)
		{
		case V_TEMP:
			pChild->SetValue(vType, (float)atof(payload.c_str()));
			bHaveValue = true;
			break;
		case V_HUM:
			pChild->SetValue(vType, atoi(payload.c_str()));
			bHaveValue = true;
			break;
		case V_PRESSURE:
			pChild->SetValue(vType, (float)atof(payload.c_str()));
			bHaveValue = true;
			break;
		case V_VAR1: //Custom value
		case V_VAR2:
		case V_VAR3:
		case V_VAR4:
		case V_VAR5:
			UpdateVar(node_id, child_sensor_id, sub_type, payload);
			break;
		case V_TRIPPED:
			//	Tripped status of a security sensor. 1 = Tripped, 0 = Untripped
			pChild->SetValue(vType, atoi(payload.c_str()));
			bHaveValue = true;
			break;
		case V_ARMED:
			pChild->SetValue(vType, atoi(payload.c_str()));
			bHaveValue = true;
			break;
		case V_LOCK_STATUS:
			pChild->SetValue(vType, atoi(payload.c_str()));
			bHaveValue = true;
			break;
		case V_STATUS:
			//	Light status. 0 = off 1 = on
			pChild->SetValue(vType, atoi(payload.c_str()));
			bHaveValue = true;
			break;
		case V_SCENE_ON:
			//	Scene On
			pChild->SetValue(vType, atoi(payload.c_str()));
			bHaveValue = true;
			break;
		case V_SCENE_OFF:
			//	Scene Off
			pChild->SetValue(vType, atoi(payload.c_str()));
			bHaveValue = true;
			break;
		case V_RGB:
			pChild->SetValue(vType, atoi(payload.c_str()));
			bHaveValue = true;
			break;
		case V_RGBW:
			pChild->SetValue(vType, atoi(payload.c_str()));
			bHaveValue = true;
			break;
		case V_PERCENTAGE:
			//	Dimmer value. 0 - 100 %
			pChild->SetValue(vType, atoi(payload.c_str()));
			bHaveValue = true;
			break;
		case V_UP:
			pChild->SetValue(vType, int(blinds_sOpen));
			bHaveValue = true;
			break;
		case V_DOWN:
			pChild->SetValue(vType, int(blinds_sClose));
			bHaveValue = true;
			break;
		case V_STOP:
			pChild->SetValue(vType, int(blinds_sStop));
			bHaveValue = true;
			break;
		case V_LEVEL:
			pChild->SetValue(vType, atoi(payload.c_str()));
			pChild->SetValue(vType, (float)atof(payload.c_str()));
			bHaveValue = true;
			break;
		case V_RAIN:
			pChild->SetValue(vType, (float)atof(payload.c_str()));
			bHaveValue = true;
			break;
		case V_WATT:
			pChild->SetValue(vType, (float)atof(payload.c_str()));
			bHaveValue = true;
			break;
		case V_KWH:
			pChild->SetValue(vType, (float)atof(payload.c_str()));
			bHaveValue = true;
			break;
		case V_DISTANCE:
			pChild->SetValue(vType, (float)atof(payload.c_str()));
			bHaveValue = true;
			break;
		case V_FLOW:
			//Flow of water in meter
			pChild->SetValue(vType, (float)atof(payload.c_str()));
			bHaveValue = true;
			break;
		case V_VOLUME:
			//Water Volume
			pChild->SetValue(vType, (float)atof(payload.c_str()));
			bHaveValue = true;
			break;
		case V_WIND:
			pChild->SetValue(vType, (float)atof(payload.c_str()));
			bHaveValue = true;
			break;
		case V_GUST:
			pChild->SetValue(vType, (float)atof(payload.c_str()));
			bHaveValue = true;
			break;
		case V_DIRECTION:
			pChild->SetValue(vType, round(atof(payload.c_str())));
			bHaveValue = true;
			break;
		case V_LIGHT_LEVEL:
			pChild->SetValue(vType, (float)atof(payload.c_str()));
			/*
			//convert percentage to 1000 scale
			pSensor->floatValue = (1000.0f / 100.0f)*pSensor->floatValue;
			if (pSensor->floatValue > 1000.0f)
				pSensor->floatValue = 1000.0f;
			*/
			bHaveValue = true;
			break;
		case V_FORECAST:
			pChild->SetValue(vType, payload);
			//	Whether forecast.One of "stable", "sunny", "cloudy", "unstable", "thunderstorm" or "unknown"
			pChild->SetValue(vType, int(bmpbaroforecast_unknown));
			if (payload == "stable")
				pChild->SetValue(vType, int(bmpbaroforecast_stable));
			else if (payload == "sunny")
				pChild->SetValue(vType, int(bmpbaroforecast_sunny));
			else if (payload == "cloudy")
				pChild->SetValue(vType, int(bmpbaroforecast_cloudy));
			else if (payload == "unstable")
				pChild->SetValue(vType, int(bmpbaroforecast_unstable));
			else if (payload == "thunderstorm")
				pChild->SetValue(vType, int(bmpbaroforecast_thunderstorm));
			else if (payload == "unknown")
				pChild->SetValue(vType, int(bmpbaroforecast_unknown));
			bHaveValue = true;
			break;
		case V_VOLTAGE:
			pChild->SetValue(vType, (float)atof(payload.c_str()));
			bHaveValue = true;
			break;
		case V_UV:
			pChild->SetValue(vType, (float)atof(payload.c_str()));
			bHaveValue = true;
			break;
		case V_IMPEDANCE:
			pChild->SetValue(vType, (float)atof(payload.c_str()));
			bHaveValue = true;
			break;
		case V_WEIGHT:
			pChild->SetValue(vType, (float)atof(payload.c_str()));
			bHaveValue = true;
			break;
		case V_CURRENT:
			pChild->SetValue(vType, (float)atof(payload.c_str()));
			bHaveValue = true;
			break;
		case V_HVAC_SETPOINT_HEAT:
		case V_HVAC_SETPOINT_COOL:
			pChild->SetValue(vType, (float)atof(payload.c_str()));
			bHaveValue = true;
			break;
		case V_TEXT:
			pChild->SetValue(vType, payload);
			bHaveValue = true;
			break;
		case V_IR_RECEIVE:
			pChild->SetValue(vType, (int)boost::lexical_cast<unsigned int>(payload));
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
			SendSensor2Domoticz(pNode, pChild, vType);
		}
	}
	else if (message_type == MT_Presentation)
	{
		//Ignored for now
		if ((node_id == 255) || (child_sensor_id == 255))
			return;

		bool bDoAdd = false;

		_ePresentationType pType = (_ePresentationType)sub_type;
		_eSetType vType = V_UNKNOWN;

		switch (pType)
		{
		case S_DOOR:
		case S_MOTION:
			vType = V_TRIPPED;
			bDoAdd = true;
			break;
		case S_SMOKE:
			vType = V_ARMED;
			bDoAdd = true;
			break;
		case S_TEMP:
			vType = V_TEMP;
			bDoAdd = true;
			break;
		case S_HUM:
			vType = V_HUM;
			bDoAdd = true;
			break;
		case S_BARO:
			vType = V_PRESSURE;
			bDoAdd = true;
			break;
		case S_LOCK:
			vType = V_LOCK_STATUS;
			bDoAdd = true;
			break;
		case S_LIGHT:
			vType = V_STATUS;
			bDoAdd = true;
			break;
		case S_DIMMER:
			vType = V_PERCENTAGE;
			bDoAdd = true;
			break;
		case S_SCENE_CONTROLLER:
			vType = V_SCENE_ON;
			bDoAdd = true;
			break;
		case S_COVER:
			vType = V_UP;
			bDoAdd = true;
			break;
		case S_RGB_LIGHT:
		case S_COLOR_SENSOR:
			vType = V_RGB;
			bDoAdd = true;
			break;
		case S_RGBW_LIGHT:
			vType = V_RGBW;
			bDoAdd = true;
			break;
		}
		_tMySensorNode *pNode = FindNode(node_id);
		if (pNode == NULL)
		{
			pNode = InsertNode(node_id);
			if (pNode == NULL)
				return;
		}
		pNode->lastreceived = mytime(NULL);
		bool bUseAck = (ack != 0);
		_tMySensorChild *pSensor = pNode->FindChild(child_sensor_id);
		if (pSensor == NULL)
		{
			//Unknown sensor, add it to the system
			_tMySensorChild mSensor;
			mSensor.nodeID = node_id;
			mSensor.childID = child_sensor_id;
			pNode->m_childs.push_back(mSensor);
			pSensor = pNode->FindChild(child_sensor_id);
			if (pSensor == NULL)
				return;
		}
		bool bDiffPresentation = (
			(pSensor->presType != pType) ||
			(pSensor->childName != payload) ||
			(pSensor->useAck != bUseAck)
			);
		pSensor->lastreceived = mytime(NULL);
		pSensor->presType = pType;
		pSensor->childName = payload;
		pSensor->useAck = bUseAck;

		if (bDiffPresentation)
		{
			UpdateChildDBInfo(node_id, child_sensor_id, pType, pSensor->childName, pSensor->useAck);
		}

		if (!bDoAdd)
			return;

		if ((vType == V_STATUS) || (vType == V_RGB) || (vType == V_RGBW) || (vType == V_TRIPPED) || (vType == V_PERCENTAGE) || (vType == V_LOCK_STATUS))
		{
			//Check if switch is already in the system, if not add it
			std::string sSwitchValue;
			if (!GetSwitchValue(node_id, child_sensor_id, vType, sSwitchValue))
			{
				//Add it to the system
				if ((vType == V_STATUS) || (vType == V_PERCENTAGE) || (vType == V_LOCK_STATUS))
					UpdateSwitch(node_id, child_sensor_id, false, 100, "Light");
				else if (vType == V_TRIPPED)
					UpdateSwitch(node_id, child_sensor_id, false, 100, "Security Sensor");
				else
					SendRGBWSwitch(node_id, child_sensor_id, 255, 0, (vType == V_RGBW), (vType == V_RGBW) ? "RGBW Light" : "RGB Light");
			}
		}
		else if (vType == V_UP)
		{
			int blind_value;
			if (!GetBlindsValue(node_id, child_sensor_id, blind_value))
			{
				SendBlindSensor(node_id, child_sensor_id, 255, blinds_sOpen, "Blinds/Window");
			}
		}
	}
	else if (message_type == MT_Req)
	{
		//Request a variable
		std::string tmpstr;
		switch (sub_type)
		{
		case V_STATUS:
		case V_PERCENTAGE:
		case V_RGB:
		case V_RGBW:
			if (GetSwitchValue(node_id, child_sensor_id, sub_type, tmpstr))
				SendCommand(node_id, child_sensor_id, message_type, sub_type, tmpstr);
			break;
		case V_VAR1:
		case V_VAR2:
		case V_VAR3:
		case V_VAR4:
		case V_VAR5:
			//send back a previous stored custom variable
			tmpstr = "";
			GetVar(node_id, child_sensor_id, sub_type, tmpstr);
			SendCommand(node_id, child_sensor_id, message_type, sub_type, tmpstr);
			break;
		case V_TEXT:
			{
				//Get Text sensor value from the database
				int cNode = (node_id << 8) | child_sensor_id;
				std::stringstream sstr;
				sstr << cNode;
				tmpstr = "";
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d)",
					m_HwdID, sstr.str().c_str(), pTypeGeneral, sTypeTextStatus);
				if (!result.empty())
				{
					tmpstr = result[0][0];
				}
				SendCommand(node_id, child_sensor_id, message_type, sub_type, tmpstr);
			}
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

