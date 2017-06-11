return {
	active = true,

	on = {
		devices = {'onscript2'}
	},
	execute = function(domoticz, device)
		local a = nil .. '123' -- should raise an error
		return 'script2'
	end
}