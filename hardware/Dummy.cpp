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

		struct _mappedsensorname {
			const int mappedvalue;
			const int type;
			const int subtype;
		};

		static const _mappedsensorname mappedsensorname[] =
		{
			{ 249, 0xF9, 0x01 },
			{ 7,   0xF3, 0x16 },
			{ 9,   0x59, 0x01 },
			{ 19,  0xF3, 0x17 },
			{ 11,  0xF3, 0x1A },
			{ 113, 0x71, 0x00 },
			{ 14,  0xF3, 0x1C },
			{ 1004,0xF3, 0x1F },
			{ 13,  0xF3, 0x1B },
			{ 18,  0xF3, 0x1D },
			{ 3,   0xFB, 0x02 },
			{ 81,  0x51, 0x01 },
			{ 16,  0xF3, 0x04 },
			{ 246, 0xF6, 0x01 },
			{ 250, 0xFA, 0x01 },
			{ 2,   0xF3, 0x06 },
			{ 1,   0xF3, 0x09 },
			{ 85,  0x55, 0x03 },
			{ 241, 0xF1, 0x02 },
			{ 1003,0xF1, 0x01 },
			{ 93,  0x5D, 0x01 },
			{ 1002,0xF4, 0x3E },
			{ 15,  0xF3, 0x03 },
			{ 20,  0xF3, 0x02 },
			{ 10,  0xF3, 0x18 },
			{ 6,   0xF4, 0x49 },
			{ 80,  0x50, 0x05 },
			{ 82,  0x51, 0x01 },
			{ 84,  0x54, 0x01 },
			{ 247, 0xF7, 0x01 },
			{ 5,   0xF3, 0x13 },
			{ 8,   0xF2, 0x01 },
			{ 248, 0xF8, 0x01 },
			{ 87,  0x57, 0x01 },
			{ 12,  0xF3, 0x01 },
			{ 4,   0xF3, 0x08 },
			{ 1000,0xF3, 0x1E },
			{ 86,  0x56, 0x01 },
			{ 1001,0x56, 0x04 }

		};

		void CWebServer::RType_CreateMappedSensor(WebEmSession & session, const request& req, Json::Value &root)
		{ // deprecated (for dzVents). Use RType_CreateDevice
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

			int sensortype = atoi(ssensortype.c_str());
			unsigned int type = 0;
			unsigned int subType = 0;
			uint64_t DeviceRowIdx = -1;

			for (int i = 0; i < sizeof(mappedsensorname) / sizeof(mappedsensorname[0]); i++)
			{
				if (mappedsensorname[i].mappedvalue == sensortype)
				{
					type = mappedsensorname[i].type;
					subType = mappedsensorname[i].subtype;

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

					bool bPrevAcceptNewHardware = m_sql.m_bAcceptNewHardware;
					m_sql.m_bAcceptNewHardware = true;

					DeviceRowIdx = m_sql.CreateDevice(HwdID, type, subType, ssensorname, nid, soptions);

					m_sql.m_bAcceptNewHardware = bPrevAcceptNewHardware;

					if (DeviceRowIdx != -1)
					{
						root["status"] = "OK";
						root["title"] = "CreateVirtualSensor";
						std::stringstream ss;
						ss << vs_idx;
						root["idx"] = ss.str().c_str();
					}

					break;
				}
			}
		}

		void CWebServer::RType_CreateDevice(WebEmSession & session, const request& req, Json::Value &root)
		{
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

			if ((idx == "") || (ssensorname.empty()))
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

			if (result.size() > 0)
			{
				nid = atol(result[0][0].c_str()) + 1;
			}
			unsigned long vs_idx = nid; // OTO keep idx to be returned before masking
			nid += 82000;

			bool bPrevAcceptNewHardware = m_sql.m_bAcceptNewHardware;
			m_sql.m_bAcceptNewHardware = true;

			uint64_t DeviceRowIdx = m_sql.CreateDevice(HwdID, type, subType, ssensorname, nid, soptions);

			m_sql.m_bAcceptNewHardware = bPrevAcceptNewHardware;

			if (DeviceRowIdx != -1)
			{
				root["status"] = "OK";
				root["title"] = "CreateSensor";
				std::stringstream ss;
				ss << vs_idx;
				root["idx"] = ss.str().c_str();
			}
		}

	}
}

