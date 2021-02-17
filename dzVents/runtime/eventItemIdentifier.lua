return {

	setType = function(item, typeName, baseType, trigger)

		item.isHTTPResponse = false
		item.isShellCommandResponse = false
		item.isDevice = false
		item.isScene = false
		item.isGroup = false
		item.isTimer = false
		item.isVariable = false
		item.isSecurity = false
		item.isSystem = false
		item.isHardware = false
		item.isCustomEvent = false

		item[typeName] = true

		item.baseType = baseType

		item.trigger = trigger
		return item

	end

}
