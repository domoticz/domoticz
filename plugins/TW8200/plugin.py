# Epson TW8200 Projector Control via iTach IP2Ser
#
# Author: Dnpwwo, 2017
#
"""
<plugin key="TW8200" name="Epson TW8200" author="dnpwwo" version="1.0.2" externallink="http://www.epson.com.au/products/projector/EH-TW8200.asp">
    <params>
        <param field="Port" label="Port" width="50px" required="true" default="4999"/>
        <param field="Mode1" label="MAC Address, no colons" width="200px" required="true" />
        <param field="Mode2" label="Serial Config" width="300px" required="true" default="SERIAL,1:1,9600,FLOW_NONE,PARITY_NO"/>
        <param field="Mode6" label="Debug" width="80px">
            <options>
                <option label="True" value="Debug"/>
                <option label="False" value="Normal"  default="true" />
                <option label="File Only" value="File" />
            </options>
        </param>
    </params>
</plugin>
"""
import Domoticz

class BasePlugin:
    deviceConn = None
    hasSerial = False
    lastResponse = 0
    
    def __init__(self):
        return

    def onStart(self):
        if Parameters["Mode6"] == "Debug":
            Domoticz.Debugging(1)
            DumpConfigToLog()

        if Parameters["Mode6"] != "Normal":
            logFile = open(Parameters["HomeFolder"]+Parameters["Key"]+".log",'w')

        # Create icons and switch device if not present
        if ('TW8200'  not in Images): Domoticz.Image('TW8200.zip').Create()
        if (len(Devices) == 0):
            Domoticz.Device(Name="Status",  Unit=1, TypeName="Switch", Image=Images["TW8200"].ID).Create()
            Domoticz.Log("Device created in Devices page.")

        self.deviceConn = Domoticz.Connection(Name="Beacon", Transport="UDP/IP", Address="239.255.250.250", Port=str(9131))
        self.deviceConn.Listen()

        Domoticz.Heartbeat(25)

    def onStop(self):
        Domoticz.Log("onStop called")

    def onConnect(self, Connection, Status, Description):
        if (Connection == self.deviceConn):
            if (Status == 0):
                Domoticz.Log("Connected successfully to: "+Connection.Name+" at "+Connection.Address+":"+Connection.Port)
                if (Connection.Name == "iTach"):
                    self.deviceConn.Send("getdevices\r")
                elif (Connection.Name == "TW8200"):
                    self.lastResponse = 0
                    self.deviceConn.Send("PWR?\r")
            else:
                if (Description.find("Only one usage of each socket address") > 0):
                    Domoticz.Log(Connection.Address+":"+Connection.Port+" is busy, waiting.")
                else:
                    Domoticz.Log("Failed to connect ("+str(Status)+") to: "+Connection.Address+":"+Connection.Port)
                    Domoticz.Debug("Failed to connect ("+str(Status)+") to: "+Connection.Address+":"+Connection.Port+" with error: "+Description)
                self.deviceConn = None

    def onMessage(self, Connection, Data):
        strData = Data.decode("utf-8", "ignore")
        Domoticz.Debug("Message from: "+Connection.Name+" at "+Connection.Address+":"+Connection.Port+" with data: "+strData)

        if Parameters["Mode6"] != "Normal":
            logFile = open(Parameters["HomeFolder"]+Parameters["Key"]+".log",'a')
            logFile.write(Connection.Name+" ("+Connection.Address+"): "+strData+"\n")
            logFile.close()
        
        try:
            # Beacon messages to find the iTach
            if (Connection.Name == "Beacon"):
                if (strData.find("<-Model=iTachIP2SL>") >= 0):
                    if (strData.find("-UUID=GlobalCache_"+Parameters["Mode1"]) >= 0):
                        self.deviceConn = None
                        self.deviceConn = Domoticz.Connection(Name="iTach", Transport="TCP/IP", Protocol="Line", Address=Connection.Address, Port=str(4998))
                        self.deviceConn.Connect()
                        Domoticz.Log("iTach '"+Parameters["Mode1"]+"' discovered successfully at address: "+Connection.Address)
                    else:
                        Domoticz.Debug("Saw discovery message for wrong iTach")
                else:
                    Domoticz.Debug("Saw discovery message for device other than iTach")
            # iTach messages to check configuration
            elif (Connection.Name == "iTach"):
                self.lastResponse = 0
                if (strData.find("ETHERNET") >= 0):
                    Domoticz.Debug("ETHERNET response returned: "+strData)
                elif (strData.find("SERIAL") >= 6):
                    Domoticz.Debug("SERIAL response returned: "+strData)
                    self.hasSerial = True
                elif (strData == "endlistdevices"):
                    if (self.hasSerial == True):
                        Connection.Send("get_SERIAL,1:1\r")
                    else:
                        Domoticz.Error("iTach has not reported that it has a SERIAL device")
                elif (strData.find("SERIAL") >= 0):
                    if (strData == Parameters["Mode2"]):
                        Domoticz.Log("SERIAL port is configured correctly: "+strData)
                        self.deviceConn.Disconnect()
                        self.deviceConn = None
                        self.deviceConn = Domoticz.Connection(Name="TW8200", Transport="TCP/IP", Protocol="Line", Address=Connection.Address, Port=str(Parameters["Port"]))
                        self.deviceConn.Connect()
                    else:
                        Domoticz.Error("SERIAL details returned: "+strData+". Expected: "+Parameters["Mode2"])
                else:
                    Domoticz.Log("Unknown iTach message: "+strData)
            # TW8200 messages
            elif (Connection.Name == "TW8200"):
                self.lastResponse = 0
                Domoticz.Log("TW8200 response : "+strData)
                if (strData.find("PWR=00") >= 0):
                    UpdateDevice(1,0,"Off")
                else:
                    UpdateDevice(1,1,"On")
            else:
                Domoticz.Error("Message from unknown connection: "+str(Connection))
                Domoticz.Error("Message detail: "+strData)
        
        except Exception as inst:
            Domoticz.Error("Exception in onMessage, called with Data: '"+str(strData)+"'")
            Domoticz.Error("Exception detail: '"+str(inst)+"'")
            raise

    def onCommand(self, Unit, Command, Level, Hue):
        Domoticz.Log("onCommand called for Unit " + str(Unit) + ": Parameter '" + str(Command) + "', Level: " + str(Level))

        Command = Command.strip()
        action, sep, params = Command.partition(' ')
        action = action.capitalize()

        if ((self.deviceConn.Name == "TW8200") and self.deviceConn.Connected()):
            if (action == 'On'):
                self.deviceConn.Send("PWR ON\r")
            elif (action == 'Off'):
                self.deviceConn.Send("PWR OFF\r")
            else:
                Domoticz.Error("Unknown command: "+action)
        else:
            Domoticz.Log("Command ignored, TW8200 is not connected.")

    def onDisconnect(self, Connection):
        Domoticz.Log("Connection to "+Connection.Name+" disconnected.")
        self.deviceConn = None
        return

    def onHeartbeat(self):
        Domoticz.Debug("onHeartbeat called, last response seen "+str(self.lastResponse)+" heartbeats ago.")
        if (self.deviceConn == None):
            self.deviceConn = Domoticz.Connection(Name="Beacon", Transport="UDP/IP", Address="239.255.250.250", Port=str(9131))
            self.deviceConn.Listen()
        else:
            if (self.deviceConn.Name == "TW8200") and (self.deviceConn.Connected()):
                self.deviceConn.Send("PWR?\r")
                
            if (self.lastResponse > 5):
                Domoticz.Error(self.deviceConn.Name+" has not responded to 5 pings, terminating connection.")
                self.deviceConn = None
                self.lastResponse = -1
            self.lastResponse = self.lastResponse + 1
        
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
    
def UpdateDevice(Unit, nValue, sValue):
    # Make sure that the Domoticz device still exists (they can be deleted) before updating it 
    if (Unit in Devices):
        if (Devices[Unit].nValue != nValue) or (Devices[Unit].sValue != sValue):
            Devices[Unit].Update(nValue, str(sValue))
            Domoticz.Log("Update "+str(nValue)+":'"+str(sValue)+"' ("+Devices[Unit].Name+")")
    return
