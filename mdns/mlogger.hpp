#pragma once

#include <functional>
#include <iostream>
#include <sstream>
#include <string>

namespace mdns_cpp {

class Logger {
 public:
  static void LogIt(const std::string& s);
  static void setLoggerSink(std::function<void(const std::string&)> callback);
  static void useDefaultSink();

 private:
  static bool logger_registered;
  static std::function<void(const std::string&)> logging_callback_function;
};

class LogMessage {
 public:
  LogMessage(const char* file, int line);
  LogMessage();

  ~LogMessage();

  template <typename T>
  LogMessage& operator<<(const T& t) {
    os << t;
    return *this;
  }

 private:
  std::ostringstream os;
};

}  // namespace mdns_cpp
