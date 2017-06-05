return {
	active = true,
	on = {
		['timer'] = {
			function(domoticz)
				return domoticz.time.hour == 12
			end
		}
	},
	execute = function(domoticz, device)
		domoticz.setScene('scene 2', 'On')
		return 'script_timer_function'
	end
}