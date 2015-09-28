#include "stdafx.h"
#include "../main/Logger.h"
#include "NotificationLogitechMediaServer.h"
#include "../hardware/LogitechMediaServer.h"
#include "../main/mainworker.h"
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

	//find hardware, if exist; otherwise disable notifications
	CDomoticzHardwareBase *pHardware = m_mainworker.GetHardwareByType(HTYPE_LogitechMediaServer);
	CLogitechMediaServer* pLMS = dynamic_cast<CLogitechMediaServer*>(pHardware);
	
	// Loop through semi-colon separated IP Addresses
	std::vector<std::string> results;
	StringSplit(_PlayerIP, ";", results);

	for (int i = 0; i < (int)results.size(); i++)
	{
		std::string sPlayerId = results[i];
		pLMS->SendText(sPlayerId, sSubject, Text, _Duration);
	}
	return true;
}

bool CNotificationLogitechMediaServer::IsConfigured()
{
	return (!_PlayerIP.empty());
}