local TimedCommand = require('TimedCommand')

return {

	baseType = 'device',

	name = 'Evohome device adapter',

	matches = function (device, adapterManager)
		local res = (
			device.hardwareTypeValue == 39 or
			device.hardwareTypeValue == 40 or
			device.hardwareTypeValue == 106 or
			device.hardwareTypeValue == 75
		)

		if (not res) then
			adapterManager.addDummyMethod(device, 'updateSetPoint')
			adapterManager.addDummyMethod(device, 'setHotWater')
		end

		return res

	end,

	process = function (device, data, domoticz, utils, adapterManager)

		if (device.deviceSubType == "Hot Water" ) then

			if device.rawData[2] == "On" then device.state = "On" else device.state = "Off" end
			device.mode = tostring(device.rawData[3] or "n/a")
			device.untilDate = tostring(device.rawData[4] or "n/a")

			function device.setHotWater(state, mode, untilDate)
				 if mode == 'TemporaryOverride' and untilDate then
					mode = mode .. "&until=" .. untilDate
				 end
				local url = domoticz.settings['Domoticz url'] ..
					"/json.htm?type=setused&idx=" .. device.id ..
					"&setpoint=&state=" .. state ..
					"&mode=" .. mode ..
					"&used=true"
				return domoticz.openURL(url)
			end
		else

			device.setPoint = tonumber(device.rawData[1] or 0)
			device.mode = tostring(device.rawData[3])
			device.untilDate = tostring(device.rawData[4] or "n/a")

			function device.updateSetPoint(setPoint, mode, untilDate)
				return TimedCommand(domoticz,
					'SetSetPoint:' .. tostring(device.id),
					tostring(setPoint) .. '#' ..
					tostring(mode) .. '#' ..
					tostring(untilDate) , 'setpoint')
			end
		end
	end
}
