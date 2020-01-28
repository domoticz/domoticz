#pragma once

#include <vector>

// brain-dead minimum orm to help in tests

namespace testdb {

struct generic_table {
  template <typename T> static std::string select() {
    using namespace std::literals;
    return "SELECT "s + T::field_names() + " FROM "s + T::table_name();
  }

  template <typename T> static void all(std::vector<T> &results) {
    results.clear();

    auto &&records = query(select<T>());
    for (auto &&fields : records)
      results.emplace_back(fields);
  }

  typedef std::vector<std::vector<std::string>> sql_results;

  static sql_results query(const std::string &sql);

  static const std::string with_where_id(const std::string &sql) {
    return sql + " WHERE ID = %d";
  }
};

struct hardware : public generic_table {
  hardware(const std::vector<std::string> &fields)
      : record_id(std::stoi(fields[0])),
        mode1(std::stoi(fields[1])),
        type(std::stoi(fields[2])),
        address(fields[3]),
        name(fields[4]) {}

  static std::string table_name() { return "Hardware"; }
  static std::string field_names() { return "id, mode1, type, address, name"; }

  int record_id{0xBADBEEF};
  int mode1{0xBADBEEF};
  int type{0xBADBEEF};
  std::string address;
  std::string name;

  // TODO: make generic
  static hardware find(int record_id);

  void save();
};

struct device_status : public generic_table {
  device_status(const std::vector<std::string> &fields)
      : record_id(std::stoi(fields[0])),
        name(fields[1]),
        type(std::stoi(fields[2])) {}

  static std::string table_name() { return "DeviceStatus"; }
  static std::string field_names() { return "id, name, type"; }

  int record_id{0xBADBEEF};
  std::string name;
  int type{0xBADBEEF};
};
} // namespace testdb
