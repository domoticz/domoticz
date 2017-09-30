#           ICMP Plugin
#
#           Author:     Dnpwwo, 2017
#
#           Uses a single ICMP connection because on Windows once any device fails to ping
#           all open connections stop returning ping results :(
#
"""
<plugin key="ICMP" name="ICMP Listener" author="dnpwwo" version="2.0.0">
    <description>
ICMP Pinger Plugin.<br/><br/>
Specify comma delimted addresses (IP or DNS names) of devices that are to be pinged.<br/>
When remote devices are found a matching Domoticz device is created in the Devices tab.
    </description>
    <params>
        <param field="Address" label="Address(es)" width="300px" required="true" default="127.0.0.1"/>
        <param field="Mode1" label="Ping Frequency" width="40px">
            <options>
                <option label="1" value="1"/>
                <option label="2" value="2"/>
                <option label="4" value="4"/>
                <option label="6" value="6"/>
                <option label="8" value="8"/>
                <option label="10" value="10" default="true" />
                <option label="12" value="12"/>
                <option label="14" value="14"/>
                <option label="16" value="16"/>
                <option label="18" value="18"/>
                <option label="20" value="20"/>
            </options>
        </param>
        <param field="Mode5" label="Log File" width="75px">
            <options>
5                <option label="True" value="File"/>
                <option label="False" value="Normal"  default="true" />
            </options>
        </param>
        <param field="Mode6" label="Debug" width="75px">
            <options>
5                <option label="True" value="Debug"/>
                <option label="False" value="Normal"  default="true" />
            </options>
        </param>
    </params>
</plugin>
"""
import Domoticz
from datetime import datetime

class BasePlugin:
    icmpConn = None
    endpointList = None
    lastIndex = -1
    log = None
 
    def onStart(self):
        self.ConfigureLogging(Parameters["Mode5"])

        if Parameters["Mode6"] != "Normal":
            Domoticz.Debugging(1)
            DumpConfigToLog()

        self.endpointList = Parameters["Address"].split(",")
        
        Domoticz.Heartbeat(int(Parameters["Mode1"]))
        
    def onConnect(self, Connection, Status, Description):
        if (Status == 0):
            Domoticz.Log("Successful connect to: "+Connection.Address+" which is surprising because ICMP is connectionless.")
        else:
            Domoticz.Error("Error connecting to: "+Connection.Address+", Connection: "+Connection.Name+", Description: "+Description)

    def onMessage(self, Connection, Data):
        Domoticz.Debug("onMessage called for connection: '"+Connection.Name+"', with address: "+Connection.Address)
        if Parameters["Mode6"] == "Verbose":
            DumpICMPResponseToLog(Data)
        if isinstance(Data, dict) and (Data["Status"] == 0):
            iUnit = int(Data["IPv4"]["Source"].split(".")[3])
            Domoticz.Debug("Device: '"+Connection.Address+"' responding at: "+Data["IPv4"]["Source"]+", Unit number: "+str(iUnit))
            if not iUnit in Devices:
                # Store the address in the option field so that users can change the device name and IP address and it can still be mapped to
                Domoticz.Device(Name=Connection.Address, Unit=iUnit, Type=17, Subtype=0, Image=17, Options={"Address":Connection.Address}).Create()
                Domoticz.Log("Device: '"+Connection.Address+"' created and listed in Devices Tab.")
            UpdateDevice(iUnit, 1, "On")
        else:
            Domoticz.Debug("Device: '"+Connection.Address+"' returned '"+Data["Description"]+"'.")
            for Device in Devices:
                if (Devices[Device].Options["Address"] == Connection.Address):
                    UpdateDevice(Device, 0, "Off")
        self.icmpConn = None

    def onHeartbeat(self):
        self.icmpConn = None
        self.lastIndex += 1
        if (self.lastIndex >= len(self.endpointList)): self.lastIndex = 0
        self.icmpConn = Domoticz.Connection(Name=self.endpointList[self.lastIndex]+" Connection", Transport="ICMP/IP", Protocol="ICMP", Address=self.endpointList[self.lastIndex])
        self.icmpConn.Listen()
        self.icmpConn.Send("Domoticz",1)
        
    def ConfigureLogging(self, option):
        if (option == "File"):
            import os
            import logging
            import logging.config
            #logging.config.fileConfig(Parameters["HomeFolder"]+'plugin_log.conf')
            logging.config.dictConfig(  { 
                                            'version': 1,
                                            'disable_existing_loggers': False,
                                            'formatters': { 
                                                'standard': { 
                                                    'format': '%(asctime)s [%(levelname)s] %(name)s: %(message)s'
                                                },
                                            },
                                            'handlers': { 
                                                'default': { 
                                                    'level': 'INFO',
                                                    'formatter': 'standard',
                                                    'class': 'logging.handlers.RotatingFileHandler',
                                                    'filename': Parameters["HomeFolder"]+'plugin.log',
                                                    'mode': 'a',
                                                    'maxBytes': 10485760,
                                                    'backupCount': 5,
                                                },
                                            },
                                        })
            self.log = logging.getLogger("[" + str(os.getpid()) + "]")
            self.log.debug('This is a debug message!')
            self.log.info('This is an info message!')
            self.log.error('This is an error message!')

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
        Domoticz.Debug("Device ID:       '" + str(Devices[x].ID) + "'")
        Domoticz.Debug("Device Name:     '" + Devices[x].Name + "'")
        Domoticz.Debug("Device nValue:    " + str(Devices[x].nValue))
        Domoticz.Debug("Device sValue:   '" + Devices[x].sValue + "'")
        Domoticz.Debug("Device LastLevel: " + str(Devices[x].LastLevel))
    return

def DumpICMPResponseToLog(icmpDict):
    if isinstance(icmpDict, dict):
        Domoticz.Debug("ICMP Details ("+str(len(icmpDict))+"):")
        for x in icmpDict:
            if isinstance(icmpDict[x], dict):
                Domoticz.Debug("--->'"+x+" ("+str(len(icmpDict[x]))+"):")
                for y in icmpDict[x]:
                    Domoticz.Debug("------->'" + y + "':'" + str(icmpDict[x][y]) + "'")
            else:
                Domoticz.Debug("--->'" + x + "':'" + str(icmpDict[x]) + "'")
    else:
        Domoticz.Debug(Data.decode("utf-8", "ignore"))
