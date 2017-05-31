return {
	active = false,
	on = {
		['timer'] = 'every minute',
	},
	execute = function(domoticz, dummy, info)
		domoticz.log('Timer event was triggered', domoticz.LOG_INFO)
		-- code
	end
}