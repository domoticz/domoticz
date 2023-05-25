#pragma once

#include <deque>
#include <list>
#include <string>
#include <fstream>

enum _eLogLevel : uint32_t
{
	LOG_NORM = 0x0000001,
	LOG_STATUS = 0x0000002,
	LOG_ERROR = 0x0000004,
	LOG_DEBUG_INT = 0x0000008, // do not use directly, use _log.Debug(...) instead
	//
	LOG_ALL = 0xFFFFFFF // Used by web interface to retrieve all log types
};
enum _eDebugLevel : uint32_t
{
	DEBUG_NORM = 0x0000001,
	DEBUG_HARDWARE = 0x0000002,
	DEBUG_RECEIVED = 0x0000004,
	DEBUG_WEBSERVER = 0x0000008,
	DEBUG_EVENTSYSTEM = 0x0000010,
	DEBUG_PYTHON = 0x0000020,
	DEBUG_THREADIDS = 0x0000040,
	DEBUG_SQL = 0x0000080,
	DEBUG_AUTH = 0x0000100,
	//
	DEBUG_ALL = 0xFFFFFFF
};
enum _eLogACLF : uint8_t
{
	LOG_ACLF_ENABLED = 0x01,
	LOG_ACLF_FILE = 0x02,
	LOG_ACLF_SYSLOG = 0x04
};

class CLogger
{
      public:
	struct _tLogLineStruct
	{
		time_t logtime;
		_eLogLevel level;
		std::string logmessage;
		_tLogLineStruct(_eLogLevel nlevel, const std::string &nlogmessage);
	};

	CLogger();
	~CLogger();

	bool SetLogFlags(const std::string &sFlags);
	void SetLogFlags(const uint32_t iFlags);
	bool IsLogLevelEnabled(const _eLogLevel level);

	bool SetDebugFlags(const std::string &sFlags);
	void SetDebugFlags(const uint32_t iFlags);
	bool IsDebugLevelEnabled(const _eDebugLevel level);

	void SetACLFlogFlags(const uint8_t iFlags);
	bool IsACLFlogEnabled();

	void SetOutputFile(const char *OutputFile);
	void SetACLFOutputFile(const char *OutputFile);
	void OpenACLFOutputFile();

	void Log(_eLogLevel level, const std::string &sLogline);
	void Log(_eLogLevel level, const char *logline, ...)
#ifdef __GNUC__
		__attribute__((format(printf, 3, 4)))
#endif
		;
	void Debug(_eDebugLevel level, const std::string &sLogline);
	void Debug(_eDebugLevel level, const char *logline, ...)
#ifdef __GNUC__
		__attribute__((format(printf, 3, 4)))
#endif
		;
	void ACLFlog(const char *logline, ...)
#ifdef __GNUC__
		__attribute__((format(printf, 2, 3)))
#endif
		;

	void LogSequenceStart();
	void LogSequenceAdd(const char *logline);
	void LogSequenceAddNoLF(const char *logline);
	void LogSequenceEnd(_eLogLevel level);

	void EnableLogTimestamps(bool bEnableTimestamps);
	bool IsLogTimestampsEnabled();

	void ForwardErrorsToNotificationSystem(bool bDoForward);

	std::list<_tLogLineStruct> GetLog(_eLogLevel level, time_t lastlogtime = 0);
	void ClearLog();

	std::list<_tLogLineStruct> GetNotificationLogs();
	bool NotificationLogsEnabled();

      private:
	uint32_t m_log_flags = 0;
	uint32_t m_debug_flags = 0;
	uint8_t m_aclf_flags = 0;
	uint32_t m_aclf_loggedlinescnt = 0;

	std::mutex m_mutex;
	std::ofstream m_outputfile;
	const char *m_aclflogfile = nullptr;
	std::ofstream m_aclfoutputfile;
	std::map<_eLogLevel, std::deque<_tLogLineStruct>> m_lastlog;
	std::deque<_tLogLineStruct> m_notification_log;
	bool m_bInSequenceMode;
	bool m_bEnableLogTimestamps;
	bool m_bEnableLogThreadIDs;
	bool m_bEnableErrorsToNotificationSystem;
	time_t m_LastLogNotificationsSend;
	std::stringstream m_sequencestring;
};
extern CLogger _log;
