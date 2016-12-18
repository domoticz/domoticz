#pragma once

#include <deque>
#include <list>
#include <string>
#include <fstream>

enum _eLogLevel
{
	LOG_NORM=0,
	LOG_ERROR,
	LOG_STATUS,
};

enum _eLogFileVerboseLevel
{
	VBL_ALL=0,
	VBL_STATUS_ERROR,
	VBL_ERROR,
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
	void LogNoLF(const _eLogLevel level, const char* logline, ...);

	void LogSequenceStart();
	void LogSequenceAdd(const char* logline);
	void LogSequenceAddNoLF(const char* logline);
	void LogSequenceEnd(const _eLogLevel level);

	void EnableLogTimestamps(const bool bEnableTimestamps);

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
	bool m_bEnableErrorsToNotificationSystem;
	time_t m_LastLogNotificationsSend;
	std::stringstream m_sequencestring;
	_eLogFileVerboseLevel m_verbose_level;
};
extern CLogger _log;
