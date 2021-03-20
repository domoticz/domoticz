#include "stdafx.h"
#include "Dummy.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include <json/json.h>
#include "hardwaretypes.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <sstream>

CDummy::CDummy(const int ID)
{
	m_HwdID=ID;
	m_bSkipReceiveCheck = true;
}

CDummy::~CDummy()
{
	m_bIsStarted = false;
}

void CDummy::Init()
{
}

bool CDummy::StartHardware()
{
	RequestStart();

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
	Log(LOG_STATUS, "Received null operation for %s", sdevicetype.c_str());
#endif
	return true;
}

//Webserver helpers
namespace http {
	namespace server {
		using _mappedsensorname = struct
		{
			int mappedvalue;
			int type;
			int subtype;
		};

		constexpr auto mappedsensorname = std::array<_mappedsensorname, 40>{
			_mappedsensorname{ 249, pTypeAirQuality, sTypeVoltcraft },	    // Air Quality
			_mappedsensorname{ 7, pTypeGeneral, sTypeAlert },		    // Alert
			_mappedsensorname{ 9, pTypeCURRENT, sTypeELEC1 },		    // Ampere (3 Phase)
			_mappedsensorname{ 19, pTypeGeneral, sTypeCurrent },		    // Ampere (1 Phase)
			_mappedsensorname{ 11, pTypeGeneral, sTypeBaro },		    // Barometer
			_mappedsensorname{ 113, pTypeRFXMeter, sTypeRFXMeterCount },	    // Counter
			_mappedsensorname{ 14, pTypeGeneral, sTypeCounterIncremental },	    // Counter Incremental
			_mappedsensorname{ 1004, pTypeGeneral, sTypeCustom },		    // Custom Sensor
			_mappedsensorname{ 13, pTypeGeneral, sTypeDistance },		    // Distance
			_mappedsensorname{ 18, pTypeGeneral, sTypeKwh },		    // Electric (Instant+Counter)
			_mappedsensorname{ 3, pTypeP1Gas, sTypeP1Gas },			    // Gas
			_mappedsensorname{ 81, pTypeHUM, sTypeHUM1 },			    // Humidity
			_mappedsensorname{ 16, pTypeGeneral, sTypeLeafWetness },	    // Leaf Wetness
			_mappedsensorname{ 246, pTypeLux, sTypeLux },			    // Lux
			_mappedsensorname{ 250, pTypeP1Power, sTypeP1Power },		    // P1 Smart Meter (Electric)
			_mappedsensorname{ 1005, pTypeGeneral, sTypeManagedCounter },	    // Managed Counter
			_mappedsensorname{ 2, pTypeGeneral, sTypePercentage },		    // Percentage
			_mappedsensorname{ 1, pTypeGeneral, sTypePressure },		    // Pressure (Bar)
			_mappedsensorname{ 85, pTypeRAIN, sTypeRAIN3 },			    // Rain
			_mappedsensorname{ 241, pTypeColorSwitch, sTypeColor_RGB },	    // RGB Switch
			_mappedsensorname{ 1003, pTypeColorSwitch, sTypeColor_RGB_W },	    // RGBW Switch
			_mappedsensorname{ 93, pTypeWEIGHT, sTypeWEIGHT1 },		    // Scale
			_mappedsensorname{ 1002, pTypeGeneralSwitch, sSwitchTypeSelector }, // Selector Switch
			_mappedsensorname{ 15, pTypeGeneral, sTypeSoilMoisture },	    // Soil Moisture
			_mappedsensorname{ 20, pTypeGeneral, sTypeSolarRadiation },	    // Solar Radiation
			_mappedsensorname{ 10, pTypeGeneral, sTypeSoundLevel },		    // Sound Level
			_mappedsensorname{ 6, pTypeGeneralSwitch, sSwitchGeneralSwitch },   // Switch
			_mappedsensorname{ 80, pTypeTEMP, sTypeTEMP5 },			    // Temperature
			_mappedsensorname{ 82, pTypeTEMP_HUM, sTypeTH1 },		    // Temp+Hum
			_mappedsensorname{ 84, pTypeTEMP_HUM_BARO, sTypeTHB1 },		    // Temp+Hum+Baro
			_mappedsensorname{ 247, pTypeTEMP_BARO, sTypeBMP085 },		    // Temp+Baro
			_mappedsensorname{ 5, pTypeGeneral, sTypeTextStatus },		    // Text
			_mappedsensorname{ 8, pTypeThermostat, sTypeThermSetpoint },	    // Thermostat Setpoint
			_mappedsensorname{ 248, pTypeUsage, sTypeElectric },		    // Usage (Electric)
			_mappedsensorname{ 87, pTypeUV, sTypeUV1 },			    // UV
			_mappedsensorname{ 12, pTypeGeneral, sTypeVisibility },		    // Visibility
			_mappedsensorname{ 4, pTypeGeneral, sTypeVoltage },		    // Voltage
			_mappedsensorname{ 1000, pTypeGeneral, sTypeWaterflow },	    // Waterflow
			_mappedsensorname{ 86, pTypeWIND, sTypeWIND1 },			    // Wind
			_mappedsensorname{ 1001, pTypeWIND, sTypeWIND4 }		    // Wind+Temp+Chill
		};

		//TODO: Is this function called from anywhere, or can it be removed?
		void CWebServer::RType_CreateMappedSensor(WebEmSession & session, const request& req, Json::Value &root)
		{ // deprecated (for dzVents). Use RType_CreateDevice
			std::string Username = "Admin";
			if (!session.username.empty())
				Username = session.username;
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string ssensorname = request::findValue(&req, "sensorname");
			std::string ssensortype = request::findValue(&req, "sensortype");
			std::string soptions = request::findValue(&req, "sensoroptions");

			if ((idx.empty()) || (ssensortype.empty()) || (ssensorname.empty()))
				return;

			int sensortype = atoi(ssensortype.c_str());
			unsigned int type = 0;
			unsigned int subType = 0;
			uint64_t DeviceRowIdx = (uint64_t )-1;

			for (const auto &sensor : mappedsensorname)
			{
				if (sensor.mappedvalue == sensortype)
				{
					type = sensor.type;
					subType = sensor.subtype;

					int HwdID = atoi(idx.c_str());

					//Make a unique number for ID
					std::vector<std::vector<std::string> > result;
					result = m_sql.safe_query("SELECT MAX(ID) FROM DeviceStatus");

					unsigned long nid = 1; //could be the first device ever

					if (!result.empty())
					{
						nid = atol(result[0][0].c_str()) + 1;
					}
					unsigned long vs_idx = nid; // OTO keep idx to be returned before masking
					nid += 82000;

					bool bPrevAcceptNewHardware = m_sql.m_bAcceptNewHardware;
					m_sql.m_bAcceptNewHardware = true;

					std::string szCreateUser = Username + " (IP: " + session.remote_host + ")";
					DeviceRowIdx = m_sql.CreateDevice(HwdID, type, subType, ssensorname, nid, soptions, szCreateUser);

					m_sql.m_bAcceptNewHardware = bPrevAcceptNewHardware;

					if (DeviceRowIdx != (uint64_t)-1)
					{
						root["status"] = "OK";
						root["title"] = "CreateVirtualSensor";
						root["idx"] = std::to_string(vs_idx);
					}
					break;
				}
			}
		}

		void CWebServer::RType_CreateDevice(WebEmSession & session, const request& req, Json::Value &root)
		{
			std::string Username = "Admin";
			if (!session.username.empty())
				Username = session.username;
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string ssensorname = request::findValue(&req, "sensorname");
			std::string soptions = request::findValue(&req, "sensoroptions");

			std::string ssensormappedtype = request::findValue(&req, "sensormappedtype");

			std::string devicetype = request::findValue(&req, "devicetype");
			std::string devicesubtype = request::findValue(&req, "devicesubtype");

			if ((idx.empty()) || (ssensorname.empty()))
				return;

			unsigned int type;
			unsigned int subType;

			if (!ssensormappedtype.empty())
			{ // for creating dummy from web (for example ssensormappedtype=0xF401)
				uint16_t fullType;
				std::stringstream ss;
				ss << std::hex << ssensormappedtype;
				ss >> fullType;

				type = fullType >> 8;
				subType = fullType & 0xFF;
			}
			else
				if (!devicetype.empty() && !devicesubtype.empty())
				{ // for creating any device (type=x&subtype=y) from json api or code
					type = atoi(devicetype.c_str());
					subType = atoi(devicesubtype.c_str());
				}
				else
					return;

			int HwdID = atoi(idx.c_str());

			//Make a unique number for ID
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT MAX(ID) FROM DeviceStatus");

			unsigned long nid = 1; //could be the first device ever

			if (!result.empty())
			{
				nid = atol(result[0][0].c_str()) + 1;
			}
			unsigned long vs_idx = nid; // OTO keep idx to be returned before masking
			nid += 82000;

			bool bPrevAcceptNewHardware = m_sql.m_bAcceptNewHardware;
			m_sql.m_bAcceptNewHardware = true;

			std::string szCreateUser = Username + " (IP: " + session.remote_host + ")";
			uint64_t DeviceRowIdx = m_sql.CreateDevice(HwdID, type, subType, ssensorname, nid, soptions, szCreateUser);

			m_sql.m_bAcceptNewHardware = bPrevAcceptNewHardware;

			if (DeviceRowIdx != (uint64_t)-1)
			{
				root["status"] = "OK";
				root["title"] = "CreateSensor";
				root["idx"] = std::to_string(vs_idx);
			}
		}

	} // namespace server
} // namespace http
