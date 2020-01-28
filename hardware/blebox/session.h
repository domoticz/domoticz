#pragma once

#include <string>

#include "ip.h"

namespace Json {
class Value;
}

namespace blebox {
class session {
public:
  session(::blebox::ip ip, int connection_timeout, int read_timeout);
  virtual ~session() = default;

  auto fetch(const std::string &path) const -> Json::Value;
  auto post_json(const std::string &path, const Json::Value &data) const
      -> Json::Value;

  const ::blebox::ip &ip() const { return _ip; }

private:
  ::blebox::ip _ip;
  int _connection_timeout;
  int _read_timeout;

  // virtual and string-based for easier mocking in tests
  virtual std::string post_raw(const std::string &path,
                               const std::string &data) const;

  // virtual and string-based for easier mocking in tests
  virtual std::string fetch_raw(const std::string &path) const;

  void set_timeouts() const;
};
} // namespace blebox
