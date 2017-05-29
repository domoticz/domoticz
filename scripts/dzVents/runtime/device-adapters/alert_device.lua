return {

	baseType = 'device',

	name = 'Voltage device adapter',

	matches = function (device)
		return (device.deviceSubType == 'Alert')
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		function device.updateAlertSensor(level, text)
			--[[ level can be
				domoticz.ALERTLEVEL_GREY
				domoticz.ALERTLEVEL_GREEN
				domoticz.ALERTLEVEL_YELLOW
				domoticz.ALERTLEVEL_ORANGE
				domoticz.ALERTLEVEL_RED
			]]
			device.update(level, text)
		end

	end

}