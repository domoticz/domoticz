local utils = require('Utils')
local evenItemIdentifier = require('eventItemIdentifier')

local function ShellCommandResponse(domoticz, responseData, testResponse)
	local self = {}

	self.callback = responseData.callback
	self.trigger = responseData.callback
	self.shellCommandResponse = responseData.callback
	self.data = responseData.data
	if self.data == "\n" or self.data == '\n\r' then self.data = '' end
	self.statusCode = responseData.statusCode % 255
	self.errorText = responseData.errorText:gsub('\n','')
	self.timeoutOccurred = responseData.timeoutOccurred
	if (self.errorText == "" or self.errorText == "\r" or self.errorText == '\r\n') and self.timeoutOccurred then self.errorText = 'Timeout occurred' end
	self.statusText = self.errorText or ''
	self.ok = self.statusCode == 0


	function self.dump( filename )
		domoticz.logObject(self, filename, 'ShellCommandResponse')
	end

	self.isXML = false
	self.isJSON = false
	self.hasLines = false

	evenItemIdentifier.setType(self, 'isShellCommandResponse', domoticz.BASETYPE_SHELLCOMMAND_RESPONSE, responseData.callback)

	if self.data then
		if utils.isJSON(self.data, self._contentType) then
			local json = utils.fromJSON(self.data)
			if (json) then
				self.isJSON = true
				self.json = json
			end
		elseif utils.isXML(self.data, self._contentType) then
			 local xml = utils.fromXML(self.data)
			 if (xml) then
				self.isXML = true
				self.xml = xml
				self.xmlVersion = self.data:match([[<?xml version="(.-)"]])
				self.xmlEncoding = self.data:match([[encoding="(.-)"]])
			 end
		elseif utils.hasLines(self.data) then
			local lines = utils.fromLines(self.data)
			if lines then
				self.hasLines = true
				self.lines = lines
			end
		end
	end

	return self
end

return ShellCommandResponse
