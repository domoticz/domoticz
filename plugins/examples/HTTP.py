# Goolgle Home page example
#
# Author: Dnpwwo, 2017
#
#   Demonstrates HTTP connectivity.
#   After connection it performs a GET on  www.google.com and receives a 302 (Page Moved) response
#   It then does a subsequent GET on the Location specified in the 302 response and receives a 200 response.
#
"""
<plugin key="Google" name="Goolgle Home page example" author="Dnpwwo" version="1.0.0" externallink="https://www.google.com">
    <params>
        <param field="Address" label="IP Address" width="200px" required="true" default="www.google.com"/>
        <param field="Port" label="Port" width="30px" required="true" default="80"/>
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

class BasePlugin:
    httpConn = None
   
    def __init__(self):
        return

    def onStart(self):
        if Parameters["Mode6"] == "Debug":
            Domoticz.Debugging(1)
        DumpConfigToLog()
        self.httpConn = Domoticz.Connection(Name="HTTP Test", Transport="TCP/IP", Protocol="HTTP", Address=Parameters["Address"], Port=Parameters["Port"])
        self.httpConn.Connect()

    def onStop(self):
        Domoticz.Log("Plugin is stopping.")

    def onConnect(self, Connection, Status, Description):
        if (Status == 0):
            Domoticz.Debug("Google connected successfully.")
            data = ''
            headers = { 'Content-Type': 'text/xml; charset=utf-8', \
                        'Connection': 'keep-alive', \
                        'Accept': 'Content-Type: text/html; charset=UTF-8', \
                        'Host': Parameters["Address"]+":"+Parameters["Port"], \
                        'User-Agent':'Domoticz/1.0', \
                        'Content-Length' : "%d"%(len(data)) }
            Connection.Send(data, 'GET', '/', headers)
        else:
            Domoticz.Log("Failed to connect ("+str(Status)+") to: "+Parameters["Address"]+":"+Parameters["Port"]+" with error: "+Description)

    def onMessage(self, Connection, Data, Status, Extra):
        strData = Data.decode("utf-8", "ignore")
        Domoticz.Log("onMessage: Status="+str(Status))
        Domoticz.Log("Headers:")
        for x in Extra:
            Domoticz.Log("    '" + x + "':'" + str(Extra[x]))

        if (Status == 200):
            Domoticz.Log("Good Response received from Google.")
            self.httpConn.Disconnect()
        elif (Status == 302):
            Domoticz.Log("Google returned a Page Moved Error.")
            headers = { 'Content-Type': 'text/xml; charset=utf-8', \
                        'Connection': 'keep-alive', \
                        'Accept': 'Content-Type: text/html; charset=UTF-8', \
                        'Host': Parameters["Address"]+":"+Parameters["Port"], \
                        'User-Agent':'Domoticz/1.0', \
                        'Content-Length' : "0" }
            Connection.Send("", "GET", Extra["Location"], headers)
        elif (Status == 400):
            Domoticz.Error("Google returned a Bad Request Error.")
        elif (Status == 500):
            Domoticz.Error("Google returned a Server Error.")
        else:
            Domoticz.Error("Google returned a status: "+str(Status))

    def onCommand(self, Unit, Command, Level, Hue):
        Domoticz.Debug("onCommand called for Unit " + str(Unit) + ": Parameter '" + str(Command) + "', Level: " + str(Level))

    def onDisconnect(self, Connection):
        Domoticz.Log("onDisconnect called for connection to: "+Connection.Address+":"+Connection.Port)
        self.httpConn = None

    def onHeartbeat(self):
        Domoticz.Debug("onHeartbeat called")

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

def onMessage(Connection, Data, Status, Extra):
    global _plugin
    _plugin.onMessage(Connection, Data, Status, Extra)

def onCommand(Unit, Command, Level, Hue):
    global _plugin
    _plugin.onCommand(Unit, Command, Level, Hue)

def onNotification(Name, Subject, Text, Status, Priority, Sound, ImageFile):
    global _plugin
    _plugin.onNotification(Name, Subject, Text, Status, Priority, Sound, ImageFile)

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