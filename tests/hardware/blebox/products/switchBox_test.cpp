#include <gmock/gmock.h>

#include <boost/assert.hpp>

#include "../../../../hardware/BleBox.h"
#include "../../../../hardware/blebox/box.h"
#include "../../../../hardware/blebox/errors.h"

#include "../../../memdb.h"
#include "../../../worker_helper.h"

#include "../mocks.h"

#include "fixtures/switchBox_fixtures.h"

namespace SwitchBoxTest {

class IdentifiedSwitchBox : public IdentifiedBox {
  std::string default_status() const final { return switchBox_status_ok1; }
};

class IdentifiedSwitchBox2 : public IdentifiedBox {
  std::string default_status() const final { return switchBox_status_ok2; }
};

class WebControlledSwitchBox : public WorkerScenario {
  std::string default_status() const final { return switchBox_status_ok1; }

  void add_state_fetch_expectations(session_type &) final {
    // state is already passed in device info
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
  dev.switch_type = 0;

  dev.node_type_identifier = "000000B0"; // state is first method (0)
  dev.unit = 0xA0 + 0;                   // first relay

  dev.name = "My switchBox 1.0.state";
  dev.string_value = "0";

  dev.description = "";
  dev.color = "";
  dev.last_level = 0;
  return dev;
}
} // namespace features

TEST(SwitchBox, Identify) {
  MemDBSession mem_session;

  auto ip = blebox::make_address_v4("192.168.1.239");
  ::testing::StrictMock<GMockSession> session(ip, 1, 1);

  EXPECT_CALL(session, fetch_raw("/api/device/state"))
      .WillOnce(::testing::Return(switchBox_status_ok1));
  auto box = blebox::box::from_session(session);

  ASSERT_STREQ(box->type().c_str(), "switchBox");
  ASSERT_STREQ(box->hardware_version().c_str(), "0.2");
  ASSERT_STREQ(box->firmware_version().c_str(), "0.247");
  ASSERT_STREQ(box->unique_id().c_str(), "1afe34e750b8");
  ASSERT_STREQ(box->name().c_str(), "My switchBox 1");

  ASSERT_STREQ(box->api_version().c_str(), "1.1.0");
}

TEST_F(IdentifiedSwitchBox, UpdateWithoutFeatures) {
  MemDBSession mem_session;

  BleBox bb(234, "192.168.1.17", 30);

  // No HTTP GET expected here - state obtained during identify()
  box->fetch(_session, bb);

  std::set<test_helper::device_status> all_expected;
  ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
            test_helper::device_status::json());
}

TEST_F(IdentifiedSwitchBox, AddSupportedFeatures) {
  MemDBSession mem_session;

  BleBox bb(234, "192.168.1.17", 30);
  box->add_missing_supported_features(bb.hw_id());

  // No HTTP GET expected here - state obtained during identify()
  box->fetch(_session, bb);

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::relay();
      expected.numeric_value = 0; // command = off
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(IdentifiedSwitchBox2, UpdateWhenOn) {
  MemDBSession mem_session;

  BleBox bb(234, "192.168.1.17", 30);
  box->add_missing_supported_features(bb.hw_id());
  box->fetch(_session, bb);

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::relay();
      expected.name = "My switchBox 2.0.state";
      expected.numeric_value = 1;
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(WebControlledSwitchBox, TurnOnRelay) {
  // TODO: use POST version
  //EXPECT_POST("/api/relay/set", with_relay_state(0, "on"), switchBox_state_after_relay_on);
  EXPECT_CALL(_session, fetch_raw("/s/1"))
    .WillOnce(::testing::Return(switchBox_state_after_relay_on));

  ASSERT_TRUE(worker_helper::switch_on(idx("000000B0", 0)));

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::relay();
      expected.numeric_value = 1; // cmd = on
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}
} // namespace SwitchBoxTest
