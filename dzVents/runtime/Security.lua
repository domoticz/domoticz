local evenItemIdentifier = require('eventItemIdentifier')
local function Security(domoticz, rule)

    local self = {
        state = domoticz.security
    }

	evenItemIdentifier.setType(self, 'isSecurity', domoticz.BASETYPE_SECURITY, rule)

    return self

end

return Security
