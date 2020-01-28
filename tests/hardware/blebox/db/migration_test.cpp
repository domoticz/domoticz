#include <gmock/gmock.h>

// TODO: move this test to db dir
// TODO: remove old hardware via domoticz sql api

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/format.hpp>

#include <stdexcept>
#include <map>

#include "../../../../hardware/ColorSwitch.h"

typedef unsigned char BYTE; // for RFXtrx.h
#include "../../../../main/RFXtrx.h"

#include "../../../../main/SQLHelper.h"
#include "../../../../main/RFXNames.h"
#include "../../../../main/Logger.h"

#include "../../../../hardware/hardwaretypes.h"

#include "../../../memdb.h"
#include "../../../test_helper.h"
#include "../../../testdb.h"

#include "../mocks.h"

#include "../products/fixtures/airSensor_fixtures.h"
#include "../products/fixtures/dimmerBox_fixtures.h"
#include "../products/fixtures/gateBox_fixtures.h"
#include "../products/fixtures/shutterBox_fixtures.h"
#include "../products/fixtures/switchBoxD_fixtures.h"
#include "../products/fixtures/switchBox_fixtures.h"
#include "../products/fixtures/tempSensor_fixtures.h"
#include "../products/fixtures/wLightBoxS_fixtures.h"
#include "../products/fixtures/wLightBox_fixtures.h"

struct old_device {
  int deviceID;
  uint8_t subType;
  int switchType;
};

const std::map<std::string, old_device> OldDevInfo = {
    {"switchBox", {pTypeLighting2, sTypeAC, STYPE_OnOff}},
    {"shutterBox", {pTypeLighting2, sTypeAC, STYPE_BlindsPercentageInverted}},
    {"wLightBoxS", {pTypeLighting2, sTypeAC, STYPE_Dimmer}},
    {"wLightBox", {pTypeColorSwitch, sTypeColor_RGB_W, STYPE_Dimmer}},
    {"gateBox", {pTypeGeneral, sTypePercentage, 0}},
    {"dimmerBox", {pTypeLighting2, sTypeAC, STYPE_Dimmer}},
    {"switchBoxD", {pTypeLighting2, sTypeAC, STYPE_OnOff}},
    {"airSensor", {pTypeAirQuality, sTypeVoltcraft, 0}},
    {"tempSensor", {pTypeGeneral, sTypeTemperature, 0}}};

static std::string IPToHex(const std::string &IPAddress, const int type) {
  using namespace std::literals;
  using namespace boost;

  std::vector<std::string> strarray;
  boost::split(strarray, IPAddress, boost::is_any_of("."));
  if (strarray.size() != 4)
    throw std::invalid_argument("bad ip address :"s + IPAddress);

  auto i0 = uint32_t(std::stoi(strarray[0]));
  auto i1 = uint32_t(std::stoi(strarray[1]));
  auto i2 = uint32_t(std::stoi(strarray[2]));
  auto i3 = uint32_t(std::stoi(strarray[3]));

  // because exists inconsistency when comparing deviceID in method decode_xxx
  // in mainworker(Limitless uses small letter, lighting2 etc uses capital
  // letter)
  if (type == pTypeColorSwitch)
    return (format("%X%02X%02X%02X") % i0 % i1 % i2 % i3).str();

  return (format("%08X") % ((i0 << 24) | (i1 << 16) | (i2 << 8) | (i3))).str();
}

static void insert(int hw_id, const char *did, unsigned char unit,
                   uint8_t devid, uint8_t stype, int swtype,
                   const std::string &name) {
  m_sql.InsertDevice(hw_id, did, unit, devid, stype, swtype, 0, "Unavailable",
                     name);
}

static void AddNode(int hw_id, const std::string &name, const std::string &type,
                    const std::string &IPAddress) {
  using namespace std::literals;

  try {
    auto &&dev = OldDevInfo.at(type);
    auto devid = uint8_t(dev.deviceID);

    std::string szIdx = IPToHex(IPAddress, devid);
    auto did = szIdx.c_str();
    auto stype = dev.subType;
    auto swtype = dev.switchType;

    if ("gateBox"s == type) {
      insert(hw_id, did, 1, devid, stype, dev.switchType, name);
      insert(hw_id, did, 2, pTypeGeneralSwitch, sTypeAC, STYPE_PushOn, name);
      insert(hw_id, did, 3, pTypeGeneralSwitch, sTypeAC, STYPE_PushOn, name);
    } else if ("switchBox"s == type) {
      insert(hw_id, did, 0, devid, stype, swtype, name);
      insert(hw_id, did, 1, devid, stype, swtype, name);
    } else {
      insert(hw_id, did, 0, devid, stype, swtype, name);
    }
  } catch (std::out_of_range &) {
    throw std::runtime_error("unknown blebox device type: "s + type.c_str());
  }
}

static int last_hw_id() {
  const char *query = "Select MAX(ID) from Hardware where type=%d";
  return std::stoi(m_sql.safe_query(query, HTYPE_BleBox).at(0).at(0));
}

static int create_old_hardware() {
  m_sql.safe_query("INSERT INTO Hardware (Name, Enabled, Type, Address, Port, "
                   "SerialPort, Username, Password, Extra, Mode1, Mode2, "
                   "Mode3, Mode4, Mode5, Mode6, DataTimeout) VALUES (%Q,%d, "
                   "%d,%Q,%d,%Q,%Q,%Q,%Q,%d,%d,%d,%d,%d,%d,%d)",
                   "foobar", 1, HTYPE_BleBox, "", 80, "", "", "", "", 0, 0, 0,
                   0, 0, 0, 0);
  return last_hw_id();
}

static void verify_hardware(int new_hw_id, const char *name, const char *hwmac,
                            const char *ip) {
  using namespace std::literals;

  std::vector<testdb::hardware> results;
  testdb::hardware::all(results);
  auto blebox_count = std::count_if(results.begin(), results.end(),
                                    [](auto &item) { return item.type == 82; });
  ASSERT_EQ(1, blebox_count);
  auto hw = testdb::hardware::find(new_hw_id);
  ASSERT_EQ(HTYPE_BleBox, hw.type);
  ASSERT_EQ(std::string(hwmac) + ":"s + ip, hw.address);
  ASSERT_EQ(name, hw.name);
}

namespace MigrationTests {
using namespace test_helper;
using namespace ::testing;

class Migration : public Test {
protected:
  MemDBSession mem_session;
};

static void migration(const char *name, const char *type, const char *ipv4,
                      const char *fixture) {
  StrictMock<GMockSession> session(blebox::make_address_v4(ipv4), 1, 1);
  int _old_hw_id = create_old_hardware();
  AddNode(_old_hw_id, name, type, ipv4);
  EXPECT_CALL(session, fetch_raw("/api/device/state"))
      .WillOnce(Return(fixture));
  ASSERT_TRUE(blebox::db::migrate(&session));
}

static auto common_feature(int hw_id) {
  test_helper::device_status dev;
  dev.battery_level = 255;
  dev.signal_level = 12;

  dev.hw_id = hw_id;

  dev.unit = 0xFF; // self method

  dev.string_value = "(added)";
  dev.numeric_value = 0;

  dev.description = "";
  dev.color = "";
  dev.last_level = 0;
  return dev;
}

namespace SwitchBoxMigrationTest {

namespace features {
static auto relay(int hw_id) {
  auto dev = common_feature(hw_id);
  dev.type = 0xF4;     // pTypeGeneralSwitch
  dev.sub_type = 0x49; // sSwitchGeneralSwitch
  dev.switch_type = 0;

  dev.node_type_identifier = "000000B0"; // state is first method (0)
  dev.unit = 0xA0 + 0;                   // first relay

  dev.name = "My switchBox 1.0.state";
  return dev;
}
} // namespace features

TEST_F(Migration, switchBox) {
  migration("switch 1", "switchBox", "192.168.3.4", switchBox_status_ok1);

  int new_hw_id = last_hw_id();
  ASSERT_EQ(device_status::dump_json({features::relay(new_hw_id)}),
            device_status::json());
  verify_hardware(new_hw_id, "switch 1", "1afe34e750b8", "192.168.3.4");
}
} // namespace SwitchBoxMigrationTest

namespace ShutterBoxMigrationTest {

namespace features {
static auto desired_position(int hw_id) {
  auto dev = common_feature(hw_id);
  dev.type = 0xF4;      // pTypeGeneralSwitch
  dev.sub_type = 0x49;  // sSwitchGeneralSwitch
  dev.switch_type = 13; // STYPE_BlindsPercentage

  dev.node_type_identifier = "000000B0"; // state is first method (0)

  dev.name = "My shutter 1.desired_position";
  return dev;
}

static auto current_position(int hw_id) {
  auto dev = common_feature(hw_id);
  dev.type = 0xF3;     // pTypeGeneral
  dev.sub_type = 0x06; // sTypePercentage
  dev.switch_type = 0; // default

  dev.node_type_identifier = "0000B1FF"; // B1 = method, FF = unit (self)

  dev.name = "My shutter 1.actual_position";
  dev.unit = 1; // gDevice has not unit field (assumed 1 by mainworker)
  return dev;
}
} // namespace features

TEST_F(Migration, shutterBox) {
  migration("shutter 1", "shutterBox", "192.168.3.4", shutterBox_status_ok1);

  int new_hw_id = last_hw_id();
  ASSERT_EQ(device_status::dump_json({features::desired_position(new_hw_id),
                                      features::current_position(new_hw_id)}),
            device_status::json());
  verify_hardware(new_hw_id, "shutter 1", "2bee34e750b8", "192.168.3.4");
}
} // namespace ShutterBoxMigrationTest

namespace WLightBoxSMigrationTest {

namespace features {
static auto desired_color(int hw_id) {
  auto dev = common_feature(hw_id);

  dev.type = 0xF4;     // pTypeGeneralSwitch
  dev.sub_type = 0x49; // sSwitchGeneralSwitch
  dev.switch_type = 7; // STYPE_Dimmer

  dev.node_type_identifier = "000000B0"; // state is first method (0)

  dev.name = "My light 1.desired_color";
  return dev;
}
} // namespace features

TEST_F(Migration, wLightBoxS) {
  migration("light 1", "wLightBoxS", "192.168.3.4", wLightBoxS_status_ok1);

  int new_hw_id = last_hw_id();
  ASSERT_EQ(device_status::dump_json({features::desired_color(new_hw_id)}),
            device_status::json());
  verify_hardware(new_hw_id, "light 1", "eafe34e750b9", "192.168.3.4");
}

} // namespace WLightBoxSMigrationTest

namespace WLightBoxMigrationTest {
namespace features {
static auto desired_color(int hw_id) {
  auto dev = common_feature(hw_id);

  dev.type = 0xF1;     // pTypeColorSwitch
  dev.sub_type = 0x1;  // sTypeColor_RGB_W
  dev.switch_type = 7; // STYPE_Dimmer

  dev.node_type_identifier = "000000B0"; // state is first method (0)
  dev.name = "My light 1.desired_color";
  return dev;
}
} // namespace features

TEST_F(Migration, wLightBox) {
  migration("light 1", "wLightBox", "192.168.3.4", wLightBox_status_ok1);

  int new_hw_id = last_hw_id();
  ASSERT_EQ(device_status::dump_json({features::desired_color(new_hw_id)}),
            device_status::json());
  verify_hardware(new_hw_id, "light 1", "1afe34e750b8", "192.168.3.4");
}
} // namespace WLightBoxMigrationTest

namespace gateBoxMigrationTest {

namespace features {
static auto current_position(int hw_id) {
  auto dev = common_feature(hw_id);

  dev.node_type_identifier = "0000B2FF";
  dev.name = "My gate 1.currentPos";

  dev.type = 0xF3;    // pTypeGeneral
  dev.sub_type = 0x6; // sTypePercentage
  dev.switch_type = 0;

  dev.unit = 1; // gDevice has not unit field (assumed 1 by mainworker)
  return dev;
}

static auto button(int hw_id) {
  auto dev = common_feature(hw_id);
  dev.type = 0xF4;     // pTypeGeneralSwitch
  dev.sub_type = 0x49; // sSwitchGeneralSwitch
  dev.switch_type = 9; // STYPE_PushOn
  return dev;
}

static auto button1(int hw_id) {
  auto dev = button(hw_id);
  dev.node_type_identifier = "000000B0";
  dev.name = "My gate 1.primary";
  return dev;
}

static auto button2(int hw_id) {
  auto dev = button(hw_id);
  dev.node_type_identifier = "000000B1";
  dev.name = "My gate 1.secondary";
  return dev;
}

} // namespace features

TEST_F(Migration, gateBox) {
  migration("gate 1", "gateBox", "192.168.3.4", gateBox_status_ok1);

  int new_hw_id = last_hw_id();
  ASSERT_EQ(device_status::dump_json({features::current_position(new_hw_id),
                                      features::button1(new_hw_id),
                                      features::button2(new_hw_id)}),

            device_status::json());
  verify_hardware(new_hw_id, "gate 1", "1afe34db9437", "192.168.3.4");
}
} // namespace gateBoxMigrationTest

namespace dimmerBoxMigrationTest {

namespace features {
static auto brightness(int hw_id) {
  auto dev = common_feature(hw_id);
  dev.node_type_identifier = "000000B0"; // first method

  dev.name = "My dimmer 1.brightness";

  dev.type = 244;      // pTypeGeneralSwitch
  dev.sub_type = 73;   //  sSwitchGeneralSwitch
  dev.switch_type = 7; // STYPE_Dimmer
  return dev;
}
} // namespace features
TEST_F(Migration, dimmerBox) {
  migration("dimmer 1", "dimmerBox", "192.168.3.4", dimmerBox_status_ok1);

  int new_hw_id = last_hw_id();
  ASSERT_EQ(device_status::dump_json({features::brightness(new_hw_id)}),

            device_status::json());
  verify_hardware(new_hw_id, "dimmer 1", "1afe34e750b8", "192.168.3.4");
}
} // namespace dimmerBoxMigrationTest

namespace switchBoxDMigrationTest {
namespace features {
static auto relay(int hw_id) {
  auto dev = common_feature(hw_id);
  dev.type = 0xF4;     // pTypeGeneralSwitch
  dev.sub_type = 0x49; // sSwitchGeneralSwitch
  dev.switch_type = 0; // STYPE_OnOff

  dev.node_type_identifier = "000000B0"; // state is first method (0)
  return dev;
}

static auto relay1(int hw_id) {
  auto dev = relay(hw_id);
  dev.name = "My switchBoxD 1.0.state";
  dev.unit = 0xA0 + 0;
  return dev;
}

static auto relay2(int hw_id) {
  auto dev = relay(hw_id);
  dev.name = "My switchBoxD 1.1.state";
  dev.unit = 0xA0 + 1;
  return dev;
}

} // namespace features

TEST_F(Migration, switchBoxD) {
  migration("switch 1", "switchBoxD", "192.168.3.4", switchBoxD_status_ok1);

  int new_hw_id = last_hw_id();
  ASSERT_EQ(device_status::dump_json(
                {features::relay1(new_hw_id), features::relay2(new_hw_id)}),
            device_status::json());
  verify_hardware(new_hw_id, "switch 1", "7eaf1234dc79", "192.168.3.4");
}
} // namespace switchBoxDMigrationTest

namespace airSensorMigrationTest {

namespace features {
static auto value(int hw_id) {
  auto dev = common_feature(hw_id);
  dev.type = 0xF9;  // pTypeAirQuality
  dev.sub_type = 1; // sTypeBleboxAirSensor = sTypeVoltcraft = 0x01
  dev.switch_type = 0;

  dev.node_type_identifier = "176"; // 0xB0
  return dev;
}

static auto pm1(int hw_id) {
  auto dev = value(hw_id);
  dev.name = "My backyard air sensor.pm1.value";
  dev.unit = 0xA0 + 0;
  return dev;
}

static auto pm2(int hw_id) {
  auto dev = value(hw_id);
  dev.name = "My backyard air sensor.pm2.5.value";
  dev.unit = 0xA0 + 1;
  return dev;
}

static auto pm10(int hw_id) {
  auto dev = value(hw_id);
  dev.name = "My backyard air sensor.pm10.value";
  dev.unit = 0xA0 + 2;
  return dev;
}

static auto kick(int hw_id) {
  auto dev = common_feature(hw_id);
  dev.type = 0xF4;     // pTypeGeneralSwitch
  dev.sub_type = 0x49; // sSwitchGeneralSwitch
  dev.switch_type = 9; // STYPE_PushOn

  dev.name = "My backyard air sensor.kick";

  dev.node_type_identifier = "000000B0";
  return dev;
}
} // namespace features

TEST_F(Migration, airSensor) {
  migration("air 1", "airSensor", "192.168.3.4", airSensor_status_ok1);

  int new_hw_id = last_hw_id();
  ASSERT_EQ(device_status::dump_json(
                {features::pm1(new_hw_id), features::pm2(new_hw_id),
                 features::pm10(new_hw_id), features::kick(new_hw_id)}),

            device_status::json());
  verify_hardware(new_hw_id, "air 1", "1afe34db9437", "192.168.3.4");
}
} // namespace airSensorMigrationTest

namespace tempSensorMigrationTest {

namespace features {
static auto temperature(int hw_id) {
  auto dev = common_feature(hw_id);
  dev.type = 0x50;   // pTypeTemp
  dev.sub_type = 0x05; // sTypeTEMPBleBox = sTypeTEMP5
  dev.switch_type = 0;

  dev.node_type_identifier = "45216"; // 0xB0A0,  0xE0 - method, 0xA0 - unit(0)
  dev.unit = 0xA0 + 0;

  dev.name = "My tempSensor.0.temp";
  return dev;
}
} // namespace features

TEST_F(Migration, tempSensor) {
  migration("temp 1", "tempSensor", "192.168.3.4", tempSensor_status_ok1);

  int new_hw_id = last_hw_id();
  ASSERT_EQ(device_status::dump_json({features::temperature(new_hw_id)}),
            device_status::json());
  verify_hardware(new_hw_id, "temp 1", "1afe34db9437", "192.168.3.4");
}

} // namespace tempSensorMigrationTest

} // namespace MigrationTests
