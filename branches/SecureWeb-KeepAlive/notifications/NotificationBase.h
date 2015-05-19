#pragma once
#include "../webserver/cWebem.h"

#define OPTIONS_NONE 0
#define OPTIONS_URL_SUBJECT 1
#define OPTIONS_URL_BODY 2
#define OPTIONS_HTML_SUBJECT 4
#define OPTIONS_HTML_BODY 8
#define OPTIONS_URL_PARAMS 16

class CNotificationBase {
	friend class CNotificationHelper;
protected:
	CNotificationBase(const std::string &subsystemid, const int options = OPTIONS_NONE);
	~CNotificationBase();
	bool SendMessage(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const bool bFromNotification);
	bool SendMessageEx(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string &Sound, const bool bFromNotification);
	void SetConfigValue(const std::string &Key, const std::string &Value);
	std::string GetSubsystemId();
	bool IsInConfig(const std::string &Key);
	bool IsInConfigString(const std::string &Key);
	bool IsInConfigInt(const std::string &Key);
	bool IsInConfigBase64(const std::string &Key);
	void ConfigFromGetvars(http::server::cWebem *webEm, const bool save);
	virtual bool IsConfigured() = 0;
	void SetupConfig(const std::string &Key, std::string& Value);
	void SetupConfig(const std::string &Key, int *Value);
	void SetupConfigBase64(const std::string &Key, std::string& Value);
	virtual bool SendMessageImplementation(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string &Sound, const bool bFromNotification) = 0;
	void LoadConfig();
	std::string MakeHtml(const std::string &txt);

	int m_IsEnabled;
private:
	std::string _subsystemid;
	std::map<std::string, std::string* > _configValues;
	std::map<std::string, std::string* > _configValuesBase64;
	std::map<std::string, int* > _configValuesInt;
	int _options;
};
