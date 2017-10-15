#pragma once

#include <deque>
#include <list>
#include <string>
#include <fstream>

enum _eLogLevel
{
	LOG_ERROR = 0,
	LOG_STATUS = 1,
	LOG_NORM = 2,
	LOG_TRACE = 3
};

enum _eLogFileVerboseLevel
{
	VBL_ERROR = 0,
	VBL_STATUS_ERROR = 1,
	VBL_ALL = 2,
	VBL_TRACE,

};

class CLogger
{
public:
	struct _tLogLineStruct
	{
		time_t logtime;
		_eLogLevel level;
		std::string logmessage;
		_tLogLineStruct(const _eLogLevel nlevel, const std::string &nlogmessage);
	};

	CLogger(void);
	~CLogger(void);

	void SetOutputFile(const char *OutputFile);
	void SetVerboseLevel(_eLogFileVerboseLevel vLevel);

	void Log(const _eLogLevel level, const char* logline, ...);

	void LogSequenceStart();
	void LogSequenceAdd(const char* logline);
	void LogSequenceAddNoLF(const char* logline);
	void LogSequenceEnd(const _eLogLevel level);

	void EnableLogTimestamps(const bool bEnableTimestamps);
	bool IsLogTimestampsEnabled();

	void EnableLogThreadIDs(const bool bEnableThreadIDs);
	bool IsLogThreadIDsEnabled();

	void SetFilterString(std::string &Filter);
	bool isTraceEnabled();
	bool TestFilter(const char *cbuffer);
	void setLogVerboseLevel(int LogLevel);
	void SetLogPreference(std::string LogFilter, std::string LogFileName, std::string LogLevel);
	void GetLogPreference();
	void SetLogDebug(bool debug);
	bool GetLogDebug();
	void ForwardErrorsToNotificationSystem(const bool bDoForward);

	std::list<_tLogLineStruct> GetLog(const _eLogLevel lType);
	void ClearLog();

	std::list<_tLogLineStruct> GetNotificationLogs();
	bool NotificationLogsEnabled();
private:
	boost::mutex m_mutex;
	std::ofstream m_outputfile;
	std::deque<_tLogLineStruct> m_lastlog;
	std::deque<_tLogLineStruct> m_last_status_log;
	std::deque<_tLogLineStruct> m_last_error_log;
	std::deque<_tLogLineStruct> m_notification_log;
	bool m_bInSequenceMode;
	bool m_bEnableLogTimestamps;
	bool m_bEnableLogThreadIDs;
	bool m_bEnableErrorsToNotificationSystem;
	time_t m_LastLogNotificationsSend;
	std::stringstream m_sequencestring;
	std::string FilterString;
	std::vector<std::string> FilterStringList;	//contain the list of filtered words
	std::vector<std::string> KeepStringList;	//contain the list of  words to be kept
	_eLogFileVerboseLevel m_verbose_level;
	bool m_debug;
};
extern CLogger _log;
