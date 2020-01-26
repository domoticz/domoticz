local utils = require('Utils')

local function HTTPResponce(domoticz, responseData, testResponse)

	local self = {}
	local lowerCaseHeaders = {} 
	self.headers = responseData.headers or {}

	for key, data in pairs(self.headers) do 
		lowerCaseHeaders[string.lower(key)] = data  -- Case can vary (Content_type, content_type, Content_Type, ?) 
	end
	self._contentType = lowerCaseHeaders['content-type'] or ''

	self.baseType = domoticz.BASETYPE_HTTP_RESPONSE
	self.data = responseData.data
	self.statusText = responseData.statusText
	self.protocol = responseData.protocol
	self.statusCode = responseData.statusCode

	if self.statusCode >= 200 and self.statusCode <= 299 then
		self.ok = true
	else
		self.ok = false
		if ( not testResponse ) and ( utils.log(self.protocol .. " response: " .. self.statusCode .. " ==>> " .. self.statusText ,utils.LOG_ERROR) ) then end
	end

	self.isHTTPResponse = true
	self.isDevice = false
	self.isScene = false
	self.isGroup = false
	self.isTimer = false
	self.isVariable = false
	self.isSecurity = false
    self.isXML = false
	self.isJSON = false
	
	self.callback = responseData.callback
	self.trigger = responseData.callback
    
    if (self._contentType):match('application/json') and self.data then
		local json = utils.fromJSON(self.data)
		if (json) then
			self.isJSON = true
			self.json = json
		end
	elseif (self._contentType):find('text/xml') and self.data then
         local xml = utils.fromXML(self.data)
		 if (xml) then
			self.isXML = true
			self.xml = xml
            self.xmlVersion = self.data:match([[<?xml version="(.-)"]])
            self.xmlEncoding = self.data:match([[encoding="(.-)"]])
		 end
	end

	return self
end

return HTTPResponce
