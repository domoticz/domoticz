#include <boost/asio/ip/address_v4.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#ifndef UNIT_TESTING
#include "../../httpclient/HTTPClient.h"
#else
#include <boost/current_function.hpp>
#endif

#include "../../main/Logger.h"

#include "../../json/value.h"
#include "../../json/writer.h"
#include "../../main/json_helper.h"
#include "errors.h"
#include "ip.h"

#include "session.h"

blebox::session::session(::blebox::ip ip, int connection_timeout,
                         int read_timeout)
    : _ip(std::move(ip)),
      _connection_timeout(connection_timeout),
      _read_timeout(read_timeout) {
  if (_ip.is_unspecified())
    throw blebox::errors::failure("host is unspecified");
}

static auto extract_result(const std::string &result, const std::string &url) {
  using namespace std::literals;

  if (result.empty()) {
    Json::Value root;
    root["_blebox.domoticz.info"] = "empty response";
    return root;
  }

  Json::Value root;
  if (!ParseJSonStrict(result, root))
    throw blebox::errors::response::invalid("invalid json from: "s + url +
                                            " JSON: "s +
                                            std::string(result, 0, 1000));

  if (root.empty())
    throw blebox::errors::response::empty(url);

  _log.Debug(DEBUG_HARDWARE, "Done fetching %s", url.c_str());

  // TODO: is this really needed? Error handling?
  return root.isArray() ? root[0] : root;
}

static auto make_url(blebox::ip _ip, const std::string &path) {
  // TODO: url library?
  std::stringstream sstr;
  sstr << "http://" << _ip << path;
  return sstr.str();
}

void blebox::session::set_timeouts() const {
#ifndef UNIT_TESTING
  HTTPClient::SetConnectionTimeout(_connection_timeout);
  HTTPClient::SetTimeout(_read_timeout);
#else
  std::ignore = _connection_timeout;
  std::ignore = _read_timeout;
#endif
}

auto blebox::session::fetch(const std::string &path) const -> Json::Value {
  auto &&url = make_url(_ip, path); // for error messages
  _log.Debug(DEBUG_HARDWARE, "Fetching %s ...", url.c_str());
  return extract_result(fetch_raw(path), url);
}

std::string blebox::session::fetch_raw(const std::string &path) const {
  set_timeouts();

#ifndef UNIT_TESTING
  auto &&url = make_url(_ip, path);
  std::string result;
  if (!HTTPClient::GET(url, result, true))
    throw blebox::errors::request_failed(url);
  return result;
#else
  using namespace std::literals;
  throw std::runtime_error("not mocked: "s + _ip.to_string() + path + " at "s +
                           BOOST_CURRENT_FUNCTION);
#endif
}

// TODO: extract elswhere
static auto compact_json(const Json::Value &data) {
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  builder["commentStyle"] = "None";
  return Json::writeString(builder, data);
}

auto blebox::session::post_json(const std::string &path,
                                const Json::Value &data) const -> Json::Value {
  auto &&url = make_url(_ip, path); // for error messages
  _log.Debug(DEBUG_HARDWARE, "Posting %s ...", url.c_str());
  auto &&result = post_raw(path, compact_json(data));
  return extract_result(result, url);
}

std::string blebox::session::post_raw(const std::string &path,
                                      const std::string &data) const {
  set_timeouts();

  std::vector<std::string> extra_headers;
  extra_headers.emplace_back("Content-Type: application/json");

#ifndef UNIT_TESTING
  auto &&url = make_url(_ip, path);
  std::string result;
  if (!HTTPClient::POST(url, data, extra_headers, result))
    throw blebox::errors::request_failed(url);
  return result;
#else
  using namespace std::literals;
  throw std::runtime_error("not mocked: "s + _ip.to_string() + path + " at "s +
                           std::string(BOOST_CURRENT_FUNCTION) +
                           " with data: "s + data);
#endif
}
