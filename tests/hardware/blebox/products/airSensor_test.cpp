#include <gmock/gmock.h>

#include <boost/assert.hpp>

#include "../../../../hardware/BleBox.h"
#include "../../../../hardware/blebox/box.h"
#include "../../../../hardware/blebox/errors.h"

#include "../../../memdb.h"
#include "../../../worker_helper.h"

#include "../mocks.h"

#include "fixtures/airSensor_fixtures.h"

namespace airSensorTest {

class IdentifiedAirSensor : public IdentifiedBox {
  std::string default_status() const final { return airSensor_status_ok1; }
};

class WebControlledAirSensor : public WorkerScenario {
  std::string default_status() const final { return airSensor_status_ok1; }

  void add_state_fetch_expectations(session_type &session) final {
    EXPECT_CALL(session, fetch_raw("/api/air/state"))
        .WillOnce(::testing::Return(airSensor_state_ok1));
  }
};

namespace features {
static auto value() {
  test_helper::device_status dev;
  dev.battery_level = 255;
  dev.signal_level = 12;

  dev.hw_id = 234;

  dev.type = 0xF9;  // pTypeAirQuality
  dev.sub_type = 1; // sTypeBleboxAirSensor = sTypeVoltcraft = 0x01

  dev.switch_type = 0;

  dev.node_type_identifier = "176"; // 0xB0

  dev.color = "";
  dev.last_level = 0;

  dev.description = "";
  dev.string_value = "";
  return dev;
}

static auto pm1() {
  auto dev = value();
  dev.name = "My backyard air sensor.pm1.value";
  dev.unit = 0xA0 + 0;
  return dev;
}

static auto pm2() {
  auto dev = value();
  dev.name = "My backyard air sensor.pm2.5.value";
  dev.unit = 0xA0 + 1;
  return dev;
}

static auto pm10() {
  auto dev = value();
  dev.name = "My backyard air sensor.pm10.value";
  dev.unit = 0xA0 + 2;
  return dev;
}

static auto kick() {
  test_helper::device_status dev;
  dev.battery_level = 255;
  dev.signal_level = 12;

  dev.hw_id = 234;

  dev.type = 0xF4;     // pTypeGeneralSwitch
  dev.sub_type = 0x49; // sSwitchGeneralSwitch
  dev.switch_type = 9; // STYPE_PushOn

  dev.name = "My backyard air sensor.kick";
  dev.numeric_value = 0;
  dev.unit = 0xFF;

  dev.node_type_identifier = "000000B0";

  dev.color = "";
  dev.last_level = 0;

  dev.description = "";
  dev.string_value = "(added)";
  return dev;
}
} // namespace features

TEST(AirSensor, Identify) {
  MemDBSession mem_session;

  static auto ip = blebox::make_address_v4("192.168.1.11");
  ::testing::StrictMock<GMockSession> session(ip, 1, 1);

  EXPECT_CALL(session, fetch_raw("/api/device/state"))
      .WillOnce(testing::Return(airSensor_status_ok1));
  auto box = blebox::box::from_session(session);

  ASSERT_STREQ(box->type().c_str(), "airSensor");
  ASSERT_STREQ(box->hardware_version().c_str(), "0.6");
  ASSERT_STREQ(box->firmware_version().c_str(), "0.973");
  ASSERT_STREQ(box->unique_id().c_str(), "1afe34db9437");
  ASSERT_STREQ(box->name().c_str(), "My backyard air sensor");

  ASSERT_STREQ(box->api_version().c_str(), "1.1.0");
}

TEST_F(IdentifiedAirSensor, UpdateWithoutFeatures) {
  MemDBSession mem_session;

  BleBox bb(123, "192.168.1.17", 30);

  EXPECT_CALL(_session, fetch_raw("/api/air/state"))
      .WillOnce(::testing::Return(airSensor_state_ok1));
  box->fetch(_session, bb);

  std::set<test_helper::device_status> all_expected;
  ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
            test_helper::device_status::json());
}

TEST_F(IdentifiedAirSensor, UpdateWhenStateOk) {
  MemDBSession mem_session;

  EXPECT_CALL(_session, fetch_raw("/api/air/state"))
      .WillOnce(::testing::Return(airSensor_state_ok1));

  BleBox bb(234, "192.168.1.17", 30);
  box->add_missing_supported_features(bb.hw_id());
  box->fetch(_session, bb);

  {
    std::set<test_helper::device_status> all_expected;

    {
      auto expected = features::pm1();
      expected.numeric_value = 111;
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::pm2();
      expected.numeric_value = 222;
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::pm10();
      expected.numeric_value = 333;
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::kick();
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(IdentifiedAirSensor, NoUpdateWhenErrorState) {
  MemDBSession mem_session;

  BleBox bb(234, "192.168.1.17", 30);
  box->add_missing_supported_features(bb.hw_id());

  EXPECT_CALL(_session, fetch_raw("/api/air/state"))
      .WillOnce(::testing::Return(airSensor_state_error_but_one_ok));
  box->fetch(_session, bb);

  {
    std::set<test_helper::device_status> all_expected;

    // NOTE: error state here
    {
      auto expected = features::pm1();
      expected.numeric_value = 111;
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    // NOTE: error - null value here
    {
      auto expected = features::pm2();
      expected.string_value = "(added)"; // but not updated
      expected.numeric_value = 0;
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::pm10();
      expected.numeric_value = 444;
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::kick();
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(WebControlledAirSensor, Kick) {
  EXPECT_CALL(_session, fetch_raw(std::string("/api/air/kick")))
      .WillOnce(::testing::Return(airSensor_state_after_kick));

  ASSERT_TRUE(worker_helper::push(idx("000000B0")));

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::pm1();
      expected.numeric_value = 111;
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::pm2();
      expected.numeric_value = 222;
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::pm10();
      expected.numeric_value = 333;
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::kick();
      expected.numeric_value = 1;
      expected.string_value = "0";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}
} // namespace airSensorTest
