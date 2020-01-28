#include "mocks.h"

IdentifiedBox::IdentifiedBox()
    : _session(blebox::make_address_v4("192.168.1.239"), 1, 1) {}

void IdentifiedBox::SetUp() {
  EXPECT_CALL(_session, fetch_raw("/api/device/state"))
      .WillOnce(::testing::Return(default_status()));
  box = blebox::box::from_session(_session);
}

WorkerScenario::WorkerScenario()
    : _session(blebox::make_address_v4("192.168.1.239"), 1, 1), mem_session() {}

void WorkerScenario::SetUp() {
  // so that same device_status records are used during prep & test
  const int common_hardware_id = 234;

  {
    //_identify
    session_type temp_session(blebox::make_address_v4("192.168.1.239"), 1, 1);
    add_identify_expectations(temp_session);
    auto &&box = blebox::box::from_session(temp_session);
    ASSERT_TRUE(testing::Mock::VerifyAndClear(&temp_session));

    // detect features
    BleBox temp_bb(common_hardware_id, "192.168.1.17", 30, &temp_session);
    box->add_missing_supported_features(temp_bb.hw_id());
    ASSERT_TRUE(testing::Mock::VerifyAndClear(&temp_session));

    // fetch
    add_state_fetch_expectations(temp_session);
    box->fetch(temp_session, temp_bb);
    ASSERT_TRUE(testing::Mock::VerifyAndClear(&temp_session));
    test_helper::device_status::read_from_db(_features);
  }

  add_identify_expectations(_session);
  bb = std::make_unique<BleBox>(234, "192.168.1.17", 30, &_session);
  worker.start_with_hardware(bb.get());
}

void WorkerScenario::TearDown() {
  worker.stop();
  worker_helper::remove_hardware(bb.release());
}

static uint8_t encode_db_unit(int unit) {
  if (unit == 0xFF)
    return 0xFF;

  // NOTE: this is just to detect bugs
  BOOST_ASSERT(unit >= 0 && unit < 0x10);
  return unit == 0xFF ? 0xFF : 0xA0 + uint8_t(unit);
}

int WorkerScenario::idx(std::string node_id, int decoded_unit) const {
  uint8_t unit = encode_db_unit(decoded_unit);

  auto &&feature = std::find_if(
      _features.begin(), _features.end(), [node_id, unit](auto &&arg) {
        return arg.node_type_identifier == node_id && arg.unit == unit;
      });

  if (feature == _features.end()) {
    using namespace std::literals;
    auto records = test_helper::device_status::json();
    throw std::runtime_error("TESTS: Expected feature "s + node_id + "["s +
                             std::to_string(int(unit)) +
                             "] not found among: " + records);
  }
  return feature->record_id;
}

void WorkerScenario::add_identify_expectations(session_type &session) {
  ON_CALL(session, fetch_raw("/api/device/state"))
      .WillByDefault(
          ::testing::Throw(std::logic_error("calling unmocked method")));
  EXPECT_CALL(session, fetch_raw("/api/device/state"))
      .WillOnce(::testing::Return(default_status()));
}
