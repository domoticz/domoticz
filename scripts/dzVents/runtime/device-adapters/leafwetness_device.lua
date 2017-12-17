return {

	baseType = 'device',

	name = 'Leaf wetness device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceSubType == 'Leaf Wetness')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateWetness')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device.wetness = data.data._nValue

		function device.updateWetness(wetness)
			-- /json.htm?type=command&param=udevice&idx=14&nvalue=10&svalue=0
			local url = domoticz.settings['Domoticz url'] .. '/json.htm?type=command&param=udevice&idx=' .. device.id .. '&nvalue=' .. tonumber(wetness)
			domoticz.openURL(url)
		end

	end

}