#		   Awox SmartPlug Plugin
#
#		   Author:	 zaraki673, 2017
#
"""
<plugin key="AwoxSMP" name="Awox SmartPlug" author="zaraki673" version="1.0.0">
	<params>
		<param field="Address" label="MAC Address" width="150px" required="true"/>
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
import binascii
import struct
import lib.pySmartPlugSmpB16
from bluepy import btle

START_OF_MESSAGE = b'\x0f'
END_OF_MESSAGE = b'\xff\xff'
SMPstate = 0
SMPconso = 0

class BasePlugin:
	enabled = False
	pluginState = "Not Ready"
	sessionCookie = ""
	privateKey = b""
	socketOn = "FALSE"
	
	def __init__(self):
		return

	def onStart(self):
		global SMPstate, SMPconso
		if Parameters["Mode6"] == "Debug":
			Domoticz.Debugging(1)
		if (len(Devices) == 0):
			Domoticz.Device(Name="Status", Unit=1, Type=17, Switchtype=0).Create()
			Domoticz.Device(Name="Conso", Unit=2, TypeName="Usage").Create()
			Domoticz.Log("Devices created.")
		else:
			if (1 in Devices): SMPstate = Devices[1].nValue
			if (2 in Devices): SMPconso = Devices[2].nValue
		DumpConfigToLog()
		Domoticz.Log("Plugin is started.")
		Domoticz.Heartbeat(20)

	def onStop(self):
		Domoticz.Log("Plugin is stopping.")

	def onConnect(self, Status, Description):
		return

	def onMessage(self, Data, Status, Extra):
		return

	def onCommand(self, Unit, Command, Level, Hue):
		Domoticz.Debug("onCommand called for Unit " + str(Unit) + ": Parameter '" + str(Command) + "', Level: " + str(Level))
		Command = Command.strip()
		action, sep, params = Command.partition(' ')
		action = action.capitalize()
		if (action == 'On'):
			try:
				plug = SmartPlug(Parameters["Address"])
				plug.on()
				UpdateDevice(1,1,'On')
				plug.disconnect()
			except btle.BTLEException as err:
				Domoticz.Log('error when setting plug %s on (code %d)' % (Parameters["Address"], err.code))
		elif (action == 'Off'):
			try:
				plug = SmartPlug(Parameters["Address"])
				plug.off()
				UpdateDevice(1,0,'Off')
				plug.disconnect()
			except btle.BTLEException as err:
				Domoticz.Log('error when setting plug %s on (code %d)' % (Parameters["Address"], err.code))
		return True

	def onDisconnect(self):
		return

	def onHeartbeat(self):
		global SMPstate, SMPconso
		try:
			plug = SmartPlug(Parameters["Address"])
			(SMPstate, SMPconso) = plug.status_request()
			plug.disconnect()
			SMPstate = 'on' if SMPstate else 'off'
			Domoticz.Log('plug state = %s' % SMPstate)
			if (SMPstate == 'off'): UpdateDevice(1,0,'Off')
			else: UpdateDevice(1,1,'On')
			Domoticz.Log('plug power = %d W' % SMPconso)
			UpdateDevice(2,0,str(SMPconso))
		except btle.BTLEException as err:
			Domoticz.Log('error when requesting stat to plug %s (code %d)' % (Parameters["Address"], err.code))
		return True

	def SetSocketSettings(self, power):
		return

	def GetSocketSettings(self):
		return

	def genericPOST(self, commandName):
		return
 

global _plugin
_plugin = BasePlugin()

def onStart():
	global _plugin
	_plugin.onStart()

def onStop():
	global _plugin
	_plugin.onStop()

def onConnect(Status, Description):
	global _plugin
	_plugin.onConnect(Status, Description)

def onMessage(Data, Status, Extra):
	global _plugin
	_plugin.onMessage(Data, Status, Extra)

def onCommand(Unit, Command, Level, Hue):
	global _plugin
	_plugin.onCommand(Unit, Command, Level, Hue)

def onDisconnect():
	global _plugin
	_plugin.onDisconnect()

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
		Domoticz.Debug("Device:		   " + str(x) + " - " + str(Devices[x]))
		Domoticz.Debug("Device ID:	   '" + str(Devices[x].ID) + "'")
		Domoticz.Debug("Device Name:	 '" + Devices[x].Name + "'")
		Domoticz.Debug("Device nValue:	" + str(Devices[x].nValue))
		Domoticz.Debug("Device sValue:   '" + Devices[x].sValue + "'")
		Domoticz.Debug("Device LastLevel: " + str(Devices[x].LastLevel))
	return	

def UpdateDevice(Unit, nValue, sValue):
	# Make sure that the Domoticz device still exists (they can be deleted) before updating it 
	if (Unit in Devices):
		if (Devices[Unit].nValue != nValue) or (Devices[Unit].sValue != sValue):
			Devices[Unit].Update(nValue, str(sValue))
			Domoticz.Log("Update "+str(nValue)+":'"+str(sValue)+"' ("+Devices[Unit].Name+")")
	return

class SmartPlug(btle.Peripheral):
	def __init__(self, addr):
		btle.Peripheral.__init__(self, addr)
		self.delegate = NotificationDelegate()
		self.setDelegate(self.delegate)
		self.plug_svc = self.getServiceByUUID('0000fff0-0000-1000-8000-00805f9b34fb')
		self.plug_cmd_ch = self.plug_svc.getCharacteristics('0000fff3-0000-1000-8000-00805f9b34fb')[0]

	def on(self):
		self.delegate.chg_is_ok = False
		self.plug_cmd_ch.write(self.get_buffer(binascii.unhexlify('0300010000')))
		self.wait_data(0.5)
		return self.delegate.chg_is_ok

	def off(self):
		self.delegate.chg_is_ok = False
		self.plug_cmd_ch.write(self.get_buffer(binascii.unhexlify('0300000000')))
		self.wait_data(0.5)
		return self.delegate.chg_is_ok

	def status_request(self):
		self.plug_cmd_ch.write(self.get_buffer(binascii.unhexlify('04000000')))
		self.wait_data(2.0)
		return self.delegate.state, self.delegate.power

	def program_request(self):
		self.plug_cmd_ch.write(self.get_buffer(binascii.unhexlify('07000000')))
		self.wait_data(2.0)
		return self.delegate.programs

	def calculate_checksum(self, message):
		return (sum(bytearray(message)) + 1) & 0xff

	def get_buffer(self, message):
		return START_OF_MESSAGE + struct.pack("b",len(message) + 1) + message + struct.pack("b",self.calculate_checksum(message)) + END_OF_MESSAGE 

	def wait_data(self, timeout):
		self.delegate.need_data = True
		while self.delegate.need_data and self.waitForNotifications(timeout):
			pass

class NotificationDelegate(btle.DefaultDelegate):
	def __init__(self):
		btle.DefaultDelegate.__init__(self)
		self.state = False
		self.power = 0
		self.chg_is_ok = False
		self.programs = []
		self._buffer = b''
		self.need_data = True

	def handleNotification(self, cHandle, data):
		#not sure 0x0f indicate begin of buffer but
		if data[:1] == START_OF_MESSAGE:
			self._buffer = data
		else:
			self._buffer = self._buffer + data
		if self._buffer[-2:] == END_OF_MESSAGE:
			self.handle_data(self._buffer)
			self._buffer = b''
			self.need_data = False 

	def handle_data(self, bytes_data):
		# it's a state change confirm notification ?
		if bytes_data[0:3] == b'\x0f\x04\x03':
			self.chg_is_ok = True
		# it's a state/power notification ?
		if bytes_data[0:3] == b'\x0f\x0f\x04':
			(state, dummy, power) = struct.unpack_from(">?BI", bytes_data, offset=4)
			self.state = state
			self.power = power / 1000
		# it's a 0x0a notif ?
		if bytes_data[0:3] == b'\x0f\x33\x0a':
			print ("0A notif %s" % bytes_data)
		# it's a programs notif ?
		if bytes_data[0:3] == b'\x0f\x71\x07' :
			program_offset = 4
			self.programs = []
			while program_offset + 21 < len(bytes_data):
				(present, name, flags, start_hour, start_minute, end_hour, end_minute) = struct.unpack_from(">?16sbbbbb", bytes_data, program_offset)
				#TODO interpret flags (day of program ?)
				if present:
					self.programs.append({ "name" : name.decode('iso-8859-1').strip('\0'), "flags":flags, "start":"{0:02d}:{1:02d}".format(start_hour, start_minute), "end":"{0:02d}:{1:02d}".format(end_hour, end_minute)})
				program_offset += 22

