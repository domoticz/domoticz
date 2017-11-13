local min = 'minute'
return {
	active = true,
	on = {
		['timer'] = {'every ' .. min}
	},
	execute = function(domoticz, timer, triggerInfo)
		domoticz.notify('Me', timer.triggerRule .. ' ' .. triggerInfo.type .. ' ' .. triggerInfo.trigger)
		return 'script_timer_table'
	end
}
