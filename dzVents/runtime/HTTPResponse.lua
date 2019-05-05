local _  = require('lodash')
local utils = require('Utils')

local function HTTPResponce(domoticz, responseData)

	local self = {}
	 
	self.headers = responseData.headers or {}
	self.baseType = domoticz.BASETYPE_HTTP_RESPONSE
	self.data = responseData.data
	self.status = responseData.status
	self._contentType = _.get(self.headers, {'Content-Type'}, '')

	self.isJSON = false

	if self.status and self.status ~= ""  then
		local chunks = utils.stringSplit(self.status)
		self.protocol = chunks[1]
		self.statusCode = tonumber(chunks[2] )
		self.statusMessage = table.concat(_.slice(chunks,3)," ")
	else
		self.statusMessage = ""
		self.protocol = ""
		self.statusCode = 0
	end

	self.ok = ( self.statusCode >= 200 and self.statusCode <= 299 )
	
	self.isHTTPResponse = true
	self.isDevice = false
	self.isScene = false
	self.isGroup = false
	self.isTimer = false
	self.isVariable = false
	self.isSecurity = false

	self.callback = responseData.callback
	self.trigger = responseData.callback

	if (string.match(self._contentType, 'application/json') and self.data) then
		local json = utils.fromJSON(self.data)

		if (json) then
			self.isJSON = true
			self.json = json
		end
	end
	return self
end

return HTTPResponce
