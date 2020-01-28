#include <stdexcept>
#include <string>

#include "../../json/value.h"

#include "../../httpclient/HTTPClient.h"
#include "../../main/HTMLSanitizer.h"

#include "../../main/WebServer.h"

#include "../../main/Logger.h"

using BYTE = unsigned char; // for RFXtrx.h in mainworker.h
#include "../../main/mainworker.h" // for GetHardwareByIDType

#include "../../webserver/request.hpp"

#include "../BleBox.h"
#include "ip.h"

// Webserver helpers
namespace http {
namespace server {

// ------------------ safely extracting request parameters --------------------

// throw invalid_argument
static auto stoi_x(const request &req, const char *name) -> int {
  try {
    return std::stoi(request::findValue(&req, name));
  } catch (std::range_error &ex) { throw std::invalid_argument(ex.what()); }
}

// throws invalid_argument
static auto xss_x(const request &req, const char *name) -> std::string {
  using namespace std::literals;

  const std::string &result =
      HTMLSanitizer::Sanitize(request::findValue(&req, name));
  if (result.empty())
    throw std::invalid_argument("Invalid parameter: "s + name);

  return result;
}

// ---------------------- request handling --------------------

// throws invalid_argument
static auto respond_x(WebEmSession &session, const request &req,
                      Json::Value &root, const std::string &name) -> BleBox * {
  using namespace std::literals;

  if (session.rights != 2) {
    session.reply_status = reply::forbidden;
    throw std::invalid_argument(
        "Only admin allowed here"); // Only admin user allowed
  }

  int hw_id = stoi_x(req, "hw_id");
  BleBox *blebox = reinterpret_cast<BleBox *>(
      m_mainworker.GetHardwareByIDType(std::to_string(hw_id), HTYPE_BleBox));

  if (blebox == nullptr)
    throw std::invalid_argument("Could not find matching hardware: "s +
                                std::to_string(hw_id));

  root["status"] = "OK";
  root["title"] = name.c_str();
  return blebox;
}

// TODO: show error results in web UI?
static void log_web_exception(const std::string &action,
                              const std::exception &ex) {
  _log.Log(LOG_ERROR, "WebServer::%s action failed: '%s'", action.c_str(),
           ex.what());
}

class web_action_trace {
public:
  web_action_trace(const std::string &name) {
    size_t params_start = name.find('(');
    size_t name_start = name.rfind(' ', params_start);
    if (name_start != std::string::npos)
      _name = name.substr(name_start + 1, params_start - name_start - 1) + "()";
    else
      _name = name;

    _log.Debug(DEBUG_HARDWARE, "WebServer::%s action started", _name.c_str());
  }

  ~web_action_trace() {
    _log.Debug(DEBUG_HARDWARE, "WebServer::%s action done", _name.c_str());
  }

  auto name() const -> const std::string & { return _name; }

private:
  std::string _name;
};

void CWebServer::Cmd_BleBoxDetectHardware(WebEmSession &session,
                                          const request &req,
                                          Json::Value &root) {
  web_action_trace trace(BOOST_CURRENT_FUNCTION);
  try {
    if (session.rights != 2) {
      session.reply_status = reply::forbidden;
      throw std::invalid_argument(
          "Only admin allowed here"); // Only admin user allowed
    }

    root["status"] = "OK";
    root["title"] = trace.name();

    // NOTE: not really a network mask, but a pattern, e.g. 192.168.0.*
    BleBox::SearchNetwork(xss_x(req, "ipmask"));
  } catch (std::exception &ex) { log_web_exception(trace.name(), ex); }
}

void CWebServer::Cmd_BleBoxUpgradeFirmware(WebEmSession &session,
                                           const request &req,
                                           Json::Value &root) {
  web_action_trace trace(BOOST_CURRENT_FUNCTION);
  try {
    respond_x(session, req, root, trace.name())->UpgradeFirmware();
  } catch (std::exception &ex) { log_web_exception(trace.name(), ex); }
}

void CWebServer::Cmd_BleBoxBestIpPattern(WebEmSession & /*session*/,
                                         const request & /*req*/,
                                         Json::Value &root) {
  web_action_trace trace(BOOST_CURRENT_FUNCTION);
  try {
    boost::asio::io_service netService;
    using boost::asio::ip::udp;
    udp::resolver resolver(netService);
    udp::resolver::query query(udp::v4(), "google.com", "");
    udp::resolver::iterator endpoints = resolver.resolve(query);
    udp::endpoint ep = *endpoints;
    udp::socket socket(netService);
    socket.connect(ep);
    boost::asio::ip::address addr = socket.local_endpoint().address();
    root["result"]["ip"] = addr.to_string();

  } catch (std::exception &ex) { log_web_exception(trace.name(), ex); }
}

void CWebServer::Cmd_BleBoxRemoveFeature(WebEmSession &session,
                                         const request &req,
                                         Json::Value &root) {
  web_action_trace trace(BOOST_CURRENT_FUNCTION);
  try {
    BleBox *bb = respond_x(session, req, root, trace.name());
    bb->AsyncRemoveFeature(stoi_x(req, "nodeid"));
  } catch (std::exception &ex) { log_web_exception(trace.name(), ex); }
}

void CWebServer::Cmd_BleBoxClearFeatures(WebEmSession &session,
                                         const request &req,
                                         Json::Value &root) {
  web_action_trace trace(BOOST_CURRENT_FUNCTION);
  try {
    respond_x(session, req, root, trace.name())->AsyncRemoveAllFeatures();
  } catch (std::exception &ex) { log_web_exception(trace.name(), ex); }
}

void CWebServer::Cmd_BleBoxGetFeatures(WebEmSession &session,
                                       const request &req, Json::Value &root) {
  web_action_trace trace(BOOST_CURRENT_FUNCTION);
  try {
    respond_x(session, req, root, trace.name())->FetchAllFeatures(root);
  } catch (std::exception &ex) { log_web_exception(trace.name(), ex); }
}

void CWebServer::Cmd_BleBoxDetectFeatures(WebEmSession &session,
                                          const request &req,
                                          Json::Value &root) {
  web_action_trace trace(BOOST_CURRENT_FUNCTION);
  try {
    respond_x(session, req, root, trace.name())->DetectFeatures();
  } catch (std::exception &ex) { log_web_exception(trace.name(), ex); }
}

void CWebServer::Cmd_BleBoxRefreshFeatures(WebEmSession &session,
                                           const request &req,
                                           Json::Value &root) {
  web_action_trace trace(BOOST_CURRENT_FUNCTION);
  try {
    respond_x(session, req, root, trace.name())->RefreshFeatures();
  } catch (std::exception &ex) { log_web_exception(trace.name(), ex); }
}

void CWebServer::Cmd_BleBoxUseFeatures(WebEmSession &session,
                                           const request &req,
                                           Json::Value &root) {
  web_action_trace trace(BOOST_CURRENT_FUNCTION);
  try {
    respond_x(session, req, root, trace.name())->UseFeatures();
  } catch (std::exception &ex) { log_web_exception(trace.name(), ex); }
}

} // namespace server
} // namespace http
