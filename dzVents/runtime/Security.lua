local function Security(domoticz, rule)

    local self = {
        baseType = domoticz.BASETYPE_SECURITY,
        trigger = rule,
        isVariable = false,
		isHTTPResponse = false,
	    isDevice = false,
	    isScene = false,
	    isGroup = false,
	    isTimer = false,
        isSecurity = true,
        state = domoticz.security
    }

    return self

end

return Security
