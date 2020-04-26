return {
	on = {
		timer = {
			'every 5 minutes' -- just an example to trigger the event
		},
		customEvents = {
			'MyEvent' -- event triggered by emitEvent
		}
	},
	data = {},
	logging = {},
	execute = function(domoticz, triggeredItem)
		if (triggeredItem.isCustomEvent) then
			domoticz.utils._.print(triggeredItem.data)
		else
			-- second parameter can be anything, number, string, boolean or table
			domoticz.emitEvent('MyEvent', 'Some data')
		end
	end
}
