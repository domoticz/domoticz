return {
	active = true,
	on = {
		devices = {'sceneCancelledSwitch1'},
		scenes = {'scCancelledScene'},
	},
	execute = function(domoticz, scene)
		domoticz.log("cancelledScene now " .. domoticz.globalData.cancelledScene ,domoticz.LOG_FORCE)
		domoticz.globalData.cancelledScene = domoticz.globalData.cancelledScene + 1
		domoticz.log("cancelledScene now " .. domoticz.globalData.cancelledScene ,domoticz.LOG_FORCE)
		scene.cancelQueuedCommands()
	end
}
