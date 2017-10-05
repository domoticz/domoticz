return {
	active = true,
	on = {
		scenes = {'mygroup2'}
	},
	data = {},
	execute = function(domoticz, group, triggerInfo)
		return group.name
	end
}