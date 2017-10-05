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
			return device.update(tostring(moisture), 0)
		end

	end

}