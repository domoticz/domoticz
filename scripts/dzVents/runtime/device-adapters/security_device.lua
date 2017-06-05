return {

	baseType = 'device',

	name = 'Switch device adapter',

	matches = function (device)
		return (device.deviceType == 'Security' and device.deviceSubType == 'Security Panel')
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		function device.disarm()
			domoticz.sendCommand(device.name, 'Disarm')
		end

		function device.armAway()
			domoticz.sendCommand(device.name, 'Arm Away')
		end

		function device.armHome()
			domoticz.sendCommand(device.name, 'Arm Home')
		end

	end

}