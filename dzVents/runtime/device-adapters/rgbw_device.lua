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
		adapterManager.addDummyMethod(device, 'setDiscoMode')

		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		if (device.deviceSubType == 'RGBWW') then
			function device.setKelvin(kelvin)
				local url
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

		function device.setDiscoMode(modeNum)
			if (type(modeNum) ~= 'number' or modeNum < 1 or modeNum > 9) then
				domoticz.log('Mode number needs to be a number from 1-9', utils.LOG_ERROR)
			end
			local url
			url = domoticz.settings['Domoticz url'] ..
					'/json.htm?type=command&param=discomodenum' .. tonumber(modeNum) .. '&idx=' .. device.id
			return domoticz.openURL(url)
		end

		function device.setRGB(r, g, b)
			if (not type(r) == 'number' or not type(b) == 'number' or not type(g) == number) then
				domoticz.log('RGB values need to be numbers from 0-255', utils.LOG_ERROR)
				return
			end

			if (r < 0 or r > 255 or b < 0 or b > 255 or g < 0 or g > 255) then
				domoticz.log('RGB values need to be numbers from 0-255', utils.LOG_ERROR)
				return
			end

			local h, s, b, isWhite = domoticz.utils.rgbToHSB(tonumber(r), tonumber(g), tonumber(b))
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
