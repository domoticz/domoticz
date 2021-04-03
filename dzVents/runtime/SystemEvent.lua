local _ = require('lodash')
local evenItemIdentifier = require('eventItemIdentifier')

local eventMapping = {
	backupDoneWeb = 'manualBackupFinished',
	backupDoneDay = 'dailyBackupFinished',
	backupDoneHour = 'hourlyBackupFinished',
	backupDoneMonth = 'monthlyBackupFinished',
	start = 'start',
	stop = 'stop',
	resetAllEvents = 'resetAllEvents',
	resetAllDeviceStatus = 'resetAllDeviceStatus',
}

local function SystemEvent(domoticz, eventData)

	-- eventData: {["message"]="", ["status"]="info", ["type"]="domoticzStart"}

	local self = {}

	self.type = eventMapping[eventData.type]
	self.status = eventData.status
	self.message = eventData.message
	self.data = eventData.data
	self.duration = eventData.data and eventData.data.duration or nil
	self.location = eventData.data and eventData.data.location or nil

	function self.dump( filename )
		domoticz.logObject(self, filename, 'systemEvent')
	end

	evenItemIdentifier.setType(
		self,
		'isSystemEvent',
		domoticz.BASETYPE_SYSTEM_EVENT,
		eventMapping[eventData.type]
	)

	return self
end

return SystemEvent
