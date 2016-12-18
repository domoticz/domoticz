#
#       Denon AVR 4306 Plugin
#
#       Author:     Dnpwwo, 2016
#
#   Plugin parameter definition below will be parsed during startup and copied into Manifest.xml, this will then drive the user interface in the Hardware web page
#
"""
<plugin key="Denon4306" version="1.0.0" name="Denon 4306 Amplifier" author="dnpwwo" wikilink="http://www.domoticz.com/wiki/plugins/Denon.html" externallink="http://www.denon.co.uk/uk">
    <params>
        <param field="Address" label="IP Address" width="200px" required="true" default="127.0.0.1"/>
        <param field="Port" label="Port" width="30px" required="true" default="23"/>
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

isConnected = False
isStarting = False
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

selectorMap = {0:'OFF',10:'DVD',20:'VDP',30:'TV',40:'CD',50:'DBS',60:'Tuner',70:'Phono',80:'VCR-1',90:'VCR-2',100:'V.Aux',110:'CDR/Tape',120:'AuxNet',130:'AuxIPod'}

def onStart():
    if Parameters["Mode6"] == "Debug":
        Domoticz.Debugging(1)
    if (len(Devices) == 0):
        Domoticz.Device(Name="Main Zone",   Unit=1, Type=244, Subtype=62, Switchtype=18, Image=5, Options="LevelActions:fHx8fHx8fHx8fHx8fA==;LevelNames:T2ZmfERWRHxWRFB8VFZ8Q0R8REJTfFR1bmVyfFBob25vfFZDUi0xfFZDUi0yfFYuQXV4fENEUi9UYXBlfEF1eE5ldHxBdXhJUG9k;LevelOffHidden:ZmFsc2U=;SelectorStyle:MQ==").Create()
        Domoticz.Device(Name="Main Volume", Unit=2, Type=244, Subtype=73, Switchtype=7,  Image=8).Create()
        Domoticz.Device(Name="Zone 2",      Unit=3, Type=244, Subtype=62, Switchtype=18, Image=5, Options="LevelActions:fHx8fHx8fHx8fHx8fA==;LevelNames:T2ZmfERWRHxWRFB8VFZ8Q0R8REJTfFR1bmVyfFBob25vfFZDUi0xfFZDUi0yfFYuQXV4fENEUi9UYXBlfEF1eE5ldHxBdXhJUG9k;LevelOffHidden:ZmFsc2U=;SelectorStyle:MQ==").Create()
        Domoticz.Device(Name="Volume 2",    Unit=4, Type=244, Subtype=73, Switchtype=7,  Image=8).Create()
        Domoticz.Device(Name="Zone 3",      Unit=5, Type=244, Subtype=62, Switchtype=18, Image=5, Options="LevelActions:fHx8fHx8fHx8fHx8fA==;LevelNames:T2ZmfERWRHxWRFB8VFZ8Q0R8REJTfFR1bmVyfFBob25vfFZDUi0xfFZDUi0yfFYuQXV4fENEUi9UYXBlfEF1eE5ldHxBdXhJUG9k;LevelOffHidden:ZmFsc2U=;SelectorStyle:MQ==").Create()
        Domoticz.Device(Name="Volume 3",    Unit=6, Type=244, Subtype=73, Switchtype=7,  Image=8).Create()
        Domoticz.Log("Devices created.")
    DumpConfigToLog()
    Domoticz.Transport("TCP/IP", Parameters["Address"], Parameters["Port"])
    Domoticz.Protocol("Line")
    Domoticz.Connect()
    return

def onConnect(Status, Description):
    global isConnected
    if (Status == 0):
        isConnected = True
        Domoticz.Log("Connected successfully to: "+Parameters["Address"]+":"+Parameters["Port"])
        Domoticz.Send('PW?\r')
    else:
        isConnected = False
        Domoticz.Log("Failed to connect ("+str(Status)+") to: "+Parameters["Address"]+":"+Parameters["Port"])
        Domoticz.Debug("Failed to connect ("+str(Status)+") to: "+Parameters["Address"]+":"+Parameters["Port"]+" with error: "+Description)
        # Turn devices off in Domoticz
        for Key in Devices:
            UpdateDevice(Key, 0, Devices[Key].sValue)
    return True

def onMessage(Data):
    global oustandingPings, isStarting
    global selectorMap, powerOn, mainSource, mainVolume1, zone2Source, zone2Volume, zone3Source, zone3Volume

    oustandingPings = oustandingPings - 1
    Domoticz.Debug("onMessage ("+str(isStarting)+") called with Data: '"+str(Data)+"'")
    
    Data = Data.strip()
    action = Data[0:2]
    detail = Data[2:]

    if (action == "PW"):        # Power Status
        if (detail == "STANDBY"):
            powerOn = False
        elif (detail == "ON"):
            if (powerOn == False):
                Domoticz.Send('ZM?\r')
                isStarting = True
            powerOn = True
        else: Domoticz.Debug("Unknown: Action "+action+", Detail '"+detail+"' ignored.")
    elif (action == "ZM"):      # Main Zone on/off
        if (detail == "ON"):
            Domoticz.Send('SI?\r')
            mainVolume1 = abs(mainVolume1)
        elif (detail == "OFF"):
            mainSource = 0
            mainVolume1 = abs(mainVolume1)*-1
            if (isStarting == True): Domoticz.Send('MU?\r')
        else: Domoticz.Debug("Unknown: Action "+action+", Detail '"+detail+"' ignored.")
    elif (action == "SI"):      # Main Zone Source Input
        for key, value in selectorMap.items():
            if (detail == value):      mainSource = key
        if (isStarting == True): Domoticz.Send('MV?\r')
    elif (action == "MV"):      # Master Volume
        if (detail.isdigit()):
            mainVolume1 = int(detail)
            Domoticz.Send('MU?\r')
        elif (detail[0:3] == "MAX"): Domoticz.Debug("Unknown: Action "+action+", Detail '"+detail+"' ignored.")
        else: Domoticz.Log("Unknown: Action "+action+", Detail '"+detail+"' ignored.")
    elif (action == "MU"):      # Overall Mute
        if (detail == "ON"):         mainVolume1 = abs(mainVolume1)*-1
        elif (detail == "OFF"):      mainVolume1 = abs(mainVolume1)
        else: Domoticz.Debug("Unknown: Action "+action+", Detail '"+detail+"' ignored.")
        if (isStarting == True): Domoticz.Send('Z2?\r')
    elif (action == "Z2"):      # Zone 2
        if (detail.isdigit()):
            zone2Volume = int(detail)
        else:
            for key, value in selectorMap.items():
                if (detail == value):      zone2Source = key
            if (zone2Source == 0):  zone2Volume = abs(zone2Volume)*-1
            else:                   zone2Volume = abs(zone2Volume)*-1
        if (isStarting == True): Domoticz.Send('Z3?\r')
    elif (action == "Z3"):      # Zone 3
        isStarting = False
        if (detail.isdigit()):
            zone3Volume = int(detail)
        else:
            for key, value in selectorMap.items():
                if (detail == value):      zone3Source = key
            if (zone3Source == 0):  zone3Volume = abs(zone3Volume)*-1
            else:                   zone3Volume = abs(zone3Volume)*-1
    elif (action == "SS"):
        Domoticz.Debug("Message '"+action+"' ignored.")
    else:
        Domoticz.Error("Unknown message '"+action+"' ignored.")
    SyncDevices()
    return

def onCommand(Unit, Command, Level, Hue):
    global selectorMap, powerOn, mainSource, mainVolume1, zone2Source, zone2Volume, zone3Source, zone3Volume

    Domoticz.Log("onCommand called for Unit " + str(Unit) + ": Parameter '" + str(Command) + "', Level: " + str(Level))

    Command = Command.strip()
    action, sep, params = Command.partition(' ')
    if (powerOn == False):
        Domoticz.Send('PWON\r')  # Any commands sent within 4 seconds of this will potentially be ignored
    else:
        if (action == "On"):
            if (Unit == 1):     Domoticz.Send('ZMON\r')
            elif (Unit == 2):   Domoticz.Send('MUOFF\r')
            elif (Unit == 3):   Domoticz.Send('Z2ON\r')
            elif (Unit == 4):   Domoticz.Send('Z2MUOFF\r')
            elif (Unit == 5):   Domoticz.Send('Z3ON\r')
            elif (Unit == 6):   Domoticz.Send('Z3MUOFF\r')
            else: Domoticz.Error( "Unknown Unit number in command "+str(Unit)+".")
        elif (action == "Set"):
                if (params.capitalize() == 'Level') or (Command.lower() == 'Volume'):
                    if (Unit == 1):     # Main selector
                        Domoticz.Send('SI'+selectorMap[Level]+'\r')
                    elif (Unit == 2):   # Volume control
                        Domoticz.Send('MV'+str(Level)+'\r')
                    elif (Unit == 3):   # Zone 2 selector
                        Domoticz.Send('Z2'+selectorMap[Level]+'\r')
                    elif (Unit == 4):   # Zone 2 Volume control
                        Domoticz.Send('Z2'+str(Level)+'\r')
                    elif (Unit == 5):   # Zone 3 selector
                        Domoticz.Send('Z3'+selectorMap[Level]+'\r')
                    elif (Unit == 6):   # Zone 3 Volume control
                        Domoticz.Send('Z3'+str(Level)+'\r')
                    SyncDevices()
        elif (action == "Off"):
            if (Unit == 1):     Domoticz.Send('ZMOFF\r')
            elif (Unit == 2):   Domoticz.Send('MUON\r')
            elif (Unit == 3):   Domoticz.Send('Z2OFF\r')
            elif (Unit == 4):   Domoticz.Send('Z2MUON\r')
            elif (Unit == 5):   Domoticz.Send('Z3OFF\r')
            elif (Unit == 6):   Domoticz.Send('Z3MUON\r')
            else: Domoticz.Error( "Unknown Unit number in command "+str(Unit)+".")
        else:
            Domoticz.Error("Unhandled action '"+action+"' ignored, options are On/Set/Off")
    return

def onDisconnect():
    global isConnected, powerOn

    isConnected = False
    powerOn = False
    Domoticz.Log("Device has disconnected.")
    SyncDevices()
    return

def onStop():
    Domoticz.Log("onStop called")
    return 8

def onHeartbeat():
    global isConnected, nextConnect, oustandingPings
    if (isConnected == True):
        if (oustandingPings > 5):
            Domoticz.Disconnect()
            nextConnect = 0
        else:
            Domoticz.Send('PW?\r')
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
        UpdateDevice(1, 0, "0")
        UpdateDevice(2, 0, str(mainVolume1))
        UpdateDevice(3, 0, "0")
        UpdateDevice(4, 0, str(zone2Volume))
        UpdateDevice(5, 0, "0")
        UpdateDevice(6, 0, str(zone3Volume))
    else:
        UpdateDevice(1, mainSource, str(mainSource))
        if (mainVolume1 <= 0): UpdateDevice(2, 0, str(abs(mainVolume1)))
        else: UpdateDevice(2, 2, str(mainVolume1))
        UpdateDevice(3, zone2Source, str(zone2Source))
        if (zone2Volume <= 0): UpdateDevice(4, 0, str(abs(zone2Volume)))
        else: UpdateDevice(4, 2, str(zone2Volume))
        UpdateDevice(5, zone3Source, str(zone3Source))
        if (zone3Volume <= 0): UpdateDevice(6, 0, str(abs(zone3Volume)))
        else: UpdateDevice(6, 2, str(zone3Volume))
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
