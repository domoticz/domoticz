local function Timer(domoticz, rule)

    local self = {
        baseType = domoticz.BASETYPE_TIMER,
        trigger = rule,
        isVariable = false,
		isHTTPResponse = false,
	    isDevice = false,
	    isScene = false,
	    isGroup = false,
	    isTimer = true,
        isSecurity = false
    }

    return self

end

return Timer
