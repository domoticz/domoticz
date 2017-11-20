return {
	active = true,
	on = {
		scenes = {'mygroup1'}
	},
	data = {},
	execute = function(domoticz, group, triggerInfo)
		group.switchOn()
		return group.name
	end
}