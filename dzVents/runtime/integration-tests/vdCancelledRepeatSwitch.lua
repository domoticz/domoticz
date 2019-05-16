return {
	active = true,
	on = {
		devices = {'vdCancelledRepeatSwitch'},
	},

	logging = {
		level = domoticz.LOG_DEBUG,
		marker = httpResponses
	},

	execute = function(domoticz, switch)
		domoticz.globalData.cancelledRepeatSwitch = domoticz.globalData.cancelledRepeatSwitch + 1
		switch.cancelQueuedCommands()
		domoticz.devices('vdText').cancelQueuedCommands()
		domoticz.log("What do you see here ? ",domoticz.LOG_DEBUG)
	end
}
