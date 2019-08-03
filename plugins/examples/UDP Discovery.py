# UDP Discovery Example
#
# Logs traffic on the Dynamic Device Discovery port 
#
# Works on Linux, Windows appears to filter out UDP from the network even when the firewall is set to allow it
#
# Useful IP Address and Port combinations that can be set via the Hardware page:
#    239.255.250.250:9161 - Dynamic Device Discovery (DDD) (default, shows Global Cache, Denon Amps and more)
#    239.255.255.250:1900 - Simple Service Discovery Protocol (SSDP), (shows Windows, Kodi, Denon, Chromebooks, Gateways, ...)
# Author: Dnpwwo, 2017
#
"""
<plugin key="UdpDiscover" name="UDP Discovery Example" author="dnpwwo" version="2.2.0">
    <params>
        <param field="Mode1" label="Discovery Type" width="275px">
            <options>
                <option label="Dynamic Device Discovery" value="239.255.250.250:9161"/>
                <option label="Simple Service Discovery Protocol" value="239.255.255.250:1900"  default="true" />
            </options>
        </param>
        <param field="Mode2" label="Create Devices" width="75px">
            <options>
                <option label="True" value="True"/>
                <option label="False" value="False"  default="true" />
            </options>
        </param>
        <param field="Mode6" label="Debug" width="150px">
            <options>
                <option label="None" value="0"  default="true" />
                <option label="Python Only" value="2"/>
                <option label="Basic Debugging" value="62"/>
                <option label="Basic+Messages" value="126"/>
                <option label="Connections Only" value="16"/>
                <option label="Connections+Queue" value="144"/>
                <option label="All" value="-1"/>
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
        if Parameters["Mode6"] != "0":
            Domoticz.Debugging(int(Parameters["Mode6"]))
            DumpConfigToLog()

        if Parameters["Mode6"] != "Normal":
            logFile = open(Parameters["HomeFolder"]+Parameters["Key"]+".log",'w')

        sAddress, sep, sPort = Parameters["Mode1"].partition(':')
        self.BeaconConn = Domoticz.Connection(Name="Beacon", Transport="UDP/IP", Address=sAddress, Port=str(sPort))
        self.BeaconConn.Listen()

    def onMessage(self, Connection, Data):
        try:
            strData = Data.decode("utf-8", "ignore")
            Domoticz.Log("onMessage called from: "+Connection.Address+":"+Connection.Port+" with data: "+strData)
            Domoticz.Debug("Connection detail: : "+str(Connection))

            if (Parameters["Mode2"] == "True"):
                existingDevice = 0
                existingName = (Parameters["Name"]+" - "+Connection.Address)
                for dev in Devices:
                    if (Devices[dev].Name == existingName):
                        existingDevice = dev
                if (existingDevice == 0):
                    Domoticz.Device(Name=Connection.Address, Unit=len(Devices)+1, TypeName="Text", Image=17).Create()
                    Domoticz.Log("Created device: "+Connection.Address)
                    Devices[len(Devices)].Update(nValue=1, sValue=Connection.Address)
                else:
                    Devices[existingDevice].Update(nValue=1, sValue=Connection.Address)

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
