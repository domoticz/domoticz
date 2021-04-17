local evenItemIdentifier = require('eventItemIdentifier')
local function Security(domoticz, rule)

	local self = {
		state = domoticz.security
	}

	evenItemIdentifier.setType(self, 'isSecurity', domoticz.BASETYPE_SECURITY, rule)

	function self.dump( filename )
		domoticz.logObject(self, filename, 'security')
	end

	return self

end

return Security
