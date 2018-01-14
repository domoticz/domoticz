#           MQTT Test Plugin
#
"""
<plugin key="MoTT_Test" name="MoTT Test" version="0.0.1">
    <description>
MQTT test plugin.<br/><br/>
Specify MQTT server and port.<br/>
<br/>
Creates 2 devices per address, one to show status and one to show how long ping took.<br/>
    </description>
    <params>
        <param field="Address" label="MQTT Server address" width="300px" required="true" default="127.0.0.1"/>
        <param field="Port" label="Port" width="300px" required="true" default="1883"/>
        <param field="Username" label="Username" width="300px"/>
        <param field="Password" label="Password" width="300px"/>
        <param field="Mode1" label="CA Filename" width="300px"/>
           
        <param field="Mode2" label="Topic" width="300px" default="Domoticz_test"/>
        
        <param field="Mode3" label="Publish Interval (s)" width="40px">
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
import json

class MqttClient:
    Address = ""
    Port = ""
    mqttConn = None

    def __init__(self, destination, port):
        Domoticz.Debug("MqttDevice::__init__")
        self.Address = destination
        self.Port = port
        self.Open()

    def __str__(self):
        Domoticz.Debug("MqttDevice::__str__")
        if (self.mqttConn != None):
            return str(self.mqttConn)
        else:
            return "None"

    def Open(self):
        Domoticz.Debug("MqttDevice::Open")
        if (self.mqttConn != None):
            self.Close()
        self.mqttConn = Domoticz.Connection(Name=self.Address, Transport="MQTT", Protocol="MQTT", Address=self.Address, Port=self.Port)
        self.mqttConn.Connect()
    
    def Publish(self, topic, payload):
        Domoticz.Debug("MqttDevice::Send")
        if (self.mqttConn == None):
            self.Open()
        else:
            #self.mqttConn.Send("Domoticz!")
            self.mqttConn.Send({'Topic': topic, 'Payload': bytearray(payload, 'utf8')})
    
    def Subscribe(self, topic):
        Domoticz.Debug("MqttDevice::Send")
        if (self.mqttConn == None):
            self.Open()
        else:
            #self.mqttConn.Send("Domoticz!")
            self.mqttConn.Send({'Type': 'Subscribe', 'Topic': topic})
    
    def Close(self):
        Domoticz.Debug("MqttDevice::Close")
        self.mqttConn = None
    
class BasePlugin:
    mqttClient = None
    mqttserveraddress = ""
    mqttserverport = ""
    mqtttopic = ""
    nextDev = 0
 
    def onStart(self):
        if Parameters["Mode6"] != "Normal":
            DumpConfigToLog()
            Domoticz.Debugging(1)
        Domoticz.Heartbeat(int(Parameters["Mode3"]))

        # Find devices that already exist, create those that don't
        self.mqttserveraddress = Parameters["Address"].replace(" ", "")
        self.mqttserverport = Parameters["Port"].replace(" ", "")
        self.mqtttopic = Parameters["Mode2"]
        #for destination in self.icmpList:
        #    Domoticz.Debug("Endpoint '"+destination+"' found.")
        #    deviceFound = False
        #    for Device in Devices:
        #        if (("Name" in Devices[Device].Options) and (Devices[Device].Options["Name"] == destination)): deviceFound = True
        #    if (deviceFound == False):
        #        Domoticz.Device(Name=destination, Unit=len(Devices)+1, Type=243, Subtype=31, Image=17, Options={"Custom":"1;ms"}).Create()
        #        Domoticz.Device(Name=destination, Unit=len(Devices)+1, Type=17, Subtype=0, Image=17, Options={"Name":destination,"Related":str(len(Devices))}).Create()
              
        # Mark all devices as connection lost if requested
        deviceLost = 0
        #if Parameters["Mode5"] == "True":
        #    deviceLost = 1
        for Device in Devices:
            UpdateDevice(Device, Devices[Device].nValue, Devices[Device].sValue, deviceLost)
                
    def onConnect(self, Connection, Status, Description):
        if (Status == 0):
            Domoticz.Log("Successful connect to: "+Connection.Address+":"+Connection.Port)
            self.mqttClient.Subscribe(self.mqtttopic)
        else:
            Domoticz.Log("Failed to connect to: "+Connection.Address+":"+Connection.Port+", Description: "+Description)
        #self.icmpConn = None

    def onMessage(self, Connection, Data):
        Domoticz.Debug("onMessage called for connection: '"+Connection.Name+"' topic:'"+Data['Topic']+"' payload:'"+str(len(Data['Payload'])))
        #Domoticz.Debug("onMessage called for connection: '"+Connection.Name+"' topic:'"+Data['Topic']+"' payload:'"+Data['Payload'].decode('utf8')+"'")
        #Domoticz.Debug("onMessage called for connection: '"+Connection.Name+"' topic:'"+Data['Topic']+"' payload:'"+Data+"'")
        topic = Data['Topic']
        message = Data['Payload'].decode('utf8')
        if Parameters["Mode6"] == "Verbose":
            DumpMQTTMessageToLog(topic, message)
        # if isinstance(Data, dict) and (Data["Status"] == 0):
           # iUnit = -1
           # for Device in Devices:
               # if ("Name" in Devices[Device].Options):
                   # Domoticz.Debug("Checking: '"+Connection.Name+"' against '"+Devices[Device].Options["Name"]+"'")
                   # if (Devices[Device].Options["Name"] == Connection.Name):
                       # iUnit = Device
           # if (iUnit > 0):
              # Device found, set it to On and if elapsed time suplied update related device
                # UpdateDevice(iUnit, 1, "On", 0)
                # relatedDevice = int(Devices[iUnit].Options["Related"])
                # if ("ElapsedMs" in Data):
                    # UpdateDevice(relatedDevice, Data["ElapsedMs"], str(Data["ElapsedMs"]), 0)
        # else:
            # Domoticz.Log("Device: '"+Connection.Name+"' received '"+Data["Description"]+"'.")
            # if Parameters["Mode6"] == "Verbose":
                # DumpMQTTMessageToLog(Data)
            # TimedOut = 0
            # if Parameters["Mode5"] == "True": TimedOut = 1
            # for Device in Devices:
                # if (("Name" in Devices[Device].Options) and (Devices[Device].Options["Name"] == Connection.Name)):
                    # UpdateDevice(Device, 0, "Off", TimedOut)
        #self.icmpConn = None

    def onHeartbeat(self):
        Domoticz.Debug("Heartbeating...")

        # No response to previous heartbeat so mark as Off
        #if (self.mqttConn != None):
        #    for Device in Devices:
        #        if (Devices[Device].Options["Name"] == self.icmpConn.Name):
        #            Domoticz.Log("Device: '"+Devices[Device].Options["Name"]+"' address '"+self.icmpConn.Address+"' - No response.")
        #            TimedOut = 0
        #            if Parameters["Mode5"] == "True": TimedOut = 1
        #            UpdateDevice(Device, 0, "Off", TimedOut)
        #            break
        #    self.icmpConn = None

        #try:
        #Domoticz.Debug("Heartbeating '"+self.mqttClient+"'")
        #except IndexError:
        #    Domoticz.Log("Exception: IndexError, nextDev is '"+str(self.nextDev)+"', size of icmpList is '"+str(len(self.icmpList))+"'")
        if (self.mqttClient == None):
            self.mqttClient = MqttClient(self.mqttserveraddress, self.mqttserverport)
        if (self.mqttClient != None):
            self.mqttClient.Publish(self.mqtttopic, 'Domoticz!')
        #self.nextDev += 1
        #if (self.nextDev >= len(self.icmpList)):
        #    self.nextDev = 0
 
 
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

def DumpMQTTMessageToLog(topic, message):
    #Domoticz.Log(topic+":"+message)
	Domoticz.Log(topic)
    