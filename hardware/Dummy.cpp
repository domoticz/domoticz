#include "stdafx.h"
#include "Dummy.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include "../json/json.h"
#include "hardwaretypes.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <sstream>

CDummy::CDummy(const int ID)
{
	m_HwdID=ID;
	m_bSkipReceiveCheck = true;
}

CDummy::~CDummy(void)
{
	m_bIsStarted=false;
}

void CDummy::Init()
{
}

bool CDummy::StartHardware()
{
	Init();
	m_bIsStarted=true;
	sOnConnected(this);
	return true;
}

bool CDummy::StopHardware()
{
	m_bIsStarted=false;
    return true;
}

bool CDummy::WriteToHardware(const char *pdata, const unsigned char length)
{
#ifdef _DEBUG
	if (length < 2)
		return false;
	std::string sdevicetype = RFX_Type_Desc(pdata[1], 1);
	if (pdata[1] == pTypeGeneral)
	{
		const _tGeneralDevice *pMeter = reinterpret_cast<const _tGeneralDevice*>(pdata);
		sdevicetype += "/" + std::string(RFX_Type_SubType_Desc(pMeter->type, pMeter->subtype));
	}
	_log.Log(LOG_STATUS, "Dummy: Received null operation for %s", sdevicetype.c_str());
#endif
	return true;
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::RType_CreateVirtualSensor(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string ssensorname = request::findValue(&req, "sensorname");
			std::string ssensortype = request::findValue(&req, "sensortype");
			std::string soptions = request::findValue(&req, "sensoroptions");
			if ((idx == "") || (ssensortype.empty()) || (ssensorname.empty()))
				return;

			bool bCreated = false;
			int iSensorType = atoi(ssensortype.c_str());

			int HwdID = atoi(idx.c_str());

			//Make a unique number for ID
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT MAX(ID) FROM DeviceStatus");

			unsigned long nid = 1; //could be the first device ever

			if (result.size() > 0)
			{
				nid = atol(result[0][0].c_str()) + 1;
			}
			unsigned long vs_idx = nid; // OTO keep idx to be returned before masking
			nid += 82000;
			char ID[40];
			sprintf(ID, "%lu", nid);

			std::string devname;

			bool bPrevAcceptNewHardware = m_sql.m_bAcceptNewHardware;
			m_sql.m_bAcceptNewHardware = true;
			uint64_t DeviceRowIdx = -1;
			switch (iSensorType)
			{
			case 1:
				//Pressure (Bar)
				{
					std::string rID = std::string(ID);
					padLeft(rID, 8, '0');
					DeviceRowIdx=DeviceRowIdx=m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeGeneral, sTypePressure, 12, 255, 0, "0.0", devname);
					bCreated = true;
				}
				break;
			case 2:
				//Percentage
				{
					std::string rID = std::string(ID);
					padLeft(rID, 8, '0');
					DeviceRowIdx=m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeGeneral, sTypePercentage, 12, 255, 0, "0.0", devname);
					bCreated = true;
				}
				break;
			case 3:
				//Gas
				DeviceRowIdx=m_sql.UpdateValue(HwdID, ID, 1, pTypeP1Gas, sTypeP1Gas, 12, 255, 0, "0", devname);
				bCreated = true;
				break;
			case 4:
				//Voltage
				{
					std::string rID = std::string(ID);
					padLeft(rID, 8, '0');
					DeviceRowIdx=m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeGeneral, sTypeVoltage, 12, 255, 0, "0.000", devname);
					bCreated = true;
				}
				break;
			case 5:
				//Text
				{
					std::string rID = std::string(ID);
					padLeft(rID, 8, '0');
					DeviceRowIdx=m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeGeneral, sTypeTextStatus, 12, 255, 0, "Hello World", devname);
					bCreated = true;
				}
				break;
			case 6:
				//Switch
				{
					sprintf(ID, "%08lX", nid);
					DeviceRowIdx=m_sql.UpdateValue(HwdID, ID, 1, pTypeGeneralSwitch, sSwitchGeneralSwitch, 12, 255, 0, "100", devname);
					bCreated = true;
				}
				break;
			case 7:
				//Alert
				DeviceRowIdx=m_sql.UpdateValue(HwdID, ID, 1, pTypeGeneral, sTypeAlert, 12, 255, 0, "No Alert!", devname);
				bCreated = true;
				break;
			case 8:
				//Thermostat Setpoint
				{
					unsigned char ID1 = (unsigned char)((nid & 0xFF000000) >> 24);
					unsigned char ID2 = (unsigned char)((nid & 0x00FF0000) >> 16);
					unsigned char ID3 = (unsigned char)((nid & 0x0000FF00) >> 8);
					unsigned char ID4 = (unsigned char)((nid & 0x000000FF));
					sprintf(ID, "%X%02X%02X%02X", ID1, ID2, ID3, ID4);
				}
				DeviceRowIdx=m_sql.UpdateValue(HwdID, ID, 1, pTypeThermostat, sTypeThermSetpoint, 12, 255, 0, "20.5", devname);
				bCreated = true;
				break;
			case 9:
				//Current/Ampere
				DeviceRowIdx=m_sql.UpdateValue(HwdID, ID, 1, pTypeCURRENT, sTypeELEC1, 12, 255, 0, "0.0;0.0;0.0", devname);
				bCreated = true;
				break;
			case 10:
				//Sound Level
				{
					std::string rID = std::string(ID);
					padLeft(rID, 8, '0');
					DeviceRowIdx=m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeGeneral, sTypeSoundLevel, 12, 255, 0, "65", devname);
					bCreated = true;
				}
				break;
			case 11:
				//Barometer (hPa)
				{
					std::string rID = std::string(ID);
					padLeft(rID, 8, '0');
					DeviceRowIdx=m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeGeneral, sTypeBaro, 12, 255, 0, "1021.34;0", devname);
					bCreated = true;
				}
				break;
			case 12:
				//Visibility (km)
				DeviceRowIdx=m_sql.UpdateValue(HwdID, ID, 1, pTypeGeneral, sTypeVisibility, 12, 255, 0, "10.3", devname);
				bCreated = true;
				break;
			case 13:
				//Distance (cm)
				{
					std::string rID = std::string(ID);
					padLeft(rID, 8, '0');
					DeviceRowIdx=m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeGeneral, sTypeDistance, 12, 255, 0, "123.4", devname);
					bCreated = true;
				}
				break;
			case 14: //Counter Incremental
				DeviceRowIdx=m_sql.UpdateValue(HwdID, ID, 1, pTypeGeneral, sTypeCounterIncremental, 12, 255, 0, "0", devname);
				bCreated = true;
				break;
			case 15:
				//Soil Moisture
				{
					std::string rID = std::string(ID);
					padLeft(rID, 8, '0');
					DeviceRowIdx=m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeGeneral, sTypeSoilMoisture, 12, 255, 3, devname);
					bCreated = true;
				}
				break;
			case 16:
				//Leaf Wetness
				{
					std::string rID = std::string(ID);
					padLeft(rID, 8, '0');
					DeviceRowIdx=m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeGeneral, sTypeLeafWetness, 12, 255, 2, devname);
					bCreated = true;
				}
				break;
			case 17:
				//Thermostat Clock
				{
					std::string rID = std::string(ID);
					padLeft(rID, 8, '0');
					DeviceRowIdx=m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeGeneral, sTypeZWaveClock, 12, 255, 0, "24:12:00", devname);
					bCreated = true;
				}
				break;
			case 18:
				//kWh
				{
					std::string rID = std::string(ID);
					padLeft(rID, 8, '0');
					DeviceRowIdx=m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeGeneral, sTypeKwh, 12, 255, 0, "0;0.0", devname);
					bCreated = true;
				}
				break;
			case 19:
				//Current (Single)
				{
					std::string rID = std::string(ID);
					padLeft(rID, 8, '0');
					DeviceRowIdx=m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeGeneral, sTypeCurrent, 12, 255, 0, "6.4", devname);
					bCreated = true;
				}
				break;
			case 20:
				//Solar Radiation
				{
					std::string rID = std::string(ID);
					padLeft(rID, 8, '0');
					DeviceRowIdx = m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeGeneral, sTypeSolarRadiation, 12, 255, 0, "1.0", devname);
					bCreated = true;
				}
				break;
			case pTypeLimitlessLights:
				//RGB switch
				{
					std::string rID = std::string(ID);
					padLeft(rID, 8, '0');
					DeviceRowIdx=m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeLimitlessLights, sTypeLimitlessRGB, 12, 255, 1, devname);
					if (DeviceRowIdx != -1)
					{
						//Set switch type to dimmer
						m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d WHERE (ID==%" PRIu64 ")", STYPE_Dimmer, DeviceRowIdx);
					}
					bCreated = true;
				}
				break;
			case pTypeTEMP:
				DeviceRowIdx=m_sql.UpdateValue(HwdID, ID, 1, pTypeTEMP, sTypeTEMP5, 12, 255, 0, "0.0", devname);
				bCreated = true;
				break;
			case pTypeTEMP_BARO:
				DeviceRowIdx = m_sql.UpdateValue(HwdID, ID, 1, pTypeTEMP_BARO, sTypeBMP085, 12, 255, 0, "0.0;1038.0;0;188.0", devname);
				bCreated = true;
				break;
			case pTypeWEIGHT:
				DeviceRowIdx=m_sql.UpdateValue(HwdID, ID, 1, pTypeWEIGHT, sTypeWEIGHT1, 12, 255, 0, "0.0", devname);
				bCreated = true;
				break;
			case pTypeHUM:
				DeviceRowIdx=m_sql.UpdateValue(HwdID, ID, 1, pTypeHUM, sTypeHUM1, 12, 255, 50, "1", devname);
				bCreated = true;
				break;
			case pTypeTEMP_HUM:
				DeviceRowIdx=m_sql.UpdateValue(HwdID, ID, 1, pTypeTEMP_HUM, sTypeTH1, 12, 255, 0, "0.0;50;1", devname);
				bCreated = true;
				break;
			case pTypeTEMP_HUM_BARO:
				DeviceRowIdx=m_sql.UpdateValue(HwdID, ID, 1, pTypeTEMP_HUM_BARO, sTypeTHB1, 12, 255, 0, "0.0;50;1;1010;1", devname);
				bCreated = true;
				break;
			case pTypeWIND:
				DeviceRowIdx=m_sql.UpdateValue(HwdID, ID, 1, pTypeWIND, sTypeWIND1, 12, 255, 0, "0;N;0;0;0;0", devname);
				bCreated = true;
				break;
			case pTypeRAIN:
				DeviceRowIdx=m_sql.UpdateValue(HwdID, ID, 1, pTypeRAIN, sTypeRAIN3, 12, 255, 0, "0;0", devname);
				bCreated = true;
				break;
			case pTypeUV:
				DeviceRowIdx=m_sql.UpdateValue(HwdID, ID, 1, pTypeUV, sTypeUV1, 12, 255, 0, "0;0", devname);
				bCreated = true;
				break;
			case pTypeRFXMeter:
				DeviceRowIdx=m_sql.UpdateValue(HwdID, ID, 1, pTypeRFXMeter, sTypeRFXMeterCount, 10, 255, 0, "0", devname);
				bCreated = true;
				break;
			case pTypeAirQuality:
				DeviceRowIdx=m_sql.UpdateValue(HwdID, ID, 1, pTypeAirQuality, sTypeVoltcraft, 12, 255, 0, devname);
				bCreated = true;
				break;
			case pTypeUsage:
				DeviceRowIdx=m_sql.UpdateValue(HwdID, ID, 1, pTypeUsage, sTypeElectric, 12, 255, 0, "0", devname);
				bCreated = true;
				break;
			case pTypeLux:
				DeviceRowIdx=m_sql.UpdateValue(HwdID, ID, 1, pTypeLux, sTypeLux, 12, 255, 0, "0", devname);
				bCreated = true;
				break;
			case pTypeP1Power:
				DeviceRowIdx=m_sql.UpdateValue(HwdID, ID, 1, pTypeP1Power, sTypeP1Power, 12, 255, 0, "0;0;0;0;0;0", devname);
				bCreated = true;
				break;
			case 1000:
				//Waterflow
				{
					std::string rID = std::string(ID);
					padLeft(rID, 8, '0');
					DeviceRowIdx=m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeGeneral, sTypeWaterflow, 12, 255, 0, "0.0", devname);
					bCreated = true;
				}
				break;
			case 1001:
				//Wind + Temp + Chill
				DeviceRowIdx=m_sql.UpdateValue(HwdID, ID, 1, pTypeWIND, sTypeWIND4, 12, 255, 0, "0;N;0;0;0;0", devname);
				bCreated = true;
				break;
			case 1002:
				//Selector Switch
				{
					unsigned char ID1 = (unsigned char)((nid & 0xFF000000) >> 24);
					unsigned char ID2 = (unsigned char)((nid & 0x00FF0000) >> 16);
					unsigned char ID3 = (unsigned char)((nid & 0x0000FF00) >> 8);
					unsigned char ID4 = (unsigned char)((nid & 0x000000FF));
					sprintf(ID, "%02X%02X%02X%02X", ID1, ID2, ID3, ID4);
					DeviceRowIdx=m_sql.UpdateValue(HwdID, ID, 1, pTypeGeneralSwitch, sSwitchTypeSelector, 12, 255, 0, "0", devname);
					if (DeviceRowIdx != -1)
					{
						//Set switch type to selector
						m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d WHERE (ID==%" PRIu64 ")", STYPE_Selector, DeviceRowIdx);
						//Set default device options
						m_sql.SetDeviceOptions(DeviceRowIdx, m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|Level1|Level2|Level3", false));
					}
					bCreated = true;
				}
				break;
			case 1003:
				//RGBW switch
				{
					std::string rID = std::string(ID);
					padLeft(rID, 8, '0');
					DeviceRowIdx = m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeLimitlessLights, sTypeLimitlessRGBW, 12, 255, 1, devname);
					if (DeviceRowIdx != -1)
					{
						//Set switch type to dimmer
						m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d WHERE (ID==%" PRIu64 ")", STYPE_Dimmer, DeviceRowIdx);
					}
					bCreated = true;
				}
				break;
			case 1004:
				//Custom
				if (!soptions.empty())
				{
					std::string rID = std::string(ID);
					padLeft(rID, 8, '0');
					DeviceRowIdx = m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeGeneral, sTypeCustom, 12, 255, 0, "0.0", devname);
					if (DeviceRowIdx != -1)
					{
						//Set the Label
						m_sql.safe_query("UPDATE DeviceStatus SET Options='%q' WHERE (ID==%" PRIu64 ")", soptions.c_str(), DeviceRowIdx);
					}
					bCreated = true;
				}
				break;
			}

			m_sql.m_bAcceptNewHardware = bPrevAcceptNewHardware;

			if (bCreated)
			{
				root["status"] = "OK";
				root["title"] = "CreateVirtualSensor";
				std::stringstream ss;
				ss << vs_idx;
				root["idx"] = ss.str().c_str();
			}
			if (DeviceRowIdx != -1)
			{
				m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', Used=1 WHERE (ID==%" PRIu64 ")", ssensorname.c_str(), DeviceRowIdx);
				m_mainworker.m_eventsystem.GetCurrentStates();
			}
		}
	}
}
