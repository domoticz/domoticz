#include "stdafx.h"
#include "NotificationFCM.h"
#include "../httpclient/HTTPClient.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/json_helper.h"

#define JWT_DISABLE_BASE64
#include "../jwt-cpp/jwt.h"
#include "../webserver/Base64.h"

#define GAPI_FCM_POST_URL_BASE "https://fcm.googleapis.com/v1/projects/##PROJECTID##/messages:send"
#define GAPI_FCM_SCOPE "https://www.googleapis.com/auth/firebase.messaging"
#define GAPI_OAUTH2_TOKEN_URL "https://oauth2.googleapis.com/token"

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
	// Check if the FCM issuer is set
	if (GAPI_FCM_issuer.empty())
	{
		// Let's replace this later with something better... this beats at least hardcoding some values that should not be hardcoded
		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT ID, Value FROM UserVariables WHERE (Name=='GAPI_FCM_issuer')");
		if (!result.empty())
		{
			GAPI_FCM_issuer = result[0][1];
		}
		else
		{
			_log.Log(LOG_ERROR, "FCM: No FCM issuer found!");
			return false;
		}
	}
	// Check if the FCM private key is set
	if (GAPI_FCM_privkey.empty())
	{
		// Let's replace this later with something better... this beats at least hardcoding some values that should not be hardcoded
		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT ID, Value FROM UserVariables WHERE (Name=='GAPI_FCM_privkey')");
		if (!result.empty())
		{
			GAPI_FCM_privkey = result[0][1];
		}
		else
		{
			_log.Log(LOG_ERROR, "FCM: No FCM private key found!");
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

	std::string sMidx;
	std::vector<std::string> vDevices;
	std::vector<std::string> vExtraData;
	if (ExtraData.find("midx_") != std::string::npos) {
		sMidx = ExtraData.substr(5);
		boost::split(vDevices, sMidx, boost::is_any_of(";"));
	}
	else if (ExtraData.find("|") != std::string::npos) {
		std::string temp;
		std::stringstream tempssstr { ExtraData };

		while (std::getline(tempssstr, temp, '|')) {
			vExtraData.push_back(temp);
		}
	}

	//Get All Devices
	std::vector<std::vector<std::string> > result;
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

	sstr << R"({ "validate_only": true, "message": {)";	// Open Send Message struct

	if (!vExtraData.empty())
	{
		sstr << R"("data": { "ExtraData": ")" << ExtraData << R"("}, )";
	}

	if (bFromNotification)
	{
		sstr << R"("notification": { "title": ")" << Subject << R"(", "body": ")" << Text << R"("}, )";
	}

	sstr << R"("token": ")" << mobileDevices[0] << R"(")";		// Add where to send
	sstr << R"(} })";											// Close Send Message struct
	std::string szPostdata = sstr.str();
	_log.Debug(DEBUG_EVENTSYSTEM, "FCM: Generated message for %ld devices: .%s.", mobileDevices.size(), szPostdata.c_str());

	std::string sFCMjwt, slAccessToken;
	if (!createFCMjwt(GAPI_FCM_issuer, sFCMjwt))
	{
		_log.Log(LOG_ERROR, "FCM: Unable to create JWT!");
		return false;
	}
	if (!getSlAccessToken(sFCMjwt, slAccessToken))
	{
		_log.Log(LOG_ERROR, "FCM: Unable to get access token!");
		return false;
	}

	std::vector<std::string> ExtraHeaders;
	std::stringstream sstr2;
	std::string sResult;

	sstr2 << "Authorization: Bearer " << slAccessToken;
	ExtraHeaders.push_back(sstr2.str());
	ExtraHeaders.push_back("Content-Type: application/json");

	if (HTTPClient::POST(GAPI_FCM_PostURL, szPostdata, ExtraHeaders, sResult))
	{
		_log.Debug(DEBUG_EVENTSYSTEM, "FCM: Message sent to %ld devices (%s)", mobileDevices.size(), sResult.c_str());

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

bool CNotificationFCM::getSlAccessToken(const std::string &bearer_token, std::string &slAccessToken)
{
	std::vector<std::string> ExtraHeaders;
	std::string sPostBody, sResult;

	ExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded");

	sPostBody = "grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Ajwt-bearer&assertion=" + bearer_token;

	_log.Debug(DEBUG_EVENTSYSTEM, "FCM: Requesting AccessToken (%s)", sPostBody.c_str());

	if (HTTPClient::POST(GAPI_OAUTH2_TOKEN_URL, sPostBody, ExtraHeaders, sResult))
	{
		Json::Value root;

		bool ret = ParseJSon(sResult, root);
		if (ret)
		{
			if (!root["access_token"].empty())
			{
				slAccessToken = root["access_token"].asString();
				_log.Debug(DEBUG_EVENTSYSTEM, "FCM: AccessToken retrieved (%s)", slAccessToken.c_str());
				return true;
			}
		}
		_log.Debug(DEBUG_EVENTSYSTEM, "FCM: Failed to retrieve AccessToken, JSON Error! (%s)", sResult.c_str());
	}
	else
	{
		_log.Debug(DEBUG_EVENTSYSTEM, "FCM: Failed to retrieve AccessToken, HTTP Error! (%s)", sResult.c_str());
	}
	return false;
}

bool CNotificationFCM::createFCMjwt(const std::string &FCMissuer, std::string &sFCMjwt)
{
	std::string sPrivKey;
	sPrivKey = GAPI_FCM_privkey;

	sFCMjwt.clear();

	try
	{
	auto JWT = jwt::create()
		.set_type("JWT")
		.set_issuer(FCMissuer)
		.set_audience(GAPI_OAUTH2_TOKEN_URL)
		.set_issued_at(std::chrono::system_clock::now())
		.set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds{600})
		//.set_not_before(std::chrono::system_clock::now())
		//.set_subject(user)
		//.set_key_id(std::to_string(keyID))
		//.set_id(GenerateUUID())
		.set_payload_claim("scope", jwt::claim(std::string{GAPI_FCM_SCOPE}));
		sFCMjwt = JWT.sign(jwt::algorithm::rs256{"", sPrivKey, "", ""}, &base64url_encode);
	}
	catch(const std::exception& err)
	{
		_log.Debug(DEBUG_EVENTSYSTEM,"FCM: Exception creating FCM jwt (%s)", err.what());
	}

	return !sFCMjwt.empty();
}