return {
	active = true,
	on = {
		['timer'] = {
			'every minute'
		}
	},
	execute = function(domoticz, device)
		domoticz.setScene('scene 1', 'On')
		return 'script_timer_table'
	end
}