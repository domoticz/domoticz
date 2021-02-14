#include "stdafx.h"
#include "HttpPush.h"
#include "../main/Helper.h"
#include "../httpclient/HTTPClient.h"
#include "../main/Logger.h"
#include "../hardware/hardwaretypes.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../webserver/Base64.h"
#include "../main/localtime_r.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include "../main/mainworker.h"
#include <json/json.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

extern CHttpPush m_httppush;

CHttpPush::CHttpPush()
{
	m_PushType = PushType::PUSHTYPE_HTTP;
	m_bLinkActive = false;
}

void CHttpPush::Start()
{
	UpdateActive();
	ReloadPushLinks(m_PushType);
	m_sConnection = m_mainworker.sOnDeviceReceived.connect([this](auto id, auto idx, const auto &name, auto rx) { OnDeviceReceived(id, idx, name, rx); });
}

void CHttpPush::Stop()
{
	if (m_sConnection.connected())
		m_sConnection.disconnect();
}


void CHttpPush::UpdateActive()
{
	int fActive = 0;
	m_sql.GetPreferencesVar("HttpActive", fActive);
	m_bLinkActive = (fActive == 1);
}

void CHttpPush::OnDeviceReceived(const int m_HwdID, const uint64_t DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand)
{
	if (m_bLinkActive)
	{
		DoHttpPush(DeviceRowIdx);
	}
}

void CHttpPush::DoHttpPush(const uint64_t DeviceRowIdx)
{
	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query("SELECT A.DeviceRowID, A.DelimitedValue, B.ID, B.Type, B.SubType, B.nValue, B.sValue, A.TargetType, A.TargetVariable, A.TargetDeviceID, A.TargetProperty, "
				  "A.IncludeUnit, B.SwitchType, strftime('%%s', B.LastUpdate), B.Name FROM PushLink as A, DeviceStatus as B "
				  "WHERE (A.PushType==%d AND A.DeviceRowID == '%" PRIu64 "' AND A.Enabled = '1' AND A.DeviceRowID==B.ID)",
				  PushType::PUSHTYPE_HTTP, DeviceRowIdx);
	if (result.empty())
		return;

	std::string httpUrl;
	std::string httpData;
	std::string httpHeaders;
	int httpMethodInt = 0;
	int httpAuthInt = 0;
	std::string httpAuthBasicLogin;
	std::string httpAuthBasicPassword;
	m_sql.GetPreferencesVar("HttpMethod", httpMethodInt);
	m_sql.GetPreferencesVar("HttpAuth", httpAuthInt);
	m_sql.GetPreferencesVar("HttpAuthBasicLogin", httpAuthBasicLogin);
	m_sql.GetPreferencesVar("HttpAuthBasicPassword", httpAuthBasicPassword);

	int httpDebugActiveInt = 0;
	bool httpDebugActive = false;
	m_sql.GetPreferencesVar("HttpDebug", httpDebugActiveInt);
	if (httpDebugActiveInt == 1) {
		httpDebugActive = true;
	}

	for (const auto &sd : result)
	{
		std::string sendValue;
		m_sql.GetPreferencesVar("HttpUrl", httpUrl);
		m_sql.GetPreferencesVar("HttpData", httpData);
		m_sql.GetPreferencesVar("HttpHeaders", httpHeaders);
		if (httpUrl.empty())
			return;

		//unsigned int deviceId = atoi(sd[0].c_str());
		std::string sdeviceId = sd[0];
		std::string ldelpos = sd[1];
		int delpos = atoi(sd[1].c_str());
		int dType = atoi(sd[3].c_str());
		int dSubType = atoi(sd[4].c_str());
		int nValue = atoi(sd[5].c_str());
		std::string sValue = sd[6];
		//int targetType = atoi(sd[7].c_str());
		std::string targetVariable = sd[8];
		//int targetDeviceID = atoi(sd[9].c_str());
		//std::string targetProperty = sd[10].c_str();
		int includeUnit = atoi(sd[11].c_str());
		int metertype = atoi(sd[12].c_str());
		int lastUpdate = atoi(sd[13].c_str());
		std::string ltargetVariable = sd[8];
		std::string ltargetDeviceId = sd[9];
		std::string lname = sd[14];
		sendValue = sValue;

		unsigned long tzoffset = get_tzoffset();

#ifdef WIN32
		unsigned __int64 localTime = lastUpdate;
		unsigned __int64 localTimeUtc = lastUpdate - tzoffset;
#else
		unsigned long long int localTime = lastUpdate;
		unsigned long long int localTimeUtc = lastUpdate - tzoffset;
#endif

		char szLocalTime[21];
		sprintf(szLocalTime, "%llu", localTime);
		char szLocalTimeUtc[21];
		sprintf(szLocalTimeUtc, "%llu", localTimeUtc);
		char szLocalTimeMs[21];
		sprintf(szLocalTimeMs, "%llu", localTime * 1000);
		char szLocalTimeUtcMs[21];
		sprintf(szLocalTimeUtcMs, "%llu", localTimeUtc * 1000);

		std::string llastUpdate = get_lastUpdate(localTimeUtc);

		// Replace keywords
		/*
		%v : Value
		%t0 : Timestamp (epoc time localtime)
		%t1 : Timestamp (epoc ms localtime)
		%t2 : Timestamp (epoc time UTC)
		%t3 : Timestamp (epoc ms UTC)
		%t4 : Timestamp : "2015-01-29T21:50:44Z"
		%D : Target Device id
		%V : Target Variable
		%u : Unit
		%n : Device name
		%T0 : Type
		%T1 : SubType
		%h : hostname
		%idx : 'Original device' id (idx)
		*/

		std::string lunit = getUnit(dType, dSubType, delpos, metertype);
		std::string lType = RFX_Type_Desc(dType, 1);
		std::string lSubType = RFX_Type_SubType_Desc(dType, dSubType);

		char hostname[256];
		gethostname(hostname, sizeof(hostname));

		std::vector<std::string> strarray;
		if (sendValue.find(';') != std::string::npos)
		{
			StringSplit(sendValue, ";", strarray);
			if (int(strarray.size()) >= delpos && delpos > 0)
			{
				std::string rawsendValue = strarray[delpos - 1];
				sendValue = ProcessSendValue(DeviceRowIdx, rawsendValue, delpos, nValue, false, dType, dSubType, metertype);
			}
		}
		else
		{
			sendValue = ProcessSendValue(DeviceRowIdx, sendValue, delpos, nValue, false, dType, dSubType, metertype);
		}
		if (sendValue.empty())
			continue;

		ltargetDeviceId += "_";
		ltargetDeviceId += ldelpos;

		replaceAll(httpUrl, "%v", sendValue);
		replaceAll(httpUrl, "%u", includeUnit ? lunit : "");
		replaceAll(httpUrl, "%D", ltargetDeviceId);
		replaceAll(httpUrl, "%V", ltargetVariable);
		replaceAll(httpUrl, "%t0", std::string(szLocalTime));
		replaceAll(httpUrl, "%t1", std::string(szLocalTimeMs));
		replaceAll(httpUrl, "%t2", std::string(szLocalTimeUtc));
		replaceAll(httpUrl, "%t3", std::string(szLocalTimeUtcMs));
		replaceAll(httpUrl, "%t4", llastUpdate);
		replaceAll(httpUrl, "%n", lname);
		replaceAll(httpUrl, "%T0", lType);
		replaceAll(httpUrl, "%T1", lSubType);
		replaceAll(httpUrl, "%h", std::string(hostname));
		replaceAll(httpUrl, "%idx", sdeviceId);

		replaceAll(httpData, "%v", sendValue);
		replaceAll(httpData, "%u", includeUnit ? lunit : "");
		replaceAll(httpData, "%D", ltargetDeviceId);
		replaceAll(httpData, "%V", ltargetVariable);
		replaceAll(httpData, "%t0", std::string(szLocalTime));
		replaceAll(httpData, "%t1", std::string(szLocalTimeMs));
		replaceAll(httpData, "%t2", std::string(szLocalTimeUtc));
		replaceAll(httpData, "%t3", std::string(szLocalTimeUtcMs));
		replaceAll(httpData, "%t4", llastUpdate);
		replaceAll(httpData, "%n", lname);
		replaceAll(httpData, "%T0", lType);
		replaceAll(httpData, "%T1", lSubType);
		replaceAll(httpData, "%h", std::string(hostname));
		replaceAll(httpData, "%idx", sdeviceId);

		std::string sResult;
		std::vector<std::string> ExtraHeaders;
		if (httpAuthInt == 1) {			// BASIC authentication
			std::stringstream sstr;
			sstr << httpAuthBasicLogin << ":" << httpAuthBasicPassword;
			std::string m_AccessToken = base64_encode(sstr.str());
			ExtraHeaders.push_back("Authorization:Basic " + m_AccessToken);
		}
		sendValue = CURLEncode::URLEncode(sendValue);

		// data
		if (httpDebugActive) {
			_log.Log(LOG_NORM, "HttpLink: sending global variable %s with value: %s", targetVariable.c_str(), sendValue.c_str());
		}

		if (httpMethodInt == 0) {			// GET
			if (!HTTPClient::GET(httpUrl, ExtraHeaders, sResult, true))
			{
				_log.Log(LOG_ERROR, "HttpLink: Error sending data to http with GET!");
			}
		}
		else if (httpMethodInt == 1) {		// POST
			if (!httpHeaders.empty())
			{
				// Add additional headers
				std::vector<std::string> ExtraHeaders2;
				StringSplit(httpHeaders, "\r\n", ExtraHeaders2);
				std::copy(ExtraHeaders2.begin(), ExtraHeaders2.end(), std::back_inserter(ExtraHeaders));
			}
			if (!HTTPClient::POST(httpUrl, httpData, ExtraHeaders, sResult, true, true))
			{
				_log.Log(LOG_ERROR, "HttpLink: Error sending data to http with POST!");
			}
		}
		else if (httpMethodInt == 2) {		// PUT
			if (!HTTPClient::PUT(httpUrl, httpData, ExtraHeaders, sResult, true))
			{
				_log.Log(LOG_ERROR, "HttpLink: Error sending data to http with PUT!");
			}
		}

		// debug
		if (httpDebugActive) {
			_log.Log(LOG_NORM, "HttpLink: response %s", sResult.c_str());
		}
	}
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_SaveHttpLinkConfig(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string url = request::findValue(&req, "url");
			std::string method = request::findValue(&req, "method");
			std::string data = request::findValue(&req, "data");
			std::string headers = request::findValue(&req, "headers");
			std::string linkactive = request::findValue(&req, "linkactive");
			std::string debugenabled = request::findValue(&req, "debugenabled");
			std::string auth = request::findValue(&req, "auth");
			std::string authbasiclogin = request::findValue(&req, "authbasiclogin");
			std::string authbasicpassword = request::findValue(&req, "authbasicpassword");
			if ((url.empty()) || (method.empty()) || (linkactive.empty()) || (debugenabled.empty()))
				return;
			if ((method != "0") && (data.empty())) //PUT/POST should have data
				return;
			int ilinkactive = atoi(linkactive.c_str());
			int idebugenabled = atoi(debugenabled.c_str());
			m_sql.UpdatePreferencesVar("HttpUrl", url);
			m_sql.UpdatePreferencesVar("HttpMethod", atoi(method.c_str()));
			m_sql.UpdatePreferencesVar("HttpData", data);
			m_sql.UpdatePreferencesVar("HttpHeaders", headers);
			m_sql.UpdatePreferencesVar("HttpActive", ilinkactive);
			m_sql.UpdatePreferencesVar("HttpDebug", idebugenabled);
			m_sql.UpdatePreferencesVar("HttpAuth", atoi(auth.c_str()));
			m_sql.UpdatePreferencesVar("HttpAuthBasicLogin", authbasiclogin);
			m_sql.UpdatePreferencesVar("HttpAuthBasicPassword", authbasicpassword);

			m_httppush.UpdateActive();
			root["status"] = "OK";
			root["title"] = "SaveHttpLinkConfig";
		}

		void CWebServer::Cmd_GetHttpLinkConfig(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			std::string sValue;
			int nValue;
			if (m_sql.GetPreferencesVar("HttpActive", nValue)) {
				root["HttpActive"] = nValue;
			}
			else {
				root["HttpActive"] = 0;
			}
			if (m_sql.GetPreferencesVar("HttpDebug", nValue)) {
				root["HttpDebug"] = nValue;
			}
			else {
				root["HttpDebug"] = 0;
			}
			if (m_sql.GetPreferencesVar("HttpUrl", sValue))
			{
				root["HttpUrl"] = sValue;
			}
			if (m_sql.GetPreferencesVar("HttpData", sValue))
			{
				root["HttpData"] = sValue;
			}
			if (m_sql.GetPreferencesVar("HttpHeaders", sValue))
			{
				root["HttpHeaders"] = sValue;
			}
			if (m_sql.GetPreferencesVar("HttpMethod", nValue))
			{
				root["HttpMethod"] = nValue;
			}
			else {
				root["HttpMethod"] = 0;
			}
			if (m_sql.GetPreferencesVar("HttpAuth", nValue))
			{
				root["HttpAuth"] = nValue;
			}
			else {
				root["HttpAuth"] = 0;
			}
			if (m_sql.GetPreferencesVar("HttpAuthBasicLogin", sValue))
			{
				root["HttpAuthBasicLogin"] = sValue;
			}
			if (m_sql.GetPreferencesVar("HttpAuthBasicPassword", sValue))
			{
				root["HttpAuthBasicPassword"] = sValue;
			}
			root["status"] = "OK";
			root["title"] = "GetHttpLinkConfig";
		}

		void CWebServer::Cmd_GetHttpLinks(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT A.ID,A.DeviceRowID,A.Delimitedvalue,A.TargetType,A.TargetVariable,A.TargetDeviceID,A.TargetProperty,A.Enabled, B.Name, A.IncludeUnit FROM PushLink as A, DeviceStatus as B WHERE (A.PushType==%d AND A.DeviceRowID==B.ID)", CBasePush::PushType::PUSHTYPE_HTTP);
			if (!result.empty())
			{
				int ii = 0;
				for (const auto &sd : result)
				{
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
			root["title"] = "GetHttpLinks";
		}

		void CWebServer::Cmd_SaveHttpLink(WebEmSession & session, const request& req, Json::Value &root)
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
			std::string targetvariable = request::findValue(&req, "targetvariable");
			std::string targetdeviceid = request::findValue(&req, "targetdeviceid");
			std::string targetproperty = request::findValue(&req, "targetproperty");
			std::string linkactive = request::findValue(&req, "linkactive");
			std::string includeunit = request::findValue(&req, "includeunit");
			if ((targettypei == 0) && (targetvariable.empty()))
				return;
			if ((targettypei == 1) && ((targetdeviceid.empty()) || (targetproperty.empty())))
				return;
			if ((targettypei == 2) && (targetdeviceid.empty()))
				return;
			if (idx == "0") {
				m_sql.safe_query(
					"INSERT INTO PushLink (PushType, DeviceRowID,DelimitedValue,TargetType,TargetVariable,TargetDeviceID,TargetProperty,IncludeUnit,Enabled) VALUES ('%d','%d','%d','%d','%q','%d','%q','%d','%d')",
					CBasePush::PushType::PUSHTYPE_HTTP,
					deviceidi,
					atoi(valuetosend.c_str()),
					targettypei,
					targetvariable.c_str(),
					atoi(targetdeviceid.c_str()),
					targetproperty.c_str(),
					atoi(includeunit.c_str()),
					atoi(linkactive.c_str())
				);
			}
			else {
				m_sql.safe_query(
					"UPDATE PushLink SET DeviceRowID='%d', DelimitedValue=%d, TargetType=%d, TargetVariable='%q', TargetDeviceID=%d, TargetProperty='%q', IncludeUnit='%d', Enabled='%d' WHERE (ID == '%q')",
					deviceidi,
					atoi(valuetosend.c_str()),
					targettypei,
					targetvariable.c_str(),
					atoi(targetdeviceid.c_str()),
					targetproperty.c_str(),
					atoi(includeunit.c_str()),
					atoi(linkactive.c_str()),
					idx.c_str()
				);
			}
			m_httppush.ReloadPushLinks(CBasePush::PushType::PUSHTYPE_HTTP);
			root["status"] = "OK";
			root["title"] = "SaveHttpLink";
		}

		void CWebServer::Cmd_DeleteHttpLink(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			m_sql.safe_query("DELETE FROM PushLink WHERE (ID=='%q')", idx.c_str());
			m_httppush.ReloadPushLinks(CBasePush::PushType::PUSHTYPE_HTTP);
			root["status"] = "OK";
			root["title"] = "DeleteHttpLink";
		}
	} // namespace server
} // namespace http
