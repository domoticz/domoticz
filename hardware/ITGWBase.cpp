
#include "stdafx.h"
#include "ITGWBase.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/SQLHelper.h"

#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include "../json/json.h"
#include "itgw/ITGWSwitchIntertechno.h"
#include "itgw/ITGWSwitchBrennenstuhl.h"

#define _DEBUG
#ifdef _DEBUG
	#define ENABLE_LOGGING
#endif


struct _tITGWStringIntHelper
{
	std::string szType;
	int gType;
};

const _tITGWStringIntHelper rfswitches[] =
{
	{ "Intertechno 2 Dials relay", sSwitchTypeInterTechno },               // p9
	{ "Brennestuh Dipswith relay", sSwitchTypeBrennenstuhl },              // p3
	{ "RFCustom", sSwitchTypeRFCustom },
	{ "", -1 }
};

const _tITGWStringIntHelper rfswitchcommands[] =
{
	{ "ON", gswitch_sOn },
	{ "OFF", gswitch_sOff },
	{ "", -1 }
};

const _tITGWStringIntHelper rfblindcommands[] =
{
	{ "UP", blinds_sOpen },
	{ "DOWN", blinds_sClose },
	{ "STOP", blinds_sStop },
	{ "CONFIRM", blinds_sConfirm },
	{ "LIMIT", blinds_sLimit },
	{ "", -1 }
};

const std::string txversion = "1,";
const std::string suffix = "0;";

int GetGeneralITGWFromString(const _tITGWStringIntHelper *pTable, const std::string &szType)
{
	int ii = 0;
	while (pTable[ii].gType!=-1)
	{
		if (pTable[ii].szType == szType)
			return pTable[ii].gType;
		ii++;
	}
	return -1;
}

std::string GetGeneralITGWFromInt(const _tITGWStringIntHelper *pTable, const int gType)
{
	int ii = 0;
	while (pTable[ii].gType!=-1)
	{
		if (pTable[ii].gType == gType)
			return pTable[ii].szType;
		ii++;
	}
	return "";
}

CITGWBase::CITGWBase()
{
	m_rfbufferpos=0;
	memset(&m_rfbuffer,0,sizeof(m_rfbuffer));
}

CITGWBase::~CITGWBase()
{
}


/*
void CITGWBase::Add2SendQueue(const char* pData, const size_t length)
{
	std::string sBytes;
	sBytes.insert(0,pData,length);
	boost::lock_guard<boost::mutex> l(m_sendMutex);
	m_sendqueue.push_back(sBytes);
}
*/

void CITGWBase::ParseData(const char *data, size_t len)
{
  ParseLine(data);
}

#define round(a) ( int ) ( a + .5 )

void GetITGWSwitchType(const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, int &switchType)
{
	switchType = 0;
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT SwitchType FROM DeviceStatus WHERE (DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)", ID, unit, devType, subType);
	if (result.size() != 0)
	{
		switchType = atoi(result[0][0].c_str());
	}
}


bool CITGWBase::WriteToHardware(const char *pdata, const unsigned char length)
{
	const _tGeneralSwitch *pSwitch = reinterpret_cast<const _tGeneralSwitch*>(pdata);


//	_log.Log(LOG_ERROR, "ITGW: type: %d", pSwitch->type);
//	_log.Log(LOG_ERROR, "ITGW: subtype: %d", pSwitch->subtype);

	if ((pSwitch->type != pTypeGeneralSwitch))
		return false; //only allowed to control regular switches

//	_log.Log(LOG_ERROR, "ITGW: switch type: %d", pSwitch->subtype);
	std::string switchtype = GetGeneralITGWFromInt(rfswitches, pSwitch->subtype);
	if (switchtype.empty())
	{
		_log.Log(LOG_ERROR, "ITGW: trying to send unknown switch type: %d", pSwitch->subtype);
		return false;
	}

	_log.Log(LOG_NORM, "ITGW: switch id %04X accessed", pSwitch->id);
//	_log.Log(LOG_ERROR, "ITGW: subtype: %d", pSwitch->subtype);

	// get SwitchType from SQL table
	int m_SwitchType = 0;
	char szDeviceID[20];
	sprintf(szDeviceID, "%04X", pSwitch->id);
	std::string ID = szDeviceID;
	GetITGWSwitchType(ID.c_str(), pSwitch->unitcode, pSwitch->type, pSwitch->subtype, m_SwitchType);
//	_log.Log(LOG_ERROR, "ITGW: switch type: %d", m_SwitchType);
//  _log.Log(LOG_ERROR, "ITGW: switch cmd: %d", pSwitch->cmnd);


	if (pSwitch->type == pTypeGeneralSwitch) {
		std::string switchcmnd = GetGeneralITGWFromInt(rfswitchcommands, pSwitch->cmnd);

		if (switchcmnd.empty()) {
			_log.Log(LOG_ERROR, "ITGW: trying to send unknown switch command: %d", pSwitch->cmnd);
			return false;
		}

		//Build send string
		std::stringstream sstr;
    if(pSwitch->subtype == sSwitchTypeInterTechno){
      ITGWSwitchIntertechno *its = new ITGWSwitchIntertechno();
      std::string itcmnd;
      if(pSwitch->cmnd == gswitch_sOn) {
        itcmnd = its->GetOnCommandString(ID.c_str());
      }
      else if(pSwitch->cmnd == gswitch_sOff) {
        itcmnd =  its->GetOffCommandString(ID.c_str());
      }
      sstr << its->GetHeader() << itcmnd << txversion << its->GetSpeed();
    }
    else if(pSwitch->subtype == sSwitchTypeBrennenstuhl){
      ITGWSwitchBrennenstuhl *its = new ITGWSwitchBrennenstuhl();
      std::string itcmnd;
      if(pSwitch->cmnd == gswitch_sOn) {
        itcmnd = its->GetOnCommandString(ID.c_str());

      }
      else if(pSwitch->cmnd == gswitch_sOff) {
        itcmnd =  its->GetOffCommandString(ID.c_str());
      }
      sstr << its->GetHeader() << itcmnd << txversion << its->GetSpeed();
    }

		sstr << suffix << "\n";

#ifdef _DEBUG
		_log.Log(LOG_STATUS, "ITGW Sending: %s", sstr.str().c_str());
#endif
		m_bTXokay = false; // clear OK flag
		WriteInt(sstr.str());
		time_t atime = mytime(NULL);
		time_t btime = mytime(NULL);

		// Wait for an OK response(ITGW just sends a reply) from ITGW to make sure the command was executed
		while (m_bTXokay == false) {
			if (difftime(btime,atime) > 4) {
				_log.Log(LOG_ERROR, "ITGW: TX time out...");
				return false;
			}
			btime = mytime(NULL);
		}
		return true;
	}
}

bool CITGWBase::SendSwitchInt(const unsigned char *pdata)
{
	sDecodeRXMessage(this, pdata, NULL, 0);
	return true;
}

bool CITGWBase::ParseLine(const std::string &sLine)
{
  //ITGW only ever sends its identifier string
	m_LastReceivedTime = mytime(NULL);

	std::vector<std::string> results;
	StringSplit(sLine, ";", results);

#ifdef ENABLE_LOGGING
		_log.Log(LOG_NORM, "ITGW: %s", sLine.c_str());
#endif

	//std::string Sensor_ID = results[1];
	if (results.size() >2)
	{
		//Status reply
		std::string Name_ID = results[0];
		if (Name_ID.find("HCGW:VC:ITECHNO") != std::string::npos)
		{
			m_bTXokay = true; // variable to indicate an OK was received
			mytime(&m_LastHeartbeatReceive);  // keep heartbeat happy
			mytime(&m_LastHeartbeat);  // keep heartbeat happy
			m_LastReceivedTime = m_LastHeartbeat;
#ifdef ENABLE_LOGGING
			_log.Log(LOG_STATUS, "ITGW identified by reply - ITGW only ever reply's its ID.");
#endif
		}
    else {
      return false;
    }
	}
  return false;
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::RType_CreateITGWDevice(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string scommand = request::findValue(&req, "command");
			if (idx.empty() || scommand.empty())
			{
				return;
			}

			#ifdef _DEBUG
			_log.Log(LOG_STATUS, "ITGW Custom Command: %s", scommand.c_str());
			_log.Log(LOG_STATUS, "ITGW Custom Command idx: %s", idx.c_str());
			#endif

			bool bCreated = false;						// flag to know if the command was a success
			CITGWBase *pRFLINK = reinterpret_cast<CITGWBase*>(m_mainworker.GetHardware(atoi(idx.c_str())));
			if (pRFLINK == NULL)
				return;

			if (scommand.substr(0, 14).compare("10;rfdebug=on;") == 0) {
				pRFLINK->m_bRFDebug = true; // enable debug
				_log.Log(LOG_STATUS, "User: %s initiated ITGW Enable Debug mode with command: %s", session.username.c_str(), scommand.c_str());
				pRFLINK->WriteInt("10;RFDEBUG=ON;\n");
				root["status"] = "OK";
				root["title"] = "DebugON";
				return;
			}
			if (scommand.substr(0, 15).compare("10;rfdebug=off;") == 0) {
				pRFLINK->m_bRFDebug = false; // disable debug
				_log.Log(LOG_STATUS, "User: %s initiated ITGW Disable Debug mode with command: %s", session.username.c_str(), scommand.c_str());
				pRFLINK->WriteInt("10;RFDEBUG=OFF;\n");
				root["status"] = "OK";
				root["title"] = "DebugOFF";
				return;
			}

			_log.Log(LOG_STATUS, "User: %s initiated a ITGW Device Create command: %s", session.username.c_str(), scommand.c_str());
			scommand = "11;" + scommand;
			#ifdef _DEBUG
			_log.Log(LOG_STATUS, "User: %s initiated a ITGW Device Create command: %s", session.username.c_str(), scommand.c_str());
			#endif
			scommand += "\r\n";

			bCreated = true;
			pRFLINK->m_bTXokay = false; // clear OK flag
			pRFLINK->WriteInt(scommand.c_str());
			time_t atime = mytime(NULL);
			time_t btime = mytime(NULL);

			// Wait for an OK response from ITGW to make sure the command was executed
			while (pRFLINK->m_bTXokay == false) {
				if (difftime(btime,atime) > 4) {
					_log.Log(LOG_ERROR, "ITGW: TX time out...");
					bCreated = false;
					break;
				}
				btime = mytime(NULL);
			}

			#ifdef _DEBUG
			_log.Log(LOG_STATUS, "ITGW custom command done");
			#endif

			if (bCreated)
			{
				root["status"] = "OK";
				root["title"] = "CreateITGWDevice";
			}
		}
	}
}
