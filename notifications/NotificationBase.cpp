#include "stdafx.h"
#include "NotificationBase.h"
#include "../main/SQLHelper.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../httpclient/UrlEncode.h"
#include "../webserver/Base64.h"

typedef std::map<std::string, std::string* >::iterator it_conf_type;
typedef std::map<std::string, int* >::iterator it_conf_type_int;

using namespace http::server;

CNotificationBase::CNotificationBase(const std::string &subsystemid, const int options):
_subsystemid(subsystemid),
_options(options),
m_IsEnabled(1)
{
}

CNotificationBase::~CNotificationBase()
{
}

void CNotificationBase::SetupConfig(const std::string &Key, std::string& Value)
{
	_configValues[Key] = &Value;
}

void CNotificationBase::SetupConfigBase64(const std::string &Key, std::string& Value)
{
	_configValuesBase64[Key] = &Value;
}

void CNotificationBase::SetupConfig(const std::string &Key, int *Value)
{
	_configValuesInt[Key] = Value;
}

void CNotificationBase::LoadConfig()
{
	for (it_conf_type iter = _configValues.begin(); iter != _configValues.end(); ++iter) {
		std::string Value;
		if (!m_sql.GetPreferencesVar(iter->first, Value)) {
			//_log.Log(LOG_ERROR, std::string(std::string("Subsystem ") + _subsystemid + std::string(", var: ") + iter->first + std::string(": Not Found!")).c_str());
			continue;
		}
		if (_options & OPTIONS_URL_PARAMS) {
			Value = CURLEncode::URLEncode(Value);
		}
		*(iter->second) = Value;
	}
	for (it_conf_type_int iter2 = _configValuesInt.begin(); iter2 != _configValuesInt.end(); ++iter2) {
		int Value;
		if (!m_sql.GetPreferencesVar(iter2->first, Value)) {
			//_log.Log(LOG_ERROR, std::string(std::string("Subsystem ") + _subsystemid + std::string(", var: ") + iter2->first + std::string(": Not Found!")).c_str());
			continue;
		}
		*(iter2->second) = Value;
	}
	for (it_conf_type iter3 = _configValuesBase64.begin(); iter3 != _configValuesBase64.end(); ++iter3) {
		std::string Value;
		if (!m_sql.GetPreferencesVar(iter3->first, Value)) {
			//_log.Log(LOG_ERROR, std::string(std::string("Subsystem ") + _subsystemid + std::string(", var: ") + iter3->first + std::string(": Not Found!")).c_str());
			continue;
		}
		Value = base64_decode(Value);
		if (_options & OPTIONS_URL_PARAMS) {
			Value = CURLEncode::URLEncode(Value);
		}
		*(iter3->second) = Value;
	}
}

bool CNotificationBase::SendMessage(
	const uint64_t Idx,
	const std::string &Name,
	const std::string &Subject,
	const std::string &Text,
	const std::string &ExtraData,
	const bool bFromNotification)
{
	return SendMessageEx(Idx, Name, Subject, Text, std::string(""), 0, std::string(""), bFromNotification);
}

bool CNotificationBase::SendMessageEx(
	const uint64_t Idx,
	const std::string &Name,
	const std::string &Subject,
	const std::string &Text,
	const std::string &ExtraData,
	const int Priority,
	const std::string &Sound,
	const bool bFromNotification)
{
	if (!IsConfigured()) {
		// subsystem not configured, skip
		return false;
	}
	if (bFromNotification)
	{
		if (!m_IsEnabled)
			return true; //not enabled
	}

	std::string fSubject = Subject;
	std::string fText = Text;

	if (_options & OPTIONS_HTML_SUBJECT) {
		fSubject = MakeHtml(Subject);
	}
	if (_options & OPTIONS_HTML_BODY) {
		fText = MakeHtml(Text);
	}
	if (_options & OPTIONS_URL_SUBJECT) {
		fSubject = CURLEncode::URLEncode(fSubject);
	}
	if (_options & OPTIONS_URL_BODY) {
		fText = CURLEncode::URLEncode(fText);
	}

	std::unique_lock<std::mutex> SendMessageEx(SendMessageExMutex);
	bool bRet = SendMessageImplementation(Idx, Name, fSubject, fText, ExtraData, Priority, Sound, bFromNotification);
	if (bRet) {
		_log.Log(LOG_NORM, "Notification sent (%s) => Success", _subsystemid.c_str());
	}
	else {
		_log.Log(LOG_NORM, "Notification sent (%s) => Failed", _subsystemid.c_str());
	}
	return bRet;
}

void CNotificationBase::SetConfigValue(const std::string &Key, const std::string &Value)
{
	for (it_conf_type iter = _configValues.begin(); iter != _configValues.end(); ++iter) {
		std::string fValue = Value;
		if (_options & OPTIONS_URL_PARAMS) {
			fValue = CURLEncode::URLEncode(Value);
		}
		if (Key == iter->first) {
			*(iter->second) = fValue;
		}
	}
	for (it_conf_type_int iter2 = _configValuesInt.begin(); iter2 != _configValuesInt.end(); ++iter2) {
		if (Key == iter2->first) {
			*(iter2->second) = atoi(Value.c_str());
		}
	}
	for (it_conf_type iter3 = _configValuesBase64.begin(); iter3 != _configValuesBase64.end(); ++iter3) {
		std::string fValue = Value;
		if (_options & OPTIONS_URL_PARAMS) {
			fValue = CURLEncode::URLEncode(Value);
		}
		if (Key == iter3->first) {
			*(iter3->second) = fValue;
		}
	}
}

std::string CNotificationBase::GetSubsystemId()
{
	return _subsystemid;
}

void CNotificationBase::ConfigFromGetvars(const request& req, const bool save)
{
	for (it_conf_type iter = _configValues.begin(); iter != _configValues.end(); ++iter) {
		if (request::hasValue(&req, iter->first.c_str())) {
			std::string Value = CURLEncode::URLDecode(std::string(request::findValue(&req, iter->first.c_str())));
			*(iter->second) = Value;
			if (save) {
				m_sql.UpdatePreferencesVar(iter->first, Value);
			}
		}
	}
	for (it_conf_type_int iter2 = _configValuesInt.begin(); iter2 != _configValuesInt.end(); ++iter2) {
		int Value = 0;
		if (request::hasValue(&req, iter2->first.c_str())) {
			std::string sValue = request::findValue(&req, iter2->first.c_str());
			if (sValue == "on")
				Value = 1;
			else if (sValue == "off")
				Value = 0;
			else
				Value=atoi(sValue.c_str());
		}
		*(iter2->second) = Value;
		if (save) {
			m_sql.UpdatePreferencesVar(iter2->first, Value);
		}
	}
	for (it_conf_type iter3 = _configValuesBase64.begin(); iter3 != _configValuesBase64.end(); ++iter3) {
		if (request::hasValue(&req, iter3->first.c_str())) {
			std::string Value = CURLEncode::URLDecode(std::string(request::findValue(&req, iter3->first.c_str())));
			*(iter3->second) = Value;
			if (save) {
				std::string ValueBase64 = base64_encode(Value);
				m_sql.UpdatePreferencesVar(iter3->first, ValueBase64);
			}
		}
	}
}

bool CNotificationBase::IsInConfig(const std::string &Key)
{
	if (IsInConfigString(Key))
		return true;
	if (IsInConfigInt(Key))
		return true;
	if (IsInConfigBase64(Key))
		return true;
	return false;
}

bool CNotificationBase::IsInConfigString(const std::string &Key)
{
	for (it_conf_type iter = _configValues.begin(); iter != _configValues.end(); ++iter) {
		if (Key == iter->first) {
			return true;
		}
	}
	return false;
}

bool CNotificationBase::IsInConfigInt(const std::string &Key)
{
	for (it_conf_type_int iter = _configValuesInt.begin(); iter != _configValuesInt.end(); ++iter) {
		if (Key == iter->first) {
			return true;
		}
	}
	return false;
}

bool CNotificationBase::IsInConfigBase64(const std::string &Key)
{
	for (it_conf_type iter = _configValuesBase64.begin(); iter != _configValuesBase64.end(); ++iter) {
		if (Key == iter->first) {
			return true;
		}
	}
	return false;
}

