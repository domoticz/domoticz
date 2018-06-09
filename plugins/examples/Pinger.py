#           ICMP Plugin
#
#           Author:     Dnpwwo, 2017 - 2018
#
"""
<plugin key="ICMP" name="Pinger (ICMP)" author="dnpwwo" version="3.1.1">
    <description>
ICMP Pinger Plugin.<br/><br/>
Specify comma delimted addresses (IP or DNS names) of devices that are to be pinged.<br/>
When remote devices are found a matching Domoticz device is created in the Devices tab.
    </description>
    <params>
        <param field="Address" label="Address(es) comma separated" width="300px" required="true" default="127.0.0.1"/>
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
        <param field="Mode6" label="Debug" width="150px">
            <options>
                <option label="None" value="0"  default="true" />
                <option label="Python Only" value="2"/>
                <option label="Basic Debugging" value="62"/>
                <option label="Basic+Messages" value="126"/>
                <option label="Connections Only" value="16"/>
                <option label="Connections+Python" value="18"/>
                <option label="Connections+Queue" value="144"/>
                <option label="All" value="-1"/>
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
        self.icmpConn = Domoticz.Connection(Name=self.Address, Transport="ICMP/IP", Protocol="ICMP", Address=self.Address)
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
        if Parameters["Mode6"] != "0":
            DumpConfigToLog()
            Domoticz.Debugging(int(Parameters["Mode6"]))
        Domoticz.Heartbeat(int(Parameters["Mode1"]))

        # Find devices that already exist, create those that don't
        self.icmpList = Parameters["Address"].replace(" ", "").split(",")
        for destination in self.icmpList:
            Domoticz.Debug("Endpoint '"+destination+"' found.")
            deviceFound = False
            for Device in Devices:
                if (("Name" in Devices[Device].Options) and (Devices[Device].Options["Name"] == destination)): deviceFound = True
            if (deviceFound == False):
                Domoticz.Device(Name=destination, Unit=len(Devices)+1, Type=243, Subtype=31, Image=17, Options={"Custom":"1;ms"}).Create()
                Domoticz.Device(Name=destination, Unit=len(Devices)+1, Type=17, Subtype=0, Image=17, Options={"Name":destination,"Related":str(len(Devices))}).Create()
              
        # Mark all devices as connection lost if requested
        deviceLost = 0
        if Parameters["Mode5"] == "True":
            deviceLost = 1
        for Device in Devices:
            UpdateDevice(Device, Devices[Device].nValue, Devices[Device].sValue, deviceLost)
                
    def onConnect(self, Connection, Status, Description):
        if (Status == 0):
            Domoticz.Log("Successful connect to: "+Connection.Address+" which is surprising because ICMP is connectionless.")
        else:
            Domoticz.Log("Failed to connect to: "+Connection.Address+", Description: "+Description)
        self.icmpConn = None

    def onMessage(self, Connection, Data):
        Domoticz.Debug("onMessage called for connection: '"+Connection.Name+"'")
        if Parameters["Mode6"] == "1":
            DumpICMPResponseToLog(Data)
        if isinstance(Data, dict) and (Data["Status"] == 0):
            iUnit = -1
            for Device in Devices:
                if ("Name" in Devices[Device].Options):
                    Domoticz.Debug("Checking: '"+Connection.Name+"' against '"+Devices[Device].Options["Name"]+"'")
                    if (Devices[Device].Options["Name"] == Connection.Name):
                        iUnit = Device
                        break
            if (iUnit > 0):
                # Device found, set it to On and if elapsed time suplied update related device
                UpdateDevice(iUnit, 1, "On", 0)
                relatedDevice = int(Devices[iUnit].Options["Related"])
                if ("ElapsedMs" in Data):
                    UpdateDevice(relatedDevice, Data["ElapsedMs"], str(Data["ElapsedMs"]), 0)
        else:
            Domoticz.Log("Device: '"+Connection.Name+"' returned '"+Data["Description"]+"'.")
            if Parameters["Mode6"] == "1":
                DumpICMPResponseToLog(Data)
            TimedOut = 0
            if Parameters["Mode5"] == "True": TimedOut = 1
            for Device in Devices:
                if (("Name" in Devices[Device].Options) and (Devices[Device].Options["Name"] == Connection.Name)):
                    UpdateDevice(Device, 0, "Off", TimedOut)
        self.icmpConn = None

    def onHeartbeat(self):
        Domoticz.Debug("Heartbeating...")

        # No response to previous heartbeat so mark as Off
        if (self.icmpConn != None):
            for Device in Devices:
                if (("Name" in Devices[Device].Options) and (Devices[Device].Options["Name"] == self.icmpConn.Name)):
                    Domoticz.Log("Device: '"+Devices[Device].Options["Name"]+"' address '"+self.icmpConn.Address+"' - No response.")
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
