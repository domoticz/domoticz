#include "stdafx.h"
#include "NotificationFCM.h"
#include "../httpclient/HTTPClient.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/json_helper.h"

#define GAPI_POST_URL "https://fcm.googleapis.com/fcm/send"
#define GAPI "AIzaSyBnRMroiDaXCKbwPeOmoxkNiQfjWkGMre8"

constexpr const char *FCMMessage = R"FCM(
{
  "to": "<to>",
  "message": {
    "notification": {
      "title": "<title>",
      "body": "<body>"
    },
    "data": {
      "param1": "<extradata>"
    },
    "apns": {
      "headers": {
        "apns-priority": "5"
      },
      "payload": {
        "aps": {
          "sound": "default"
        }
      }
    },
    "android": {
      "priority": "medium",
      "notification": {
        "sound": "default"
      }
    }
  }
})FCM";

CNotificationFCM::CNotificationFCM() : CNotificationBase(std::string("fcm"), OPTIONS_NONE)
{
	SetupConfig(std::string("FCMEnabled"), &m_IsEnabled);
}

bool CNotificationFCM::IsConfigured()
{
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

	//std::string szPostData(FCMMessage);
	//stdreplace(szPostData, "<to>", sTo);
	//stdreplace(szPostData, "<title>", Subject);
	//stdreplace(szPostData, "<body>", Text);
	//stdreplace(szPostData, "<extradata>", ExtraData);

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

	std::string sTo;
	for (const auto &r : result)
	{
		if (!sTo.empty())
			sTo += ", ";
		sTo += r[0];
	}

	//We need to distinguish between Android and iOS devices for the following JSon notification call

	std::vector<std::string> androidDevices;
	std::vector<std::string> iOSDevices;

	for (const auto &sd : result)
	{
		std::string sSenderID = sd[0];
		std::string sDeviceType = sd[1];

		if ((sDeviceType.empty()) || (sDeviceType.find("Android") == 0))
			androidDevices.push_back(sSenderID);
		else
			iOSDevices.push_back(sSenderID);
	}

	//Android Devices
	if (!androidDevices.empty())
	{
		std::stringstream sstr, sstr2;
		std::string sResult;
		sstr << "{ \"registration_ids\": [";

		int ii = 0;

		for (const auto &device : androidDevices)
		{
			if (ii != 0)
				sstr << ", ";
			sstr << "\"" << device << "\"";
			ii++;
		}

		sstr << R"(], "data" : { "subject": ")" << Subject << R"(", "body": ")" << Text << R"(", "extradata": ")" << ExtraData << R"(", "priority": ")" << std::to_string(Priority) << "\", ";
		sstr << R"("deviceid": ")" << std::to_string(Idx) << R"(", "message": ")" << Subject << "\" } }";
		std::string szPostdata = sstr.str();

		std::vector<std::string> ExtraHeaders;
		ExtraHeaders.push_back("Content-Type: application/json");

		sstr2 << "Authorization: key=" << GAPI;
		ExtraHeaders.push_back(sstr2.str());

		if (!HTTPClient::POST(GAPI_POST_URL, szPostdata, ExtraHeaders, sResult))
		{
			_log.Log(LOG_ERROR, "FCM: Could not send message, HTTP Error");
			return false;
		}

		Json::Value root;

		bool ret = ParseJSon(sResult, root);
		if (!ret)
		{
			_log.Log(LOG_ERROR, "FCM: Can not connect to FCM API URL");
			return false;
		}
	}

	//iOS Devices
	if (!iOSDevices.empty())
	{
		std::stringstream sstr, sstr2;
		std::string sResult;
		sstr << "{ \"registration_ids\": [";

		int ii = 0;

		for (const auto &device : iOSDevices)
		{
			if (ii != 0)
				sstr << ", ";
			sstr << "\"" << device << "\"";
			ii++;
		}

		sstr << R"(], "notification" : { "subject": ")" << Subject << R"(", "body": ")" << Text << R"(", "extradata": ")" << ExtraData << R"(", "priority": ")" << std::to_string(Priority)
		     << R"(", "sound": "default", )";
		sstr << R"("deviceid": ")" << std::to_string(Idx) << R"(", "message": ")" << Subject << R"(", "content_available": true } })";
		std::string szPostdata = sstr.str();

		std::vector<std::string> ExtraHeaders;
		ExtraHeaders.push_back("Content-Type: application/json");

		sstr2 << "Authorization: key=" << GAPI;
		ExtraHeaders.push_back(sstr2.str());

		if (!HTTPClient::POST(GAPI_POST_URL, szPostdata, ExtraHeaders, sResult))
		{
			_log.Log(LOG_ERROR, "FCM: Could not send message, HTTP Error");
			return false;
		}

		Json::Value root;

		bool ret = ParseJSon(sResult, root);
		if (!ret)
		{
			_log.Log(LOG_ERROR, "FCM: Can not connect to FCM API URL");
			return false;
		}

		if (!root["failure"].empty())
		{
			int iFailure = root["failure"].asInt();
			return (iFailure == 0);
		}


	}
	return true;
}

