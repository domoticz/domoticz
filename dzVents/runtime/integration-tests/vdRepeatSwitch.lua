local log
local dz

return {
	active = true,
	on = {
		devices = {'vdRepeatSwitch'},
	},
	execute = function(domoticz, switch)
		local res = true
		dz = domoticz
		log = dz.log
		dz.globalData.repeatSwitch.add(switch.state)
	end
}