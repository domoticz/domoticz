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
	SetupConfig(std::string("LmsPlayerMac"), _PlayerMac);
	SetupConfig(std::string("LmsDuration"), &_Duration);
}

CNotificationLogitechMediaServer::~CNotificationLogitechMediaServer()
{
}

bool CNotificationLogitechMediaServer::SendMessageImplementation(
	const uint64_t Idx,
	const std::string &Name,
	const std::string &Subject,
	const std::string &Text,
	const std::string &ExtraData,
	const int Priority,
	const std::string &Sound,
	const bool bFromNotification)
{
	std::string	sSubject("Domoticz");
	if (Subject != Text)
	{
		sSubject = Subject;
	}

	CDomoticzHardwareBase *pHardware = m_mainworker.GetHardwareByType(HTYPE_LogitechMediaServer);
	CLogitechMediaServer* pLMS = dynamic_cast<CLogitechMediaServer*>(pHardware);

	if (pHardware == NULL) {
		std::stringstream logline;
		logline << "Error sending notification: 'Logitech Media Server' not found in Hardware-list";
		_log.Log(LOG_ERROR, "%s", logline.str().c_str());
		return false;
	}

	// Loop through semi-colon separated IP Addresses
	std::vector<std::string> results;
	StringSplit(_PlayerMac, ";", results);

	for (int i = 0; i < (int)results.size(); i++)
	{
		std::string sPlayerId = results[i];
		pLMS->SendText(sPlayerId, sSubject, Text, _Duration);
	}
	return true;
}

bool CNotificationLogitechMediaServer::IsConfigured()
{
	return (!_PlayerMac.empty());
}

