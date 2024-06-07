"""
<plugin key="ManualSwitchesExample" name="Manual Switches Example" author="ping" version="1.0">
    <description>
        <h2>Manual Switches Example</h2><br/>
    </description>
    <params>
    </params>
</plugin>
"""

import Domoticz

def onCommand(Unit, Command, Level, sColor):
	Domoticz.Status("onCommand(" + str(Unit) + ", " + str(Command) + ", " + str(Level) + ", " + str(sColor) + ")")
	device=Devices[Unit]

	match device.SwitchType:
		case 0: #On/Off
			device.Update(nValue=1 if Command == "On" else 0, sValue = Command)
		case 1: #Doorbell
			device.Update(nValue=1 if Command == "On" else 0, sValue = Command)
		case 2: #Contact: Open: nValue = 1, Closed: nValue = 0
			device.Update(nValue=1 if Command == "Open" else 0, sValue = Command)
		case 3: #Blinds
			device.Update(nValue=1 if Command == "Open" else 0, sValue = Command)
		case 4: #X10 Siren
			device.Update(nValue=1 if Command == "On" else 0, sValue = Command)
		case 5: #Smoke Detector
			device.Update(nValue=1 if Command == "On" else 0, sValue = Command)
		case 6: 
			pass
		case 7: #Dimmer
			pass
		case 8: #Motion Sensor: Motion: nValue = 1, Off: nValue = 0
			device.Update(nValue=1 if Command == "Motion" else 0, sValue = Command)
		case 9: #Push On Button 	Push On
			device.Update(nValue=1, sValue = Command)
		case 10:#Push Off Button 	Push Off
			device.Update(nValue=0, sValue = Command)
		case 11:#Door Contact:  Open: nValue = 1, Closed: nValue = 0
			device.Update(nValue=1 if Command == "Open" else 0, sValue = Command)
		case 12:#Dusk Sensor
			device.Update(nValue=1 if Command == "On" else 0, sValue = Command)
		case 13:#Blinds Percentage
			pass
		case 14:#Venetian Blinds US
			pass
		case 15:#Venetian Blinds EU
			pass
		case 16:
			pass
		case 17:#Media Player
			pass
		case 18:#Selector
			pass
		case 19:#Door Lock
			pass
		case 20:#Door Lock Inverted
			pass
		case 21:#Blinds Percentage With Stop
			pass


def HasManualSwitchesSupport():
	return True

def GetManualSwitchesJsonConfiguration():
	return '''[
		{"idx":"1","name":"X10 Like Devices","parameters":[
			{"name":"housecode","display":"House Code","type":"housecode16"},
			{"name":"unitcode","display":"Unit Code","type":"unitcode16"}]
		},
		{"idx":"2","name":"Devices with other parameters","parameters":[
			{"name":"mybool","display":"My Bool","type":"bool"},
			{"name":"mystring","display":"My String","type":"string"}]
		}
	]'''

def AddManualSwitch(Name, SwitchType, Type, Parameters):
	"""Add a switch with custom parameters"""
	Domoticz.Status("AddManualSwitch(" + Name + ", " + str(SwitchType) + ", " + str(Type) + ", " + str(Parameters) + ")")
	if Type == 1:
		devID = str(Parameters['housecode']) + str(Parameters['unitcode'])
	elif Type == 2:
		devID = str(Parameters['mybool']) + str(Parameters['mystring'])
	else:
		return False
	currentUnit = 1
	while currentUnit in Devices:
		currentUnit += 1
	dev = Domoticz.Device(Name=Name, Unit=currentUnit, DeviceID=devID, Type=244, Subtype=73, Switchtype=SwitchType, Used=True)
	dev.Create()
	return True

def TestManualSwitch(SwitchType, Type, Parameters):
	"""Test a switch command with custom parameters (send On/Open depending of the switch type)"""
	Domoticz.Status("TestManualSwitch(" + str(SwitchType) + ", " + str(Type) + ", " + str(Parameters) + ")")
	return True
