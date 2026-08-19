"""
<plugin key="ColorRangeTest" name="Color Range Test" author="domoticz" version="1.0.0">
    <params>
    </params>
</plugin>

EXAMPLE PLUGIN -- shipping location and run location
----------------------------------------------------
The tracked copy of this file lives under:
    plugins/examples/ColorRangeTest/

Domoticz does NOT scan plugins/examples/; it only discovers plugins under
    <userdata>/plugins/<Name>/plugin.py
test/python/test_plugin_color_ranges.py copies this single file to
plugins/ColorRangeTest/plugin.py inside its own throwaway userdata folder
for the duration of a test run, and removes it afterwards. It never touches
a real installation.

Purpose
-------
Exercises the plugin Color write path (Domoticz.Device.Update(Color=...),
which calls into CSQLHelper::UpdateDeviceValue) with a fixed set of payloads.

DeviceStatus.Color carries two unrelated things: RGB/WW color state for
color switches, and the bar range definitions the utility, temperature and
weather cards render (www/app/widgets/dzBar.js). A plugin has to be able to
write the second kind without it being mistaken for the first.

Flipping the "Apply Color Payloads" switch (Unit 1) via the JSON API makes
onCommand() assign one fixed payload to each of the other devices. The test
then reads the stored Color back and compares it to what should have been
kept, normalized, or rejected.

The payloads here are a fixed contract with test_plugin_color_ranges.py;
if they change here, they must change there too.
"""
import Domoticz

UNIT_TRIGGER = 1
UNIT_UTILITY = 2
UNIT_TEMPERATURE = 3
UNIT_QUOTED = 4
UNIT_COLOUR_STATE = 5
UNIT_GARBAGE = 6

# Payloads assigned on every trigger.
# Fixed contract with test_plugin_color_ranges.py -- keep both sides in sync.
#
#   UNIT_UTILITY       bare array, the shape the utility card expects
#                      (dzUtilityWidget.js getBarRanges requires a leading '[')
#   UNIT_TEMPERATURE   keyed object, the shape the temperature and weather cards
#                      expect (dzBar.js parseRangesForKey requires a leading '{')
#   UNIT_QUOTED        a range payload whose color string contains a single
#                      quote and SQL fragments. Legal JSON, so it must reach the
#                      database verbatim, and must not alter any other row on the
#                      way in. This is the guard for the UPDATE statement built in
#                      CSQLHelper::UpdateDeviceValue.
#   UNIT_COLOUR_STATE  genuine RGB color state, which must still be normalized
#                      through _tColor rather than passed through.
#   UNIT_GARBAGE       not JSON at all, which must still be rejected.
PAYLOADS = {
    UNIT_UTILITY: '[{"from":0,"to":80,"color":"#66bb6a"},{"from":80,"to":100,"color":"#ef5350"}]',
    UNIT_TEMPERATURE: '{"temp":[{"from":-10,"to":18,"color":"#42a5f5"},{"from":18,"to":40,"color":"#ef5350"}]}',
    UNIT_QUOTED: '[{"from":0,"to":10,"color":"#fff\' , Name=\'INJECTED\' WHERE 1=1 -- "}]',
    UNIT_COLOUR_STATE: '{"b":255,"cw":0,"g":128,"m":3,"r":10,"t":0,"ww":0}',
    UNIT_GARBAGE: 'not json at all',
}


class BasePlugin:
    def onStart(self):
        Domoticz.Log("ColorRangeTest v%s started" % PLUGIN_VERSION)

        if UNIT_TRIGGER not in Devices:
            Domoticz.Device(Name="Apply Color Payloads", Unit=UNIT_TRIGGER, TypeName="Switch").Create()
        # pTypeGeneral / sTypeCustom -- a Custom Sensor is a utility-card device,
        # which is one of the card families that renders bar ranges.
        for unit, name in ((UNIT_UTILITY, "Utility Ranges"),
                           (UNIT_QUOTED, "Quoted Ranges"),
                           (UNIT_COLOUR_STATE, "Color State"),
                           (UNIT_GARBAGE, "Garbage Payload")):
            if unit not in Devices:
                Domoticz.Device(Name=name, Unit=unit, Type=243, Subtype=31,
                                Options={"Custom": "1;units"}).Create()
        if UNIT_TEMPERATURE not in Devices:
            # pTypeTEMP / sTypeTEMP5
            Domoticz.Device(Name="Temperature Ranges", Unit=UNIT_TEMPERATURE, Type=80, Subtype=5).Create()

    def onStop(self):
        Domoticz.Log("ColorRangeTest stopping")

    def onCommand(self, Unit, Command, Level, Hue):
        Domoticz.Log("ColorRangeTest onCommand: Unit %d, Command '%s', Level %d" % (Unit, Command, Level))
        if Unit != UNIT_TRIGGER:
            return

        nvalue = 1 if Command == "On" else 0
        Devices[UNIT_TRIGGER].Update(nValue=nvalue, sValue=Command)
        if Command != "On":
            return

        # Give every device a value first: a device with an empty sValue is not
        # serialized by getdevices, so the test could not read its Color back.
        Devices[UNIT_TEMPERATURE].Update(nValue=0, sValue="21.5")
        for unit in (UNIT_UTILITY, UNIT_QUOTED, UNIT_COLOUR_STATE, UNIT_GARBAGE):
            Devices[unit].Update(nValue=0, sValue="42")

        for unit, payload in PAYLOADS.items():
            Devices[unit].Update(nValue=0, sValue=Devices[unit].sValue, Color=payload)
        Domoticz.Log("ColorRangeTest: applied %d color payload(s)" % len(PAYLOADS))


PLUGIN_VERSION = "1.0.0"
_plugin = BasePlugin()


def onStart():
    _plugin.onStart()


def onStop():
    _plugin.onStop()


def onCommand(Unit, Command, Level, Hue):
    _plugin.onCommand(Unit, Command, Level, Hue)
