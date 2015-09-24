#include "stdafx.h"
#include "../main/Logger.h"
#include "NotificationLogitechMediaServer.h"
#include "../main/Helper.h"
#include "../main/RFXNames.h"

CNotificationLogitechMediaServer::CNotificationLogitechMediaServer() : CNotificationBase(std::string("lms"), OPTIONS_NONE)
{
	SetupConfig(std::string("LmsEnabled"), &m_IsEnabled);
	SetupConfig(std::string("LmsServerIP"), _ServerIP);
	SetupConfig(std::string("LmsServerPort"), &_ServerPort);
	SetupConfig(std::string("LmsPlayerIP"), _PlayerIP);
	SetupConfig(std::string("LmsDuration"), &_Duration);
}

CNotificationLogitechMediaServer::~CNotificationLogitechMediaServer()
{
}

bool CNotificationLogitechMediaServer::SendMessageImplementation(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string &Sound, const bool bFromNotification)
{
	std::string	sSubject("Domoticz");
	if (Subject != Text)
	{
		sSubject = Subject;
	}
	//????????????

	// Loop through semi-colon separated IP Addresses
	std::vector<std::string> results;
	StringSplit(_PlayerIP, ";", results);
	for (int i = 0; i < (int)results.size(); i++)
	{
		std::stringstream logline;
		logline << "Lms Notification (" << results[i] << "): " << sSubject << ", " << Text;
		_log.Log(LOG_NORM, "%s", logline.str().c_str());

		std::stringstream postdata;
		postdata << sSubject << ": " << Text << "(" << results[i] << ")";

		Json::Value root = Query(_ServerIP, _ServerPort, postdata.str().c_str());

	}
	return true;
}

bool CNotificationLogitechMediaServer::IsConfigured()
{
	return (!_PlayerIP.empty());
}

Json::Value CNotificationLogitechMediaServer::Query(std::string sIP, int iPort, std::string sPostdata)
{
	Json::Value root;
	std::vector<std::string> ExtraHeaders;
	std::string sResult;
	std::stringstream sURL;
	std::stringstream sPostData;

	sURL << "http://" << sIP << ":" << iPort << "/jsonrpc.js";
//	sPostData << sPostdata;
//	HTTPClient::SetTimeout(m_iPingTimeoutms / 1000);
//	bool bRetVal = HTTPClient::POST(sURL.str(), sPostData.str(), ExtraHeaders, sResult);
//
//	if (!bRetVal)
//	{
//		return root;
//	}
//	Json::Reader jReader;
//	bRetVal = jReader.parse(sResult, root);
//	if (!bRetVal)
//	{
//		_log.Log(LOG_ERROR, "Logitech Media Server: PARSE ERROR: %s", sResult.c_str());
//		return root;
//	}
//	if (root["method"].empty())
//	{
//		_log.Log(LOG_ERROR, "Logitech Media Server: '%s' request '%s'", sURL.str().c_str(), sPostData.str().c_str());
//		return root;
//	}
	return root["result"];
}
