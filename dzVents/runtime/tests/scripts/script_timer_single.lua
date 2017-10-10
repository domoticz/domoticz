local min = 'minute'
return {
	active = true,
	on = {
		['timer'] = {'every ' .. min}
	},
	execute = function(domoticz, device, triggerInfo)
		domoticz.notify('Me', triggerInfo.type .. ' ' .. triggerInfo.trigger)
		return 'script_timer_table'
	end
}