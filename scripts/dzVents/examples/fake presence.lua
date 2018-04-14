-- example script to fake the presence of people being at home

return {
	active = true,
	on = {
		timer = {
			'at sunset',
			'at 23:30'
		}
	},
	execute = function(domoticz, timer)

		if (timer.trigger == 'at sunset') then
			domoticz.devices('mySwitch').switchOn()
			domoticz.devices('anotherSwitch').dimTo(40)
			-- add whatever you want
		else
			-- switch off at a random moment after 23:30
			domoticz.devices('mySwitch').switchOff().withinMin(60)
			domoticz.devices('anotherSwitch').switchOff().withinMin(60)
		end
	end
}
