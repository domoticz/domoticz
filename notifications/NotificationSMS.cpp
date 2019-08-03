#include "stdafx.h"
#include "NotificationSMS.h"
#include "../httpclient/HTTPClient.h"
#include "../httpclient/UrlEncode.h"
#include "../main/Logger.h"
#include "../main/Helper.h"

CNotificationSMS::CNotificationSMS() : CNotificationBase(std::string("clickatell"), OPTIONS_URL_SUBJECT | OPTIONS_URL_BODY | OPTIONS_URL_PARAMS)
{
	SetupConfig(std::string("ClickatellEnabled"), &m_IsEnabled);
	SetupConfigBase64(std::string("ClickatellAPI"), _clickatellApi);
	SetupConfigBase64(std::string("ClickatellUser"), _clickatellUser);
	SetupConfigBase64(std::string("ClickatellPassword"), _clickatellPassword);
	SetupConfigBase64(std::string("ClickatellTo"), _clickatellTo);
	SetupConfigBase64(std::string("ClickatellFrom"), _clickatellFrom);

	// standard defaults, see Clickatell developer docs:
	_maxCredits = 10;
	_escalate = 0; // valid values are 0 (OFF), 1 (60) or 2 (30).
	_features = 0x0020 | 0x4000; // (Numeric sender id plus concatenation)
	_concat = 3;
}

CNotificationSMS::~CNotificationSMS()
{
}

bool CNotificationSMS::SendMessageImplementation(
	const uint64_t Idx,
	const std::string &Name,
	const std::string &Subject,
	const std::string &Text,
	const std::string &ExtraData,
	const int Priority,
	const std::string &Sound,
	const bool bFromNotification)
{
	//send message by Clickatell SMS
	bool bRet = false;

	std::string MsgFrom = _clickatellFrom;
	stdreplace(MsgFrom, "+", "");
	stdreplace(MsgFrom, " ", "");
	std::string rfrom = CURLEncode::URLDecode(MsgFrom);
	MsgFrom = stdstring_trim(rfrom);
	if (MsgFrom.empty())
		return false;

	std::string thisTo = CURLEncode::URLDecode(_clickatellTo);

	stdreplace(thisTo, "+", "");
	stdreplace(thisTo, " ", "");

	// One can send to multiple destination addresses by delimiting the addresses with commas
	stdreplace(thisTo, ";", ",");

	thisTo = stdstring_trim(thisTo);
	if (thisTo.empty())
		return false;

	std::string sResult;	
	std::stringstream sPostData;
    
	sPostData 
		<< "api_id=" << _clickatellApi 
		<< "&user=" << _clickatellUser 
		<< "&password=" << _clickatellPassword 
		<< "&from=" << MsgFrom 
		<< "&max_credits=" << _maxCredits 
		<< "&escalate=" << _escalate 
		<< "&req_feat=" << _features 
		<< "&concat=" << _concat 
		<< "&to=" << thisTo 
		<< "&text=" << Text;

	std::vector<std::string> ExtraHeaders;
	bRet |= HTTPClient::POST("https://platform.clickatell.com/messages/http/send", sPostData.str(), ExtraHeaders, sResult);
	if (sResult.find("ERR:") != std::string::npos)
	{
		//We have an error
		bRet = false;
		_log.Log(LOG_ERROR, "Clickatell SMS Gateway: %s", sResult.c_str());
	}
	return bRet;
}

bool CNotificationSMS::IsConfigured()
{
	return (_clickatellApi != "" && _clickatellUser != "" && _clickatellPassword != "" && _clickatellTo != "" && _clickatellFrom != "");
}
