return {

	baseType = 'device',

	name = 'Evohome device adapter',

	matches = function (device)
		return (device.hardwareTypeVal == 39 and device.deviceSubType == 'Zone')
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device['SetPoint'] = device.rawData[1] or 0

		function device.updateSetPoint(setPoint, mode, untilDate)
			local url = domoticz.settings['Domoticz url'] ..
					'/json.htm?type=setused&idx=' .. device.id .. '&setpoint=' .. setPoint .. '&mode=' .. tostring(mode) .. '&used=true'

			if (untilDate) then
				url = url .. '&until=' .. tostring(untilDate)
			end

			utils.log('Setting setpoint using openURL ' .. url, utils.LOG_DEBUG)
			domoticz.openURL(url)
		end

	end

}