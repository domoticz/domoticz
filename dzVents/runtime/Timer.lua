local evenItemIdentifier = require('eventItemIdentifier')

local function Timer(domoticz, rule)

	local self = {
		baseType = domoticz.BASETYPE_TIMER,
	}

	evenItemIdentifier.setType(self, 'isTimer', domoticz.BASETYPE_TIMER, rule)

	return self

end

return Timer
