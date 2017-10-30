#           ICMP Plugin
#
#           Author:     Dnpwwo, 2017
#
"""
<plugin key="ICMP" name="Pinger (ICMP)" author="dnpwwo" version="2.0.6">
    <description>
ICMP Pinger Plugin.<br/><br/>
Specify comma delimted addresses (IP or DNS names) of devices that are to be pinged.<br/>
When remote devices are found a matching Domoticz device is created in the Devices tab.
    </description>
    <params>
        <param field="Address" label="Address(es)" width="300px" required="true" default="127.0.0.1"/>
        <param field="Mode1" label="Ping Frequency" width="40px">
            <options>
                <option label="2" value="2"/>
                <option label="3" value="3"/>
                <option label="4" value="4"/>
                <option label="5" value="5"/>
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
        <param field="Mode5" label="Time Out Lost Devices" width="75px">
            <options>
                <option label="True" value="True" default="true"/>
                <option label="False" value="False" />
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
        else:
            self.icmpConn.Send("Domoticz")
    
    def Close(self):
        self.icmpConn = None
    
class BasePlugin:
    icmpConn = None
    icmpList = []
    nextDev = 0
 
    def onStart(self):
        if Parameters["Mode6"] != "Normal":
            DumpConfigToLog()
            #Domoticz.Debugging(1)
        self.icmpList = Parameters["Address"].split(",")
        for destination in self.icmpList:
            Domoticz.Debug("Endpoint '"+destination+"' found.")
        Domoticz.Heartbeat(int(Parameters["Mode1"]))
        if Parameters["Mode5"] == "True":
            for Device in Devices:
                UpdateDevice(Device, Devices[Device].nValue, Devices[Device].sValue, 1)

    def onConnect(self, Connection, Status, Description):
        if (Status == 0):
            Domoticz.Log("Successful connect to: "+Connection.Address+" which is surprising because ICMP is connectionless.")
        else:
            Domoticz.Log("Failed to connect to: "+Connection.Address+", Description: "+Description)
            Conn.Close()
        self.icmpConn = None

    def onMessage(self, Connection, Data):
        Domoticz.Debug("onMessage called for connection: '"+Connection.Name+"', with address: "+Connection.Address)
        if Parameters["Mode6"] == "Verbose":
            DumpICMPResponseToLog(Data)
        if isinstance(Data, dict) and (Data["Status"] == 0):
            iUnit = int(Data["IPv4"]["Source"].split(".")[3])
            Domoticz.Log("Device: '"+Connection.Address+"' responding at: "+Data["IPv4"]["Source"]+", Unit number: "+str(iUnit))
            if not iUnit in Devices:
                # Store the address in the option field so that users can change the device name and IP address and it can still be mapped to
                Domoticz.Device(Name=Connection.Address, Unit=iUnit, Type=17, Subtype=0, Image=17, Options={"Address":Connection.Address}).Create()
                Domoticz.Log("Device: '"+Connection.Address+"' created and listed in Devices Tab.")
            UpdateDevice(iUnit, 1, "On", 0)
        else:
            Domoticz.Log("Device: '"+Connection.Address+"' returned '"+Data["Description"]+"'.")
            if Parameters["Mode6"] == "Verbose":
                DumpICMPResponseToLog(Data)
            TimedOut = 0
            if Parameters["Mode5"] == "True": TimedOut = 1
            for Device in Devices:
                if (Devices[Device].Options["Address"] == Connection.Address):
                    UpdateDevice(Device, 0, "Off", TimedOut)
        self.icmpConn = None

    def onHeartbeat(self):
        # No response to previous heartbeat so mark as Off
        if (self.icmpConn != None):
            for Device in Devices:
                if (Devices[Device].Options["Address"] == self.icmpConn.Address):
                    Domoticz.Log("Device: '"+self.icmpConn.Address+"' - No response.")
                    TimedOut = 0
                    if Parameters["Mode5"] == "True": TimedOut = 1
                    UpdateDevice(Device, 0, "Off", TimedOut)
                    break
            self.icmpConn = None
    
        Domoticz.Debug("Heartbeating '"+self.icmpList[self.nextDev]+"'")
        self.icmpConn = IcmpDevice(self.icmpList[self.nextDev])
        self.nextDev += 1
        if (self.nextDev >= len(self.icmpList)):
            self.nextDev = 0
 
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

def UpdateDevice(Unit, nValue, sValue, TimedOut):
    # Make sure that the Domoticz device still exists (they can be deleted) before updating it 
    if (Unit in Devices):
        if (Devices[Unit].nValue != nValue) or (Devices[Unit].sValue != sValue) or (Devices[Unit].TimedOut != TimedOut):
            Devices[Unit].Update(nValue=nValue, sValue=str(sValue), TimedOut=TimedOut)
            Domoticz.Log("Update "+str(nValue)+":'"+str(sValue)+"' ("+Devices[Unit].Name+")")
    return

def DumpConfigToLog():
    for x in Parameters:
        if Parameters[x] != "":
            Domoticz.Log( "'" + x + "':'" + str(Parameters[x]) + "'")
    Domoticz.Log("Device count: " + str(len(Devices)))
    for x in Devices:
        Domoticz.Log("Device:           " + str(x) + " - " + str(Devices[x]))
        Domoticz.Log("Device ID:       '" + str(Devices[x].ID) + "'")
        Domoticz.Log("Device Name:     '" + Devices[x].Name + "'")
        Domoticz.Log("Device nValue:    " + str(Devices[x].nValue))
        Domoticz.Log("Device sValue:   '" + Devices[x].sValue + "'")
        Domoticz.Log("Device LastLevel: " + str(Devices[x].LastLevel))
    return

def DumpICMPResponseToLog(icmpList):
    if isinstance(icmpList, dict):
        Domoticz.Log("ICMP Details ("+str(len(icmpList))+"):")
        for x in icmpList:
            if isinstance(icmpList[x], dict):
                Domoticz.Log("--->'"+x+" ("+str(len(icmpList[x]))+"):")
                for y in icmpList[x]:
                    Domoticz.Log("------->'" + y + "':'" + str(icmpList[x][y]) + "'")
            else:
                Domoticz.Log("--->'" + x + "':'" + str(icmpList[x]) + "'")
    else:
        Domoticz.Log(Data.decode("utf-8", "ignore"))
