# Basic Python SNMP Example
#
# Author: Dnpwwo, 2017
#
"""
<plugin key="SNMP" name="SNMP Example" author="dnpwwo" version="1.2.0" externallink="https://pypi.python.org/pypi/rpdb/">
    <params>
        <param field="Address" label="IP Address" width="200px" required="true" default="224.0.0.1"/>
        <param field="Port" label="Port" width="30px" required="true" default="8888"/>
        <param field="Mode6" label="Debug" width="100px">
            <options>
                <option label="True" value="Debug"/>
                <option label="Python" value="Debugger"/>
                <option label="Logging" value="File"/>
                <option label="False" value="Normal"  default="true" />
            </options>
        </param>
    </params>
</plugin>
"""
import Domoticz
from pyasn1.codec.ber import encoder, decoder
from pysnmp.proto import api

class BasePlugin:

    udpListenConn = None
    udpBcastConn = None

    def __init__(self):
        return

    def onStart(self):
        if Parameters["Mode6"] != "Normal":
            Domoticz.Debugging(1)
        if Parameters["Mode6"] == "Debugger":
            Domoticz.Log("Debugger started, use 'telnet 0.0.0.0 4444' to connect")
            import rpdb
            #rpdb.set_trace()
        DebugMessage("Plugin has " + str(len(Devices)) + " devices associated with it.")
        DumpConfigToLog()

        # Create listener connection to catch response
        self.udpListenConn = Domoticz.Connection(Name="UDP Listen Connection", Transport="UDP/IP", Protocol="None", Port=Parameters["Port"])
        self.udpListenConn.Listen()
       
        pMod = api.protoModules[api.protoVersion2c]

        # Build PDU
        reqPDU = pMod.GetRequestPDU()
        pMod.apiPDU.setDefaults(reqPDU)
        pMod.apiPDU.setVarBinds(reqPDU, (('1.3.6.1.2.1.1.1.0', pMod.Null('')), ('1.3.6.1.2.1.1.3.0', pMod.Null(''))))

        # Build message
        reqMsg = pMod.Message()
        pMod.apiMessage.setDefaults(reqMsg)
        pMod.apiMessage.setCommunity(reqMsg, 'public')
        pMod.apiMessage.setPDU(reqMsg, reqPDU)

         # Create broadcast connection to send messages
        self.udpBcastConn = Domoticz.Connection(Name="UDP Broadcast Connection", Transport="UDP/IP", Protocol="None", Address=Parameters["Address"], Port=Parameters["Port"])
        self.udpBcastConn.Send(encoder.encode(reqMsg))
        
        DebugMessage("Leaving on start")

    def onConnect(self, Connection, Status, Description):
        DebugMessage("Connected successfully to: "+Connection.Address+":"+Connection.Port)

    def onMessage(self, Connection, Data):
        DebugMessage("onMessage called for connection to: "+Connection.Address+":"+Connection.Port)

    def onCommand(self, Unit, Command, Level, Hue):
        DebugMessage("onCommand called for Unit " + str(Unit) + ": Parameter '" + str(Command) + "', Level: " + str(Level))

    def onDisconnect(self, Connection):
        DebugMessage("onDisconnect called for connection to: "+Connection.Address+":"+Connection.Port)

    def onHeartbeat(self):
        return

    def onStop(self):
        DebugMessage("onStop called")

global _plugin
_plugin = BasePlugin()

def onStart():
    global _plugin
    _plugin.onStart()

def onStop():
    global _plugin
    _plugin.onStop()

def onConnect(Connection, Status, Description):
    global _plugin
    _plugin.onConnect(Connection, Status, Description)

def onMessage(Connection, Data):
    global _plugin
    _plugin.onMessage(Connection, Data)

def onCommand(Unit, Command, Level, Hue):
    global _plugin
    _plugin.onCommand(Unit, Command, Level, Hue)

def onDisconnect(Connection):
    global _plugin
    _plugin.onDisconnect(Connection)

def onHeartbeat():
    global _plugin
    _plugin.onHeartbeat()

# Generic helper functions
def DebugMessage(Message):
    if Parameters["Mode6"] == "File":
        f = open(Parameters["HomeFolder"]+Parameters["Key"]+".log","w")
        f.write(Message)
        f.close()
    Domoticz.Debug(Message)

def DumpConfigToLog():
    for x in Parameters:
        if Parameters[x] != "":
            DebugMessage( "'" + x + "':'" + str(Parameters[x]) + "'")
    DebugMessage("Device count: " + str(len(Devices)))
    for x in Devices:
        DebugMessage("Device:           " + str(x) + " - " + str(Devices[x]))
        DebugMessage("Device ID:       '" + str(Devices[x].ID) + "'")
        DebugMessage("Device Name:     '" + Devices[x].Name + "'")
        DebugMessage("Device nValue:    " + str(Devices[x].nValue))
        DebugMessage("Device sValue:   '" + Devices[x].sValue + "'")
        DebugMessage("Device LastLevel: " + str(Devices[x].LastLevel))
    return