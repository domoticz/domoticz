#include "stdafx.h"
#include "../main/Logger.h"
#include "NotificationKodi.h"
#include "../main/Helper.h"
#include "xmbcclient.h"
#include "RFXNames.h"

extern std::string szWWWFolder;

CNotificationKodi::CNotificationKodi() : CNotificationBase(std::string("kodi"), OPTIONS_NONE)
{
	SetupConfig(std::string("KodiEnabled"), &m_IsEnabled);
	SetupConfig(std::string("KodiIPAddress"), _IPAddress);
	SetupConfig(std::string("KodiPort"), &_Port);
	SetupConfig(std::string("KodiTimeToLive"), &_TTL);
}

CNotificationKodi::~CNotificationKodi()
{
}

/*
	Locate a valid image to send with the message.  Logic is:
	o	If an image is specified, use it if it exists
	o	If a switch type is specified use some simple rules to work out the image and use it if it exists
		o	Look for custom image if set plus '48_State'
		o	Look for base image plus '_State'
		o	Look for base image plus '-state'
		o	Look for base image plus 'state'
		o	Look for base image
	o	If it exists, use the logo as the default image
*/
const char * CNotificationKodi::IconFile(const std::string &ExtraData)
{
	std::string	szImageFile;

	int	posImage = (int)ExtraData.find("|Image=");
	if (posImage >= 0)
	{
//		_log.Log(LOG_NORM, "Image data found in extra data: %s, %d", ExtraData.c_str(), posImage);
		posImage+=7;
#ifdef WIN32
		szImageFile = szWWWFolder  + "\\images\\" + ExtraData.substr(posImage, ExtraData.find("|", posImage)-posImage) + ".png";
#else
		szImageFile = szWWWFolder  + "/images/" + ExtraData.substr(posImage, ExtraData.find("|", posImage)-posImage) + ".png";
#endif
		if (file_exist(szImageFile.c_str()))
		{
			_log.Log(LOG_NORM, "Icon file to be used: %s", szImageFile.c_str());
			return szImageFile.c_str();
		}
		_log.Log(LOG_NORM, "File does not exist:  %s, %d", szImageFile.c_str(), posImage-7);
	}

	// if a switch type was supplied try and work out the image
	std::string	szStatus = "On";
	int	posStatus = (int)ExtraData.find("|Status=");
	if (posStatus >= 0)
	{
		posStatus+=8;
		szStatus = ExtraData.substr(posStatus, ExtraData.find("|", posStatus)-posStatus);
	}
	int	posType = (int)ExtraData.find("|SwitchType=");
	if (posType >= 0)
	{
//		_log.Log(LOG_NORM, "SwitchType data found in extra data: %s, %d", ExtraData.c_str(), posType);
		posType+=12;
		std::string	szType = ExtraData.substr(posType, ExtraData.find("|", posType)-posType);
		std::string	szTypeImage;
		_eSwitchType switchtype=(_eSwitchType)atoi(szType.c_str());
		switch (switchtype)
		{
			case STYPE_OnOff:
				szTypeImage = "Light48";
				break;
			case STYPE_Doorbell:
				szTypeImage = "doorbell48";
				break;
			case STYPE_Contact:
				szTypeImage = "contact48";
				break;
			case STYPE_Blinds:
			case STYPE_BlindsPercentage:
			case STYPE_VenetianBlindsUS:
			case STYPE_VenetianBlindsEU:
			case STYPE_BlindsPercentageInverted:
			case STYPE_BlindsInverted:
				szTypeImage = "blinds48";
				break;
			case STYPE_X10Siren:
				szTypeImage = "siren";
				break;
			case STYPE_SMOKEDETECTOR:
				szTypeImage = "smoke48";
				break;
			case STYPE_Dimmer:
				szTypeImage = "Dimmer48";
				break;
			case STYPE_Motion:
				szTypeImage = "motion48";
				break;
			case STYPE_PushOn:
				szTypeImage = "pushon48";
				break;
			case STYPE_PushOff:
				szTypeImage = "pushon48";
				break;
			case STYPE_DoorLock:
				szTypeImage = "door48";
				break;
			default:
				szTypeImage = "logo";
		}
#ifdef WIN32
		szImageFile = szWWWFolder  + "\\images\\" + szTypeImage + "_" + szStatus + ".png";
#else
		szImageFile = szWWWFolder  + "/images/" + szTypeImage + "_" + szStatus + ".png";
#endif
		if (file_exist(szImageFile.c_str()))
		{
			_log.Log(LOG_NORM, "Icon file to be used: %s", szImageFile.c_str());
			return szImageFile.c_str();
		}
		_log.Log(LOG_NORM, "File does not exist:  %s", szImageFile.c_str());

#ifdef WIN32
			szImageFile = szWWWFolder  + "\\images\\" + szTypeImage + ((szStatus=="Off") ? "-off" : "-on") + ".png";
#else
			szImageFile = szWWWFolder  + "/images/" + szTypeImage + ((szStatus=="Off") ? "-off" : "-on") + ".png";
#endif
		if (file_exist(szImageFile.c_str()))
		{
			_log.Log(LOG_NORM, "Icon file to be used: %s", szImageFile.c_str());
			return szImageFile.c_str();
		}
		_log.Log(LOG_NORM, "File does not exist:  %s", szImageFile.c_str());

#ifdef WIN32
			szImageFile = szWWWFolder  + "\\images\\" + szTypeImage + ((szStatus=="Off") ? "off" : "on") + ".png";
#else
			szImageFile = szWWWFolder  + "/images/" + szTypeImage + ((szStatus=="Off") ? "off" : "on") + ".png";
#endif
		if (file_exist(szImageFile.c_str()))
		{
			_log.Log(LOG_NORM, "Icon file to be used: %s", szImageFile.c_str());
			return szImageFile.c_str();
		}
		_log.Log(LOG_NORM, "File does not exist:  %s", szImageFile.c_str());

#ifdef WIN32
			szImageFile = szWWWFolder  + "\\images\\" + szTypeImage + ".png";
#else
			szImageFile = szWWWFolder  + "/images/" + szTypeImage + ".png";
#endif
		if (file_exist(szImageFile.c_str()))
		{
			_log.Log(LOG_NORM, "Icon file to be used: %s", szImageFile.c_str());
			return szImageFile.c_str();
		}
		_log.Log(LOG_NORM, "File does not exist:  %s", szImageFile.c_str());
	}

	// Image of last resort is the logo
#ifdef WIN32
		szImageFile = szWWWFolder  + "\\images\\logo.png";
#else
		szImageFile = szWWWFolder  + "/images/logo.png";
#endif
	if (!file_exist(szImageFile.c_str()))
	{
		_log.Log(LOG_NORM, "Logo image file does not exist: %s", szImageFile.c_str());
		return NULL;
	}
	return szImageFile.c_str();
}

bool CNotificationKodi::SendMessageImplementation(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string &Sound, const bool bFromNotification)
{
	std::stringstream logline;
	CAddress	_Address;
	int			_Sock;
	bool		bMulticast = (_IPAddress.substr(0,4) >= "224.") && (_IPAddress.substr(0,4) <= "239.");

	CAddress my_addr(_IPAddress.c_str(), _Port);
	_Address = my_addr;
	_Sock = -1;
	if (bMulticast) {
		_Sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		setsockopt(_Sock, IPPROTO_IP, IP_MULTICAST_TTL, (const char*)&_TTL, sizeof(_TTL));
		u_char loop = 1;
		setsockopt(_Sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
	}
	else {
		_Sock = socket(AF_INET, SOCK_DGRAM, 0);
	}
	
	if (_Sock < 0)
	{
		logline << "Error creating socket: " << _IPAddress << ":" << _Port;
		_log.Log(LOG_ERROR, std::string(logline.str()).c_str());
		return false;
	}

	_Address.Bind(_Sock);

	// Don't notify the same thing in two fields
	std::string	sSubject("Domoticz");
	if (Subject != Text)
	{
		sSubject = Subject;
	}
	else
	{
		size_t	posDevice = ExtraData.find("|Name=");
		if (posDevice != std::string::npos)
		{
			posDevice+=6;
			sSubject = ExtraData.substr(posDevice, ExtraData.find("|", posDevice)-posDevice);
		}
	}

	logline << "Kodi Notification (" << _IPAddress << ":" << _Port << ", TTL " << _TTL << "): " << sSubject << ", " << Text;
	_log.Log(LOG_NORM, std::string(logline.str()).c_str());

	CPacketNOTIFICATION packet(sSubject.c_str(), Text.c_str(), ICON_PNG, IconFile(ExtraData));
	if (!packet.Send(_Sock, _Address)) {
		logline << "Error sending notification: " << _IPAddress << ":" << _Port;
		_log.Log(LOG_ERROR, std::string(logline.str()).c_str());
		return false;
	}

	return true;
}

bool CNotificationKodi::IsConfigured()
{
	return (!_IPAddress.empty());
}
