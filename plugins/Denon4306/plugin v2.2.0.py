#
#       Denon AVR 4306 Plugin
#
#       Author:     Dnpwwo, 2016
#
#   Mode3 ("Sources") needs to have '|' delimited names of sources that the Denon knows about.  The Selector can be changed afterwards to any  text and the plugin will still map to the actual Denon name.
#
"""
<plugin key="Denon4306" version="2.2.0" name="Denon AVR 4306 Amplifier" author="dnpwwo" wikilink="" externallink="http://www.denon.co.uk/uk">
    <params>
        <param field="Address" label="IP Address" width="200px" required="true" default="127.0.0.1"/>
        <param field="Port" label="Port" width="30px" required="true" default="23"/>
        <param field="Mode1" label="Zone Count" width="50px" required="true">
            <options>
                <option label="1" value="1"/>
                <option label="2" value="2"/>
                <option label="3" value="3"  default="true" />
            </options>
        </param>
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

isConnected = False
nextConnect = 3
oustandingPings = 0

# Device Status - When off set device values to negative
powerOn = False
mainSource = 0
mainVolume1 = 0
zone2Source = 0
zone2Volume = 0
zone3Source = 0
zone3Volume = 0

ignoreMessages = "|SS|SV|SD|MS|PS|CV|"
selectorMap = {}
pollingDict = {"PW":"ZM?\r", "ZM":"SI?\r", "SI":"MV?\r", "MV":"MU?\r", "MU":"Z2?\r", "Z2":"Z3?\r", "Z3":"PW?\r" }
lastMessage = ""

def onStart():
    global mainSource, mainVolume1, zone2Source, zone2Volume, zone3Source, zone3Volume

    if Parameters["Mode6"] == "Debug":
        Domoticz.Debugging(1)
    LevelActions = '|'*Parameters["Mode3"].count('|')
    # if Zone 3 required make sure at least themain device exists, otherwise suppress polling for it
    if (Parameters["Mode1"] > "2"):
        if (6 not in Devices):
            Domoticz.Device(Name="Zone 3", Unit=6, TypeName="Selector Switch", Switchtype=18, Image=5, \
                            Options="LevelActions:"+stringToBase64(LevelActions)+";LevelNames:"+stringToBase64(Parameters["Mode3"])+";LevelOffHidden:ZmFsc2U=;SelectorStyle:MQ==").Create()
            if (7 not in Devices): Domoticz.Device(Name="Volume 3", Unit=7, Type=244, Subtype=73, Switchtype=7,  Image=8).Create()
            Domoticz.Log("Created Zone 3 device(s) because zone requested in hardware setup but device not found.")
        else:
            zone3Source = Devices[6].nValue
            if (7 in Devices): zone3Volume = int(Devices[7].sValue)
    else:
        pollingDict.pop("Z3")
        pollingDict["Z2"] = "PW?\r"
    
    # if Zone 2 required make sure at least themain device exists, otherwise suppress polling for it
    if (Parameters["Mode1"] > "1"):
        if (4 not in Devices):
            Domoticz.Device(Name="Zone 2", Unit=4, TypeName="Selector Switch", Switchtype=18, Image=5, \
                            Options="LevelActions:"+stringToBase64(LevelActions)+";LevelNames:"+stringToBase64(Parameters["Mode3"])+";LevelOffHidden:ZmFsc2U=;SelectorStyle:MQ==").Create()
            if (5 not in Devices): Domoticz.Device(Name="Volume 2", Unit=5, Type=244, Subtype=73, Switchtype=7,  Image=8).Create()
            Domoticz.Log("Created Zone 2 device(s) because zone requested in hardware setup but device not found.")
        else:
            zone2Source = Devices[4].nValue
            if (5 in Devices): zone2Volume = int(Devices[5].sValue)
    else:
        pollingDict.pop("Z2")
        pollingDict["MU"] = "PW?\r"
        
    if (1 not in Devices):
        Domoticz.Device(Name="Power", Unit=1, TypeName="Switch",  Image=5).Create()
        if (2 not in Devices): Domoticz.Device(Name="Main Zone", Unit=2, TypeName="Selector Switch", Switchtype=18, Image=5, \
                        Options="LevelActions:"+stringToBase64(LevelActions)+";LevelNames:"+stringToBase64(Parameters["Mode3"])+";LevelOffHidden:ZmFsc2U=;SelectorStyle:MQ==").Create()
        if (3 not in Devices): Domoticz.Device(Name="Main Volume", Unit=3, Type=244, Subtype=73, Switchtype=7,  Image=8).Create()
        Domoticz.Log("Created Main Zone devices because they were not found.")
    else:
        mainSource = Devices[2].nValue
        if (3 in Devices): mainVolume1 = int(Devices[3].sValue)
        
    DumpConfigToLog()
    dictValue=0
    for item in Parameters["Mode3"].split('|'):
        selectorMap[dictValue] = item
        dictValue = dictValue + 10
    Domoticz.Transport("TCP/IP", Parameters["Address"], Parameters["Port"])
    Domoticz.Protocol("Line")
    Domoticz.Connect()
    return

def onConnect(Status, Description):
    global isConnected, powerOn
    if (Status == 0):
        isConnected = True
        Domoticz.Log("Connected successfully to: "+Parameters["Address"]+":"+Parameters["Port"])
        Domoticz.Send('PW?\r')
    else:
        isConnected = False
        powerOn = False
        Domoticz.Log("Failed to connect ("+str(Status)+") to: "+Parameters["Address"]+":"+Parameters["Port"])
        Domoticz.Debug("Failed to connect ("+str(Status)+") to: "+Parameters["Address"]+":"+Parameters["Port"]+" with error: "+Description)
        SyncDevices()
    return

def onMessage(Data, Status, Extra):
    global oustandingPings, lastMessage, selectorMap, powerOn, pollingDict, ignoreMessages
    global mainSource, mainVolume1, zone2Source, zone2Volume, zone3Source, zone3Volume

    oustandingPings = oustandingPings - 1
    strData = Data.decode("utf-8", "ignore")
    Domoticz.Debug("onMessage called with Data: '"+str(strData)+"'")
    
    strData = strData.strip()
    action = strData[0:2]
    detail = strData[2:]
    if (action in pollingDict): lastMessage = action

    if (action == "PW"):        # Power Status
        if (detail == "STANDBY"):
            powerOn = False
        elif (detail == "ON"):
            powerOn = True
        else: Domoticz.Debug("Unknown: Action "+action+", Detail '"+detail+"' ignored.")
    elif (action == "ZM"):      # Main Zone on/off
        if (detail == "ON"):
            mainVolume1 = abs(mainVolume1)
        elif (detail == "OFF"):
            mainSource = 0
            mainVolume1 = abs(mainVolume1)*-1
        else: Domoticz.Debug("Unknown: Action "+action+", Detail '"+detail+"' ignored.")
    elif (action == "SI"):      # Main Zone Source Input
        for key, value in selectorMap.items():
            if (detail == value):      mainSource = key
    elif (action == "MV"):      # Master Volume
        if (detail.isdigit()):
            mainVolume1 = int(detail[0:2])
        elif (detail[0:3] == "MAX"): Domoticz.Debug("Unknown: Action "+action+", Detail '"+detail+"' ignored.")
        else: Domoticz.Log("Unknown: Action "+action+", Detail '"+detail+"' ignored.")
    elif (action == "MU"):      # Overall Mute
        if (detail == "ON"):         mainVolume1 = abs(mainVolume1)*-1
        elif (detail == "OFF"):      mainVolume1 = abs(mainVolume1)
        else: Domoticz.Debug("Unknown: Action "+action+", Detail '"+detail+"' ignored.")
    elif (action == "Z2"):      # Zone 2
        if (detail.isdigit()):
            zone2Volume = int(detail[0:2])
        else:
            for key, value in selectorMap.items():
                if (detail == value):      zone2Source = key
            if (zone2Source == 0):  zone2Volume = abs(zone2Volume)*-1
            else:                   zone2Volume = abs(zone2Volume)*-1
    elif (action == "Z3"):      # Zone 3
        if (detail.isdigit()):
            zone3Volume = int(detail[0:2])
        else:
            for key, value in selectorMap.items():
                if (detail == value):      zone3Source = key
            if (zone3Source == 0):  zone3Volume = abs(zone3Volume)*-1
            else:                   zone3Volume = abs(zone3Volume)*-1
    else:
        if (ignoreMessages.find(action) < 0):
            Domoticz.Error("Unknown message '"+action+"' ignored.")
    SyncDevices()
    return

def onCommand(Unit, Command, Level, Hue):
    global selectorMap, powerOn

    Domoticz.Log("onCommand called for Unit " + str(Unit) + ": Parameter '" + str(Command) + "', Level: " + str(Level))

    Command = Command.strip()
    action, sep, params = Command.partition(' ')
    delay = 0
    if (powerOn == False):
        Domoticz.Send('PWON\r')  # Any commands sent within 4 seconds of this will potentially be ignored
        delay = int(Parameters["Mode2"])
    if (action == "On"):
        if (Unit == 1):     Domoticz.Send(Message='ZMON\r', Delay=delay)
        elif (Unit == 2):   Domoticz.Send(Message='ZMON\r', Delay=delay)
        elif (Unit == 3):   Domoticz.Send(Message='MUOFF\r', Delay=delay)
        elif (Unit == 4):   Domoticz.Send(Message='Z2ON\r', Delay=delay)
        elif (Unit == 5):   Domoticz.Send(Message='Z2MUOFF\r', Delay=delay)
        elif (Unit == 6):   Domoticz.Send(Message='Z3ON\r', Delay=delay)
        elif (Unit == 7):   Domoticz.Send(Message='Z3MUOFF\r', Delay=delay)
        else: Domoticz.Error( "Unknown Unit number in command "+str(Unit)+".")
    elif (action == "Set"):
            if (params.capitalize() == 'Level') or (Command.lower() == 'Volume'):
                if (Unit == 2):     # Main selector
                    Domoticz.Send(Message='SI'+selectorMap[Level]+'\r', Delay=delay)
                elif (Unit == 3):   # Volume control
                    Domoticz.Send(Message='MV'+str(Level)+'\r', Delay=delay)
                elif (Unit == 4):   # Zone 2 selector
                    Domoticz.Send(Message='Z2'+selectorMap[Level]+'\r', Delay=delay)
                elif (Unit == 5):   # Zone 2 Volume control
                    Domoticz.Send(Message='Z2'+str(Level)+'\r', Delay=delay)
                elif (Unit == 6):   # Zone 3 selector
                    Domoticz.Send(Message='Z3'+selectorMap[Level]+'\r', Delay=delay)
                elif (Unit == 7):   # Zone 3 Volume control
                    Domoticz.Send(Message='Z3'+str(Level)+'\r', Delay=delay)
                SyncDevices()
    elif (action == "Off"):
        if (Unit == 1):     Domoticz.Send(Message='ZMOFF\r', Delay=delay)
        elif (Unit == 2):   Domoticz.Send(Message='ZMOFF\r', Delay=delay)
        elif (Unit == 3):   Domoticz.Send(Message='MUON\r', Delay=delay)
        elif (Unit == 4):   Domoticz.Send(Message='Z2OFF\r', Delay=delay)
        elif (Unit == 5):   Domoticz.Send(Message='Z2MUON\r', Delay=delay)
        elif (Unit == 6):   Domoticz.Send(Message='Z3OFF\r', Delay=delay)
        elif (Unit == 7):   Domoticz.Send(Message='Z3MUON\r', Delay=delay)
        else: Domoticz.Error( "Unknown Unit number in command "+str(Unit)+".")
    else:
        Domoticz.Error("Unhandled action '"+action+"' ignored, options are On/Set/Off")
    return

def onDisconnect():
    global isConnected, powerOn
    isConnected = False
    powerOn = False
    Domoticz.Log("Denon device has disconnected.")
    SyncDevices()
    return

def onHeartbeat():
    global isConnected, nextConnect, oustandingPings, lastMessage, pollingDict
    if (isConnected == True):
        if (oustandingPings > 5):
            Domoticz.Disconnect()
            nextConnect = 0
        else:
            Domoticz.Send(pollingDict[lastMessage])
            Domoticz.Debug("onHeartbeat: lastMessage "+lastMessage+", Sending '"+pollingDict[lastMessage][0:2]+"'.")
            oustandingPings = oustandingPings + 1
    else:
        # if not connected try and reconnected every 3 heartbeats
        oustandingPings = 0
        nextConnect = nextConnect - 1
        if (nextConnect <= 0):
            nextConnect = 3
            Domoticz.Connect()
    return

def SyncDevices():
    global powerOn, mainSource, mainVolume1, zone2Source, zone2Volume, zone3Source, zone3Volume
    
    if (powerOn == False):
        UpdateDevice(1, 0, "Off")
        UpdateDevice(2, 0, "0")
        UpdateDevice(3, 0, str(abs(mainVolume1)))
        UpdateDevice(4, 0, "0")
        UpdateDevice(5, 0, str(abs(zone2Volume)))
        UpdateDevice(6, 0, "0")
        UpdateDevice(7, 0, str(abs(zone3Volume)))
    else:
        UpdateDevice(1, 1, "On")
        UpdateDevice(2, mainSource, str(mainSource))
        if (mainVolume1 <= 0): UpdateDevice(3, 0, str(abs(mainVolume1)))
        else: UpdateDevice(3, 2, str(mainVolume1))
        UpdateDevice(4, zone2Source, str(zone2Source))
        if (zone2Volume <= 0): UpdateDevice(5, 0, str(abs(zone2Volume)))
        else: UpdateDevice(5, 2, str(zone2Volume))
        UpdateDevice(6, zone3Source, str(zone3Source))
        if (zone3Volume <= 0): UpdateDevice(7, 0, str(abs(zone3Volume)))
        else: UpdateDevice(7, 2, str(zone3Volume))
    return

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

def stringToBase64(s):
    return base64.b64encode(s.encode('utf-8')).decode("utf-8")

def base64ToString(b):
    return base64.b64decode(b).decode('utf-8')