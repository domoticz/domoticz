return {
	active = true,
	on = {
		devices = {'vdRepeatSwitch'},
	},
	execute = function(domoticz, switch)
		local latest = domoticz.globalData.repeatSwitch.getLatest()

		domoticz.globalData.repeatSwitch.add( {
			state = switch.state,
			delta = domoticz.round((latest.time.msAgo / 1000))
		} )


	end
}