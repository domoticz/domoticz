return {

	baseType = 'device',

	name = 'Smoke Detector device adapter',

	matches = function (device, adapterManager)
		local res = ( device.switchType == 'Smoke Detector')
		if not(res) then
			adapterManager.addDummyMethod(device, 'reset')
			adapterManager.addDummyMethod(device, 'activate')
		end	
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)
		if (device.switchType == 'Smoke Detector') then

			function device.activate()
				local TimedCommand = require('TimedCommand')
				return TimedCommand(domoticz, device.name, 'On', 'device', device.state)
			end

			function device.reset()
				local url
				url = domoticz.settings['Domoticz url'] ..
						"/json.htm?type=command&param=resetsecuritystatus&idx=" .. 
						device.id .. 
						"&switchcmd=Normal"
				return domoticz.openURL(url)
			end

		end
	end
}
