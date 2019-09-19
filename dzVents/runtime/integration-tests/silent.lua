-- removed 'vdAirQuality' sensor because of changed updateDevice method in V4.10705 
-- function was removed completely from EventSystem.cpp and now all are using 
-- the function in MainWorker.cpp
-- see https://github.com/domoticz/domoticz/issues/3626
--
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
		-- devices = {'vdSilentSwitch', 'sceneSilentSwitch1', 'groupSilentSwitch1', 'vdAirQuality'},
		devices = {'vdSilentSwitch', 'sceneSilentSwitch1', 'groupSilentSwitch1'}, 
		variables = {'varSilent'},

	},
	execute = function(domoticz, item, triggerInfo)
		log = domoticz.log
		dz = domoticz
		err('NOTRIGGER is not working. ' .. triggerInfo.type)
		domoticz.devices('switchSilentResults').switchOn()
	end
}