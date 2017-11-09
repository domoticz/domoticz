return {
	active = true,
	on = {
		devices = {'sceneCancelledSwitch1'},
		scenes = {'scCancelledScene'},
	},
	execute = function(domoticz, scene)
		domoticz.globalData.cancelledScene = domoticz.globalData.cancelledScene + 1
		scene.cancelQueuedCommands()
	end
}
