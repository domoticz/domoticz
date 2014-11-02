//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/Log.h"
#include <stdarg.h>

#if defined(_LINUX)
#include <syslog.h>
#elif defined(_WINDOWS)
#include <windows.h>
#include "common/Strings.h"
#include "service/Messages.h"
#endif

class Log::PrivateData {
public:
	PrivateData() : logOutput(Log::System), debug(false) {}

	Log::LogOutput logOutput;
	bool debug;

	static Log *instance;
#ifdef _WINDOWS
	HANDLE eventSource;
#endif
};

Log *Log::PrivateData::instance = 0;

Log::Log()
	:d(new PrivateData) {
#if defined(_LINUX)
	setlogmask(LOG_UPTO(LOG_INFO));
	openlog("telldusd", LOG_CONS, LOG_USER);
#elif defined(_MACOSX)
	d->logOutput = Log::StdOut;
#elif defined(_WINDOWS)
	// Add ourselves to the registy
	HKEY hRegKey = NULL;
	DWORD dwError = 0;
	TCHAR filePath[MAX_PATH];

	std::wstring path(L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\TelldusService");
	dwError = RegCreateKey( HKEY_LOCAL_MACHINE, path.c_str(), &hRegKey );

	GetModuleFileName( NULL, filePath, MAX_PATH );
	dwError = RegSetValueEx( hRegKey, L"EventMessageFile", 0, REG_EXPAND_SZ, (PBYTE) filePath, (DWORD)(wcslen(filePath) + 1) * sizeof TCHAR );

	DWORD dwTypes = LOG_DEBUG | LOG_NOTICE | LOG_WARNING | LOG_ERR;
		dwError = RegSetValueEx( hRegKey, L"TypesSupported", 0, REG_DWORD, (LPBYTE) &dwTypes, sizeof dwTypes );

	RegCloseKey(hRegKey);

	d->eventSource = RegisterEventSource(NULL, L"TelldusService");
#endif
}

Log::~Log() {
#if defined(_LINUX)
	closelog();
#elif defined(_WINDOWS)
	if (d->eventSource != NULL) {
		DeregisterEventSource(d->eventSource);
	}
#endif
	delete d;
}

void Log::destroy() {
	if (PrivateData::instance == 0) {
		return;
	}
	delete PrivateData::instance;
	PrivateData::instance = 0;
}

void Log::debug(const char *fmt, ...) {
	Log *log = Log::instance();
	va_list ap;
	va_start(ap, fmt);
	log->message(Debug, fmt, ap);
	va_end(ap);
}

void Log::notice(const char *fmt, ...) {
	Log *log = Log::instance();
	va_list ap;
	va_start(ap, fmt);
	log->message(Notice, fmt, ap);
	va_end(ap);
}

void Log::warning(const char *fmt, ...) {
	Log *log = Log::instance();
	va_list ap;
	va_start(ap, fmt);
	log->message(Warning, fmt, ap);
	va_end(ap);
}

void Log::error(const char *fmt, ...) {
	Log *log = Log::instance();
	va_list ap;
	va_start(ap, fmt);
	log->message(Error, fmt, ap);
	va_end(ap);
}

void Log::setDebug() {
	Log *log = Log::instance();
	log->d->debug = true;
	Log::debug("Debug message output enabled");
}

void Log::setLogOutput(LogOutput logOutput) {
#ifdef _MACOSX
	// Always stdout
	return;
#endif
	Log *log = Log::instance();
	log->d->logOutput = logOutput;
}

void Log::message(Log::LogLevel logLevel, const char *format, va_list ap) const {
	if (logLevel == Debug && d->debug == false) {
		return;
	}
	if (d->logOutput == StdOut) {
		FILE *stream = stdout;
		if (logLevel == Warning || logLevel == Error) {
			stream = stderr;
		}
		vfprintf(stream, format, ap);
		fprintf(stream, "\n");
		fflush(stream);
	} else {
#if defined(_LINUX)
		switch (logLevel) {
			case Debug:
				vsyslog(LOG_DEBUG, format, ap);
				break;
			case Notice:
				vsyslog(LOG_NOTICE, format, ap);
				break;
			case Warning:
				vsyslog(LOG_WARNING, format, ap);
				break;
			case Error:
				vsyslog(LOG_ERR, format, ap);
				break;
		}
#elif defined(_WINDOWS)
		LPWSTR pInsertStrings[2] = {NULL, NULL};
		std::wstring str = TelldusCore::charToWstring(TelldusCore::sformatf(format, ap).c_str());
		pInsertStrings[0] = (LPWSTR)str.c_str();

		switch (logLevel) {
			case Debug:
				ReportEvent(d->eventSource, EVENTLOG_SUCCESS, NULL, LOG_DEBUG, NULL, 1, 0, reinterpret_cast<LPCWSTR*>(&pInsertStrings), NULL);
				break;
			case Notice:
				ReportEvent(d->eventSource, EVENTLOG_INFORMATION_TYPE, NULL, LOG_NOTICE, NULL, 1, 0, reinterpret_cast<LPCWSTR*>(&pInsertStrings), NULL);
				break;
			case Warning:
				ReportEvent(d->eventSource, EVENTLOG_WARNING_TYPE, NULL, LOG_WARNING, NULL, 1, 0, reinterpret_cast<LPCWSTR*>(&pInsertStrings), NULL);
				break;
			case Error:
				ReportEvent(d->eventSource, EVENTLOG_ERROR_TYPE, NULL, LOG_ERR, NULL, 1, 0, reinterpret_cast<LPCWSTR*>(&pInsertStrings), NULL);
				break;
		}
#endif
	}
}

Log *Log::instance() {
	if (PrivateData::instance == 0) {
		PrivateData::instance = new Log();
	}
	return PrivateData::instance;
}
