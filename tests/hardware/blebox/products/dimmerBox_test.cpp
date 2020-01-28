#include <gmock/gmock.h>

#include <boost/assert.hpp>
#include <boost/format.hpp>

#include "../../../../hardware/BleBox.h"
#include "../../../../hardware/blebox/box.h"
#include "../../../../hardware/blebox/errors.h"

#include "../../../memdb.h"
#include "../../../worker_helper.h"

#include "../mocks.h"

#include "fixtures/dimmerBox_fixtures.h"

namespace dimmerBoxTest {

class IdentifiedDimmerBox : public IdentifiedBox {
  std::string default_status() const final { return dimmerBox_status_ok1; }
};

class WebControlledDimmerBox : public WorkerScenario {
  std::string default_status() const final { return dimmerBox_status_ok1; }

  void add_state_fetch_expectations(session_type &) final {}
};

namespace features {
static auto brightness() {
  test_helper::device_status dev;
  dev.battery_level = 255;
  dev.signal_level = 12;

  dev.hw_id = 234;

  dev.node_type_identifier = "000000B0"; // first method
  dev.unit = 0xFF;                       // self method

  dev.name = "My dimmer 1.brightness";

  dev.type = 244;      // pTypeGeneralSwitch
  dev.sub_type = 73;   //  sSwitchGeneralSwitch
  dev.switch_type = 7; // STYPE_Dimmer

  dev.description = "";
  dev.color = "";
  dev.last_level = 0;
  return dev;
}
} // namespace features

TEST(dimmerBox, Identify) {
  MemDBSession mem_session;

  auto ip = blebox::make_address_v4("192.168.1.239");
  ::testing::StrictMock<GMockSession> session(ip, 1, 1);

  EXPECT_CALL(session, fetch_raw("/api/device/state"))
      .WillOnce(::testing::Return(dimmerBox_status_ok1));
  auto box = blebox::box::from_session(session);

  ASSERT_STREQ(box->type().c_str(), "dimmerBox");
  ASSERT_STREQ(box->hardware_version().c_str(), "0.2");
  ASSERT_STREQ(box->firmware_version().c_str(), "0.247");
  ASSERT_STREQ(box->unique_id().c_str(), "1afe34e750b8");
  ASSERT_STREQ(box->name().c_str(), "My dimmer 1");
  ASSERT_STREQ(box->api_version().c_str(), "1.1.0");
}

TEST_F(IdentifiedDimmerBox, UpdateWithoutFeatures) {
  MemDBSession mem_session;

  BleBox bb(234, "192.168.1.17", 30);

  // No HTTP GET expected here - state obtained during identify()
  box->fetch(_session, bb);

  std::set<test_helper::device_status> all_expected;
  ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
            test_helper::device_status::json());
}

TEST_F(IdentifiedDimmerBox, AddSupportedFeatures) {
  MemDBSession mem_session;

  BleBox bb(234, "192.168.1.17", 30);
  box->add_missing_supported_features(bb.hw_id());

  // No HTTP GET expected here - state obtained during identify()
  box->fetch(_session, bb);

  std::set<test_helper::device_status> all_expected;

  {
    auto expected = features::brightness();
    expected.string_value = "93"; // 237/255 (blebox) -> 92/100 + adjust -> 93
    expected.numeric_value = 2;   // cmd = gswitch_sSetLevel
    BOOST_ASSERT(all_expected.insert(expected).second);
  }

  ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
            test_helper::device_status::json());
}

static std::string with_desired_brightness(int brightness) {
  Json::Value form;
  form["dimmer"]["desiredBrightness"] = brightness;
  return test_helper::compact_json(form);
}

TEST_F(WebControlledDimmerBox, SetDimmer) {
  EXPECT_POST("/api/dimmer/set", with_desired_brightness(53),
              dimmerBox_state_after_set_53);
  // 35/255 -> 21/100
  ASSERT_TRUE(worker_helper::set_level(idx("000000B0"), 21));

  {
    std::set<test_helper::device_status> all_expected;

    {
      auto expected = features::brightness();
      expected.string_value = "21";
      expected.numeric_value = 2; // cmd = gswitch_sSetLevel
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(WebControlledDimmerBox, SetDimmerWithHighValue) {
  EXPECT_POST("/api/dimmer/set", with_desired_brightness(239),
              dimmerBox_state_after_set_239);
  ASSERT_TRUE(worker_helper::set_level(idx("000000B0"), 94));

  {
    std::set<test_helper::device_status> all_expected;

    {
      auto expected = features::brightness();
      expected.string_value = "94";
      expected.numeric_value = 2; // cmd = gswitch_sSetLevel
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(WebControlledDimmerBox, SetDimmerWithZero) {
  EXPECT_POST("/api/dimmer/set", with_desired_brightness(0),
              dimmerBox_state_after_set_0);
  ASSERT_TRUE(worker_helper::set_level(idx("000000B0"), 0));
  {
    std::set<test_helper::device_status> all_expected;

    {
      auto expected = features::brightness();
      expected.string_value = "0";
      expected.numeric_value = 0; // gswitch_sOff
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(WebControlledDimmerBox, SetDimmerWithMax) {
  EXPECT_POST("/api/dimmer/set", with_desired_brightness(255),
              dimmerBox_state_after_set_255);
  ASSERT_TRUE(worker_helper::set_level(idx("000000B0"), 100));

  {
    std::set<test_helper::device_status> all_expected;

    {
      auto expected = features::brightness();
      expected.string_value = "100";
      expected.numeric_value = 2; // cmd = gswitch_sSetLevel
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}
} // namespace dimmerBoxTest
