#pragma once

#include "Logger.h"
#include <libwebem/IWebServerLogger.h>
#include <cstdarg>
#include <cstdio>

namespace http {
namespace server {

class WebServerLoggerAdapter : public IWebServerLogger
{
public:
	void Log(LogLevel level, const char *fmt, ...) override
	{
		_eLogLevel domo_level;
		switch (level) {
		case LogLevel::Error:  domo_level = LOG_ERROR; break;
		case LogLevel::Status: domo_level = LOG_STATUS; break;
		case LogLevel::Debug:  domo_level = LOG_NORM; break;
		default:               domo_level = LOG_NORM; break;
		}

		va_list args;
		va_start(args, fmt);
		char buf[2048];
		vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);
		_log.Log(domo_level, "%s", buf);
	}

	void Debug(DebugCategory category, const char *fmt, ...) override
	{
		_eDebugLevel domo_level;
		switch (category) {
		case DebugCategory::WebServer: domo_level = DEBUG_WEBSERVER; break;
		case DebugCategory::Auth:      domo_level = DEBUG_AUTH; break;
		default:                       domo_level = DEBUG_WEBSERVER; break;
		}

		va_list args;
		va_start(args, fmt);
		char buf[2048];
		vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);
		_log.Debug(domo_level, "%s", buf);
	}

	bool IsAccessLogEnabled() override
	{
		return _log.IsACLFlogEnabled();
	}

	void AccessLog(const char *fmt, ...) override
	{
		va_list args;
		va_start(args, fmt);
		char buf[4096];
		vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);
		_log.ACLFlog("%s", buf);
	}
};

} // namespace server
} // namespace http
