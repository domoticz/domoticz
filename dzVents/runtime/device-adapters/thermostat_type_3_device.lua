return {

	baseType = 'device',

	name = 'Thermostat type 3 device adapter',

	matches = function (device, adapterManager)
		local res = device.deviceSubType and device.deviceSubType:find('Mertik') and device.deviceType == 'Thermostat 3'
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateMode')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		local modes = {'Off','On','Up','Down','Run Up','Run Down','Stop'}

		device.mode = device.nValue
		device.modeString = modes[ device.nValue + 1 ]
		device.modes = table.concat(modes,',')

		device.sValue = '' -- sValue is reported wrong it's empty

		function device.updateMode(mode)
			local mode = mode:gsub(' ','%%20')
			local url = domoticz.settings['Domoticz url'] .. '/json.htm?type=command&param=switchlight&idx=' .. device.id  ..'&switchcmd=' .. mode
			return domoticz.openURL(url)
		end

	end

}
