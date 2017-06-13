return {
	active = true,
	on = {
		'onscript1'
	},
	execute = function(domoticz, device)
		device.dimTo(10)
		return 'script3'
	end
}