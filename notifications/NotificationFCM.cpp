#include "stdafx.h"
#include "NotificationFCM.h"
#include "../httpclient/HTTPClient.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/json_helper.h"

#define GAPI_FCM_POST_URL_BASE "https://fcm.googleapis.com/v1/projects/##PROJECTID##/messages:send"

// FCM v1 send message format
//{
//  "validate_only": boolean,
//  "message": {
//    object (Message)
//  }
//}

// FCM v1 message format
//{
//  "name": string,				# Output only!
//  "data": {
//    string: string,			# An object containing a list of "key": value pairs. Example: { "name": "wrench", "mass": "1.3kg", "count": "3" }.
//    ...
//  },
//  "notification": {
//  	"title": string,
//  	"body": string,
//  	"image": string
//  },
//  "android": {
//    object (AndroidConfig)
//  },
//  "webpush": {
//    object (WebpushConfig)
//  },
//  "apns": {
//    object (ApnsConfig)
//  },
//  "fcm_options": {
//    object (FcmOptions)
//  },
//
//  // Union field target can be only one of the following:
//  "token": string,			# Registration token to send a message to, either a single device or a devicegroup
//  "topic": string,
//  "condition": string
//  // End of list of possible types for union field target.
//}

CNotificationFCM::CNotificationFCM() : CNotificationBase(std::string("fcm"), OPTIONS_NONE)
{
	SetupConfig(std::string("FCMEnabled"), &m_IsEnabled);
}

bool CNotificationFCM::IsConfigured()
{
	// Check if the FCM Project ID is set
	if (GAPI_FCM_ProjectID.empty())
	{
		// Let's replace this later with something better... this beats at least hardcoding some values that should not be hardcoded
		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT ID, Value FROM UserVariables WHERE (Name=='GAPI_FCM_ProjectID')");
		if (!result.empty())
		{
			GAPI_FCM_ProjectID = result[0][1];
		}
		else
		{
			_log.Log(LOG_ERROR, "FCM: No FCM ProjectID set!");
			return false;
		}
	}
	GAPI_FCM_PostURL = GAPI_FCM_POST_URL_BASE;
	stdreplace(GAPI_FCM_PostURL, "##PROJECTID##", GAPI_FCM_ProjectID);
	// Check if the FCM Bearer token is set
	if (GAPI_FCM_bearer_token.empty())
	{
		// Let's replace this later with something better... this beats at least hardcoding some values that should not be hardcoded
		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT ID, Value FROM UserVariables WHERE (Name=='GAPI_FCM_bearer_token')");
		if (!result.empty())
		{
			GAPI_FCM_bearer_token = result[0][1];
		}
		else
		{
			_log.Log(LOG_ERROR, "FCM: No FCM Bearer token found!");
			return false;
		}
	}
	return true;
}

bool CNotificationFCM::SendMessageImplementation(
	const uint64_t Idx,
	const std::string &Name,
	const std::string &Subject,
	const std::string &Text,
	const std::string &ExtraData,
	const int Priority,
	const std::string &Sound,
	const bool bFromNotification)
{
	//send message to FCM
	//ExtraData should be empty, is only filled currently when hitting the test button from the mobile devices setup page in the web gui

	//Get All Devices
	std::vector<std::vector<std::string> > result;

	std::string sMidx;
	std::vector<std::string> vDevices;
	if (ExtraData.find("midx_") != std::string::npos) {
		sMidx = ExtraData.substr(5);
		boost::split(vDevices, sMidx, boost::is_any_of(";"));
	}

	std::string szQuery("SELECT SenderID, DeviceType FROM MobileDevices");
	if (!vDevices.empty()) {
		szQuery += " WHERE (ID IN (" + boost::algorithm::join(vDevices, ",") + "))";
	}
	else {
		szQuery += " WHERE (Active == 1)";
	}

	result = m_sql.safe_query(szQuery.c_str());
	if (result.empty())
		return true;

	std::vector<std::string> mobileDevices;

	for (const auto &r : result)
	{
		mobileDevices.push_back(r[0]);	//Store the SenderID for later use
	}

	// Build the message
	std::stringstream sstr;

	sstr << "{ \"registration_ids\": [";

	int ii = 0;

	for (const auto &device : mobileDevices)
	{
		if (ii != 0)
			sstr << ", ";
		sstr << "\"" << device << "\"";
		ii++;
	}

	sstr << R"(], "data" : { "subject": ")" << Subject << R"(", "body": ")" << Text << R"(", "extradata": ")" << ExtraData << R"(", "priority": ")" << std::to_string(Priority) << "\", ";
	sstr << R"("deviceid": ")" << std::to_string(Idx) << R"(", "message": ")" << Subject << "\" } }";
	std::string szPostdata = sstr.str();
	_log.Debug(DEBUG_EVENTSYSTEM, "FCM: Generated message for %d devices: .%s.", ii, szPostdata.c_str());

	std::vector<std::string> ExtraHeaders;
	std::stringstream sstr2;
	std::string sResult;

	ExtraHeaders.push_back("Content-Type: application/json");

	sstr2 << "Authorization: Bearer " << GAPI_FCM_bearer_token;
	ExtraHeaders.push_back(sstr2.str());

	if (HTTPClient::POST(GAPI_FCM_PostURL, szPostdata, ExtraHeaders, sResult))
	{
		_log.Debug(DEBUG_EVENTSYSTEM, "FCM: Message sent to %d devices (%s)", ii, sResult.c_str());

		Json::Value root;

		bool ret = ParseJSon(sResult, root);
		if (!ret)
		{
			_log.Log(LOG_ERROR, "FCM: Can not connect to FCM API URL");
			return false;
		}

		if (!root["error"].empty())
		{
			Json::Value jsonError = root["error"];
			_log.Log(LOG_ERROR, "FCM: Could not send message! Errorcode %d (%s)", jsonError["code"].asInt(), jsonError["message"].asCString());
			return false;
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "FCM: Could not send message, HTTP Error");
		return false;
	}

	return true;
}
