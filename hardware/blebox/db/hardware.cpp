#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/assert.hpp>
#include <boost/current_function.hpp>
#include <ext/alloc_traits.h>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../../main/Logger.h"
#include "../../../main/RFXNames.h"
#include "../../../main/SQLHelper.h"
#include "../../../main/json_helper.h"

#include "annotate.h"

#include "hardware.h"

// Simple SQL convenience wrappers for Hardware Table
const int DB_HARDWARE_DEVICE_ENABLED = 1;

static bool extract_unique_id_and_address(std::string &unique_id,
                                          blebox::ip &ip,
                                          const std::string &value) {
  // TODO: log about invalid value?
  if (std::count(value.begin(), value.end(), ':') != 1)
    return false; // TODO: coverage

  std::vector<std::string> both;
  boost::split(both, value, boost::is_any_of(":"));
  unique_id = both[0];
  try {
    ip = blebox::make_address_v4(both[1]);
  } catch (boost::system::system_error &) {
    throw std::invalid_argument("empty or invalid IP address");
  }
  return !ip.is_unspecified();
}

// TODO: all dangerous record operations should check for BleBox type (82)

// TODO: cleanup
static bool insert(const std::string &name, const std::string &unique_id,
                   blebox::ip ip, int &record_id, const std::string &type) {
  std::string field = unique_id + std::string(":") + ip.to_string();

  static std::mutex m_insert_safety_mutex;
  std::lock_guard<std::mutex> last_insert_id_safety(m_insert_safety_mutex);

  Json::Value extra;
  extra["type"] = type;
  auto extra_json = JSonToRawString(extra);

  // IP available - update self
  const char *query =
      "INSERT INTO Hardware (Name, Address, Type, Enabled, Extra) "
      "VALUES (%Q, %Q, %d, %d, %Q);";

  const std::string annotated =
      blebox::db::annotate(BOOST_CURRENT_FUNCTION, query);
  auto results = m_sql.safe_query(
      annotated.c_str(), name.c_str(), field.c_str(), HTYPE_BleBox,
      DB_HARDWARE_DEVICE_ENABLED, extra_json.c_str());

  auto results2 = m_sql.safe_query("SELECT last_insert_rowid()");

  if (results2.empty()) {
    _log.Log(LOG_ERROR, "Failed to insert BleBox hardware: %s", name.c_str());
    return false;
  }

  record_id = std::stoi(results2.at(0).at(0));

  return true;
}

static bool find_by_record_id(std::string &unique_id, blebox::ip &ip,
                              int record_id) {
  const char *query = "SELECT address FROM Hardware WHERE (ID=%d)";
  std::string annotated = blebox::db::annotate((BOOST_CURRENT_FUNCTION), query);
  auto result = m_sql.safe_query(annotated.c_str(), record_id);

  // TODO: error handling
  // TODO: if error is a result of visiting a page for a deleted hardware,
  // redirect to 404 and return to hardware list instead
  if (result.empty())
    throw std::runtime_error("record not found for hardware: " +
                             std::to_string(record_id));

  return extract_unique_id_and_address(unique_id, ip, result[0][0]);
}

// TODO: sanitize unique_id and address against ":" occurrences
// TODO: other params (port) - but use an ORM instead to handle this

// update by raw Domoticz HWdID
// TODO: rename to update_address
static bool update_by_record_id(CSQLHelper &sql, int record_id,
                                const std::string &unique_id, blebox::ip ip,
                                const std::string &type) {
  // TODO: fail if no records updated?
  std::string field = unique_id + std::string(":") + ip.to_string();

  const char *query = "UPDATE Hardware SET address=%Q, extra=%Q WHERE ID=%d";
  std::string annotated = blebox::db::annotate(BOOST_CURRENT_FUNCTION, query);

  Json::Value extra;
  extra["type"] = type;
  auto extra_json = JSonToRawString(extra);

  sql.safe_query(annotated.c_str(), field.c_str(), extra_json.c_str(),
                 record_id);

  return true;
}

static bool find_by_unique_id(int &record_id, blebox::ip &ip,
                              const std::string &unique_id) {
  const char *query =
      "SELECT ID, address FROM Hardware WHERE (Address LIKE '%q:%%')";

  std::string annotated = blebox::db::annotate(BOOST_CURRENT_FUNCTION, query);
  auto result = m_sql.safe_query(annotated.c_str(), unique_id.c_str());
  if (result.empty())
    return false;

  // TODO: exceptions
  record_id = stoi(result[0][0]);

  std::string tmp;
  const std::string &field = result[0][1];
  if (!extract_unique_id_and_address(tmp, ip, field)) {
    // TODO: raise here (invalid database state) and handle
    // TODO: database constraints
    _log.Log(LOG_ERROR, "Could not extract blebox unique_id + address from: %s",
             field.c_str());
    return false;
  }

  BOOST_ASSERT(tmp == unique_id);
  return true;
}

bool blebox::db::hardware::add_or_update_by_unique_id(
    const std::string &unique_id, blebox::ip new_ip, bool &new_record,
    const std::string &insert_name, int &record_id, const std::string &type) {
  new_record = false;
  blebox::ip existing_ip;
  if (find_by_unique_id(record_id, existing_ip, unique_id)) {
    // TODO: inform about IP being updated?
    // TODO: error handling
    // TODO: rename method since it only really updates the IP
    if (existing_ip != new_ip)
      return update_by_record_id(m_sql, record_id, unique_id, new_ip, type);
    return true;
  }

  // TODO: find by address+port just in case? maybe delete?
  // TODO: error handling
  if (!insert(insert_name, unique_id, new_ip, record_id, type)) {
    _log.Log(LOG_ERROR, "Failed to insert BleBox hardware: %s (%s)",
             insert_name.c_str(), type.c_str());
    return false;
  }

  new_record = true;
  return true;
}

bool blebox::db::hardware::all(std::vector<blebox::db::hardware> &boxes) {
  // TODO: coverage
  const char *query = "SELECT ID, Address FROM Hardware WHERE "
                      "Type=%d and Address like '%%:%%'";
  std::string annotated = blebox::db::annotate(BOOST_CURRENT_FUNCTION, query);
  auto records = m_sql.safe_query(annotated.c_str(), HTYPE_BleBox);
  if (records.empty())
    return false;

  for (const auto &fields : records) {
    const auto &id = stoi(fields[0]);

    std::string unique_id;
    blebox::ip ip;
    if (!extract_unique_id_and_address(unique_id, ip, fields[1]))
      continue;

    blebox::db::hardware hw(id, ip);
    boxes.push_back(hw);
  }
  return true;
}

auto blebox::db::hardware::ip_for(int hw_id) -> blebox::ip {
  blebox::ip ip;
  std::string unique_id;
  if (!find_by_record_id(unique_id, ip, hw_id))
    throw std::runtime_error("record not found for hardware: " +
                             std::to_string(hw_id));
  return ip;
}

void blebox::db::hardware::update_settings(int hw_id, int poll_interval) {
  const char *query = "UPDATE Hardware SET Mode1=%d WHERE (ID == %d)";
  std::string annotated = blebox::db::annotate(BOOST_CURRENT_FUNCTION, query);
  m_sql.safe_query(annotated.c_str(), poll_interval, hw_id);
}
