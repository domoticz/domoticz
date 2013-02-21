#include "stdafx.h"
#include "Logger.h"
#include <iostream>     /* standard I/O functions                         */
#include <stdarg.h>

#define MAX_LOG_LINE_BUFFER 100

CLogger::CLogger(void)
{
}


CLogger::~CLogger(void)
{
	if (m_outputfile.is_open())
		m_outputfile.close();
}

void CLogger::SetOutputFile(const char *OutputFile)
{
	if (m_outputfile.is_open())
		m_outputfile.close();

	if (OutputFile==NULL)
		return;

	try {
		m_outputfile.open(OutputFile);
	} catch(...)
	{
		std::cerr << "Error opening output log file..." << std::endl;
	}
}

void CLogger::Log(const _eLogLevel level, const char* logline, ...)
{
	va_list argList;
	char cbuffer[1024];
	va_start(argList, logline);
	vsnprintf(cbuffer, 1024, logline, argList);
	va_end(argList);

	if (m_lastlog.size()>=MAX_LOG_LINE_BUFFER)
		m_lastlog.erase(m_lastlog.begin());
	m_lastlog.push_back(_tLogLineStruct(level,cbuffer));

	if (level==LOG_NORM)
	{
		std::cout << cbuffer << std::endl;
		if (m_outputfile.is_open())
			m_outputfile << cbuffer << std::endl;
	}
	else
	{
		std::cerr << cbuffer << std::endl;
		if (m_outputfile.is_open())
			m_outputfile << "Error: " << cbuffer << std::endl;
	}
	if (m_outputfile.is_open())
		m_outputfile.flush();
}

void CLogger::LogNoLF(const _eLogLevel level, const char* logline, ...)
{
	va_list argList;
	char cbuffer[1024];
	va_start(argList, logline);
	vsnprintf(cbuffer, 1024, logline, argList);
	va_end(argList);

	if (m_lastlog.size()>=MAX_LOG_LINE_BUFFER)
		m_lastlog.erase(m_lastlog.begin());
	m_lastlog.push_back(_tLogLineStruct(level,cbuffer));

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
	if (m_outputfile.is_open())
		m_outputfile.flush();
}

std::list<CLogger::_tLogLineStruct> CLogger::GetLog()
{
	//boost::unique_lock< boost::mutex > lock(m_mutex);
	std::list<_tLogLineStruct> mlist;
	std::deque<_tLogLineStruct>::const_iterator itt;
	for (itt=m_lastlog.begin(); itt!=m_lastlog.end(); ++itt)
	{
		mlist.push_back(*itt);
	};
	return mlist;
}

