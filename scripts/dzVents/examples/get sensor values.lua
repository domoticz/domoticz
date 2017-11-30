return {
	active = true,
	on = {
		devices = {
			'My sensor'
		}
	},
	execute = function(domoticz, mySensor)

		if (mySensor.temperature > 50) then
			domoticz.notify('Hey!', 'The house might be on fire!!',
				domoticz.PRIORITY_EMERGENCY,
				domoticz.SOUND_SIREN)
			domoticz.log('Fire alert', domoticz.LOG_ERROR)
		end

	end
}

