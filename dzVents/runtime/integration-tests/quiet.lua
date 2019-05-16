-- These devices will be triggered

return {

	on = {
		devices = {'vdQuietOnSwitch', 'vdQuietOffSwitch'},

	},
	execute = function(dz, item)
		dz.devices('switchQuietResults').switchOn()
	end
}