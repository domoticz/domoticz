#include <gmock/gmock.h>

#include <boost/assert.hpp>

#include "../../../../hardware/BleBox.h"
#include "../../../../hardware/blebox/box.h"
#include "../../../../hardware/blebox/errors.h"

#include "../../../memdb.h"
#include "../../../worker_helper.h"

#include "../mocks.h"

#include "fixtures/shutterBox_fixtures.h"

namespace shutterBoxTest {

class IdentifiedShutterBox : public IdentifiedBox {
  std::string default_status() const final { return shutterBox_status_ok1; }
};

class WebControlledShutterBox : public WorkerScenario {
  std::string default_status() const final { return shutterBox_status_ok1; }

  void add_state_fetch_expectations(session_type &session) final {
    EXPECT_CALL(session, fetch_raw("/api/shutter/state"))
        .WillOnce(::testing::Return(shutterBox_state_ok1));
  }
};

namespace features {
static auto common() {
  test_helper::device_status dev;
  dev.battery_level = 255;
  dev.signal_level = 12;

  dev.hw_id = 234;

  dev.unit = 0xFF; // self method

  dev.description = "";
  dev.color = "";
  dev.last_level = 0;
  return dev;
}

static auto desired_position() {
  auto dev = common();
  dev.type = 0xF4;             // pTypeGeneralSwitch
  dev.sub_type = 0x49;         // sSwitchGeneralSwitch
  dev.switch_type = 0xBADBEEF; // must initialize in given test case

  dev.node_type_identifier = "000000B0"; // state is first method (0)

  dev.numeric_value = 2; // gSwitch_sSetLevel

  dev.name = "My shutter 1.desired_position";
  return dev;
}

static auto current_position() {
  auto dev = common();
  dev.type = 0xF3;     // pTypeGeneral
  dev.sub_type = 0x06; // sTypePercentage
  dev.switch_type = 0; // default

  dev.node_type_identifier = "0000B1FF"; // B1 = method, FF = unit (self)

  dev.numeric_value = 0; // no commands for percentage

  dev.name = "My shutter 1.actual_position";
  dev.unit = 1; // gDevice has not unit field (assumed 1 by mainworker)
  return dev;
}

} // namespace features

TEST(shutterBox, Identify) {
  MemDBSession mem_session;

  auto ip = blebox::make_address_v4("192.168.1.11");
  ::testing::StrictMock<GMockSession> session(ip, 1, 1);

  EXPECT_CALL(session, fetch_raw("/api/device/state"))
      .WillOnce(::testing::Return(shutterBox_status_ok1));
  auto box = blebox::box::from_session(session);

  ASSERT_STREQ(box->type().c_str(), "shutterBox");
  ASSERT_STREQ(box->hardware_version().c_str(), "0.7");
  ASSERT_STREQ(box->firmware_version().c_str(), "0.147");
  ASSERT_STREQ(box->unique_id().c_str(), "2bee34e750b8");
  ASSERT_STREQ(box->name().c_str(), "My shutter 1");

  // TODO: fix when available in API?
  ASSERT_STREQ(box->api_version().c_str(), "1.1.0");
}

TEST_F(IdentifiedShutterBox, UpdateWithoutFeatures) {
  MemDBSession mem_session;

  BleBox bb(123, "192.168.1.17", 30);

  EXPECT_CALL(_session, fetch_raw("/api/shutter/state"))
      .WillOnce(::testing::Return(shutterBox_state_ok1));
  box->fetch(_session, bb);

  std::set<test_helper::device_status> all_expected;
  ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
            test_helper::device_status::json());
}

TEST_F(IdentifiedShutterBox, AddSupportedFeatures) {
  MemDBSession mem_session;

  BleBox bb(234, "192.168.1.17", 30);
  box->add_missing_supported_features(bb.hw_id());

  EXPECT_CALL(_session, fetch_raw("/api/shutter/state"))
      .WillOnce(::testing::Return(shutterBox_state_ok1));
  box->fetch(_session, bb);

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::desired_position();
      expected.string_value = "78";
      // expected.switch_type = 16; // STYPE_BlindsPercentageInverted
      expected.switch_type = 13; // STYPE_BlindsPercentage
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::current_position();
      expected.string_value = "34.00";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(WebControlledShutterBox, SetPosition) {
  // WriteToHarware identifies the device from scratch
  EXPECT_CALL(_session, fetch_raw("/s/p/21"))
      .WillOnce(::testing::Return(shutterBox_state_after_set_21));

  ASSERT_TRUE(worker_helper::set_level(idx("000000B0"), 21));

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::desired_position();
      expected.string_value = "21";
      expected.switch_type = 13; // STYPE_BlindsPercentage
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::current_position();
      expected.string_value = "34.00";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(WebControlledShutterBox, SetPositionInverted) {
  // overkill to load all but we're expecting only one anyway
  std::set<test_helper::device_status> devices;
  test_helper::device_status::read_from_db(devices);

  ASSERT_EQ(2, devices.size());

  // simulate the user changing the device subtype
  test_helper::device_status::set_switch_type(idx("000000B0"),
                                              STYPE_BlindsPercentageInverted);

  {
    auto ip = blebox::make_address_v4("192.168.1.17");
    ::testing::StrictMock<GMockSession> session(ip, 1, 1);
    BleBox bbox(234, "192.168.1.17", 30, &session);

    EXPECT_CALL(session, fetch_raw("/api/device/state"))
        .WillOnce(::testing::Return(shutterBox_status_ok1));
    auto &&box = blebox::box::from_session(session);
    ASSERT_TRUE(testing::Mock::VerifyAndClear(&session));

    box->add_missing_supported_features(bbox.hw_id());

    EXPECT_CALL(session, fetch_raw("/api/shutter/state"))
        .WillOnce(::testing::Return(shutterBox_state_ok1));
    box->fetch(session, bbox);
    ASSERT_TRUE(testing::Mock::VerifyAndClear(&session));
  }

  // Check the features were correctly updated with new subtype
  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::desired_position();
      expected.string_value = "78";
      expected.switch_type = 16; // STYPE_BlindsPercentageInverted
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::current_position();
      expected.string_value = "34.00";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }

  EXPECT_CALL(_session, fetch_raw("/s/p/13"))
      .WillOnce(::testing::Return(shutterBox_state_after_set_13));

  // EXPECT_CALL(_session, fetch_raw("/api/device/state"))
  //    .WillOnce(::testing::Return(shutterBox_status_ok1));
  ASSERT_TRUE(worker_helper::set_level(idx("000000B0"), 13));

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::desired_position();
      expected.string_value = "13";
      expected.switch_type = 16; // STYPE_BlindsPercentageInverted
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::current_position();
      expected.string_value = "72.00";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}
} // namespace shutterBoxTest
