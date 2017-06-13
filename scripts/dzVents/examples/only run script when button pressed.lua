return {

	-- create a function for the active key, when the switch in Domoticz
	-- called 'script_blabla' is active, then the script is executed.

	-- Note: keep this function really light weight because it is ALWAYS
	-- executed in every cycle, even when 'My switch' hasn't changed!!

	active = function(domoticz)
		return (domoticz.devices('script_blabla').state == 'On')
	end,

	on = {
		devices = {
			'My switch'
		}
	},

	execute = function(domoticz, mySwitch)
		-- do some weird complicated stuff
		-- that takes quite some processing time ;-)
	end
}