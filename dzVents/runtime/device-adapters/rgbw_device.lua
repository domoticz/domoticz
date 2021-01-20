local TimedCommand = require('TimedCommand')

return {

	baseType = 'device',

	name = 'RGB(W) device adapter',

	matches = function (device, adapterManager)
		local res = device.deviceType == 'Color Switch'
		adapterManager.addDummyMethod(device, 'setKelvin')
		adapterManager.addDummyMethod(device, 'setWhiteMode')
		adapterManager.addDummyMethod(device, 'increaseBrightness')
		adapterManager.addDummyMethod(device, 'decreaseBrightness')
		adapterManager.addDummyMethod(device, 'setNightMode')
		adapterManager.addDummyMethod(device, 'setRGB')
		adapterManager.addDummyMethod(device, 'setHex')
		adapterManager.addDummyMethod(device, 'setHue')
		adapterManager.addDummyMethod(device, 'setColor')
		adapterManager.addDummyMethod(device, 'setColorBrightness')
		adapterManager.addDummyMethod(device, 'setHue')
		adapterManager.addDummyMethod(device, 'getColor')
		adapterManager.addDummyMethod(device, 'setDiscoMode')

		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		local function methodNotAvailableMessage(device, methodName)
			domoticz.log('Method ' .. methodName .. ' is not available for device ' .. ' "' .. device.name .. '"' , utils.LOG_ERROR)
			domoticz.log('(id=' .. device.id .. ', type=' .. device.deviceType .. ', subtype=' .. device.deviceSubType .. ', hardwareType=' .. device.hardwareType .. ')', utils.LOG_ERROR)
			domoticz.log('If you believe this is not correct, please report on the forum.', utils.LOG_ERROR)
		end

		if (device.deviceSubType == 'RGBWW') or (device.deviceSubType == 'WW')  then
			function device.setKelvin(kelvin)
				local url
				url = domoticz.settings['Domoticz url'] ..
						'/json.htm?param=setkelvinlevel&type=command&idx=' .. device.id .. '&kelvin=' .. tonumber(kelvin)
				return domoticz.openURL(url)
			end
		end

		function device.setWhiteMode()
			local url
			url = domoticz.settings['Domoticz url'] ..
					'/json.htm?param=whitelight&type=command&idx=' .. device.id
			return domoticz.openURL(url)
		end

		function device.increaseBrightness()
			-- API will be removed; kept here until then to get user reports (if any)
			if false and device.hardwareType and device.hardwareType == 'YeeLight LED' then 
				local url
				url = domoticz.settings['Domoticz url'] ..
						'/json.htm?param=brightnessup&type=command&idx=' .. device.id
				return domoticz.openURL(url)
			else
				methodNotAvailableMessage(device, 'increaseBrightness')
			end
		end

		function device.decreaseBrightness()
            -- API will be removed; kept here until then to get user reports (if any)
			if false and device.hardwareType and device.hardwareType == 'YeeLight LED' then 
				local url
				url = domoticz.settings['Domoticz url'] ..
						'/json.htm?param=brightnessdown&type=command&idx=' .. device.id
				return domoticz.openURL(url)
			else
				methodNotAvailableMessage(device, 'decreaseBrightness')
			end
		end

		function device.setNightMode()
			local url
			url = domoticz.settings['Domoticz url'] ..
					'/json.htm?param=nightlight&type=command&idx=' .. device.id
			return domoticz.openURL(url)
		end

		function device.setDiscoMode(modeNum)
			if false then  -- API will be removed; kept here until then to get user reports (if any)
                if (type(modeNum) ~= 'number' or modeNum < 1 or modeNum > 9) then
                    domoticz.log('Mode number needs to be a number from 1-9', utils.LOG_ERROR)
                end
                local url
                url = domoticz.settings['Domoticz url'] ..
                        '/json.htm?param=discomodenum' .. tonumber(modeNum) .. '&type=command&idx=' .. device.id
                return domoticz.openURL(url)
            else
				methodNotAvailableMessage(device, 'setDiscoMode')
			end
		end

		local function inRange(value, low, high)
			return (value >= low and value <= high )
		end

		local function validRGB(r, g, b)
			return  (
				type(r) == 'number' and
				type(b) == 'number' and
				type(g) == 'number' and
				inRange(r, 0, 255) and
				inRange(b, 0, 255) and
				inRange(g, 0, 255)
			)
		end

		local function RGBError()
			 domoticz.log('RGB values need to be numbers from 0-255', utils.LOG_ERROR)
		end

		function device.setRGB(r, g, b)
			if not(validRGB(r, g, b)) then RGBError() return false end
			local h, s, b, isWhite = domoticz.utils.rgbToHSB(r, g, b)
			local url = domoticz.settings['Domoticz url'] ..
						'/json.htm?param=setcolbrightnessvalue&type=command'  ..
						'&idx=' .. device.id ..
						'&hue=' .. math.floor(tonumber(h) + 0.5) .. 
						'&brightness=' .. math.floor(tonumber(b) + 0.5) ..
						'&iswhite=' .. tostring(isWhite)
			return domoticz.openURL(url)
		end

		function device.setColor(r, g, b, br, cw, ww, m, t)
			if not(validRGB(r, g, b)) then RGBError() return false end
			if br and not(inRange(br, 0, 100)) then
				domoticz.log('Brightness value need to be number from 0-100', utils.LOG_ERROR)
				return false
			end
			if m == nil then m = 3 end
			local colors = domoticz.utils.fromJSON(device.color,{ t=0, cw=0, ww=0})
			local brightness = br or device.level or 100
			if cw == nil then cw = colors.cw end
			if ww == nil then ww = colors.ww end
			if t  == nil then t = colors.t end
			local url = domoticz.settings['Domoticz url'] ..
						'/json.htm?type=command&param=setcolbrightnessvalue' ..
						'&idx=' .. device.id   ..
						'&brightness=' .. brightness ..
						'&color={"m":' .. m ..
						',"t":' .. t ..
						',"cw":' .. cw ..
						',"ww":' .. ww ..
						',"r":' .. r ..
						',"g":' .. g ..
						',"b":' .. b .. '}'
			return domoticz.openURL(url)
		end

	   function device.setColorBrightness(r, g, b, br, cw, ww, m, t)
			return device.setColor(r, g, b, br, cw, ww, m, t)
		end

		function device.setHex(r,g,b)
			local _ , _ , brightness = domoticz.utils.rgbToHSB(r, g, b)
			local hex = string.format("%02x",r) .. string.format("%02x", g) .. string.format("%02x", b)
			local url = domoticz.settings['Domoticz url'] .. '/json.htm?type=command&param=setcolbrightnessvalue' ..
				'&idx=' .. device.id ..
				'&brightness=' .. brightness ..
				'&hex=' .. hex ..
				'&iswhite=false'
			return domoticz.openURL(url)
		end

		function device.setHue(hue,brightness,isWhite)
			if not(brightness) or not(inRange(brightness, 0, 100)) then
				domoticz.log('Brightness value need to be number from 0-100', utils.LOG_ERROR)
				return false
			end
			if not(hue) or not(inRange(hue, 0, 360)) then
				domoticz.log('hue value need to be number from 0-360', utils.LOG_ERROR)
				return false
			end
			if isWhite and type(isWhite) ~= "boolean" then
				domoticz.log('isWhite need to be of boolean type', utils.LOG_ERROR)
				return false
			end
			if not(isWhite) then isWhite = false end
				local url = domoticz.settings['Domoticz url'] .. '/json.htm?type=command&param=setcolbrightnessvalue' ..
					'&idx=' .. device.id ..
					'&brightness=' .. brightness ..
					'&hue=' .. hue ..
					'&iswhite=' .. tostring(isWhite)
				return domoticz.openURL(url)
		end

		function device.getColor()
			if not(device.color) or device.color == "" then
				domoticz.log('Color field not set for this device', utils.LOG_ERROR)
				return nil
			end
			local ct = domoticz.utils.fromJSON(device.color, {})
			ct.hue, ct.saturation, ct.value, ct.isWhite = domoticz.utils.rgbToHSB(ct.r, ct.g, ct.b)
			ct.red  = ct.r
			ct.blue = ct.b
			ct.green = ct.g
			ct["warm white"] = ct.ww
			ct["cold white"] = ct.cw
			ct.temperature = ct.t
			ct.mode = ct.m
			ct.br = device.level
			ct.brightness = ct.value
			return (ct)
		end
	end
}