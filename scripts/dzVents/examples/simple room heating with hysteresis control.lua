-- assumptions:
-- the setpoint is set by a selector dummy device where the values are numeric temperatures
-- but you can easily change it to a setpoint device

local BOILER_DEVICE = 'Boiler' -- switch device
local SETPOINT_DEVICE = 'Setpoint' -- selector dummy device
local TEMPERATURE_SENSOR = 'Temperature'
local HYSTERESIS = 0.5 -- temp has to drop this value below setpoint before boiler is on again
local SMOOTH_FACTOR = 3
local LOGGING = true

return {
	on = {
		['timer'] = {
			'every minute',
		},
		devices = {
			TEMPERATURE_SENSOR,
			SETPOINT_DEVICE
		}
	},
	data = {
		temperatureReadings = { history = true, maxItems = SMOOTH_FACTOR }
	},
	active = true,
	execute = function(domoticz, item)

		local avgTemp
		local temperatureReadings = domoticz.data.temperatureReadings
		local sensor = domoticz.devices(TEMPERATURE_SENSOR)
		local current = sensor.temperature
		local boiler = domoticz.devices(BOILER_DEVICE)
		local setpoint = domoticz.devices(SETPOINT_DEVICE)

		-- first check if the sensor got a new reading or the setpoint was changed:
		if (item.isDevice) then

			if (sensor.changed) then
				-- sensor just reported a new reading
				-- add it to the readings table

				if (current ~= 0 and current ~= nil) then
					temperatureReadings.add(current)
				else
					-- no need to be here, weird state detected
					domoticz.log('Strange sensor reading.. skiping', domoticz.LOG_ERROR)
					return
				end

			elseif (domoticz.devices(SETPOINT_DEVICE).changed) then
				-- a new setpoint was set
				if LOGGING then domoticz.log('Setpoint was set to ' .. item.state) end
			else
				-- no business here, bail out...
				return
			end
		end

		-- now determine what to do
		if (setpoint.state == nil or setpoint.state == 'Off') then
			boiler.switchOff()
			return -- we're done here
		end

		local setpointValue = tonumber(setpoint.state)

		-- determine at which temperature the boiler should be
		-- switched on
		local switchOnTemp = setpointValue - HYSTERESIS

		-- don't use the current reading but average it out over
		-- the past <SMOOTH_FACTOR> readings (data smoothing) to get rid of noise, wrong readings etc
		local avgTemp = temperatureReadings.avg(1, SMOOTH_FACTOR, current) -- fallback to current when history is empty

		if LOGGING then
			domoticz.log('Average: ' .. avgTemp, domoticz.LOG_INFO)
			domoticz.log('Setpoint: ' .. setpointValue, domoticz.LOG_INFO)
			domoticz.log('Current boiler state: ' .. boiler.state, domoticz.LOG_INFO)
			domoticz.log('Switch-on temperature: ' .. switchOnTemp, domoticz.LOG_INFO)
		end

		if (avgTemp >= setpointValue and boiler.state == 'On') then
			if LOGGING then domoticz.log('Target temperature reached, boiler off') end
			boiler.switchOff()
		end

		if (avgTemp < setpointValue and boiler.state == 'Off') then
			if (avgTemp < switchOnTemp) then
				if LOGGING then domoticz.log('Heating is required, boiler switched on') end
				boiler.switchOn()
			else
				if LOGGING then domoticz.log('Average temperature below setpoint but within hysteresis range, waiting for temperature to drop to ' .. switchOnTemp) end
			end
		end
	end
}
