#include <gmock/gmock.h>

#include <boost/assert.hpp>

#include "../../../../hardware/BleBox.h"
#include "../../../../hardware/blebox/box.h"
#include "../../../../hardware/blebox/errors.h"

#include "../../../memdb.h"
#include "../../../worker_helper.h"

#include "../mocks.h"

#include "fixtures/switchBoxD_fixtures.h"

namespace switchBoxDTest {

class IdentifiedSwitchBoxD : public IdentifiedBox {
  std::string default_status() const final { return switchBoxD_status_ok1; }
};

class WebControlledSwitchBoxD : public WorkerScenario {
  std::string default_status() const final { return switchBoxD_status_ok1; }

  void add_state_fetch_expectations(session_type &session) final {
    EXPECT_CALL(session, fetch_raw("/api/relay/state"))
        .WillOnce(::testing::Return(switchBoxD_state_ok1));
  }
};

namespace features {
static auto relay() {
  test_helper::device_status dev;
  dev.battery_level = 255;
  dev.signal_level = 12;

  dev.hw_id = 234;

  dev.type = 0xF4;     // pTypeGeneralSwitch
  dev.sub_type = 0x49; // sSwitchGeneralSwitch
  dev.switch_type = 0; // STYPE_OnOff

  dev.node_type_identifier = "000000B0"; // state is first method (0)

  dev.string_value = "0";

  dev.description = "";
  dev.color = "";
  dev.last_level = 0;
  return dev;
}

static auto relay1() {
  auto dev = relay();
  dev.name = "My switchBoxD 1.0.state";
  dev.unit = 0xA0 + 0;
  return dev;
}

static auto relay2() {
  auto dev = relay();
  dev.name = "My switchBoxD 1.1.state";
  dev.unit = 0xA0 + 1;
  return dev;
}

} // namespace features

TEST(SwitchBoxD, Identify) {
  MemDBSession mem_session;

  auto ip = blebox::make_address_v4("192.168.1.239");
  ::testing::StrictMock<GMockSession> session(ip, 1, 1);

  EXPECT_CALL(session, fetch_raw("/api/device/state"))
      .WillOnce(::testing::Return(switchBoxD_status_ok1));
  auto box = blebox::box::from_session(session);

  ASSERT_STREQ(box->type().c_str(), "switchBoxD");
  ASSERT_STREQ(box->hardware_version().c_str(), "0.7");
  ASSERT_STREQ(box->firmware_version().c_str(), "0.201");
  ASSERT_STREQ(box->unique_id().c_str(), "7eaf1234dc79");
  ASSERT_STREQ(box->name().c_str(), "My switchBoxD 1");

  // TODO: fix when available in API?
  ASSERT_STREQ(box->api_version().c_str(), "1.1.0");
}

TEST(SwitchBoxD, IdentifyWithOldApiLevel) {
  MemDBSession mem_session;

  auto ip = blebox::make_address_v4("192.168.1.239");
  ::testing::StrictMock<GMockSession> session(ip, 1, 1);

  EXPECT_CALL(session, fetch_raw("/api/device/state"))
      .WillOnce(::testing::Return(switchBoxD_status_ok1_with_old_apilevel));
  auto box = blebox::box::from_session(session);

  ASSERT_STREQ(box->type().c_str(), "switchBoxD");
  ASSERT_STREQ(box->hardware_version().c_str(), "9.1");
  ASSERT_STREQ(box->firmware_version().c_str(), "0.603");
  ASSERT_STREQ(box->unique_id().c_str(), "bbe62d8d49f4");
  ASSERT_STREQ(box->name().c_str(), "My lamp 1");

  // TODO: fix when available in API?
  ASSERT_STREQ(box->api_version().c_str(), "1.1.0");
}

TEST_F(IdentifiedSwitchBoxD, UpdateWithoutFeatures) {
  MemDBSession mem_session;

  BleBox bb(123, "192.168.1.17", 30);

  EXPECT_CALL(_session, fetch_raw("/api/relay/state"))
      .WillOnce(::testing::Return(switchBoxD_state_ok1));
  box->fetch(_session, bb);

  std::set<test_helper::device_status> all_expected;
  ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
            test_helper::device_status::json());
}

TEST_F(IdentifiedSwitchBoxD, AddSupportedFeatures) {
  MemDBSession mem_session;

  BleBox bb(234, "192.168.1.17", 30);
  box->add_missing_supported_features(bb.hw_id());

  EXPECT_CALL(_session, fetch_raw("/api/relay/state"))
      .WillOnce(::testing::Return(switchBoxD_state_ok1));
  box->fetch(_session, bb);

  {
    std::set<test_helper::device_status> all_expected;

    {
      auto expected = features::relay1();
      expected.numeric_value = 0; // cmd = off
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::relay2();
      expected.numeric_value = 1; // cmd = on
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(WebControlledSwitchBoxD, TurnOnFirstRelay) {
  // TODO: use POST version
  EXPECT_CALL(_session, fetch_raw("/s/0/1"))
      .WillOnce(::testing::Return(switchBoxD_state_after_first_relay_on));

  ASSERT_TRUE(worker_helper::switch_on(idx("000000B0", 0)));

  // NOTE: commands are stored in db, so we can at least test that
  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::relay1();
      expected.numeric_value = 1; // cmd = on
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::relay2();
      expected.numeric_value = 1; // cmd = on
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(WebControlledSwitchBoxD, TurnOffSecondRelay) {
  // TODO: use POST version
  EXPECT_CALL(_session, fetch_raw(std::string("/s/1/0")))
      .WillOnce(::testing::Return(switchBoxD_state_after_second_relay_off));
  ASSERT_TRUE(worker_helper::switch_off(idx("000000B0", 1)));

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::relay1();
      expected.numeric_value = 1; // cmd = on
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::relay2();
      expected.numeric_value = 0; // cmd = gswitch_sOff
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}
} // namespace switchBoxDTest
