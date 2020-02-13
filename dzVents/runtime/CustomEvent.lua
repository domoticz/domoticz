local _ = require('lodash')
local evenItemIdentifier = require('eventItemIdentifier')

local function CustomEvent(domoticz, eventData)

	-- {["status"]="info", ["type"]="customEvent", ["data"]={["name"]="", ["data"]=""}, ["message"]=""}

	local self = {}

	self.type = eventData.type
	self.status = eventData.status
	self.message = eventData.message
	self.trigger = eventData.data.name
    if eventData.data.data:match('%b{}') then 
		self.data = domoticz.utils.fromJSON(eventData.data.data)
	else
		self.data = eventData.data.data
	end
	evenItemIdentifier.setType(
		self,
		'isCustomEvent',
		domoticz.BASE_TYPE_CUSTOM_EVENT,
		self.trigger
	)

	return self
end

return CustomEvent
