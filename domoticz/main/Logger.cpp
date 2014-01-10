#include "stdafx.h"
#include "Logger.h"
#include <iostream>     /* standard I/O functions                         */
#include <stdarg.h>
#include <time.h>
#include "localtime_r.h"

#define MAX_LOG_LINE_BUFFER 100

CLogger::_tLogLineStruct::_tLogLineStruct(const _eLogLevel nlevel, const std::string &nlogmessage)
{
	logtime=mytime(NULL);
	level=nlevel;
	logmessage=nlogmessage;
}

CLogger::CLogger(void)
{
	m_bInSequenceMode=false;
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
		m_outputfile.open(OutputFile, std::ios::out | std::ios::app);
	} catch(...)
	{
		std::cerr << "Error opening output log file..." << std::endl;
	}
}

void CLogger::Log(const _eLogLevel level, const char* logline, ...)
{
	boost::unique_lock< boost::mutex > lock(m_mutex);
	va_list argList;
	char cbuffer[1024];
	va_start(argList, logline);
	vsnprintf(cbuffer, 1024, logline, argList);
	va_end(argList);

	if (m_lastlog.size()>=MAX_LOG_LINE_BUFFER)
		m_lastlog.erase(m_lastlog.begin());
	if (level==LOG_ERROR)
	{
		//Error log, add date
		// convert now to string form
		time_t now = time(0);
		char *szDate = asctime(localtime(&now));
		szDate[strlen(szDate)-1]=0;

		std::stringstream sstr;
		sstr << szDate << " " << cbuffer;
		strcpy(cbuffer,sstr.str().c_str());
	}
	m_lastlog.push_back(_tLogLineStruct(level,cbuffer));

	if (level==LOG_NORM)
	{
		std::cout << cbuffer << std::endl;
		if (m_outputfile.is_open())
			m_outputfile << cbuffer << std::endl;
	}
	else
	{
		//Error log
		std::cerr << cbuffer << std::endl;
		if (m_outputfile.is_open())
			m_outputfile << "Error: " << cbuffer << std::endl;
	}
	if (m_outputfile.is_open())
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
	va_list argList;
	char cbuffer[1024];
	va_start(argList, logline);
	vsnprintf(cbuffer, 1024, logline, argList);
	va_end(argList);

	if (m_lastlog.size()>=MAX_LOG_LINE_BUFFER)
		m_lastlog.erase(m_lastlog.begin());
	std::string message=cbuffer;
	if (strhasEnding(message,"\n"))
	{
		message=message.substr(0,message.size()-1);
	}
	m_lastlog.push_back(_tLogLineStruct(level,message));

	if (level==LOG_NORM)
	{
		std::cout << cbuffer;
		if (m_outputfile.is_open())
			m_outputfile << cbuffer;
	}
	else
	{
		std::cerr << cbuffer;
		if (m_outputfile.is_open())
			m_outputfile << "Error: " << cbuffer;
	}

	std::cout.flush();

	if (m_outputfile.is_open())
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

	//boost::unique_lock< boost::mutex > lock(m_mutex);
	std::list<_tLogLineStruct> mlist;
	std::deque<_tLogLineStruct>::const_iterator itt;
	for (itt=m_lastlog.begin(); itt!=m_lastlog.end(); ++itt)
	{
		mlist.push_back(*itt);
	};
	return mlist;
}

