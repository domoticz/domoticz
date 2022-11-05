# Basic Python Listener Example
#
# Author: Dnpwwo, 2017
#
# Example creates and listens on an HTTP socket. During heartbeats the code then
# creates client connections to the listening sockets
# and sends either GET or POST requests to it
#
#
"""
<plugin key="HTTPListen" name="Listener Sample Plugin" author="dnpwwo" version="1.0.0">
    <params>
        <param field="Port" label="Port" width="30px" required="true" default="8008"/>
        <param field="Mode6" label="Debug" width="100px">
            <options>
                <option label="True" value="Debug"/>
                <option label="False" value="Normal"  default="true" />
                <option label="Logging" value="File"/>
            </options>
        </param>
    </params>
</plugin>
"""
import Domoticz

class BasePlugin:
    enabled = False
    httpServerConn = None
    httpServerConns = {}
    httpClientConn = None
    heartbeats = 0

    def __init__(self):
        return

    def onStart(self):
        if Parameters["Mode6"] != "Normal":
            Domoticz.Debugging(1)
        DumpConfigToLog()
        self.httpServerConn = Domoticz.Connection(Name="Server Connection", Transport="TCP/IP", Protocol="HTTP", Port=Parameters["Port"])
        self.httpServerConn.Listen()
        Domoticz.Log("Leaving on start")

    def onConnect(self, Connection, Status, Description):
        if (Status == 0):
            Domoticz.Log("Connected successfully to: "+Connection.Address+":"+Connection.Port)
        else:
            Domoticz.Log("Failed to connect ("+str(Status)+") to: "+Connection.Address+":"+Connection.Port+" with error: "+Description)
        Domoticz.Log(str(Connection))
        if (Connection != self.httpClientConn):
            self.httpServerConns[Connection.Name] = Connection

    def onMessage(self, Connection, Data):
        Domoticz.Log("onMessage called for connection: "+Connection.Address+":"+Connection.Port)
        DumpHTTPResponseToLog(Data)
        
        # Incoming Requests
        if "Verb" in Data:
            strVerb = Data["Verb"]
            #strData = Data["Data"].decode("utf-8", "ignore")
            LogMessage(strVerb+" request received.")
            data = "<!doctype html><html><head></head><body><h1>Successful GET!!!</h1><body></html>"
            if (strVerb == "GET"):
                self.httpClientConn.Send({"Status":"200 OK", "Headers": {"Connection": "keep-alive", "Accept": "Content-Type: text/html; charset=UTF-8"}, "Data": data})
            elif (strVerb == "POST"):
                self.httpClientConn.Send({"Status":"200 OK", "Headers": {"Connection": "keep-alive", "Accept": "Content-Type: text/html; charset=UTF-8"}, "Data": data})
            else:
                Domoticz.Error("Unknown verb in request: "+strVerb)
        
        #Domoticz.Log("Headers ("+str(len(Extra))+"):")
        #for x in Extra:
        #    Domoticz.Log("    '" + x + "':'" + str(Extra[x]) + "'")
        #Connection.Send()

        #HTTP/1.1 200 OK
        #Date: Mon, 12 Jun 2017 02:28:46 GMT
        #Expires: -1
        #Cache-Control: private, max-age=0
        #Content-Type: text/html; charset=ISO-8859-1
        #Server: Domoticz
        #Accept-Ranges: none

        #<!doctype html><html itemscope="" itemtype="http://schema.org/WebPage" lang="en-AU"><head></head><body>Response OK</body>

    def onDisconnect(self, Connection):
        Domoticz.Log("onDisconnect called for connection '"+Connection.Name+"'.")
        Domoticz.Log("Server Connections:")
        for x in self.httpServerConns:
            Domoticz.Log("--> "+str(x)+"'.")
        if Connection.Name in self.httpServerConns:
            del self.httpServerConns[Connection.Name]

    def onHeartbeat(self):
        if (self.httpClientConn == None or self.httpClientConn.Connected() != True):
            self.httpClientConn = Domoticz.Connection(Name="Client Connection", Transport="TCP/IP", Protocol="HTTP", Address="127.0.0.1", Port=Parameters["Port"])
            self.httpClientConn.Connect()
            self.heartbeats = 0
        else:
            if (self.heartbeats == 1):
                self.httpClientConn.Send({"Verb":"GET", "URL":"/page.html", "Headers": {"Connection": "keep-alive", "Accept": "Content-Type: text/html; charset=UTF-8"}})
            elif (self.heartbeats == 2):
                postData = "param1=value&param2=other+value"
                self.httpClientConn.Send({'Verb':'POST', 'URL':'/MediaRenderer/AVTransport/Control', 'Data': postData})
            elif (self.heartbeats == 3) and (Parameters["Mode6"] != "File"):
                self.httpClientConn.Disconnect()
        self.heartbeats += 1

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

def onDisconnect(Connection):
    global _plugin
    _plugin.onDisconnect(Connection)

def onHeartbeat():
    global _plugin
    _plugin.onHeartbeat()

# Generic helper functions
def LogMessage(Message):
    if Parameters["Mode6"] != "Normal":
        Domoticz.Log(Message)
    elif Parameters["Mode6"] != "Debug":
        Domoticz.Debug(Message)
    else:
        f = open("http.html","w")
        f.write(Message)
        f.close()   

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

def DumpHTTPResponseToLog(httpDict):
    if isinstance(httpDict, dict):
        Domoticz.Log("HTTP Details ("+str(len(httpDict))+"):")
        for x in httpDict:
            if isinstance(httpDict[x], dict):
                Domoticz.Log("--->'"+x+" ("+str(len(httpDict[x]))+"):")
                for y in httpDict[x]:
                    Domoticz.Log("------->'" + y + "':'" + str(httpDict[x][y]) + "'")
            else:
                Domoticz.Log("--->'" + x + "':'" + str(httpDict[x]) + "'")
