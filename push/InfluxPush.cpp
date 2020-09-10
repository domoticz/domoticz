#include "stdafx.h"
#include "InfluxPush.h"
#include "../hardware/hardwaretypes.h"
#include "../httpclient/HTTPClient.h"
#include <json/json.h>
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/mainworker.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../main/WebServer.h"
#include "../webserver/Base64.h"
#include "../webserver/cWebem.h"
#include "../main/localtime_r.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

using namespace boost::placeholders;

CInfluxPush::CInfluxPush() :
	m_InfluxPort(8086),
	m_bInfluxDebugActive(false)
{
	m_PushType = PushType::PUSHTYPE_INFLUXDB;
	m_bLinkActive = false;
}

bool CInfluxPush::Start()
{
	Stop();

	RequestStart();

	UpdateSettings();

	m_thread = std::make_shared<std::thread>(&CInfluxPush::Do_Work, this);
	SetThreadName(m_thread->native_handle(), "InfluxPush");

	m_sConnection = m_mainworker.sOnDeviceReceived.connect(boost::bind(&CInfluxPush::OnDeviceReceived, this, _1, _2, _3, _4));

	return (m_thread != NULL);
}

void CInfluxPush::Stop()
{
	if (m_sConnection.connected())
		m_sConnection.disconnect();

	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
}

void CInfluxPush::UpdateSettings()
{
	int fActive = 0;
	m_sql.GetPreferencesVar("InfluxActive", fActive);
	m_bLinkActive = (fActive == 1);
	m_InfluxPort = 8086;
	m_sql.GetPreferencesVar("InfluxIP", m_InfluxIP);
	m_sql.GetPreferencesVar("InfluxPort", m_InfluxPort);
	m_sql.GetPreferencesVar("InfluxPath", m_InfluxPath);
	m_sql.GetPreferencesVar("InfluxDatabase", m_InfluxDatabase);
	m_sql.GetPreferencesVar("InfluxUsername", m_InfluxUsername);
	m_sql.GetPreferencesVar("InfluxPassword", m_InfluxPassword);

	int InfluxDebugActiveInt = 0;
	m_bInfluxDebugActive = false;
	m_sql.GetPreferencesVar("InfluxDebug", InfluxDebugActiveInt);
	if (InfluxDebugActiveInt == 1) {
		m_bInfluxDebugActive = true;
	}
	m_szURL = "";
	if (
		(m_InfluxIP.empty()) ||
		(m_InfluxPort == 0) ||
		(m_InfluxDatabase.empty())
		)
		return;
	std::stringstream sURL;
	if (m_InfluxIP.find("://") == std::string::npos)
		sURL << "http://";
	sURL << m_InfluxIP << ":" << m_InfluxPort;
	if (!m_InfluxPath.empty())
		sURL << "/" << m_InfluxPath;
	sURL << "/write?";
	if ((!m_InfluxUsername.empty()) && (!m_InfluxPassword.empty()))
		sURL << "u=" << m_InfluxUsername << "&p=" << base64_decode(m_InfluxPassword) << "&";
	sURL << "db=" << m_InfluxDatabase << "&precision=s";
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
		"SELECT A.DeviceRowID, A.DelimitedValue, B.ID, B.Type, B.SubType, B.nValue, B.sValue, A.TargetType, A.TargetVariable, A.TargetDeviceID, A.TargetProperty, A.IncludeUnit, B.Name, B.SwitchType FROM PushLink as A, DeviceStatus as B "
		"WHERE (A.PushType==%d AND A.DeviceRowID == '%" PRIu64 "' AND A.Enabled==1 AND A.DeviceRowID==B.ID)",
		PushType::PUSHTYPE_INFLUXDB,
		m_DeviceRowIdx);
	if (!result.empty())
	{
		time_t atime = mytime(NULL);
		std::string sendValue;
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;
			int delpos = atoi(sd[1].c_str());
			int dType = atoi(sd[3].c_str());
			int dSubType = atoi(sd[4].c_str());
			int nValue = atoi(sd[5].c_str());
			std::string sValue = sd[6].c_str();
			int targetType = atoi(sd[7].c_str());
			//std::string targetVariable = sd[8].c_str();
			//int targetDeviceID = atoi(sd[9].c_str());
			//std::string targetProperty = sd[10].c_str();
			int includeUnit = atoi(sd[11].c_str());
			std::string name = sd[12];
			int metertype = atoi(sd[13].c_str());

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

			if (sendValue != "") {
				std::string szKey;
				std::string vType = CBasePush::DropdownOptionsValue(std::stoi(sd[0]), delpos);
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

				std::lock_guard<std::mutex> l(m_background_task_mutex);
				if (m_background_task_queue.size() < 50)
					m_background_task_queue.push_back(pItem);
			}
		}
	}
}

void CInfluxPush::Do_Work()
{
	std::vector<_tPushItem> _items2do;

	while (!IsStopRequested(500))
	{
		{ // additional scope for lock (accessing size should be within lock too)
			std::lock_guard<std::mutex> l(m_background_task_mutex);
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
			if (!sSendData.empty())
				sSendData += '\n';

			std::stringstream sziData;
			sziData << itt->skey << " value=" << itt->svalue;
			if (m_bInfluxDebugActive) {
				_log.Log(LOG_NORM, "InfluxLink: value %s", sziData.str().c_str());
			}
			sziData << " " << itt->stimestamp;
			sSendData += sziData.str();
			++itt;
		}
		std::vector<std::string> ExtraHeaders;
		std::string sResult;
		if (!HTTPClient::POST(m_szURL, sSendData, ExtraHeaders, sResult, true, true))
		{
			_log.Log(LOG_ERROR, "InfluxLink: Error sending data to InfluxDB server! (check address/port/database/username/password)");
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
			std::string path = request::findValue(&req, "path");
			std::string database = request::findValue(&req, "database");
			std::string username = request::findValue(&req, "username");
			std::string password = request::findValue(&req, "password");
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
			m_sql.UpdatePreferencesVar("InfluxPath", path.c_str());
			m_sql.UpdatePreferencesVar("InfluxDatabase", database.c_str());
			m_sql.UpdatePreferencesVar("InfluxUsername", username.c_str());
			m_sql.UpdatePreferencesVar("InfluxPassword", base64_encode(password));
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
			if (m_sql.GetPreferencesVar("InfluxPath", sValue))
			{
				root["InfluxPath"] = sValue;
			}
			if (m_sql.GetPreferencesVar("InfluxDatabase", sValue))
			{
				root["InfluxDatabase"] = sValue;
			}
			if (m_sql.GetPreferencesVar("InfluxUsername", sValue))
			{
				root["InfluxUsername"] = sValue;
			}
			if (m_sql.GetPreferencesVar("InfluxPassword", sValue))
			{
				root["InfluxPassword"] = base64_decode(sValue);
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
			result = m_sql.safe_query(
				"SELECT A.ID,A.DeviceRowID,A.Delimitedvalue,A.TargetType,A.TargetVariable,A.TargetDeviceID,A.TargetProperty,A.Enabled, B.Name, A.IncludeUnit FROM PushLink as A, DeviceStatus as B WHERE (A.PushType==%d AND A.DeviceRowID==B.ID)", CBasePush::PushType::PUSHTYPE_INFLUXDB);
			if (!result.empty())
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
					"INSERT INTO PushLink (PushType,DeviceRowID,DelimitedValue,TargetType,TargetVariable,TargetDeviceID,TargetProperty,IncludeUnit,Enabled) VALUES (%d,'%d',%d,%d,'%q',%d,'%q',%d,%d)",
					CBasePush::PushType::PUSHTYPE_INFLUXDB,
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
					"UPDATE PushLink SET DeviceRowID=%d, DelimitedValue=%d, TargetType=%d, Enabled=%d WHERE (ID == '%q')",
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
