#           RAVEn Plugin
#
#           Author:     Dnpwwo, 2016
#
#
#   Plugin parameter definition below will be parsed during startup and copied into Manifest.xml, this will then drive the user interface in the Hardware web page
#
"""
<plugin key="RAVEn" name="RAVEn Zigbee energy monitor" author="dnpwwo" version="1.0.0" wikilink="http://www.domoticz.com/wiki/plugins/RAVEn.html" externallink="https://rainforestautomation.com/rfa-z106-raven/">
    <params>
        <param field="SerialPort" label="Serial Port" width="150px" required="true" default="/dev/ttyRAVEn"/>
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
import xml.etree.ElementTree as ET

demandFreq=30       # seconds between demand events
summaryFreq=300     # seconds between summary updates
fScale = demandFreq / 3600.0
summation = 0.0
connectStatus = "Disconnected"

isConnected = False

def onStart():
    if Parameters["Mode6"] == "Debug":
        Domoticz.Debugging(1)
    if (len(Devices) == 0):
        Domoticz.Device(Name="Usage", Unit=1, Type=243, Subtype=29, Switchtype=0, Image=0, Options="").Create()
        Domoticz.Device("Total", 2, 113).Create()
        Domoticz.Log("Devices created.")
    Domoticz.Log("Plugin has " + str(len(Devices)) + " devices associated with it.")
    DumpConfigToLog()
#    Domoticz.Heartbeat(900)
    Domoticz.Transport("Serial", Parameters["SerialPort"], 115200)
    Domoticz.Protocol("XML")
    Domoticz.Connect()
    return

def onConnect(Status, Description):
    global isConnected
    if (Status == 0):
        isConnected = True
        Domoticz.Log("Connected successfully to: "+Parameters["SerialPort"])
        Domoticz.Send("<Command>\n  <Name>get_device_info</Name>\n</Command>")
    else:
        Domoticz.Log("Failed to connect ("+str(Status)+") to: "+Parameters["SerialPort"])
        Domoticz.Debug("Failed to connect ("+str(Status)+") to: "+Parameters["SerialPort"]+" with error: "+Description)
    return True

def onMessage(Data):
    global connectStatus, fScale, summation
    Domoticz.Debug(Data)
    xmltree = ET.fromstring(Data)
    if xmltree.tag == 'DeviceInfo':
        Domoticz.Log( "Manufacturer: %s, Device ID: %s, Install Code: %s" % (xmltree.find('Manufacturer').text, xmltree.find('DeviceMacId').text, xmltree.find('InstallCode').text) )
        Domoticz.Log( "Hardware: Version %s, Firmware Version: %s, Model: %s" % (xmltree.find('HWVersion').text, xmltree.find('FWVersion').text, xmltree.find('ModelId').text) )
        Domoticz.Send("<Command>\n  <Name>get_network_info</Name>\n</Command>\n")
    elif xmltree.tag == 'NetworkInfo':
        Domoticz.Debug( "NetworkInfo response, Status = '%s' - %s, Link Strength = %d" % (xmltree.find('Status').text, xmltree.find('Description').text, int(xmltree.find('LinkStrength').text,16)))
        Domoticz.Send("<Command>\n  <Name>get_connection_status</Name>\n</Command>\n")
    elif xmltree.tag == 'ConnectionStatus':
        if connectStatus != 'Connected':
            Domoticz.Send("<Command>\n  <Name>get_meter_list</Name>\n</Command>\n")
        connectStatus = xmltree.find('Status').text
        Domoticz.Log( "MeterMacId: %s, Connection Status, Status = '%s' - %s, Link Strength = %d" % (xmltree.find('MeterMacId').text, xmltree.find('Status').text, xmltree.find('Description').text, int(xmltree.find('LinkStrength').text,16)))
    elif xmltree.tag == 'MeterList':
        for meter in xmltree.iter('MeterMacId'):
            Domoticz.Debug( "MeterMacId: %s, MeterList response" % meter.text)
            Domoticz.Send("<Command>\n  <Name>get_meter_info</Name>\n  <MeterMacId>"+meter.text+"</MeterMacId>\n</Command>\n")
    elif xmltree.tag == 'MeterInfo':
        Domoticz.Debug( "MeterMacId: %s, MeterInfo response, Enabled = %s" % (xmltree.find('MeterMacId').text, xmltree.find('Enabled').text))
        Domoticz.Send("<Command>\n  <Name>get_schedule</Name>\n  <MeterMacId>"+xmltree.find('MeterMacId').text+"</MeterMacId>\n</Command>\n")
    elif xmltree.tag == 'ScheduleInfo':
        iFreq = int(xmltree.find('Frequency').text,16)
        Domoticz.Debug( "MeterMacId: %s, ScheduleInfo response: Type '%s', Frequency %d, Enabled %s" % (xmltree.find('MeterMacId').text, xmltree.find('Event').text, iFreq, xmltree.find('Enabled').text))
        if (xmltree.find('Event').text == 'demand') and (iFreq != demandFreq):
            Domoticz.Debug( "MeterMacId: %s, Setting 'demand' schedule to: Frequency %d" % (xmltree.find('MeterMacId').text, demandFreq))
            Domoticz.Send("<Command>\n  <Name>set_schedule</Name>\n  <MeterMacId>"+xmltree.find('MeterMacId').text+"</MeterMacId>\n <Event>demand</Event>\n  <Frequency>" + str(hex(demandFreq)) + "</Frequency>\n  <Enabled>Y</Enabled>\n</Command>\n")
        if (xmltree.find('Event').text == 'summation') and (iFreq != summaryFreq):
            Domoticz.Debug( "MeterMacId: %s, Setting 'summation' schedule to: Frequency %d" % (xmltree.find('MeterMacId').text, summaryFreq))
            Domoticz.Send("<Command>\n  <Name>set_schedule</Name>\n  <MeterMacId>"+xmltree.find('MeterMacId').text+"</MeterMacId>\n  <Event>summation</Event>\n  <Frequency>" + str(hex(summaryFreq)) + "</Frequency>\n  <Enabled>Y</Enabled>\n</Command>\n")
        if (xmltree.find('Event').text == 'summation'):
            Domoticz.Send("<Command>\n  <Name>get_current_summation_delivered</Name>\n  <MeterMacId>"+xmltree.find('MeterMacId').text+"</MeterMacId>\n  <Refresh>Y</Refresh>\n</Command>\n")
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

def onDisconnect():
    global isConnected
    isConnected = False
    Domoticz.Log("RAVEn Disconnected")
    return

def onHeartbeat():
    global isConnected
    if (isConnected != True):
        Domoticz.Connect()
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

