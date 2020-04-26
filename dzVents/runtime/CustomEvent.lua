local utils = require('Utils')
local evenItemIdentifier = require('eventItemIdentifier')

local function CustomEvent(domoticz, eventData)

	-- {["status"]="info", ["type"]="customEvent", ["data"]={["name"]="", ["data"]=""}, ["message"]=""}

	local self = {}

	self.type = eventData.type
	self.status = eventData.status
	self.message = eventData.message
	self.trigger = eventData.data.name
	self.customEvent = self.trigger

	self.isXML = false
	self.isJSON = false
	self.data = eventData.data.data

	if self.data:match('%b{}') then 
		local json = utils.fromJSON(self.data:gsub("'",'"')) 
		if json and type(json) == "table" then
			self.isJSON = true
			self.json = json
		end
	elseif self.data:match('%b<>') then
		 local xml = utils.fromXML(self.data)
		 if (xml) then
			self.isXML = true
			self.xml = xml
		 end
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
