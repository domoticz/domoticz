local evenItemIdentifier = require('eventItemIdentifier')

local function Timer(domoticz, rule)

	local self = {
		baseType = domoticz.BASETYPE_TIMER,
	}

	evenItemIdentifier.setType(self, 'isTimer', domoticz.BASETYPE_TIMER, rule)

	function self.dump( filename )
		domoticz.logObject(self, filename, 'timer')
	end

	return self

end

return Timer
