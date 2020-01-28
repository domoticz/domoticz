#include <gmock/gmock.h>

#include <boost/assert.hpp>

#include <algorithm>

#include "../../../../hardware/ColorSwitch.h"

#include "../../../../hardware/BleBox.h"
#include "../../../../hardware/blebox/box.h"
#include "../../../../hardware/blebox/errors.h"

#include "../../../memdb.h"
#include "../../../worker_helper.h"

#include "../mocks.h"

#include "fixtures/wLightBox_fixtures.h"

namespace wLightBoxTest {

class IdentifiedWLightBox : public IdentifiedBox {
  std::string default_status() const final { return wLightBox_status_ok1; }
};

class WebControlledWLightBox : public WorkerScenario {
  std::string default_status() const final { return wLightBox_status_ok1; }

  void add_state_fetch_expectations(session_type &) final {
    // state is already passed in device info
  }
};

namespace features {
static auto desired_color() {
  test_helper::device_status dev;
  dev.battery_level = 255;
  dev.signal_level = 12;

  dev.hw_id = 234;

  dev.type = 0xF1;     // pTypeColorSwitch
  dev.sub_type = 0x1;  // sTypeColor_RGB_W
  dev.switch_type = 7; // STYPE_Dimmer

  dev.node_type_identifier = "000000B0"; // state is first method (0)
  dev.unit = 0xFF;                       // self method

  dev.name = "My light 1.desired_color";

  dev.description = "";
  return dev;
}
} // namespace features

TEST(wLightBox, Identify) {
  MemDBSession mem_session;

  auto ip = blebox::make_address_v4("192.168.1.237");
  ::testing::StrictMock<GMockSession> session(ip, 1, 1);

  EXPECT_CALL(session, fetch_raw("/api/device/state"))
      .WillOnce(::testing::Return(wLightBox_status_ok1));
  auto box = blebox::box::from_session(session);

  ASSERT_STREQ(box->type().c_str(), "wLightBox");
  ASSERT_STREQ(box->hardware_version().c_str(), "0.2");
  ASSERT_STREQ(box->firmware_version().c_str(), "0.247");
  ASSERT_STREQ(box->unique_id().c_str(), "1afe34e750b8");
  ASSERT_STREQ(box->name().c_str(), "My light 1");

  ASSERT_STREQ(box->api_version().c_str(), "1.1.0");
}

TEST_F(IdentifiedWLightBox, UpdateWhenStateOk) {
  MemDBSession mem_session;

  BleBox bb(123, "192.168.1.17", 30);

  // No HTTP GET expected here - state obtained during identify()
  box->fetch(_session, bb);

  std::set<test_helper::device_status> all_expected;
  ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
            test_helper::device_status::json());
}

TEST_F(IdentifiedWLightBox, AddSupportedFeatures) {
  MemDBSession mem_session;

  BleBox bb(234, "192.168.1.17", 30);
  box->add_missing_supported_features(bb.hw_id());

  // No HTTP GET expected here - state obtained during identify()
  box->fetch(_session, bb);

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::desired_color();
      expected.numeric_value = 10; // command = set color
      expected.string_value = "0"; // level
      expected.last_level = 0;
      expected.color = R"({"b":0,"cw":0,"g":0,"m":4,"r":255,"t":0,"ww":0})";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(WebControlledWLightBox, SetBrightness) {
  // TODO: post?
  EXPECT_CALL(_session, fetch_raw("/s/ff000035")) // 53 = 0x35
      .WillOnce(::testing::Return(wLightBox_state_ok2));

  ASSERT_TRUE(worker.set_level(idx("000000B0"), 21));

  {
    std::set<test_helper::device_status> all_expected;

    {
      auto expected = features::desired_color();
      expected.numeric_value = 15;  // command = Color_SetBrightnessLevel
      expected.string_value = "21"; // level
      expected.last_level = 21;
      // 97 = 0x61
      // 53 = 0x35
      expected.color = R"({"b":0,"cw":0,"g":0,"m":4,"r":255,"t":0,"ww":53})";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(WebControlledWLightBox, SetColor) {
  EXPECT_CALL(_session, fetch_raw("/s/fea16f70")) // 44/100 -> 0x70/255
      .WillOnce(::testing::Return(wLightBox_state_after_set));

  uint8_t level_100 = 44; // -> 0x70/255
  ASSERT_TRUE(
      worker.rgbw_color_set(idx("000000B0"), 0xfe, 0xa1, 0x6f, level_100));

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::desired_color();
      expected.numeric_value = 10;  // command = Color_SetColor
      expected.string_value = "44"; // level
      expected.last_level = 44;
      expected.color = R"({"b":111,"cw":0,"g":161,"m":3,"r":254,"t":0,"ww":0})";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(WebControlledWLightBox, SetWhiteModeBrightness) {
  EXPECT_CALL(_session, fetch_raw("/s/000000b5")) // 71/100 -> 0xb5/255
      .WillOnce(::testing::Return(wLightBox_state_after_set_wm));

  uint8_t level_100 = 71;

  ASSERT_TRUE(worker.rgbw_whitemode_set(idx("000000B0"), level_100));

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::desired_color();
      expected.numeric_value = 10;  // command = Color_SetColor
      expected.string_value = "71"; // level
      expected.last_level = 71;
      expected.color = R"({"b":0,"cw":255,"g":0,"m":1,"r":0,"t":0,"ww":255})";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(WebControlledWLightBox, SetOff) {
  EXPECT_CALL(_session, fetch_raw("/s/onoff/last"))
      .WillOnce(::testing::Return(wLightBox_state_after_off));

  ASSERT_TRUE(worker.rgbw_off(idx("000000B0")));

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::desired_color();
      expected.numeric_value = 0;  // command = Color_LedOff
      expected.string_value = "0"; // level
      expected.last_level = 0;
      expected.color = R"({"b":0,"cw":0,"g":0,"m":4,"r":255,"t":0,"ww":0})";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(WebControlledWLightBox, SetOffOnLastColor) {
  EXPECT_CALL(_session, fetch_raw("/s/onoff/last"))
      .WillOnce(::testing::Return(wLightBox_state_after_off));
  ASSERT_TRUE(worker.rgbw_off(idx("000000B0")));

  EXPECT_CALL(_session, fetch_raw("/s/onoff/last"))
      .WillOnce(::testing::Return(wLightBox_state_after_offon));
  ASSERT_TRUE(worker.rgbw_on(idx("000000B0")));

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::desired_color();
      expected.numeric_value = 1; // command = Color_LedOn
      expected.string_value = "0";
      expected.last_level = 0;
      expected.color = R"({"b":0,"cw":0,"g":0,"m":4,"r":255,"t":0,"ww":0})";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

} // namespace wLightBoxTest
