local genericAdapter = require('generic_device')

local deviceAdapters = {
	'airquality_device',
	'alert_device',
	'ampere_1_phase_device',
	'ampere_3_phase_device',
	'barometer_device',
	'counter_device',
	'custom_sensor_device',
	'distance_device',
	'electric_usage_device',
	'evohome_device',
	'gas_device',
	'group_device',
	'humidity_device',
	'kwh_device',
	'leafwetness_device',
	'lux_device',
	'opentherm_gateway_device',
	'p1_smartmeter_device',
	'percentage_device',
	'pressure_device',
	'rain_device',
	'rgbw_device',
	'scaleweight_device',
	'scene_device',
	'security_device',
	'solar_radiation_device',
	'soilmoisture_device',
	'soundlevel_device',
	'switch_device',
	'thermostat_setpoint_device',
	'temperature_device',
	'temperature_barometer_device',
	'temperature_humidity_device',
	'temperature_humidity_barometer_device',
	'text_device',
	'uv_device',
	'visibility_device',
	'voltage_device',
	'waterflow_device',
	'wind_device',
	'zone_heating_device',
	'zwave_thermostat_mode_device',
	'kodi_device'
}

local utils = require('Utils')

function string:split(sep)
	local sep, fields = sep or ":", {}
	local pattern = string.format("([^%s]+)", sep)
	self:gsub(pattern, function(c) fields[#fields + 1] = c end)
	return fields
end

local function DeviceAdapters(dummyLogger)


	local self = {}


	self.name = 'Adapter manager'

	function self.getDeviceAdapters(device)
		-- find a matching adapters

		local adapters = {}

		for i, adapterName in pairs(deviceAdapters) do

			-- do a safe call and catch possible errors
			ok, adapter = pcall(require, adapterName)
			if (not ok) then
				utils.log(adapter, utils.LOG_ERROR)
			else
				if (adapter.baseType == device.baseType) then
					local matches = adapter.matches(device, self)
					if (matches) then
						utils.log('Device-adapter found for ' .. device.name .. ': ' .. adapter.name, utils.LOG_DEBUG)
						table.insert(adapters, adapter)
					end
				end
			end
		end

		return adapters
	end

	self.genericAdapter = genericAdapter
	self.deviceAdapters = deviceAdapters

	function self.parseFormatted (sValue, radixSeparator)

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

	function self.getDummyMethod(device, name)
		if (dummyLogger ~= nil) then
			dummyLogger(device, name)
		end

		return function()

			utils.log('Method ' ..
					name ..
					' is not available for device "' ..
					device.name ..
					'" (deviceType=' .. device.deviceType .. ', deviceSubType=' .. device.deviceSubType .. '). ' ..
					'If you believe this is not correct, please report.',
				utils.LOG_ERROR)
		end
	end


	function self.addDummyMethod(device, name)
		if (device[name] == nil) then
			device[name] = self.getDummyMethod(device, name)
		end
	end

	function self.round(num, numDecimalPlaces)
		if (num == nil) then
			--print(debug.traceback())
			num = 0
		end
		local mult = 10 ^ (numDecimalPlaces or 0)
		return math.floor(num * mult + 0.5) / mult
	end

	self.states = {
		on = { b = true, inv = 'Off' },
		open = { b = true, inv = 'Off' },
		['group on'] = { b = true },
		panic = { b = true, inv = 'Off' },
		normal = { b = true, inv = 'Alarm' },
		alarm = { b = true, inv = 'Normal' },
		chime = { b = true },
		video = { b = true },
		audio = { b = true },
		photo = { b = true },
		playing = { b = true, inv = 'Pause' },
		motion = { b = true },
		off = { b = false, inv = 'On' },
		closed = { b = false, inv = 'On' },
		['group off'] = { b = false },
		['panic end'] = { b = false },
		['no motion'] = { b = false, inv = 'Off' },
		stop = { b = false, inv = 'Open' },
		stopped = { b = false },
		paused = { b = false, inv = 'Play' },
		['all on'] = { b = true, inv = 'All Off' },
		['all off'] = { b = false, inv = 'All On' },
	}

	return self

end

return DeviceAdapters
