#pragma once

#include <deque>
#include <list>
#include <string>
#include <fstream>

enum _eLogLevel
{
	LOG_NORM=0,
	LOG_ERROR,
};

class CLogger
{
public:
	struct _tLogLineStruct
	{
		_eLogLevel level;
		std::string logmessage;
		_tLogLineStruct(const _eLogLevel nlevel, const std::string nlogmessage)
		{
			level=nlevel;
			logmessage=nlogmessage;
		}
	};

	CLogger(void);
	~CLogger(void);

	void SetOutputFile(const char *OutputFile);

	void Log(const _eLogLevel level, const char* logline, ...);
	void LogNoLF(const _eLogLevel level, const char* logline, ...);

	std::list<_tLogLineStruct> GetLog();
private:
	std::ofstream m_outputfile;
	std::deque<_tLogLineStruct> m_lastlog;
};
extern CLogger _log;
