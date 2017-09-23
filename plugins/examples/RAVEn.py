#           RAVEn Plugin
#
#           Author:     Dnpwwo, 2016
#
#
#   Plugin parameter definition below will be parsed during startup and copied into Manifest.xml, this will then drive the user interface in the Hardware web page
#
"""
<plugin key="RAVEn" name="RAVEn Zigbee energy monitor" author="dnpwwo" version="1.3.10" externallink="https://rainforestautomation.com/rfa-z106-raven/">
    <params>
        <param field="SerialPort" label="Serial Port" width="150px" required="true" default="/dev/ttyRAVEn"/>
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
import xml.etree.ElementTree as ET

SerialConn = None

demandFreq=30       # seconds between demand events
summaryFreq=300     # seconds between summary updates
fScale = demandFreq / 3600.0
summation = 0.0
hasConnected = False
nextCommand = ""

def onStart():
    global SerialConn
    if Parameters["Mode6"] != "Normal":
        Domoticz.Debugging(1)
    if Parameters["Mode6"] == "Debug":
        f = open(Parameters["HomeFolder"]+"plugin.log","w")
        f.write("Plugin started.")
        f.close()
    if (len(Devices) == 0):
        Domoticz.Device(Name="Usage", Unit=1, Type=243, Subtype=29, Switchtype=0, Image=0, Options="").Create()
        Domoticz.Device("Total", 2, 113).Create()
        Domoticz.Log("Devices created.")
    Domoticz.Log("Plugin has " + str(len(Devices)) + " devices associated with it.")
    DumpConfigToLog()
    SerialConn = Domoticz.Connection(Name="RAVEn", Transport="Serial", Protocol="XML", Address=Parameters["SerialPort"], Baud=115200)
    SerialConn.Connect()
    return

def onConnect(Connection, Status, Description):
    global SerialConn
    if (Status == 0):
        Domoticz.Log("Connected successfully to: "+Parameters["SerialPort"])
        Connection.Send("<Command>\n  <Name>restart</Name>\n</Command>")
        SerialConn = Connection
    else:
        Domoticz.Log("Failed to connect ("+str(Status)+") to: "+Parameters["SerialPort"])
        Domoticz.Debug("Failed to connect ("+str(Status)+") to: "+Parameters["SerialPort"]+" with error: "+Description)
    return True

def onMessage(Connection, Data):
    global hasConnected, nextCommand, fScale, summation
    strData = Data.decode("utf-8", "ignore")
    LogMessage(strData)
    xmltree = ET.fromstring(strData)
    if xmltree.tag == 'ConnectionStatus':
        strLog = ""
        if (xmltree.find('MeterMacId') != None): strLog = "MeterMacId: "+xmltree.find('MeterMacId').text+", "
        connectStatus = xmltree.find('Status').text
        strLog += "Connection Status = '"+connectStatus+"'"
        if (xmltree.find('Description') != None): strLog += " - "+xmltree.find('Description').text
        if (xmltree.find('LinkStrength') != None): strLog += ", Link Strength = "+str(int(xmltree.find('LinkStrength').text,16))
        Domoticz.Log(strLog)
        if connectStatus == 'Initializing...':
            hasConnected = False
        elif (connectStatus == 'Connected') and (hasConnected == False):
            nextCommand = "get_device_info"
            hasConnected = True
    elif xmltree.tag == 'DeviceInfo':
        Domoticz.Log( "Manufacturer: %s, Device ID: %s, Install Code: %s" % (xmltree.find('Manufacturer').text, xmltree.find('DeviceMacId').text, xmltree.find('InstallCode').text) )
        Domoticz.Log( "Hardware: Version %s, Firmware Version: %s, Model: %s" % (xmltree.find('HWVersion').text, xmltree.find('FWVersion').text, xmltree.find('ModelId').text) )
        nextCommand = "get_network_info"
    elif xmltree.tag == 'NetworkInfo':
        LogMessage( "NetworkInfo response, Status = '%s' - %s, Link Strength = %d" % (xmltree.find('Status').text, xmltree.find('Description').text, int(xmltree.find('LinkStrength').text,16)))
        nextCommand = "get_meter_list"
    elif xmltree.tag == 'MeterList':
        nextCommand = ""
        for meter in xmltree.iter('MeterMacId'):
            LogMessage( "MeterMacId: %s, MeterList response" % meter.text)
            Connection.Send("<Command>\n  <Name>get_meter_info</Name>\n  <MeterMacId>"+meter.text+"</MeterMacId>\n</Command>\n")
    elif xmltree.tag == 'MeterInfo':
        LogMessage( "MeterMacId: %s, MeterInfo response, Enabled = %s" % (xmltree.find('MeterMacId').text, xmltree.find('Enabled').text))
        Connection.Send("<Command>\n  <Name>get_schedule</Name>\n  <MeterMacId>"+xmltree.find('MeterMacId').text+"</MeterMacId>\n</Command>\n")
    elif xmltree.tag == 'ScheduleInfo':
        iFreq = int(xmltree.find('Frequency').text,16)
        LogMessage( "MeterMacId: %s, ScheduleInfo response: Type '%s', Frequency %d, Enabled %s" % (xmltree.find('MeterMacId').text, xmltree.find('Event').text, iFreq, xmltree.find('Enabled').text))
        if (xmltree.find('Event').text == 'demand') and (iFreq != demandFreq):
            LogMessage( "MeterMacId: %s, Setting 'demand' schedule to: Frequency %d" % (xmltree.find('MeterMacId').text, demandFreq))
            Connection.Send("<Command>\n  <Name>set_schedule</Name>\n  <MeterMacId>"+xmltree.find('MeterMacId').text+"</MeterMacId>\n <Event>demand</Event>\n  <Frequency>" + str(hex(demandFreq)) + "</Frequency>\n  <Enabled>Y</Enabled>\n</Command>\n")
        if (xmltree.find('Event').text == 'summation') and (iFreq != summaryFreq):
            LogMessage( "MeterMacId: %s, Setting 'summation' schedule to: Frequency %d" % (xmltree.find('MeterMacId').text, summaryFreq))
            Connection.Send("<Command>\n  <Name>set_schedule</Name>\n  <MeterMacId>"+xmltree.find('MeterMacId').text+"</MeterMacId>\n  <Event>summation</Event>\n  <Frequency>" + str(hex(summaryFreq)) + "</Frequency>\n  <Enabled>Y</Enabled>\n</Command>\n")
        if (xmltree.find('Event').text == 'summation'):
            Connection.Send("<Command>\n  <Name>get_current_summation_delivered</Name>\n  <MeterMacId>"+xmltree.find('MeterMacId').text+"</MeterMacId>\n  <Refresh>Y</Refresh>\n</Command>\n")
    elif xmltree.tag == 'InstantaneousDemand':
        demand = float(getInstantDemandKWh(xmltree))
        if (summation == 0.0):
          Domoticz.Log("MeterMacId: %s, Instantaneous Demand = %f, NO SUMMARY DATA" % (xmltree.find('MeterMacId').text, demand))
        else:
          delta = fScale * demand
          summation = summation + delta
          Domoticz.Log( "MeterMacId: %s, Instantaneous Demand = %.3f, Summary Total = %.3f, Delta = %f" % (xmltree.find('MeterMacId').text, demand, summation, delta))
          sValue = "%.3f;%.3f" % (demand,summation)
          Devices[1].Update(0, sValue.replace('.',''))
    elif xmltree.tag == 'CurrentSummationDelivered':
        total = float(getCurrentSummationKWh(xmltree))
        if (total > summation):
          summation = total
        sValue = "%.3f" % (total)
        Devices[2].Update(0, sValue.replace('.',''))
        Domoticz.Log( "MeterMacId: %s, Current Summation = %.3f" % (xmltree.find('MeterMacId').text, total))

    elif xmltree.tag == 'TimeCluster':
      Domoticz.Debug( xmltree.tag + " response" )
    elif xmltree.tag == 'PriceCluster':
      Domoticz.Debug( xmltree.tag + " response" )
    elif xmltree.tag == 'CurrentPeriodUsage':
      Domoticz.Debug( xmltree.tag + " response" )
    elif xmltree.tag == 'LastPeriodUsage':
      Domoticz.Debug( xmltree.tag + " response" )
    elif xmltree.tag == 'ProfileData':
      Domoticz.Debug( xmltree.tag + " response" )
    else:
        Domoticz.Error("Unrecognised (not implemented) XML Fragment ("+xmltree.tag+").")
    return

def onDisconnect(Connection):
    Domoticz.Log("Connection '"+Connection.Name+"' disconnected.")
    return

def onHeartbeat():
    global hasConnected, nextCommand, SerialConn
    if (SerialConn.Connected()):
        if (nextCommand != ""):
            Domoticz.Debug("Sending command: "+nextCommand)
            SerialConn.Send("<Command>\n  <Name>"+nextCommand+"</Name>\n</Command>\n")
    else:
        hasConnected = False
        SerialConn.Connect()
    return True

# RAVEn support functions
def getCurrentSummationKWh(xmltree):
  '''Returns a single float value for the SummationDelivered from a Summation response from RAVEn'''
  # Get the Current Summation (Meter Reading)
  fReading = float(int(xmltree.find('SummationDelivered').text,16))
  fResult = calculateRAVEnNumber(xmltree, fReading)
  return formatRAVEnDigits(xmltree, fResult)

def getInstantDemandKWh(xmltree):
  '''Returns a single float value for the Demand from an Instantaneous Demand response from RAVEn'''
  # Get the Instantaneous Demand
  fDemand = float(int(xmltree.find('Demand').text,16))
  fResult = calculateRAVEnNumber(xmltree, fDemand)
  return formatRAVEnDigits(xmltree, fResult)

def calculateRAVEnNumber(xmltree, value):
  '''Calculates a float value from RAVEn using Multiplier and Divisor in XML response'''
  # Get calculation parameters from XML - Multiplier, Divisor
  fDivisor = float(int(xmltree.find('Divisor').text,16))
  fMultiplier = float(int(xmltree.find('Multiplier').text,16))
  if (fMultiplier > 0 and fDivisor > 0):
    fResult = float( (value * fMultiplier) / fDivisor)
  elif (fMultiplier > 0):
    fResult = float(value * fMultiplier)
  else: # (Divisor > 0) or anything else
    fResult = float(value / fDivisor)
  return fResult

def formatRAVEnDigits(xmltree, value):
  '''Formats a float value according to DigitsRight, DigitsLeft and SuppressLeadingZero settings from RAVEn XML response'''
  # Get formatting parameters from XML - DigitsRight, DigitsLeft
  iDigitsRight = int(xmltree.find('DigitsRight').text,16)
  iDigitsLeft = int(xmltree.find('DigitsLeft').text,16)
  sResult = ("{:0%d.%df}" % (iDigitsLeft+iDigitsRight+1,iDigitsRight)).format(value)
  # Suppress Leading Zeros if specified in XML
  if xmltree.find('SuppressLeadingZero').text == 'Y':
    while sResult[0] == '0':
      sResult = sResult[1:]
    if sResult[0] == '.':
      sResult = '0' + sResult
  return sResult

# Generic helper functions
def LogMessage(Message):
    if Parameters["Mode6"] == "File":
        f = open(Parameters["HomeFolder"]+"plugin.log","a")
        f.write(Message+"\r\n")
        f.close()
    Domoticz.Debug(Message)

def DumpConfigToLog():
    for x in Parameters:
        if Parameters[x] != "":
            LogMessage( "'" + x + "':'" + str(Parameters[x]) + "'")
    LogMessage("Device count: " + str(len(Devices)))
    for x in Devices:
        LogMessage("Device:           " + str(x) + " - " + str(Devices[x]))
        LogMessage("Device ID:       '" + str(Devices[x].ID) + "'")
        LogMessage("Device Name:     '" + Devices[x].Name + "'")
        LogMessage("Device nValue:    " + str(Devices[x].nValue))
        LogMessage("Device sValue:   '" + Devices[x].sValue + "'")
        LogMessage("Device LastLevel: " + str(Devices[x].LastLevel))
    return

