#include "stdafx.h"
#include "DataPush.h"
#include "Helper.h"
#include "../httpclient/HTTPClient.h"
#include "Logger.h"
#include "mainworker.h"


void CDataPush::SetMainWorker(MainWorker *pMainWorker)
{
	m_pMain=pMainWorker;
}

void CDataPush::DoWork(const unsigned long long DeviceRowIdxIn)
{
	DeviceRowIdx = DeviceRowIdxIn;
	int fActive;
	m_pMain->m_sql.GetPreferencesVar("FibaroActive", fActive);
	if (fActive) {
		DoFibaroPush();
	}
}

void CDataPush::DoFibaroPush()
{			
	std::string fibaroIP = "";
	std::string fibaroUsername = "";
	std::string fibaroPassword = "";
	m_pMain->m_sql.GetPreferencesVar("FibaroIP", fibaroIP);
	m_pMain->m_sql.GetPreferencesVar("FibaroUsername", fibaroUsername);
	m_pMain->m_sql.GetPreferencesVar("FibaroPassword", fibaroPassword);

	if (
		(fibaroIP!="")&&
		(fibaroUsername!="")&&
		(fibaroPassword!="")
	) {
		std::vector<std::vector<std::string> > result;
		char szTmp[300];
		sprintf(szTmp, 
			"SELECT A.DeviceID, A.DelimitedValue, B.ID, B.Type, B.SubType, B.nValue, B.sValue, A.TargetType, A.TargetVariable, A.TargetDeviceID, A.TargetProperty FROM FibaroLink as A, DeviceStatus as B "
			"WHERE (A.DeviceID == '%llu' AND A.Enabled = '1' AND A.DeviceID==B.ID)",
			DeviceRowIdx);
		result=m_pMain->m_sql.query(szTmp);
		if (result.size()>0)
		{
			std::string sendValue;
			std::vector<std::vector<std::string> >::const_iterator itt;
			int ii=0;
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

				if (delpos == 0) {
					std::string lstatus="";
					int llevel=0;
					bool bHaveDimmer=false;
					bool bHaveGroupCmd=false;
					int maxDimLevel=0;
    				GetLightStatus(dType,dSubType,nValue,sValue,lstatus,llevel,bHaveDimmer,maxDimLevel,bHaveGroupCmd);
					sendValue = lstatus;
				}
				else if (delpos>0) {
					std::vector<std::string> strarray;
					if (sValue.find(";")!=std::string::npos) {
						StringSplit(sValue, ";", strarray);
						if (int(strarray.size())>=delpos)
						{
							sendValue = strarray[delpos-1].c_str();
						}
					}
				}
				if (sendValue !="") {
					std::string sResult;
					std::stringstream sPostData;
					std::stringstream Url;
					std::vector<std::string> ExtraHeaders;
					
					if (targetType==0) {
						Url << "http://" << fibaroUsername << ":" << fibaroPassword << "@" << fibaroIP << "/api/globalVariables";
						sPostData << "{\"name\": \"" << targetVariable << "\", \"value\": \"" << sendValue << "\"}";
						if (!HTTPClient::PUT(Url.str(),sPostData.str(),ExtraHeaders,sResult))
						{
							_log.Log(LOG_ERROR,"Error sending data to Fibaro!");
						
						}
						//_log.Log(LOG_NORM,"response: %s",sResult.c_str());

					}	
					else if (targetType==1) {
						Url << "http://" << fibaroUsername << ":" << fibaroPassword << "@" << fibaroIP << "/api/callAction?deviceid=" << targetDeviceID << "&name=setProperty&arg1=" << targetProperty << "&arg2=" << sendValue;
						if (!HTTPClient::GET(Url.str(),sResult))
						{
							_log.Log(LOG_ERROR,"Error sending data to Fibaro!");
						}
						//_log.Log(LOG_NORM,"response: %s",sResult.c_str());
					}
				}
			}
		}
	}
}
