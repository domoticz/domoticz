return {
	active = true,
	on = {
		variables = {
			'varCancelled'
		}
	},
	execute = function(domoticz, variable)

		-- this should not happen because it should have been cancelled by varString.lua
		variable.set(1).silent()
		print('This should not have happened')

	end
}
