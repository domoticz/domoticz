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

	function self.dump( filename )
		domoticz.logObject(self, filename, 'customEvent')
	end

	if self.data then
		local json = utils.isJSON(self.data) and utils.fromJSON(self.data)
		if json and type(json) == 'table' then
			self.isJSON = true
			self.json = json
		elseif utils.isXML(self.data) then
			local xml = utils.fromXML(self.data)
			if (xml) and type(xml) == 'table' then
				self.isXML = true
				self.xml = xml
				self.xmlVersion = self.data:match([[<?xml version="(.-)"]])
				self.xmlEncoding = self.data:match([[encoding="(.-)"]])
			end
		elseif utils.hasLines(self.data) then
			local lines = utils.fromLines(self.data)
			if lines and type(lines) == 'table' then
				self.hasLines = true
				self.lines = lines
			end
		end
	end

	evenItemIdentifier.setType(
		self,
		'isCustomEvent',
		domoticz.BASETYPE_CUSTOM_EVENT,
		self.trigger
	)

	return self
end

return CustomEvent
