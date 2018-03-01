#include "stdafx.h"
#include "NotificationGCM.h"
#include "../httpclient/HTTPClient.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../json/json.h"
#include <boost/lexical_cast.hpp>

#define GAPI_POST_URL "https://gcm-http.googleapis.com/gcm/send"
#define GAPI "AIzaSyBnRMroiDaXCKbwPeOmoxkNiQfjWkGMre8"

CNotificationGCM::CNotificationGCM() : CNotificationBase(std::string("gcm"), OPTIONS_NONE)
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
	//ExtraData should be empty, is only filled currently when hitting the test button from the mobile devices setup page in the web gui

	//Get All Devices
	std::vector<std::vector<std::string> > result;

	std::string sMidx;
	std::vector<std::string> vDevices;
	//std::string sDevice = "";
	if (ExtraData.find("midx_") != std::string::npos)
		sMidx = ExtraData.substr(5);
	if (ExtraData.find("|Device=") != std::string::npos) {
		std::string sDevice = ExtraData.substr(8);
		if (!sDevice.empty())
			boost::split(vDevices, sDevice, boost::is_any_of(";"));
	}

	std::string szQuery("SELECT SenderID, DeviceType FROM MobileDevices");
	if (!sMidx.empty())
		szQuery += " WHERE (ID == " + sMidx + ")";
	else if (!vDevices.empty())
		szQuery += " WHERE (ID IN (" + boost::algorithm::join(vDevices, ",") + "))";
	else
		szQuery += " WHERE (Active == 1)";
	result = m_sql.safe_query(szQuery.c_str());
	if (result.empty())
		return true;

	//We need to distinguish between Android and iOS devices for the following JSon notification call

	std::vector<std::string> androidDevices;
	std::vector<std::string> iOSDevices;

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt = result.begin(); itt != result.end(); ++itt)
	{
		std::vector<std::string> sd = *itt;
		std::string sSenderID = sd[0];
		std::string sDeviceType = sd[1];

		if ((sDeviceType.empty()) || (sDeviceType.find("Android") == 0))
			androidDevices.push_back(sSenderID);
		else
			iOSDevices.push_back(sSenderID);
	}

	std::vector<std::string>::const_iterator ittDevice;
	
	//Android Devices
	if (!androidDevices.empty())
	{
		std::stringstream sstr, sstr2;
		std::string sResult;
		sstr << "{ \"registration_ids\": [";

		int ii = 0;

		for (ittDevice = androidDevices.begin(); ittDevice != androidDevices.end(); ++ittDevice)
		{
			if (ii != 0)
				sstr << ", ";
			sstr << "\"" << *ittDevice << "\"";
			ii++;
		}

		sstr << "], \"data\" : { \"subject\": \"" << Subject << "\", \"body\": \"" << Text << "\", \"extradata\": \"" << ExtraData << "\", \"priority\": \"" << boost::lexical_cast<std::string>(Priority) << "\", ";
		sstr << "\"deviceid\": \"" << boost::lexical_cast<std::string>(Idx) << "\", \"message\": \"" << Subject << "\" } }";
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
	}

	//iOS Devices
	if (!iOSDevices.empty())
	{
		std::stringstream sstr, sstr2;
		std::string sResult;
		sstr << "{ \"registration_ids\": [";

		int ii = 0;

		for (ittDevice = iOSDevices.begin(); ittDevice != iOSDevices.end(); ++ittDevice)
		{
			if (ii != 0)
				sstr << ", ";
			sstr << "\"" << *ittDevice << "\"";
			ii++;
		}

		sstr << "], \"notification\" : { \"subject\": \"" << Subject << "\", \"body\": \"" << Text << "\", \"extradata\": \"" << ExtraData << "\", \"priority\": \"" << boost::lexical_cast<std::string>(Priority) << "\", ";
		sstr << "\"deviceid\": \"" << boost::lexical_cast<std::string>(Idx) << "\", \"message\": \"" << Subject << "\", \"content_available\": true } }";
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
	}
	return true;
}

bool CNotificationGCM::IsConfigured()
{
	return true;
}
