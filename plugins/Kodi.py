#       Kodi Plugin
#
#           Author:     Dnpwwo, 2016
#
#           Parameters:
#			<param field="Address" label="IP Address" width="200px" required="true" default="127.0.0.1"/>
#			<param field="Port" label="Port" width="30px" required="true" default="9090"/>
#			<param field="Mode1" label="MAC Address" width="150px" required="false"/>
#			<param field="Mode2" label="Shutdown Command" width="100px">
#			<param field="Mode6" label="Debug" width="75px">
#
#           Devices:
#			<device name="Status"  unit="1" type="17" subtype="0" switchtype="17" icon=""/>
#			<device name="Source" unit="2" type="244" subtype="62" switchtype="18" icon="12" options="LevelActions:fHx8fA==;LevelNames:T2ZmfFZpZGVvfE11c2ljfFRWIFNob3dzfExpdmUgVFY=;LevelOffHidden:ZmFsc2U=;SelectorStyle:MA=="/>/>
#			<device name="Volume" unit="3" type="244" subtype="73" switchtype="7" icon="8"/>
#
import Domoticz

def onStart():
    if Parameters["Mode6"] == "Debug":
        Domoticz.Debug(1)
        for x in Parameters:
            if Parameters[x] != "":
                Domoticz.Log(0, "'" + x + "':'" + str(Parameters[x]) + "'")
        Domoticz.Log(0, "Device count: " + str(len(Devices)))
        for x in Devices:
            Domoticz.Log(0, "Device:           " + str(x) + " - " + str(Devices[x]))
            Domoticz.Log(0, "Device ID:       '" + str(Devices[x].ID) + "'")
            Domoticz.Log(0, "Device Name:     '" + Devices[x].Name + "'")
            Domoticz.Log(0, "Device nValue:    " + str(Devices[x].nValue))
            Domoticz.Log(0, "Device sValue:   '" + Devices[x].sValue + "'")
            Domoticz.Log(0, "Device LastLevel: " + str(Devices[x].LastLevel))
    Domoticz.Transport("TCP/IP")
    Domoticz.Protocol("JSON")
    Domoticz.Heartbeat(1)
    return 1

def onHeartbeat():
    return 1

def onConnect():
    Domoticz.Log(0, "onConnect called")
    return 3

def onTimeout():
    Domoticz.Log(0, "onTimeout called")
    return 4

def onMessage():
    Domoticz.Log(0, "onMessage called")
    return 5

def onCommand(Unit, Command, Level, Hue):
    Domoticz.Log(0, "onCommand called for Unit " + str(Unit) + ": Parameter '" + str(Command) + "', Level: " + str(Level))
    if (Command == 'On'):
        Devices[1].Update(1, Devices[1].sValue)                # Status On no message
        Devices[2].Update(1, Devices[2].sValue)                # Input On to default level
        Devices[3].Update(1, Devices[3].sValue)                # Volume On to default level
    if (Command == 'Set Level'):
        if (Level > 0):
            if (Devices[1].nValue == 0):
                Devices[1].Update(1, Devices[1].sValue)        # Status On no message
                Devices[2].Update(1, Devices[2].sValue)        # Input On to default level
                Devices[3].Update(1, Devices[3].sValue)        # Volume On to default level
            Devices[Unit].Update(1, str(Level))
        else:
            Devices[Unit].Update(0, str(Level))
    if (Command == 'Off'):
        Devices[1].Update(0, Devices[1].sValue)                # Status Off no message
        Devices[2].Update(0, Devices[2].sValue)                # Input Off and back to previous level
        Devices[3].Update(0, Devices[3].sValue)                # Volume Off and back to previous level
    return 6

def onDisconnect():
    Domoticz.Log(0, "onDisconnect called")
    return 7

def onStop():
    Domoticz.Log(0, "onStop called")
    return 8
