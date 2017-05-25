local genericAdapter = require('generic_device')

local deviceAdapters = {
	'airquality_device',
	'barometer_device',
	'electric_usage_device',
	'evohome_device',
	'humidity_device',
	'kwh_device',
	'lux_device',
	'opentherm_gateway_device',
	'p1_smartmeter_device',
	'rain_device',
	'thermostat_setpoint_device',
	'temperature_device',
	'temperature_humidity_device',
	'temperature_humidity_barometer_device',
	'text_device',
	'uv_device',
	'wind_device',
	'zone_heating_device',
	'kodi_device'
}

local _utils = require('Utils')

function string:split(sep)
	local sep, fields = sep or ":", {}
	local pattern = string.format("([^%s]+)", sep)
	self:gsub(pattern, function(c) fields[#fields + 1] = c end)
	return fields
end

local function DeviceAdapters(utils)

	if (utils == nil) then
		utils = _utils
	end

	local self = {

		name = 'Generic device adapter',

		getDeviceAdapters = function(device)
			-- find a matching adapters

			local adapters = {}

			for i, adapterName in pairs(deviceAdapters) do

				-- do a safe call and catch possible errors
				ok, adapter = pcall(require, adapterName)
				if (not ok) then
					utils.log(adapter, utils.LOG_ERROR)
				else
					local matches = adapter.matches(device)
					if (matches) then
						table.insert(adapters, adapter)
					end
				end
			end

			return adapters
		end,

		deviceAdapters = deviceAdapters,

		genericAdapter = genericAdapter,

		parseFormatted = function(sValue, radixSeparator)

			local splitted = string.split(sValue, ' ')

			local sV = splitted[1]
			local unit = splitted[2]

			-- normalize radix to .

			if (sV ~= nil) then
				sV = string.gsub(sV, '%' .. radixSeparator, '.')
			end

			local value = sV ~= nil and tonumber(sV) or 0

			return {
				['value'] = value,
				['unit'] = unit ~= nil and unit or ''
			}

		end

	}

	return self

end

return DeviceAdapters
