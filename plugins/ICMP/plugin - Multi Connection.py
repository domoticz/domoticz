#           ICMP Plugin
#
#           Author:     Dnpwwo, 2017
#
"""
<plugin key="ICMP" name="ICMP Listener" author="dnpwwo" version="1.5.5">
    <description>
ICMP Pinger Plugin.<br/><br/>
Specify comma delimted addresses (IP or DNS names) of devices that are to be pinged.<br/>
When remote devices are found a matching Domoticz device is created in the Devices tab.
    </description>
    <params>
        <param field="Address" label="Address(es)" width="300px" required="true" default="127.0.0.1"/>
        <param field="Mode1" label="Ping Frequency" width="40px">
            <options>
                <option label="6" value="6"/>
                <option label="8" value="8"/>
                <option label="10" value="10" default="true" />
                <option label="12" value="12"/>
                <option label="14" value="14"/>
                <option label="16" value="16"/>
                <option label="18" value="18"/>
                <option label="20" value="20"/>
            </options>
        </param>
        <param field="Mode6" label="Debug" width="75px">
            <options>
                <option label="Verbose" value="Verbose"/>
                <option label="True" value="Debug"/>
                <option label="False" value="Normal"  default="true" />
            </options>
        </param>
    </params>
</plugin>
"""
import Domoticz
from datetime import datetime

class IcmpDevice:
    Address = ""
    icmpConn = None

    def __init__(self, destination):
        self.Address = destination
        self.Open()

    def __str__(self):
        if (self.icmpConn != None):
            return str(self.icmpConn)
        else:
            return "None"

    def Open(self):
        if (self.icmpConn != None):
            self.Close()
        self.icmpConn = Domoticz.Connection(Name=self.Address+" Connection", Transport="ICMP/IP", Protocol="ICMP", Address=self.Address)
        self.icmpConn.Listen()
    
    def Send(self):
        if (self.icmpConn == None):
            self.Open()
        self.icmpConn.Send("Domoticz", 1)
    
    def Close(self):
        self.icmpConn = None
    
class BasePlugin:
    icmpDict = {}
 
    def onStart(self):
        if Parameters["Mode6"] != "Normal":
            Domoticz.Debugging(1)
            DumpConfigToLog()

        endpointList = Parameters["Address"].split(",")
        for destination in endpointList:
            Domoticz.Debug("Endpoint '"+destination+"' found.")
            self.icmpDict[destination] = IcmpDevice(destination)
        
        Domoticz.Heartbeat(int(Parameters["Mode1"]))

    def onConnect(self, Connection, Status, Description):
        if (Status == 0):
            Domoticz.Log("Successful connect to: "+Connection.Address+" which is surprising because ICMP is connectionless.")
        else:
            Domoticz.Error("Error connecting to: "+Connection.Address+", Connection: "+Connection.Name+", Description: "+Description)
            del self.icmpDict[Connection.Address]

    def onMessage(self, Connection, Data):
        Domoticz.Debug("onMessage called for connection: '"+Connection.Name+"', with address: "+Connection.Address)
        if Parameters["Mode6"] == "Verbose":
            DumpICMPResponseToLog(Data)
        if isinstance(Data, dict) and (Data["Status"] == 0):
            iUnit = int(Data["IPv4"]["Source"].split(".")[3])
            Domoticz.Debug("Device: '"+Connection.Address+"' responding at: "+Data["IPv4"]["Source"]+", Unit number: "+str(iUnit))
            if not iUnit in Devices:
                # Store the address in the option field so that users can change the device name and IP address and it can still be mapped to
                Domoticz.Device(Name=Connection.Address, Unit=iUnit, Type=17, Subtype=0, Image=17, Options={"Address":Connection.Address}).Create()
                Domoticz.Log("Device: '"+Connection.Address+"' created and listed in Devices Tab.")
            UpdateDevice(iUnit, 1, "On")
        else:
            Domoticz.Debug("Device: '"+Connection.Address+"' returned '"+Data["Description"]+"'.")
            for Device in Devices:
                if (Devices[Device].Options["Address"] == Connection.Address):
                    UpdateDevice(Device, 0, "Off")
            # closing connections that return failures (and re-opening them later) is more reliable
            for Conn in self.icmpDict:
                if (self.icmpDict[Conn].Address == Connection.Address):
                    self.icmpDict[Conn].Close()

    def onHeartbeat(self):
        for destination in self.icmpDict:
            Domoticz.Log(str(self.icmpDict[destination]))
            self.icmpDict[destination].Send()
        
    def onDisconnect(self, Connection):
        Domoticz.Error("Disconnected from: "+Connection.Name+", Address: "+Connection.Address)
        for Conn in self.icmpDict:
            if (self.icmpDict[Conn].Address == Connection.Address):
                self.icmpDict[Conn].Close()

global _plugin
_plugin = BasePlugin()

def onStart():
    global _plugin
    _plugin.onStart()

def onConnect(Connection, Status, Description):
    global _plugin
    _plugin.onConnect(Connection, Status, Description)

def onMessage(Connection, Data):
    global _plugin
    _plugin.onMessage(Connection, Data)

def onHeartbeat():
    global _plugin
    _plugin.onHeartbeat()

def onDisconnect(Connection):
    global _plugin
    _plugin.onDisconnect(Connection)

def UpdateDevice(Unit, nValue, sValue):
    # Make sure that the Domoticz device still exists (they can be deleted) before updating it 
    if (Unit in Devices):
        if (Devices[Unit].nValue != nValue) or (Devices[Unit].sValue != sValue):
            Devices[Unit].Update(nValue, str(sValue))
            Domoticz.Log("Update "+str(nValue)+":'"+str(sValue)+"' ("+Devices[Unit].Name+")")
    return

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

def DumpICMPResponseToLog(icmpDict):
    if isinstance(icmpDict, dict):
        Domoticz.Debug("ICMP Details ("+str(len(icmpDict))+"):")
        for x in icmpDict:
            if isinstance(icmpDict[x], dict):
                Domoticz.Debug("--->'"+x+" ("+str(len(icmpDict[x]))+"):")
                for y in icmpDict[x]:
                    Domoticz.Debug("------->'" + y + "':'" + str(icmpDict[x][y]) + "'")
            else:
                Domoticz.Debug("--->'" + x + "':'" + str(icmpDict[x]) + "'")
    else:
        Domoticz.Debug(Data.decode("utf-8", "ignore"))
