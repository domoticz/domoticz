# Samsung TV
#
# Author: Dnpwwo
#
# Interesting URLs:
# http://192.168.0.17:52235/dmr/SamsungMRDesc.xml
# http://192.168.0.17:52235/pmr/PersonalMessageReceiver.xml
# http://192.168.0.17:52235/pmr/MessageBoxService.xml
#
"""
<plugin key="Samsung" name="Samsung TV" author="dnpwwo" version="1.0.0" wikilink="http://www.domoticz.com/wiki/plugins/SamsungTV.html" externallink="https://www.google.com/">
    <params>
        <param field="Address" label="IP Address" width="200px" required="true" default="127.0.0.1"/>
        <param field="Port" label="Port" width="60px" required="true" default="52235"/>
        <param field="Mode5" label="Notifications" width="75px">
            <options>
                <option label="True" value="True"/>
                <option label="False" value="False"  default="true" />
            </options>
        </param>
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
import sys
import subprocess

class BasePlugin:
    isConnected = False
    isFound = False

    def onStart(self):
        if Parameters["Mode6"] == "Debug":
            Domoticz.Debugging(1)
        if (len(Devices) == 0):
            Domoticz.Device(Name="Status",  Unit=1, Type=17, Image=2).Create()
        #    Options = "LevelActions:"+stringToBase64("||||")+";LevelNames:"+stringToBase64("Off|Video|Music|TV Shows|Live TV")+";LevelOffHidden:ZmFsc2U=;SelectorStyle:MA=="
        #    Domoticz.Device(Name="Source",  Unit=2, TypeName="Selector Switch", Switchtype=18, Image=12, Options=Options).Create()
        #    Domoticz.Device(Name="Volume",  Unit=3, Type=244, Subtype=73, \
        #                    Switchtype=7,  Image=8).Create()
        #    Domoticz.Device(Name="Playing", Unit=4, Type=244, Subtype=73, Switchtype=7,  Image=12).Create()
        #    Domoticz.Log("Devices created.")
        else:
            if (1 in Devices): self.isFound = (Devices[1].nValue == 1)
        #    if (2 in Devices): self.mediaLevel = Devices[2].nValue
        DumpConfigToLog()
        Domoticz.Transport(Transport="TCP/IP", Address=Parameters["Address"], Port=Parameters["Port"])
        Domoticz.Log("Started...")

    def onConnect(self, Status, Description):
        if (Status == 0):
            self.isConnected = True
            Domoticz.Log("Connected successfully to: "+Parameters["Address"]+":"+Parameters["Port"])
            headers = { 'Content-Type': 'text/xml; charset=utf-8', \
                        'Connection': 'keep-alive', \
                        'Accept': 'Content-Type: text/html; charset=UTF-8', \
                        'Host': Parameters["Address"]+":"+Parameters["Port"], \
                        'User-Agent':'Domoticz/1.0', \
                        'Content-Length' : "0" }
            Domoticz.Send("", 'GET', '/dmr/SamsungMRDesc.xml', headers)
        else:
            self.isConnected = False
            Domoticz.Log("Failed to connect ("+str(Status)+") to: "+Parameters["Address"]+":"+Parameters["Port"])
            Domoticz.Debug("Failed to connect ("+str(Status)+") to: "+Parameters["Address"]+":"+Parameters["Port"]+" with error: "+Description)
            # Turn devices off in Domoticz
            for Key in Devices:
                UpdateDevice(Key, 0, Devices[Key].sValue)

    def onMessage(self, Data, Status, Extra):
        Domoticz.Log("onMessage called")

    def onHeartbeat(self):
        Domoticz.Debug("onHeartbeat called")
        if (self.isConnected != True):
            # TV actively disconnects afte 10 seconds so just ping to see if its still on.
            if (sys.platform == 'win32'):
                currentstate = subprocess.call('ping -n 1 -w 2000 '+ Parameters["Address"] + ' > nul 2>&1', shell=True)
            else:
                currentstate = subprocess.call('ping -q -c1 -W 2 '+ Parameters["Address"] + ' > /dev/null', shell=True)
            
            Domoticz.Debug("Pinging "+Parameters["Address"]+", returned "+str(currentstate)+".")
            if (currentstate == 0) and (self.isFound == False):
                self.isFound = True
                if (1 in Devices): UpdateDevice(1, 1, Devices[1].sValue)
                Domoticz.Log("Ping request answered, Television is on.")
            if (currentstate != 0) and (self.isFound == True):
                self.isFound = False
                for Key in Devices:
                    UpdateDevice(Key, 0, Devices[Key].sValue)
                Domoticz.Log("Ping request ignored, Television is off.")

    def onCommand(self, Unit, Command, Level, Hue):
        Domoticz.Log("onCommand called for Unit " + str(Unit) + ": Parameter '" + str(Command) + "', Level: " + str(Level))

    def onNotification(self, Name, Subject, Text, Status, Priority, Sound, ImageFile):
        Domoticz.Log("Notification: " + Name + "," + Subject + "," + Text + "," + Status + "," + str(Priority) + "," + Sound + "," + ImageFile)

    def onDisconnect(self):
        self.isConnected = False
        Domoticz.Log("Television has disconnected")

    def onStop(self):
        Domoticz.Log("Stopped...")

global _plugin
_plugin = BasePlugin()

def onStart():
    global _plugin
    _plugin.onStart()

def onStop():
    global _plugin
    _plugin.onStop()

def onConnect(Status, Description):
    global _plugin
    _plugin.onConnect(Status, Description)

def onMessage(Data, Status, Extra):
    global _plugin
    _plugin.onMessage(Data, Status, Extra)

def onCommand(Unit, Command, Level, Hue):
    global _plugin
    _plugin.onCommand(Unit, Command, Level, Hue)

def onNotification(Name, Subject, Text, Status, Priority, Sound, ImageFile):
    global _plugin
    _plugin.onNotification(Name, Subject, Text, Status, Priority, Sound, ImageFile)

def onDisconnect():
    global _plugin
    _plugin.onDisconnect()

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

import base64
def stringToBase64(s):
    return base64.b64encode(s.encode('utf-8')).decode("utf-8")

def base64ToString(b):
    return base64.b64decode(b).decode('utf-8')