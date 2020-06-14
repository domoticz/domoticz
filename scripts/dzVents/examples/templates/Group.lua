return {
	on = {
		groups = {
			'myGroup'
		}
	},
	execute = function(domoticz, group)
		domoticz.log('Group ' .. group.name .. ' was changed', domoticz.LOG_INFO)
	end
}