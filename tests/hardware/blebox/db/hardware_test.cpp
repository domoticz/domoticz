#include <gmock/gmock.h>

#include <stdexcept>
#include <vector>

#include "../../../memdb.h"
#include "../../../test_helper.h"
#include "../../../testdb.h"

#include "../../../../hardware/blebox/db/annotate.h"
#include "../../../../hardware/blebox/db/device_status.h"
#include "../../../../hardware/blebox/db/hardware.h"
#include "../../../../hardware/blebox/ip.h"

namespace foobar {
struct bar {
  static auto annotated_example(int, const char *) -> std::string {
    return blebox::db::annotate(BOOST_CURRENT_FUNCTION,
                                "SELECT * from example");
  }
};
} // namespace foobar

TEST(HardwareTest, Annotated) {
  using namespace std::literals;

  MemDBSession session;
  ASSERT_EQ(foobar::bar::annotated_example(123, "foobar"),
            "/* foobar::bar::annotated_example() */ SELECT * from example"s);
}

TEST(HardwareTest, FindExistingHardwareIP) {
  MemDBSession session;

  std::string unique_id = "abcd1234efgh";
  blebox::ip ip = blebox::make_address_v4("192.168.1.5");

  int record_id = session.create_blebox_hardware(
      unique_id, ip, "My BleBox hardware", "airSensor");

  ASSERT_EQ(ip, blebox::db::hardware::ip_for(record_id));
}

TEST(HardwareTest, HandleNoIP) {
  MemDBSession session;

  std::string unique_id = "abcd1234efgh";
  blebox::ip ip = blebox::make_address_v4("192.168.1.5");

  int record_id = session.create_blebox_hardware(
      unique_id, ip, "My BleBox hardware", "airSensor");

  auto &&record = testdb::hardware::find(record_id);
  record.address = "abcd1234efgh:";
  record.save();

  ASSERT_THROW(blebox::db::hardware::ip_for(record_id), std::invalid_argument);
}

TEST(HardwareTest, UpdateExistingHardwareWithSameIP) {
  MemDBSession session;

  std::string unique_id = "abcd1234efgh";
  blebox::ip ip = blebox::make_address_v4("192.168.1.5");
  session.create_blebox_hardware(unique_id, ip, "My BleBox hardware",
                                 "tempSensor");

  int record_id = 0;          // output
  bool is_new_record = false; // output
  std::string insert_name = "new hardware";
  ASSERT_EQ(true, blebox::db::hardware::add_or_update_by_unique_id(
                      unique_id, ip, is_new_record, insert_name, record_id,
                      "tempSensor"));

  ASSERT_EQ(ip, blebox::db::hardware::ip_for(record_id));
  ASSERT_EQ(is_new_record, false);
}

TEST(HardwareTest, UpdateExistingHardwareWithNewIP) {
  MemDBSession session;

  std::string unique_id = "abcd1234efgh";
  blebox::ip existing_ip = blebox::make_address_v4("192.168.0.100");
  session.create_blebox_hardware(unique_id, existing_ip, "My BleBox hardware",
                                 "airSensor");

  int record_id = 0;          // output
  bool is_new_record = false; // output
  std::string insert_name = "new hardware";
  blebox::ip new_ip = blebox::make_address_v4("192.168.7.8");
  ASSERT_EQ(true, blebox::db::hardware::add_or_update_by_unique_id(
                      unique_id, new_ip, is_new_record, insert_name, record_id,
                      "airSensor"));

  ASSERT_EQ(new_ip, blebox::db::hardware::ip_for(record_id));
  ASSERT_EQ(is_new_record, false);
}

TEST(HardwareTest, InsertNewHardwareWithNewIP) {
  MemDBSession session;

  std::string unique_id = "abcd1234efgh";

  int record_id = 0;          // output
  bool is_new_record = false; // output
  std::string insert_name = "new hardware";
  blebox::ip new_ip = blebox::make_address_v4("192.168.7.8");
  ASSERT_EQ(true, blebox::db::hardware::add_or_update_by_unique_id(
                      unique_id, new_ip, is_new_record, insert_name, record_id,
                      "airSensor"));

  ASSERT_EQ(new_ip, blebox::db::hardware::ip_for(record_id));
  ASSERT_EQ(is_new_record, true);
}

TEST(HardwareTest, TestUpdateSetting) {
  MemDBSession session;

  blebox::ip ip = blebox::make_address_v4("192.168.0.100");
  int record_id = session.create_blebox_hardware(
      "abcd1234efgh", ip, "My BleBox hardware", "saunaBox");

  blebox::db::hardware::update_settings(record_id, 77);

  auto hw = testdb::hardware::find(record_id);
  ASSERT_EQ(77, hw.mode1);
}

TEST(HardwareTest, RemoveAllFeatures) {
  using namespace std::literals;

  MemDBSession session;

  int record_id1;

  {
    blebox::ip ip = blebox::make_address_v4("192.168.0.100");
    record_id1 =
        session.create_blebox_hardware("abcd1234efgh", ip, "box1", "airSensor");
    session.create_blebox_feature(record_id1, "My feature1", "1234");
    session.create_blebox_feature(record_id1, "My feature2", "2345");
  }

  {
    blebox::ip ip = blebox::make_address_v4("192.168.3.4");
    int record_id = session.create_blebox_hardware("xyz123456789", ip, "box2",
                                                   "tempSensor");
    session.create_blebox_feature(record_id, "My featureX", "5678");
  }

  blebox::db::device_status::remove_all(record_id1);

  {
    std::vector<testdb::hardware> results;
    testdb::hardware::all(results);

    auto blebox_count =
        std::count_if(results.begin(), results.end(),
                      [](auto &item) { return item.type == 82; });
    ASSERT_EQ(2, blebox_count);
    auto other_count = long(results.size()) - blebox_count;
    ASSERT_EQ(1, other_count); // internal domoticz record
  }

  {
    std::vector<testdb::device_status> results;
    testdb::device_status::all(results);
    ASSERT_EQ(1, results.size());
    ASSERT_EQ("My featureX"s, results[0].name);
  }
}
