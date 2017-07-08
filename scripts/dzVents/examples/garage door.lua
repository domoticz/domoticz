--[[

From the domoticz examples online:
http://www.domoticz.com/wiki/Event_script_examples

Send a warning when the garage door has been open for more than 10 minutes

 ]]

return {
	active = true,
	on = {
		['timer'] = {
			'every minute',
		}
	},
	execute = function(domoticz)

		local door = domoticz.devices('Garage door')

		if (door.state == 'Open' and door.lastUpdate.minutesAgo > 10) then
			domoticz.notify('Garage door alert',
				'The garage door has been open for more than 10 minutes!',
				domoticz.PRIORITY_HIGH)
		end
	end
}