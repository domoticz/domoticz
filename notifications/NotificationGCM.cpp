#include "stdafx.h"
#include "NotificationGCM.h"
#include "../httpclient/HTTPClient.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../json/json.h"
#include <boost/lexical_cast.hpp>

#define GAPI_POST_URL "https://gcm-http.googleapis.com/gcm/send"
#define GAPI "AIzaSyBnRMroiDaXCKbwPeOmoxkNiQfjWkGMre8"

CNotificationGCM::CNotificationGCM() : CNotificationBase(std::string("gcm"), OPTIONS_URL_SUBJECT | OPTIONS_URL_BODY | OPTIONS_URL_PARAMS)
{
	SetupConfig(std::string("GCMEnabled"), &m_IsEnabled);
}

CNotificationGCM::~CNotificationGCM()
{
}

bool CNotificationGCM::SendMessageImplementation(
	const uint64_t Idx,
	const std::string &Name,
	const std::string &Subject,
	const std::string &Text,
	const std::string &ExtraData,
	const int Priority,
	const std::string &Sound,
	const bool bFromNotification)
{
	//send message to GCM

	//Get All Devices
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT SenderID FROM MobileDevices WHERE (Active == 1)");
	if (result.empty())
		return true;

	std::stringstream sstr, sstr2;
	std::string sResult;
	sstr << "{ \"registration_ids\": [";

	int ii = 0;
	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt = result.begin(); itt != result.end(); ++itt)
	{
		std::vector<std::string> sd = *itt;
		if (ii != 0)
			sstr << ", ";
		sstr << "\"" << sd[0] << "\"";
		ii++;
	}
	sstr << "], \"data\" : { \"subject\": \""<< Subject << "\", \"body\": \""<< Text << "\", \"extradata\": \""<< ExtraData << "\", \"priority\": \""<< boost::lexical_cast<std::string>(Priority) << "\", ";
	sstr << "\"deviceid\": \""<< boost::lexical_cast<std::string>(Idx) << "\", \"message\": \"" << Subject << "\" } }";
	std::string szPostdata = sstr.str();

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Content-Type: application/json");

	sstr2 << "Authorization: key=" << GAPI;
	ExtraHeaders.push_back(sstr2.str());

	if (!HTTPClient::POST(GAPI_POST_URL, szPostdata, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "GCM: Could not send message, HTTP Error");
		return false;
	}

	Json::Value root;

	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "GCM: Can not connect to GCM API URL");
		return false;
	}
	return true;
/*
//silence the errors, we should change this system anyway
bool bSuccess = root["success"].asInt() == 1;
if (!bSuccess)
_log.Log(LOG_ERROR, "GCM: %s", root["error"].asString().c_str());
return bSuccess;
*/
}

bool CNotificationGCM::IsConfigured()
{
	return true;
}
