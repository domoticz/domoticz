local min = 'minute'
return {
	active = true,
	on = {
		['timer'] = {'every ' .. min}
	},
	execute = function(domoticz, timer, triggerInfo)
		if (timer.isTimer) then
			domoticz.notify('Me', timer.trigger .. ' ' .. triggerInfo.type .. ' ' .. triggerInfo.trigger .. ' ' .. triggerInfo.scriptName)
		end
		return 'script_timer_table'
	end
}
