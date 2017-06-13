return {
	active = true,
	on = {
		'My switch'
	},

	execute = function(domoticz, mySwitch)
		if (mySwitch.state == 'On') then
			domoticz.notify('Hey!', 'I am on!', domoticz.PRIORITY_NORMAL)
		end
	end
}
