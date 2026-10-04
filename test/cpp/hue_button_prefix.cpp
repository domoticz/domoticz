#include <json/json.h>
#include <climits>
#include <cassert>
#include <map>
#include <string>
#include <vector>
constexpr int LOG_ERROR = 1;
struct Button { std::string owner_rid; bool supports_long_press; int control_id; std::string id_v1; };
struct Sensors {
    std::vector<Button> buttons;
    const std::vector<Button>& GetButtons() { return buttons; }
};
struct CPhilipsHue {
    Sensors* m_v2sensors;
    int updates = 0, level = -1;
    void HandleSSEButton(const Json::Value&);
    int ParseV1NumericId(const std::string&) { return 1; }
    int NodeIDFromRid(const std::string&) { return 1; }
    std::string GetV2DeviceName(const std::string&) { return "fixture"; }
    uint8_t GetBatteryForOwner(const std::string&) { return 100; }
    void Log(int, const char*, ...) {}
    bool InsertUpdateSelectorSwitch(int, int, uint8_t value, const std::string&, uint8_t) {
        ++updates; level = value; return false;
    }
    void SetSwitchOptions(int, int, const std::map<std::string, std::string>&) {}
};
