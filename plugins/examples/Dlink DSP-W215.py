# Dlink smart plug DSP-W215
#
# Author: Dnpwwo, 2017
#
"""
<plugin key="DSP-W215" name="Dlink smart plug DSP-W215" author="Dnpwwo" version="1.2.0" externallink="https://www.dlink.com.au/home-solutions/dsp-w215-mydlink-wi-fi-smart-plug">
    <params>
        <param field="Address" label="IP Address" width="200px" required="true" default="127.0.0.1"/>
        <param field="Port" label="Port" width="30px" required="true" default="80"/>
        <param field="Mode1" label="Username" width="200px" required="true" default=""/>
        <param field="Mode2" label="Password" width="200px" required="true" default=""/>
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
import hmac
import hashlib
import time

class BasePlugin:
    enabled = False
    pluginState = "Not Ready"
    sessionCookie = ""
    privateKey = b""
    socketOn = "FALSE"
    httpConn = None
    
    def __init__(self):
        return

    def onStart(self):
        if Parameters["Mode6"] == "Debug":
            Domoticz.Debugging(1)
        if (len(Devices) == 0):
            Domoticz.Device(Name="Socket 1", Unit=1, TypeName="Switch").Create()
            Domoticz.Log("Device created.")
        DumpConfigToLog()
        self.httpConn = Domoticz.Connection(Name="DLink", Transport="TCP/IP", Protocol="HTTP", Address=Parameters["Address"], Port=Parameters["Port"])
        self.httpConn.Connect()
        Domoticz.Heartbeat(8)
        if (1 in Devices) and (Devices[1].nValue == 1):
            self.socketOn = "TRUE"

    def onStop(self):
        Domoticz.Log("Plugin is stopping.")

    def onConnect(self, Connection, Status, Description):
        if (Status == 0):
            Domoticz.Debug("Smart plug connected successfully.")
            data = '<?xml version="1.0" encoding="utf-8"?>' + \
                   '<soap:Envelope xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">' + \
                       '<soap:Body>' \
                           '<Login xmlns="http://purenetworks.com/HNAP1/">' + \
                               '<Action>request</Action>' + \
                               '<Username>'+Parameters["Mode1"]+'</Username>' + \
                               '<LoginPassword></LoginPassword>' + \
                               '<Captcha></Captcha>' + \
                           '</Login>' + \
                       '</soap:Body>' + \
                   '</soap:Envelope>'
            headers = { 'Content-Type': 'text/xml; charset=utf-8', \
                        'Connection': 'keep-alive', \
                        'Accept': 'Content-Type: text/html; charset=UTF-8', \
                        'Host': Parameters["Address"]+":"+Parameters["Port"], \
                        'User-Agent':'Domoticz/1.0', \
                        'SOAPAction' : '"http://purenetworks.com/HNAP1/Login"', \
                        'Content-Length' : "%d"%(len(data)) }
            self.httpConn.Send({'Verb':'POST', 'URL':'/HNAP1/', "Headers": headers, 'Data': data})
            self.pluginState = "GetAuth"
        else:
            self.pluginState = "Not Ready"
            Domoticz.Log("Failed to connect ("+str(Status)+") to: "+Parameters["Address"]+":"+Parameters["Port"])
            Domoticz.Debug("Failed to connect ("+str(Status)+") to: "+Parameters["Address"]+":"+Parameters["Port"]+" with error: "+Description)

    def onMessage(self, Connection, Data):
        strData = Data.decode("utf-8", "ignore")
        if (Status == 200):
            Domoticz.Debug("Good Response received for '"+self.pluginState+"'.")
            if (self.pluginState == "GetAuth"):
                challenge = extractTagValue('Challenge', strData).encode()
                self.sessionCookie = extractTagValue('Cookie', strData)
                publickey = extractTagValue('PublicKey', strData)
                loginresult = extractTagValue('LoginResult', strData)
                encdata = hmac.new((publickey + Parameters["Mode2"]).encode(), challenge, hashlib.md5)
                self.privateKey = encdata.hexdigest().upper().encode()
                encdata = hmac.new(self.privateKey, challenge, hashlib.md5)
                loginpassword = encdata.hexdigest().upper()
                data = '<?xml version="1.0" encoding="utf-8"?>' + \
                       '<soap:Envelope xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">' + \
                           '<soap:Body>' + \
                               '<Login xmlns="http://purenetworks.com/HNAP1/">' + \
                                   '<Action>login</Action>' + \
                                   '<Username>Admin</Username>' + \
                                   '<LoginPassword>' + loginpassword + '</LoginPassword>' + \
                                   '<Captcha></Captcha>' + \
                               '</Login>' + \
                           '</soap:Body>' + \
                       '</soap:Envelope>'
                headers = { 'Content-Type': 'text/xml; charset=utf-8',
                            'Accept': 'Content-Type: text/html; charset=UTF-8', \
                            'Host': Parameters["Address"]+":"+Parameters["Port"], \
                            'User-Agent':'Domoticz/1.0', \
                            'SOAPAction' : '"http://purenetworks.com/HNAP1/Login"', \
                            'Content-Length' : "%d"%(len(data)), \
                            'Cookie' : 'uid=' + self.sessionCookie
                            }
                self.httpConn.Send({'Verb':'POST', 'URL':'/HNAP1/', "Headers": headers, 'Data': data})
                self.pluginState = "Login"
            elif (self.pluginState == "Login"):
                loginresult = extractTagValue('LoginResult', strData)
                if (loginresult.upper() != "SUCCESS"):
                    Domoticz.Error("Login failed, check username and password in Hardware page")
                else:
                    Domoticz.Log("Smart plug authentication successful.")
                    self.pluginState = "Ready"
            elif (self.pluginState == "Ready"):
                if (strData.find('GetSocketSettingsResult') > 0):
                    opStatus = extractTagValue('OPStatus', strData).upper()
                    if (len(opStatus) and (self.socketOn != opStatus)):
                        self.socketOn = opStatus
                        Domoticz.Log("Socket State has changed, Now: "+self.socketOn)
                        if (self.socketOn == "TRUE"):
                            if (1 in Devices): Devices[1].Update(1,"100")
                        else:
                            if (1 in Devices): Devices[1].Update(0,"0")
                elif (strData.find('SetSocketSettingsResult') > 0):
                    self.GetSocketSettings()
                Domoticz.Debug(self.pluginState+": "+strData)
            else:
                Domoticz.Debug(self.pluginState+": "+strData)
        elif (Status == 400):
            Domoticz.Error("Smart plug returned a Bad Request Error.")
        elif (Status == 500):
            Domoticz.Error("Smart plug returned a Server Error.")

    def onCommand(self, Unit, Command, Level, Hue):
        Domoticz.Debug("onCommand called for Unit " + str(Unit) + ": Parameter '" + str(Command) + "', Level: " + str(Level))
        if (Command.upper() == 'ON'):
            self.SetSocketSettings("true")
        else:
            self.SetSocketSettings("false")

    def onDisconnect(self, Connection):
        self.pluginState = "Not Ready"
        Domoticz.Log("Device has disconnected")
        self.httpConn.Connect()

    def onHeartbeat(self):
        Domoticz.Debug("onHeartbeat called")
        if (self.pluginState == "Not Ready"):
            self.httpConn.Connect()
        elif (self.pluginState == "Ready"):
            self.GetSocketSettings()

    def SetSocketSettings(self, power):
        timestamp = str(int(time.time()))
        encdata = hmac.new(self.privateKey, (timestamp + '"http://purenetworks.com/HNAP1/SetSocketSettings"').encode(), hashlib.md5)
        hnapauth = encdata.hexdigest().upper() + ' ' + timestamp
        data = '<?xml version="1.0" encoding="utf-8"?>' + \
               '<soap:Envelope xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">' + \
                   '<soap:Body>' + \
                       '<SetSocketSettings xmlns="http://purenetworks.com/HNAP1/">' + \
                           '<ModuleID>1</ModuleID>' + \
                           '<NickName>Socket 1</NickName>' + \
                           '<Description>Socket 1</Description>' + \
                           '<OPStatus>' + power + '</OPStatus>' + \
                           '<Controller>1</Controller>' + \
                       '</SetSocketSettings>' + \
                   '</soap:Body>' + \
               '</soap:Envelope>'
        headers = {'Content-Type': 'text/xml; charset=utf-8',
                   'Accept': 'Content-Type: text/html; charset=UTF-8', \
                   'Connection': 'keep-alive', \
                   'Host': Parameters["Address"]+":"+Parameters["Port"], \
                   'User-Agent':'Domoticz/1.0', \
                   'SOAPAction' : '"http://purenetworks.com/HNAP1/SetSocketSettings"',
                   'HNAP_AUTH'  :  hnapauth,
                   'Content-Length' : "%d"%(len(data)),
                   'Cookie' : 'uid=' + self.sessionCookie
                  }
        # The device resets the connection every 2 minutes, if its not READY assume that is happening and delay the send by 2 seconds  to allow reconnection
        if (self.pluginState == "Ready"):
            self.httpConn.Send({'Verb':'POST', 'URL':'/HNAP1/', "Headers": headers, 'Data': data})
        else:
            self.httpConn.Send({'Verb':'POST', 'URL':'/HNAP1/', "Headers": headers, 'Data': data}, 2)

    def GetSocketSettings(self):
        timestamp = str(int(time.time()))
        encdata = hmac.new(self.privateKey, (timestamp + '"http://purenetworks.com/HNAP1/GetSocketSettings"').encode(), hashlib.md5)
        hnapauth = encdata.hexdigest().upper() + ' ' + timestamp
        data = '<?xml version="1.0" encoding="utf-8"?>' + \
               '<soap:Envelope xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">' + \
                   '<soap:Body>' + \
                       '<GetSocketSettings xmlns="http://purenetworks.com/HNAP1/">' + \
                           '<ModuleID>1</ModuleID>' + \
                       '</GetSocketSettings>' + \
                   '</soap:Body>' + \
               '</soap:Envelope>'
        headers = {'Content-Type': 'text/xml; charset=utf-8',
                   'Accept': 'Content-Type: text/html; charset=UTF-8', \
                   'Connection': 'keep-alive', \
                   'Host': Parameters["Address"]+":"+Parameters["Port"], \
                   'User-Agent':'Domoticz/1.0', \
                   'SOAPAction' : '"http://purenetworks.com/HNAP1/GetSocketSettings"',
                   'HNAP_AUTH'  :  hnapauth,
                   'Content-Length' : "%d"%(len(data)),
                   'Cookie' : 'uid=' + self.sessionCookie
                  }
        self.httpConn.Send({'Verb':'POST', 'URL':'/HNAP1/', "Headers": headers, 'Data': data})

    def genericPOST(self, commandName):
        timestamp = str(int(time.time()))
        encdata = hmac.new(self.privateKey, (timestamp + '"http://purenetworks.com/HNAP1/'+commandName+'"').encode(), hashlib.md5)
        hnapauth = encdata.hexdigest().upper() + ' ' + timestamp
        data = '<?xml version="1.0" encoding="utf-8"?>' + \
               '<soap:Envelope xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">' + \
                   '<soap:Body>' + \
                       '<'+commandName+' xmlns="http://purenetworks.com/HNAP1/"/>' + \
                   '</soap:Body>' + \
               '</soap:Envelope>'
        headers = {'Content-Type': 'text/xml; charset=utf-8',
                   'Accept': 'Content-Type: text/html; charset=UTF-8', \
                   'Connection': 'keep-alive', \
                   'Host': Parameters["Address"]+":"+Parameters["Port"], \
                   'User-Agent':'Domoticz/1.0', \
                   'SOAPAction' : '"http://purenetworks.com/HNAP1/'+commandName+'"',
                   'HNAP_AUTH'  :  hnapauth,
                   'Content-Length' : "%d"%(len(data)),
                   'Cookie' : 'uid=' + self.sessionCookie
                  }
        self.httpConn.Send({'Verb':'POST', 'URL':'/HNAP1/', "Headers": headers, 'Data': data})
        return
            
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

# xml built in parser threw import error on expat so just do it manually
def extractTagValue(tagName, XML):
    startPos = XML.find(tagName)
    endPos = XML.find(tagName, startPos+1)
    if ((startPos == -1) or (endPos == -1)): Domoticz.Error("'"+tagName+"' not found in supplied XML")
    return XML[startPos+len(tagName)+1:endPos-2]

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

def DumpHTTPResponseToLog(httpDict):
    if isinstance(httpDict, dict):
        Domoticz.Debug("HTTP Details ("+str(len(httpDict))+"):")
        for x in httpDict:
            if isinstance(httpDict[x], dict):
                Domoticz.Debug("--->'"+x+" ("+str(len(httpDict[x]))+"):")
                for y in httpDict[x]:
                    Domoticz.Debug("------->'" + y + "':'" + str(httpDict[x][y]) + "'")
            else:
                Domoticz.Debug("--->'" + x + "':'" + str(httpDict[x]) + "'")
