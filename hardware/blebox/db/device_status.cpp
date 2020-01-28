#include <algorithm>
#include <boost/current_function.hpp>
#include <boost/format.hpp>
#include <cstddef>

#include "../../../main/SQLHelper.h"

#include "../workarounds/gcc_6.h"

#include "annotate.h"

#include "device_status.h"

// TODO: refactor whole class with an ORM
blebox::db::device_status::record_not_found::record_not_found(
    int hw_id, const std::string &dev_id, int unit)
    : std::runtime_error(
          (boost::format("device_status not found: (hw:%d/devid:%s/unit:%d)") %
           hw_id % dev_id % unit)
              .str()) {}

blebox::db::device_status::device_status(int record_id) {
  // TODO: coverage
  const char *query =
      "SELECT ID, Unit, Name, sValue, Description, Color, LastLevel, switchtype"
      " FROM DeviceStatus WHERE (ID=='%d')";

  auto result = m_sql.safe_query(
      blebox::db::annotate(BOOST_CURRENT_FUNCTION, query).c_str(), record_id);

  if (result.empty())
    throw record_not_found(record_id);

  const auto &fields = result[0];

  _id = std::stoi(fields[0]);
  _unit = std::stoi(fields[1]);
  _name = fields[2];
  _value = fields[3];
  _description = fields[4];
  _color = fields[5];
  _last_level = std::stoi(fields[6]);
  _switchtype = _eSwitchType(std::stoi(fields[7]));
}

// called from device during post (mostly just to get switch type and color)
blebox::db::device_status::device_status(int hw_id, const std::string &dev_id,
                                         int unit) {
  const char *query =
      "SELECT ID, Unit, Name, sValue, Description, Color, LastLevel, switchtype"
      " FROM DeviceStatus WHERE (HardwareID=%d) and (DeviceID=%Q) and "
      "(Unit=%d)";

  auto &&result = m_sql.safe_query(
      blebox::db::annotate(BOOST_CURRENT_FUNCTION, query).c_str(), hw_id,
      dev_id.c_str(), unit);

  if (result.empty())
    throw record_not_found(hw_id, dev_id, unit); // TODO: coverage

  auto &&fields = result[0];

  // TODO: check ranges, etc.
  _id = std::stoi(fields[0]);
  _unit = std::stoi(fields[1]);
  _name = fields[2];
  _value = fields[3];
  _description = fields[4];
  _color = fields[5];
  _last_level = std::stoi(fields[6]);
  _switchtype = _eSwitchType(std::stoi(fields[7]));
}

// called from web
void blebox::db::device_status::remove(const int id) {
  // TODO: coverage
  const char *query = "DELETE FROM DeviceStatus WHERE (ID=='%d')";
  m_sql.safe_query(blebox::db::annotate(BOOST_CURRENT_FUNCTION, query).c_str(),
                   id);
}

// called from web ("clear nodes" by admin user)
void blebox::db::device_status::remove_all(int hw_id) {
  const char *query = "DELETE FROM DeviceStatus WHERE (HardwareID==%d)";
  m_sql.safe_query(blebox::db::annotate(BOOST_CURRENT_FUNCTION, query).c_str(),
                   hw_id);
}

bool blebox::db::device_status::all(
    int hw_id, std::vector<blebox::db::device_status> &output) {
  // TODO: coverage
  const char *query = "SELECT ID, Unit, Name, sValue, Description FROM "
                      "DeviceStatus WHERE (HardwareID=='%d')";
  auto result = m_sql.safe_query(
      blebox::db::annotate(BOOST_CURRENT_FUNCTION, query).c_str(), hw_id);
  if (result.empty())
    return false;

  for (const auto &fields : result) {
    int record_id = stoi(fields[0]);
    int unit = stoi(fields[1]);
    const std::string &name = fields[2];
    const std::string &value = fields[3];
    const std::string &description = fields[4];

    output.emplace_back(record_id, unit, name, value, description);
  }

  return true;
}

bool blebox::db::device_status::get_values(int &nvalue, std::string &svalue,
                                           int hw_id, std::byte node_id,
                                           int ChildID, int packet_type,
                                           int sub_type) {
  std::string dev_id = (boost::format("%08X") % int(node_id)).str();
  auto result = m_sql.safe_query(
      "SELECT nValue, sValue FROM DeviceStatus WHERE (HardwareID==%d) AND "
      "(DeviceID==%Q) AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)",
      hw_id, dev_id.c_str(), ChildID, packet_type, sub_type);

  if (result.empty())
    return false;

  // TODO: coverage
  const auto &fields = result[0];
  nvalue = std::stoi(fields[0]);
  svalue = fields[1];
  return true;
}

void blebox::db::device_status::use(int hw_id) {
  // TODO: coverage
  m_sql.safe_query("UPDATE DeviceStatus SET Used=1 WHERE (HardwareID==%d)",
                   hw_id);
}

// TODO: remove (merge code into above all() method)
blebox::db::sql_result blebox::db::device_status::all_for_hw(int hw_id) {
  const char *query = "SELECT DeviceID, Unit, Type, SubType, SwitchType Name "
                      "FROM DeviceStatus WHERE (HardwareID=%d)";
  return m_sql.safe_query(query, hw_id);
}
