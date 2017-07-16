return {

	baseType = 'device',

	name = 'Alert sensor  adapter',

	matches = function (device, adapterManager)
		local res =  (device.deviceSubType == 'Alert')

		if (not res) then
			adapterManager.addDummyMethod(device, 'updateAlertSensor')
		end

		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device.color = data.data._nValue

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