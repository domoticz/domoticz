#pragma once

#include <string>
#include <utility>
#include <vector>

#include "../ip.h"

namespace blebox {
namespace db {
class hardware {
public:
  // update by device's factory id
  static bool add_or_update_by_unique_id(const std::string &unique_id,
                                         blebox::ip ip, bool &new_record,
                                         const std::string &insert_name,
                                         int &record_id,
                                         const std::string &type);

  static bool all(std::vector<hardware> &boxes);
  static auto ip_for(int hw_id) -> blebox::ip;
  static void update_settings(int hw_id, int poll_interval);

  // TODO: for now, it's just a db record wrapper to get the id
  hardware(int hw_id, blebox::ip ip) : _hw_id(hw_id), _ip(std::move(ip)) {}

  auto hw_id() const { return _hw_id; }
  auto ip() const { return _ip; }

private:
  int _hw_id;
  blebox::ip _ip;
};
} // namespace db
} // namespace blebox
