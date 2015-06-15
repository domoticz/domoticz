#include "stdafx.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "../hardware/hardwaretypes.h"
#include "NotificationHelper.h"
#include "NotificationProwl.h"
#include "NotificationNma.h"
#include "NotificationPushover.h"
#include "NotificationPushalot.h"
#include "NotificationEmail.h"
#include "NotificationSMS.h"
#include "NotificationHTTP.h"
#include "NotificationKodi.h"
#include <map>

#if defined WIN32
	#include "../msbuild/WindowsHelper.h"
#endif

typedef std::map<std::string, CNotificationBase*>::iterator it_noti_type;

CNotificationHelper::CNotificationHelper()
{
	AddNotifier(new CNotificationProwl());
	AddNotifier(new CNotificationNma());
	AddNotifier(new CNotificationPushover());
	AddNotifier(new CNotificationPushalot());
	AddNotifier(new CNotificationEmail());
	AddNotifier(new CNotificationSMS());
	AddNotifier(new CNotificationHTTP());
	AddNotifier(new CNotificationKodi());
	/* more notifiers can be added here */
}

CNotificationHelper::~CNotificationHelper()
{
	for (it_noti_type iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter) {
		delete iter->second;
	}
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

void CNotificationHelper::ConfigFromGetvars(http::server::cWebem *webEm, const bool save)
{
	for (it_noti_type iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter) {
		iter->second->ConfigFromGetvars(webEm, save);
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

bool CNotificationHelper::CheckAndHandleTempHumidityNotification(
	const int HardwareID,
	const std::string &ID,
	const unsigned char unit,
	const unsigned char devType,
	const unsigned char subType,
	const float temp,
	const int humidity,
	const bool bHaveTemp,
	const bool bHaveHumidity)
{
	char szTmp[600];

	unsigned long long ulID = 0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp, "SELECT ID, Name FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)", HardwareID, ID.c_str(), unit, devType, subType);
	result = m_sql.query(szTmp);
	if (result.size() == 0)
		return false;
	std::stringstream s_str(result[0][0]);
	s_str >> ulID;
	std::string devicename = result[0][1];
	std::string szExtraData = "|Name=" + devicename + "|";
	std::vector<_tNotification> notifications = GetNotifications(ulID);
	if (notifications.size() == 0)
		return false;

	time_t atime = mytime(NULL);

	//check if not sent 12 hours ago, and if applicable

	int nNotificationInterval = 12 * 3600;
	m_sql.GetPreferencesVar("NotificationSensorInterval", nNotificationInterval);
	atime -= nNotificationInterval;

	std::string msg = "";

	std::string signtemp = Notification_Type_Desc(NTYPE_TEMPERATURE, 1);
	std::string signhum = Notification_Type_Desc(NTYPE_HUMIDITY, 1);

	std::vector<_tNotification>::const_iterator itt;
	for (itt = notifications.begin(); itt != notifications.end(); ++itt)
	{
		if ((atime >= itt->LastSend) || (itt->Priority>1)) //emergency always goes true
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
			}
			else if ((ntype == signhum) && (bHaveHumidity))
			{
				//humanity
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
			}
			if (bSendNotification)
			{
				if (!itt->CustomMessage.empty())
					msg = itt->CustomMessage;
				SendMessageEx(itt->ActiveSystems, msg, msg, szExtraData, itt->Priority, std::string(""), true);
				TouchNotification(itt->ID);
			}
		}
	}
	return true;
}

bool CNotificationHelper::CheckAndHandleDewPointNotification(
	const int HardwareID,
	const std::string &ID,
	const unsigned char unit,
	const unsigned char devType,
	const unsigned char subType,
	const float temp,
	const float dewpoint)
{
	char szTmp[600];

	unsigned long long ulID = 0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp, "SELECT ID, Name FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)", HardwareID, ID.c_str(), unit, devType, subType);
	result = m_sql.query(szTmp);
	if (result.size() == 0)
		return false;
	std::stringstream s_str(result[0][0]);
	s_str >> ulID;
	std::string devicename = result[0][1];
	std::string szExtraData = "|Name=" + devicename + "|Image=temp-0-5|";
	std::vector<_tNotification> notifications = GetNotifications(ulID);
	if (notifications.size() == 0)
		return false;

	time_t atime = mytime(NULL);

	//check if not sent 12 hours ago, and if applicable

	int nNotificationInterval = 12 * 3600;
	m_sql.GetPreferencesVar("NotificationSensorInterval", nNotificationInterval);
	atime -= nNotificationInterval;

	std::string msg = "";

	std::string signdewpoint = Notification_Type_Desc(NTYPE_DEWPOINT, 1);

	std::vector<_tNotification>::const_iterator itt;
	for (itt = notifications.begin(); itt != notifications.end(); ++itt)
	{
		if ((atime >= itt->LastSend) || (itt->Priority>1)) //emergency always goes true
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
				}
			}
			if (bSendNotification)
			{
				if (!itt->CustomMessage.empty())
					msg = itt->CustomMessage;
				SendMessageEx(itt->ActiveSystems, msg, msg, szExtraData, itt->Priority, std::string(""), true);
				TouchNotification(itt->ID);
			}
		}
	}
	return true;
}

bool CNotificationHelper::CheckAndHandleAmpere123Notification(
	const int HardwareID,
	const std::string &ID,
	const unsigned char unit,
	const unsigned char devType,
	const unsigned char subType,
	const float Ampere1,
	const float Ampere2,
	const float Ampere3)
{
	char szTmp[600];

	unsigned long long ulID = 0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp, "SELECT ID, Name FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)", HardwareID, ID.c_str(), unit, devType, subType);
	result = m_sql.query(szTmp);
	if (result.size() == 0)
		return false;
	std::stringstream s_str(result[0][0]);
	s_str >> ulID;
	std::string devicename = result[0][1];
	std::string szExtraData = "|Name=" + devicename + "|Image=current48|";
	std::vector<_tNotification> notifications = GetNotifications(ulID);
	if (notifications.size() == 0)
		return false;

	time_t atime = mytime(NULL);

	//check if not sent 12 hours ago, and if applicable

	int nNotificationInterval = 12 * 3600;
	m_sql.GetPreferencesVar("NotificationSensorInterval", nNotificationInterval);
	atime -= nNotificationInterval;

	std::string msg = "";

	std::string signamp1 = Notification_Type_Desc(NTYPE_AMPERE1, 1);
	std::string signamp2 = Notification_Type_Desc(NTYPE_AMPERE2, 2);
	std::string signamp3 = Notification_Type_Desc(NTYPE_AMPERE3, 3);

	std::vector<_tNotification>::const_iterator itt;
	for (itt = notifications.begin(); itt != notifications.end(); ++itt)
	{
		if ((atime >= itt->LastSend) || (itt->Priority>1)) //emergency always goes true
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
			}
			if (bSendNotification)
			{
				if (!itt->CustomMessage.empty())
					msg = itt->CustomMessage;
				SendMessageEx(itt->ActiveSystems, msg, msg, szExtraData, itt->Priority, std::string(""), true);
				TouchNotification(itt->ID);
			}
		}
	}
	return true;
}

bool CNotificationHelper::CheckAndHandleNotification(
	const int HardwareID,
	const std::string &ID,
	const unsigned char unit,
	const unsigned char devType,
	const unsigned char subType,
	const _eNotificationTypes ntype,
	const float mvalue)
{
	char szTmp[600];

	unsigned long long ulID = 0;
	double intpart;
	std::string pvalue;
	if (modf(mvalue, &intpart) == 0)
		sprintf(szTmp, "%.0f", mvalue);
	else
		sprintf(szTmp, "%.1f", mvalue);
	pvalue = szTmp;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp, "SELECT ID, Name, SwitchType FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)", HardwareID, ID.c_str(), unit, devType, subType);
	result = m_sql.query(szTmp);
	if (result.size() == 0)
		return false;
	std::stringstream s_str(result[0][0]);
	s_str >> ulID;
	std::string devicename = result[0][1];
	std::string szExtraData = "|Name=" + devicename + "|SwitchType=" + result[0][2] + "|";
	std::vector<_tNotification> notifications = GetNotifications(ulID);
	if (notifications.size() == 0)
		return false;

	time_t atime = mytime(NULL);

	//check if not sent 12 hours ago, and if applicable

	int nNotificationInterval = 12 * 3600;
	m_sql.GetPreferencesVar("NotificationSensorInterval", nNotificationInterval);
	atime -= nNotificationInterval;

	std::string msg = "";

	std::string ltype = Notification_Type_Desc(ntype, 0);
	std::string nsign = Notification_Type_Desc(ntype, 1);
	std::string label = Notification_Type_Label(ntype);

	std::vector<_tNotification>::const_iterator itt;
	for (itt = notifications.begin(); itt != notifications.end(); ++itt)
	{
		if ((atime >= itt->LastSend) || (itt->Priority>1)) //emergency always goes true
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
					msg = itt->CustomMessage;
				SendMessageEx(itt->ActiveSystems, msg, msg, szExtraData, itt->Priority, std::string(""), true);
				TouchNotification(itt->ID);
			}
		}
	}
	return true;
}

bool CNotificationHelper::CheckAndHandleSwitchNotification(
	const int HardwareID,
	const std::string &ID,
	const unsigned char unit,
	const unsigned char devType,
	const unsigned char subType,
	const _eNotificationTypes ntype)
{
	char szTmp[600];

	unsigned long long ulID = 0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp, "SELECT ID, Name, SwitchType, CustomImage FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)", HardwareID, ID.c_str(), unit, devType, subType);
	result = m_sql.query(szTmp);
	if (result.size() == 0)
		return false;
	std::stringstream s_str(result[0][0]);
	s_str >> ulID;
	std::string devicename = result[0][1];
	_eSwitchType switchtype = (_eSwitchType)atoi(result[0][2].c_str());
	std::string szExtraData = "|Name=" + devicename + "|SwitchType=" + result[0][2] + "|CustomImage=" + result[0][3] + "|";
	std::vector<_tNotification> notifications = GetNotifications(ulID);
	if (notifications.size() == 0)
		return false;

	std::string msg = "";

	std::string ltype = Notification_Type_Desc(ntype, 1);

	time_t atime = mytime(NULL);
	int nNotificationInterval = 0;
	m_sql.GetPreferencesVar("NotificationSwitchInterval", nNotificationInterval);
	atime -= nNotificationInterval;

	std::vector<_tNotification>::const_iterator itt;
	for (itt = notifications.begin(); itt != notifications.end(); ++itt)
	{
		if ((atime >= itt->LastSend) || (itt->Priority>1)) //emergency always goes true
		{
			std::vector<std::string> splitresults;
			StringSplit(itt->Params, ";", splitresults);
			if (splitresults.size() < 1)
				continue; //impossible
			std::string atype = splitresults[0];

			bool bSendNotification = false;

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
						msg += " pressed";
						break;
					case STYPE_Contact:
						msg += " Open";
						szExtraData += "Image=contact48_open|";
						break;
					case STYPE_DoorLock:
						msg += " Open";
						szExtraData += "Image=door48open|";
						break;
					case STYPE_Motion:
						msg += " movement detected";
						break;
					case STYPE_SMOKEDETECTOR:
						msg += " ALARM/FIRE !";
						break;
					default:
						msg += " >> ON";
						break;
					}

				}
				else {
					szExtraData += "Status=Off|";
					switch (switchtype)
					{
					case STYPE_DoorLock:
					case STYPE_Contact:
						msg += " Closed";
						break;
					default:
						msg += " >> OFF";
						break;
					}
				}
			}
			if (bSendNotification)
			{
				if (!itt->CustomMessage.empty())
					msg = itt->CustomMessage;
				SendMessageEx(itt->ActiveSystems, msg, msg, szExtraData, itt->Priority, std::string(""), true);
				TouchNotification(itt->ID);
			}
		}
	}
	return true;
}

bool CNotificationHelper::CheckAndHandleRainNotification(
	const int HardwareID,
	const std::string &ID,
	const unsigned char unit,
	const unsigned char devType,
	const unsigned char subType,
	const _eNotificationTypes ntype,
	const float mvalue)
{
	char szTmp[600];

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp, "SELECT ID,AddjValue,AddjMulti FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)", HardwareID, ID.c_str(), unit, devType, subType);
	result = m_sql.query(szTmp);
	if (result.size() == 0)
		return false;
	std::string devidx = result[0][0];
	double AddjValue = atof(result[0][1].c_str());
	double AddjMulti = atof(result[0][2].c_str());

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
		std::stringstream szQuery;
		szQuery << "SELECT MIN(Total) FROM Rain WHERE (DeviceRowID=" << devidx << " AND Date>='" << szDateEnd << "')";
		result = m_sql.query(szQuery.str());
		if (result.size() > 0)
		{
			std::vector<std::string> sd = result[0];

			float total_min = static_cast<float>(atof(sd[0].c_str()));
			float total_max = mvalue;
			double total_real = total_max - total_min;
			total_real *= AddjMulti;

			CheckAndHandleNotification(HardwareID, ID, unit, devType, subType, NTYPE_RAIN, (float)total_real);
		}
	}
	else
	{
		//value is already total rain
		double total_real = mvalue;
		total_real *= AddjMulti;
		CheckAndHandleNotification(HardwareID, ID, unit, devType, subType, NTYPE_RAIN, (float)total_real);
	}
	return false;
}

void CNotificationHelper::TouchNotification(const unsigned long long ID)
{
	char szTmp[300];
	char szDate[100];
	time_t atime = mytime(NULL);
	struct tm ltime;
	localtime_r(&atime, &ltime);
	sprintf(szDate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);

	//Set LastSend date
	sprintf(szTmp,
		"UPDATE Notifications SET LastSend='%s' WHERE (ID = %llu)", szDate, ID);
	m_sql.query(szTmp);
}

bool CNotificationHelper::AddNotification(const std::string &DevIdx, const std::string &Param, const std::string &CustomMessage, const std::string &ActiveSystems, const int Priority)
{
	std::vector<std::vector<std::string> > result;

	std::stringstream szQuery;

	//First check for duplicate, because we do not want this
	szQuery << "SELECT ROWID FROM Notifications WHERE (DeviceRowID==" << DevIdx << ") AND (Params=='" << Param << "')";
	result = m_sql.query(szQuery.str());
	if (result.size() > 0)
		return false;//already there!

	szQuery.clear();
	szQuery.str("");
	szQuery << "INSERT INTO Notifications (DeviceRowID, Params, CustomMessage, ActiveSystems, Priority) VALUES ('" << DevIdx << "','" << Param << "','" << CustomMessage << "','" << ActiveSystems << "','" << Priority << "')";
	result = m_sql.query(szQuery.str());
	return true;
}

bool CNotificationHelper::RemoveDeviceNotifications(const std::string &DevIdx)
{
	std::vector<std::vector<std::string> > result;

	std::stringstream szQuery;
	szQuery.clear();
	szQuery.str("");
	szQuery << "DELETE FROM Notifications WHERE (DeviceRowID==" << DevIdx << ")";
	result = m_sql.query(szQuery.str());
	return true;
}

bool CNotificationHelper::RemoveNotification(const std::string &ID)
{
	std::vector<std::vector<std::string> > result;

	std::stringstream szQuery;
	szQuery.clear();
	szQuery.str("");
	szQuery << "DELETE FROM Notifications WHERE (ID==" << ID << ")";
	result = m_sql.query(szQuery.str());
	return true;
}


std::vector<_tNotification> CNotificationHelper::GetNotifications(const unsigned long long DevIdx)
{
	std::vector<_tNotification> ret;
	std::vector<std::vector<std::string> > result;

	std::stringstream szQuery;
	szQuery.clear();
	szQuery.str("");
	szQuery << "SELECT ID, Params, CustomMessage, ActiveSystems, Priority, LastSend FROM Notifications WHERE (DeviceRowID==" << DevIdx << ")";
	result = m_sql.query(szQuery.str());
	if (result.size() == 0)
		return ret;

	time_t mtime = mytime(NULL);
	struct tm atime;
	localtime_r(&mtime, &atime);

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt = result.begin(); itt != result.end(); ++itt)
	{
		std::vector<std::string> sd = *itt;

		_tNotification notification;
		std::stringstream s_str(sd[0]);
		s_str >> notification.ID;

		notification.Params = sd[1];
		notification.CustomMessage = sd[2];
		notification.ActiveSystems = sd[3];
		notification.Priority = atoi(sd[4].c_str());

		std::string stime = sd[5];
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

		ret.push_back(notification);
	}
	return ret;
}

std::vector<_tNotification> CNotificationHelper::GetNotifications(const std::string &DevIdx)
{
	std::stringstream s_str(DevIdx);
	unsigned long long idxll;
	s_str >> idxll;
	return GetNotifications(idxll);
}

bool CNotificationHelper::HasNotifications(const unsigned long long DevIdx)
{
	std::vector<std::vector<std::string> > result;

	std::stringstream szQuery;
	szQuery << "SELECT COUNT(*) FROM Notifications WHERE (DeviceRowID==" << DevIdx << ")";
	result = m_sql.query(szQuery.str());
	if (result.size() == 0)
		return false;
	std::vector<std::string> sd = result[0];
	int totnotifications = atoi(sd[0].c_str());
	return (totnotifications > 0);
}
bool CNotificationHelper::HasNotifications(const std::string &DevIdx)
{
	std::stringstream s_str(DevIdx);
	unsigned long long idxll;
	s_str >> idxll;
	return HasNotifications(idxll);
}
