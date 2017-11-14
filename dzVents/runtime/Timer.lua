local function Timer(domoticz, rule)

    local self = {
        baseType = domoticz.BASETYPE_TIMER,
        triggerRule = rule,
        isVariable = false,
		isHTTPResponse = false,
	    isDevice = false,
	    isScene = false,
	    isGroup = false,
	    isTimer = true,
    }

    return self

end

return Timer
