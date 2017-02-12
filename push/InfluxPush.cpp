#include "stdafx.h"
#include "InfluxPush.h"
#include "../hardware/hardwaretypes.h"
#include "../httpclient/HTTPClient.h"
#include "../json/json.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/mainworker.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../webserver/Base64.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

CInfluxPush::CInfluxPush()
{
	m_bLinkActive = false;
}

void CInfluxPush::Start()
{
	UpdateActive();
	m_sConnection = m_mainworker.sOnDeviceReceived.connect(boost::bind(&CInfluxPush::OnDeviceReceived, this, _1, _2, _3, _4));
}

void CInfluxPush::Stop()
{
	if (m_sConnection.connected())
		m_sConnection.disconnect();
}

void CInfluxPush::UpdateActive()
{
	int fActive;
	m_sql.GetPreferencesVar("InfluxActive", fActive);
	m_bLinkActive = (fActive == 1);
}

void CInfluxPush::OnDeviceReceived(const int m_HwdID, const uint64_t DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand)
{
	m_DeviceRowIdx = DeviceRowIdx;
	if (m_bLinkActive)
	{
		DoInfluxPush();
	}
}

void CInfluxPush::DoInfluxPush()
{			
	std::string InfluxIP = "";
	int InfluxPort = 8086;
	std::string InfluxDatabase = "";
	m_sql.GetPreferencesVar("InfluxIP", InfluxIP);
	m_sql.GetPreferencesVar("InfluxPort", InfluxPort);
	m_sql.GetPreferencesVar("InfluxDatabase", InfluxDatabase);

	
	int InfluxDebugActiveInt;
	bool InfluxDebugActive = false;
	m_sql.GetPreferencesVar("InfluxDebug", InfluxDebugActiveInt);
	if (InfluxDebugActiveInt == 1) {
		InfluxDebugActive = true;
	}

	if (
		(InfluxIP == "") ||
		(InfluxPort == 0) ||
		(InfluxDatabase == "")
		)
		return;
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT A.DeviceID, A.DelimitedValue, B.ID, B.Type, B.SubType, B.nValue, B.sValue, A.TargetType, A.TargetVariable, A.TargetDeviceID, A.TargetProperty, A.IncludeUnit, B.Name, B.SwitchType FROM PushLink as A, DeviceStatus as B "
		"WHERE (A.PushType==1 AND A.DeviceID == '%" PRIu64 "' AND A.Enabled==1 AND A.DeviceID==B.ID)",
		m_DeviceRowIdx);
	if (result.size()>0)
	{
		std::string sendValue;
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;
			int delpos = atoi(sd[1].c_str());
			int dType = atoi(sd[3].c_str());
			int dSubType = atoi(sd[4].c_str());
			int nValue = atoi(sd[5].c_str());
			std::string sValue = sd[6].c_str();
			int targetType = atoi(sd[7].c_str());
			std::string targetVariable = sd[8].c_str();
			int targetDeviceID = atoi(sd[9].c_str());
			std::string targetProperty = sd[10].c_str();
			int includeUnit = atoi(sd[11].c_str());
			std::string name = sd[12];
			int metertype = atoi(sd[13].c_str());
			std::string lstatus="";

			std::vector<std::string> strarray;
			if (sValue.find(";")!=std::string::npos) {
				StringSplit(sValue, ";", strarray);
				if (int(strarray.size())>=delpos)
				{
					std::string rawsendValue = strarray[delpos-1].c_str();
					sendValue = ProcessSendValue(rawsendValue,delpos,nValue,includeUnit,metertype);
				}
			}
			if (sendValue !="") {
				std::stringstream szKey;
				std::string vType = DropdownOptionsValue(m_DeviceRowIdx, delpos);
				stdreplace(vType, " ", "-");
				stdreplace(name, " ", "-");
				szKey << vType << ",idx=" << sd[0] << ",name=" << name << " value=" << sendValue;

				std::string szPostdata = szKey.str();
				std::vector<std::string> ExtraHeaders;

				std::stringstream sURL;
				if (InfluxIP.find("://") == std::string::npos)
					sURL << "http://";
				sURL << InfluxIP << ":" << InfluxPort << "/write?db=" << InfluxDatabase;


				if (InfluxDebugActive) {
					_log.Log(LOG_NORM, "InfluxLink: value %s", szPostdata.c_str());
				}

				std::string sResult;
				if (!HTTPClient::POST(sURL.str(), szPostdata, ExtraHeaders, sResult))
				{
					_log.Log(LOG_ERROR, "InfluxLink: Error sending data to InfluxDB server! (check address/port/database)");
					return;
				}
			}
		}
	}
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_SaveInfluxLinkConfig(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string linkactive = request::findValue(&req, "linkactive");
			std::string remote = request::findValue(&req, "remote");
			std::string port = request::findValue(&req, "port");
			std::string database = request::findValue(&req, "database");
			std::string debugenabled = request::findValue(&req, "debugenabled");
			if (
				(linkactive == "") ||
				(remote == "") ||
				(port == "") ||
				(database == "") ||
				(debugenabled == "")
				)
				return;
			int ilinkactive = atoi(linkactive.c_str());
			int idebugenabled = atoi(debugenabled.c_str());
			m_sql.UpdatePreferencesVar("InfluxActive", ilinkactive);
			m_sql.UpdatePreferencesVar("InfluxIP", remote.c_str());
			m_sql.UpdatePreferencesVar("InfluxPort", atoi(port.c_str()));
			m_sql.UpdatePreferencesVar("InfluxDatabase", database.c_str());
			m_sql.UpdatePreferencesVar("InfluxDebug", idebugenabled);
			m_mainworker.m_influxpush.UpdateActive();
			root["status"] = "OK";
			root["title"] = "SaveInfluxLinkConfig";
		}

		void CWebServer::Cmd_GetInfluxLinkConfig(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			std::string sValue;
			int nValue;
			if (m_sql.GetPreferencesVar("InfluxActive", nValue)) {
				root["InfluxActive"] = nValue;
			}
			else {
				root["InfluxActive"] = 0;
			}
			if (m_sql.GetPreferencesVar("InfluxIP", sValue))
			{
				root["InfluxIP"] = sValue;
			}
			if (m_sql.GetPreferencesVar("InfluxPort", nValue))
			{
				root["InfluxPort"] = nValue;
			}
			if (m_sql.GetPreferencesVar("InfluxDatabase", sValue))
			{
				root["InfluxDatabase"] = sValue;
			}
			if (m_sql.GetPreferencesVar("InfluxDebug", nValue)) {
				root["InfluxDebug"] = nValue;
			}
			else {
				root["InfluxDebug"] = 0;
			}
			root["status"] = "OK";
			root["title"] = "GetInfluxLinkConfig";
		}

		void CWebServer::Cmd_GetInfluxLinks(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT A.ID,A.DeviceID,A.Delimitedvalue,A.TargetType,A.TargetVariable,A.TargetDeviceID,A.TargetProperty,A.Enabled, B.Name, A.IncludeUnit FROM PushLink as A, DeviceStatus as B WHERE (A.PushType==1 AND A.DeviceID==B.ID)");
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["DeviceID"] = sd[1];
					root["result"][ii]["Delimitedvalue"] = sd[2];
					root["result"][ii]["TargetType"] = sd[3];
					root["result"][ii]["TargetVariable"] = sd[4];
					root["result"][ii]["TargetDevice"] = sd[5];
					root["result"][ii]["TargetProperty"] = sd[6];
					root["result"][ii]["Enabled"] = sd[7];
					root["result"][ii]["Name"] = sd[8];
					root["result"][ii]["IncludeUnit"] = sd[9];
					ii++;
				}
			}
			root["status"] = "OK";
			root["title"] = "GetInfluxLinks";
		}

		void CWebServer::Cmd_SaveInfluxLink(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			std::string idx = request::findValue(&req, "idx");
			std::string deviceid = request::findValue(&req, "deviceid");
			int deviceidi = atoi(deviceid.c_str());
			std::string valuetosend = request::findValue(&req, "valuetosend");
			std::string linkactive = request::findValue(&req, "linkactive");
			if (idx == "0") {
				m_sql.safe_query(
					"INSERT INTO PushLink (PushType,DeviceID,DelimitedValue,TargetType,TargetVariable,TargetDeviceID,TargetProperty,IncludeUnit,Enabled) VALUES (%d,'%d',%d,%d,'%q',%d,'%q',%d,%d)",
					1,
					deviceidi,
					atoi(valuetosend.c_str()),
					0,
					"-",
					0,
					"-",
					0,
					atoi(linkactive.c_str())
					);
			}
			else {
				m_sql.safe_query(
					"UPDATE PushLink SET DeviceID='%d', DelimitedValue=%d, Enabled='%d' WHERE (ID == '%q')",
					deviceidi,
					atoi(valuetosend.c_str()),
					atoi(linkactive.c_str()),
					idx.c_str()
					);
			}
			root["status"] = "OK";
			root["title"] = "SaveInfluxLink";
		}

		void CWebServer::Cmd_DeleteInfluxLink(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			m_sql.safe_query("DELETE FROM PushLink WHERE (ID=='%q')", idx.c_str());
			root["status"] = "OK";
			root["title"] = "DeleteInfluxLink";
		}
	}
}