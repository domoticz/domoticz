return {

	baseType = 'device',

	name = 'RGB(W) device adapter',

	matches = function (device, adapterManager)
		local res = device.deviceType == 'Lighting Limitless/Applamp'
		adapterManager.addDummyMethod(device, 'setKelvin')
		adapterManager.addDummyMethod(device, 'setWhiteMode')
		adapterManager.addDummyMethod(device, 'increaseBrightness')
		adapterManager.addDummyMethod(device, 'decreaseBrightness')
		adapterManager.addDummyMethod(device, 'setNightMode')
		adapterManager.addDummyMethod(device, 'setRGB')

		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		if (device.deviceSubType == 'RGBWW') then
			function device.setKelvin(kelvin)
				local url
				if (not device.active) then
					device.switchOn()
				end
				url = domoticz.settings['Domoticz url'] ..
						'/json.htm?type=command&param=setkelvinlevel&idx=' .. device.id .. '&kelvin=' .. tonumber(kelvin)
				return domoticz.openURL(url)
			end
		end

		function device.setWhiteMode()
			local url
			url = domoticz.settings['Domoticz url'] ..
					'/json.htm?type=command&param=whitelight&idx=' .. device.id
			return domoticz.openURL(url)
		end

		function device.increaseBrightness()
			local url
			url = domoticz.settings['Domoticz url'] ..
					'/json.htm?type=command&param=brightnessup&idx=' .. device.id
			return domoticz.openURL(url)
		end

		function device.decreaseBrightness()
			local url
			url = domoticz.settings['Domoticz url'] ..
					'/json.htm?type=command&param=brightnessdown&idx=' .. device.id
			return domoticz.openURL(url)
		end

		function device.setNightMode()
			local url
			url = domoticz.settings['Domoticz url'] ..
					'/json.htm?type=command&param=nightlight&idx=' .. device.id
			return domoticz.openURL(url)
		end

		function device.setRGB(r, g, b)
			local h, s, b, isWhite = domoticz.utils.rgbToHSB(r, g, b)
			url = domoticz.settings['Domoticz url'] ..
					'/json.htm?type=command&param=setcolbrightnessvalue&idx=' ..
					device.id ..
					'&hue=' .. tostring(h) ..
					'&brightness=' .. tostring(b) ..
					'&iswhite=' ..  tostring(isWhite)
			return domoticz.openURL(url)
		end
	end
}
