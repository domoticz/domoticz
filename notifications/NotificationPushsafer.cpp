#include "stdafx.h"
#include "NotificationPushsafer.h"
#include "../httpclient/HTTPClient.h"
#include "../main/Logger.h"
#include "../httpclient/UrlEncode.h"
#include "../httpclient/HTTPClient.h"
#include "../webserver/Base64.h"
#include "../main/Helper.h"

CNotificationPushsafer::CNotificationPushsafer() : CNotificationBase(std::string("pushsafer"), OPTIONS_URL_SUBJECT | OPTIONS_URL_BODY | OPTIONS_URL_PARAMS)
{
	SetupConfig(std::string("PushsaferEnabled"), &m_IsEnabled);
	SetupConfig(std::string("PushsaferAPI"), _apikey);
	SetupConfig(std::string("PushsaferImage"), _apiuser);
}

CNotificationPushsafer::~CNotificationPushsafer()
{
}

bool CNotificationPushsafer::SendMessageImplementation(
	const uint64_t Idx,
	const std::string &Name,
	const std::string &Subject,
	const std::string &Text,
	const std::string &ExtraData,
	const int Priority,
	const std::string &Sound,
	const bool bFromNotification)
	
{
	//send message to Pushsafer
	std::string cSubject = (Subject == Text) ? "Domoticz" : Subject;

	bool bRet;
	std::string sResult;
	std::stringstream sPostData;
	std::vector<std::string> ExtraHeaders;

	std::vector<unsigned char> camimage;
	std::vector<std::string> ExtraHeadersBinary;

	std::vector<std::string> splitresults;
	StringSplit(CURLEncode::URLDecode(Text), "|", splitresults);
	if (splitresults.size() == 11)
	{
		// [0] private or alias key | [1] deviceid | [2] icon | [3] sound | [4] vibration | [5] title | [6] message | [7] pictureurl | [8] url | [9] urltitle | [10] time2live
		std::string PS_k = splitresults[0];
		std::string PS_d = splitresults[1];
		std::string PS_i = splitresults[2];
		std::string PS_s = splitresults[3];
		std::string PS_v = splitresults[4];
		std::string PS_t = splitresults[5];
		std::string PS_m = splitresults[6];
		std::string PS_p = splitresults[7];
		std::string PS_u = splitresults[8];
		std::string PS_ut = splitresults[9];
		std::string PS_l = splitresults[10];

		if (PS_k.length() >= 15)
		{
			_apikey = PS_k;
		}

		if (PS_m.empty())
		{
			PS_m = Text;
		}
		
		if (PS_t.empty())
		{
			PS_t = cSubject;
		}

		if (!PS_p.empty())
		{
			if (HTTPClient::GETBinary(CURLEncode::URLDecode(PS_p), ExtraHeadersBinary, camimage, 10))
			{
				std::string base64ImageString(camimage.begin(), camimage.end());
				base64ImageString = base64_encode(base64ImageString);
				sPostData << "&k=" << _apikey << "&d=" << PS_d << "&i=" << PS_i << "&s=" << PS_s << "&v=" << PS_v << "&t=" << PS_t << "&m=" << PS_m << "&p=data:image/jpeg;base64," << base64ImageString << "&u=" << PS_u << "&ut=" << PS_ut << "&l=" << PS_l << "&pr=" << Priority;
			}
			else
			{
				_log.Log(LOG_ERROR, "Pushsafer: can't download image > wrong url");
				sPostData << "&k=" << _apikey << "&d=" << PS_d << "&i=" << PS_i << "&s=" << PS_s << "&v=" << PS_v << "&t=" << PS_t << "&m=" << PS_m << "&u=" << PS_u << "&ut=" << PS_ut << "&l=" << PS_l << "&pr=" << Priority;
			}
		}
		else
		{
			sPostData << "&k=" << _apikey << "&d=" << PS_d << "&i=" << PS_i << "&s=" << PS_s << "&v=" << PS_v << "&t=" << PS_t << "&m=" << PS_m << "&u=" << PS_u << "&ut=" << PS_ut << "&l=" << PS_l << "&pr=" << Priority;
		}
	}
	else
	{
		if (!_apiuser.empty())
		{
			if (HTTPClient::GETBinary(CURLEncode::URLDecode(_apiuser.c_str()), ExtraHeadersBinary, camimage, 10))
			{
				std::string base64ImageString(camimage.begin(), camimage.end());
				base64ImageString = base64_encode(base64ImageString);
				sPostData << "i=12&k=" << _apikey << "&t=" << cSubject << "&m=" << Text << "&p=data:image/jpeg;base64," << base64ImageString;
			}
			else
			{
				_log.Log(LOG_ERROR, "Pushsafer: can't download image > wrong url");
				sPostData << "i=12&k=" << _apikey << "&t=" << cSubject << "&m=" << Text;
			}
		} else {
			sPostData << "i=12&k=" << _apikey << "&t=" << cSubject << "&m=" << Text;
		}
	}

	//Add the required Content Type
	ExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded");
	
	bRet = HTTPClient::POST("https://www.pushsafer.com/api",sPostData.str(),ExtraHeaders,sResult);
	bool bSuccess = (sResult.find("\"success\":") != std::string::npos);
	if (!bSuccess)
		_log.Log(LOG_ERROR, "Pushsafer: %s", sResult.c_str());
	return (bRet && bSuccess);
}

bool CNotificationPushsafer::IsConfigured()
{
	return _apikey != "";
}
