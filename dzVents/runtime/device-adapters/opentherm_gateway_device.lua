return {

	baseType = 'device',

	name = 'OpenTherm gateway device adapter',

	matches = function (device, adapterManager)
		local res = (device.hardwareTypeValue == 20 and device.deviceSubType == 'SetPoint')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateSetPoint')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device['setPoint'] = device.rawData[1] or 0

		function device.updateSetPoint(setPoint)
			-- send the command using openURL otherwise, due to a bug in Domoticz, you will get a timeout on the script
			local url = domoticz.settings['Domoticz url'] ..
					'/json.htm?param=udevice&type=command&idx=' .. device.id .. '&nvalue=0&svalue=' .. setPoint
			return domoticz.openURL(url)
		end

	end

}
