local RANDOM_DELAY_MINS = 30
return {
	active = true,
	on = {
		['timer'] = {
			'at sunset',
			'at 01:00'
		}
	},
	execute = function(domoticz)

		if (domoticz.security ~= domoticz.SECURITY_DISARMED) then

			local light = domoticz.devices('Window light')

			if (not light.bState) then -- i.e. state == 'On'
				light.switchOn().withinMin(RANDOM_DELAY_MINS)
			else
				light.switchOff().withinMin(RANDOM_DELAY_MINS)
			end

		end
	end
}
