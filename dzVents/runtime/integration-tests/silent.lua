local log
local dz
local err = function(msg)
	log(msg, dz.LOG_ERROR)
end

-- all these devices and variable should NOT be triggered due to
-- stage1 changes

return {
	active = true,
	on = {
		devices = {'vdSilentSwitch', 'sceneSilentSwitch1', 'groupSilentSwitch1', 'vdAirQuality'},
		variables = {'varSilent'},

	},
	execute = function(domoticz, item, triggerInfo)
		log = domoticz.log
		dz = domoticz
		err('NOTRIGGER is not working. ' .. triggerInfo.type)
		domoticz.devices('switchSilentResults').switchOn()
	end
}