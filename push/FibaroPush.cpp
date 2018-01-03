#include "stdafx.h"
#include "FibaroPush.h"
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

CFibaroPush::CFibaroPush()
{
	m_bLinkActive = false;
}

void CFibaroPush::Start()
{
	UpdateActive();
	m_sConnection = m_mainworker.sOnDeviceReceived.connect(boost::bind(&CFibaroPush::OnDeviceReceived, this, _1, _2, _3, _4));
}

void CFibaroPush::Stop()
{
	if (m_sConnection.connected())
		m_sConnection.disconnect();
}

void CFibaroPush::UpdateActive()
{
	int fActive;
	m_sql.GetPreferencesVar("FibaroActive", fActive);
	m_bLinkActive = (fActive == 1);
}

void CFibaroPush::OnDeviceReceived(const int m_HwdID, const uint64_t DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand)
{
	m_DeviceRowIdx = DeviceRowIdx;
	if (m_bLinkActive)
	{
		DoFibaroPush();
	}
}

void CFibaroPush::DoFibaroPush()
{			
	std::string fibaroIP = "";
	std::string fibaroUsername = "";
	std::string fibaroPassword = "";
	int iIsVersion4 = 0;
	m_sql.GetPreferencesVar("FibaroIP", fibaroIP);
	m_sql.GetPreferencesVar("FibaroUsername", fibaroUsername);
	m_sql.GetPreferencesVar("FibaroPassword", fibaroPassword);
	m_sql.GetPreferencesVar("FibaroVersion4", iIsVersion4);

	
	int fibaroDebugActiveInt;
	bool fibaroDebugActive = false;
	m_sql.GetPreferencesVar("FibaroDebug", fibaroDebugActiveInt);
	if (fibaroDebugActiveInt == 1) {
		fibaroDebugActive = true;
	}
	bool bIsV4 = (iIsVersion4 != 0);

	if (
		(fibaroIP == "") ||
		(fibaroUsername == "") ||
		(fibaroPassword == "")
		)
		return;
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT A.DeviceID, A.DelimitedValue, B.ID, B.Type, B.SubType, B.nValue, B.sValue, A.TargetType, A.TargetVariable, A.TargetDeviceID, A.TargetProperty, A.IncludeUnit, B.SwitchType FROM FibaroLink as A, DeviceStatus as B "
		"WHERE (A.DeviceID == '%" PRIu64 "' AND A.Enabled = '1' AND A.DeviceID==B.ID)",
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
			int metertype = atoi(sd[12].c_str());
			std::string lstatus="";

			if ((targetType==0)||(targetType==1)) {
				if (delpos == 0) {
					int llevel=0;
					bool bHaveDimmer=false;
					bool bHaveGroupCmd=false;
					int maxDimLevel=0;
    				GetLightStatus(dType,dSubType,(_eSwitchType)metertype,nValue,sValue,lstatus,llevel,bHaveDimmer,maxDimLevel,bHaveGroupCmd);
					sendValue = lstatus;
				}
				else if (delpos>0) {
					std::vector<std::string> strarray;
					if (sValue.find(";")!=std::string::npos) {
						StringSplit(sValue, ";", strarray);
						if (int(strarray.size())>=delpos)
						{
							std::string rawsendValue = strarray[delpos-1].c_str();
							sendValue = ProcessSendValue(rawsendValue,delpos,nValue,includeUnit,dType, dSubType, metertype);
						}
					}
					else
						sendValue = ProcessSendValue(sValue, delpos, nValue, includeUnit, dType, dSubType, metertype);
				}
			}
			else { // scenes/reboot, only on/off
				int llevel=0;
				bool bHaveDimmer=false;
				bool bHaveGroupCmd=false;
				int maxDimLevel=0;
    			GetLightStatus(dType,dSubType, STYPE_OnOff,nValue,sValue,lstatus,llevel,bHaveDimmer,maxDimLevel,bHaveGroupCmd);
				sendValue = lstatus;
			}
			if (sendValue !="") {
				std::string sResult;
				std::stringstream sPostData;
				std::stringstream Url;
				std::vector<std::string> ExtraHeaders;

				Url << "http://";

				if (bIsV4) {
					// Create basic authentication header
					std::stringstream sstr;
					sstr << fibaroUsername << ":" << fibaroPassword;
					std::string m_AccessToken = base64_encode((const unsigned char *)(sstr.str().c_str()), strlen(sstr.str().c_str()));
					ExtraHeaders.push_back("Authorization:Basic " + m_AccessToken);
				} 
				else {
					Url << fibaroUsername << ":" << fibaroPassword << "@"; // Add user name in url for earlier than V4
				}

				Url << fibaroIP << "/";

				sendValue = CURLEncode::URLEncode(sendValue);
				
				if (targetType==0) {
					Url << "api/globalVariables";

					if (bIsV4)
						Url << "/" << targetVariable;

					sPostData << "{\"name\": \"" << targetVariable << "\", \"value\": \"" << sendValue << "\"";
					
					if (bIsV4)
						sPostData << ", \"invokeScenes\": true";
					
					sPostData << " }";

					if (fibaroDebugActive) {
						_log.Log(LOG_NORM,"FibaroLink: sending global variable %s with value: %s",targetVariable.c_str(),sendValue.c_str());
					}
					if (!HTTPClient::PUT(Url.str(),sPostData.str(),ExtraHeaders,sResult))
					{
						_log.Log(LOG_ERROR,"Error sending data to Fibaro!");
						
					}
				}	
				else if (targetType==1) {
					Url << "api/callAction?deviceid=" << targetDeviceID << "&name=setProperty&arg1=" << targetProperty << "&arg2=" << sendValue;
					if (fibaroDebugActive) {
						_log.Log(LOG_NORM,"FibaroLink: sending value %s to property %s of virtual device id %d",sendValue.c_str(),targetProperty.c_str(),targetDeviceID);
					}
					if (!HTTPClient::GET(Url.str(), ExtraHeaders, sResult))
					{
						_log.Log(LOG_ERROR,"Error sending data to Fibaro!");
					}
				}
				else if (targetType==2) {
					if (((delpos==0)&&(lstatus=="Off"))||((delpos==1)&&(lstatus=="On"))) {
						Url << "api/sceneControl?id=" << targetDeviceID << "&action=start";
						if (fibaroDebugActive) {
							_log.Log(LOG_NORM,"FibaroLink: activating scene %d",targetDeviceID);
						}
						if (!HTTPClient::GET(Url.str(), ExtraHeaders, sResult))
						{
							_log.Log(LOG_ERROR,"Error sending data to Fibaro!");
						}
					}
				}
				else if (targetType==3) {
					if (((delpos==0)&&(lstatus=="Off"))||((delpos==1)&&(lstatus=="On"))) {
						Url << "api/settings/reboot";
						if (fibaroDebugActive) {
							_log.Log(LOG_NORM,"FibaroLink: reboot");
						}
						if (!HTTPClient::POST(Url.str(),sPostData.str(),ExtraHeaders,sResult))
						{
							_log.Log(LOG_ERROR,"Error sending data to Fibaro!");
						}
					}
				}
			}
		}
	}
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_SaveFibaroLinkConfig(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string remote = request::findValue(&req, "remote");
			std::string username = request::findValue(&req, "username");
			std::string password = request::findValue(&req, "password");
			std::string linkactive = request::findValue(&req, "linkactive");
			std::string isversion4 = request::findValue(&req, "isversion4");
			std::string debugenabled = request::findValue(&req, "debugenabled");
			if (
				(remote == "") ||
				(username == "") ||
				(password == "") ||
				(linkactive == "") ||
				(isversion4 == "") ||
				(debugenabled == "")
				)
				return;
			int ilinkactive = atoi(linkactive.c_str());
			int iisversion4 = atoi(isversion4.c_str());
			int idebugenabled = atoi(debugenabled.c_str());
			m_sql.UpdatePreferencesVar("FibaroIP", remote.c_str());
			m_sql.UpdatePreferencesVar("FibaroUsername", username.c_str());
			m_sql.UpdatePreferencesVar("FibaroPassword", password.c_str());
			m_sql.UpdatePreferencesVar("FibaroActive", ilinkactive);
			m_sql.UpdatePreferencesVar("FibaroVersion4", iisversion4);
			m_sql.UpdatePreferencesVar("FibaroDebug", idebugenabled);
			m_fibaropush.UpdateActive();
			root["status"] = "OK";
			root["title"] = "SaveFibaroLinkConfig";
		}

		void CWebServer::Cmd_GetFibaroLinkConfig(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			std::string sValue;
			int nValue;
			if (m_sql.GetPreferencesVar("FibaroActive", nValue)) {
				root["FibaroActive"] = nValue;
			}
			else {
				root["FibaroActive"] = 0;
			}
			if (m_sql.GetPreferencesVar("FibaroVersion4", nValue)) {
				root["FibaroVersion4"] = nValue;
			}
			else {
				root["FibaroVersion4"] = 0;
			}
			if (m_sql.GetPreferencesVar("FibaroDebug", nValue)) {
				root["FibaroDebug"] = nValue;
			}
			else {
				root["FibaroDebug"] = 0;
			}
			if (m_sql.GetPreferencesVar("FibaroIP", sValue))
			{
				root["FibaroIP"] = sValue;
			}
			if (m_sql.GetPreferencesVar("FibaroUsername", sValue))
			{
				root["FibaroUsername"] = sValue;
			}
			if (m_sql.GetPreferencesVar("FibaroPassword", sValue))
			{
				root["FibaroPassword"] = sValue;
			}
			root["status"] = "OK";
			root["title"] = "GetFibaroLinkConfig";
		}

		void CWebServer::Cmd_GetFibaroLinks(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT A.ID,A.DeviceID,A.Delimitedvalue,A.TargetType,A.TargetVariable,A.TargetDeviceID,A.TargetProperty,A.Enabled, B.Name, A.IncludeUnit FROM FibaroLink as A, DeviceStatus as B WHERE (A.DeviceID==B.ID)");
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
			root["title"] = "GetFibaroLinks";
		}

		void CWebServer::Cmd_SaveFibaroLink(WebEmSession & session, const request& req, Json::Value &root)
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
			if ((targettypei == 0) && (targetvariable == ""))
				return;
			if ((targettypei == 1) && ((targetdeviceid == "") || (targetproperty == "")))
				return;
			if ((targettypei == 2) && (targetdeviceid == ""))
				return;
			if (idx == "0") {
				m_sql.safe_query(
					"INSERT INTO FibaroLink (DeviceID,DelimitedValue,TargetType,TargetVariable,TargetDeviceID,TargetProperty,IncludeUnit,Enabled) VALUES ('%d','%d','%d','%q','%d','%q','%d','%d')",
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
					"UPDATE FibaroLink SET DeviceID='%d', DelimitedValue=%d, TargetType=%d, TargetVariable='%q', TargetDeviceID=%d, TargetProperty='%q', IncludeUnit='%d', Enabled='%d' WHERE (ID == '%q')",
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
			root["status"] = "OK";
			root["title"] = "SaveFibaroLink";
		}

		void CWebServer::Cmd_DeleteFibaroLink(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			m_sql.safe_query("DELETE FROM FibaroLink WHERE (ID=='%q')", idx.c_str());
			root["status"] = "OK";
			root["title"] = "DeleteFibaroLink";
		}
	}
}