#include <gmock/gmock.h>

#include <boost/assert.hpp>

#include "../../../../hardware/BleBox.h"
#include "../../../../hardware/blebox/box.h"
#include "../../../../hardware/blebox/errors.h"

#include "../../../memdb.h"
#include "../../../worker_helper.h"

#include "../mocks.h"

#include "fixtures/tempSensor_fixtures.h"

namespace TempSensorTest {

class IdentifiedTempSensor : public IdentifiedBox {
  std::string default_status() const final { return tempSensor_status_ok1; }
};

namespace features {
static auto temperature() {
  test_helper::device_status dev;
  dev.battery_level = 255;
  dev.signal_level = 15;

  dev.hw_id = 234;

  dev.type = 0x50;   // pTypeTemp
  dev.sub_type = 0x05; // sTypeTEMPBleBox = sTypeTEMP5

  dev.switch_type = 0;

  dev.node_type_identifier = "45216"; // 0xB0A0,  0xE0 - method, 0xA0 - unit(0)
  dev.unit = 0xA0 + 0;

  dev.name = "My tempSensor.0.temp";
  dev.numeric_value = 0;

  dev.description = "";
  dev.color = "";
  dev.last_level = 0;
  return dev;
}
} // namespace features

TEST(TempSensor, Identify) {
  MemDBSession mem_session;

  auto ip = blebox::make_address_v4("192.168.1.11");
  ::testing::StrictMock<GMockSession> session(ip, 1, 1);

  EXPECT_CALL(session, fetch_raw("/api/device/state"))
      .WillOnce(::testing::Return(tempSensor_status_ok1));
  auto box = blebox::box::from_session(session);

  ASSERT_STREQ(box->type().c_str(), "tempSensor");
  ASSERT_STREQ(box->hardware_version().c_str(), "0.6");
  ASSERT_STREQ(box->firmware_version().c_str(), "0.176");
  ASSERT_STREQ(box->unique_id().c_str(), "1afe34db9437");
  ASSERT_STREQ(box->name().c_str(), "My tempSensor");

  ASSERT_STREQ(box->api_version().c_str(), "1.1.0");
}

TEST_F(IdentifiedTempSensor, UpdateWithoutFeatures) {
  MemDBSession mem_session;

  BleBox bb(123, "192.168.1.17", 30);

  EXPECT_CALL(_session, fetch_raw("/api/tempsensor/state"))
      .WillOnce(::testing::Return(tempSensor_state_ok1));
  box->fetch(_session, bb);

  std::set<test_helper::device_status> all_expected;
  ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
            test_helper::device_status::json());
}

TEST_F(IdentifiedTempSensor, AddSupportedFeatures) {
  MemDBSession mem_session;

  BleBox bb(234, "192.168.1.17", 30);
  box->add_missing_supported_features(bb.hw_id());

  EXPECT_CALL(_session, fetch_raw("/api/tempsensor/state"))
      .WillOnce(::testing::Return(tempSensor_state_ok1));
  box->fetch(_session, bb);

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::temperature();
      expected.string_value = "22.1";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(IdentifiedTempSensor, NoUpdateWhenErrorState) {
  MemDBSession mem_session;

  BleBox bb(234, "192.168.1.17", 30);
  box->add_missing_supported_features(bb.hw_id());

  EXPECT_CALL(_session, fetch_raw("/api/tempsensor/state"))
      .WillOnce(::testing::Return(tempSensor_state_error_status));
  box->fetch(_session, bb);

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::temperature();
      expected.string_value = "22.1";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}
} // namespace TempSensorTest
