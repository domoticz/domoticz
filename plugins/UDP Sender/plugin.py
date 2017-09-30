#           UDP Sender Plugin
#
#           Author:     Dnpwwo, 2016 - 2017
#
"""
<plugin key="UDP_Sender" name="UDP Sender" author="dnpwwo" version="1.8.0">
    <params>
        <param field="Address" label="IP Address" width="200px" required="true" default="127.0.0.1"/>
        <param field="Port" label="Port" width="30px" required="true" default="9090"/>
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
import sys
import json

class BasePlugin:

    def onStart(self):
        if Parameters["Mode6"] == "Debug":
            Domoticz.Debugging(1)
            DumpConfigToLog()

        udpBcastConn = Domoticz.Connection(Name="UDP Broadcast Connection", Transport="UDP/IP", Protocol="None", Address=Parameters["Address"], Port=Parameters["Port"])
        try:
            udpBcastConn.Send("Hello World!")
        except:
            Domoticz.Error("Failed to send message")

    def onHeartbeat(self):
        return True
        
global _plugin
_plugin = BasePlugin()

def onStart():
    global _plugin
    _plugin.onStart()

def onHeartbeat():
    global _plugin
    _plugin.onHeartbeat()

def DumpConfigToLog():
    for x in Parameters:
        if Parameters[x] != "":
            Domoticz.Debug( "'" + x + "':'" + str(Parameters[x]) + "'")
    Domoticz.Debug("Device count: " + str(len(Devices)))
    for x in Devices:
        Domoticz.Debug("Device:           " + str(x) + " - " + str(Devices[x]))
        Domoticz.Debug("Device ID:       '" + str(Devices[x].ID) + "'")
        Domoticz.Debug("Device Name:     '" + Devices[x].Name + "'")
        Domoticz.Debug("Device nValue:    " + str(Devices[x].nValue))
        Domoticz.Debug("Device sValue:   '" + Devices[x].sValue + "'")
        Domoticz.Debug("Device LastLevel: " + str(Devices[x].LastLevel))
    return
