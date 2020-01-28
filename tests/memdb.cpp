#include <boost/assert.hpp>

#include <mutex>
#include <stdexcept>
#include <string>

#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/json_helper.h"
#include "../main/RFXNames.h" // for HTYPE_BleBox

#include "../hardware/blebox/ip.h"

#include "memdb.h"

static CSQLHelper &mdb() { return m_sql; }

static std::mutex &insert_safety_mutex() {
  static std::mutex m_insert_safety_mutex;
  return m_insert_safety_mutex;
}

// TODO: remove this method
auto MemDBSession::create_blebox_hardware(const std::string &unique_id,
                                          const blebox::ip &ip,
                                          const std::string &name,
                                          const std::string &type) -> int {
  using namespace std::literals;
  const std::string address = unique_id + ":"s + ip.to_string();

  std::lock_guard<std::mutex> last_insert_id_safety(insert_safety_mutex());

  Json::Value extra;
  extra["type"] = type;

  const char *raw_query =
      "INSERT INTO Hardware (address, type, name, extra) VALUES "
      "(%Q, %d, %Q, %Q)";

  auto results = mdb().safe_query(raw_query, address.c_str(), HTYPE_BleBox,
                                  name.c_str(), JSonToRawString(extra).c_str());
  auto results2 = mdb().safe_query("SELECT last_insert_rowid()");
  int record_id = std::stoi(results2.at(0).at(0));
  return record_id;
}

auto MemDBSession::create_blebox_feature(int hw_id, const std::string &name,
                                         const std::string &deviceid) -> int {
  std::lock_guard<std::mutex> last_insert_id_safety(insert_safety_mutex());

  const char *raw_query = "INSERT INTO DeviceStatus"
                          " (HardwareID, Name, DeviceID, Type, SubType)"
                          " VALUES (%d, %Q, %Q, %d, %d)";

  const uint8_t type = 0xF9;    // pTypeAirQuality as an example
  const uint8_t subtype = 0x02; // e.g. sTypeBleBoxAirSensor

  auto results = mdb().safe_query(raw_query, hw_id, name.c_str(),
                                  deviceid.c_str(), type, subtype);

  auto results2 = mdb().safe_query("SELECT last_insert_rowid()");
  int record_id = std::stoi(results2.at(0).at(0));
  return record_id;
}

MemDBSession::MemDBSession() {
  // TODO: bad way to check if db was opened/prepared
  std::string dbversion;
  if (!mdb().GetPreferencesVar("DB_Version", dbversion)) {
    _log.SetOutputFile("/dev/null");
    _log.SetDebugFlags("normal,hardware,received");
    _log.SetLogFlags("normal,status,error,debug");
    m_sql.SetDatabaseName(":memory:");
    BOOST_ASSERT(m_sql.OpenDatabase());
  }

  mdb().safe_query("BEGIN TRANSACTION");
}

MemDBSession::~MemDBSession() { mdb().safe_query("ROLLBACK"); }
