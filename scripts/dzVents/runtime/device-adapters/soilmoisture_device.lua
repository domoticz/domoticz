return {

	baseType = 'device',

	name = 'Soil Moisture device',

	matches = function (device, adapterManager)
		local res = (device.deviceSubType == "Soil Moisture")
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateSoilMoisture')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device.moisture = data.data._nValue --cB centibar

		device['updateSoilMoisture'] = function (moisture)
			local url = domoticz.settings['Domoticz url'] .. '/json.htm?type=command&param=udevice&idx=' .. device.id .. '&nvalue=' .. tonumber(moisture)
			domoticz.openURL(url)
		end

	end

}