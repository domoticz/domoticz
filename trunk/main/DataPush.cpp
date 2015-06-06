#include "stdafx.h"
#include "DataPush.h"
#include "Helper.h"
#include "../httpclient/HTTPClient.h"
#include "Logger.h"
#include "../hardware/hardwaretypes.h"
#include "RFXtrx.h"
#include "SQLHelper.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include "../main/mainworker.h"
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
	m_bFibaroLinkActive = false;
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
	m_bFibaroLinkActive = (fActive == 1);
}

void CDataPush::OnDeviceReceived(const int m_HwdID, const unsigned long long DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand)
{
	m_DeviceRowIdx = DeviceRowIdx;
	if (m_bFibaroLinkActive)
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

std::vector<std::string> CDataPush::DropdownOptions(const unsigned long long DeviceRowIdxIn)
{
	std::vector<std::string> dropdownOptions;

	std::vector<std::vector<std::string> > result;
	char szTmp[300];
	sprintf(szTmp, "SELECT Type, SubType FROM DeviceStatus WHERE (ID== %llu)", DeviceRowIdxIn);
	result=m_sql.query(szTmp);
	if (result.size()>0)
	{
		int dType=atoi(result[0][0].c_str());
		int dSubType=atoi(result[0][1].c_str());

		std::string sOptions = RFX_Type_SubType_Values(dType,dSubType);
		std::vector<std::string> tmpV; 
		StringSplit(sOptions, ",", tmpV); 
		for (int i = 0; i < (int) tmpV.size(); ++i) { 
			dropdownOptions.push_back(tmpV[i]); 
		}
	}
   	else 
	{
		dropdownOptions.push_back("Not implemented");
	}
	return dropdownOptions;
}

std::string CDataPush::DropdownOptionsValue(const unsigned long long DeviceRowIdxIn, const int pos)
{	
	std::string wording = "???";
	int getpos = pos-1; // 0 pos is always nvalue/status, 1 and higher goes to svalues
	std::vector<std::vector<std::string> > result;
	char szTmp[300];
	
	sprintf(szTmp, "SELECT Type, SubType FROM DeviceStatus WHERE (ID== %llu)", DeviceRowIdxIn);
	result=m_sql.query(szTmp);
	if (result.size()>0)
	{
		int dType=atoi(result[0][0].c_str());
		int dSubType=atoi(result[0][1].c_str());

		std::string sOptions = RFX_Type_SubType_Values(dType,dSubType);
		std::vector<std::string> tmpV; 
		StringSplit(sOptions, ",", tmpV); 
		if ( (int) tmpV.size() >= pos) {
			wording = tmpV[getpos];
		}
	}
	return wording;
}

std::string CDataPush::ProcessSendValue(const std::string &rawsendValue, const int delpos, const int nValue, const int includeUnit, const int metertypein)
{
	std::string vType = DropdownOptionsValue(m_DeviceRowIdx,delpos);
	unsigned char tempsign=m_sql.m_tempsign[0];
	_eMeterType metertype = (_eMeterType) metertypein;
	char szData[100]= "";

	if ((vType=="Temperature") || (vType=="Temperature 1") || (vType=="Temperature 2")|| (vType == "Set point"))
	{
		double tvalue=ConvertTemperature(atof(rawsendValue.c_str()),tempsign);
		if (includeUnit) {
			sprintf(szData,"%.1f %c", tvalue,tempsign);
		}
		else {
			sprintf(szData,"%.1f", tvalue);
		}
	
	}
	else if (vType == "Humidity")
	{
		if (includeUnit) {
			sprintf(szData,"%d %%", atoi(rawsendValue.c_str()));
		}
		else {
			sprintf(szData,"%d", atoi(rawsendValue.c_str()));
		}

	}
	else if (vType == "Humidity Status")
	{
			sprintf(szData,"%s", RFX_Humidity_Status_Desc(atoi(rawsendValue.c_str())));	
	}
	else if (vType == "Barometer")
	{
		if (includeUnit) {
			sprintf(szData,"%.1f hPa", atof(rawsendValue.c_str()));
		}
		else {
			sprintf(szData,"%.1f", atof(rawsendValue.c_str()));
		}

	}
	else if (vType == "Forecast")
	{
		int forecast=atoi(rawsendValue.c_str());
		if (forecast!=baroForecastNoInfo)
		{
			sprintf(szData,"%s", RFX_Forecast_Desc(forecast));
		}
		else {
			sprintf(szData,"%d", forecast);
		}
	}
	else if (vType == "Altitude")
	{
		sprintf(szData,"Not supported yet");
	}

	else if (vType == "UV")
	{
		float UVI = static_cast<float>(atof(rawsendValue.c_str()));
		if (includeUnit) {
			sprintf(szData,"%.1f UVI",UVI);
		}
		else {
			sprintf(szData,"%.1f",UVI);
		}
	}
	else if (vType == "Direction")
	{
		float Direction = static_cast<float>(atof(rawsendValue.c_str()));
		if (includeUnit) {
			sprintf(szData,"%.1f Degrees",Direction); 
		}
		else {
			sprintf(szData,"%.1f",Direction); 
		}
	}
	else if (vType == "Direction string")
	{
		sprintf(szData,"%s",rawsendValue.c_str());
	}
	else if (vType == "Speed")
	{
		int intSpeed=atoi(rawsendValue.c_str());
		if (includeUnit) {
			sprintf(szData,"%.1f",float(intSpeed) * m_sql.m_windscale); //todo: unit?
		}
		else {
			sprintf(szData,"%.1f",float(intSpeed) * m_sql.m_windscale);
		}
	}
	else if (vType == "Gust")
	{
		int intGust=atoi(rawsendValue.c_str());
		if (includeUnit) {
			sprintf(szData,"%.1f",float(intGust) *m_sql.m_windscale); //todo: unit?
		}
		else {
			sprintf(szData,"%.1f",float(intGust) *m_sql.m_windscale);
		}
	}
	else if (vType == "Chill")
	{
		double tvalue=ConvertTemperature(atof(rawsendValue.c_str()),tempsign);
		if (includeUnit) {
			sprintf(szData,"%.1f %c", tvalue,tempsign);
		}
		else {
			sprintf(szData,"%.1f", tvalue);
		}
	}
	else if (vType == "Rain rate")
	{
		sprintf(szData,"Not supported yet");
	}
	else if (vType == "Total rain")
	{
		sprintf(szData,"Not supported yet");
	}	
	else if (vType == "Counter")
	{
		sprintf(szData,"Not supported yet");
	}	
	else if (vType == "Mode")
	{
		sprintf(szData,"Not supported yet");
	}
	else if (vType == "Status")
	{
		sprintf(szData,"Not supported yet");
	}	
	else if ((vType == "Current 1") || (vType == "Current 2") || (vType == "Current 3"))
	{
		sprintf(szData,"Not supported yet");
	}	
	else if (vType == "Instant")
	{
		sprintf(szData,"Not supported yet");
	}	
	else if ((vType == "Usage") || (vType == "Usage 1") || (vType == "Usage 2") )
	{
		if (includeUnit) {
			sprintf(szData,"%.1f Watt",atof(rawsendValue.c_str()));
		}
		else {
			sprintf(szData,"%.1f",atof(rawsendValue.c_str()));
		}
	}	
	else if ((vType == "Delivery") || (vType == "Delivery 1") || (vType == "Delivery 2") )
	{
		if (includeUnit) {
			sprintf(szData,"%.1f Watt",atof(rawsendValue.c_str()));
		}
		else {
			sprintf(szData,"%.1f",atof(rawsendValue.c_str()));
		}
	}

	else if (vType == "Usage current")
	{
		if (includeUnit) {
			sprintf(szData,"%.1f Watt",atof(rawsendValue.c_str()));
		}
		else {
			sprintf(szData,"%.1f",atof(rawsendValue.c_str()));
		}
	}
	else if (vType == "Delivery current")
	{
		if (includeUnit) {
			sprintf(szData,"%.1f Watt",atof(rawsendValue.c_str()));
		}
		else {
			sprintf(szData,"%.1f",atof(rawsendValue.c_str()));
		}
	}
	else if (vType == "Gas usage")
	{
		sprintf(szData,"Not supported yet");
	}
	else if (vType == "Weight")
	{
		if (includeUnit) {
			sprintf(szData,"%.1f kg",atof(rawsendValue.c_str()));
		}
		else {
			sprintf(szData,"%.1f",atof(rawsendValue.c_str()));
		}
	}	
	else if (vType == "Voltage")
	{
		if (includeUnit) {
			sprintf(szData,"%.3f V",atof(rawsendValue.c_str()));
		}
		else {
			sprintf(szData,"%.3f",atof(rawsendValue.c_str()));
		}
	}
	else if (vType == "Value")
	{
		sprintf(szData,"%d", atoi(rawsendValue.c_str())); //??
	}
	else if (vType == "Visibility")
	{
		float vis = static_cast<float>(atof(rawsendValue.c_str()));
		if (includeUnit) {
			if (metertype==0)
			{
				//km
				sprintf(szData,"%.1f km",vis);
			}
			else
			{
				//miles
				sprintf(szData,"%.1f mi",vis*0.6214f);
			}
		}
		else {
			if (metertype==0)
			{
				//km
				sprintf(szData,"%.1f",vis);
			}
			else
			{
				//miles
				sprintf(szData,"%.1f",vis*0.6214f);
			}
		}
	}
	else if (vType == "Solar Radiation")
	{
		float radiation = static_cast<float>(atof(rawsendValue.c_str()));
		if (includeUnit) {
			sprintf(szData,"%.1f Watt/m2",radiation);
		}
		else {
			sprintf(szData,"%.1f",radiation);
		}
	}
	else if (vType == "Soil Moisture")
	{
		if (includeUnit) {
			sprintf(szData,"%d cb",nValue);
		}
		else {
			sprintf(szData,"%d",nValue);
		}
	}
	else if (vType == "Leaf Wetness")
	{
		sprintf(szData,"%d",nValue);
	}
	else if (vType == "Percentage")
	{
		if (includeUnit) {
			sprintf(szData,"%.2f%%",atof(rawsendValue.c_str()));
		}
		else {
			sprintf(szData,"%.2f",atof(rawsendValue.c_str()));
		}
	}
	else if (vType == "Fanspeed")
	{
		if (includeUnit) {
			sprintf(szData,"%d RPM",atoi(rawsendValue.c_str()));
		}
		else {
			sprintf(szData,"%d",atoi(rawsendValue.c_str()));
		}
	}
	else if (vType == "Pressure")
	{
		if (includeUnit) {
			sprintf(szData,"%.1f Bar",atof(rawsendValue.c_str()));
		}
		else {
			sprintf(szData,"%.1f",atof(rawsendValue.c_str()));
		}
	}
	else if (vType == "Lux")
	{
		if (includeUnit) {
			sprintf(szData,"%.0f Lux",atof(rawsendValue.c_str()));
		}
		else {
			sprintf(szData,"%.0f",atof(rawsendValue.c_str()));
		}
	}
	if (szData[0] != '\0') { 
		std::string sendValue(szData);
		return sendValue;
	}
	else {
		_log.Log(LOG_ERROR,"Could not determine data push value");
		return "";
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
