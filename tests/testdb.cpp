#include <boost/format.hpp>

#include <utility>
#include <vector>

#include "../main/SQLHelper.h"

#include "testdb.h"

static CSQLHelper &db() { return m_sql; }

testdb::generic_table::sql_results
testdb::generic_table::query(const std::string &sql) {
  return db().safe_query(sql.c_str());
}

testdb::hardware testdb::hardware::find(int record_id) {
  auto &&query = with_where_id(select<hardware>());
  auto &&records = db().safe_query(query.c_str(), record_id);

  if (records.size() != 1)
    throw std::runtime_error(
        (boost::format("expected 1 record, found %d") % records.size()).str());

  return records[0]; // implicit
}

void testdb::hardware::save() {
  using namespace std::literals;
  std::string sql = "UPDATE "s + hardware::table_name() +
                    " SET mode1=%d, type=%d, address=%Q, name=%Q";

  auto query = with_where_id(sql);
  db().safe_query(query.c_str(), mode1, type, address.c_str(), name.c_str(),
                  record_id);
}

// device_status table
