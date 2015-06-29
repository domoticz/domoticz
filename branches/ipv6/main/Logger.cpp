#include "stdafx.h"
#include "Logger.h"
#include <iostream>     /* standard I/O functions                         */
#include <stdarg.h>
#include <time.h>
#include "localtime_r.h"

#ifndef WIN32
	#include <syslog.h>
	#include <errno.h>
#endif


#define MAX_LOG_LINE_BUFFER 100
#define MAX_LOG_LINE_LENGTH 2048

extern bool g_bRunAsDaemon;
extern bool g_bUseSyslog;

CLogger::_tLogLineStruct::_tLogLineStruct(const _eLogLevel nlevel, const std::string &nlogmessage)
{
	logtime=mytime(NULL);
	level=nlevel;
	logmessage=nlogmessage;
}

CLogger::CLogger(void)
{
	m_bInSequenceMode=false;
	m_verbose_level=VBL_ALL;
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

	if (OutputFile==NULL)
		return;

	try {
#ifdef _DEBUG
		m_outputfile.open(OutputFile, std::ios::out | std::ios::trunc);
#else
		m_outputfile.open(OutputFile, std::ios::out | std::ios::app);
#endif
	} catch(...)
	{
		std::cerr << "Error opening output log file..." << std::endl;
	}
}

void CLogger::SetVerboseLevel(_eLogFileVerboseLevel vLevel)
{
	m_verbose_level=vLevel;
}

void CLogger::Log(const _eLogLevel level, const char* logline, ...)
{
	boost::unique_lock< boost::mutex > lock(m_mutex);

	bool bDoLog = false;
	if (m_verbose_level == VBL_ALL)
		bDoLog = true;
	else if ((m_verbose_level == VBL_STATUS_ERROR) && ((level == LOG_STATUS) || (level == LOG_ERROR)))
		bDoLog = true;
	else if ((m_verbose_level == VBL_ERROR) && (level == LOG_ERROR))
		bDoLog = true;

	if (!bDoLog)
		return;

	va_list argList;
	char cbuffer[MAX_LOG_LINE_LENGTH];
	va_start(argList, logline);
	vsnprintf(cbuffer, sizeof(cbuffer), logline, argList);
	va_end(argList);

	char szDate[100];
#if !defined WIN32
	// Get a timestamp
	struct timeval tv;
	gettimeofday(&tv, NULL);

	struct tm timeinfo;
	localtime_r(&tv.tv_sec, &timeinfo);

	// create a time stamp string for the log message
	snprintf(szDate, sizeof(szDate), "%04d-%02d-%02d %02d:%02d:%02d.%03d ",
		timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
		timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, (int)tv.tv_usec / 1000);
#else
	// Get a timestamp
	SYSTEMTIME time;
	::GetLocalTime(&time);
	// create a time stamp string for the log message
	sprintf_s(szDate, sizeof(szDate), "%04d-%02d-%02d %02d:%02d:%02d.%03d ", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
#endif

	std::stringstream sstr;
	
	if ((level==LOG_NORM)||(level==LOG_STATUS))
	{
		sstr << szDate << " " << cbuffer;
	}
	else {
		sstr << szDate << " Error: " << cbuffer;
	}

	if (m_lastlog.size()>=MAX_LOG_LINE_BUFFER)
		m_lastlog.erase(m_lastlog.begin());
	m_lastlog.push_back(_tLogLineStruct(level,sstr.str()));

	if (!g_bRunAsDaemon)
	{
		//output to console
		std::cout << sstr.str() << std::endl;
	}
#ifndef WIN32
	if (g_bUseSyslog)
	{
		int sLogLevel = LOG_INFO;
		if (level == LOG_ERROR)
			sLogLevel =  LOG_ERR;
		else if (level == LOG_STATUS)
			sLogLevel = LOG_NOTICE;
		syslog(sLogLevel, "%s", sstr.str().c_str());
	}
#endif
	if (!m_outputfile.is_open())
		return;

	//output to file

	m_outputfile << sstr.str() << std::endl;
	m_outputfile.flush();
}

bool strhasEnding(std::string const &fullString, std::string const &ending)
{
	if (fullString.length() >= ending.length()) {
		return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
	} else {
		return false;
	}
}

void CLogger::LogNoLF(const _eLogLevel level, const char* logline, ...)
{
	boost::unique_lock< boost::mutex > lock(m_mutex);

	bool bDoLog = false;
	if (m_verbose_level == VBL_ALL)
		bDoLog = true;
	else if ((m_verbose_level == VBL_STATUS_ERROR) && ((level == LOG_STATUS) || (level == LOG_ERROR)))
		bDoLog = true;
	else if ((m_verbose_level == VBL_ERROR) && (level == LOG_ERROR))
		bDoLog = true;

	if (!bDoLog)
		return;

	va_list argList;
	char cbuffer[MAX_LOG_LINE_LENGTH];
	va_start(argList, logline);
	vsnprintf(cbuffer, sizeof(cbuffer), logline, argList);
	va_end(argList);

	std::string message=cbuffer;
	if (strhasEnding(message,"\n"))
	{
		message=message.substr(0,message.size()-1);
	}
	if (m_lastlog.size()>=MAX_LOG_LINE_BUFFER)
		m_lastlog.erase(m_lastlog.begin());
	m_lastlog.push_back(_tLogLineStruct(level,message));

	if (!g_bRunAsDaemon)
	{
		if ((level == LOG_NORM) || (level == LOG_STATUS))
		{
			std::cout << cbuffer;
			std::cout.flush();
		}
		else
		{
			std::cerr << cbuffer;
			std::cerr.flush();
		}
	}

#ifndef WIN32
	if (g_bUseSyslog)
	{
		int sLogLevel = LOG_INFO;
		if (level == LOG_ERROR)
			sLogLevel =  LOG_ERR;
		else if (level == LOG_STATUS)
			sLogLevel = LOG_NOTICE;
		syslog(sLogLevel, "%s", cbuffer);
	}
#endif

	if (!m_outputfile.is_open())
		return;

	//output to file

	if ((level==LOG_NORM)||(level==LOG_STATUS))
		m_outputfile << cbuffer;
	else
		m_outputfile << "Error: " << cbuffer;
	m_outputfile.flush();
}

void CLogger::LogSequenceStart()
{
	m_bInSequenceMode=true;
	m_sequencestring.clear();
	m_sequencestring.str("");
}

void CLogger::LogSequenceEnd(const _eLogLevel level)
{
	m_bInSequenceMode=false;
	LogNoLF(level,m_sequencestring.str().c_str());
	m_sequencestring.clear();
	m_sequencestring.str("");
}

void CLogger::LogSequenceAdd(const char* logline)
{
	m_sequencestring << logline << std::endl;
}

void CLogger::LogSequenceAddNoLF(const char* logline)
{
	m_sequencestring << logline;
}

std::list<CLogger::_tLogLineStruct> CLogger::GetLog()
{
	boost::unique_lock< boost::mutex > lock(m_mutex);
	std::list<_tLogLineStruct> mlist;
	std::deque<_tLogLineStruct>::const_iterator itt;
	for (itt=m_lastlog.begin(); itt!=m_lastlog.end(); ++itt)
	{
		mlist.push_back(*itt);
	};
	return mlist;
}
