#           Sonos Plugin
#
#           Author:     Tester22, 2017
#
"""
<plugin key="Sonos" name="Sonos Players" author="tester22" version="2.0" wikilink="http://www.domoticz.com/wiki/plugins/Sonos.html" externallink="https://sonos.com/">
    <params>
        <param field="Address" label="IP Address" width="200px" required="true" default="127.0.0.1"/>
        <param field="Mode1" label="Update interval" width="150px" required="true" default="30"/>
        <param field="Mode2" label="Volume bar" width="75px">
            <options>
                <option label="True" value="Volume"/>
                <option label="False" value="Fixed"  default="true" />
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
import http.client

class BasePlugin:
    # player status
    playerState = 0
    mediaLevel = 0
    mediaDescription = ""
    muted = 2
    creator = None
    title = None
    
    def onStart(self):
        if Parameters["Mode6"] == "Debug":
            Domoticz.Debugging(1)

        if (len(Devices) == 0):
            Domoticz.Device(Name="Status",  Unit=1, Type=17,  Switchtype=17).Create()
            if Parameters["Mode2"] == "Volume":
                Domoticz.Device(Name="Volume",  Unit=2, Type=244, Subtype=73, Switchtype=7,  Image=8).Create()
            Domoticz.Log("Devices created.")
        elif (Parameters["Mode2"] == "Volume" and 2 not in Devices):
            Domoticz.Device(Name="Volume",  Unit=2, Type=244, Subtype=73, Switchtype=7,  Image=8).Create()
            Domoticz.Log("Volume device created.")
        elif (Parameters["Mode2"] != "Volume" and 2 in Devices):
            Devices[2].Delete()
            Domoticz.Log("Volume device deleted.")
        else:
            if (1 in Devices): self.playerState = Devices[1].nValue
            if (2 in Devices): self.mediaLevel = Devices[2].nValue
    
        if self.is_number(Parameters["Mode1"]):
            if  int(Parameters["Mode1"]) < 30:
                Domoticz.Log("Update interval set to " + Parameters["Mode1"])
                Domoticz.Heartbeat(int(Parameters["Mode1"]))
            else:
                Domoticz.Heartbeat(30)
        else:
            Domoticz.Heartbeat(30)
        DumpConfigToLog()
        
        return

    def onConnect(self, Connection, Status, Description):
        return

    def onMessage(self, Connection, Data, Status, Extra):
        return
    

    def parseMessage(self, Data):
        strData = str(Data)
        if (strData.find('CurrentTransportState') > 0):
            CurrentTransportState = extractTagValue('CurrentTransportState', strData).upper()
            if CurrentTransportState != None:
                if CurrentTransportState  == 'PLAYING':
                    self.playerState = 1
                if CurrentTransportState  == 'PAUSED_PLAYBACK' or CurrentTransportState  == 'STOPPED':
                    self.playerState = 0
                    self.mediaDescription = ''
                UpdateDevice(1, self.playerState, self.mediaDescription)
        elif (strData.find('TrackMetaData') > 0):
            if (extractTagValue('TrackMetaData', strData).upper() == "NOT_IMPLEMENTED"):
                self.mediaDescription = "Grouped"
                self.playerState = 0
                UpdateDevice(1, self.playerState, self.mediaDescription)
            else:
                strData = strData.replace('&lt;','<').replace('&gt;','>')
                temp_creator = extractTagValue('dc:creator', strData)
                temp_title = extractTagValue('dc:title', strData)
                #Domoticz.Log(strData)
                
                if temp_creator != None:
                    self.creator = temp_creator
                if temp_title != None:
                    self.title = temp_title
                
                if not self.title:
                    dash = ""
                else:
                    dash = " - "
                    
                self.mediaDescription = str(self.creator) + dash + str(self.title)
                #mediaDescription = ""
                #UpdateDevice(1, self.playerState, self.mediaDescription)
            
        return

    def onCommand(self, Unit, Command, Level, Hue):
        Domoticz.Debug("onCommand called for Unit " + str(Unit) + ": Parameter '" + str(Command) + "'")
    
        Command = Command.strip()
        action, sep, params = Command.partition(' ')
        action = action.capitalize()
    
    
        if (Unit == 1):  # Playback control
            if (action == 'On') or (action == 'Play'):
                self.sendMessage('<?xml version="1.0"?><s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><s:Body><u:Play xmlns:u="urn:schemas-upnp-org:service:AVTransport:1"><InstanceID>0</InstanceID><Speed>1</Speed></u:Play></s:Body></s:Envelope>', 'urn:schemas-upnp-org:service:AVTransport:1#Play', "/MediaRenderer/AVTransport/Control")
                self.playerState = 1
                UpdateDevice(1, self.playerState, self.mediaDescription)
            elif (action == 'Pause') or (action == 'Off'):
                self.sendMessage('<?xml version="1.0"?><s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><s:Body><u:Pause xmlns:u="urn:schemas-upnp-org:service:AVTransport:1"><InstanceID>0</InstanceID><Speed>1</Speed></u:Pause></s:Body></s:Envelope>', 'urn:schemas-upnp-org:service:AVTransport:1#Pause', "/MediaRenderer/AVTransport/Control")
                self.playerState = 0
                UpdateDevice(1, self.playerState, self.mediaDescription)
            elif (action == 'Stop'):
                self.sendMessage('urn:schemas-upnp-org:service:AVTransport:1#Stop', '<?xml version="1.0"?><s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><s:Body><u:Stop xmlns:u="urn:schemas-upnp-org:service:AVTransport:1"><InstanceID>0</InstanceID><Speed>1</Speed></u:Stop></s:Body></s:Envelope>', "/MediaRenderer/AVTransport/Control")
                self.playerState = 0
                UpdateDevice(1, self.playerState, self.mediaDescription)
            elif (action == 'Next'):
                self.sendMessage('<?xml version="1.0"?><s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><s:Body><u:Next xmlns:u="urn:schemas-upnp-org:service:AVTransport:1"><InstanceID>0</InstanceID><Speed>1</Speed></u:Next></s:Body></s:Envelope>', 'urn:schemas-upnp-org:service:AVTransport:1#Next', "/MediaRenderer/AVTransport/Control")
            elif (action == 'Previous'):
                self.sendMessage('<?xml version="1.0"?><s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><s:Body><u:Previous xmlns:u="urn:schemas-upnp-org:service:AVTransport:1"><InstanceID>0</InstanceID><Speed>1</Speed></u:Previous></s:Body></s:Envelope>', 'urn:schemas-upnp-org:service:AVTransport:1#Previous', "/MediaRenderer/AVTransport/Control")
        if (Unit == 2):  # Volume control
            if (action == 'Set') and ((params.capitalize() == 'Level') or (Command.lower() == 'Volume')):
                self.mediaLevel = Level
                self.sendMessage('<?xml version="1.0"?><s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><s:Body><u:SetVolume xmlns:u="urn:schemas-upnp-org:service:RenderingControl:1"><InstanceID>0</InstanceID><Channel>Master</Channel><DesiredVolume>' + str(self.mediaLevel) +  '</DesiredVolume></u:SetVolume></s:Body></s:Envelope>', 'urn:schemas-upnp-org:service:RenderingControl:1#SetVolume', "/MediaRenderer/RenderingControl/Control")
                # Send an update request to get updated data from the player
                self.sendMessage('<?xml version="1.0"?><s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><s:Body><u:GetVolume xmlns:u="urn:schemas-upnp-org:service:RenderingControl:1"><InstanceID>0</InstanceID><Channel>Master</Channel></u:GetVolume></s:Body></s:Envelope>', 'urn:schemas-upnp-org:service:RenderingControl:1#GetVolume', "/MediaRenderer/RenderingControl/Control")
                UpdateDevice(2, 0, self.mediaLevel)
            elif (action == 'On') or (action == 'Off'):
                if action == 'On':
                    DesiredMute = "0"
                else:
                    DesiredMute = "1"
                self.sendMessage('<?xml version="1.0"?><s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><s:Body><u:SetMute xmlns:u="urn:schemas-upnp-org:service:RenderingControl:1"><InstanceID>0</InstanceID><Channel>Master</Channel><DesiredMute>' + DesiredMute + '</DesiredMute></u:SetMute></s:Body></s:Envelope>', 'urn:schemas-upnp-org:service:RenderingControl:1#SetMute', "/MediaRenderer/RenderingControl/Control")
                self.sendMessage('<?xml version="1.0"?><s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><s:Body><u:GetMute xmlns:u="urn:schemas-upnp-org:service:RenderingControl:1"><InstanceID>0</InstanceID><Channel>Master</Channel></u:GetMute></s:Body></s:Envelope>', 'urn:schemas-upnp-org:service:RenderingControl:1#GetMute', "/MediaRenderer/RenderingControl/Control")
                if playerState == 0:
                    UpdateDevice(2, 0, self.mediaLevel)
                else:
                    UpdateDevice(2, self.muted, self.mediaLevel)
        self.SyncDevices()

        return

    def onDisconnect(self, Connection):
        return

    def is_number(self, s):
        try:
            float(s)
            return True
        except ValueError:
            return False

    def sendMessage(self, data, method, url):
        conn = http.client.HTTPConnection(Parameters["Address"] + ":1400")
        headers = {"Content-Type": 'text/xml; charset="utf-8"', "SOAPACTION": method}
        conn.request("POST", url, data, headers)
        response = conn.getresponse()
        conn.close()
    
        if response.status == 200:
            data = response.read().decode("utf-8")
            self.parseMessage(data)
        return

    def onHeartbeat(self):
        self.sendMessage('<?xml version="1.0"?><s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><s:Body><u:GetTransportInfo xmlns:u="urn:schemas-upnp-org:service:AVTransport:1"><InstanceID>0</InstanceID></u:GetTransportInfo></s:Body></s:Envelope>', 'urn:schemas-upnp-org:service:AVTransport:1#GetTransportInfo', "/MediaRenderer/AVTransport/Control")
        # Dont upate mediainfo if player is stopped
        if self.playerState == 1:
            self.sendMessage('<?xml version="1.0"?><s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><s:Body><u:GetPositionInfo xmlns:u="urn:schemas-upnp-org:service:AVTransport:1"><InstanceID>0</InstanceID><Channel>Master</Channel></u:GetPositionInfo></s:Body></s:Envelope>', 'urn:schemas-upnp-org:service:AVTransport:1#GetPositionInfo', "/MediaRenderer/AVTransport/Control")
        if Parameters["Mode2"] == "Volume":
            self.sendMessage('<?xml version="1.0"?><s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><s:Body><u:GetVolume xmlns:u="urn:schemas-upnp-org:service:RenderingControl:1"><InstanceID>0</InstanceID><Channel>Master</Channel></u:GetVolume></s:Body></s:Envelope>', 'urn:schemas-upnp-org:service:RenderingControl:1#GetVolume', "/MediaRenderer/RenderingControl/Control")
            self.sendMessage('<?xml version="1.0"?><s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><s:Body><u:GetMute xmlns:u="urn:schemas-upnp-org:service:RenderingControl:1"><InstanceID>0</InstanceID><Channel>Master</Channel></u:GetMute></s:Body></s:Envelope>', 'urn:schemas-upnp-org:service:RenderingControl:1#GetMute', "/MediaRenderer/RenderingControl/Control")
        return

    def SyncDevices(self):
        # Make sure that the Domoticz devices are in sync (by definition, the device is connected)
        UpdateDevice(1, self.playerState, self.mediaDescription)
        if self.playerState == 0:
            UpdateDevice(2, 0, self.mediaLevel)
        else:
            if self.muted == 2:
                UpdateDevice(2, 2, self.mediaLevel)
            else:
                UpdateDevice(2, 0, self.mediaLevel)
                
        return
        
global _plugin
_plugin = BasePlugin()

def onStart():
    global _plugin
    _plugin.onStart()

def onConnect(Connection, Status, Description):
    global _plugin
    _plugin.onConnect(Connection, Status, Description)

def onMessage(Connection, Data, Status, Extra):
    global _plugin
    _plugin.onMessage(Connection, Data, Status, Extra)

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

# xml built in parser threw import error on expat so just do it manually
def extractTagValue(tagName, XML):
    startPos = XML.find(tagName)
    endPos = XML.find(tagName, startPos+1)
    if ((startPos == -1) or (endPos == -1)): Domoticz.Error("'"+tagName+"' not found in supplied XML")
    return XML[startPos+len(tagName)+1:endPos-2]

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