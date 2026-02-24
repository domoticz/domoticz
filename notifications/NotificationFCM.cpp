#include "stdafx.h"
#include "NotificationFCM.h"
#include "../httpclient/HTTPClient.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/json_helper.h"

#define JWT_DISABLE_BASE64
#include <jwt-cpp/jwt.h>
#include <libwebem/Base64.h>

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
	SetupConfig(std::string("FCMClientEmail"), m_FCMClientEmail);
	SetupConfigBase64(std::string("FCMPrivateKey"), m_FCMPrivateKey);
	SetupConfig(std::string("FCMProjectId"), m_FCMProjectId);
}

bool CNotificationFCM::IsConfigured()
{
	// Check if user has provided FCM configuration fields
	if (m_FCMClientEmail.empty() || m_FCMPrivateKey.empty() || m_FCMProjectId.empty())
	{
		if (!m_FCMClientEmail.empty() || !m_FCMPrivateKey.empty() || !m_FCMProjectId.empty())
		{
			if (m_FCMClientEmail.empty())
				_log.Log(LOG_STATUS, "FCM: Client Email not configured. Please configure in Settings > Notifications.");
			if (m_FCMPrivateKey.empty())
				_log.Log(LOG_STATUS, "FCM: Private Key not configured. Please configure in Settings > Notifications.");
			if (m_FCMProjectId.empty())
				_log.Log(LOG_STATUS, "FCM: Project ID not configured. Please configure in Settings > Notifications.");
		}
		return false;
	}

	m_GAPI_FCM_issuer = m_FCMClientEmail;
	m_GAPI_FCM_privkey = m_FCMPrivateKey;

	if (m_GAPI_FCM_issuer.empty() || m_GAPI_FCM_privkey.empty() || m_FCMProjectId.empty())
	{
		_log.Log(LOG_ERROR, "FCM: Invalid configuration - missing required fields (client_email, private_key, or project_id)");
		return false;
	}

	m_GAPI_FCM_PostURL = GAPI_FCM_POST_URL_BASE;
	stdreplace(m_GAPI_FCM_PostURL, "##PROJECTID##", m_FCMProjectId);

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

	uint64_t AltIdx = Idx;
	std::string sMidx, sExtraData;
	std::vector<std::string> vExtraData;
	if (!ExtraData.empty()) {
		_log.Debug(DEBUG_EVENTSYSTEM, "FCM: ExtraData found! (%s) (deviceid %ld)", ExtraData.c_str(), static_cast<unsigned long>(Idx));
		if (ExtraData.find("midx_") != std::string::npos) {
			sMidx = ExtraData.substr(5);
			stdreplace(sMidx, ";", ",");
		}
		else if (ExtraData.find("|") != std::string::npos) {
			if (ExtraData.find("|Device=") == 0) {
				std::string sAltIdx = ExtraData.substr(8);
				if (Idx == 0 && isInt(sAltIdx)) {
					AltIdx = std::stoull(sAltIdx);
				}
			}
			else {
				std::string temp;
				std::stringstream tempssstr { ExtraData };

				while (std::getline(tempssstr, temp, '|')) {
					vExtraData.push_back(temp);
				}
			}
		}
		else {
			sExtraData = ExtraData;
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

	// Add the default 'data' fields we always want to send if available
	vExtraData.push_back("deviceid=" + std::to_string(AltIdx));
	vExtraData.push_back("priority=" + std::to_string(Priority));
	if (!Subject.empty()) {
		vExtraData.push_back("subject=" + Subject);
		vExtraData.push_back("message=" + Subject);		// To-Do: Depricated - This is not needed anymore for the updated mobile App. Will be removed soon
	}
	if (!Name.empty())
		vExtraData.push_back("message=" + Name);
	if (!Text.empty())
		vExtraData.push_back("body=" + Text);
	if (!Sound.empty())
		vExtraData.push_back("sound=" + Sound);
	if (!sExtraData.empty())
		vExtraData.push_back("extradata=" + sExtraData);

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
		Json::Value root;

		root["validate_only"] = false;

		for (std::string &extraDataKV : vExtraData)
		{
			if (extraDataKV.find("=") == std::string::npos)
				continue;
			std::vector<std::string> aKV;
			StringSplit(extraDataKV, "=", aKV);
			if (aKV.size() != 2)
				continue;	// Skip invalid key-value pairs
			root["message"]["data"][aKV[0]] = aKV[1];
		}

		/* For now, we do NOT use this as a Notification is handled by the device OS itself
		 * and the app itself is not aware of the notification
		if (bFromNotification)
		{
			root["message"]["notification"]["title"] = Subject;
			root["message"]["notification"]["body"] = Text;
		}
		*/

		root["message"]["token"] = mobileDevice[4];
		std::string szPostdata { root.toStyledString() };
		
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
					_log.Log(LOG_ERROR, "FCM: Could not send message for device (%s)! Errorcode %d (%s)", mobileDevice[2].c_str(), jsonError["code"].asInt(), jsonError["message"].asCString());
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
			_log.Debug(DEBUG_EVENTSYSTEM, "FCM: Using Cached Token! (Expires at %ld)", static_cast<unsigned long>(m_slAccessToken_exp_time));
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
				_log.Debug(DEBUG_EVENTSYSTEM, "FCM: AccessToken retrieved (%s...) expires in %ld seconds (at %ld)", slAccessToken.substr(0,10).c_str(), static_cast<unsigned long>(slAccessToken_exp_seconds), static_cast<unsigned long>(m_slAccessToken_exp_time));
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