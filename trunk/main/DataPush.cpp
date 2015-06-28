#include "stdafx.h"
#include "DataPush.h"
#include "Helper.h"
#include "../httpclient/HTTPClient.h"
#include "Logger.h"
#include "../hardware/hardwaretypes.h"
#include "RFXtrx.h"
#include "SQLHelper.h"
#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include "../json/json.h"

typedef struct _STR_TABLE_ID1_ID2 {
	unsigned long    id1;
	unsigned long    id2;
	const char   *str1;
} STR_TABLE_ID1_ID2;

const char *findVarTableID1ID2 (_STR_TABLE_ID1_ID2 *t, unsigned long id1, unsigned long id2)
{
	while (t->str1) {
		if ( (t->id1 == id1) && (t->id2 == id2) )
			return t->str1;
		t++;
	}
	return "Not supported";
}

CDataPush::CDataPush()
{
	m_bLinkActive = false;
}

void CDataPush::Start()
{
	UpdateActive();
	m_sConnection = m_mainworker.sOnDeviceReceived.connect(boost::bind(&CDataPush::OnDeviceReceived, this, _1, _2, _3, _4));
}

void CDataPush::Stop()
{
	if (m_sConnection.connected())
		m_sConnection.disconnect();
}

void CDataPush::UpdateActive()
{
	int fActive;
	m_sql.GetPreferencesVar("FibaroActive", fActive);
	m_bLinkActive = (fActive == 1);
}

void CDataPush::OnDeviceReceived(const int m_HwdID, const unsigned long long DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand)
{
	m_DeviceRowIdx = DeviceRowIdx;
	if (m_bLinkActive)
	{
		DoFibaroPush();
	}
}

void CDataPush::DoFibaroPush()
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
	char szTmp[300];
	sprintf(szTmp, 
		"SELECT A.DeviceID, A.DelimitedValue, B.ID, B.Type, B.SubType, B.nValue, B.sValue, A.TargetType, A.TargetVariable, A.TargetDeviceID, A.TargetProperty, A.IncludeUnit, B.SwitchType FROM FibaroLink as A, DeviceStatus as B "
		"WHERE (A.DeviceID == '%llu' AND A.Enabled = '1' AND A.DeviceID==B.ID)",
		m_DeviceRowIdx);
	result=m_sql.query(szTmp);
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
							sendValue = ProcessSendValue(rawsendValue,delpos,nValue,includeUnit,metertype);
						}
					}
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
					sendValue = CURLEncode::URLEncode(sendValue);
					
				if (targetType==0) {
					Url << "http://" << fibaroUsername << ":" << fibaroPassword << "@" << fibaroIP << "/api/globalVariables";

					if (bIsV4)
						Url << "/" << targetVariable;

					sPostData << "{\"name\": \"" << targetVariable << "\", \"value\": \"" << sendValue << "\"}";
					if (fibaroDebugActive) {
						_log.Log(LOG_NORM,"FibaroLink: sending global variable %s with value: %s",targetVariable.c_str(),sendValue.c_str());
					}
					if (!HTTPClient::PUT(Url.str(),sPostData.str(),ExtraHeaders,sResult))
					{
						_log.Log(LOG_ERROR,"Error sending data to Fibaro!");
						
					}
				}	
				else if (targetType==1) {
					Url << "http://" << fibaroUsername << ":" << fibaroPassword << "@" << fibaroIP << "/api/callAction?deviceid=" << targetDeviceID << "&name=setProperty&arg1=" << targetProperty << "&arg2=" << sendValue;
					if (fibaroDebugActive) {
						_log.Log(LOG_NORM,"FibaroLink: sending value %s to property %s of virtual device id %d",sendValue.c_str(),targetProperty.c_str(),targetDeviceID);
					}
					if (!HTTPClient::GET(Url.str(),sResult))
					{
						_log.Log(LOG_ERROR,"Error sending data to Fibaro!");
					}
				}
				else if (targetType==2) {
					if (((delpos==0)&&(lstatus=="Off"))||((delpos==1)&&(lstatus=="On"))) {
						Url << "http://" << fibaroUsername << ":" << fibaroPassword << "@" << fibaroIP << "/api/sceneControl?id=" << targetDeviceID << "&action=start";
						if (fibaroDebugActive) {
							_log.Log(LOG_NORM,"FibaroLink: activating scene %d",targetDeviceID);
						}
						if (!HTTPClient::GET(Url.str(),sResult))
						{
							_log.Log(LOG_ERROR,"Error sending data to Fibaro!");
						}
					}
				}
				else if (targetType==3) {
					if (((delpos==0)&&(lstatus=="Off"))||((delpos==1)&&(lstatus=="On"))) {
						Url << "http://" << fibaroUsername << ":" << fibaroPassword << "@" << fibaroIP << "/api/settings/reboot";
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
		void CWebServer::Cmd_SaveFibaroLinkConfig(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string remote = m_pWebEm->FindValue("remote");
			std::string username = m_pWebEm->FindValue("username");
			std::string password = m_pWebEm->FindValue("password");
			std::string linkactive = m_pWebEm->FindValue("linkactive");
			std::string isversion4 = m_pWebEm->FindValue("isversion4");
			std::string debugenabled = m_pWebEm->FindValue("debugenabled");
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
			m_mainworker.m_datapush.UpdateActive();
			root["status"] = "OK";
			root["title"] = "SaveFibaroLinkConfig";
		}

		void CWebServer::Cmd_GetFibaroLinkConfig(Json::Value &root)
		{
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

		void CWebServer::Cmd_GetFibaroLinks(Json::Value &root)
		{
			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT A.ID,A.DeviceID,A.Delimitedvalue,A.TargetType,A.TargetVariable,A.TargetDeviceID,A.TargetProperty,A.Enabled, B.Name, A.IncludeUnit FROM FibaroLink as A, DeviceStatus as B WHERE (A.DeviceID==B.ID)";
			result = m_sql.query(szQuery.str());
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

		void CWebServer::Cmd_SaveFibaroLink(Json::Value &root)
		{
			std::string idx = m_pWebEm->FindValue("idx");
			std::string deviceid = m_pWebEm->FindValue("deviceid");
			int deviceidi = atoi(deviceid.c_str());
			std::string valuetosend = m_pWebEm->FindValue("valuetosend");
			std::string targettype = m_pWebEm->FindValue("targettype");
			int targettypei = atoi(targettype.c_str());
			std::string targetvariable = m_pWebEm->FindValue("targetvariable");
			std::string targetdeviceid = m_pWebEm->FindValue("targetdeviceid");
			std::string targetproperty = m_pWebEm->FindValue("targetproperty");
			std::string linkactive = m_pWebEm->FindValue("linkactive");
			std::string includeunit = m_pWebEm->FindValue("includeunit");
			if ((targettypei == 0) && (targetvariable == ""))
				return;
			if ((targettypei == 1) && ((targetdeviceid == "") || (targetproperty == "")))
				return;
			if ((targettypei == 2) && (targetdeviceid == ""))
				return;
			std::vector<std::vector<std::string> > result;
			char szTmp[300];
			if (idx == "0") {
				sprintf(szTmp, "INSERT INTO FibaroLink (DeviceID,DelimitedValue,TargetType,TargetVariable,TargetDeviceID,TargetProperty,IncludeUnit,Enabled) VALUES ('%d','%d','%d','%s','%d','%s','%d','%d')",
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
				sprintf(szTmp,
					"UPDATE FibaroLink SET DeviceID='%d', DelimitedValue=%d, TargetType=%d, TargetVariable='%s', TargetDeviceID=%d, TargetProperty='%s', IncludeUnit='%d', Enabled='%d' WHERE (ID == %s)",
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
			result = m_sql.query(szTmp);

			root["status"] = "OK";
			root["title"] = "SaveFibaroLink";
		}

		void CWebServer::Cmd_DeleteFibaroLink(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "")
				return;
			std::vector<std::vector<std::string> > result;
			char szTmp[300];
			sprintf(szTmp, "DELETE FROM FibaroLink WHERE (ID==%s)", idx.c_str());
			result = m_sql.query(szTmp);

			root["status"] = "OK";
			root["title"] = "DeleteFibaroLink";
		}
	}
}
