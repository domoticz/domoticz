#include "stdafx.h"
#include "MQTTPush.h"
#include "../hardware/hardwaretypes.h"
#include "../httpclient/HTTPClient.h"
#include <json/json.h>
#include "../main/Helper.h"
#include "../main/json_helper.h"
#include "../main/Logger.h"
#include "../main/mainworker.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../main/WebServer.h"
#include "../webserver/Base64.h"
#include "../webserver/cWebem.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

extern CMQTTPush m_mqttpush;

CMQTTPush::CMQTTPush()
{
	m_HwdID = 0;
	m_Name = "MQTTPush";
	m_PushType = PushType::PUSHTYPE_MQTT;
	m_bLinkActive = false;
}

bool CMQTTPush::Start()
{
	Stop();

	RequestStart();

	UpdateSettings();
	ReloadPushLinks(m_PushType);

	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadName(m_thread->native_handle(), "MQTTPush");

	m_sConnection = m_mainworker.sOnDeviceReceived.connect([this](auto id, auto idx, const auto& name, auto rx) { OnDeviceReceived(id, idx, name, rx); });

	return (m_thread != nullptr);
}

void CMQTTPush::Stop()
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

void CMQTTPush::UpdateSettings()
{
	int fActive = 0;
	m_sql.GetPreferencesVar("MQTTPushActive", fActive);
	m_bLinkActive = (fActive == 1);

	m_sql.GetPreferencesVar("MQTTPushIP", m_szIPAddress);
	m_sql.GetPreferencesVar("MQTTPushPort", m_usIPPort);
	m_sql.GetPreferencesVar("MQTTPushUsername", m_UserName);
	m_sql.GetPreferencesVar("MQTTPushPassword", m_Password);
	m_sql.GetPreferencesVar("MQTTPushTopicOut", m_TopicOut);
	m_sql.GetPreferencesVar("MQTTPushCAFile", m_CAFilename);
	m_sql.GetPreferencesVar("MQTTPushTLSVersion", m_TLS_Version);
}

void CMQTTPush::OnDeviceReceived(int m_HwdID, uint64_t DeviceRowIdx, const std::string& DeviceName, const unsigned char* pRXCommand)
{
	DoMQTTPush(DeviceRowIdx);
}

void CMQTTPush::DoMQTTPush(const uint64_t DeviceRowIdx, const bool bForced)
{
	if (!m_bLinkActive)
		return;
	if (!IsLinkInDatabase(DeviceRowIdx))
		return;

	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query(
		"SELECT A.DeviceRowID, A.DelimitedValue, B.ID, B.Type, B.SubType, B.nValue, B.sValue, A.TargetType, A.IncludeUnit, B.Name, B.SwitchType FROM PushLink as A, DeviceStatus as B "
		"WHERE (A.PushType==%d AND A.DeviceRowID == '%" PRIu64 "' AND A.Enabled==1 AND A.DeviceRowID==B.ID)",
		PushType::PUSHTYPE_MQTT, DeviceRowIdx);
	if (result.empty())
		return;

	time_t atime = mytime(nullptr);
	for (const auto& sd : result)
	{
		std::string sendValue;
		int delpos = atoi(sd[1].c_str());
		int dType = atoi(sd[3].c_str());
		int dSubType = atoi(sd[4].c_str());
		int nValue = atoi(sd[5].c_str());
		std::string sValue = sd[6];
		int targetType = atoi(sd[7].c_str());
		int includeUnit = atoi(sd[8].c_str());
		std::string name = sd[9];
		int metertype = atoi(sd[10].c_str());

		std::vector<std::string> strarray;
		if (sValue.find(';') != std::string::npos)
		{
			StringSplit(sValue, ";", strarray);
			if (int(strarray.size()) >= delpos)
			{
				std::string rawsendValue = strarray[delpos - 1];
				sendValue = ProcessSendValue(DeviceRowIdx, rawsendValue, delpos, nValue, includeUnit, dType, dSubType, metertype);
			}
		}
		else
			sendValue = ProcessSendValue(DeviceRowIdx, sValue, delpos, nValue, includeUnit, dType, dSubType, metertype);

		if (sendValue.empty())
			continue;

		std::string szKey;
		std::string vType = CBasePush::DropdownOptionsValue(dType, dSubType, delpos);
		stdreplace(vType, " ", "-");
		stdreplace(name, " ", "-");
		szKey = vType + ",idx=" + sd[0] + ",name=" + name;

		_tPushItem pItem;
		pItem.skey = szKey;
		pItem.stimestamp = atime;
		pItem.svalue = sendValue;

		if ((targetType == 0) && (!bForced))
		{
			// Only send on change
			auto itt = m_PushedItems.find(szKey);
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

void CMQTTPush::Do_Work()
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

		if (!m_IsConnected)
			continue;

		std::string sSendData;

		for (const auto& item : _items2do)
		{
			if (!sSendData.empty())
				sSendData += '\n';

			std::stringstream sziData;
			sziData << item.skey << " value=" << item.svalue;
			sziData << " " << item.stimestamp;
			sSendData += sziData.str();
		}
	}
}

// Webserver helpers
namespace http
{
	namespace server
	{
		void CWebServer::Cmd_SaveMQTTLinkConfig(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string linkactive = request::findValue(&req, "linkactive");
			std::string ipaddress = request::findValue(&req, "ipaddress");
			std::string port = request::findValue(&req, "port");
			std::string username = request::findValue(&req, "username");
			std::string password = request::findValue(&req, "password");
			std::string topic_out = request::findValue(&req, "topicout");
			std::string cafile = request::findValue(&req, "cafile");
			std::string tls_version = request::findValue(&req, "tlsversion");

			if ((linkactive.empty()) || (ipaddress.empty()) || (port.empty()) || (username.empty()) || (password.empty()) || (topic_out.empty()) || (tls_version.empty()))
				return;

			int ilinkactive = atoi(linkactive.c_str());
			int iTLSVersion = atoi(tls_version.c_str());
			if (iTLSVersion == 0) iTLSVersion = 2;

			m_sql.UpdatePreferencesVar("MQTTPushActive", ilinkactive);
			m_sql.UpdatePreferencesVar("MQTTPushIP", ipaddress);
			m_sql.UpdatePreferencesVar("MQTTPushPort", port);
			m_sql.UpdatePreferencesVar("MQTTPushUsername", username);
			m_sql.UpdatePreferencesVar("MQTTPushPassword", password);
			m_sql.UpdatePreferencesVar("MQTTPushTopicOut", topic_out);
			m_sql.UpdatePreferencesVar("MQTTPushCAFile", cafile);
			m_sql.UpdatePreferencesVar("MQTTPushTLSVersion", iTLSVersion);

			m_mqttpush.UpdateSettings();
			root["status"] = "OK";
			root["title"] = "SaveMQTTLinkConfig";
		}

		void CWebServer::Cmd_GetMQTTLinkConfig(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			std::string sValue;
			int nValue = 0;
			if (m_sql.GetPreferencesVar("MQTTPushActive", nValue))
			{
				root["linkactive"] = nValue;
			}
			else
			{
				root["linkactive"] = 0;
			}

			std::string svalue;

			if (m_sql.GetPreferencesVar("MQTTPushIP", svalue))
			{
				root["ipaddress"] = svalue;
			}
			if (m_sql.GetPreferencesVar("MQTTPushPort", svalue))
			{
				root["port"] = svalue;
			}
			if (m_sql.GetPreferencesVar("MQTTPushUsername", svalue))
			{
				root["username"] = svalue;
			}
			if (m_sql.GetPreferencesVar("MQTTPushPassword", svalue))
			{
				root["password"] = svalue;
			}
			if (m_sql.GetPreferencesVar("MQTTPushTopicOut", svalue))
			{
				root["topicout"] = svalue;
			}
			if (m_sql.GetPreferencesVar("MQTTPushCAFile", svalue))
			{
				root["cafile"] = svalue;
			}
			nValue = 0;
			if (m_sql.GetPreferencesVar("MQTTPushTLSVersion", nValue))
			{
				root["tlsversion"] = nValue;
			}

			root["status"] = "OK";
			root["title"] = "GetMQTTLinkConfig";
		}

		void CWebServer::Cmd_GetMQTTLinks(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT A.ID,A.DeviceRowID,A.Delimitedvalue,A.TargetType,A.TargetVariable,A.TargetDeviceID,A.TargetProperty,A.Enabled, B.Name, A.IncludeUnit, "
				"B.Type, B.SubType FROM PushLink as A, DeviceStatus as B WHERE (A.PushType==%d AND A.DeviceRowID==B.ID)",
				CBasePush::PushType::PUSHTYPE_MQTT);
			if (!result.empty())
			{
				int ii = 0;
				for (const auto& sd : result)
				{
					int Delimitedvalue = std::stoi(sd[2]);
					int devType = std::stoi(sd[10]);
					int devSubType = std::stoi(sd[11]);

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["DeviceID"] = sd[1];
					root["result"][ii]["Delimitedvalue"] = Delimitedvalue;
					root["result"][ii]["Delimitedname"] = CBasePush::DropdownOptionsValue(devType, devSubType, Delimitedvalue);
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
			root["title"] = "GetMQTTLinks";
		}

		void CWebServer::Cmd_SaveMQTTLink(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}
			std::string idx = request::findValue(&req, "idx");
			std::string deviceid = request::findValue(&req, "deviceid");
			int deviceidi = atoi(deviceid.c_str());
			std::string valuetosend = request::findValue(&req, "valuetosend");
			std::string targettype = request::findValue(&req, "targettype");
			int targettypei = atoi(targettype.c_str());
			std::string linkactive = request::findValue(&req, "linkactive");
			if (idx == "0")
			{
				m_sql.safe_query("INSERT INTO PushLink (PushType,DeviceRowID,DelimitedValue,TargetType,TargetVariable,TargetDeviceID,TargetProperty,IncludeUnit,Enabled) VALUES "
					"(%d,'%d',%d,%d,'%q',%d,'%q',%d,%d)",
					CBasePush::PushType::PUSHTYPE_MQTT, deviceidi, atoi(valuetosend.c_str()), targettypei, "-", 0, "-", 0, atoi(linkactive.c_str()));
			}
			else
			{
				m_sql.safe_query("UPDATE PushLink SET DeviceRowID=%d, DelimitedValue=%d, TargetType=%d, Enabled=%d WHERE (ID == '%q')", deviceidi, atoi(valuetosend.c_str()),
					targettypei, atoi(linkactive.c_str()), idx.c_str());
			}
			m_mqttpush.ReloadPushLinks(CBasePush::PushType::PUSHTYPE_MQTT);
			root["status"] = "OK";
			root["title"] = "SaveMQTTLink";
		}

		void CWebServer::Cmd_DeleteMQTTLink(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			m_sql.safe_query("DELETE FROM PushLink WHERE (ID=='%q')", idx.c_str());
			m_mqttpush.ReloadPushLinks(CBasePush::PushType::PUSHTYPE_MQTT);
			root["status"] = "OK";
			root["title"] = "DeleteMQTTLink";
		}
	} // namespace server
} // namespace http
