# UDP Discovery Example
#
# Logs traffic on the Dynamic Device Discovery port 
#
# Works on Linux, Windows appears to filter out UDP from the network even when the firewall is set to allow it
#
# Author: Dnpwwo, 2017
#
"""
<plugin key="UdpDiscover" name="UDP Discovery Example" author="dnpwwo" version="1.0.0">
    <params>
        <param field="Address" label="IP Address" width="200px" required="true" default="239.255.250.250"/>
        <param field="Port" label="Port" width="200px" required="true" default="9131"/>
        <param field="Mode6" label="Debug" width="100px">
            <options>
                <option label="True" value="Debug"/>
                <option label="File Only" value="File" />
                <option label="False" value="Normal"  default="true" />
            </options>
        </param>
    </params>
</plugin>
"""
import Domoticz

class BasePlugin:
    BeaconConn = None
    
    def __init__(self):
        return

    def onStart(self):
        if Parameters["Mode6"] == "Debug":
            Domoticz.Debugging(1)
            DumpConfigToLog()

        if Parameters["Mode6"] != "Normal":
            logFile = open(Parameters["HomeFolder"]+Parameters["Key"]+".log",'w')

        self.BeaconConn = Domoticz.Connection(Name="Beacon", Transport="UDP/IP", Address=Parameters["Address"], Port=str(Parameters["Port"]))
        self.BeaconConn.Listen()

    def onMessage(self, Connection, Data):
        try:
            strData = Data.decode("utf-8", "ignore")
            Domoticz.Log("onMessage called from: "+Connection.Address+":"+Connection.Port+" with data: "+strData)
            Domoticz.Debug("Connection detail: : "+str(Connection))

            if Parameters["Mode6"] != "Normal":
                logFile = open(Parameters["HomeFolder"]+Parameters["Key"]+".log",'a')
                logFile.write(Connection.Name+" ("+Connection.Address+"): "+strData+"\n")
                logFile.close()

        except Exception as inst:
            Domoticz.Error("Exception in onMessage, called with Data: '"+str(strData)+"'")
            Domoticz.Error("Exception detail: '"+str(inst)+"'")
            raise

global _plugin
_plugin = BasePlugin()

def onStart():
    global _plugin
    _plugin.onStart()

def onMessage(Connection, Data):
    global _plugin
    _plugin.onMessage(Connection, Data)

    # Generic helper functions
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
