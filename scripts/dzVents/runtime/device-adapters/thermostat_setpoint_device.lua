return {

	baseType = 'device',

	name = 'Thermostat setpoint device adapter',

	matches = function (device)
		return device.deviceSubType == 'SetPoint'
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device['SetPoint'] = device.rawData[1] or 0

		function device.updateSetPoint(setPoint)
			-- send the command using openURL otherwise, due to a bug in Domoticz, you will get a timeout on the script
			local url = domoticz.settings['Domoticz url']..
					'/json.htm?type=command&param=udevice&idx=' .. device.id .. '&nvalue=0&svalue=' .. setPoint
			utils.log('Setting setpoint using openURL ' .. url, utils.LOG_DEBUG)
			domoticz.openURL(url)
		end

	end

}

