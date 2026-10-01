"""
<plugin key="CalibrationTest" name="Calibration Test" author="domoticz" version="1.0.0">
    <params>
    </params>
</plugin>

EXAMPLE PLUGIN -- shipping location and run location
----------------------------------------------------
The tracked copy of this file lives under:
    plugins/examples/CalibrationTest/

Domoticz does NOT scan plugins/examples/; it only discovers plugins under
    <userdata>/plugins/<Name>/plugin.py
test/python/test_calibration.py copies this single file to
plugins/CalibrationTest/plugin.py inside its own throwaway userdata folder
for the duration of a test run, and removes it afterwards. It never touches
a real installation.

Purpose
-------
Exercises the plugin device-update ingest path (Domoticz.Device.Update(),
which calls straight into CSQLHelper::UpdateValue) with a deterministic
trigger: flipping the "Push Raw Values" switch (Unit 1) via the JSON API
makes onCommand() push a fixed, hardcoded set of raw sensor readings into
the other devices. The test then reads the stored values back and compares
them against what calibration (AddjValue/AddjMulti on those devices) should
have produced.

The raw values pushed here are a fixed contract with test_calibration.py;
if they change here, they must change there too.
"""
import Domoticz

UNIT_TRIGGER = 1
UNIT_TEMP = 2
UNIT_TEMP_HUM = 3
UNIT_TEMP_HUM_BARO_INT = 4
UNIT_TEMP_HUM_BARO_FLOAT = 5
UNIT_BARO = 6
UNIT_UV = 7

# Raw (uncalibrated) values pushed into the sensor devices on every trigger.
# Fixed contract with test_calibration.py -- keep both sides in sync.
RAW_VALUES = {
    UNIT_TEMP: "20.0",
    UNIT_TEMP_HUM: "20.0;50;1",
    UNIT_TEMP_HUM_BARO_INT: "20.0;50;1;1010;0",
    UNIT_TEMP_HUM_BARO_FLOAT: "20.0;50;1;1010.3;0",
    UNIT_BARO: "1010.0;0",
    UNIT_UV: "5.0;0.0",
}


class BasePlugin:
    def onStart(self):
        Domoticz.Log("CalibrationTest v%s started" % PLUGIN_VERSION)

        if UNIT_TRIGGER not in Devices:
            Domoticz.Device(Name="Push Raw Values", Unit=UNIT_TRIGGER, TypeName="Switch").Create()
        if UNIT_TEMP not in Devices:
            # pTypeTEMP / sTypeTEMP5
            Domoticz.Device(Name="Temp", Unit=UNIT_TEMP, Type=80, Subtype=5).Create()
        if UNIT_TEMP_HUM not in Devices:
            # pTypeTEMP_HUM / sTypeTH1
            Domoticz.Device(Name="TempHum", Unit=UNIT_TEMP_HUM, Type=82, Subtype=1).Create()
        if UNIT_TEMP_HUM_BARO_INT not in Devices:
            # pTypeTEMP_HUM_BARO / sTypeTHB1 (integer barometer)
            Domoticz.Device(Name="TempHumBaroInt", Unit=UNIT_TEMP_HUM_BARO_INT, Type=84, Subtype=1).Create()
        if UNIT_TEMP_HUM_BARO_FLOAT not in Devices:
            # pTypeTEMP_HUM_BARO / sTypeTHBFloat (float barometer)
            Domoticz.Device(Name="TempHumBaroFloat", Unit=UNIT_TEMP_HUM_BARO_FLOAT, Type=84, Subtype=16).Create()
        if UNIT_BARO not in Devices:
            # pTypeGeneral / sTypeBaro (bare barometer)
            Domoticz.Device(Name="Baro", Unit=UNIT_BARO, Type=243, Subtype=26).Create()
        if UNIT_UV not in Devices:
            # pTypeUV / sTypeUV1
            Domoticz.Device(Name="UV", Unit=UNIT_UV, Type=87, Subtype=1).Create()

    def onStop(self):
        Domoticz.Log("CalibrationTest stopping")

    def onCommand(self, Unit, Command, Level, Hue):
        Domoticz.Log("CalibrationTest onCommand: Unit %d, Command '%s', Level %d" % (Unit, Command, Level))
        if Unit != UNIT_TRIGGER:
            return

        nvalue = 1 if Command == "On" else 0
        Devices[UNIT_TRIGGER].Update(nValue=nvalue, sValue=Command)
        if Command != "On":
            return

        for unit, raw in RAW_VALUES.items():
            Devices[unit].Update(nValue=0, sValue=raw)
        Domoticz.Log("CalibrationTest: pushed raw values to %d sensor device(s)" % len(RAW_VALUES))


PLUGIN_VERSION = "1.0.0"
_plugin = BasePlugin()


def onStart():
    _plugin.onStart()


def onStop():
    _plugin.onStop()


def onCommand(Unit, Command, Level, Hue):
    _plugin.onCommand(Unit, Command, Level, Hue)
