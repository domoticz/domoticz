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
	m_slAccessToken_exp_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now()).time_since_epoch()).count();

	SetupConfig(std::string("FCMEnabled"), &m_IsEnabled);
}

bool CNotificationFCM::IsConfigured()
{
	constexpr const char* FCMKeyB64 = R"pKey(ewogICJ0eXBlIjogInNlcnZpY2VfYWNjb3VudCIsCiAgInByb2plY3RfaWQiOiAiZG9tb3RpY3otMTE5MSIsCiAgInByaXZhdGVfa2V5X2lkIjogImYzMDgwYzQyZWMyMzQ0YTg2OWM5OWQ2OGQ1NGE5YmM2YWZkZmNjMjQiLAogICJwcml2YXRlX2tleSI6ICItLS0tLUJFR0lOIFBSSVZBVEUgS0VZLS0tLS1cbk1JSUV2Z0lCQURBTkJna3Foa2lHOXcwQkFRRUZBQVNDQktnd2dnU2tBZ0VBQW9JQkFRQ1MvVHRGY3hLNHVvdzNcbjM3VDcvYnBhWSsyRDhLVTdiS2NENGhEbDRmeDVkRVhCZ0NIK3M5YzBsaWx5N1kyakV1V21aTEZ1bCtGNEpQY0hcbjdHYm45YXp2U0taV0RtVWo0d2lBNzliY3NsMW0zOGkxcVB0TlFaNWQwajZzR1UxUVhXYlk2S0lJQXpQRWZORFJcbkN3VlQzdFJyRWJwVW1IckY5bk9hMzdrdDFtbUM4aHdQZmd0NkEyaHRONklSQ2FmUUZhNjhURXBlTVVYTk1WRndcbnhIWHRWMjM5SHYwbDIxR3NjYnZhZmV1RnNSbGxYY0dHZTJKRG5EMGtaVkpIMGhsb245eHFMZG9QV3I2VFlTU3Vcbm9pRmRNdEx5b3ZPb3ZrM2VLR3pLam1qbk5SL1JzK0M1b2xuYnZxR3JnUVNCOGpJQTFrci83bkRQS0xUOFB3SlZcblJla2svUUJ6QWdNQkFBRUNnZ0VBTjFEOGR0RHRDVDhQR05Ia0F2ZEVJOU02WmU1d3lGSEY4c1NuVmxQN0Yxd0dcbkNtR2xxWHhWcjNFWmJTcHdKS0F1YzdXdVBuQURCcHRtcWdFeDR2YUdXOUNyNUhQclpSdUNMai9VVE44RkhudDJcblJjTXZ1NjNIb3doRjFLOTY5SlVKNlBSM1VldWVsa0hndGVMZllPWlphMmZUQnhZUkVpSUM5SlJnVkpzOXZ1eTdcblJSWGtROC9aVDNaYTBpdjBNdzBYaFBnNmpTNkhWcm1qS2hVTWhNa1p5RERUN1RwNjl0amtjS3ZTSHRXMSt5VUFcbmE4eEJ5NVVBKzV3R3ExSFM0bkFUQU1MbTRmb2RleHZsV1VTVUNRaHhaYmhRc29ZVkVoNlhKdlUzYnVRalQ5SytcbkIrU3FDVVBoYktTTFJqR04zcE1PUWUwN2tQaGhqZzBOdzY1ZHcrTHpFUUtCZ1FESVVtdnNsZTBIbUs4QzhHUmVcbmxsOFZaY1FJNXhwOHhBc1V1ZFdpdytxOExIRS95NmNBcysyaG1CMWlva0VYbDdCNEtNTUhWWCthZXNrVHdRdWlcbkw5RXRNZXhPYmthZDNtMUJwOUhDb1JHMFNSUHlsaVU2a1FhUGxTNTM4OWZWN08zYm1OUFlxcGo3eUdhVXlmcEFcblhHU0tmNkh0cXVRRWd2UXVZOHFqdjJFZThRS0JnUUM3MkFFa0dYYkIvYy9rRkdVTmN0bStjelYwNEhmYksvRkJcblROV2docTlmaHVkYk1OdVZWVlU0T1kvZm9VdG8vQVJ0dzc3alpOT2lYRUFRZDlWdnN2ZHpqYjZMK21xMGt1bm9cblBFdlJhS1ZGRU1tdWVrOGljWFdvSEU4ak9VRmxjUm40LzRCWFNqRzFEb3hCaUJsZHJhR1VCamZQWE92Y0NjUXdcbmZJR0tmZlVkb3dLQmdRQzNoRUtZNkRUUXU4REJPWWRKM0FwWnFQUXhqNzZGUkhnK1VZejA0dlQ5MU4wZks2dmdcbkNmWU9EelFFYzA2Y2xYdTJhT2xhbzZvVjFKeTNleWYwT2tnd1ZrYjlCeXVzWHVQS3ZUcCtTbDdVd2dvRE9DZjlcbkFuVjNQcHptaVQ1WEhnbytIa0VkZ2hSS3ZhbTBiMkRTYTVJMEMzOUdJME5uR3EwbWZvZGJBZTJ3a1FLQmdRQ0VcbjNlRFF2QUk0YldOakhObnRqVk5mVnZaMDJXRnQ4Y01RUVZ6SjB2cXhReWJhWmRDcjdGUC9GUmdqUWwrb3Zyb1Rcbk9lWUh5Z1c0VGpBeWRkRE8xWFFhbENRM3RzVkxQSytleVNlSDA5RXk3UkQ3RFNCMGFIU2ttSUdSbEtvaEtzTUlcblVSRmlyT1JFQVNwTUlBeHhLcU9qcEZKTnBwaGVaN29SNGI5dlNuRGFxUUtCZ0JRNGYyMVkrQklqVjZuSkhpaC9cbll0QUhqUVpNdyt1NjU5TXF0WTBiTzcxMGVaMzhMc3NoZ1M4SGllMTlBRnhVWkNpY2JrQ2FvWmlPcmF5R0VCMkhcbi9TM0lmZjR1MEZVSzJocXFtcEF2K1RvSWVOSURwbTVDUlg2Q2U1T2dLNVRMTWY5ZC9tYm1wR24yYm1xR1Uwamhcblc0eEVQeHFrQjk3bEhVbUFCbXM0T0NIVVxuLS0tLS1FTkQgUFJJVkFURSBLRVktLS0tLVxuIiwKICAiY2xpZW50X2VtYWlsIjogImZpcmViYXNlLWFkbWluc2RrLWhteGE5QGRvbW90aWN6LTExOTEuaWFtLmdzZXJ2aWNlYWNjb3VudC5jb20iLAogICJjbGllbnRfaWQiOiAiMTAzNzY5NTQ0NzM3NzU5Mjg5MDI1IiwKICAiYXV0aF91cmkiOiAiaHR0cHM6Ly9hY2NvdW50cy5nb29nbGUuY29tL28vb2F1dGgyL2F1dGgiLAogICJ0b2tlbl91cmkiOiAiaHR0cHM6Ly9vYXV0aDIuZ29vZ2xlYXBpcy5jb20vdG9rZW4iLAogICJhdXRoX3Byb3ZpZGVyX3g1MDlfY2VydF91cmwiOiAiaHR0cHM6Ly93d3cuZ29vZ2xlYXBpcy5jb20vb2F1dGgyL3YxL2NlcnRzIiwKICAiY2xpZW50X3g1MDlfY2VydF91cmwiOiAiaHR0cHM6Ly93d3cuZ29vZ2xlYXBpcy5jb20vcm9ib3QvdjEvbWV0YWRhdGEveDUwOS9maXJlYmFzZS1hZG1pbnNkay1obXhhOSU0MGRvbW90aWN6LTExOTEuaWFtLmdzZXJ2aWNlYWNjb3VudC5jb20iLAogICJ1bml2ZXJzZV9kb21haW4iOiAiZ29vZ2xlYXBpcy5jb20iCn0=)pKey";

	Json::Value root;

	if (ParseJSon(base64_decode(FCMKeyB64), root))
	{
		std::string sGAPI_FCM_ProjectID;

		m_GAPI_FCM_issuer = root["client_email"].asString();
		m_GAPI_FCM_privkey = root["private_key"].asString();
		sGAPI_FCM_ProjectID = root["project_id"].asString();

		m_GAPI_FCM_PostURL = GAPI_FCM_POST_URL_BASE;
		stdreplace(m_GAPI_FCM_PostURL, "##PROJECTID##", sGAPI_FCM_ProjectID);

		return true;
	}

	return false;
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
	std::vector<std::string> vExtraData;
	if (ExtraData.find("midx_") != std::string::npos) {
		sMidx = ExtraData.substr(5);
		stdreplace(sMidx, ";", ",");
	}
	else if (ExtraData.find("|") != std::string::npos) {
		std::string temp;
		std::stringstream tempssstr { ExtraData };

		while (std::getline(tempssstr, temp, '|')) {
			vExtraData.push_back(temp);
		}
	}

	//Get All Devices
	std::vector<std::vector<std::string>> mobileDevices;
	std::string szQuery("SELECT ID,Active,Name,DeviceType,SenderID FROM MobileDevices");
	if (!sMidx.empty()) {
		szQuery += " WHERE (ID IN (" + sMidx + "))";
	}
	else {
		szQuery += " WHERE (Active == 1)";
	}

	mobileDevices = m_sql.safe_query(szQuery.c_str());
	if (mobileDevices.empty())
		return true;

	// Get an access token to send the message
	// First create a JWT with the FCM issuer and correct scope
	std::string sFCMjwt, slAccessToken;
	if (!createFCMjwt(m_GAPI_FCM_issuer, sFCMjwt))
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
	uint8_t iSend = 0;

	sstr2 << "Authorization: Bearer " << slAccessToken;
	ExtraHeaders.push_back(sstr2.str());
	ExtraHeaders.push_back("Content-Type: application/json");

	// Send the message to all devices
	for (auto &mobileDevice : mobileDevices)
	{
		if (mobileDevice[4].empty())
		{
			_log.Debug(DEBUG_EVENTSYSTEM, "FCM: No SenderID for device %s", mobileDevice[2].c_str());
			continue;
		}

		// Build the message
		std::stringstream sstr;

		sstr << R"({ "validate_only": false, "message": {)";	// Open Send Message struct

		if (!vExtraData.empty())
		{
			uint8_t iKVs = 0;
			sstr << R"("data": { )";
			for (std::string &extraDataKV : vExtraData)
			{
				if (extraDataKV.find("=") == std::string::npos)
					continue;
				if (iKVs > 0)
					sstr << R"(, )";
				std::vector<std::string> aKV;
				StringSplit(extraDataKV, "=", aKV);
				sstr << R"(")" << aKV[0] << R"(": ")" << aKV[1] << R"(")";
				iKVs++;
			}
			sstr << R"(}, )";
		}

		if (bFromNotification)
		{
			sstr << R"("notification": { "title": ")" << Subject << R"(", "body": ")" << Text << R"("}, )";
		}

		sstr << R"("token": ")" << mobileDevice[4] << R"(")";		// Add where to send
		sstr << R"(} })";											// Close Send Message struct
		std::string szPostdata = sstr.str();
		
		_log.Debug(DEBUG_EVENTSYSTEM, "FCM: Generated message for device (%s): .%s.", mobileDevice[2].c_str(), szPostdata.c_str());

		std::string sResult;
		if (HTTPClient::POST(m_GAPI_FCM_PostURL, szPostdata, ExtraHeaders, sResult))
		{
			Json::Value root;
			bool ret = ParseJSon(sResult, root);
			if (ret)
			{
				if (!root["error"].empty())
				{
					Json::Value jsonError = root["error"];
					_log.Log(LOG_ERROR, "FCM: Could not send message! Errorcode %d (%s)", jsonError["code"].asInt(), jsonError["message"].asCString());
				}
				else
				{
					iSend++;
					_log.Debug(DEBUG_EVENTSYSTEM, "FCM: Message sent to device (%s): .%s.", mobileDevice[2].c_str(), sResult.c_str());
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "FCM: Can not connect to FCM API URL");
				_log.Debug(DEBUG_EVENTSYSTEM, "FCM: API call failed, JSON Error! (%s)", sResult.c_str());
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "FCM: Could not send message, HTTP Error");
		}
	}

	return (iSend == mobileDevices.size());
}

bool CNotificationFCM::getSlAccessToken(const std::string &bearer_token, std::string &slAccessToken)
{
	if (!m_slAccesToken_cached.empty())
	{
		uint64_t cur_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now()).time_since_epoch()).count();
		if (cur_time < m_slAccessToken_exp_time)
		{
			_log.Debug(DEBUG_EVENTSYSTEM, "FCM: Using Cached Token! (Expires at %ld)", m_slAccessToken_exp_time);
			slAccessToken = m_slAccesToken_cached;
			return true;
		}
	}

	std::vector<std::string> ExtraHeaders;
	std::string sPostBody, sResult;

	ExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded");

	sPostBody = "grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Ajwt-bearer&assertion=" + bearer_token;

	_log.Debug(DEBUG_EVENTSYSTEM, "FCM: Requesting AccessToken (%s)", sPostBody.c_str());

	if (HTTPClient::POST(GAPI_OAUTH2_TOKEN_URL, sPostBody, ExtraHeaders, sResult))
	{
		Json::Value root;

		if (ParseJSon(sResult, root))
		{
			if (!root["access_token"].empty())
			{
				slAccessToken = root["access_token"].asString();
				uint64_t slAccessToken_exp_seconds = 0;
				if (!root["expires_in"].empty())
				{
					slAccessToken_exp_seconds = (root["expires_in"].asInt() - 120);		// 2 minutes before expiration
					m_slAccessToken_exp_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch()).count();
					m_slAccessToken_exp_time = m_slAccessToken_exp_time + slAccessToken_exp_seconds;
					m_slAccesToken_cached = slAccessToken;
				}
				_log.Debug(DEBUG_EVENTSYSTEM, "FCM: AccessToken retrieved (%s...) expires in %ld seconds (at %ld)", slAccessToken.substr(0,10).c_str(), slAccessToken_exp_seconds, m_slAccessToken_exp_time);
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
	sFCMjwt.clear();

	try
	{
	auto JWT = jwt::create()
		.set_type("JWT")
		.set_issuer(FCMissuer)
		.set_audience(GAPI_OAUTH2_TOKEN_URL)
		.set_issued_at(std::chrono::system_clock::now())
		.set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds{600})
		.set_payload_claim("scope", jwt::claim(std::string{GAPI_FCM_SCOPE}));
		sFCMjwt = JWT.sign(jwt::algorithm::rs256{"", m_GAPI_FCM_privkey, "", ""}, &base64url_encode);
	}
	catch(const std::exception& err)
	{
		_log.Debug(DEBUG_EVENTSYSTEM,"FCM: Exception creating FCM jwt (%s)", err.what());
	}

	return !sFCMjwt.empty();
}