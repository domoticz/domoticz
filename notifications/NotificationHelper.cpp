#include "stdafx.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "../hardware/hardwaretypes.h"
#include "NotificationHelper.h"
#include "NotificationProwl.h"
#include "NotificationNma.h"
#include "NotificationPushbullet.h"
#include "NotificationPushover.h"
#include "NotificationPushsafer.h"
#include "NotificationPushalot.h"
#include "NotificationEmail.h"
#include "NotificationSMS.h"
#include "NotificationHTTP.h"
#include "NotificationKodi.h"
#include "NotificationLogitechMediaServer.h"
#include "NotificationGCM.h"


#include <boost/lexical_cast.hpp>
#include <map>

#if defined WIN32
	#include "../msbuild/WindowsHelper.h"
#endif

typedef std::map<std::string, CNotificationBase*>::iterator it_noti_type;

CNotificationHelper::CNotificationHelper()
{
	m_NotificationSwitchInterval = 0;
	m_NotificationSensorInterval = 12 * 3600;

	/* more notifiers can be added here */

	AddNotifier(new CNotificationProwl());
	AddNotifier(new CNotificationNma());
	AddNotifier(new CNotificationPushbullet());
	AddNotifier(new CNotificationPushover());
	AddNotifier(new CNotificationPushsafer());
	AddNotifier(new CNotificationPushalot());
	AddNotifier(new CNotificationEmail());
	AddNotifier(new CNotificationSMS());
	AddNotifier(new CNotificationHTTP());
	AddNotifier(new CNotificationKodi());
	AddNotifier(new CNotificationLogitechMediaServer());
	AddNotifier(new CNotificationGCM());
}

CNotificationHelper::~CNotificationHelper()
{
	for (it_noti_type iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter) {
		delete iter->second;
	}
}

void CNotificationHelper::Init()
{
	ReloadNotifications();
}

void CNotificationHelper::AddNotifier(CNotificationBase *notifier)
{
	m_notifiers[notifier->GetSubsystemId()] = notifier;
}

bool CNotificationHelper::SendMessage(const std::string &subsystems, const std::string &Subject, const std::string &Text, const std::string &ExtraData, const bool bFromNotification)
{
	return SendMessageEx(subsystems, Subject, Text, ExtraData, 0, std::string(""), bFromNotification);
}

bool CNotificationHelper::SendMessageEx(const std::string &subsystems, const std::string &Subject, const std::string &Text, const std::string &ExtraData, int Priority, const std::string &Sound, const bool bFromNotification)
{
	bool bRet = false;
#if defined WIN32
	//Make a system tray message
	ShowSystemTrayNotification(Subject.c_str());
#endif
	std::vector<std::string> sResult;
	StringSplit(subsystems, ";", sResult);

	std::map<std::string, int> ActiveSystems;

	std::vector<std::string>::const_iterator itt;
	for (itt = sResult.begin(); itt != sResult.end(); ++itt)
	{
		ActiveSystems[*itt] = 1;
	}

	for (it_noti_type iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter) {
		std::map<std::string, int>::const_iterator ittSystem = ActiveSystems.find(iter->first);
		if (ActiveSystems.empty() || (ittSystem!=ActiveSystems.end() && iter->second->IsConfigured())) 
		{
			bRet |= iter->second->SendMessageEx(Subject, Text, ExtraData, Priority, Sound, bFromNotification);
		}
	}
	return bRet;
}

void CNotificationHelper::SetConfigValue(const std::string &key, const std::string &value)
{
	for (it_noti_type iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter) {
		iter->second->SetConfigValue(key, value);
	}
}

void CNotificationHelper::ConfigFromGetvars(const request& req, const bool save)
{
	for (it_noti_type iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter) {
		iter->second->ConfigFromGetvars(req, save);
	}
}

bool CNotificationHelper::IsInConfig(const std::string &Key)
{
	for (it_noti_type iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter) {
		if (iter->second->IsInConfig(Key)) {
			return true;
		}
	}
	return false;
}

void CNotificationHelper::LoadConfig()
{
	int tot = 0, active = 0;
	std::stringstream logline;
	logline << "Active notification subsystems:";
	for (it_noti_type iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter) {
		tot++;
		iter->second->LoadConfig();
		if (iter->second->IsConfigured()) {
			if ((iter->second->m_IsEnabled) && (iter->first != "gcm"))
			{
				if (active == 0)
					logline << " " << iter->first;
				else
					logline << ", " << iter->first;
				active++;
			}
		}
	}
	logline << " (" << active << "/" << tot << ")";
	_log.Log(LOG_NORM, std::string(logline.str()).c_str());
}

std::string CNotificationHelper::ParseCustomMessage(const std::string &cMessage, const std::string &sName, const std::string &sValue)
{
	std::string ret = cMessage;
	stdreplace(ret, "$name", sName);
	stdreplace(ret, "$value", sValue);
	return ret;
}

bool CNotificationHelper::CheckAndHandleTempHumidityNotification(
	const unsigned long long Idx,
	const std::string &devicename,
	const float temp,
	const int humidity,
	const bool bHaveTemp,
	const bool bHaveHumidity)
{
	std::vector<_tNotification> notifications = GetNotifications(Idx);
	if (notifications.size() == 0)
		return false;

	char szTmp[600];
	std::string notValue;

	std::string szExtraData = "|Name=" + devicename + "|";

	time_t atime = mytime(NULL);

	//check if not sent 12 hours ago, and if applicable

	atime -= m_NotificationSensorInterval;

	std::string msg = "";

	std::string signtemp = Notification_Type_Desc(NTYPE_TEMPERATURE, 1);
	std::string signhum = Notification_Type_Desc(NTYPE_HUMIDITY, 1);

	std::vector<_tNotification>::const_iterator itt;
	for (itt = notifications.begin(); itt != notifications.end(); ++itt)
	{
		if ((atime >= itt->LastSend) || (itt->SendAlways)) //emergency always goes true
		{
			std::vector<std::string> splitresults;
			StringSplit(itt->Params, ";", splitresults);
			if (splitresults.size() < 3)
				continue; //impossible
			std::string ntype = splitresults[0];
			bool bWhenIsGreater = (splitresults[1] == ">");
			float svalue = static_cast<float>(atof(splitresults[2].c_str()));
			if (m_sql.m_tempunit == TEMPUNIT_F)
			{
				//Convert to Celsius
				svalue = (svalue / 1.8f) - 32.0f;
			}

			bool bSendNotification = false;

			if ((ntype == signtemp) && (bHaveTemp))
			{
				//temperature
				if (temp > 30.0) szExtraData += "Image=temp-gt-30|";
				else if (temp > 25.0) szExtraData += "Image=temp-25-30|";
				else if (temp > 20.0) szExtraData += "Image=temp-20-25|";
				else if (temp > 15.0) szExtraData += "Image=temp-15-20|";
				else if (temp > 10.0) szExtraData += "Image=temp-10-15|";
				else if (temp > 5.0) szExtraData += "Image=temp-5-10|";
				else szExtraData += "Image=temp48|";
				if (bWhenIsGreater)
				{
					if (temp > svalue)
					{
						bSendNotification = true;
						sprintf(szTmp, "%s temperature is %.1f degrees", devicename.c_str(), temp);
						msg = szTmp;
					}
				}
				else
				{
					if (temp < svalue)
					{
						bSendNotification = true;
						sprintf(szTmp, "%s temperature is %.1f degrees", devicename.c_str(), temp);
						msg = szTmp;
					}
				}
				if (bSendNotification)
				{
					sprintf(szTmp, "%.1f", temp);
					notValue = szTmp;
				}
			}
			else if ((ntype == signhum) && (bHaveHumidity))
			{
				//humidity
				szExtraData += "Image=moisture48|";
				if (bWhenIsGreater)
				{
					if (humidity > svalue)
					{
						bSendNotification = true;
						sprintf(szTmp, "%s Humidity is %d %%", devicename.c_str(), humidity);
						msg = szTmp;
					}
				}
				else
				{
					if (humidity < svalue)
					{
						bSendNotification = true;
						sprintf(szTmp, "%s Humidity is %d %%", devicename.c_str(), humidity);
						msg = szTmp;
					}
				}
				if (bSendNotification)
				{
					sprintf(szTmp, "%d", humidity);
					notValue = szTmp;
				}
			}
			if (bSendNotification)
			{
				if (!itt->CustomMessage.empty())
					msg = ParseCustomMessage(itt->CustomMessage, devicename, notValue);
				SendMessageEx(itt->ActiveSystems, msg, msg, szExtraData, itt->Priority, std::string(""), true);
				TouchNotification(itt->ID);
			}
		}
	}
	return true;
}

bool CNotificationHelper::CheckAndHandleDewPointNotification(
	const unsigned long long Idx,
	const std::string &devicename,
	const float temp,
	const float dewpoint)
{
	std::vector<_tNotification> notifications = GetNotifications(Idx);
	if (notifications.size() == 0)
		return false;

	char szTmp[600];
	std::string szExtraData = "|Name=" + devicename + "|Image=temp-0-5|";
	std::string notValue;

	time_t atime = mytime(NULL);

	//check if not sent 12 hours ago, and if applicable

	atime -= m_NotificationSensorInterval;

	std::string msg = "";

	std::string signdewpoint = Notification_Type_Desc(NTYPE_DEWPOINT, 1);

	std::vector<_tNotification>::const_iterator itt;
	for (itt = notifications.begin(); itt != notifications.end(); ++itt)
	{
		if ((atime >= itt->LastSend) || (itt->SendAlways)) //emergency always goes true
		{
			std::vector<std::string> splitresults;
			StringSplit(itt->Params, ";", splitresults);
			if (splitresults.size() < 1)
				continue; //impossible
			std::string ntype = splitresults[0];

			bool bSendNotification = false;

			if (ntype == signdewpoint)
			{
				//dewpoint
				if (temp <= dewpoint)
				{
					bSendNotification = true;
					sprintf(szTmp, "%s Dew Point reached (%.1f degrees)", devicename.c_str(), temp);
					msg = szTmp;
					sprintf(szTmp, "%.1f", temp);
					notValue = szTmp;
				}
			}
			if (bSendNotification)
			{
				if (!itt->CustomMessage.empty())
					msg = ParseCustomMessage(itt->CustomMessage, devicename, notValue);
				SendMessageEx(itt->ActiveSystems, msg, msg, szExtraData, itt->Priority, std::string(""), true);
				TouchNotification(itt->ID);
			}
		}
	}
	return true;
}

bool CNotificationHelper::CheckAndHandleAmpere123Notification(
	const unsigned long long Idx,
	const std::string &devicename,
	const float Ampere1,
	const float Ampere2,
	const float Ampere3)
{
	std::vector<_tNotification> notifications = GetNotifications(Idx);
	if (notifications.size() == 0)
		return false;

	char szTmp[600];

	std::string szExtraData = "|Name=" + devicename + "|Image=current48|";

	time_t atime = mytime(NULL);

	//check if not sent 12 hours ago, and if applicable
	atime -= m_NotificationSensorInterval;

	std::string msg = "";
	std::string notValue;

	std::string signamp1 = Notification_Type_Desc(NTYPE_AMPERE1, 1);
	std::string signamp2 = Notification_Type_Desc(NTYPE_AMPERE2, 2);
	std::string signamp3 = Notification_Type_Desc(NTYPE_AMPERE3, 3);

	std::vector<_tNotification>::const_iterator itt;
	for (itt = notifications.begin(); itt != notifications.end(); ++itt)
	{
		if ((atime >= itt->LastSend) || (itt->SendAlways)) //emergency always goes true
		{
			std::vector<std::string> splitresults;
			StringSplit(itt->Params, ";", splitresults);
			if (splitresults.size() < 3)
				continue; //impossible
			std::string ntype = splitresults[0];
			bool bWhenIsGreater = (splitresults[1] == ">");
			float svalue = static_cast<float>(atof(splitresults[2].c_str()));

			bool bSendNotification = false;

			if (ntype == signamp1)
			{
				//Ampere1
				if (bWhenIsGreater)
				{
					if (Ampere1 > svalue)
					{
						bSendNotification = true;
						sprintf(szTmp, "%s Ampere1 is %.1f Ampere", devicename.c_str(), Ampere1);
						msg = szTmp;
					}
				}
				else
				{
					if (Ampere1 < svalue)
					{
						bSendNotification = true;
						sprintf(szTmp, "%s Ampere1 is %.1f Ampere", devicename.c_str(), Ampere1);
						msg = szTmp;
					}
				}
				if (bSendNotification)
				{
					sprintf(szTmp, "%.1f", Ampere1);
					notValue = szTmp;
				}
			}
			else if (ntype == signamp2)
			{
				//Ampere2
				if (bWhenIsGreater)
				{
					if (Ampere2 > svalue)
					{
						bSendNotification = true;
						sprintf(szTmp, "%s Ampere2 is %.1f Ampere", devicename.c_str(), Ampere2);
						msg = szTmp;
					}
				}
				else
				{
					if (Ampere2 < svalue)
					{
						bSendNotification = true;
						sprintf(szTmp, "%s Ampere2 is %.1f Ampere", devicename.c_str(), Ampere2);
						msg = szTmp;
					}
				}
				if (bSendNotification)
				{
					sprintf(szTmp, "%.1f", Ampere2);
					notValue = szTmp;
				}
			}
			else if (ntype == signamp3)
			{
				//Ampere3
				if (bWhenIsGreater)
				{
					if (Ampere3 > svalue)
					{
						bSendNotification = true;
						sprintf(szTmp, "%s Ampere3 is %.1f Ampere", devicename.c_str(), Ampere3);
						msg = szTmp;
					}
				}
				else
				{
					if (Ampere3 < svalue)
					{
						bSendNotification = true;
						sprintf(szTmp, "%s Ampere3 is %.1f Ampere", devicename.c_str(), Ampere3);
						msg = szTmp;
					}
				}
				if (bSendNotification)
				{
					sprintf(szTmp, "%.1f", Ampere3);
					notValue = szTmp;
				}
			}
			if (bSendNotification)
			{
				if (!itt->CustomMessage.empty())
					msg = ParseCustomMessage(itt->CustomMessage, devicename, notValue);
				SendMessageEx(itt->ActiveSystems, msg, msg, szExtraData, itt->Priority, std::string(""), true);
				TouchNotification(itt->ID);
			}
		}
	}
	return true;
}

bool CNotificationHelper::CheckAndHandleNotification(
	const unsigned long long Idx,
	const std::string &devicename,
	const _eNotificationTypes ntype,
	const std::string &message)
{
	std::vector<_tNotification> notifications = GetNotifications(Idx);
	if (notifications.size() == 0)
		return false;

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT SwitchType, CustomImage FROM DeviceStatus WHERE (ID=%llu)", Idx);
	if (result.size() == 0)
		return false;

	std::string szExtraData = "|Name=" + devicename + "|SwitchType=" + result[0][0] + "|CustomImage=" + result[0][1] + "|";
	std::string notValue;

	time_t atime = mytime(NULL);

	//check if not sent 12 hours ago, and if applicable
	atime -= m_NotificationSensorInterval;

	std::string ltype = Notification_Type_Desc(ntype, 1);
	std::vector<_tNotification>::const_iterator itt;
	for (itt = notifications.begin(); itt != notifications.end(); ++itt)
	{
		std::vector<std::string> splitresults;
		StringSplit(itt->Params, ";", splitresults);
		if (splitresults.size() < 1)
			continue; //impossible
		std::string atype = splitresults[0];
		if (atype == ltype)
		{
			if ((atime >= itt->LastSend) || (itt->SendAlways)) //emergency always goes true
			{
				std::string msg = message;
				if (!itt->CustomMessage.empty())
					msg = ParseCustomMessage(itt->CustomMessage, devicename, notValue);
				SendMessageEx(itt->ActiveSystems, msg, msg, szExtraData, itt->Priority, std::string(""), true);
				TouchNotification(itt->ID);
			}
		}
	}
	return true;
}

bool CNotificationHelper::CheckAndHandleNotification(
	const unsigned long long Idx,
	const std::string &devicename,
	const unsigned char devType,
	const unsigned char subType,
	const _eNotificationTypes ntype,
	const float mvalue)
{
	std::vector<_tNotification> notifications = GetNotifications(Idx);
	if (notifications.size() == 0)
		return false;

	char szTmp[600];

	double intpart;
	std::string pvalue;
	if (modf(mvalue, &intpart) == 0)
		sprintf(szTmp, "%.0f", mvalue);
	else
		sprintf(szTmp, "%.1f", mvalue);
	pvalue = szTmp;

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT SwitchType FROM DeviceStatus WHERE (ID=%llu)", Idx);
	if (result.size() == 0)
		return false;
	std::string szExtraData = "|Name=" + devicename + "|SwitchType=" + result[0][0] + "|";

	time_t atime = mytime(NULL);

	//check if not sent 12 hours ago, and if applicable
	atime -= m_NotificationSensorInterval;

	std::string msg = "";

	std::string ltype = Notification_Type_Desc(ntype, 0);
	std::string nsign = Notification_Type_Desc(ntype, 1);
	std::string label = Notification_Type_Label(ntype);

	std::vector<_tNotification>::const_iterator itt;
	for (itt = notifications.begin(); itt != notifications.end(); ++itt)
	{
		if ((atime >= itt->LastSend) || (itt->SendAlways)) //emergency always goes true
		{
			std::vector<std::string> splitresults;
			StringSplit(itt->Params, ";", splitresults);
			if (splitresults.size() < 3)
				continue; //impossible
			std::string ntype = splitresults[0];
			bool bWhenIsGreater = (splitresults[1] == ">");
			float svalue = static_cast<float>(atof(splitresults[2].c_str()));

			bool bSendNotification = false;

			if (ntype == nsign)
			{
				if (bWhenIsGreater)
				{
					if (mvalue > svalue)
					{
						bSendNotification = true;
						sprintf(szTmp, "%s %s is %s %s",
							devicename.c_str(),
							ltype.c_str(),
							pvalue.c_str(),
							label.c_str()
							);
						msg = szTmp;
					}
				}
				else
				{
					if (mvalue < svalue)
					{
						bSendNotification = true;
						sprintf(szTmp, "%s %s is %s %s",
							devicename.c_str(),
							ltype.c_str(),
							pvalue.c_str(),
							label.c_str()
							);
						msg = szTmp;
					}
				}
			}
			if (bSendNotification)
			{
				if (!itt->CustomMessage.empty())
				{
					msg = ParseCustomMessage(itt->CustomMessage, devicename, pvalue);
				}
				SendMessageEx(itt->ActiveSystems, msg, msg, szExtraData, itt->Priority, std::string(""), true);
				TouchNotification(itt->ID);
			}
		}
	}
	return true;
}

bool CNotificationHelper::CheckAndHandleSwitchNotification(
	const unsigned long long Idx,
	const std::string &devicename,
	const _eNotificationTypes ntype)
{
	std::vector<_tNotification> notifications = GetNotifications(Idx);
	if (notifications.size() == 0)
		return false;

	std::vector<std::vector<std::string> > result;

	result = m_sql.safe_query("SELECT SwitchType, CustomImage FROM DeviceStatus WHERE (ID=%llu)",
		Idx);
	if (result.size() == 0)
		return false;
	_eSwitchType switchtype = (_eSwitchType)atoi(result[0][0].c_str());
	std::string szExtraData = "|Name=" + devicename + "|SwitchType=" + result[0][0] + "|CustomImage=" + result[0][1] + "|";

	std::string msg = "";

	std::string ltype = Notification_Type_Desc(ntype, 1);

	time_t atime = mytime(NULL);
	atime -= m_NotificationSwitchInterval;

	std::vector<_tNotification>::const_iterator itt;
	for (itt = notifications.begin(); itt != notifications.end(); ++itt)
	{
		if ((atime >= itt->LastSend) || (itt->SendAlways)) //emergency always goes true
		{
			std::vector<std::string> splitresults;
			StringSplit(itt->Params, ";", splitresults);
			if (splitresults.size() < 1)
				continue; //impossible
			std::string atype = splitresults[0];

			bool bSendNotification = false;
			std::string notValue;

			if (atype == ltype)
			{
				bSendNotification = true;
				msg = devicename;
				if (ntype == NTYPE_SWITCH_ON)
				{
					szExtraData += "Status=On|";
					switch (switchtype)
					{
					case STYPE_Doorbell:
						notValue = "pressed";
						break;
					case STYPE_Contact:
						notValue = "Open";
						szExtraData += "Image=contact48_open|";
						break;
					case STYPE_DoorLock:
						notValue = "Open";
						szExtraData += "Image=door48open|";
						break;
					case STYPE_Motion:
						notValue = "movement detected";
						break;
					case STYPE_SMOKEDETECTOR:
						notValue = "ALARM/FIRE !";
						break;
					default:
						notValue = ">> ON";
						break;
					}
				}
				else {
					szExtraData += "Status=Off|";
					switch (switchtype)
					{
					case STYPE_DoorLock:
					case STYPE_Contact:
						notValue = "Closed";
						break;
					default:
						notValue = ">> OFF";
						break;
					}
				}
				msg += " " + notValue;
			}
			if (bSendNotification)
			{
				if (!itt->CustomMessage.empty())
					msg = ParseCustomMessage(itt->CustomMessage, devicename, notValue);
				SendMessageEx(itt->ActiveSystems, msg, msg, szExtraData, itt->Priority, std::string(""), true);
				TouchNotification(itt->ID);
			}
		}
	}
	return true;
}

bool CNotificationHelper::CheckAndHandleSwitchNotification(
	const unsigned long long Idx,
	const std::string &devicename,
	const _eNotificationTypes ntype,
	const int llevel)
{
	std::vector<_tNotification> notifications = GetNotifications(Idx);
	if (notifications.size() == 0)
		return false;
	std::vector<std::vector<std::string> > result;

	result = m_sql.safe_query("SELECT SwitchType, CustomImage, Options FROM DeviceStatus WHERE (ID=%llu)",
		Idx);
	if (result.size() == 0)
		return false;
	_eSwitchType switchtype = (_eSwitchType)atoi(result[0][0].c_str());
	std::string szExtraData = "|Name=" + devicename + "|SwitchType=" + result[0][0] + "|CustomImage=" + result[0][1] + "|";
	std::string sOptions = result[0][2].c_str();

	std::string msg = "";

	std::string ltype = Notification_Type_Desc(ntype, 1);

	time_t atime = mytime(NULL);
	atime -= m_NotificationSwitchInterval;

	std::vector<_tNotification>::const_iterator itt;
	for (itt = notifications.begin(); itt != notifications.end(); ++itt)
	{
		if ((atime >= itt->LastSend) || (itt->SendAlways)) //emergency always goes true
		{
			std::vector<std::string> splitresults;
			StringSplit(itt->Params, ";", splitresults);
			if (splitresults.size() < 1)
				continue; //impossible
			std::string atype = splitresults[0];

			bool bSendNotification = false;
			std::string notValue;

			if (atype == ltype)
			{
				msg = devicename;
				if (ntype == NTYPE_SWITCH_ON)
				{
					if (splitresults.size() < 3)
						continue; //impossible
					bool bWhenEqual = (splitresults[1] == "=");
					int iLevel = atoi(splitresults[2].c_str());
					if (!bWhenEqual || iLevel < 10 || iLevel > 100)
						continue; //invalid

					if (llevel == iLevel) 
					{
						bSendNotification = true;
						std::string sLevel = boost::lexical_cast<std::string>(llevel);
						szExtraData += "Status=Level " + sLevel + "|";

						if (switchtype == STYPE_Selector)
						{
							std::map<std::string, std::string> options = m_sql.BuildDeviceOptions(sOptions);
							std::string levelNames = options["LevelNames"];
							std::vector<std::string> splitresults;
							StringSplit(levelNames, "|", splitresults);
							msg += " >> " + splitresults[(llevel / 10)];
							notValue = ">> " + splitresults[(llevel / 10)];
						}
						else
						{
							msg += " >> LEVEL " + sLevel;
							notValue = ">> LEVEL " + sLevel;
						}
					}
				}
				else 
				{
					bSendNotification = true;
					szExtraData += "Status=Off|";
					msg += " >> OFF";
					notValue = ">> OFF";
				}
			}
			if (bSendNotification)
			{
				if (!itt->CustomMessage.empty())
					msg = ParseCustomMessage(itt->CustomMessage, devicename, notValue);
				SendMessageEx(itt->ActiveSystems, msg, msg, szExtraData, itt->Priority, std::string(""), true);
				TouchNotification(itt->ID);
			}
		}
	}
	return true;
}

bool CNotificationHelper::CheckAndHandleRainNotification(
	const unsigned long long Idx,
	const std::string &devicename,
	const unsigned char devType,
	const unsigned char subType,
	const _eNotificationTypes ntype,
	const float mvalue)
{
	std::vector<std::vector<std::string> > result;

	result = m_sql.safe_query("SELECT AddjValue,AddjMulti FROM DeviceStatus WHERE (ID=%llu)",
		Idx);
	if (result.size() == 0)
		return false;
	//double AddjValue = atof(result[0][0].c_str());
	double AddjMulti = atof(result[0][1].c_str());

	char szDateEnd[40];

	time_t now = mytime(NULL);
	struct tm tm1;
	localtime_r(&now, &tm1);
	struct tm ltime;
	ltime.tm_isdst = tm1.tm_isdst;
	ltime.tm_hour = 0;
	ltime.tm_min = 0;
	ltime.tm_sec = 0;
	ltime.tm_year = tm1.tm_year;
	ltime.tm_mon = tm1.tm_mon;
	ltime.tm_mday = tm1.tm_mday;
	sprintf(szDateEnd, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

	if (subType != sTypeRAINWU)
	{
		result = m_sql.safe_query("SELECT MIN(Total) FROM Rain WHERE (DeviceRowID=%llu AND Date>='%q')",
			Idx, szDateEnd);
		if (result.size() > 0)
		{
			std::vector<std::string> sd = result[0];

			float total_min = static_cast<float>(atof(sd[0].c_str()));
			float total_max = mvalue;
			double total_real = total_max - total_min;
			total_real *= AddjMulti;
			CheckAndHandleNotification(Idx, devicename, devType, subType, NTYPE_RAIN, (float)total_real);
		}
	}
	else
	{
		//value is already total rain
		double total_real = mvalue;
		total_real *= AddjMulti;
		CheckAndHandleNotification(Idx, devicename, devType, subType, NTYPE_RAIN, (float)total_real);
	}
	return false;
}

void CNotificationHelper::TouchNotification(const unsigned long long ID)
{
	char szDate[50];
	time_t atime = mytime(NULL);
	struct tm ltime;
	localtime_r(&atime, &ltime);
	sprintf(szDate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);

	//Set LastSend date
	m_sql.safe_query("UPDATE Notifications SET LastSend='%q' WHERE (ID=%llu)",
		szDate, ID);

	//Also touch it internally
	boost::lock_guard<boost::mutex> l(m_mutex);

	std::map<unsigned long long, std::vector<_tNotification> >::iterator itt;
	for (itt = m_notifications.begin(); itt != m_notifications.end(); ++itt)
	{
		std::vector<_tNotification>::iterator itt2;
		for (itt2 = itt->second.begin(); itt2 != itt->second.end(); ++itt2)
		{
			if (itt2->ID == ID)
			{
				itt2->LastSend = atime;
				return;
			}
		}
	}
}

bool CNotificationHelper::AddNotification(const std::string &DevIdx, const std::string &Param, const std::string &CustomMessage, const std::string &ActiveSystems, const int Priority, const bool SendAlways)
{
	std::vector<std::vector<std::string> > result;

	//First check for duplicate, because we do not want this
	result = m_sql.safe_query("SELECT ROWID FROM Notifications WHERE (DeviceRowID=='%q') AND (Params=='%q')",
		DevIdx.c_str(), Param.c_str());
	if (result.size() > 0)
		return false;//already there!

	int iSendAlways = (SendAlways == true) ? 1 : 0;
	m_sql.safe_query("INSERT INTO Notifications (DeviceRowID, Params, CustomMessage, ActiveSystems, Priority, SendAlways) VALUES ('%q','%q','%q','%q',%d,%d)",
		DevIdx.c_str(), Param.c_str(), CustomMessage.c_str(), ActiveSystems.c_str(), Priority, iSendAlways);
	ReloadNotifications();
	return true;
}

bool CNotificationHelper::RemoveDeviceNotifications(const std::string &DevIdx)
{
	m_sql.safe_query("DELETE FROM Notifications WHERE (DeviceRowID=='%q')",
		DevIdx.c_str());
	ReloadNotifications();
	return true;
}

bool CNotificationHelper::RemoveNotification(const std::string &ID)
{
	m_sql.safe_query("DELETE FROM Notifications WHERE (ID=='%q')",
		ID.c_str());
	ReloadNotifications();
	return true;
}

std::vector<_tNotification> CNotificationHelper::GetNotifications(const std::string &DevIdx)
{
	std::stringstream s_str(DevIdx);
	unsigned long long idxll;
	s_str >> idxll;
	return GetNotifications(idxll);
}

std::vector<_tNotification> CNotificationHelper::GetNotifications(const unsigned long long DevIdx)
{
	boost::lock_guard<boost::mutex> l(m_mutex);
	std::vector<_tNotification> ret;
	std::map<unsigned long long, std::vector<_tNotification> >::const_iterator itt = m_notifications.find(DevIdx);
	if (itt != m_notifications.end())
	{
		ret = itt->second;
	}
	return ret;
}

bool CNotificationHelper::HasNotifications(const std::string &DevIdx)
{
	std::stringstream s_str(DevIdx);
	unsigned long long idxll;
	s_str >> idxll;
	return HasNotifications(idxll);
}

bool CNotificationHelper::HasNotifications(const unsigned long long DevIdx)
{
	boost::lock_guard<boost::mutex> l(m_mutex);
	return (m_notifications.find(DevIdx) != m_notifications.end());
}

//Re(Loads) all notifications stored in the database, so we do not have to query this all the time
void CNotificationHelper::ReloadNotifications()
{
	boost::lock_guard<boost::mutex> l(m_mutex);
	m_notifications.clear();
	std::vector<std::vector<std::string> > result;

	m_sql.GetPreferencesVar("NotificationSensorInterval", m_NotificationSensorInterval);
	m_sql.GetPreferencesVar("NotificationSwitchInterval", m_NotificationSwitchInterval);

	result = m_sql.safe_query("SELECT ID, DeviceRowID, Params, CustomMessage, ActiveSystems, Priority, SendAlways, LastSend FROM Notifications ORDER BY DeviceRowID");
	if (result.size() == 0)
		return;

	time_t mtime = mytime(NULL);
	struct tm atime;
	localtime_r(&mtime, &atime);

	std::stringstream sstr;

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt = result.begin(); itt != result.end(); ++itt)
	{
		std::vector<std::string> sd = *itt;

		_tNotification notification;
		unsigned long long Idx;

		sstr.clear();
		sstr.str("");

		sstr << sd[0];
		sstr >> notification.ID;

		sstr.clear();
		sstr.str("");

		sstr << sd[1];
		sstr >> Idx;

		notification.Params = sd[2];
		notification.CustomMessage = sd[3];
		notification.ActiveSystems = sd[4];
		notification.Priority = atoi(sd[5].c_str());
		notification.SendAlways = (atoi(sd[6].c_str())!=0);

		std::string stime = sd[7];
		if (stime == "0")
		{
			notification.LastSend = 0;
		}
		else
		{
			struct tm ntime;
			ntime.tm_isdst = atime.tm_isdst;
			ntime.tm_year = atoi(stime.substr(0, 4).c_str()) - 1900;
			ntime.tm_mon = atoi(stime.substr(5, 2).c_str()) - 1;
			ntime.tm_mday = atoi(stime.substr(8, 2).c_str());
			ntime.tm_hour = atoi(stime.substr(11, 2).c_str());
			ntime.tm_min = atoi(stime.substr(14, 2).c_str());
			ntime.tm_sec = atoi(stime.substr(17, 2).c_str());
			notification.LastSend = mktime(&ntime);
		}

		m_notifications[Idx].push_back(notification);
	}
}
