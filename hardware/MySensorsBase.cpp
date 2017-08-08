#include "stdafx.h"
#include "MySensorsBase.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "../main/WebServer.h"
#include "../main/mainworker.h"
#include "hardwaretypes.h"
#include <string>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "../webserver/cWebem.h"
#include "../json/json.h"

#include <ctime>

#define round(a) ( int ) ( a + .5 )

std::string MySensorsBase::GetMySensorsValueTypeStr(const enum _eSetType vType)
{
	switch (vType)
	{
	case V_TEMP:
		return "V_TEMP";
	case V_HUM:
		return "V_HUM";
	case V_STATUS: //V_LIGHT
		return "V_STATUS";
	case V_PERCENTAGE: //V_DIMMER
		return "V_PERCENTAGE";
	case V_PRESSURE:
		return "V_PRESSURE";
	case V_FORECAST:
		return "V_FORECAST";
	case V_RAIN:
		return "V_RAIN";
	case V_RAINRATE:
		return "V_RAINRATE";
	case V_WIND:
		return "V_WIND";
	case V_GUST:
		return "V_GUST";
	case V_DIRECTION:
		return "V_DIRECTION";
	case V_UV:
		return "V_UV";
	case V_WEIGHT:
		return "V_WEIGHT";
	case V_DISTANCE:
		return "V_DISTANCE";
	case V_IMPEDANCE:
		return "V_IMPEDANCE";
	case V_ARMED:
		return "V_ARMED";
	case V_TRIPPED:
		return "V_TRIPPED";
	case V_WATT:
		return "V_WATT";
	case V_KWH:
		return "V_KWH";
	case V_SCENE_ON:
		return "V_SCENE_ON";
	case V_SCENE_OFF:
		return "V_SCENE_OFF";
	case V_HVAC_FLOW_STATE:
		return "V_HVAC_FLOW_STATE";
	case V_HVAC_SPEED:
		return "V_HVAC_SPEED";
	case V_LIGHT_LEVEL:
		return "V_LIGHT_LEVEL";
	case V_VAR1:
		return "V_VAR1";
	case V_VAR2:
		return "V_VAR2";
	case V_VAR3:
		return "V_VAR3";
	case V_VAR4:
		return "V_VAR4";
	case V_VAR5:
		return "V_VAR5";
	case V_UP:
		return "V_UP";
	case V_DOWN:
		return "V_DOWN";
	case V_STOP:
		return "V_STOP";
	case V_IR_SEND:
		return "V_IR_SEND";
	case V_IR_RECEIVE:
		return "V_IR_RECEIVE";
	case V_FLOW:
		return "V_FLOW";
	case V_VOLUME:
		return "V_VOLUME";
	case V_LOCK_STATUS:
		return "V_LOCK_STATUS";
	case V_LEVEL:
		return "V_LEVEL";
	case V_VOLTAGE:
		return "V_VOLTAGE";
	case V_CURRENT:
		return "V_CURRENT";
	case V_RGB:
		return "V_RGB";
	case V_RGBW:
		return "V_RGBW";
	case V_ID:
		return "V_ID";
	case V_UNIT_PREFIX:
		return "V_UNIT_PREFIX";
	case V_HVAC_SETPOINT_COOL:
		return "V_HVAC_SETPOINT_COOL";
	case V_HVAC_SETPOINT_HEAT:
		return "V_HVAC_SETPOINT_HEAT";
	case V_HVAC_FLOW_MODE:
		return "V_HVAC_FLOW_MODE";
	case V_TEXT:
		return "V_TEXT";
	case V_CUSTOM:
		return "V_CUSTOM";
	case V_POSITION:
		return "V_POSITION";
	case V_IR_RECORD:
		return "V_IR_RECORD";
	case V_PH:
		return "V_PH";
	case V_ORP:
		return "V_ORP";
	case V_EC:
		return "V_EC";
	case V_VAR:
		return "V_VAR";
	case V_VA:
		return "V_VA";
	case V_POWER_FACTOR:
		return "V_POWER_FACTOR";
	default:
		return "V_UNKNOWN";

	}
	return "Unknown!";
}

std::string MySensorsBase::GetMySensorsPresentationTypeStr(const enum _ePresentationType pType)
{
	switch (pType)
	{
	case S_DOOR:
		return "S_DOOR";
	case S_MOTION:
		return "S_MOTION";
	case S_SMOKE:
		return "S_SMOKE";
	case S_LIGHT:
		return "S_LIGHT/S_BINARY";
	case S_DIMMER:
		return "S_DIMMER";
	case S_COVER:
		return "S_COVER";
	case S_TEMP:
		return "S_TEMP";
	case S_HUM:
		return "S_HUM";
	case S_BARO:
		return "S_BARO";
	case S_WIND:
		return "S_WIND";
	case S_RAIN:
		return "S_RAIN";
	case S_UV:
		return "S_UV";
	case S_WEIGHT:
		return "S_WEIGHT";
	case S_POWER:
		return "S_POWER";
	case S_HEATER:
		return "S_HEATER";
	case S_DISTANCE:
		return "S_DISTANCE";
	case S_LIGHT_LEVEL:
		return "S_LIGHT_LEVEL";
	case S_ARDUINO_NODE:
		return "S_ARDUINO_NODE";
	case S_ARDUINO_REPEATER_NODE:
		return "S_ARDUINO_REPEATER_NODE";
	case S_LOCK:
		return "S_LOCK";
	case S_IR:
		return "S_IR";
	case S_WATER:
		return "S_WATER";
	case S_AIR_QUALITY:
		return "S_AIR_QUALITY";
	case S_CUSTOM:
		return "S_CUSTOM";
	case S_DUST:
		return "S_DUST";
	case S_SCENE_CONTROLLER:
		return "S_SCENE_CONTROLLER";
	case S_RGB_LIGHT:
		return "S_RGB_LIGHT";
	case S_RGBW_LIGHT:
		return "S_RGBW_LIGHT";
	case S_COLOR_SENSOR:
		return "S_COLOR_SENSOR";
	case S_HVAC:
		return "S_HVAC";
	case S_MULTIMETER:
		return "S_MULTIMETER";
	case S_SPRINKLER:
		return "S_SPRINKLER";
	case S_WATER_LEAK:
		return "S_WATER_LEAK";
	case S_SOUND:
		return "S_SOUND";
	case S_VIBRATION:
		return "S_VIBRATION";
	case S_MOISTURE:
		return "S_MOISTURE";
	case S_INFO:
		return "S_INFO";
	case S_GAS:
		return "S_GAS";
	case S_GPS:
		return "S_GPS";
	case S_WATER_QUALITY:
		return "S_WATER_QUALITY";
	case S_UNKNOWN:
		return "S_UNKNOWN";
	}
	return "Unknown!";
}

MySensorsBase::MySensorsBase(void):
m_GatewayVersion("?")
{
	m_bufferpos = 0;
	m_bAckReceived = false;
	m_AckNodeID = -1;
	m_AckChildID = -1;
	m_AckSetType = V_UNKNOWN;
}

MySensorsBase::~MySensorsBase(void)
{
}

void MySensorsBase::LoadDevicesFromDatabase()
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	m_nodes.clear();

	std::vector<std::vector<std::string> > result,result2;
	result = m_sql.safe_query("SELECT ID, Name, SketchName, SketchVersion FROM MySensors WHERE (HardwareID=%d) ORDER BY ID ASC", m_HwdID);
	if (result.size() > 0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;

			int ID = atoi(sd[0].c_str());
			std::string Name = sd[1];
			std::string SkectName = sd[2];
			std::string SkectVersion = sd[3];

			_tMySensorNode mNode;
			mNode.nodeID = ID;
			mNode.Name = Name;
			mNode.SketchName = SkectName;
			mNode.SketchVersion = SkectVersion;
			mNode.lastreceived = 0;
			//Load the Childs
			result2 = m_sql.safe_query("SELECT ChildID, [Type], [Name], UseAck, AckTimeout FROM MySensorsChilds WHERE (HardwareID=%d) AND (NodeID=%d) ORDER BY ChildID ASC", m_HwdID, ID);
			if (result2.size() > 0)
			{
				int gID = 1;
				std::vector<std::vector<std::string> >::const_iterator itt2;
				for (itt2 = result2.begin(); itt2 != result2.end(); ++itt2)
				{
					std::vector<std::string> sd2 = *itt2;
					_tMySensorChild mSensor;
					mSensor.nodeID = ID;
					mSensor.childID = atoi(sd2[0].c_str());
					mSensor.presType = (_ePresentationType)atoi(sd2[1].c_str());
					std::vector<_tMySensorChild>::iterator itt3;
					for (itt3 = mNode.m_childs.begin(); itt3 != mNode.m_childs.end(); ++itt3)
					{
						if ((itt3->presType == mSensor.presType) && (itt3->groupID == gID))
							gID++;
					}
					mSensor.groupID = gID;
					mSensor.childName = sd2[2];
					mSensor.useAck = atoi(sd2[3].c_str()) != 0;
					mSensor.ackTimeout = atoi(sd2[4].c_str());
					mNode.m_childs.push_back(mSensor);
				}
			}
			m_nodes[ID] = mNode;
		}
	}
}

void MySensorsBase::Add2Database(const int nodeID, const std::string &SketchName, const std::string &SketchVersion)
{
	m_sql.safe_query("INSERT INTO MySensors (HardwareID, ID, Name, SketchName, SketchVersion) VALUES (%d,%d, '%q', '%q', '%q')", m_HwdID, nodeID, SketchName.c_str(), SketchName.c_str(), SketchVersion.c_str());
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
	mNode.Name = "Unknown";
	mNode.SketchName = "Unknown";
	mNode.SketchVersion = "1.0";
	mNode.lastreceived = 0;
	m_nodes[mNode.nodeID] = mNode;
	Add2Database(mNode.nodeID, mNode.SketchName, mNode.SketchVersion);
	return FindNode(nodeID);
}

void MySensorsBase::UpdateNode(const int nodeID, const std::string &name)
{
	if (_tMySensorNode *pNode = FindNode(nodeID))
	{
		m_sql.safe_query("UPDATE MySensors SET [Name]='%q' WHERE (HardwareID==%d) AND (ID=='%d')", name.c_str(), m_HwdID, nodeID);
		pNode->Name = name;
	}
	else
	{
		_log.Log(LOG_ERROR, "MySensors: Update command received for unknown node_id: %d", nodeID);
	}
}

void MySensorsBase::RemoveNode(const int nodeID)
{
	m_sql.safe_query("DELETE FROM MySensors WHERE (HardwareID==%d) AND (ID==%d)", m_HwdID, nodeID);
	m_sql.safe_query("DELETE FROM MySensorsChilds WHERE (HardwareID==%d) AND (NodeID=='%d')",m_HwdID, nodeID);
}

void MySensorsBase::RemoveChild(const int nodeID, const int childID)
{
	m_sql.safe_query("DELETE FROM MySensorsChilds WHERE (HardwareID==%d) AND (NodeID=='%d') AND (ChildID=='%d')", m_HwdID, nodeID, childID);
}

void MySensorsBase::UpdateChild(const int nodeID, const int childID, const bool UseAck, const int AckTimeout)
{
	if (_tMySensorNode *pNode = FindNode(nodeID))
	{
		m_sql.safe_query("UPDATE MySensorsChilds SET [UseAck]='%d', [AckTimeout]='%d' WHERE (HardwareID==%d) AND (NodeID=='%d') AND (ChildID=='%d')", (UseAck == true) ? 1 : 0, AckTimeout, m_HwdID, nodeID, childID);
		_tMySensorChild *pChild = pNode->FindChild(childID);
		if (pChild)
		{
			pChild->useAck = UseAck;
			pChild->ackTimeout = AckTimeout;
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "MySensors: Update command received for unknown node_id: %d, child_id: %d", nodeID, childID);
	}
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
MySensorsBase::_tMySensorChild* MySensorsBase::FindChildWithValueType(const int nodeID, const _eSetType valType, const int groupID)
{
	std::map<int, _tMySensorNode>::iterator ittNode;
	ittNode = m_nodes.find(nodeID);
	if (ittNode == m_nodes.end())
		return NULL;
	_tMySensorNode *pNode = &ittNode->second;
	std::vector<_tMySensorChild>::iterator itt;
	for (itt = pNode->m_childs.begin(); itt != pNode->m_childs.end(); ++itt)
	{
		if ((itt->groupID == groupID) || (groupID == 0))
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

		//Uncomment the below to for a sensor update
/*
		std::map<_eSetType, _tMySensorValue>::const_iterator itt2;
		for (itt2 = itt->values.begin(); itt2 != itt->values.end(); ++itt2)
		{
			if (itt2->second.bValidValue)
			{
				_eSetType vType = itt2->first;
				SendSensor2Domoticz(pNode, &(*itt), vType);
			}
		}
*/
	}
}

void MySensorsBase::UpdateNodeHeartbeat(const int nodeID)
{

	std::map<int, _tMySensorNode>::iterator ittNode = m_nodes.find(nodeID);
	if (ittNode == m_nodes.end())
		return; //Not found

	int intValue;
	mytime(&m_LastHeartbeatReceive);
	_tMySensorNode *pNode = &ittNode->second;
	std::vector<_tMySensorChild>::iterator itt;


	for (itt = pNode->m_childs.begin(); itt != pNode->m_childs.end(); ++itt)
	{
		std::map<_eSetType, _tMySensorValue>::const_iterator itt2;
		for (itt2 = itt->values.begin(); itt2 != itt->values.end(); ++itt2)
		{
			if (itt2->second.bValidValue)
			{
				_eSetType vType = itt2->first;
				switch (vType)
				{
				case V_TRIPPED:
				case V_ARMED:
				case V_LOCK_STATUS:
				case V_STATUS:
				case V_PERCENTAGE:
					UpdateSwitchLastUpdate(nodeID, itt->childID);
					break;
				case V_SCENE_ON:
				case V_SCENE_OFF:
					if (itt->GetValue(vType, intValue))
						UpdateSwitchLastUpdate(nodeID, itt->childID + intValue);
					break;
				case V_UP:
				case V_DOWN:
				case V_STOP:
					if (itt->GetValue(vType, intValue))
						UpdateBlindSensorLastUpdate(nodeID, itt->childID);
					break;
				case V_RGB:
				case V_RGBW:
					if (itt->GetValue(vType, intValue))
						UpdateRGBWSwitchLastUpdate(nodeID, itt->childID);
					break;
				}
			}
		}
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
	pChild = FindChildWithValueType(nodeID, V_WIND, 0);
	if (!pChild)
		return;
	if (!pChild->GetValue(V_WIND, fWind))
		return;
	fGust = fWind;
	ChildID = pChild->childID;
	iBatteryLevel = pChild->batValue;

	pChild = FindChildWithValueType(nodeID, V_DIRECTION, 0);
	if (!pChild)
		return;
	if (!pChild->GetValue(V_DIRECTION, iDirection))
		return;

	pChild = FindChildWithValueType(nodeID, V_GUST, 0);
	if (pChild)
	{
		if (!pChild->GetValue(V_GUST, fGust))
			return;
	}

	pChild = FindChildWithValueType(nodeID, V_TEMP, 0);
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
	SendWind(cNode, iBatteryLevel, iDirection, fWind, fGust, fTemp, fChill, bHaveTemp, sname);
}

void MySensorsBase::SendSensor2Domoticz(_tMySensorNode *pNode, _tMySensorChild *pChild, const _eSetType vType)
{
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
		_tMySensorChild *pChildHum = FindChildWithValueType(pChild->nodeID, V_HUM, pChild->groupID);
		_tMySensorChild *pChildBaro = FindChildWithValueType(pChild->nodeID, V_PRESSURE, pChild->groupID);
		if (pChildHum && pChildBaro)
		{
			int Humidity;
			float Baro;
			bool bHaveHumidity = pChildHum->GetValue(V_HUM, Humidity);
			bool bHaveBaro = pChildBaro->GetValue(V_PRESSURE, Baro);
			if (bHaveHumidity && bHaveBaro)
			{
				int forecast = bmpbaroforecast_unknown;
				_tMySensorChild *pSensorForecast = FindChildWithValueType(pChild->nodeID, V_FORECAST, pChild->groupID);
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
		_tMySensorChild *pChildTemp = FindChildWithValueType(pChild->nodeID, V_TEMP, pChild->groupID);
		_tMySensorChild *pChildBaro = FindChildWithValueType(pChild->nodeID, V_PRESSURE, pChild->groupID);
		int forecast = bmpbaroforecast_unknown;
		_tMySensorChild *pSensorForecast = FindChildWithValueType(pChild->nodeID, V_FORECAST, pChild->groupID);
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
		int Humidity;
		pChild->GetValue(V_HUM, Humidity);
		if (pChildTemp && pChildBaro)
		{
			float Baro;
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
			SendHumiditySensor(cNode, pChild->batValue, Humidity, (!pChild->childName.empty()) ? pChild->childName : "Hum");
		}
	}
	break;
	case V_PRESSURE:
	{
		float Baro;
		pChild->GetValue(V_PRESSURE, Baro);
		_tMySensorChild *pSensorTemp = FindChildWithValueType(pChild->nodeID, V_TEMP, pChild->groupID);
		_tMySensorChild *pSensorHum = FindChildWithValueType(pChild->nodeID, V_HUM, pChild->groupID);
		int forecast = bmpbaroforecast_unknown;
		_tMySensorChild *pSensorForecast = FindChildWithValueType(pChild->nodeID, V_FORECAST, pChild->groupID);
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
			SendBaroSensor(pChild->nodeID, pChild->childID, pChild->batValue, Baro, forecast, (!pChild->childName.empty()) ? pChild->childName : "Baro");
	}
	break;
	case V_TRIPPED:
		//	Tripped status of a security sensor. 1 = Tripped, 0 = Untripped
		if (pChild->GetValue(vType, intValue))
			UpdateSwitch(vType, pChild->nodeID, pChild->childID, (intValue == 1), 100, (!pChild->childName.empty()) ? pChild->childName : "Security Sensor", pChild->batValue);
		break;
	case V_ARMED:
		//Armed status of a security sensor. 1 = Armed, 0 = Bypassed
		if (pChild->GetValue(vType, intValue))
			UpdateSwitch(vType, pChild->nodeID, pChild->childID, (intValue == 1), 100, (!pChild->childName.empty()) ? pChild->childName : "Security Sensor", pChild->batValue);
		break;
	case V_LOCK_STATUS:
		//Lock status. 1 = Locked, 0 = Unlocked
		if (pChild->GetValue(vType, intValue))
			UpdateSwitch(vType, pChild->nodeID, pChild->childID, (intValue == 1), 100, (!pChild->childName.empty()) ? pChild->childName : "Lock Sensor", pChild->batValue);
		break;
	case V_STATUS:
		//	Light status. 0 = off 1 = on
		if (pChild->GetValue(vType, intValue))
			UpdateSwitch(vType, pChild->nodeID, pChild->childID, (intValue != 0), 100, (!pChild->childName.empty()) ? pChild->childName : "Light", pChild->batValue);
		break;
	case V_SCENE_ON:
		if (pChild->GetValue(vType, intValue))
			UpdateSwitch(vType, pChild->nodeID, pChild->childID + intValue, true, 100, (!pChild->childName.empty()) ? pChild->childName : "Scene", pChild->batValue);
		break;
	case V_SCENE_OFF:
		if (pChild->GetValue(vType, intValue))
			UpdateSwitch(vType, pChild->nodeID, pChild->childID + intValue, false, 100, (!pChild->childName.empty()) ? pChild->childName : "Scene", pChild->batValue);
		break;
	case V_PERCENTAGE:
		//	Dimmer value. 0 - 100 %
		if (pChild->GetValue(vType, intValue))
		{
			int level = intValue;
			UpdateSwitch(vType, pChild->nodeID, pChild->childID, (level != 0), level, (!pChild->childName.empty()) ? pChild->childName : "Light", pChild->batValue);
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
			//Light level in percentage
			SendPercentageSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Light Level");
			//SendLuxSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Light Level");
		}
		break;
	case V_LEVEL: //stored as Int AND Float
		if (pChild->GetValue(vType, intValue))
		{
			if (pChild->presType == S_DUST)
			{
				if (pChild->GetValue(vType, floatValue))
					SendCustomSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Dust Sensor", "ug/m3");
			}
			else if (pChild->presType == S_AIR_QUALITY)
			{
				SendAirQualitySensor(pChild->nodeID, pChild->childID, pChild->batValue, intValue, (!pChild->childName.empty()) ? pChild->childName : "Air Quality");
			}
			else if (pChild->presType == S_LIGHT_LEVEL)
			{
				if (pChild->GetValue(vType, floatValue))
					SendLuxSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Lux");
			}
			else if (pChild->presType == S_SOUND)
			{
				SendSoundSensor(cNode, pChild->batValue, intValue, (!pChild->childName.empty()) ? pChild->childName : "Sound Level");
			}
			else if (pChild->presType == S_MOISTURE)
			{
				SendMoistureSensor(cNode, pChild->batValue, intValue, (!pChild->childName.empty()) ? pChild->childName : "Moisture");
			}
			if (pChild->presType == S_VIBRATION)
			{
				if (pChild->GetValue(vType, floatValue))
					SendCustomSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Vibration", "Hz");
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
						SendKwhMeter(pSensorKwh->nodeID, pSensorKwh->childID, pSensorKwh->batValue, floatValue, Kwh, (!pChild->childName.empty()) ? pChild->childName : "Meter");
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
					SendKwhMeter(pChild->nodeID, pChild->childID, pChild->batValue, Watt, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Meter");
			}
			else {
				SendKwhMeter(pChild->nodeID, pChild->childID, pChild->batValue, 0, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Meter");
			}
		}
		break;
	case V_DISTANCE:
		if (pChild->GetValue(vType, floatValue))
			SendDistanceSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Distance");
		break;
	case V_FLOW:
		//Flow of water/gas in meter (for now send as a percentage sensor)
		if (pChild->GetValue(vType, floatValue))
		{
			if (pChild->presType == S_WATER)
				SendWaterflowSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Water Flow");
			else
				SendWaterflowSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Gas Flow");
		}
		break;
	case V_VOLUME:
		//Water or Gas Volume
		if (pChild->GetValue(vType, floatValue))
		{
			if (pChild->presType == S_WATER)
				SendMeterSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Water");
			else
				SendMeterSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Gas");
		}
		break;
	case V_VOLTAGE:
		if (pChild->GetValue(vType, floatValue))
			SendVoltageSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Voltage");
		break;
	case V_UV:
		if (pChild->GetValue(vType, floatValue))
			SendUVSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "UV");
		break;
	case V_IMPEDANCE:
		if (pChild->GetValue(vType, floatValue))
			SendPercentageSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Impedance");
		break;
	case V_WEIGHT:
		if (pChild->GetValue(vType, floatValue))
			SendCustomSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Weight", "g");
		break;
	case V_CURRENT:
		if (pChild->GetValue(vType, floatValue))
			SendCurrentSensor(cNode, pChild->batValue, floatValue, 0, 0, (!pChild->childName.empty()) ? pChild->childName : "Current");
		break;
	case V_FORECAST:
		if (pChild->GetValue(vType, intValue))
		{
			_tMySensorChild *pSensorBaro = FindChildWithValueType(pChild->nodeID, V_PRESSURE, pChild->groupID);
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
					SendBaroSensor(pSensorBaro->nodeID, pSensorBaro->childID, pSensorBaro->batValue, Baro, forecast, (!pChild->childName.empty()) ? pChild->childName : "Baro");
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
			SendTextSensor(pNode->nodeID, pChild->childID, pChild->batValue, stringValue, (!pChild->childName.empty()) ? pChild->childName : "Text Sensor");
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
			sDecodeRXMessage(this, (const unsigned char *)&gswitch, (!pChild->childName.empty()) ? pChild->childName.c_str() : "IR Command", pChild->batValue);
		}
		break;
	case V_PH:
		if (pChild->GetValue(vType, floatValue))
			SendCustomSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Water Quality", "pH");
		break;
	case V_ORP:
		if (pChild->GetValue(vType, floatValue))
			SendCustomSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Water Quality", "mV");
		break;
	case V_EC:
		if (pChild->GetValue(vType, floatValue))
			SendCustomSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Water Quality", "S/cm");
		break;
	case V_VA:
		if (pChild->GetValue(vType, floatValue))
			SendCustomSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Voltage Ampere", "VA");
		break;
	case V_POWER_FACTOR:
		if (pChild->GetValue(vType, floatValue))
			SendPercentageSensor(pChild->nodeID, pChild->childID, pChild->batValue, floatValue, (!pChild->childName.empty()) ? pChild->childName : "Power Factor");
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

void MySensorsBase::UpdateSwitchLastUpdate(const unsigned char Idx, const int SubUnit)
{
	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", 0, 0, 0, Idx);
	std::vector<std::vector<std::string> > result;
	// LLEMARINEL : #1312  Changed pTypeLighting2 to pTypeGeneralSwitch
	result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d) AND (Type==%d) AND (Subtype==%d)", m_HwdID, szIdx, SubUnit, int(pTypeGeneralSwitch), int(sSwitchTypeAC));
	if (result.size() < 1)
		return; //not found!
	time_t now = time(0);
	struct tm ltime;
	localtime_r(&now, &ltime);

	char szLastUpdate[40];
	sprintf(szLastUpdate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);

	m_sql.safe_query("UPDATE DeviceStatus SET LastUpdate='%q' WHERE (ID = '%q')", szLastUpdate, result[0][0].c_str());
}

void MySensorsBase::UpdateBlindSensorLastUpdate(const int NodeID, const int ChildID)
{
	char szIdx[10];
	sprintf(szIdx, "%02X%02X%02X", 0, 0, NodeID);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)", m_HwdID, szIdx, ChildID);
	if (result.size() < 1)
		return;
	time_t now = time(0);
	struct tm ltime;
	localtime_r(&now, &ltime);

	char szLastUpdate[40];
	sprintf(szLastUpdate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
	m_sql.safe_query("UPDATE DeviceStatus SET LastUpdate='%q' WHERE (ID = '%q')", szLastUpdate, result[0][0].c_str());
}

void MySensorsBase::UpdateRGBWSwitchLastUpdate(const int NodeID, const int ChildID)
{
	char szIdx[10];
	if (NodeID == 1)
		sprintf(szIdx, "%d", 1);
	else
		sprintf(szIdx, "%08x", (unsigned int)NodeID);

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)", m_HwdID, szIdx, ChildID);
	if (result.size() < 1)
		return;
	time_t now = time(0);
	struct tm ltime;
	localtime_r(&now, &ltime);

	char szLastUpdate[40];
	sprintf(szLastUpdate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
	m_sql.safe_query("UPDATE DeviceStatus SET LastUpdate='%q' WHERE (ID = '%q')", szLastUpdate, result[0][0].c_str());
}

void MySensorsBase::UpdateSwitch(const _eSetType vType, const unsigned char Idx, const int SubUnit, const bool bOn, const double Level, const std::string &defaultname, const int BatLevel)
{
	// LLEMARINEL : #1312  Changed to use as pTypeGeneralSwitch : do not constrain to 16 steps anymore but 100 :
	int level = int(Level);

	char szIdx[10];
	sprintf(szIdx, "%02X%02X%02X%02X", 0, 0, 0, Idx);
	std::vector<std::vector<std::string> > result;

	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%s') AND (Unit==%d) AND (Type==%d) AND (Subtype==%d)", m_HwdID, szIdx, SubUnit, int(pTypeGeneralSwitch), int(sSwitchTypeAC));


	if (!result.empty())
	{
		if (
			(((vType != V_TRIPPED) || (!bOn))) &&
			((vType != V_SCENE_OFF) && (vType != V_SCENE_ON))
			)
		{
			//check if we have a change, if not do not update it
			int nvalue = atoi(result[0][1].c_str());
			if ((!bOn) && (nvalue == 0))
				return;
			if ((bOn && (nvalue != 0)))
			{
				//Check Level
				int slevel = atoi(result[0][2].c_str());
				if (slevel == level)
					return;
			}
		}
	}

	// LLEMARINEL : #1312  Changed to use as pTypeGeneralSwitch
	// Send as General Switch :
	_tGeneralSwitch gswitch;
	gswitch.subtype = sSwitchTypeAC;
	gswitch.id = Idx;
	gswitch.unitcode = SubUnit;
	if (!bOn)
	{
		gswitch.cmnd = gswitch_sOff;
	}
	else
	{
		gswitch.cmnd = gswitch_sOn;
	}
    gswitch.level = level; //level;
	gswitch.battery_level = BatLevel;
	gswitch.rssi = 12;
	gswitch.seqnbr = 0;
	sDecodeRXMessage(this, (const unsigned char *)&gswitch, defaultname.c_str(), BatLevel);


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

bool MySensorsBase::GetSwitchValue(const int Idx, const int SubUnit, const int sub_type, std::string &sSwitchValue)
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
		sprintf(szIdx, "%02X%02X%02X%02X", 0, 0, 0, Idx);
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

std::string MySensorsBase::GetGatewayVersion()
{
	return m_GatewayVersion;
}

bool MySensorsBase::SendNodeSetCommand(const int NodeID, const int ChildID, const _eMessageType messageType, const _eSetType SubType, const std::string &Payload, const bool bUseAck, const int AckTimeout)
{
	m_bAckReceived = false;
	m_AckNodeID = NodeID;
	m_AckChildID = ChildID;
	m_AckSetType = SubType;
	int repeat = 0;
	int repeats = 2;

	//Resend failed command
	while ((!m_bAckReceived) && (repeat < repeats))
	{
		SendCommandInt(NodeID, ChildID, messageType, bUseAck, SubType, Payload);
		if (!bUseAck)
			return true;
		//Wait some time till we receive an ACK (should be received in 1000ms, but we wait 1200ms)
		int waitRetries = AckTimeout/100;
		if (waitRetries < 1)
			waitRetries = 1;
		int actWaits = 0;
		while ((!m_bAckReceived) && (actWaits < waitRetries))
		{
			sleep_milliseconds(100);
			actWaits++;
		}
		repeat++;
	}
	return m_bAckReceived;
}

void MySensorsBase::SendNodeCommand(const int NodeID, const int ChildID, const _eMessageType messageType, const int SubType, const std::string &Payload)
{
	SendCommandInt(NodeID, ChildID, messageType, false, SubType, Payload);
}

void MySensorsBase::SendCommandInt(const int NodeID, const int ChildID, const _eMessageType messageType, const bool UseAck, const int SubType, const std::string &Payload)
{
	std::stringstream sstr;
	std::string szAck = (UseAck == true) ? "1" : "0";
	sstr << NodeID << ";" << ChildID << ";" << int(messageType) << ";" <<szAck << ";" << SubType << ";" << Payload << '\n';
	m_sendQueue.push(sstr.str());
}

bool MySensorsBase::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *pCmd = reinterpret_cast<const tRBUF *>(pdata);
	unsigned char packettype = pCmd->ICMND.packettype;
	unsigned char subtype = pCmd->ICMND.subtype;

	// LLEMARINEL : #1312  Change to pTypeGeneralSwitch insteand of Lighting2
	if (packettype == pTypeLighting2)
	{
		//Light command

		int node_id = pCmd->LIGHTING2.id4;
		int child_sensor_id = pCmd->LIGHTING2.unitcode;

		if (_tMySensorNode *pNode = FindNode(node_id))
		{
			_tMySensorChild *pChild = pNode->FindChild(child_sensor_id);
			if (!pChild)
			{
				_log.Log(LOG_ERROR, "MySensors: Light command received for unknown node_id: %d, child_id: %d", node_id, child_sensor_id);
				return false;
			}

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
				if (pChild->presType == S_LOCK)
				{
					//Door lock/contact
					return SendNodeSetCommand(node_id, child_sensor_id, MT_Set, V_LOCK_STATUS, lState, pChild->useAck, pChild->ackTimeout);
				}
				else if (pChild->presType == S_SCENE_CONTROLLER)
				{
					//Scene Controller
					return SendNodeSetCommand(node_id, child_sensor_id, MT_Set, (light_command == light2_sOn) ? V_SCENE_ON : V_SCENE_OFF, lState, pChild->useAck, pChild->ackTimeout);
				}
				else
				{
					//normal
					return SendNodeSetCommand(node_id, child_sensor_id, MT_Set, V_STATUS, lState, pChild->useAck, pChild->ackTimeout);
				}
			}
			else if (light_command == light2_sSetLevel)
			{
				float fvalue = (100.0f / 14.0f)*float(pCmd->LIGHTING2.level);
				if (fvalue > 100.0f)
					fvalue = 100.0f; //99 is fully on
				int svalue = round(fvalue);

				std::stringstream sstr;
				sstr << svalue;
				return SendNodeSetCommand(node_id, child_sensor_id, MT_Set, V_PERCENTAGE, sstr.str(), pChild->useAck, pChild->ackTimeout);
			}
		}
		else {
			_log.Log(LOG_ERROR, "MySensors: Light command received for unknown node_id: %d", node_id);
			return false;
		}
	}
	else if (packettype == pTypeGeneralSwitch)
	{
		//Light command
		const _tGeneralSwitch *pSwitch = reinterpret_cast<const _tGeneralSwitch*>(pdata);

		int node_id = pSwitch->id;
		int child_sensor_id = pSwitch->unitcode;

		if (_tMySensorNode *pNode = FindNode(node_id))
		{
			_tMySensorChild *pChild = pNode->FindChild(child_sensor_id);
			if (!pChild)
			{
				_log.Log(LOG_ERROR, "MySensors: Light command received for unknown node_id: %d, child_id: %d", node_id, child_sensor_id);
				return false;
			}

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
					// For dimmers we only allow level 0-100
					level = (level > 100) ? 100 : level;
				}
			}

			if ((cmnd == gswitch_sOn) || (cmnd == gswitch_sOff))
			{
				std::string lState = (cmnd == gswitch_sOn) ? "1" : "0";
				if (pChild->presType == S_LOCK)
				{
					//Door lock/contact
					return SendNodeSetCommand(node_id, child_sensor_id, MT_Set, V_LOCK_STATUS, lState, pChild->useAck, pChild->ackTimeout);
				}
				else if (pChild->presType == S_SCENE_CONTROLLER)
				{
					//Scene Controller
					return SendNodeSetCommand(node_id, child_sensor_id, MT_Set, (cmnd == gswitch_sOn) ? V_SCENE_ON : V_SCENE_OFF, lState, pChild->useAck, pChild->ackTimeout);
				}
				else
				{
					//normal
					return SendNodeSetCommand(node_id, child_sensor_id, MT_Set, V_STATUS, lState, pChild->useAck, pChild->ackTimeout);
				}
			}
			else if (cmnd == gswitch_sSetLevel)
			{
				std::stringstream sstr;
				sstr << level;
				return SendNodeSetCommand(node_id, child_sensor_id, MT_Set, V_PERCENTAGE, sstr.str(), pChild->useAck, pChild->ackTimeout);
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
		//unsigned char ID1 = (unsigned char)((pLed->id & 0xFF000000) >> 24);
		//unsigned char ID2 = (unsigned char)((pLed->id & 0x00FF0000) >> 16);
		unsigned char ID3 = (unsigned char)((pLed->id & 0x0000FF00) >> 8);
		unsigned char ID4 = (unsigned char)pLed->id & 0x000000FF;

		int node_id = (ID3 << 8) | ID4;
		int child_sensor_id = pLed->dunit;

		if (_tMySensorNode *pNode = FindNode(node_id))
		{
			_tMySensorChild *pChild = pNode->FindChild(child_sensor_id);
			if (!pChild)
			{
				_log.Log(LOG_ERROR, "MySensors: Light command received for unknown node_id: %d, child_id: %d", node_id, child_sensor_id);
				return false;
			}

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
				return SendNodeSetCommand(node_id, child_sensor_id, MT_Set, (bIsRGBW == true) ? V_RGBW : V_RGB, sstr.str(), pChild->useAck, pChild->ackTimeout);
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
				return SendNodeSetCommand(node_id, child_sensor_id, MT_Set, (bIsRGBW == true) ? V_RGBW : V_RGB, sstr.str(), pChild->useAck, pChild->ackTimeout);
			}
			else if (pLed->command == Limitless_SetBrightnessLevel)
			{
				float fvalue = pLed->value;
				int svalue = round(fvalue);
				if (svalue > 100)
					svalue = 100;
				std::stringstream sstr;
				sstr << svalue;
				return SendNodeSetCommand(node_id, child_sensor_id, MT_Set, V_PERCENTAGE, sstr.str(), pChild->useAck, pChild->ackTimeout);
			}
			else if ((pLed->command == Limitless_LedOff) || (pLed->command == Limitless_LedOn))
			{
				std::string lState = (pLed->command == Limitless_LedOn) ? "1" : "0";
				return SendNodeSetCommand(node_id, child_sensor_id, MT_Set, V_STATUS, lState, pChild->useAck, pChild->ackTimeout);
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
			_tMySensorChild *pChild = pNode->FindChild(child_sensor_id);
			if (!pChild)
			{
				_log.Log(LOG_ERROR, "MySensors: Light command received for unknown node_id: %d, child_id: %d", node_id, child_sensor_id);
				return false;
			}

			if (pCmd->BLINDS1.cmnd == blinds_sOpen)
			{
				return SendNodeSetCommand(node_id, child_sensor_id, MT_Set, V_UP, "", pChild->useAck, pChild->ackTimeout);
			}
			else if (pCmd->BLINDS1.cmnd == blinds_sClose)
			{
				return SendNodeSetCommand(node_id, child_sensor_id, MT_Set, V_DOWN, "", pChild->useAck, pChild->ackTimeout);
			}
			else if (pCmd->BLINDS1.cmnd == blinds_sStop)
			{
				return SendNodeSetCommand(node_id, child_sensor_id, MT_Set, V_STOP, "", pChild->useAck, pChild->ackTimeout);
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
		const _tThermostat *pMeter = reinterpret_cast<const _tThermostat *>(pCmd);

		int node_id = pMeter->id2;
		int child_sensor_id = pMeter->id3;
		_eSetType vtype_id = (_eSetType)pMeter->id4;

		if (_tMySensorNode *pNode = FindNode(node_id))
		{
			_tMySensorChild *pChild = pNode->FindChild(child_sensor_id);
			if (!pChild)
			{
				_log.Log(LOG_ERROR, "MySensors: Light command received for unknown node_id: %d, child_id: %d", node_id, child_sensor_id);
				return false;
			}

			char szTmp[10];
			sprintf(szTmp, "%.1f", pMeter->temp);
			return SendNodeSetCommand(node_id, child_sensor_id, MT_Set, vtype_id, szTmp, pChild->useAck, pChild->ackTimeout);
		}
		else {
			_log.Log(LOG_ERROR, "MySensors: Blinds/Window command received for unknown node_id: %d", node_id);
			return false;
		}
	}
	else if (packettype == pTypeGeneralSwitch)
	{
		//Used to store IR codes
		const _tGeneralSwitch *pSwitch= reinterpret_cast<const _tGeneralSwitch *>(pCmd);

		int node_id = pSwitch->unitcode;
		unsigned int ir_code = pSwitch->id;

		if (_tMySensorNode *pNode = FindNode(node_id))
		{
			_tMySensorChild* pChild = pNode->FindChildByValueType(V_IR_RECEIVE);
			if (pChild)
			{
				std::stringstream sstr;
				sstr << ir_code;
				return SendNodeSetCommand(node_id, pChild->childID, MT_Set, V_IR_SEND, sstr.str(), pChild->useAck, pChild->ackTimeout);
			}
		}
		else {
			_log.Log(LOG_ERROR, "MySensors: Blinds/Window command received for unknown node_id: %d", node_id);
			return false;
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

void MySensorsBase::UpdateChildDBInfo(const int NodeID, const int ChildID, const _ePresentationType pType, const std::string &Name)
{
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ROWID FROM MySensorsChilds WHERE (HardwareID=%d) AND (NodeID=%d) AND (ChildID=%d)", m_HwdID, NodeID, ChildID);
	if (result.size() < 1)
	{
		//Insert
		bool bUseAck = (ChildID == 255) ? false : true;
		m_sql.safe_query("INSERT INTO MySensorsChilds (HardwareID, NodeID, ChildID, [Type], [Name], UseAck) VALUES (%d, %d, %d, %d, '%q', %d)", m_HwdID, NodeID, ChildID, pType, Name.c_str(), (bUseAck) ? 1 : 0);
	}
	else
	{
		//Update
		m_sql.safe_query("UPDATE MySensorsChilds SET [Type]='%d', [Name]='%q' WHERE (ROWID = '%q')", pType, Name.c_str(), result[0][0].c_str());
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
	if (results.size() >= 6)
	{
		for (size_t ip = 0; ip < results.size() - 5; ip++)
		{
			payload = results[5+ip];
		}
	}
	std::stringstream sstr;
#ifdef _DEBUG
	_log.Log(LOG_NORM, "MySensors: NodeID: %d, ChildID: %d, MessageType: %d, Ack: %d, SubType: %d, Payload: %s",node_id,child_sensor_id,message_type,ack,sub_type,payload.c_str());
#endif

	if (message_type == MT_Internal)
	{
		switch (sub_type)
		{
		case I_VERSION:
			{
				if (node_id == 0)
				{
					//Store gateway version
					m_GatewayVersion = payload;
					_log.Log(LOG_NORM, "MySensors: Gateway Version: %s", payload.c_str());
				}
				else {
					_log.Log(LOG_NORM, "MySensors: VERSION from NodeID: %d, ChildID: %d, Payload: %s", node_id, child_sensor_id, payload.c_str());
				}
			}
		break;
		case I_ID_REQUEST:
			{
				//Set a unique node id from the controller
				int newID = FindNextNodeID();
				if (newID != -1)
				{
					sstr << newID;
					SendNodeCommand(node_id, child_sensor_id, message_type, I_ID_RESPONSE, sstr.str());
				}
			}
			break;
		case I_CONFIG:
			// (M)etric or (I)mperal back to sensor.
			//Set a unique node id from the controller
			SendNodeCommand(node_id, child_sensor_id, message_type, I_CONFIG, "M");
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
			//Request Gateway Version
			SendCommandInt(0, 0, MT_Internal, false, I_VERSION, "Get Version");
			break;
		case I_TIME:
			//send time in seconds from 1970 with timezone offset
			{
				boost::posix_time::ptime tlocal(boost::posix_time::second_clock::local_time());
				boost::posix_time::time_duration dur = tlocal - boost::posix_time::ptime(boost::gregorian::date(1970, 1, 1));
				time_t fltime(dur.total_seconds());
				sstr << fltime;
				SendNodeCommand(node_id, child_sensor_id, message_type, I_TIME, sstr.str());
			}
			break;
		case I_HEARTBEAT:
		case I_HEARTBEAT_RESPONSE:
			//Received a heartbeat request/response
			if (node_id != 255)
			{
				UpdateNodeHeartbeat(node_id);
			}
			break;
		case I_REQUEST_SIGNING:
			//Used between sensors when initiating signing.
			while (1 == 0);
			break;
		case I_PING:
			//Ping sent to node, payload incremental hop counter
			while (1 == 0);
			break;
		case I_PONG:
			//In return to ping, sent back to sender, payload incremental hop counter
			while (1 == 0);
			break;
		case I_REGISTRATION_REQUEST:
			//Register request to GW
			while (1 == 0);
			break;
		case I_REGISTRATION_RESPONSE:
			//Register response from GW
			while (1 == 0);
			break;
		case I_DEBUG:
			//Debug message
			while (1 == 0);
			break;
		default:
			while (1==0);
			break;
		}
	}
	else if (message_type == MT_Set)
	{
		_eSetType vType = (_eSetType)sub_type;

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
			if (!GetChildDBInfo(node_id, child_sensor_id, mSensor.presType, mSensor.childName, mSensor.useAck))
			{
				UpdateChildDBInfo(node_id, child_sensor_id, S_UNKNOWN, "");
			}
			pNode->m_childs.push_back(mSensor);
			pChild = pNode->FindChild(child_sensor_id);
			if (pChild == NULL)
				return;
		}
		pChild->lastreceived = pNode->lastreceived;

		if (
			(ack == 1) &&
			(node_id == m_AckNodeID) &&
			(child_sensor_id == m_AckChildID) &&
			(vType == m_AckSetType))
		{
			m_AckNodeID = m_AckChildID = -1;
			m_AckSetType = V_UNKNOWN;
			m_bAckReceived = true;
			//No need to process ack commands
			return;
		}

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
		case V_CUSTOM:
			//Request for a sensor state
			if (!payload.empty())
			{
				uint64_t idx = boost::lexical_cast<uint64_t>(payload);
				int nValue;
				std::string sValue;
				if (m_mainworker.GetSensorData(idx, nValue, sValue))
				{
					std::stringstream sstr;
					sstr << idx << ";" << nValue << ";" << sValue;
					std::string sPayload = sstr.str();
					stdreplace(sPayload, ";", "#"); //cant send payload with ;
					SendNodeCommand(node_id, child_sensor_id, message_type, sub_type, sPayload);
				}
			}
			break;
		case V_RAINRATE:
			//not needed, this is now calculated by domoticz for any type of rain sensor
			break;
		case V_PH:
		case V_ORP:
		case V_EC:
		case V_VAR:
		case V_VA:
		case V_POWER_FACTOR:
			pChild->SetValue(vType, (float)atof(payload.c_str()));
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
		if (node_id == 255)
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
		case S_INFO:
			vType = V_TEXT;
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
			(pSensor->childName != payload)
			);
		pSensor->lastreceived = mytime(NULL);
		pSensor->presType = pType;
		pSensor->childName = payload;

		if (bDiffPresentation)
		{
			UpdateChildDBInfo(node_id, child_sensor_id, pType, pSensor->childName);
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
					UpdateSwitch(vType, node_id, child_sensor_id, false, 100, (!pSensor->childName.empty()) ? pSensor->childName : "Light", pSensor->batValue);
				else if (vType == V_TRIPPED)
					UpdateSwitch(vType, node_id, child_sensor_id, false, 100, (!pSensor->childName.empty()) ? pSensor->childName : "Security Sensor", pSensor->batValue);
				else if (vType == V_RGBW)
					SendRGBWSwitch(node_id, child_sensor_id, pSensor->batValue, 0, true, (!pSensor->childName.empty()) ? pSensor->childName : "RGBW Light");
				else if (vType == V_RGB)
					SendRGBWSwitch(node_id, child_sensor_id, pSensor->batValue, 0, false, (!pSensor->childName.empty()) ? pSensor->childName : "RGB Light");
			}
		}
		else if (vType == V_UP)
		{
			int blind_value;
			if (!GetBlindsValue(node_id, child_sensor_id, blind_value))
			{
				SendBlindSensor(node_id, child_sensor_id, pSensor->batValue, blinds_sOpen, (!pSensor->childName.empty()) ? pSensor->childName : "Blinds/Window");
			}
		}
		else if (vType == V_TEXT)
		{
			bool bExits = false;
			std::string mtext=GetTextSensorText(node_id, child_sensor_id, bExits);
			if (!bExits)
			{
				SendTextSensor(node_id, child_sensor_id, pSensor->batValue, "-", (!pSensor->childName.empty()) ? pSensor->childName : "Text Sensor");
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
				SendNodeCommand(node_id, child_sensor_id, message_type, sub_type, tmpstr);
			break;
		case V_VAR1:
		case V_VAR2:
		case V_VAR3:
		case V_VAR4:
		case V_VAR5:
			//send back a previous stored custom variable
			tmpstr = "";
			GetVar(node_id, child_sensor_id, sub_type, tmpstr);
			//SendNodeSetCommand(node_id, child_sensor_id, message_type, (_eSetType)sub_type, tmpstr, true, 1000);
			SendNodeCommand(node_id, child_sensor_id, message_type, sub_type, tmpstr);
			break;
		case V_TEXT:
			{
				//Get Text sensor value from the database
				bool bExits = false;
				tmpstr = GetTextSensorText(node_id, child_sensor_id, bExits);
				SendNodeCommand(node_id, child_sensor_id, message_type, sub_type, tmpstr);
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

bool MySensorsBase::StartSendQueue()
{
	//Start worker thread
	m_send_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&MySensorsBase::Do_Send_Work, this)));
	return (m_send_thread != NULL);
}

void MySensorsBase::StopSendQueue()
{
	if (m_send_thread != NULL)
	{
		assert(m_send_thread);
		//Add a dummy queue item, so we stop
		std::string emptyString;
		m_sendQueue.push(emptyString);
		m_send_thread->join();
	}
}

void MySensorsBase::Do_Send_Work()
{
	while (true)
	{
		std::string toSend;
		bool hasPopped = m_sendQueue.timed_wait_and_pop<boost::posix_time::milliseconds>(toSend, boost::posix_time::milliseconds(2000));
		if (!hasPopped) {
			continue;
		}
		if (toSend.empty())
		{
			//Exit thread
			return;
		}
#ifdef _DEBUG
		_log.Log(LOG_STATUS, "MySensors: going to send: %s", toSend.c_str());
#endif
		WriteInt(toSend);
	}
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_MySensorsGetNodes(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			std::string hwid = request::findValue(&req, "idx");
			if (hwid == "")
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(iHardwareID);
			if (pHardware == NULL)
				return;
			if (
				(pHardware->HwdType != HTYPE_MySensorsUSB)&&
				(pHardware->HwdType != HTYPE_MySensorsTCP)&&
				(pHardware->HwdType != HTYPE_MySensorsMQTT)
				)
				return;
			MySensorsBase *pMySensorsHardware = reinterpret_cast<MySensorsBase*>(pHardware);

			root["status"] = "OK";
			root["title"] = "MySensorsGetNodes";

			std::vector<std::vector<std::string> > result, result2;
			char szTmp[100];

			result = m_sql.safe_query("SELECT ID,Name,SketchName,SketchVersion FROM MySensors WHERE (HardwareID==%d) ORDER BY ID ASC", iHardwareID);
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					int NodeID = atoi(sd[0].c_str());

					root["result"][ii]["idx"] = NodeID;
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["SketchName"] = sd[2];
					root["result"][ii]["Version"] = sd[3];

					MySensorsBase::_tMySensorNode* pNode = pMySensorsHardware->FindNode(NodeID);

					if (pNode != NULL)
					{
						if (pNode->lastreceived != 0)
						{
							struct tm loctime;
							localtime_r(&pNode->lastreceived, &loctime);
							strftime(szTmp, 80, "%Y-%m-%d %X", &loctime);
							root["result"][ii]["LastReceived"] = szTmp;
						}
						else
						{
							root["result"][ii]["LastReceived"] = "-";
						}
					}
					else
					{
						root["result"][ii]["LastReceived"] = "-";
					}
					result2 = m_sql.safe_query("SELECT COUNT(*) FROM MySensorsChilds WHERE (HardwareID=%d) AND (NodeID == %d)", iHardwareID, NodeID);
					int totChilds = 0;
					if (!result2.empty())
					{
						totChilds = atoi(result2[0][0].c_str());
					}
					root["result"][ii]["Childs"] = totChilds;
					ii++;
				}
			}
		}
		void CWebServer::Cmd_MySensorsGetChilds(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			std::string hwid = request::findValue(&req, "idx");
			std::string nodeid = request::findValue(&req, "nodeid");
			if (
				(hwid == "")||
				(nodeid == "")
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(iHardwareID);
			if (pHardware == NULL)
				return;
			if (
				(pHardware->HwdType != HTYPE_MySensorsUSB) &&
				(pHardware->HwdType != HTYPE_MySensorsTCP) &&
				(pHardware->HwdType != HTYPE_MySensorsMQTT)
				)
				return;
			MySensorsBase *pMySensorsHardware = reinterpret_cast<MySensorsBase*>(pHardware);

			root["status"] = "OK";
			root["title"] = "MySensorsGetChilds";
			int NodeID = atoi(nodeid.c_str());
			MySensorsBase::_tMySensorNode* pNode = pMySensorsHardware->FindNode(NodeID);
			std::vector<std::vector<std::string> >::const_iterator itt2;
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ChildID, [Type], Name, UseAck, AckTimeout FROM MySensorsChilds WHERE (HardwareID=%d) AND (NodeID == %d) ORDER BY ChildID ASC", iHardwareID, NodeID);
			int ii = 0;
			for (itt2 = result.begin(); itt2 != result.end(); ++itt2)
			{
				std::vector<std::string> sd2 = *itt2;
				int ChildID = atoi(sd2[0].c_str());
				root["result"][ii]["child_id"] = ChildID;
				root["result"][ii]["type"] = MySensorsBase::GetMySensorsPresentationTypeStr((MySensorsBase::_ePresentationType)atoi(sd2[1].c_str()));
				root["result"][ii]["name"] = sd2[2];
				root["result"][ii]["use_ack"] = (sd2[3] != "0") ? "true" : "false";
				root["result"][ii]["ack_timeout"] = atoi(sd2[4].c_str());
				std::string szDate = "-";
				std::string szValues = "";
				if (pNode != NULL)
				{
					MySensorsBase::_tMySensorChild*  pChild = pNode->FindChild(ChildID);
					if (pChild != NULL)
					{
						std::vector<MySensorsBase::_eSetType> ctypes = pChild->GetChildValueTypes();
						std::vector<std::string> cvalues = pChild->GetChildValues();
						size_t iVal;
						for (iVal = 0; iVal < ctypes.size(); iVal++)
						{
							if (!szValues.empty())
								szValues += ", ";
							szValues += MySensorsBase::GetMySensorsValueTypeStr(ctypes[iVal]);
							szValues += " (";
							szValues += cvalues[iVal];
							szValues += ")";
						}
						if (!szValues.empty())
						{
							szValues.insert(0, "#" + boost::lexical_cast<std::string>(pChild->groupID) + ". ");
						}
						if (pChild->lastreceived != 0)
						{
							char szTmp[100];
							struct tm loctime;
							localtime_r(&pChild->lastreceived, &loctime);
							strftime(szTmp, 80, "%Y-%m-%d %X", &loctime);
							szDate = szTmp;
						}
					}
				}
				root["result"][ii]["Values"] = szValues;
				root["result"][ii]["LastReceived"] = szDate;
				ii++;
			}
		}
		void CWebServer::Cmd_MySensorsUpdateNode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string nodeid = request::findValue(&req, "nodeid");
			std::string name = request::findValue(&req, "name");
			if (
				(hwid == "") ||
				(nodeid == "") ||
				(name == "")
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (
				(pBaseHardware->HwdType != HTYPE_MySensorsUSB) &&
				(pBaseHardware->HwdType != HTYPE_MySensorsTCP) &&
				(pBaseHardware->HwdType != HTYPE_MySensorsMQTT)
				)
				return;
			MySensorsBase *pMySensorsHardware = reinterpret_cast<MySensorsBase*>(pBaseHardware);
			int NodeID = atoi(nodeid.c_str());
			root["status"] = "OK";
			root["title"] = "MySensorsUpdateNode";
			pMySensorsHardware->UpdateNode(NodeID, name);
		}
		void CWebServer::Cmd_MySensorsRemoveNode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string nodeid = request::findValue(&req, "nodeid");
			if (
				(hwid == "") ||
				(nodeid == "")
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (
				(pBaseHardware->HwdType != HTYPE_MySensorsUSB) &&
				(pBaseHardware->HwdType != HTYPE_MySensorsTCP) &&
				(pBaseHardware->HwdType != HTYPE_MySensorsMQTT)
				)
				return;
			MySensorsBase *pMySensorsHardware = reinterpret_cast<MySensorsBase*>(pBaseHardware);
			int NodeID = atoi(nodeid.c_str());
			root["status"] = "OK";
			root["title"] = "MySensorsRemoveNode";
			pMySensorsHardware->RemoveNode(NodeID);
		}
		void CWebServer::Cmd_MySensorsRemoveChild(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string nodeid = request::findValue(&req, "nodeid");
			std::string childid = request::findValue(&req, "childid");
			if (
				(hwid == "") ||
				(nodeid == "") ||
				(childid == "")
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (
				(pBaseHardware->HwdType != HTYPE_MySensorsUSB) &&
				(pBaseHardware->HwdType != HTYPE_MySensorsTCP) &&
				(pBaseHardware->HwdType != HTYPE_MySensorsMQTT)
				)
				return;
			MySensorsBase *pMySensorsHardware = reinterpret_cast<MySensorsBase*>(pBaseHardware);
			int NodeID = atoi(nodeid.c_str());
			int ChildID = atoi(childid.c_str());
			root["status"] = "OK";
			root["title"] = "MySensorsRemoveChild";
			pMySensorsHardware->RemoveChild(NodeID,ChildID);
		}
		void CWebServer::Cmd_MySensorsUpdateChild(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string nodeid = request::findValue(&req, "nodeid");
			std::string childid = request::findValue(&req, "childid");
			std::string useack = request::findValue(&req, "useack");
			std::string ackTimeout = request::findValue(&req, "acktimeout");
			if (
				(hwid.empty()) ||
				(nodeid.empty()) ||
				(childid.empty()) ||
				(useack.empty()) ||
				(ackTimeout.empty())
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (
				(pBaseHardware->HwdType != HTYPE_MySensorsUSB) &&
				(pBaseHardware->HwdType != HTYPE_MySensorsTCP) &&
				(pBaseHardware->HwdType != HTYPE_MySensorsMQTT)
				)
				return;
			MySensorsBase *pMySensorsHardware = reinterpret_cast<MySensorsBase*>(pBaseHardware);
			int NodeID = atoi(nodeid.c_str());
			int ChildID = atoi(childid.c_str());
			root["status"] = "OK";
			root["title"] = "MySensorsUpdateChild";
			bool bUseAck = (useack == "true") ? true : false;
			int iAckTimeout = atoi(ackTimeout.c_str());
			if (iAckTimeout < 100)
				iAckTimeout = 100;
			pMySensorsHardware->UpdateChild(NodeID, ChildID, bUseAck, iAckTimeout);
		}
	}
}
