#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <boost/assert.hpp>
#include <boost/signals2.hpp>

#include <algorithm>
#include <list>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../main/Logger.h"
#include "../main/localtime_r.h"

using BYTE = unsigned char; // for RFXtrx.h in mainworker.h
#include "../main/mainworker.h"

#include "blebox/box.h"
#include "blebox/db/device_status.h"
#include "blebox/db/hardware.h"
#include "blebox/errors.h"
#include "blebox/ip.h"
#include "blebox/session.h"

#include "BleBox.h"

BleBox::BleBox(const int hw_id, const std::string &address,
               const int pollIntervalsec, blebox::session *session)
    : _poll_interval(DefaultPollInterval), _session(session) {
  auto part_count = std::count(address.begin(), address.end(), ':');

  if (part_count > 1)
    throw blebox::errors::bad_ip(address);

  if (part_count == 0)
    _ip = ::blebox::make_address_v4(address);
  else {
    std::vector<std::string> both;
    boost::split(both, address, boost::is_any_of(":"));

    _unique_id = both[0];
    size_t pos;
    try {
      std::stoul(_unique_id, &pos, 16);
    } catch (std::invalid_argument &) {
      throw blebox::errors::bad_mac(_unique_id);
    }
    if (pos != 12)
      throw blebox::errors::bad_mac(_unique_id);

    _ip = blebox::make_address_v4(both[1]);
  }

  if (_ip.is_unspecified())
    throw blebox::errors::bad_ip(_ip.to_string());

  HwdType = HTYPE_BleBox;
  m_HwdID = hw_id;

  SetSettings(pollIntervalsec);
}

BleBox::BleBox(::blebox::ip ip, int hw_id, const int pollIntervalsec,
               blebox::session *session)
    : _poll_interval(DefaultPollInterval),
      _ip(std::move(ip)),
      _session(session) {
  if (_ip.is_unspecified())
    throw blebox::errors::bad_ip(_ip.to_string());

  HwdType = HTYPE_BleBox;
  m_HwdID = hw_id;

  SetSettings(pollIntervalsec);
}

BleBox::~BleBox() {
  if (_internal_session)
    delete _internal_session;
}

bool BleBox::StartHardware() {
  RequestStart();

  m_thread = std::make_shared<std::thread>(&BleBox::Do_Work, this);
  SetThreadNameInt(m_thread->native_handle());
  m_bIsStarted = true;
  sOnConnected(this);
  return (m_thread != nullptr);
}

bool BleBox::StopHardware() {
  if (m_thread) {
    RequestStop();
    m_thread->join();
    m_thread.reset();
  }

  m_bIsStarted = false;
  return true;
}

void BleBox::Do_Work() {
  using namespace std::literals;

  int sec_counter = poll_interval() - 1;

  Log(LOG_STATUS, "Worker started...");
  while (!IsStopRequested(1000)) {
    sec_counter++;

    if (sec_counter % 12 == 0)
      m_LastHeartbeat = mytime(nullptr);

    if (sec_counter % poll_interval() != 0)
      continue;

    try {
      // TODO: log error when hardware no longer exists?

      try {
        reset_box(); // force autodetect to detect ip mismatch
        box().fetch(session(), *this);
      } catch (blebox::errors::request_failed &ex) {
        throw std::runtime_error("Failed to connect to: "s + _ip.to_string() +
                                 ": ("s + std::string(ex.what()) + ")");
      } catch (blebox::errors::response::invalid &ex) {
        throw std::runtime_error("Failed to read from BleBox device at: "s +
                                 _ip.to_string() + " because "s +
                                 std::string(ex.what()));
      }

    } catch (std::exception &ex) {
      Log(LOG_ERROR, "Unexpected error refreshing data at %s: %s",
          session().ip().to_string().c_str(), ex.what());
    }

    SetHeartbeatReceived();
  }
  Log(LOG_STATUS, "Worker stopped...");
}

bool BleBox::WriteToHardware(const char *pdata,
                             const unsigned char /*length*/) {
  try {
    auto &&bx = box();
    auto response = bx.post_raw(hw_id(), session(), pdata);
    bx.update_state_from_response(*this, response);
    return true;
  } catch (std::runtime_error &ex) {
    Log(LOG_ERROR, "failed to post command: %s", ex.what());
    return false;
  }
}

void BleBox::SetSettings(const int pollIntervalSec) {
  _poll_interval =
      (pollIntervalSec > 0 ? pollIntervalSec : DefaultPollInterval);
}

// --------------- Web request handlers --------------------

// called from web UI
void BleBox::FetchAllFeatures(Json::Value &root) {
  std::string name("unknown");
  std::string type("unknown");
  std::string uptime("unknown");
  std::string hardware_version("unknown");
  std::string firmware_version("unknown");

  // TODO: add appropriate HTML table columns for these?
  std::string &error = uptime;
  std::string &error_details = firmware_version;

  try {
    try {
      auto &&bx = box();

      // TODO: get user provided name instead?
      name = bx.name();

      type = bx.type();
      firmware_version = bx.firmware_version();
      hardware_version = bx.hardware_version();
      uptime = bx.fetch_uptime(session());

    } catch (blebox::errors::request_failed &ex) {
      error = "(connection failed))";
      error_details = std::string("(") + ex.what() + ")";
    } catch (blebox::errors::response::invalid &) {
      error = "(connection ok))";
      error_details = "(invalid/unrecognized response)";
    }

    std::vector<blebox::db::device_status> statuses;
    if (!blebox::db::device_status::all(hw_id(), statuses))
      return;

    Json::Value &info = root["result"]["device_info"];

    // copy from box (hardware info)
    info["name"] = name;
    info["type"] = type;
    info["uptime"] = uptime;
    info["hv"] = hardware_version;
    info["fv"] = firmware_version;

    int i = 0;
    for (const auto &feature : statuses) {
      auto &current = root["result"]["features"][i++];

      current["idx"] = feature.id();
      current["Name"] = feature.name();
      current["Value"] = feature.value();
      current["Description"] = feature.description();
    }
  } catch (std::runtime_error &ex) {
    Log(LOG_ERROR, "unexpected error: %s", ex.what());
  }
}

// called from search
bool BleBox::AddOrUpdateHardware(const std::string &name, blebox::ip ip,
                                 bool silent, bool migration,
                                 blebox::session *session) {
  auto severity = silent ? LOG_STATUS : LOG_ERROR;

  try {
    const int timeout = 1;
    blebox::session new_session(ip, timeout, timeout);

    // TODO: this is only for integration tests
    if (session == nullptr)
      session = &new_session;

    const auto box = blebox::box::from_session(*session);

    bool new_record = false;
    int record_id;
    std::string insert_name = (name.empty() ? box->name() : name);
    // TODO: use different field for unique_address
    // NOTE: ignore IP reported by boxice since this one obviously works
    if (blebox::db::hardware::add_or_update_by_unique_id(
            box->unique_id(), ip, new_record, insert_name, record_id,
            box->type())) {
      std::string hw_id_str = std::to_string(record_id);

      if (!migration) {
        auto hw = m_mainworker.GetHardware(record_id);
        if (hw == nullptr) {
          m_mainworker.RestartHardware(hw_id_str);
      } else {
        m_mainworker.RemoveDomoticzHardware(hw);
        m_mainworker.RestartHardware(hw_id_str);
      }
      }

      box->add_missing_supported_features(record_id);
      return true;
    }

    _log.Log(LOG_ERROR, "Failed to update/add hardware at %s",
             ip.to_string().c_str());

  } catch (blebox::errors::request_failed &ex) {
    std::string address = ip.to_string();
    _log.Log(severity, "Could not connect to: %s because %s", address.c_str(),
             ex.what());
  } catch (blebox::errors::response::invalid &ex) {
    std::string address = ip.to_string();
    _log.Log(severity, "Invalid response from %s or not a BleBox device: %s",
             address.c_str(), ex.what());
  } catch (std::runtime_error &ex) {
    std::string address = ip.to_string();
    _log.Log(LOG_ERROR, "Cannot add %s or not a BleBox device: %s",
             address.c_str(), ex.what());
  }
  return false;
}

// called from web UI
void BleBox::AsyncRemoveFeature(int device_record_id) {
  std::lock_guard<std::mutex> l(m_mutex); // since db has no constraints
  blebox::db::device_status::remove(device_record_id);
}

// called from web UI
void BleBox::AsyncRemoveAllFeatures() {
  std::lock_guard<std::mutex> l(m_mutex); // since db has no constraints

  blebox::db::device_status::remove_all(m_HwdID);
}

// called from web UI
void BleBox::SearchNetwork(const std::string &pattern) {
  static std::mutex m_mutex;
  static std::lock_guard<std::mutex> l(m_mutex);

  using namespace boost::asio;
  ip::network_v4 network;

  try {
    std::string first_ip(pattern);
    if (!first_ip.empty() && *first_ip.rbegin() == '*')
      *first_ip.rbegin() = '0';

    network = ip::network_v4(ip::make_address_v4(first_ip), 24);
  } catch (std::invalid_argument &) {
    _log.Log(LOG_ERROR,
             "Invalid or unsupported IP pattern: %s"
             " (expected e.g. 192.168.1.* or 192.168.1.0 or 192.168.1.255)",
             pattern.c_str());
    return;
  }

  auto &&hosts = network.hosts();

  std::vector<std::thread> searchingThreads;
  searchingThreads.reserve(hosts.size());

  for (auto &&ip : hosts)
    searchingThreads.emplace_back(&BleBox::AddOrUpdateHardware, "", ip, true,
                                  false, nullptr);

  for (auto &&thread : searchingThreads) {
    try {
      thread.join();
    } catch (std::exception &ex) {
      _log.Log(LOG_ERROR, "Unexpected error in a search task: %s", ex.what());
    }
  }
}

// Called from web interface
void BleBox::UpgradeFirmware() {
  std::lock_guard<std::mutex> l(m_mutex);
  // TODO: threaded + join?

  std::vector<blebox::db::hardware> boxes;
  blebox::db::hardware::all(boxes);
  for (const auto &box : boxes) {
    try {
      blebox::session client(box.ip(), DefaultFirmwareTimeout,
                             DefaultFirmwareTimeout);
      client.fetch("/api/ota/update");
    } catch (std::exception &ex) {
      const char *ip = box.ip().to_string().c_str();
      Log(LOG_ERROR, "Firmware update failed for %s because: %s", ip,
          ex.what());
    }
  }
}

// Called from web interface
void BleBox::DetectFeatures() {
  std::lock_guard<std::mutex> l(m_mutex);

  box().add_missing_supported_features(hw_id());
}

void BleBox::RefreshFeatures() {
  std::lock_guard<std::mutex> l(m_mutex);

  box().fetch(session(), *this);
}

void BleBox::UseFeatures() {
  std::lock_guard<std::mutex> l(m_mutex);

  blebox::db::device_status::use(hw_id());
}

#ifndef UNIT_TESTING
void BleBox::InternalDecodeRX(const std::string &name, const uint8_t *cmd,
                              const size_t size) {
  BOOST_ASSERT(size_t(cmd[0]) == size - 1);
  const uint8_t battery_level = 255;
  sDecodeRXMessage(this, cmd, name.c_str(), battery_level);
}
#else
void BleBox::InternalDecodeRX(const std::string &name, const uint8_t *cmd,
                              const size_t size) {
  BOOST_ASSERT(size_t(cmd[0]) == size - 1);
  // TODO: use less direct access (send + pop from message queue?)
  const uint8_t battery_level = 255;
  m_mainworker.ProcessRXMessage(this, cmd, name.c_str(), battery_level);
}
#endif

auto BleBox::session() -> blebox::session & {
  if (_session) // supplied by integration tests
    return *_session;

  // TODO: best to create a new HTTP/TCP session every time for now anyway
  const int timeout = 1;
  _internal_session = new blebox::session(_ip, timeout, timeout);
  return *_internal_session;
}

auto BleBox::box() -> blebox::box & {
  if (_box)
    return *_box;

  _box = blebox::box::from_session(session());

  if (!_unique_id.empty() && _box->unique_id() != _unique_id)
    throw blebox::errors::ip_mismatch(session().ip().to_string(), _unique_id,
                                      _box->type(), _box->unique_id());

  return *_box;
}

// force re-detecting box type (to detect IP changes)
void BleBox::reset_box() { _box.reset(); }

// Migration - moved here from separate file (hardware/blebox/db/migration.cpp) due to strange PR request I don't understand
#include "../main/SQLHelper.h"

namespace blebox {
namespace db {

static std::string extract_old_ip(const std::string &id) {
  uint8_t id1, id2, id3, id4;
  char ip[20];
  sscanf(id.c_str(), "%2hhx%2hhx%2hhx%2hhx", &id1, &id2, &id3, &id4);
  sprintf(ip, "%d.%d.%d.%d", id1, id2, id3, id4);
  return ip;
}

// NOTE: session is only for integration tests
bool migrate(session *session) {
  // TODO: if multiple devices exist for a hardware, we ignore some names if
  // they have the same IP. This is reasonable, since we don't map old features
  // to new features anyway. (We don't preserve automation setups here anyway).
  auto records = m_sql.safe_query("SELECT DISTINCT(DeviceID), MAX(Name) FROM "
                                  "DeviceStatus WHERE HardwareID in ("
                                  "SELECT ID FROM Hardware where Type = %d"
                                  ")",
                                  HTYPE_BleBox);

  // TODO: we lose the last values (which will be updated during the next
  // fetch() so we don't care) and we also lose any automation tied to those
  // devices. Two ways to do this are:
  //  1) Tie any automation to the new records (and delete the old records)
  //  2) Update the existing records (may be less easy and more error-prone)
  //
  // For now, I'm just dropping the old records but keeping the name so the user
  // can reconnect the features they want.

  // TODO: fixture out any other fields worth migrating (description?)
  m_sql.safe_query("DELETE FROM DeviceStatus WHERE HardwareID in ("
                   "SELECT ID FROM Hardware where Type = %d"
                   ")",
                   HTYPE_BleBox);
  m_sql.safe_query("DELETE FROM Hardware where Type = %d", HTYPE_BleBox);

  bool failures = false;

  for (const auto &fields : records) {
    auto addressIP = extract_old_ip(fields[0]);
    auto name = fields[1];

    try {
      auto ip = blebox::make_address_v4(addressIP);
      if (!BleBox::AddOrUpdateHardware(name, ip, false, true, session))
        failures = true;
    } catch (std::exception &ex) {
      _log.Log(LOG_ERROR,
               "Failed to migrate old records for BleBox hardware at '%s' (%s)",
               addressIP.c_str(), ex.what());
    }
  }

  return !failures;
}
} // namespace db
} // namespace blebox
