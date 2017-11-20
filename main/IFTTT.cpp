#include "stdafx.h"
#include "IFTTT.h"
#include "SQLHelper.h"
#include "Logger.h"
#include "../httpclient/HTTPClient.h"
#include "../json/json.h"
#include "../webserver/Base64.h"

bool IFTTT::Send_IFTTT_Trigger(const std::string &eventid, const std::string &svalue1, const std::string &svalue2, const std::string &svalue3)
{
	int n2Value;
	std::string sKey;
	if (
		(m_sql.GetPreferencesVar("IFTTTEnabled", n2Value)) &&
		(m_sql.GetPreferencesVar("IFTTTAPI", sKey))
		)
	{
		if (
			(n2Value == 1) &&
			(!sKey.empty())
			)
		{
			std::string sSend = "Sending IFTTT trigger: " + eventid;

			Json::Value root;
			root["value1"] = svalue1;
			root["value2"] = svalue2;
			root["value3"] = svalue3;

			std::string szPostdata = root.toStyledString();
			std::vector<std::string> ExtraHeaders;
			ExtraHeaders.push_back("content-type: application/json");

			std::string sURL;
			sURL = "https://maker.ifttt.com/trigger/" + eventid + "/with/key/" + base64_decode(sKey);
			std::string sResult;
			bool bRet = HTTPClient::POST(sURL, szPostdata, ExtraHeaders, sResult);
			if (!bRet)
			{
				_log.Log(LOG_ERROR, "Error sending trigger to IFTTT! (Check EventName/Key!)");
				sSend += " => Failed!";
				_log.Log(LOG_ERROR, sSend.c_str());
				return false;
			}
			else
			{
				sSend += " => Success!";
				_log.Log(LOG_STATUS, sSend.c_str());
				return true;
			}
		}
	}
	return false;
}