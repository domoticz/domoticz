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
#include "../main/localtime_r.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

CInfluxPush::CInfluxPush():
	m_InfluxPort(8086),
	m_bInfluxDebugActive(false),
	m_stoprequested(false)
{
	m_bLinkActive = false;
}

void CInfluxPush::Start()
{
	UpdateSettings();
	m_sConnection = m_mainworker.sOnDeviceReceived.connect(boost::bind(&CInfluxPush::OnDeviceReceived, this, _1, _2, _3, _4));
	StartThread();
}

void CInfluxPush::Stop()
{
	if (m_sConnection.connected())
		m_sConnection.disconnect();
	StopThread();
}

void CInfluxPush::UpdateSettings()
{
	int fActive;
	m_sql.GetPreferencesVar("InfluxActive", fActive);
	m_bLinkActive = (fActive == 1);
	m_InfluxPort = 8086;
	m_sql.GetPreferencesVar("InfluxIP", m_InfluxIP);
	m_sql.GetPreferencesVar("InfluxPort", m_InfluxPort);
	m_sql.GetPreferencesVar("InfluxDatabase", m_InfluxDatabase);
	int InfluxDebugActiveInt;
	m_bInfluxDebugActive = false;
	m_sql.GetPreferencesVar("InfluxDebug", InfluxDebugActiveInt);
	if (InfluxDebugActiveInt == 1) {
		m_bInfluxDebugActive = true;
	}
	m_szURL = "";
	if (
		(m_InfluxIP == "") ||
		(m_InfluxPort == 0) ||
		(m_InfluxDatabase == "")
		)
		return;
	std::stringstream sURL;
	if (m_InfluxIP.find("://") == std::string::npos)
		sURL << "http://";
	sURL << m_InfluxIP << ":" << m_InfluxPort << "/write?db=" << m_InfluxDatabase;
	m_szURL = sURL.str();
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
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT A.DeviceID, A.DelimitedValue, B.ID, B.Type, B.SubType, B.nValue, B.sValue, A.TargetType, A.TargetVariable, A.TargetDeviceID, A.TargetProperty, A.IncludeUnit, B.Name, B.SwitchType FROM PushLink as A, DeviceStatus as B "
		"WHERE (A.PushType==1 AND A.DeviceID == '%" PRIu64 "' AND A.Enabled==1 AND A.DeviceID==B.ID)",
		m_DeviceRowIdx);
	if (result.size()>0)
	{
		time_t atime = mytime(NULL);
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
			if (sValue.find(";") != std::string::npos) {
				StringSplit(sValue, ";", strarray);
				if (int(strarray.size()) >= delpos)
				{
					std::string rawsendValue = strarray[delpos - 1].c_str();
					sendValue = ProcessSendValue(rawsendValue, delpos, nValue, includeUnit, dType, dSubType, metertype);
				}
			}
			else
				sendValue = ProcessSendValue(sValue, delpos, nValue, includeUnit, dType, dSubType, metertype);

			if (sendValue !="") {
				std::string szKey;
				std::string vType = CBasePush::DropdownOptionsValue(m_DeviceRowIdx, delpos);
				stdreplace(vType, " ", "-");
				stdreplace(name, " ", "-");
				szKey = vType + ",idx=" + sd[0] + ",name=" + name;

				_tPushItem pItem;
				pItem.skey = szKey;
				pItem.stimestamp = atime;
				pItem.svalue = sendValue;

				if (targetType == 0)
				{
					//Only send on change
					std::map<std::string, _tPushItem>::iterator itt = m_PushedItems.find(szKey);
					if (itt != m_PushedItems.end())
					{
						if (sendValue == itt->second.svalue)
							continue;
					}
					m_PushedItems[szKey] = pItem;
				}

				boost::lock_guard<boost::mutex> l(m_background_task_mutex);
				if (m_background_task_queue.size() < 50)
					m_background_task_queue.push_back(pItem);
			}
		}
	}
}

bool CInfluxPush::StartThread()
{
	StopThread();
	m_stoprequested = false;
	m_background_task_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CInfluxPush::Do_Work, this)));
	return (m_background_task_thread != NULL);
}

void CInfluxPush::StopThread()
{
	if (m_background_task_thread)
	{
		m_stoprequested = true;
		m_background_task_thread->join();
	}
}


void CInfluxPush::Do_Work()
{
	std::vector<_tPushItem> _items2do;

	while (!m_stoprequested)
	{
		sleep_milliseconds(500);

		{ // additional scope for lock (accessing size should be within lock too)
			boost::lock_guard<boost::mutex> l(m_background_task_mutex);
			if (m_background_task_queue.empty())
				continue;
			_items2do = m_background_task_queue;
			m_background_task_queue.clear();
		}

		if (m_szURL.empty())
			continue;

		std::string sSendData;

		std::vector<_tPushItem>::iterator itt = _items2do.begin();
		while (itt != _items2do.end())
		{
			std::stringstream sziData;
			sziData << itt->skey << " value=" << itt->svalue;
			if (m_bInfluxDebugActive) {
				_log.Log(LOG_NORM, "InfluxLink: value %s", sziData.str().c_str());
			}
			//sziData << " " << (itt->stimestamp * 1000000000);
			if (!sSendData.empty())
				sSendData += "\n";
			sSendData += sziData.str();
			++itt;
		}
		std::vector<std::string> ExtraHeaders;
		std::string sResult;
		if (!HTTPClient::POST(m_szURL, sSendData, ExtraHeaders, sResult, true, true))
		{
			_log.Log(LOG_ERROR, "InfluxLink: Error sending data to InfluxDB server! (check address/port/database)");
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
			m_influxpush.UpdateSettings();
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
			std::string targettype = request::findValue(&req, "targettype");
			int targettypei = atoi(targettype.c_str());
			std::string linkactive = request::findValue(&req, "linkactive");
			if (idx == "0") {
				m_sql.safe_query(
					"INSERT INTO PushLink (PushType,DeviceID,DelimitedValue,TargetType,TargetVariable,TargetDeviceID,TargetProperty,IncludeUnit,Enabled) VALUES (%d,'%d',%d,%d,'%q',%d,'%q',%d,%d)",
					1,
					deviceidi,
					atoi(valuetosend.c_str()),
					targettypei,
					"-",
					0,
					"-",
					0,
					atoi(linkactive.c_str())
					);
			}
			else {
				m_sql.safe_query(
					"UPDATE PushLink SET DeviceID=%d, DelimitedValue=%d, TargetType=%d, Enabled=%d WHERE (ID == '%q')",
					deviceidi,
					atoi(valuetosend.c_str()),
					targettypei,
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