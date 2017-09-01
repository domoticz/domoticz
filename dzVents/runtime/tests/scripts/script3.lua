return {
	active = true,
	on = {
		devices = {'onscript1'}
	},
	execute = function(domoticz, device)
		device.dimTo(10)
		return 'script3'
	end
}