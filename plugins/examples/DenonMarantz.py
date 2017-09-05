#
#       Denon AVR 4306 Plugin
#
#       Author:     Dnpwwo, 2016 - 2017
#
#   Mode3 ("Sources") needs to have '|' delimited names of sources that the Denon knows about.  The Selector can be changed afterwards to any  text and the plugin will still map to the actual Denon name.
#
"""
<plugin key="Denon4306" version="2.6.2" name="Denon/Marantz Amplifier" author="dnpwwo" wikilink="" externallink="http://www.denon.co.uk/uk">
    <description>
Denon (& Marantz) AVR Plugin.<br/><br/>
&quot;Sources&quot; need to have '|' delimited names of sources that the Denon knows about from the technical manual.<br/>
The Sources Selector(s) can be changed after initial creation to any text and the plugin will still map to the actual Denon name.<br/><br/>
Devices will be created in the Devices Tab only and will need to be manually made active.
    </description>
    <params>
        <param field="Address" label="IP Address" width="200px" required="true" default="127.0.0.1"/>
        <param field="Port" label="Port" width="30px" required="true" default="23"/>
        <param field="Mode2" label="Startup Delay" width="50px" required="true">
            <options>
                <option label="2" value="2"/>
                <option label="3" value="3"/>
                <option label="4" value="4" default="true" />
                <option label="5" value="5"/>
                <option label="6" value="6"/>
                <option label="7" value="7"/>
                <option label="10" value="10"/>
            </options>
        </param>
        <param field="Mode3" label="Sources" width="550px" required="true" default="Off|DVD|VDP|TV|CD|DBS|Tuner|Phono|VCR-1|VCR-2|V.Aux|CDR/Tape|AuxNet|AuxIPod"/>
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
import base64
import datetime

class BasePlugin:
    TelnetConn = None

    nextConnect = 3
    oustandingPings = 0

    powerOn = False

    mainOn = False
    mainSource = 0
    mainVolume1 = 0
    
    zone2On = False
    zone2Source = 0
    zone2Volume = 0
    
    zone3On = False
    zone3Source = 0
    zone3Volume = 0

    ignoreMessages = "|SS|SV|SD|MS|PS|CV|SY|TP|"
    selectorMap = {}
    pollingDict =  {"PW":"ZM?\r", "ZM":"SI?\r", "SI":"MV?\r", "MV":"MU?\r", "MU":"PW?\r" }
    lastMessage = ""
    lastHeartbeat = datetime.datetime.now()

    SourceOptions = {}
    
    def onStart(self):
        if Parameters["Mode6"] == "Debug":
            Domoticz.Debugging(1)

        self.SourceOptions = {'LevelActions': '|'*Parameters["Mode3"].count('|'),
                             'LevelNames': Parameters["Mode3"],
                             'LevelOffHidden': 'false',
                             'SelectorStyle': '1'}
            
        if (len(Devices) == 0):
            Domoticz.Device(Name="Power", Unit=1, TypeName="Switch",  Image=5).Create()
            Domoticz.Device(Name="Main Zone", Unit=2, TypeName="Selector Switch", Switchtype=18, Image=5, Options=self.SourceOptions).Create()
            Domoticz.Device(Name="Main Volume", Unit=3, Type=244, Subtype=73, Switchtype=7, Image=8).Create()
        else:
            if (2 in Devices and (len(Devices[2].sValue) > 0)):
                self.mainSource = int(Devices[2].sValue)
                self.mainOn = (Devices[2].nValue != 0)
            if (3 in Devices and (len(Devices[3].sValue) > 0)):
                self.mainVolume1 = int(Devices[3].sValue) if (Devices[3].nValue != 0) else int(Devices[3].sValue)*-1
            if (4 in Devices and (len(Devices[4].sValue) > 0)):
                self.zone2Source = int(Devices[4].sValue)
                self.zone2On = (Devices[4].nValue != 0)
            if (5 in Devices and (len(Devices[5].sValue) > 0)):
                self.zone2Volume = int(Devices[5].sValue) if (Devices[5].nValue != 0) else int(Devices[5].sValue)*-1
            if (6 in Devices and (len(Devices[6].sValue) > 0)):
                self.zone3Source = int(Devices[6].sValue)
                self.zone3On = (Devices[6].nValue != 0)
            if (7 in Devices and (len(Devices[7].sValue) > 0)):
                self.zone3Volume = int(Devices[7].sValue) if (Devices[7].nValue != 0) else int(Devices[7].sValue)*-1
            if (1 in Devices):
                self.powerOn = (self.mainOn or self.zone2On or self.zone3On)
                
        DumpConfigToLog()
        dictValue=0
        for item in Parameters["Mode3"].split('|'):
            self.selectorMap[dictValue] = item
            dictValue = dictValue + 10
        self.TelnetConn = Domoticz.Connection(Name="Telnet", Transport="TCP/IP", Protocol="Line", Address=Parameters["Address"], Port=Parameters["Port"])
        self.TelnetConn.Connect()
        return

    def onConnect(self, Connection, Status, Description):
        if (Connection == self.TelnetConn):
            if (Status == 0):
                Domoticz.Log("Connected successfully to: "+Connection.Address+":"+Connection.Port)
                self.TelnetConn.Send('PW?\r')
                self.TelnetConn.Send('ZM?\r', Delay=1)
                self.TelnetConn.Send('Z2?\r', Delay=2)
                self.TelnetConn.Send('Z3?\r', Delay=3)
            else:
                self.powerOn = False
                Domoticz.Log("Failed to connect ("+str(Status)+") to: "+Parameters["Address"]+":"+Parameters["Port"])
                Domoticz.Debug("Failed to connect ("+str(Status)+") to: "+Parameters["Address"]+":"+Parameters["Port"]+" with error: "+Description)
                self.SyncDevices()
        return

    def onMessage(self, Connection, Data):
        self.oustandingPings = self.oustandingPings - 1
        strData = Data.decode("utf-8", "ignore")
        Domoticz.Debug("onMessage called with Data: '"+str(strData)+"'")
        
        strData = strData.strip()
        action = strData[0:2]
        detail = strData[2:]
        if (action in self.pollingDict): self.lastMessage = action

        if (action == "PW"):        # Power Status
            if (detail == "STANDBY"):
                self.powerOn = False
            elif (detail == "ON"):
                self.powerOn = True
            else: Domoticz.Debug("Unknown: Action "+action+", Detail '"+detail+"' ignored.")
        elif (action == "ZM"):      # Main Zone on/off
            if (detail == "ON"):
                self.mainOn = True
            elif (detail == "OFF"):
                self.mainOn = False
            else: Domoticz.Debug("Unknown: Action "+action+", Detail '"+detail+"' ignored.")
        elif (action == "SI"):      # Main Zone Source Input
            for key, value in self.selectorMap.items():
                if (detail == value):      self.mainSource = key
        elif (action == "MV"):      # Master Volume
            if (detail.isdigit()):
                if (abs(self.mainVolume1) != int(detail[0:2])): self.mainVolume1 = int(detail[0:2])
            elif (detail[0:3] == "MAX"): Domoticz.Debug("Unknown: Action "+action+", Detail '"+detail+"' ignored.")
            else: Domoticz.Log("Unknown: Action "+action+", Detail '"+detail+"' ignored.")
        elif (action == "MU"):      # Overall Mute
            if (detail == "ON"):         self.mainVolume1 = abs(self.mainVolume1)*-1
            elif (detail == "OFF"):      self.mainVolume1 = abs(self.mainVolume1)
            else: Domoticz.Debug("Unknown: Action "+action+", Detail '"+detail+"' ignored.")
        elif (action == "Z2"):      # Zone 2
            # Zone 2 response, make sure we have Zone 2 devices in Domoticz and they are polled
            if (4 not in Devices):
                LevelActions = '|'*Parameters["Mode3"].count('|')
                Domoticz.Device(Name="Zone 2", Unit=4, TypeName="Selector Switch", Switchtype=18, Image=5, Options=self.SourceOptions).Create()
                Domoticz.Log("Zone 2 responded, devices added.")
            if (5 not in Devices):
                Domoticz.Device(Name="Volume 2", Unit=5, Type=244, Subtype=73, Switchtype=7, Image=8).Create()
            if ("Z2" not in self.pollingDict):
                self.pollingDict = {"PW":"ZM?\r", "ZM":"SI?\r", "SI":"MV?\r", "MV":"MU?\r", "MU":"Z2?\r", "Z2":"PW?\r" }

            if (detail == "ON"):
                self.zone2On = True
            elif (detail == "OFF"):
                self.zone2On = False
            elif (detail == "MUON"):
                self.zone2Volume = abs(self.zone2Volume)*-1
            elif (detail == "MUOFF"):
                self.zone2Volume = abs(self.zone2Volume)
            elif (detail.isdigit()):
                if (abs(self.zone2Volume) != int(detail[0:2])): self.zone2Volume = int(detail[0:2])
            else:
                for key, value in self.selectorMap.items():
                    if (detail == value):      self.zone2Source = key
        elif (action == "Z3"):      # Zone 3
            # Zone 3 response, make sure we have Zone 3 devices in Domoticz and they are polled
            if (6 not in Devices):
                LevelActions = '|'*Parameters["Mode3"].count('|')
                Domoticz.Device(Name="Zone 3", Unit=6, TypeName="Selector Switch", Switchtype=18, Image=5, Options=self.SourceOptions).Create()
                Domoticz.Log("Zone 3 responded, devices added.")
            if (7 not in Devices):
                Domoticz.Device(Name="Volume 3", Unit=7, Type=244, Subtype=73, Switchtype=7, Image=8).Create()
            if ("Z3" not in self.pollingDict):
                self.pollingDict = {"PW":"ZM?\r", "ZM":"SI?\r", "SI":"MV?\r", "MV":"MU?\r", "MU":"Z2?\r", "Z2":"Z3?\r", "Z3":"PW?\r" }
                
            if (detail == "ON"):
                self.zone3On = True
            elif (detail == "OFF"):
                self.zone3On = False
            elif (detail == "MUON"):
                self.zone3Volume = abs(self.zone3Volume)*-1
            elif (detail == "MUOFF"):
                self.zone3Volume = abs(self.zone3Volume)
            elif (detail.isdigit()):
                if (abs(self.zone3Volume) != int(detail[0:2])): self.zone3Volume = int(detail[0:2])
            else:
                for key, value in self.selectorMap.items():
                    if (detail == value):      self.zone3Source = key
        else:
            if (self.ignoreMessages.find(action) < 0):
                Domoticz.Debug("Unknown message '"+action+"' ignored.")
        self.SyncDevices()
        return

    def onCommand(self, Unit, Command, Level, Hue):
        Domoticz.Log("onCommand called for Unit " + str(Unit) + ": Parameter '" + str(Command) + "', Level: " + str(Level))

        Command = Command.strip()
        action, sep, params = Command.partition(' ')
        action = action.capitalize()
        params = params.capitalize()
        delay = 0
        if (self.powerOn == False):
            delay = int(Parameters["Mode2"])
        else:
            # Amp will ignore commands if it is responding to a heartbeat so delay send
            lastHeartbeatDelta = (datetime.datetime.now()-self.lastHeartbeat).total_seconds()
            if (lastHeartbeatDelta < 0.5):
                delay = 1
                Domoticz.Log("Last heartbeat was "+str(lastHeartbeatDelta)+" seconds ago, delaying command send.")

        if (Unit == 1):     # Main power switch
            if (action == "On"):
                self.TelnetConn.Send(Message='PWON\r')
            elif (action == "Off"):
                self.TelnetConn.Send(Message='PWSTANDBY\r', Delay=delay)

        # Main Zone devices
        elif (Unit == 2):     # Main selector
            if (action == "On"):
                self.TelnetConn.Send(Message='ZMON\r')
            elif (action == "Set"):
                if (self.powerOn == False): self.TelnetConn.Send(Message='PWON\r')
                self.TelnetConn.Send(Message='SI'+self.selectorMap[Level]+'\r', Delay=delay)
            elif (action == "Off"):
                self.TelnetConn.Send(Message='ZMOFF\r', Delay=delay)
        elif (Unit == 3):     # Main Volume control
            if (self.powerOn == False): self.TelnetConn.Send(Message='PWON\r')
            if (action == "On"):
                self.TelnetConn.Send(Message='MUOFF\r', Delay=delay)
            elif (action == "Set"):
                self.TelnetConn.Send(Message='MV'+str(Level)+'\r', Delay=delay)
            elif (action == "Off"):
                self.TelnetConn.Send(Message='MUON\r', Delay=delay)

        # Zone 2 devices
        elif (Unit == 4):   # Zone 2 selector
            if (action == "On"):
                if (self.powerOn == False): self.TelnetConn.Send(Message='PWON\r')
                self.TelnetConn.Send(Message='Z2ON\r', Delay=delay)
            elif (action == "Set"):
                if (self.powerOn == False): self.TelnetConn.Send(Message='PWON\r')
                if (self.zone2On == False):
                    self.TelnetConn.Send(Message='Z2ON\r', Delay=delay)
                    delay += 1
                self.TelnetConn.Send(Message='Z2'+self.selectorMap[Level]+'\r', Delay=delay)
                delay += 1
                self.TelnetConn.Send(Message='Z2?\r', Delay=delay)
            elif (action == "Off"):
                self.TelnetConn.Send(Message='Z2OFF\r', Delay=delay)
        elif (Unit == 5):   # Zone 2 Volume control
            if (self.powerOn == False): self.TelnetConn.Send(Message='PWON\r')
            if (self.zone2On == False):
                self.TelnetConn.Send(Message='Z2ON\r', Delay=delay)
                delay += 1
            if (action == "On"):
                self.TelnetConn.Send(Message='Z2MUOFF\r', Delay=delay)
            elif (action == "Set"):
                self.TelnetConn.Send(Message='Z2'+str(Level)+'\r', Delay=delay)
            elif (action == "Off"):
                self.TelnetConn.Send(Message='Z2MUON\r', Delay=delay)

        # Zone 3 devices
        elif (Unit == 6):   # Zone 3 selector
            if (action == "On"):
                if (self.powerOn == False): self.TelnetConn.Send(Message='PWON\r')
                self.TelnetConn.Send(Message='Z3ON\r', Delay=delay)
            elif (action == "Set"):
                if (self.powerOn == False): self.TelnetConn.Send(Message='PWON\r')
                if (self.zone3On == False):
                    self.TelnetConn.Send(Message='Z3ON\r', Delay=delay)
                    delay += 1
                self.TelnetConn.Send(Message='Z3'+self.selectorMap[Level]+'\r', Delay=delay)
                delay += 1
                self.TelnetConn.Send(Message='Z3?\r', Delay=delay)
            elif (action == "Off"):
                self.TelnetConn.Send(Message='Z3OFF\r', Delay=delay)
        elif (Unit == 7):   # Zone 3 Volume control
            if (self.powerOn == False): self.TelnetConn.Send(Message='PWON\r')
            if (self.zone3On == False):
                self.TelnetConn.Send(Message='Z3ON\r', Delay=delay)
                delay += 1
            if (action == "On"):
                self.TelnetConn.Send(Message='Z3MUOFF\r', Delay=delay)
            elif (action == "Set"):
                self.TelnetConn.Send(Message='Z3'+str(Level)+'\r', Delay=delay)
            elif (action == "Off"):
                self.TelnetConn.Send(Message='Z3MUON\r', Delay=delay)

        return

    def onDisconnect(self, Connection):
        Domoticz.Error("Disconnected from: "+Connection.Address+":"+Connection.Port)
        return

    def onHeartbeat(self):
        if (self.TelnetConn.Connected() == True):
            if (self.oustandingPings > 5):
                self.TelnetConn.Disconnect()
                self.nextConnect = 0
            else:
                self.TelnetConn.Send(self.pollingDict[self.lastMessage])
                Domoticz.Debug("onHeartbeat: self.lastMessage "+self.lastMessage+", Sending '"+self.pollingDict[self.lastMessage][0:2]+"'.")
                self.oustandingPings = self.oustandingPings + 1
        else:
            # if not connected try and reconnected every 3 heartbeats
            self.oustandingPings = 0
            self.nextConnect = self.nextConnect - 1
            if (self.nextConnect <= 0):
                self.nextConnect = 3
                self.TelnetConn.Connect()
                
        self.lastHeartbeat = datetime.datetime.now()
        return

    def SyncDevices(self):
        if (self.powerOn == False):
            UpdateDevice(1, 0, "Off")
            UpdateDevice(2, 0, "0")
            UpdateDevice(3, 0, str(abs(self.mainVolume1)))
            UpdateDevice(4, 0, "0")
            UpdateDevice(5, 0, str(abs(self.zone2Volume)))
            UpdateDevice(6, 0, "0")
            UpdateDevice(7, 0, str(abs(self.zone3Volume)))
        else:
            UpdateDevice(1, 1, "On")
            UpdateDevice(2, self.mainSource if self.mainOn else 0, str(self.mainSource if self.mainOn else 0))
            if (self.mainVolume1 <= 0 or self.mainOn == False): UpdateDevice(3, 0, str(abs(self.mainVolume1)))
            else: UpdateDevice(3, 2, str(self.mainVolume1))
            UpdateDevice(4, self.zone2Source if self.zone2On else 0, str(self.zone2Source if self.zone2On else 0))
            if (self.zone2Volume <= 0 or self.zone2On == False): UpdateDevice(5, 0, str(abs(self.zone2Volume)))
            else: UpdateDevice(5, 2, str(self.zone2Volume))
            UpdateDevice(6, self.zone3Source if self.zone3On else 0, str(self.zone3Source if self.zone3On else 0))
            if (self.zone3Volume <= 0 or self.zone3On == False): UpdateDevice(7, 0, str(abs(self.zone3Volume)))
            else: UpdateDevice(7, 2, str(self.zone3Volume))
        return
        
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

def onCommand(Unit, Command, Level, Hue):
    global _plugin
    _plugin.onCommand(Unit, Command, Level, Hue)

def onDisconnect(Connection):
    global _plugin
    _plugin.onDisconnect(Connection)

def onHeartbeat():
    global _plugin
    _plugin.onHeartbeat()

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
        Domoticz.Debug("Internal ID:     '" + str(Devices[x].ID) + "'")
        Domoticz.Debug("External ID:     '" + str(Devices[x].DeviceID) + "'")
        Domoticz.Debug("Device Name:     '" + Devices[x].Name + "'")
        Domoticz.Debug("Device nValue:    " + str(Devices[x].nValue))
        Domoticz.Debug("Device sValue:   '" + Devices[x].sValue + "'")
        Domoticz.Debug("Device LastLevel: " + str(Devices[x].LastLevel))
    return

def stringToBase64(s):
    return base64.b64encode(s.encode('utf-8')).decode("utf-8")

def base64ToString(b):
    return base64.b64decode(b).decode('utf-8')