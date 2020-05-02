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

	if self.data and utils.isJSON(self.data) then 
		local json = utils.fromJSON(self.data:gsub("'",'"')) 
		if json and type(json) == 'table' then
			self.isJSON = true
			self.json = json
		end
	elseif self.data and utils.isXML(self.data) then
		 local xml = utils.fromXML(self.data)
		 if (xml) and type(xml) == 'table' then
			self.isXML = true
			self.xml = xml
		 end
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
