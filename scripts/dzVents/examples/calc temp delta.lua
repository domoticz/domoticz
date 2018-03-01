--[[
	Assume you have two temperature sensors and a third dummy sensor that should be the
	difference of these two sensors (e.g. you want to see the difference between water temperature
	going into a radiator and the temperature of the water going out of it
]]--


return {
	active = true,
	on = {
		['timer'] = {'every 5 minutes'}
	},
	execute = function(domoticz)
		local inTemp = domoticz.devices('Temp in').temperature
		local outTemp = domoticz.devices('Temp out').temperature
		local delta = outTemp - inTemp -- how much did the temperature drop?

		-- update the dummy sensor
		domoticz.devices('Delta temp').updateTemperature(delta)

	end
}
