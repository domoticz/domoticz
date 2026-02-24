return {
	active = true,
	on = {
		at_startup = true,
		timer = { 'every hour' },
	},
	execute = function(domoticz)
		return 'script_at_startup'
	end
}
