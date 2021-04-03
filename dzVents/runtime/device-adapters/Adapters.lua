local genericAdapter = require('generic_device')
local utils = require('Utils')

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
	'generic_device',
	'group_device',
	'hardware_device',
	'humidity_device',
	'kodi_device',
	'kwh_device',
	'leafwetness_device',
	'logitech_media_server_device',
	'lux_device',
	'onkyo_device',
	'opentherm_gateway_device',
	'p1_smartmeter_device',
	'percentage_device',
	'pressure_device',
	'rain_device',
	'rgbw_device',
	'scaleweight_device',
	'scene_device',
	'security_device',
	'smoke_detector_device',
	'soilmoisture_device',
	'solar_radiation_device',
	'soundlevel_device',
	'switch_device',
	'temperature_barometer_device',
	'temperature_device',
	'temperature_humidity_barometer_device',
	'temperature_humidity_device',
	'text_device',
	'thermostat_operating_state_device',
	'thermostat_setpoint_device',
	'thermostat_type_3_device',
	'uv_device',
	'visibility_device',
	'voltage_device',
	'waterflow_device',
	'wind_device',
	'youless_device',
	'zone_heating_device',
	'zwave_mode_type_device',
}

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

		local splitted = utils.stringSplit(sValue, ' ')

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

	self.states = {
		['alarm'] = { b = true, inv = 'Normal' },
		['all off'] = { b = false, inv = 'All On' },
		['all on'] = { b = true, inv = 'All Off' },
		['audio'] = { b = true },
		['chime'] = { b = true },
		['closed'] = { b = false, inv = 'On' },
		['cooling'] = { b = true, inv = 'idle' },
		['detected'] = { b = true, inv = 'Off'},
		['group off'] = { b = false },
		['group on'] = { b = true },
		['heating'] = { b = true, inv = 'idle' },
		['idle'] = { b = true },
		['locked'] = { b = true, inv = 'Off' },
		['motion'] = { b = true },
		['nightmode'] = { b = true, inv = 'Off' },
		['no motion'] = { b = false, inv = 'Off' },
		['normal'] = { b = true, inv = 'Alarm' },
		['off'] = { b = false, inv = 'On' },
		['on'] = { b = true, inv = 'Off' },
		['open'] = { b = true, inv = 'Off' },
		['panic end'] = { b = false },
		['panic'] = { b = true, inv = 'Off' },
		['paused'] = { b = false, inv = 'Play' },
		['photo'] = { b = true },
		['playing'] = { b = true, inv = 'Pause' },
		['set color'] = { b = true, inv = 'Off' },
		['set kelvin level'] = { b = true, inv = 'Off' },
		['set to white'] = { b = true, inv = 'Off' },
		['stop'] = { b = false, inv = 'Open' },
		['stopped'] = { b = false },
		['unlocked'] = { b = false, inv = 'On' },
		['video'] = { b = true },
	}

	return self

end

return DeviceAdapters
