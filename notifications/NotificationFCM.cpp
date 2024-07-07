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

constexpr const char* FCMKeyB64 = R"pKey(ewogICJ0eXBlIjogInNlcnZpY2VfYWNjb3VudCIsCiAgInByb2plY3RfaWQiOiAiZG9tb3RpY3otMTE5MSIsCiAgInByaXZhdGVfa2V5X2lkIjogImYzMDgwYzQyZWMyMzQ0YTg2OWM5OWQ2OGQ1NGE5YmM2YWZkZmNjMjQiLAogICJwcml2YXRlX2tleSI6ICItLS0tLUJFR0lOIFBSSVZBVEUgS0VZLS0tLS1cbk1JSUV2Z0lCQURBTkJna3Foa2lHOXcwQkFRRUZBQVNDQktnd2dnU2tBZ0VBQW9JQkFRQ1MvVHRGY3hLNHVvdzNcbjM3VDcvYnBhWSsyRDhLVTdiS2NENGhEbDRmeDVkRVhCZ0NIK3M5YzBsaWx5N1kyakV1V21aTEZ1bCtGNEpQY0hcbjdHYm45YXp2U0taV0RtVWo0d2lBNzliY3NsMW0zOGkxcVB0TlFaNWQwajZzR1UxUVhXYlk2S0lJQXpQRWZORFJcbkN3VlQzdFJyRWJwVW1IckY5bk9hMzdrdDFtbUM4aHdQZmd0NkEyaHRONklSQ2FmUUZhNjhURXBlTVVYTk1WRndcbnhIWHRWMjM5SHYwbDIxR3NjYnZhZmV1RnNSbGxYY0dHZTJKRG5EMGtaVkpIMGhsb245eHFMZG9QV3I2VFlTU3Vcbm9pRmRNdEx5b3ZPb3ZrM2VLR3pLam1qbk5SL1JzK0M1b2xuYnZxR3JnUVNCOGpJQTFrci83bkRQS0xUOFB3SlZcblJla2svUUJ6QWdNQkFBRUNnZ0VBTjFEOGR0RHRDVDhQR05Ia0F2ZEVJOU02WmU1d3lGSEY4c1NuVmxQN0Yxd0dcbkNtR2xxWHhWcjNFWmJTcHdKS0F1YzdXdVBuQURCcHRtcWdFeDR2YUdXOUNyNUhQclpSdUNMai9VVE44RkhudDJcblJjTXZ1NjNIb3doRjFLOTY5SlVKNlBSM1VldWVsa0hndGVMZllPWlphMmZUQnhZUkVpSUM5SlJnVkpzOXZ1eTdcblJSWGtROC9aVDNaYTBpdjBNdzBYaFBnNmpTNkhWcm1qS2hVTWhNa1p5RERUN1RwNjl0amtjS3ZTSHRXMSt5VUFcbmE4eEJ5NVVBKzV3R3ExSFM0bkFUQU1MbTRmb2RleHZsV1VTVUNRaHhaYmhRc29ZVkVoNlhKdlUzYnVRalQ5SytcbkIrU3FDVVBoYktTTFJqR04zcE1PUWUwN2tQaGhqZzBOdzY1ZHcrTHpFUUtCZ1FESVVtdnNsZTBIbUs4QzhHUmVcbmxsOFZaY1FJNXhwOHhBc1V1ZFdpdytxOExIRS95NmNBcysyaG1CMWlva0VYbDdCNEtNTUhWWCthZXNrVHdRdWlcbkw5RXRNZXhPYmthZDNtMUJwOUhDb1JHMFNSUHlsaVU2a1FhUGxTNTM4OWZWN08zYm1OUFlxcGo3eUdhVXlmcEFcblhHU0tmNkh0cXVRRWd2UXVZOHFqdjJFZThRS0JnUUM3MkFFa0dYYkIvYy9rRkdVTmN0bStjelYwNEhmYksvRkJcblROV2docTlmaHVkYk1OdVZWVlU0T1kvZm9VdG8vQVJ0dzc3alpOT2lYRUFRZDlWdnN2ZHpqYjZMK21xMGt1bm9cblBFdlJhS1ZGRU1tdWVrOGljWFdvSEU4ak9VRmxjUm40LzRCWFNqRzFEb3hCaUJsZHJhR1VCamZQWE92Y0NjUXdcbmZJR0tmZlVkb3dLQmdRQzNoRUtZNkRUUXU4REJPWWRKM0FwWnFQUXhqNzZGUkhnK1VZejA0dlQ5MU4wZks2dmdcbkNmWU9EelFFYzA2Y2xYdTJhT2xhbzZvVjFKeTNleWYwT2tnd1ZrYjlCeXVzWHVQS3ZUcCtTbDdVd2dvRE9DZjlcbkFuVjNQcHptaVQ1WEhnbytIa0VkZ2hSS3ZhbTBiMkRTYTVJMEMzOUdJME5uR3EwbWZvZGJBZTJ3a1FLQmdRQ0VcbjNlRFF2QUk0YldOakhObnRqVk5mVnZaMDJXRnQ4Y01RUVZ6SjB2cXhReWJhWmRDcjdGUC9GUmdqUWwrb3Zyb1Rcbk9lWUh5Z1c0VGpBeWRkRE8xWFFhbENRM3RzVkxQSytleVNlSDA5RXk3UkQ3RFNCMGFIU2ttSUdSbEtvaEtzTUlcblVSRmlyT1JFQVNwTUlBeHhLcU9qcEZKTnBwaGVaN29SNGI5dlNuRGFxUUtCZ0JRNGYyMVkrQklqVjZuSkhpaC9cbll0QUhqUVpNdyt1NjU5TXF0WTBiTzcxMGVaMzhMc3NoZ1M4SGllMTlBRnhVWkNpY2JrQ2FvWmlPcmF5R0VCMkhcbi9TM0lmZjR1MEZVSzJocXFtcEF2K1RvSWVOSURwbTVDUlg2Q2U1T2dLNVRMTWY5ZC9tYm1wR24yYm1xR1Uwamhcblc0eEVQeHFrQjk3bEhVbUFCbXM0T0NIVVxuLS0tLS1FTkQgUFJJVkFURSBLRVktLS0tLVxuIiwKICAiY2xpZW50X2VtYWlsIjogImZpcmViYXNlLWFkbWluc2RrLWhteGE5QGRvbW90aWN6LTExOTEuaWFtLmdzZXJ2aWNlYWNjb3VudC5jb20iLAogICJjbGllbnRfaWQiOiAiMTAzNzY5NTQ0NzM3NzU5Mjg5MDI1IiwKICAiYXV0aF91cmkiOiAiaHR0cHM6Ly9hY2NvdW50cy5nb29nbGUuY29tL28vb2F1dGgyL2F1dGgiLAogICJ0b2tlbl91cmkiOiAiaHR0cHM6Ly9vYXV0aDIuZ29vZ2xlYXBpcy5jb20vdG9rZW4iLAogICJhdXRoX3Byb3ZpZGVyX3g1MDlfY2VydF91cmwiOiAiaHR0cHM6Ly93d3cuZ29vZ2xlYXBpcy5jb20vb2F1dGgyL3YxL2NlcnRzIiwKICAiY2xpZW50X3g1MDlfY2VydF91cmwiOiAiaHR0cHM6Ly93d3cuZ29vZ2xlYXBpcy5jb20vcm9ib3QvdjEvbWV0YWRhdGEveDUwOS9maXJlYmFzZS1hZG1pbnNkay1obXhhOSU0MGRvbW90aWN6LTExOTEuaWFtLmdzZXJ2aWNlYWNjb3VudC5jb20iLAogICJ1bml2ZXJzZV9kb21haW4iOiAiZ29vZ2xlYXBpcy5jb20iCn0=)pKey";

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

