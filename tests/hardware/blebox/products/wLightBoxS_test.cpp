#include <gmock/gmock.h>

#include <boost/assert.hpp>
#include <boost/format.hpp>

#include "../../../../hardware/BleBox.h"
#include "../../../../hardware/blebox/box.h"
#include "../../../../hardware/blebox/errors.h"

#include "../../../memdb.h"
#include "../../../worker_helper.h"

#include "../mocks.h"

#include "fixtures/wLightBoxS_fixtures.h"

namespace wLightBoxSTest {

class IdentifiedWLightBoxS : public IdentifiedBox {
  std::string default_status() const final { return wLightBoxS_status_ok1; }
};

class WebControlledWLightBoxS : public WorkerScenario {
  std::string default_status() const final { return wLightBoxS_status_ok1; }

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

  dev.type = 0xF4;     // pTypeGeneralSwitch
  dev.sub_type = 0x49; // sSwitchGeneralSwitch
  dev.switch_type = 7; // STYPE_Dimmer

  dev.node_type_identifier = "000000B0"; // state is first method (0)
  dev.unit = 0xFF;                       // self method

  dev.name = "My light 1.desired_color";

  dev.description = "";
  dev.color = "";
  dev.last_level = 0;
  return dev;
}
} // namespace features

static auto with_desired_color(int color) {
  Json::Value form;
  form["light"]["desiredColor"] = (boost::format("%X") % color).str();
  return test_helper::compact_json(form);
}

TEST(wLightBoxS, Identify) {
  MemDBSession mem_session;

  auto ip = blebox::make_address_v4("192.168.1.237");
  ::testing::StrictMock<GMockSession> session(ip, 1, 1);

  EXPECT_CALL(session, fetch_raw("/api/device/state"))
      .WillOnce(::testing::Return(wLightBoxS_status_ok1));
  auto box = blebox::box::from_session(session);

  ASSERT_STREQ(box->type().c_str(), "wLightBoxS");
  ASSERT_STREQ(box->hardware_version().c_str(), "0.2");
  ASSERT_STREQ(box->firmware_version().c_str(), "0.247");
  ASSERT_STREQ(box->unique_id().c_str(), "eafe34e750b9");
  ASSERT_STREQ(box->name().c_str(), "My light 1");

  ASSERT_STREQ(box->api_version().c_str(), "1.1.0");
}

TEST_F(IdentifiedWLightBoxS, UpdateWithoutFeatures) {
  MemDBSession mem_session;

  BleBox bb(234, "192.168.1.17", 30);

  // No HTTP GET expected here - state obtained during identify()
  box->fetch(_session, bb);

  std::set<test_helper::device_status> all_expected;
  ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
            test_helper::device_status::json());
}

TEST_F(IdentifiedWLightBoxS, AddSupportedFeatures) {
  MemDBSession mem_session;

  BleBox bb(234, "192.168.1.17", 30);
  box->add_missing_supported_features(bb.hw_id());

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::desired_color();
      expected.string_value = "(added)"; // 0xCE/255 = 206/255 = 80.07
      expected.numeric_value = 0x0;      // command = gSwitch_sOff
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }

  // No HTTP GET expected here - state obtained during identify()
  box->fetch(_session, bb);

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::desired_color();
      expected.string_value = "81";  // 0xCE/255 = 206/255 = 80.07
      expected.numeric_value = 0x02; // command = gSwitch_sSetLevel
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(WebControlledWLightBoxS, SetHighColor) {
  // TODO: custom widget for wlightboxs
  EXPECT_POST("/api/rgbw/set", with_desired_color(0xAD),
              wLightBoxS_state_after_set_AD);
  ASSERT_TRUE(worker_helper::set_level(idx("000000B0"),
                                       68)); // 0xAB/255 =~ 0.24

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::desired_color();
      expected.string_value = "24";
      expected.switch_type = 7;      // STYPE_Dimmer
      expected.string_value = "68";  // 0xAB/255 = 117/255 = 67.05
      expected.numeric_value = 0x02; // command = gSwitch_sSetLevel
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}
} // namespace wLightBoxSTest
