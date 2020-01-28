#include <gmock/gmock.h>

#include <boost/assert.hpp>

#include "../../../../hardware/BleBox.h"
#include "../../../../hardware/blebox/box.h"
#include "../../../../hardware/blebox/errors.h"

#include "../../../memdb.h"
#include "../../../worker_helper.h"

#include "../mocks.h"

#include "fixtures/gateBox_fixtures.h"

namespace gateBoxTest {

class IdentifiedGateBox : public IdentifiedBox {
  std::string default_status() const final { return gateBox_status_ok1; }
};

class WebControlledGateBox : public WorkerScenario {
  std::string default_status() const final { return gateBox_status_ok1; }

  void add_state_fetch_expectations(session_type &session) final {
    EXPECT_CALL(session, fetch_raw("/api/gate/state"))
        .WillOnce(::testing::Return(gateBox_state_ok1));
  }
};

namespace features {
static auto current_position() {
  test_helper::device_status dev;
  dev.battery_level = 255;
  dev.signal_level = 12;

  dev.hw_id = 234;

  dev.node_type_identifier = "0000B2FF";
  dev.unit = 0xFF; // self method

  dev.name = "My gate 1.currentPos";

  dev.type = 0xF3;    // pTypeGeneral
  dev.sub_type = 0x6; // sTypePercentage
  dev.switch_type = 0;

  dev.description = "";
  dev.numeric_value = 0;

  dev.color = "";
  dev.last_level = 0;
  dev.unit = 1; // gDevice has not unit field (assumed 1 by mainworker)
  return dev;
}

static auto button() {
  test_helper::device_status dev;
  dev.battery_level = 255;
  dev.signal_level = 12;

  dev.hw_id = 234;

  dev.type = 0xF4;     // pTypeGeneralSwitch
  dev.sub_type = 0x49; // sSwitchGeneralSwitch
  dev.switch_type = 9; // STYPE_PushOn

  dev.unit = 0xFF; // self method

  dev.color = "";
  dev.last_level = 0;

  dev.description = "";
  dev.numeric_value = 0;
  dev.string_value = "(added)";
  return dev;
}
} // namespace features

TEST(gateBox, Identify) {
  MemDBSession mem_session;

  auto ip = blebox::make_address_v4("192.168.1.11");
  ::testing::StrictMock<GMockSession> session(ip, 1, 1);

  EXPECT_CALL(session, fetch_raw("/api/device/state"))
      .WillOnce(::testing::Return(gateBox_status_ok1));
  auto box = blebox::box::from_session(session);

  ASSERT_STREQ(box->type().c_str(), "gateBox");
  ASSERT_STREQ(box->hardware_version().c_str(), "0.6");
  ASSERT_STREQ(box->firmware_version().c_str(), "0.176");
  ASSERT_STREQ(box->unique_id().c_str(), "1afe34db9437");
  ASSERT_STREQ(box->name().c_str(), "My gate 1");
  ASSERT_STREQ(box->api_version().c_str(), "1.1.0");
}

TEST_F(IdentifiedGateBox, UpdateWithoutFeatures) {
  MemDBSession mem_session;

  BleBox bb(123, "192.168.1.17", 30);

  EXPECT_CALL(_session, fetch_raw("/api/gate/state"))
      .WillOnce(::testing::Return(gateBox_state_ok1));
  box->fetch(_session, bb);

  std::set<test_helper::device_status> all_expected;
  ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
            test_helper::device_status::json());
}

TEST_F(IdentifiedGateBox, AddSupportedFeatures) {
  MemDBSession mem_session;

  BleBox bb(234, "192.168.1.17", 30);
  box->add_missing_supported_features(bb.hw_id());

  EXPECT_CALL(_session, fetch_raw("/api/gate/state"))
      .WillOnce(::testing::Return(gateBox_state_ok1));
  box->fetch(_session, bb);

  {
    std::set<test_helper::device_status> all_expected;
    {
      auto expected = features::current_position();
      expected.string_value = "87.00";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::button();
      expected.name = "My gate 1.primary";
      expected.node_type_identifier = "000000B0";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::button();
      expected.name = "My gate 1.secondary";
      expected.node_type_identifier = "000000B1";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(WebControlledGateBox, Primary) {
  EXPECT_CALL(_session, fetch_raw(std::string("/s/p")))
      .WillOnce(::testing::Return(gateBox_state_after_primary));

  ASSERT_TRUE(worker_helper::push(idx("000000B0")));

  {
    std::set<test_helper::device_status> all_expected;

    {
      auto expected = features::current_position();
      expected.string_value = "91.00";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::button();
      expected.name = "My gate 1.primary";
      expected.node_type_identifier = "000000B0";
      expected.numeric_value = 1;  // gswitch_sOn (value never read anyway)
      expected.string_value = "0"; // ??
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::button();
      expected.name = "My gate 1.secondary";
      expected.node_type_identifier = "000000B1";
      expected.numeric_value = 0; // gswitch_sOff (value never read anyway)
      // TODO: translate
      expected.string_value = "(added)"; // never changed
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}

TEST_F(WebControlledGateBox, Secondary) {
  EXPECT_CALL(_session, fetch_raw(std::string("/s/s")))
      .WillOnce(::testing::Return(gateBox_state_ok1)); // no change

  ASSERT_TRUE(worker_helper::push(idx("000000B1")));

  {
    std::set<test_helper::device_status> all_expected;

    {
      auto expected = features::current_position();
      expected.string_value = "87.00";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::button();
      expected.name = "My gate 1.primary";
      expected.node_type_identifier = "000000B0";
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    {
      auto expected = features::button();
      expected.name = "My gate 1.secondary";
      expected.node_type_identifier = "000000B1";
      expected.numeric_value = 1;  // gswitch_sOn (value never read anyway)
      expected.string_value = "0"; // ?
      BOOST_ASSERT(all_expected.insert(expected).second);
    }

    ASSERT_EQ(test_helper::device_status::dump_json(all_expected),
              test_helper::device_status::json());
  }
}
} // namespace gateBoxTest
