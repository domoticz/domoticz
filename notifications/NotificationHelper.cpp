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

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

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

void CNotificationHelper::RemoveNotifier(CNotificationBase *notifier)
{
	m_notifiers.erase(notifier->GetSubsystemId());
}

bool CNotificationHelper::SendMessage(
	const uint64_t Idx,
	const std::string &Name,
	const std::string &Subsystems,
	const std::string &Subject,
	const std::string &Text,
	const std::string &ExtraData,
	const int Priority,
	const std::string &Sound,
	const bool bFromNotification)
{
	return SendMessageEx(Idx, Name, Subsystems, Subject, Text, ExtraData, -100, std::string(""), bFromNotification);
}

bool CNotificationHelper::SendMessageEx(
	const uint64_t Idx,
	const std::string &Name,
	const std::string &Subsystems,
	const std::string &Subject,
	const std::string &Text,
	const std::string &ExtraData,
	int Priority,
	const std::string &Sound,
	const bool bFromNotification)
{
	bool bRet = false;
	bool bThread = true;

	if (Priority == -100)
	{
		Priority = 0;
		bThread = false;
		bRet = true;
	}

#if defined WIN32
	//Make a system tray message
	ShowSystemTrayNotification(Subject.c_str());
#endif
	std::vector<std::string> sResult;
	StringSplit(Subsystems, ";", sResult);

	std::map<std::string, int> ActiveSystems;

	std::vector<std::string>::const_iterator itt;
	for (itt = sResult.begin(); itt != sResult.end(); ++itt)
	{
		ActiveSystems[*itt] = 1;
	}

	for (it_noti_type iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter)
	{
		std::map<std::string, int>::const_iterator ittSystem = ActiveSystems.find(iter->first);
		if ((ActiveSystems.empty() || ittSystem != ActiveSystems.end()) && iter->second->IsConfigured())
		{
			if (bThread)
				boost::thread SendMessageEx(boost::bind(&CNotificationBase::SendMessageEx, iter->second, Idx, Name, Subject, Text, ExtraData, Priority, Sound, bFromNotification));
			else
				bRet |= iter->second->SendMessageEx(Idx, Name, Subject, Text, ExtraData, Priority, Sound, bFromNotification);
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
	logline << "Active notification Subsystems:";
	for (it_noti_type iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter) {
		tot++;
		iter->second->LoadConfig();
		if (iter->second->IsConfigured()) {
			if (iter->second->m_IsEnabled)
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

bool CNotificationHelper::ApplyRule(std::string rule, bool equal, bool less)
{
	if (((rule == ">") || (rule == ">=")) && (!less) && (!equal))
		return true;
	else if (((rule == "<") || (rule == "<=")) && (less))
		return true;
	else if (((rule == "=") || (rule == ">=") || (rule == "<=")) && (equal))
		return true;
	else if ((rule == "!=") && (!equal))
		return true;
	return false;
}


bool CNotificationHelper::CheckAndHandleTempHumidityNotification(
	const uint64_t Idx,
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

	std::string ltemp = Notification_Type_Label(NTYPE_TEMPERATURE);
	std::string signtemp = Notification_Type_Desc(NTYPE_TEMPERATURE, 1);
	std::string signhum = Notification_Type_Desc(NTYPE_HUMIDITY, 1);

	std::vector<_tNotification>::const_iterator itt;
	for (itt = notifications.begin(); itt != notifications.end(); ++itt)
	{
		if (itt->LastUpdate)
			TouchLastUpdate(itt->ID);

		if ((atime >= itt->LastSend) || (itt->SendAlways) || (!itt->CustomMessage.empty())) //emergency always goes true
		{
			std::string recoverymsg;
			bool bRecoveryMessage = false;
			bRecoveryMessage = CustomRecoveryMessage(itt->ID, recoverymsg, true);
			if ((atime < itt->LastSend) && (!itt->SendAlways) && (!bRecoveryMessage))
				continue;
			std::vector<std::string> splitresults;
			StringSplit(itt->Params, ";", splitresults);
			if (splitresults.size() < 3)
				continue; //impossible
			std::string ntype = splitresults[0];
			std::string custommsg;
			float svalue = static_cast<float>(atof(splitresults[2].c_str()));
			bool bSendNotification = false;
			bool bCustomMessage = false;
			bCustomMessage = CustomRecoveryMessage(itt->ID, custommsg, false);

			if ((ntype == signtemp) && (bHaveTemp))
			{
				//temperature
				if (m_sql.m_tempunit == TEMPUNIT_F)
				{
					//Convert to Celsius
					svalue = static_cast<float>(ConvertToCelsius(svalue));
				}

				if (temp > 30.0) szExtraData += "Image=temp-gt-30|";
				else if (temp > 25.0) szExtraData += "Image=temp-25-30|";
				else if (temp > 20.0) szExtraData += "Image=temp-20-25|";
				else if (temp > 15.0) szExtraData += "Image=temp-15-20|";
				else if (temp > 10.0) szExtraData += "Image=temp-10-15|";
				else if (temp > 5.0) szExtraData += "Image=temp-5-10|";
				else szExtraData += "Image=temp48|";
				bSendNotification = ApplyRule(splitresults[1], (temp == svalue), (temp < svalue));
				if (bSendNotification && (!bRecoveryMessage || itt->SendAlways))
				{
					sprintf(szTmp, "%s temperature is %.1f %s [%s %.1f %s]", devicename.c_str(), temp, ltemp.c_str(), splitresults[1].c_str(), svalue, ltemp.c_str());
					msg = szTmp;
					sprintf(szTmp, "%.1f", temp);
					notValue = szTmp;
				}
				else if (!bSendNotification && bRecoveryMessage)
				{
					bSendNotification = true;
					msg = recoverymsg;
					std::string clearstr = "!";
					CustomRecoveryMessage(itt->ID, clearstr, true);
				}
				else
				{
					bSendNotification = false;
				}
			}
			else if ((ntype == signhum) && (bHaveHumidity))
			{
				//humidity
				szExtraData += "Image=moisture48|";
				bSendNotification = ApplyRule(splitresults[1], (humidity == svalue), (humidity < svalue));
				if (bSendNotification && (!bRecoveryMessage || itt->SendAlways))
				{
					sprintf(szTmp, "%s Humidity is %d %% [%s %.0f %%]", devicename.c_str(), humidity, splitresults[1].c_str(), svalue);
					msg = szTmp;
					sprintf(szTmp, "%d", humidity);
					notValue = szTmp;
				}
				else if (!bSendNotification && bRecoveryMessage)
				{
					bSendNotification = true;
					msg = recoverymsg;
					std::string clearstr = "!";
					CustomRecoveryMessage(itt->ID, clearstr, true);
				}
				else
				{
					bSendNotification = false;
				}
			}
			if (bSendNotification)
			{
				if (bCustomMessage && !bRecoveryMessage)
					msg = ParseCustomMessage(custommsg, devicename, notValue);
				SendMessageEx(Idx, devicename, itt->ActiveSystems, msg, msg, szExtraData, itt->Priority, std::string(""), true);
				if (!bRecoveryMessage)
				{
					TouchNotification(itt->ID);
					CustomRecoveryMessage(itt->ID, msg, true);
				}
			}
		}
	}
	return true;
}

bool CNotificationHelper::CheckAndHandleDewPointNotification(
	const uint64_t Idx,
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
		if (itt->LastUpdate)
			TouchLastUpdate(itt->ID);
		if ((atime >= itt->LastSend) || (itt->SendAlways)) //emergency always goes true
		{
			std::vector<std::string> splitresults;
			StringSplit(itt->Params, ";", splitresults);
			if (splitresults.size() < 1)
				continue; //impossible
			std::string ntype = splitresults[0];

			if (ntype == signdewpoint)
			{
				//dewpoint
				if (temp <= dewpoint)
				{
					sprintf(szTmp, "%s Dew Point reached (%.1f degrees)", devicename.c_str(), temp);
					msg = szTmp;
					sprintf(szTmp, "%.1f", temp);
					notValue = szTmp;
					if (!itt->CustomMessage.empty())
						msg = ParseCustomMessage(itt->CustomMessage, devicename, notValue);
					SendMessageEx(Idx, devicename, itt->ActiveSystems, msg, msg, szExtraData, itt->Priority, std::string(""), true);
					TouchNotification(itt->ID);
				}
			}
		}
	}
	return true;
}

bool CNotificationHelper::CheckAndHandleValueNotification(
	const uint64_t Idx,
	const std::string &DeviceName,
	const int value)
{
	std::vector<_tNotification> notifications = GetNotifications(Idx);
	if (notifications.size() == 0)
		return false;

	char szTmp[600];
	std::string szExtraData = "|Name=" + DeviceName + "|";

	time_t atime = mytime(NULL);

	//check if not sent 12 hours ago, and if applicable
	atime -= m_NotificationSensorInterval;

	std::string msg = "";
	std::string notValue;

	std::string signvalue = Notification_Type_Desc(NTYPE_VALUE, 1);

	std::vector<_tNotification>::const_iterator itt;
	for (itt = notifications.begin(); itt != notifications.end(); ++itt)
	{
		if (itt->LastUpdate)
			TouchLastUpdate(itt->ID);
		if ((atime >= itt->LastSend) || (itt->SendAlways)) //emergency always goes true
		{
			std::vector<std::string> splitresults;
			StringSplit(itt->Params, ";", splitresults);
			if (splitresults.size() < 2)
				continue; //impossible
			std::string ntype = splitresults[0];
			int svalue = static_cast<int>(atoi(splitresults[1].c_str()));

			if (ntype == signvalue)
			{
				if (value > svalue)
				{
					sprintf(szTmp, "%s is %d", DeviceName.c_str(), value);
					msg = szTmp;
					sprintf(szTmp, "%d", value);
					notValue = szTmp;
					if (!itt->CustomMessage.empty())
						msg = ParseCustomMessage(itt->CustomMessage, DeviceName, notValue);
					SendMessageEx(Idx, DeviceName, itt->ActiveSystems, msg, msg, szExtraData, itt->Priority, std::string(""), true);
					TouchNotification(itt->ID);
				}
			}
		}
	}
	return true;
}

bool CNotificationHelper::CheckAndHandleAmpere123Notification(
	const uint64_t Idx,
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
	std::string signamp2 = Notification_Type_Desc(NTYPE_AMPERE2, 1);
	std::string signamp3 = Notification_Type_Desc(NTYPE_AMPERE3, 1);

	std::vector<_tNotification>::const_iterator itt;
	for (itt = notifications.begin(); itt != notifications.end(); ++itt)
	{
		if (itt->LastUpdate)
			TouchLastUpdate(itt->ID);

		if ((atime >= itt->LastSend) || (itt->SendAlways) || (!itt->CustomMessage.empty())) //emergency always goes true
		{
			std::string recoverymsg;
			bool bRecoveryMessage = false;
			bRecoveryMessage = CustomRecoveryMessage(itt->ID, recoverymsg, true);
			if ((atime < itt->LastSend) && (!itt->SendAlways) && (!bRecoveryMessage))
				continue;
			std::vector<std::string> splitresults;
			StringSplit(itt->Params, ";", splitresults);
			if (splitresults.size() < 3)
				continue; //impossible
			std::string ntype = splitresults[0];
			std::string custommsg;
			std::string ltype;
			float svalue = static_cast<float>(atof(splitresults[2].c_str()));
			float ampere;
			bool bSendNotification = false;
			bool bCustomMessage = false;
			bCustomMessage = CustomRecoveryMessage(itt->ID, custommsg, false);

			if (ntype == signamp1)
			{
				ampere = Ampere1;
				ltype = Notification_Type_Desc(NTYPE_AMPERE1, 0);
			}
			else if (ntype == signamp2)
			{
				ampere = Ampere2;
				ltype = Notification_Type_Desc(NTYPE_AMPERE2, 0);
			}
			else if (ntype == signamp3)
			{
				ampere = Ampere3;
				ltype = Notification_Type_Desc(NTYPE_AMPERE3, 0);
			}
			bSendNotification = ApplyRule(splitresults[1], (ampere == svalue), (ampere < svalue));
			if (bSendNotification && (!bRecoveryMessage || itt->SendAlways))
			{
				sprintf(szTmp, "%s %s is %.1f Ampere [%s %.1f Ampere]", devicename.c_str(), ltype.c_str(), ampere, splitresults[1].c_str(), svalue);
				msg = szTmp;
				sprintf(szTmp, "%.1f", ampere);
				notValue = szTmp;
			}
			else if (!bSendNotification && bRecoveryMessage)
			{
				bSendNotification = true;
				msg = recoverymsg;
				std::string clearstr = "!";
				CustomRecoveryMessage(itt->ID, clearstr, true);
			}
			else
			{
				bSendNotification = false;
			}
			if (bSendNotification)
			{
				if (bCustomMessage && !bRecoveryMessage)
					msg = ParseCustomMessage(custommsg, devicename, notValue);
				SendMessageEx(Idx, devicename, itt->ActiveSystems, msg, msg, szExtraData, itt->Priority, std::string(""), true);
				if (!bRecoveryMessage)
				{
					TouchNotification(itt->ID);
					CustomRecoveryMessage(itt->ID, msg, true);
				}
			}
		}
	}
	return true;
}

bool CNotificationHelper::CheckAndHandleNotification(
	const uint64_t Idx,
	const std::string &devicename,
	const _eNotificationTypes ntype,
	const std::string &message)
{
	std::vector<_tNotification> notifications = GetNotifications(Idx);
	if (notifications.size() == 0)
		return false;

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT SwitchType, CustomImage FROM DeviceStatus WHERE (ID=%" PRIu64 ")", Idx);
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
		if (itt->LastUpdate)
			TouchLastUpdate(itt->ID);
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
				SendMessageEx(Idx, devicename, itt->ActiveSystems, msg, msg, szExtraData, itt->Priority, std::string(""), true);
				TouchNotification(itt->ID);
			}
		}
	}
	return true;
}

bool CNotificationHelper::CheckAndHandleNotification(
	const uint64_t Idx,
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
	result = m_sql.safe_query("SELECT SwitchType FROM DeviceStatus WHERE (ID=%" PRIu64 ")", Idx);
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
		if (itt->LastUpdate)
			TouchLastUpdate(itt->ID);

		if ((atime >= itt->LastSend) || (itt->SendAlways) || (!itt->CustomMessage.empty())) //emergency always goes true
		{
			std::string recoverymsg;
			bool bRecoveryMessage = false;
			bRecoveryMessage = CustomRecoveryMessage(itt->ID, recoverymsg, true);
			if ((atime < itt->LastSend) && (!itt->SendAlways) && (!bRecoveryMessage))
				continue;
			std::vector<std::string> splitresults;
			StringSplit(itt->Params, ";", splitresults);
			if (splitresults.size() < 3)
				continue; //impossible
			std::string ntype = splitresults[0];
			std::string custommsg;
			float svalue = static_cast<float>(atof(splitresults[2].c_str()));
			bool bSendNotification = false;
			bool bCustomMessage = false;
			bCustomMessage = CustomRecoveryMessage(itt->ID, custommsg, false);

			if (ntype == nsign)
			{
				bSendNotification = ApplyRule(splitresults[1], (mvalue == svalue), (mvalue < svalue));
				if (bSendNotification && (!bRecoveryMessage || itt->SendAlways))
				{
					sprintf(szTmp, "%s %s is %s %s [%s %.1f %s]", devicename.c_str(), ltype.c_str(), pvalue.c_str(), label.c_str(), splitresults[1].c_str(), svalue, label.c_str());
					msg = szTmp;
				}
				else if (!bSendNotification && bRecoveryMessage)
				{
					bSendNotification = true;
					msg = recoverymsg;
					std::string clearstr = "!";
					CustomRecoveryMessage(itt->ID, clearstr, true);
				}
				else
				{
					bSendNotification = false;
				}
			}
			if (bSendNotification)
			{
				if (bCustomMessage && !bRecoveryMessage)
					msg = ParseCustomMessage(custommsg, devicename, pvalue);
				SendMessageEx(Idx, devicename, itt->ActiveSystems, msg, msg, szExtraData, itt->Priority, std::string(""), true);
				if (!bRecoveryMessage)
				{
					TouchNotification(itt->ID);
					CustomRecoveryMessage(itt->ID, msg, true);
				}
			}
		}
	}
	return true;
}

bool CNotificationHelper::CheckAndHandleSwitchNotification(
	const uint64_t Idx,
	const std::string &devicename,
	const _eNotificationTypes ntype)
{
	std::vector<_tNotification> notifications = GetNotifications(Idx);
	if (notifications.size() == 0)
		return false;

	std::vector<std::vector<std::string> > result;

	result = m_sql.safe_query("SELECT SwitchType, CustomImage FROM DeviceStatus WHERE (ID=%" PRIu64 ")",
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
					case STYPE_DoorContact:
						notValue = "Open";
						szExtraData += "Image=door48open|";
						break;
					case STYPE_DoorLock:
						notValue = "Locked";
						szExtraData += "Image=door48|";
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
					case STYPE_DoorContact:
					case STYPE_Contact:
						notValue = "Closed";
						break;
					case STYPE_DoorLock:
						notValue = "Unlocked";
						szExtraData += "Image=door48open|";
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
				SendMessageEx(Idx, devicename, itt->ActiveSystems, msg, msg, szExtraData, itt->Priority, std::string(""), true);
				TouchNotification(itt->ID);
			}
		}
	}
	return true;
}

bool CNotificationHelper::CheckAndHandleSwitchNotification(
	const uint64_t Idx,
	const std::string &devicename,
	const _eNotificationTypes ntype,
	const int llevel)
{
	std::vector<_tNotification> notifications = GetNotifications(Idx);
	if (notifications.size() == 0)
		return false;
	std::vector<std::vector<std::string> > result;

	result = m_sql.safe_query("SELECT SwitchType, CustomImage, Options FROM DeviceStatus WHERE (ID=%" PRIu64 ")",
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
				SendMessageEx(Idx, devicename, itt->ActiveSystems, msg, msg, szExtraData, itt->Priority, std::string(""), true);
				TouchNotification(itt->ID);
			}
		}
	}
	return true;
}

bool CNotificationHelper::CheckAndHandleRainNotification(
	const uint64_t Idx,
	const std::string &devicename,
	const unsigned char devType,
	const unsigned char subType,
	const _eNotificationTypes ntype,
	const float mvalue)
{
	std::vector<std::vector<std::string> > result;

	result = m_sql.safe_query("SELECT AddjValue,AddjMulti FROM DeviceStatus WHERE (ID=%" PRIu64 ")",
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
//GB3:	Use a midday hour to avoid a clash with possible DST jump
	ltime.tm_hour=14;
	ltime.tm_min = 0;
	ltime.tm_sec = 0;
	ltime.tm_year = tm1.tm_year;
	ltime.tm_mon = tm1.tm_mon;
	ltime.tm_mday = tm1.tm_mday;
	sprintf(szDateEnd, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

	if (subType != sTypeRAINWU)
	{
		result = m_sql.safe_query("SELECT MIN(Total) FROM Rain WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')",
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


void CNotificationHelper::CheckAndHandleLastUpdateNotification()
{
	if (m_notifications.size() < 1)
		return;

	time_t atime = mytime(NULL);
	atime -= m_NotificationSensorInterval;
	std::map<uint64_t, std::vector<_tNotification> >::const_iterator itt;

	for (itt = m_notifications.begin(); itt != m_notifications.end(); ++itt)
	{
		std::vector<_tNotification>::const_iterator itt2;
		for (itt2 = itt->second.begin(); itt2 != itt->second.end(); ++itt2)
		{
			if (((atime >= itt2->LastSend) || (itt2->SendAlways) || (!itt2->CustomMessage.empty())) && (itt2->LastUpdate)) //emergency always goes true
			{
				std::vector<std::string> splitresults;
				StringSplit(itt2->Params, ";", splitresults);
				if (splitresults.size() < 3)
					continue;
				std::string ttype = Notification_Type_Desc(NTYPE_LASTUPDATE, 1);
				if (splitresults[0] == ttype)
				{
					std::string recoverymsg;
					bool bRecoveryMessage = false;
					bRecoveryMessage = CustomRecoveryMessage(itt2->ID, recoverymsg, true);
					if ((atime < itt2->LastSend) && (!itt2->SendAlways) && (!bRecoveryMessage))
						continue;
					extern time_t m_StartTime;
					time_t btime = mytime(NULL);
					std::string msg;
					std::string szExtraData;
					std::string custommsg;
					uint64_t Idx = itt->first;
					uint32_t SensorTimeOut = static_cast<uint32_t>(atoi(splitresults[2].c_str()));  // minutes
					uint32_t diff = static_cast<uint32_t>(round(difftime(btime, itt2->LastUpdate)));
					bool bStartTime = (difftime(btime, m_StartTime) < SensorTimeOut * 60);
					bool bSendNotification = ApplyRule(splitresults[1], (diff == SensorTimeOut * 60), (diff < SensorTimeOut * 60));
					bool bCustomMessage = false;
					bCustomMessage = CustomRecoveryMessage(itt2->ID, custommsg, false);

					if (bSendNotification && !bStartTime && (!bRecoveryMessage || itt2->SendAlways))
					{
						if (SystemUptime() < SensorTimeOut * 60 && (!bRecoveryMessage || itt2->SendAlways))
							continue;
						std::vector<std::vector<std::string> > result;
						result = m_sql.safe_query("SELECT SwitchType FROM DeviceStatus WHERE (ID=%" PRIu64 ")", Idx);
						if (result.size() == 0)
							continue;
						szExtraData = "|Name=" + itt2->DeviceName + "|SwitchType=" + result[0][0] + "|";
						std::string ltype = Notification_Type_Desc(NTYPE_LASTUPDATE, 0);
						std::string label = Notification_Type_Label(NTYPE_LASTUPDATE);
						char szDate[50];
						char szTmp[300];
						struct tm ltime;
						localtime_r(&itt2->LastUpdate,&ltime);
						sprintf(szDate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday,
							ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
						sprintf(szTmp,"Sensor %s %s: %s [%s %d %s]", itt2->DeviceName.c_str(), ltype.c_str(), szDate,
							splitresults[1].c_str(), SensorTimeOut, label.c_str());
						msg = szTmp;
					}
					else if (!bSendNotification && bRecoveryMessage)
					{
						msg = recoverymsg;
						std::string clearstr = "!";
						CustomRecoveryMessage(itt2->ID, clearstr, true);
					}
					else
						continue;

					if (bCustomMessage && !bRecoveryMessage)
						msg = ParseCustomMessage(custommsg, itt2->DeviceName, "");
					SendMessageEx(Idx, itt2->DeviceName, itt2->ActiveSystems, msg, msg, szExtraData, itt2->Priority, std::string(""), true);
					if (!bRecoveryMessage)
					{
						TouchNotification(itt2->ID);
						CustomRecoveryMessage(itt2->ID, msg, true);
					}
				}
			}
		}
	}
}

void CNotificationHelper::TouchNotification(const uint64_t ID)
{
	char szDate[50];
	time_t atime = mytime(NULL);
	struct tm ltime;
	localtime_r(&atime, &ltime);
	sprintf(szDate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);

	//Set LastSend date
	m_sql.safe_query("UPDATE Notifications SET LastSend='%q' WHERE (ID=%" PRIu64 ")",
		szDate, ID);

	//Also touch it internally
	boost::lock_guard<boost::mutex> l(m_mutex);

	std::map<uint64_t, std::vector<_tNotification> >::iterator itt;
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

void CNotificationHelper::TouchLastUpdate(const uint64_t ID)
{
	time_t atime = mytime(NULL);
	boost::lock_guard<boost::mutex> l(m_mutex);

	std::map<uint64_t, std::vector<_tNotification> >::iterator itt;
	for (itt = m_notifications.begin(); itt != m_notifications.end(); ++itt)
	{
		std::vector<_tNotification>::iterator itt2;
		for (itt2 = itt->second.begin(); itt2 != itt->second.end(); ++itt2)
		{
			if (itt2->ID == ID)
			{
				itt2->LastUpdate = atime;
				return;
			}
		}
	}
}

bool CNotificationHelper::CustomRecoveryMessage(const uint64_t ID, std::string &msg, const bool isRecovery)
{
	boost::lock_guard<boost::mutex> l(m_mutex);

	std::map<uint64_t, std::vector<_tNotification> >::iterator itt;
	for (itt = m_notifications.begin(); itt != m_notifications.end(); ++itt)
	{
		std::vector<_tNotification>::iterator itt2;
		for (itt2 = itt->second.begin(); itt2 != itt->second.end(); ++itt2)
		{
			if (itt2->ID == ID)
			{
				std::vector<std::string> splitresults;
				if (isRecovery)
				{
					StringSplit(itt2->Params, ";", splitresults);
					if (splitresults.size() < 4)
						return false;
					if (splitresults[3] != "1")
						return false;
				}

				std::string szTmp;
				StringSplit(itt2->CustomMessage, ";;", splitresults);
				if (msg.empty())
				{
					if (splitresults.size() > 0)
					{
						if (!splitresults[0].empty() && !isRecovery)
						{
							szTmp = splitresults[0];
							msg = szTmp;
							return true;
						}
						if (splitresults.size() > 1)
						{
							if (!splitresults[1].empty() && isRecovery)
							{
								szTmp = splitresults[1];
								msg = szTmp;
								return true;
							}
						}
					}
					return false;
				}
				if (!isRecovery)
					return false;

				if (splitresults.size() > 0)
				{
					if (!splitresults[0].empty())
						szTmp = splitresults[0];
				}
				if ((msg.find("!") != 0) && (msg.size() > 1))
				{
					szTmp.append(";;[Recovered] ");
					szTmp.append(msg);
				}
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT ID FROM Notifications WHERE (ID=='%" PRIu64 "') AND (Params=='%q')", itt2->ID, itt2->Params.c_str());
				if (result.size() == 0)
					return false;

				m_sql.safe_query("UPDATE Notifications SET CustomMessage='%q' WHERE ID=='%" PRIu64 "'", szTmp.c_str(), itt2->ID);
				itt2->CustomMessage = szTmp;
				return true;
			}
		}
	}
	return false;
}

bool CNotificationHelper::AddNotification(
	const std::string &DevIdx,
	const std::string &Param,
	const std::string &CustomMessage,
	const std::string &ActiveSystems,
	const int Priority,
	const bool SendAlways
	)
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
	uint64_t idxll;
	s_str >> idxll;
	return GetNotifications(idxll);
}

std::vector<_tNotification> CNotificationHelper::GetNotifications(const uint64_t DevIdx)
{
	boost::lock_guard<boost::mutex> l(m_mutex);
	std::vector<_tNotification> ret;
	std::map<uint64_t, std::vector<_tNotification> >::const_iterator itt = m_notifications.find(DevIdx);
	if (itt != m_notifications.end())
	{
		ret = itt->second;
	}
	return ret;
}

bool CNotificationHelper::HasNotifications(const std::string &DevIdx)
{
	std::stringstream s_str(DevIdx);
	uint64_t idxll;
	s_str >> idxll;
	return HasNotifications(idxll);
}

bool CNotificationHelper::HasNotifications(const uint64_t DevIdx)
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
	std::vector<std::string> splitresults;

	std::stringstream sstr;

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt = result.begin(); itt != result.end(); ++itt)
	{
		std::vector<std::string> sd = *itt;

		_tNotification notification;
		uint64_t Idx;

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
			ParseSQLdatetime(notification.LastSend, ntime, stime, atime.tm_isdst);
		}
		std::string ttype = Notification_Type_Desc(NTYPE_LASTUPDATE, 1);
		StringSplit(notification.Params, ";", splitresults);
		if (splitresults[0] == ttype) {
			std::vector<std::vector<std::string> > result2;
			result2 = m_sql.safe_query(
				"SELECT B.Name, B.LastUpdate "
				"FROM Notifications AS A "
				"LEFT OUTER JOIN DeviceStatus AS B "
				"ON A.DeviceRowID=B.ID "
				"WHERE (A.Params LIKE '%q%%') "
				"AND (A.ID=='%" PRIu64 "') "
				"LIMIT 1",
				ttype.c_str(), notification.ID
				);
			if (result2.size() == 1) {
				struct tm ntime;
				notification.DeviceName = result2[0][0];
				std::string stime = result2[0][1];
				ParseSQLdatetime(notification.LastUpdate, ntime, stime, atime.tm_isdst);
			}
		}
		m_notifications[Idx].push_back(notification);
	}
}
