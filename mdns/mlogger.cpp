#include "stdafx.h"
#include "mlogger.hpp"

namespace mdns_cpp {

bool Logger::logger_registered = false;

std::function<void(const std::string &)> Logger::logging_callback_function;

void Logger::LogIt(const std::string &s) {
  if (logger_registered) {
    logging_callback_function(s);
  } else {
    std::cout << "[DefaultLogger]: " << s << "\n";
  }
}

void Logger::setLoggerSink(std::function<void(const std::string &)> callback) {
  logger_registered = true;
  logging_callback_function = callback;
}

void Logger::useDefaultSink() { logger_registered = false; }

LogMessage::LogMessage(const char *file, int line) { os << "[" << file << ":" << line << "] "; }

LogMessage::LogMessage() { os << ""; }

LogMessage::~LogMessage() { Logger::LogIt(os.str()); }

}  // namespace mdns_cpp
