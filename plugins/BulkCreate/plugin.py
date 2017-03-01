#           BulkCreate Plugin
#
#           Author:     Dnpwwo, 2016
#
#
#   Plugin parameter definition below will be parsed during startup and copied into Manifest.xml, this will then drive the user interface in the Hardware web page
#
"""
<plugin key="BulkCreate" name="BulkCreate Tester" author="dnpwwo" version="1.0.0" wikilink="http://www.domoticz.com/wiki/plugins/RAVEn.html" externallink="https://rainforestautomation.com/rfa-z106-raven/">
    <params>
        <param field="Mode6" label="Debug" width="75px">
            <options>
                <option label="True" value="Debug"/>
                <option label="False" value="Normal"  default="true" />
            </options>
        </param>
    </params>
</plugin>
"""
import Domoticz

def stringToBase64(s):
    return base64.b64encode(s.encode('utf-8')).decode("utf-8")
  
def onStart():
    if Parameters["Mode6"] == "Debug":
        Domoticz.Debugging(1)
    if (len(Devices) == 0):
        Domoticz.Device(Name="Pressure", Unit=1, TypeName="Pressure", Used=0).Create()
        Domoticz.Device(Name="Percentage", Unit=2, TypeName="Percentage", Used=1).Create()
        Domoticz.Device(Name="Gas", Unit=3, TypeName="Gas", Used=0).Create()
        Domoticz.Device(Name="Voltage", Unit=4, TypeName="Voltage", Used=1).Create()
        Domoticz.Device(Name="Text", Unit=5, TypeName="Text", Used=0).Create()
        Domoticz.Device(Name="Switch", Unit=6, TypeName="Switch", Used=1).Create()
        Domoticz.Device(Name="Alert", Unit=7, TypeName="Alert", Used=0).Create()
        Domoticz.Device(Name="Current/Ampere", Unit=8, TypeName="Current/Ampere", Used=1).Create()
        Domoticz.Device(Name="Sound Level", Unit=9, TypeName="Sound Level", Used=0).Create()
        Domoticz.Device(Name="Barometer", Unit=10, TypeName="Barometer", Used=1).Create()
        Domoticz.Device(Name="Visibility", Unit=11, TypeName="Visibility", Used=0).Create()
        Domoticz.Device(Name="Distance", Unit=12, TypeName="Distance", Used=1).Create()
        Domoticz.Device(Name="Counter Incremental", Unit=13, TypeName="Counter Incremental", Used=0).Create()
        Domoticz.Device(Name="Soil Moisture", Unit=14, TypeName="Soil Moisture", Used=1).Create()
        Domoticz.Device(Name="Leaf Wetness", Unit=15, TypeName="Leaf Wetness", Used=0).Create()
        Domoticz.Device(Name="kWh", Unit=16, TypeName="kWh", Used=1).Create()
        Domoticz.Device(Name="Current (Single)", Unit=17, TypeName="Current (Single)", Used=0).Create()
        Domoticz.Device(Name="Solar Radiation", Unit=18, TypeName="Solar Radiation", Used=1).Create()
        Domoticz.Device(Name="Temperature", Unit=19, TypeName="Temperature", Used=0).Create()
        Domoticz.Device(Name="Humidity", Unit=20, TypeName="Humidity", Used=0).Create()
        Domoticz.Device(Name="Temp+Hum", Unit=21, TypeName="Temp+Hum", Used=0).Create()
        Domoticz.Device(Name="Temp+Hum+Baro", Unit=22, TypeName="Temp+Hum+Baro", Used=0).Create()
        Domoticz.Device(Name="Wind", Unit=23, TypeName="Wind", Used=0).Create()
        Domoticz.Device(Name="Rain", Unit=24, TypeName="Rain", Used=0).Create()
        Domoticz.Device(Name="UV", Unit=25, TypeName="UV", Used=0).Create()
        Domoticz.Device(Name="Air Quality", Unit=26, TypeName="Air Quality", Used=0).Create()
        Domoticz.Device(Name="Usage", Unit=27, TypeName="Usage", Used=0).Create()
        Domoticz.Device(Name="Illumination", Unit=28, TypeName="Illumination", Used=0).Create()
        Domoticz.Device(Name="Waterflow", Unit=29, TypeName="Waterflow", Used=0).Create()
        Domoticz.Device(Name="Wind+Temp+Chill", Unit=30, TypeName="Wind+Temp+Chill", Used=0).Create()
        Domoticz.Device(Name="Selector Switch", Unit=31, TypeName="Selector Switch", Used=0).Create()
        Domoticz.Device(Name="Custom", Unit=32, TypeName="Custom", Used=0).Create()
        Domoticz.Log("Devices created.")
    Domoticz.Log("Plugin has " + str(len(Devices)) + " devices associated with it.")
    return

def onHeartbeat():
    Domoticz.Log("onHeartbeat called")

