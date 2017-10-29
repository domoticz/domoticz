return {
	active = true,
	on = {
		devices = {'vdCancelledRepeatSwitch'},
	},
	execute = function(domoticz, switch)
		domoticz.globalData.cancelledRepeatSwitch = domoticz.globalData.cancelledRepeatSwitch + 1
		switch.reset()
	end
}
