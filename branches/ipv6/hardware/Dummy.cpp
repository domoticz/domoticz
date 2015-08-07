#include "stdafx.h"
#include "Dummy.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include "../json/json.h"
#include "hardwaretypes.h"

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
	return true;
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::RType_CreateVirtualSensor(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			std::string ssensortype = m_pWebEm->FindValue("sensortype");
			if ((idx == "") || (ssensortype == ""))
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
				nid = atol(result[0][0].c_str());
			}
			nid += 82000;
			char ID[40];
			sprintf(ID, "%ld", nid);

			std::string devname;

			switch (iSensorType)
			{
			case 1:
				//Pressure (Bar)
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeGeneral, sTypePressure, 12, 255, 0, "0.0", devname);
				bCreated = true;
			}
			break;
			case 2:
				//Percentage
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeGeneral, sTypePercentage, 12, 255, 0, "0.0", devname);
				bCreated = true;
			}
			break;
			case 3:
				//Gas
				m_sql.UpdateValue(HwdID, ID, 1, pTypeP1Gas, sTypeP1Gas, 12, 255, 0, "0", devname);
				bCreated = true;
				break;
			case 4:
				//Voltage
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeGeneral, sTypeVoltage, 12, 255, 0, "0.000", devname);
				bCreated = true;
			}
			break;
			case 5:
				//Text
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeGeneral, sTypeTextStatus, 12, 255, 0, "Hello World", devname);
				bCreated = true;
			}
			break;
			case 6:
				//Switch
			{
				unsigned char ID1 = (unsigned char)((nid & 0xFF000000) >> 24);
				unsigned char ID2 = (unsigned char)((nid & 0x00FF0000) >> 16);
				unsigned char ID3 = (unsigned char)((nid & 0x0000FF00) >> 8);
				unsigned char ID4 = (unsigned char)((nid & 0x000000FF));
				sprintf(ID, "%X%02X%02X%02X", ID1, ID2, ID3, ID4);
				m_sql.UpdateValue(HwdID, ID, 1, pTypeLighting2, sTypeAC, 12, 255, 0, "15", devname);
				bCreated = true;
			}
			break;
			case 7:
				//Alert
				m_sql.UpdateValue(HwdID, ID, 1, pTypeGeneral, sTypeAlert, 12, 255, 0, "No Alert!", devname);
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
			m_sql.UpdateValue(HwdID, ID, 1, pTypeThermostat, sTypeThermSetpoint, 12, 255, 0, "20.5", devname);
			bCreated = true;
			break;
			case 9:
				//Current/Ampere
				m_sql.UpdateValue(HwdID, ID, 1, pTypeCURRENT, sTypeELEC1, 12, 255, 0, "0.0;0.0;0.0", devname);
				bCreated = true;
				break;
			case 10:
				//Sound Level
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeGeneral, sTypeSoundLevel, 12, 255, 0, "65", devname);
				bCreated = true;
			}
			break;
			case 11:
				//Barometer (hPa)
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeGeneral, sTypeBaro, 12, 255, 0, "1021.34;0", devname);
				bCreated = true;
			}
			break;
			case 12:
				//Visibility (km)
				m_sql.UpdateValue(HwdID, ID, 1, pTypeGeneral, sTypeVisibility, 12, 255, 0, "10.3", devname);
				bCreated = true;
				break;
			case 13:
				//Distance (cm)
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				m_sql.UpdateValue(HwdID, rID.c_str(), 1, pTypeGeneral, sTypeDistance, 12, 255, 0, "123.4", devname);
				bCreated = true;
			}
			break;
			case 14: //Counter Incremental
				m_sql.UpdateValue(HwdID, ID, 1, pTypeGeneral, sTypeCounterIncremental, 12, 255, 0, "0", devname);
				bCreated = true;
				break;
			case pTypeTEMP:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeTEMP, sTypeTEMP1, 10, 255, 0, "0.0", devname);
				bCreated = true;
				break;
			case pTypeHUM:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeHUM, sTypeTEMP1, 10, 255, 50, "1", devname);
				bCreated = true;
				break;
			case pTypeTEMP_HUM:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeTEMP_HUM, sTypeTH1, 10, 255, 0, "0.0;50;1", devname);
				bCreated = true;
				break;
			case pTypeTEMP_HUM_BARO:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeTEMP_HUM_BARO, sTypeTHB1, 10, 255, 0, "0.0;50;1;1010;1", devname);
				bCreated = true;
				break;
			case pTypeWIND:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeWIND, sTypeWIND1, 10, 255, 0, "0;N;0;0;0;0", devname);
				bCreated = true;
				break;
			case pTypeRAIN:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeRAIN, sTypeRAIN3, 10, 255, 0, "0;0", devname);
				bCreated = true;
				break;
			case pTypeUV:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeUV, sTypeUV1, 10, 255, 0, "0;0", devname);
				bCreated = true;
				break;
			case pTypeENERGY:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeENERGY, sTypeELEC2, 10, 255, 0, "0;0.0", devname);
				bCreated = true;
				break;
			case pTypeRFXMeter:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeRFXMeter, sTypeRFXMeterCount, 10, 255, 0, "0", devname);
				bCreated = true;
				break;
			case pTypeAirQuality:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeAirQuality, sTypeVoltcraft, 10, 255, 0, devname);
				bCreated = true;
				break;
			case pTypeUsage:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeUsage, sTypeElectric, 10, 255, 0, "0", devname);
				bCreated = true;
				break;
			case pTypeLux:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeLux, sTypeLux, 10, 255, 0, "0", devname);
				bCreated = true;
				break;
			case pTypeP1Power:
				m_sql.UpdateValue(HwdID, ID, 1, pTypeP1Power, sTypeP1Power, 10, 255, 0, "0;0;0;0;0;0", devname);
				bCreated = true;
				break;
			}
			if (bCreated)
			{
				root["status"] = "OK";
				root["title"] = "CreateVirtualSensor";
			}
		}
	}
}
