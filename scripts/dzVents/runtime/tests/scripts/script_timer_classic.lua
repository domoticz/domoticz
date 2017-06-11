return {
	active = true,
	on = {
		timer = {'every minute'}
	},
	execute = function(domoticz)
		domoticz.devices('onscript1').switchOff()
		return 'script_timer_classic'
	end
}