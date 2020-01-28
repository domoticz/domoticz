#include <gmock/gmock.h>

#include <boost/assert.hpp>

#include "../../../../hardware/BleBox.h"
#include "../../../../hardware/blebox/box.h"
#include "../../../../hardware/blebox/errors.h"

#include "../../../memdb.h"
#include "../../../worker_helper.h"

#include "../mocks.h"

#include "fixtures/saunaBox_fixtures.h"

namespace saunaBoxSTest {

class IdentifiedSaunaBox : public IdentifiedBox {
  std::string default_status() const final { return saunaBox_status_ok1; }
};

class WebControlledSaunaBox : public WorkerScenario {
  std::string default_status() const final { return saunaBox_status_ok1; }

  void add_state_fetch_expectations(session_type &session) final {
    EXPECT_CALL(session, fetch_raw("/api/heat/state"))
        .WillOnce(::testing::Return(saunaBox_state_ok1));
  }
};

class WebControlledSaunaBox2 : public WorkerScenario {
  std::string default_status() const final { return saunaBox_status_ok1; }

  void add_state_fetch_expectations(session_type &session) final {
    EXPECT_CALL(session, fetch_raw("/api/heat/state"))
        .WillOnce(::testing::Return(saunaBox_state_after_set_off));
  }
};

namespace features {
static auto common() {
  test_helper::device_status dev;
  dev.battery_level = 255;
  dev.signal_level = 12;

  dev.hw_id = 234;

  dev.switch_type = 0;

  dev.description = "";
  dev.numeric_value = 0;

  dev.color = "";
  dev.last_level = 0;
  return dev;
}

static auto current_temperature() {
  auto dev = common();
  dev.name = "My saunaBox 1.0.temp";

  dev.type = 0x50;    // pTypeTEMP
  dev.sub_type = 0x05; // sTypeTEMPBleBox = sTypeTEMP5

  dev.node_type_identifier = "45216"; // 0xB0A0, F2 - method, A0 - unit
  dev.unit = 0xA0 + 0;

  dev.signal_level = 15; // why not 12?
  return dev;
}

static auto desired_temperature() {
  auto dev = common();
  dev.type = 0xF2;  // pTypeThermostat
  dev.sub_type = 1; // sTypeThermSetpoint

  dev.node_type_identifier = "000FFB0"; // unit=0xFF, method=B0
  dev.unit = 0xFF;                      // self method

  dev.name = "My saunaBox 1.desiredTemp";
  return dev;
}

static auto state() {
  auto dev = common();
  dev.type = 0xF4;     // pTypeGeneralSwitch
  dev.sub_type = 0x49; // sSwitchGeneralSwitch
  dev.switch_type = STYPE_OnOff;

  dev.node_type_identifier = "000000B1";
  dev.unit = 0xFF; // self method

  dev.name = "My saunaBox 1.state";
  return dev;
}

} // namespace features

TEST(SaunaBox, Identify) {
  MemDBSession mem_session;

  static const blebox::ip ip = blebox::make_address_v4("192.168.1.11");
  ::testing::StrictMock<GMockSession> session(ip, 1, 1);

  EXPECT_CALL(session, fetch_raw("/api/device/state"))
      .WillOnce(::testing::Return(saunaBox_status_ok1));
  auto box = blebox::box::from_session(session);

  ASSERT_STREQ(box->type().c_str(), "saunaBox");
  ASSERT_STREQ(box->hardware_version().c_str(), "0.6");
  ASSERT_STREQ(box->firmware_version().c_str(), "0.176");
  ASSERT_STREQ(box->unique_id().c_str(), "1afe34db9437");
  ASSERT_STREQ(box->name().c_str(), "My saunaBox 1");

  ASSERT_STREQ(box->api_version().c_str(), "1.1.0");
}

TEST_F(IdentifiedSaunaBox, UpdateWithoutFeatures) {
  MemDBSession mem_session;

  BleBox bb(123, "192.168.1.17", 30);

  EXPECT_CALL(_session, fetch_raw("/api/heat/state"))
      .WillOnce(::testing::Return(saunaBox_state_ok1));
  box->fetch(_session, bb);

  std::set<test_helper::device_status> all_expected;
  ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
            test_helper::device_status::json());
}

TEST_F(IdentifiedSaunaBox, AddSupportedFeatures) {
  MemDBSession mem_session;

  BleBox bb(234, "192.168.1.17", 30);
  box->add_missing_supported_features(bb.hw_id());

  EXPECT_CALL(_session, fetch_raw("/api/heat/state"))
      .WillOnce(::testing::Return(saunaBox_state_ok1));
  box->fetch(_session, bb);

  {
    std::set<test_helper::device_status> all_expected;

    {
      auto expected = features::current_temperature();
      expected.string_value = "67.9";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::desired_temperature();
      expected.string_value = "71.26";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::state();
      expected.string_value = "0";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(IdentifiedSaunaBox, NoUpdateWhenErrorState) {
  MemDBSession mem_session;

  BleBox bb(234, "192.168.1.17", 30);
  box->add_missing_supported_features(bb.hw_id());

  EXPECT_CALL(_session, fetch_raw("/api/heat/state"))
      .WillOnce(::testing::Return(saunaBox_state_error));
  box->fetch(_session, bb);

  {
    std::set<test_helper::device_status> all_expected;

    {
      auto expected = features::current_temperature();
      expected.string_value = "45.7";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::desired_temperature();
      expected.string_value = "71.26";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::state();
      expected.string_value = "0";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(WebControlledSaunaBox, SetPoint) {
  EXPECT_CALL(_session, fetch_raw("/s/t/3260"))
      .WillOnce(::testing::Return(saunaBox_state_after_setpoint_3260));

  ASSERT_TRUE(worker_helper::thermo_point_set(idx("000FFB0"), 32.6F));

  {
    std::set<test_helper::device_status> all_expected;

    {
      auto expected = features::current_temperature();
      expected.string_value = "54.3";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::desired_temperature();
      expected.string_value = "32.60";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::state();
      expected.string_value = "0";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }
    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(WebControlledSaunaBox, SetOn) {
  //EXPECT_POST("/api/heat/set", with_state(1), saunaBox_state_after_set_on);

  EXPECT_CALL(_session, fetch_raw("/s/1"))
      .WillOnce(::testing::Return(saunaBox_state_after_set_on));

  ASSERT_TRUE(worker_helper::switch_on(idx("000000B1")));

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::current_temperature();
      expected.string_value = "28.8";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::desired_temperature();
      expected.string_value = "38.90";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::state();
      expected.string_value = "0";
      expected.numeric_value = 1;
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(WebControlledSaunaBox2, SetOff) {
  //EXPECT_POST("/api/heat/set", with_state(0), saunaBox_state_after_set_off);
  EXPECT_CALL(_session, fetch_raw("/s/0"))
      .WillOnce(::testing::Return(saunaBox_state_after_set_off));

  ASSERT_TRUE(worker_helper::switch_off(idx("000000B1")));

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::state();
      expected.string_value = "0";
      expected.numeric_value = 0;
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::current_temperature();
      expected.string_value = "12.3";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::desired_temperature();
      expected.string_value = "37.70";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

} // namespace saunaBoxSTest
