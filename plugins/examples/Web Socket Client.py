"""
<plugin key="WebSocketTest" name="Web Socket Test Example" author="dnpwwo" version="0.1">
    <description>
        <h2>Web Socket Test</h2><br/>
    </description>
    <params>
        <param field="Address" label="IP Address" width="200px" required="true" default="socketsbay.com"/>
        <param field="Port" label="Port" width="30px" required="true" default="443"/>
        <param field="Mode1" label="URL" width="300px" required="true" default="/wss/v2/2/demo/"/>
        <param field="Mode6" label="Debug" width="150px">
            <options>
                <option label="None" value="0"  default="true" />
                <option label="Python Only" value="2"/>
                <option label="Basic Debugging" value="62"/>
                <option label="Basic+Messages" value="126"/>
                <option label="Queue" value="128"/>
                <option label="Connections Only" value="16"/>
                <option label="Connections+Queue" value="144"/>
                <option label="All" value="-1"/>
            </options>
        </param>
    </params>
</plugin>
"""

import DomoticzEx as Domoticz
import json
import secrets
import base64


class BasePlugin:
    websocketConn = None
    reconAgain = 3
    debug = False

    def onStart(self):
        if int(Parameters["Mode6"]) > 0:
            Domoticz.Debugging(int(Parameters["Mode6"]))
            self.debug = True

        self.websocketConn = Domoticz.Connection(Name="websocketConn", Transport="TCP/IP", Protocol="WSS",
                                               Address=Parameters["Address"], Port=Parameters["Port"])
        self.websocketConn.Connect()

    def onConnect(self, Connection, Status, Description):
        if Status == 0:
            Domoticz.Log("Connected successfully to: " + Connection.Address + ":" + Connection.Port)
            # End point is now connected so request the upgrade to Web Sockets
            send_data = {'URL': Parameters["Mode1"],
                         'Headers': {'Host': Parameters["Address"],
                                     'Origin': 'https://' + Parameters["Address"],
                                     'Sec-WebSocket-Key': base64.b64encode(secrets.token_bytes(16)).decode("utf-8")
                                     }
                        }
            Connection.Send(send_data)
        else:
            Domoticz.Log("Failed to connect (" + str(Status) + ") to: " + Connection.Address + ":" + Connection.Port)
            Domoticz.Debug("Failed to connect (" + str(
                Status) + ") to: " + Connection.Address + ":" + Connection.Port + " with error: " + Description)
        return True

    def onMessage(self, Connection, Data):
        Domoticz.Debug("onMessage called")
        if "Status" in Data:  # HTTP Message
            if Data["Status"] == "101":
                Domoticz.Log("WebSocket successfully upgraded")
            else:
                DumpWSResponseToLog(Data)
        elif "Operation" in Data:  # WebSocket control message
            if Data["Operation"] == "Ping":
                Domoticz.Log("Ping Message received")
                Connection.Send({'Operation': 'Pong', 'Payload': 'Pong', 'Mask': secrets.randbits(32)})
            elif Data["Operation"] == "Pong":
                Domoticz.Log("Pong Message received")
            elif Data["Operation"] == "Close":
                Domoticz.Log("Close Message received")
            else:
                DumpWSResponseToLog(Data)
        elif "Payload" in Data:  # WebSocket data message
            DumpWSResponseToLog(Data)

    def onHeartbeat(self):
        Domoticz.Debug("onHeartbeat called")
        if self.websocketConn.Connected():
            self.websocketConn.Send({'Payload': 'Text message', 'Mask': secrets.randbits(32)})
            Domoticz.Log("Text message packet sent")
        else:
            self.reconAgain -= 1
            if self.reconAgain <= 0:
                self.websocketConn = Domoticz.Connection(Name="websocketConn", Transport="TCP/IP", Protocol="WSS",
                                                       Address=Parameters["Address"], Port=Parameters["Port"])
                self.websocketConn.Connect()
                self.reconAgain = 3
            else:
                Domoticz.Log("Will try reconnect again in " + str(self.reconAgain) + " heartbeats.")

    def onDisconnect(self, Connection):
        Domoticz.Log("websocket device disconnected")

    def onStop(self):
        Domoticz.Log("onStop called")
        return True

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

def onDisconnect(Connection):
    global _plugin
    _plugin.onDisconnect(Connection)

def onMessage(Connection, Data):
    global _plugin
    _plugin.onMessage(Connection, Data)

def onHeartbeat():
    global _plugin
    _plugin.onHeartbeat()

# Generic helper functions
def DumpWSResponseToLog(httpDict):
    if isinstance(httpDict, dict):
        Domoticz.Log("WebSocket Details ("+str(len(httpDict))+"):")
        for x in httpDict:
            if isinstance(httpDict[x], dict):
                Domoticz.Log("--->'"+x+" ("+str(len(httpDict[x]))+"):")
                for y in httpDict[x]:
                    Domoticz.Log("------->'" + y + "':'" + str(httpDict[x][y]) + "'")
            else:
                Domoticz.Log("--->'" + x + "':'" + str(httpDict[x]) + "'")
