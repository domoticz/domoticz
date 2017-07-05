#include "stdafx.h"
#include "Logger.h"
#include <iostream>
#include <stdarg.h>
#include <time.h>
#include "localtime_r.h"
#include "Helper.h"
#include "mainworker.h"
#include "Helper.h"

#ifndef WIN32
#include <syslog.h>
#include <errno.h>
#endif

#include "SQLHelper.h"

#define MAX_LOG_LINE_BUFFER 100
#define MAX_LOG_LINE_LENGTH (2048*3)

extern bool g_bRunAsDaemon;
extern bool g_bUseSyslog;

CLogger::_tLogLineStruct::_tLogLineStruct(const _eLogLevel nlevel, const std::string &nlogmessage)
{
	logtime = mytime(NULL);
	level = nlevel;
	logmessage = nlogmessage;
}

CLogger::CLogger(void)
{
	FilterString = "";
	m_bInSequenceMode = false;
	m_bEnableLogTimestamps = true;
	m_bEnableLogThreadIDs = false;
	m_verbose_level = VBL_ALL;
	m_bEnableErrorsToNotificationSystem = false;
	m_LastLogNotificationsSend = 0;
}

CLogger::~CLogger(void)
{
	if (m_outputfile.is_open())
		m_outputfile.close();
}

void CLogger::SetOutputFile(const char *OutputFile)
{
	boost::unique_lock< boost::mutex > lock(m_mutex);
	if (m_outputfile.is_open())
		m_outputfile.close();

	if (OutputFile == NULL)
		return;
	if (*OutputFile == 0)
		return;

	try
	{
#ifdef _DEBUG
		m_outputfile.open(OutputFile, std::ios::out | std::ios::trunc);
#else
		m_outputfile.open(OutputFile, std::ios::out | std::ios::app);
#endif
	}
	catch (...)
	{
		std::cerr << "Error opening output log file..." << std::endl;
	}
}

void CLogger::SetVerboseLevel(_eLogFileVerboseLevel vLevel)
{
	m_verbose_level = vLevel;
}

void CLogger::ForwardErrorsToNotificationSystem(const bool bDoForward)
{
	m_bEnableErrorsToNotificationSystem = bDoForward;
	if (!bDoForward)
		m_notification_log.clear();
}

void CLogger::Log(const _eLogLevel level, const char* logline, ...)
{
	boost::unique_lock< boost::mutex > lock(m_mutex);

	bool bDoLog = false;
	if (level <= (_eLogLevel)m_verbose_level)
		bDoLog = true;

	if (!bDoLog)
		return;

	va_list argList;
	char cbuffer[MAX_LOG_LINE_LENGTH];
	va_start(argList, logline);
	vsnprintf(cbuffer, sizeof(cbuffer), logline, argList);
	va_end(argList);

	//test if log contain a string to be filtered from LOG content
	if (TestFilter(cbuffer))
		return;

#ifndef WIN32
	if (g_bUseSyslog)
	{
		int sLogLevel = LOG_INFO;
		if (level == LOG_ERROR)
			sLogLevel = LOG_ERR;
		else if (level == LOG_STATUS)
			sLogLevel = LOG_NOTICE;
		syslog(sLogLevel, "%s", cbuffer);
	}
#endif

	std::stringstream sstr;

	if (m_bEnableLogTimestamps)
	{
		char szDate[100];
		struct timeval tv;
		gettimeofday(&tv, NULL);
		struct tm timeinfo;
#ifdef WIN32
		//Thanks to the winsock header file
		time_t tv_sec = tv.tv_sec;
		localtime_r(&tv_sec, &timeinfo);
#else
		localtime_r(&tv.tv_sec, &timeinfo);
#endif
		// create a time stamp string for the log message
		snprintf(szDate, sizeof(szDate), "%04d-%02d-%02d %02d:%02d:%02d.%03d ",
			timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
			timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, (int)tv.tv_usec / 1000);
		sstr << szDate << " ";
	}

	if (m_bEnableLogThreadIDs)
	{
#ifdef WIN32
		sstr << "[" << std::setfill('0') << std::setw(4) << std::hex << ::GetCurrentThreadId() << "] ";
#else
		sstr << "[" << std::setfill('0') << std::setw(4) << std::hex << pthread_self() << "] ";
#endif
	}

	if ((level != LOG_ERROR))
	{
		sstr << cbuffer;
	}
	else
	{
		sstr << "Error: " << cbuffer;
	}

	std::string szIntLog = sstr.str();

	if ((level == LOG_ERROR) && (m_bEnableErrorsToNotificationSystem))
	{
		if (m_notification_log.size() >= MAX_LOG_LINE_BUFFER)
			m_notification_log.erase(m_last_error_log.begin());
		m_notification_log.push_back(_tLogLineStruct(level, szIntLog));
		if ((m_notification_log.size() == 1) && (mytime(NULL) - m_LastLogNotificationsSend >= 5))
		{
			m_mainworker.ForceLogNotificationCheck();
		}
	}

	if (!g_bRunAsDaemon)
	{
		//output to console
		std::cout << szIntLog << std::endl;
	}

	if (m_outputfile.is_open())
	{
		//output to file
		m_outputfile << szIntLog << std::endl;
		m_outputfile.flush();
	}

	if (m_lastlog.size() >= MAX_LOG_LINE_BUFFER)
		m_lastlog.erase(m_lastlog.begin());
	m_lastlog.push_back(_tLogLineStruct(level, szIntLog));

	if (level == LOG_STATUS)
	{
		if (m_last_status_log.size() >= MAX_LOG_LINE_BUFFER)
			m_last_status_log.erase(m_last_status_log.begin());
		m_last_status_log.push_back(_tLogLineStruct(level, szIntLog));
	}
	else if (level == LOG_ERROR)
	{
		if (m_last_error_log.size() >= MAX_LOG_LINE_BUFFER)
			m_last_error_log.erase(m_last_error_log.begin());
		m_last_error_log.push_back(_tLogLineStruct(level, szIntLog));
	}
}

bool strhasEnding(std::string const &fullString, std::string const &ending)
{
	return fullString.size() >= ending.size() && !fullString.compare(fullString.size() - ending.size(), ending.size(), ending);
}

void CLogger::LogSequenceStart()
{
	m_bInSequenceMode = true;
	m_sequencestring.clear();
	m_sequencestring.str("");
}

void CLogger::LogSequenceEnd(const _eLogLevel level)
{
	if (!m_bInSequenceMode)
		return;

	std::string message = m_sequencestring.str();
	if (strhasEnding(message, "\n"))
	{
		message = message.substr(0, message.size() - 1);
	}

	Log(level, message.c_str());
	m_sequencestring.clear();
	m_sequencestring.str("");

	m_bInSequenceMode = false;
}

void CLogger::LogSequenceAdd(const char* logline)
{
	if (!m_bInSequenceMode)
		return;

	m_sequencestring << logline << std::endl;
}

void CLogger::LogSequenceAddNoLF(const char* logline)
{
	if (!m_bInSequenceMode)
		return;

	m_sequencestring << logline;
}

void CLogger::EnableLogTimestamps(const bool bEnableTimestamps)
{
	m_bEnableLogTimestamps = bEnableTimestamps;
}

bool CLogger::IsLogTimestampsEnabled()
{
	return (m_bEnableLogTimestamps && !g_bUseSyslog);
}

void CLogger::EnableLogThreadIDs(const bool bEnableThreadIDs)
{
	m_bEnableLogThreadIDs = bEnableThreadIDs;
}

bool CLogger::IsLogThreadIDsEnabled()
{
	return m_bEnableLogThreadIDs && !g_bUseSyslog;
}

std::list<CLogger::_tLogLineStruct> CLogger::GetLog(const _eLogLevel lType)
{
	boost::unique_lock< boost::mutex > lock(m_mutex);
	std::list<_tLogLineStruct> mlist;
	std::deque<_tLogLineStruct>::const_iterator itt;

	if (lType == LOG_NORM)
	{
		for (itt = m_lastlog.begin(); itt != m_lastlog.end(); ++itt)
		{
			mlist.push_back(*itt);
		};
	}
	else if (lType == LOG_STATUS)
	{
		for (itt = m_last_status_log.begin(); itt != m_last_status_log.end(); ++itt)
		{
			mlist.push_back(*itt);
		};
	}
	else if (lType == LOG_ERROR)
	{
		for (itt = m_last_error_log.begin(); itt != m_last_error_log.end(); ++itt)
		{
			mlist.push_back(*itt);
		};
	}
	return mlist;
}

void CLogger::SetFilterString(std::string  &pFilter)
{
	std::vector<std::string> FilterList;
	FilterString = pFilter;
	FilterStringList.clear();
	KeepStringList.clear();
	StringSplit(pFilter, ";", FilterList);
	for (size_t i = 0; i < FilterList.size(); i++)
	{
		if (FilterList[i][0] == '+')
			KeepStringList.push_back(FilterList[i].substr(1));
		else
			FilterStringList.push_back(FilterList[i]);
	}
}

bool CLogger::isTraceEnabled()
{
	return (m_verbose_level == VBL_TRACE);
}

//return true if the log shall be filtered
//
bool CLogger::TestFilter(const char *cbuffer)
{
	bool filtered = false; //default not filtered

	//search if the log shall be filter
	for (size_t i = 0; i < FilterStringList.size(); i++)
	{
		if (strstr(cbuffer, FilterStringList[i].c_str()) != 0)
		{
			filtered = true;
			break;
		}
	}
	//if the log as been filtered , search if it shall be kept
	if (filtered)
	{
		for (size_t i = 0; i < KeepStringList.size(); i++)
		{
			if (strstr(cbuffer, KeepStringList[i].c_str()) != 0)
			{
				filtered = false;
				break;
			}
		}
	}
	return filtered;
}

void CLogger::setLogVerboseLevel(int LogLevel)
{
	SetVerboseLevel((_eLogFileVerboseLevel)(LogLevel & 0x3));
	//test verbose level
	if (LogLevel & 0x4)
		m_mainworker.SetVerboseLevel(EVBL_ALL);
	else
		m_mainworker.SetVerboseLevel(EVBL_None);
}

//set the DEBUG option in order to allow LOG_TRACE log level 
void CLogger::SetLogDebug(bool debug)
{
	m_debug = debug;
}

bool CLogger::GetLogDebug()
{
	return m_debug;
}

void CLogger::SetLogPreference(std::string  LogFilter, std::string  LogFileName, std::string  LogLevel)
{
	//if trace level is allowed
	if (GetLogDebug())
	{
		//set LogFilter/LogFileName/LogLevel from Preferences tables
		m_sql.UpdatePreferencesVar("LogFilter", 0, LogFilter.c_str());
		m_sql.UpdatePreferencesVar("LogFileName", 0, LogFileName.c_str());
		m_sql.UpdatePreferencesVar("LogLevel", 0, LogLevel.c_str());
		SetFilterString(LogFilter);
		SetOutputFile(LogFileName.c_str());
		setLogVerboseLevel(atoi(LogLevel.c_str()));
	}
}
void CLogger::GetLogPreference()
{
	std::string LogFilter, LogFileName, LogLevel;

	//if trace level is allowed
	if (GetLogDebug())
	{
		//get LogFilter/LogFileName/LogLevel from Preferences tables
		m_sql.GetPreferencesVar("LogFilter", LogFilter);
		m_sql.GetPreferencesVar("LogFileName", LogFileName);
		m_sql.GetPreferencesVar("LogLevel", LogLevel);
		SetFilterString(LogFilter);
		SetOutputFile(LogFileName.c_str());

		if (LogLevel.length() != 0)
			setLogVerboseLevel(atoi(LogLevel.c_str()));
		else
		{
			m_sql.UpdatePreferencesVar("LogLevel", 0, boost::to_string(VBL_ALL));
			SetVerboseLevel(VBL_ALL);
		}
	}
	else
	{
		//delete LogLevel key in order to dot not display in settings TAB
		m_sql.DeletePreferencesVar("LogLevel");
	}
}

void CLogger::ClearLog()
{
	boost::unique_lock< boost::mutex > lock(m_mutex);
	m_lastlog.clear();
	m_last_status_log.clear();
	m_last_error_log.clear();
}

std::list<CLogger::_tLogLineStruct> CLogger::GetNotificationLogs()
{
	boost::unique_lock< boost::mutex > lock(m_mutex);
	std::list<_tLogLineStruct> mlist;
	std::deque<_tLogLineStruct>::const_iterator itt;
	for (itt = m_notification_log.begin(); itt != m_notification_log.end(); ++itt)
	{
		mlist.push_back(*itt);
	};
	m_notification_log.clear();
	if (!mlist.empty())
		m_LastLogNotificationsSend = mytime(NULL);
	return mlist;
}

bool CLogger::NotificationLogsEnabled()
{
	return m_bEnableErrorsToNotificationSystem;
}
