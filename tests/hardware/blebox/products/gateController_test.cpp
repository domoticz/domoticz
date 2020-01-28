#include <gmock/gmock.h>

#include <boost/assert.hpp>
#include <stdexcept>

#include "../../../../hardware/BleBox.h"
#include "../../../../hardware/blebox/box.h"
#include "../../../../hardware/blebox/errors.h"

#include "../../../memdb.h"
#include "../../../worker_helper.h"

#include "../mocks.h"

#include "fixtures/gateController_fixtures.h"

namespace gateControllerTest {

class IdentifiedGateController : public IdentifiedBox {
  std::string default_status() const final { return gateController_status_ok1; }
};

class WebControlledGateController : public WorkerScenario {
  std::string default_status() const final { return gateController_status_ok1; }

  void add_state_fetch_expectations(session_type &session) final {
    // Read device state
    EXPECT_CALL(session, fetch_raw("/api/gatecontroller/state"))
        .WillOnce(::testing::Return(gateController_state_ok1));
  }
};

namespace features {
static auto desired_position() {
  test_helper::device_status dev;
  dev.battery_level = 255;
  dev.signal_level = 12;

  dev.hw_id = 234;

  dev.type = 0xF4;      // pTypeGeneralSwitch
  dev.sub_type = 0x49;  // sSwitchGeneralSwitch
  dev.switch_type = 13; // STYPE_BlindsPercentage;

  dev.node_type_identifier = "000000B0"; // state is first method (0)
  dev.unit = 0xFF;                       // self method

  dev.name = "My gate controller 1.desiredPos";

  dev.description = "";
  dev.numeric_value = 2; // gSwitch_sSetLevel

  dev.color = "";
  dev.last_level = 0;
  return dev;
}

static auto current_position() {
  test_helper::device_status dev;
  dev.battery_level = 255;
  dev.signal_level = 12;

  dev.hw_id = 234;

  dev.type = 0xF3;     // pTypeGeneral
  dev.sub_type = 0x6;  // sTypePercentage
  dev.switch_type = 0; // STYPE_OnOff

  dev.node_type_identifier = "0000B1FF"; // state is first method (0)
  dev.unit = 0xFF;                       // self method

  dev.name = "My gate controller 1.currentPos";

  dev.description = "";
  dev.numeric_value = 0;

  dev.color = "";
  dev.last_level = 0;
  dev.unit = 1;
  return dev;
}

} // namespace features

static std::string with_desired_position(int value) {
  Json::Value form(Json::objectValue);
  form["gateController"]["desiredPos"][0] = value;
  return test_helper::compact_json(form);
}

TEST(gateController, Identify) {
  MemDBSession mem_session;

  auto ip = blebox::make_address_v4("192.168.1.17");
  ::testing::StrictMock<GMockSession> session(ip, 1, 1);

  EXPECT_CALL(session, fetch_raw("/api/device/state"))
      .WillOnce(::testing::Return(gateController_status_ok1));
  auto box = blebox::box::from_session(session);

  ASSERT_STREQ(box->type().c_str(), "gateController");
  ASSERT_STREQ(box->hardware_version().c_str(), "custom.2.6");
  ASSERT_STREQ(box->firmware_version().c_str(), "1.390");
  ASSERT_STREQ(box->unique_id().c_str(), "aaa2ffaafe30db9437");
  ASSERT_STREQ(box->name().c_str(), "My gate controller 1");
  ASSERT_STREQ(box->api_version().c_str(), "1.1.0");
}

TEST_F(IdentifiedGateController, UpdateWithoutFeatures) {
  MemDBSession mem_session;

  BleBox bb(123, "192.168.1.17", 30);

  EXPECT_CALL(_session, fetch_raw("/api/gatecontroller/state"))
      .WillOnce(::testing::Return(gateController_state_ok1));
  box->fetch(_session, bb);

  std::set<test_helper::device_status> all_expected;
  ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
            test_helper::device_status::json());
}

TEST_F(IdentifiedGateController, AddSupportedFeatures) {
  MemDBSession mem_session;

  BleBox bb(234, "192.168.1.17", 30);
  box->add_missing_supported_features(bb.hw_id());

  EXPECT_CALL(_session, fetch_raw("/api/gatecontroller/state"))
      .WillOnce(::testing::Return(gateController_state_ok1));
  box->fetch(_session, bb);

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::current_position();
      expected.string_value = "87.00";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::desired_position();
      expected.string_value = "23";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(WebControlledGateController, SetPosition) {
  EXPECT_POST("/api/gatecontroller/set", with_desired_position(29),
              gateController_state_after_set_29);
  ASSERT_TRUE(worker_helper::set_level(idx("000000B0"), 29));

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::current_position();
      expected.string_value = "71.00";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::desired_position();
      expected.string_value = "29";
      expected.switch_type = 13; // STYPE_BlindsPercentage
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(WebControlledGateController, SetPositionInverted) {
  // overkill to load all but we're expecting only one anyway
  std::set<test_helper::device_status> devices;
  test_helper::device_status::read_from_db(devices);

  // simulate the user changing the device subtype
  test_helper::device_status::set_switch_type(idx("000000B0"),
                                              STYPE_BlindsPercentageInverted);

  // Usual detect features + update
  {
    auto ip = blebox::make_address_v4("192.168.1.17");
    ::testing::StrictMock<GMockSession> session(ip, 1, 1);
    EXPECT_CALL(session, fetch_raw("/api/device/state"))
        .WillOnce(::testing::Return(gateController_status_ok1));

    BleBox bbox(234, "192.168.1.17", 30, &session);

    auto &&box = blebox::box::from_session(session);

    box->add_missing_supported_features(bbox.hw_id());

    EXPECT_CALL(session, fetch_raw("/api/gatecontroller/state"))
        .WillOnce(::testing::Return(gateController_state_ok1));
    box->fetch(session, bbox);
  }

  // Check the features were correctly updated with new subtype
  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::current_position();
      expected.string_value = "87.00";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }
    {
      auto expected = features::desired_position();
      expected.string_value = "23";
      expected.switch_type = 16; // STYPE_BlindsPercentageInverted
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }

  EXPECT_POST("/api/gatecontroller/set", with_desired_position(29),
              gateController_state_after_set_29);

  // TODO: rename
  ASSERT_TRUE(worker_helper::set_level(idx("000000B0"), 29));

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::current_position();
      expected.string_value = "71.00";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::desired_position();
      expected.string_value = "29";
      expected.switch_type = 16; // STYPE_BlindsPercentageInverted
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}
} // namespace gateControllerTest
