#           Serial Plugin
#
#           Author:     Dnpwwo, 2017
#
#
#   Plugin parameter definition below will be parsed during startup and copied into Manifest.xml, this will then drive the user interface in the Hardware web page
#
"""
<plugin key="Serial" name="Serial base plugin" author="dnpwwo" version="1.0.0" >
    <params>
        <param field="SerialPort" label="Serial Port" width="150px" required="true" default=""/>
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

def onStart():
    if Parameters["Mode6"] == "Debug":
        Domoticz.Debugging(1)
    if (len(Devices) == 0):
        # Create devices here, see examples
        Domoticz.Log("Devices created.")
    Domoticz.Log("Plugin has " + str(len(Devices)) + " devices associated with it.")
    DumpConfigToLog()
    Domoticz.Transport("Serial", Parameters["SerialPort"], 115200)
    #Domoticz.Protocol("XML")  # None,XML,JSON,HTTP
    Domoticz.Connect()
    with open(Parameters["HomeFolder"]+"Response.txt", "wt") as text_file:
        print("Started.", file=text_file)
    return

def onConnect(Status, Description):
    global isConnected
    if (Status == 0):
        isConnected = True
        Domoticz.Log("Connected successfully to: "+Parameters["SerialPort"])
        # Send any initialisation commands here e.g:
        #Domoticz.Send("<Command>\n  <Name>get_device_info</Name>\n</Command>")
    else:
        Domoticz.Log("Failed to connect ("+str(Status)+") to: "+Parameters["SerialPort"])
        Domoticz.Debug("Failed to connect ("+str(Status)+") to: "+Parameters["SerialPort"]+" with error: "+Description)
    return True

def onMessage(self, Data, Status, Extra):
    Domoticz.Log("onMessage called")
    try:
        strData = Data.decode()
        if Parameters["Mode6"] == "Debug":
            with open(Parameters["HomeFolder"]+"Response.txt", "at") as text_file:
                print(strData, file=text_file)

        # update Domoticz devices here
        
    except Exception as inst:
        Domoticz.Error("Exception in onMessage, called with Data: '"+str(strData)+"'")
        Domoticz.Error("Exception detail: '"+str(inst)+"'")
        raise
        
def onCommand(Unit, Command, Level, Hue):
    global isConnected
    Domoticz.Log("onCommand called for Unit " + str(Unit) + ": Parameter '" + str(Command) + "', Level: " + str(Level))
    
    #if (isConnected == True):
    #     send commands to external hardware here
    
    return

def onDisconnect():
    global isConnected
    isConnected = False
    Domoticz.Log("Device Disconnected")
    return

def onHeartbeat():
    global isConnected
    if (isConnected != True):
        Domoticz.Connect()
    return True

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

