local function Timer(domoticz, rule)

    local self = {
        baseType = domoticz.BASETYPE_TIMER,
        triggerRule = rule
    }

    return self

end

return Timer
