local TimedCommand = require('TimedCommand')

return {

	baseType = 'group',

	name = 'Group device adapter',

	matches = function (device, adapterManager)
		local res = (device.baseType == 'group')
		if (not res) then
			adapterManager.addDummyMethod(device, 'switchOn')
			adapterManager.addDummyMethod(device, 'switchOff')
			adapterManager.addDummyMethod(device, 'toggleGroup')
			adapterManager.addDummyMethod(device, 'setDescription')
		end
		return res
	end,

	process = function (group, data, domoticz, utils, adapterManager)

		group.isGroup = true

		function group.toggleGroup()
			local current, inv
			if (group.state ~= nil) then
				current = adapterManager.states[string.lower(group.state)]
				if (current ~= nil) then
					inv = current.inv
					if (inv ~= nil) then
						return TimedCommand(domoticz, 'Group:' .. group.name, inv, 'device')
					end
				end
			end
			return nil
		end

		function group.setState(newState)
			-- generic state update method
			return TimedCommand(domoticz, 'Group:' .. group.name, newState, 'device', group.state)
		end

		function group.switchOn()
			return TimedCommand(domoticz, 'Group:' .. group.name, 'On', 'device', group.state)
		end

		function group.switchOff()
			return TimedCommand(domoticz, 'Group:' .. group.name, 'Off', 'device', group.state)
		end

		function group.setDescription(description)
			local url = domoticz.settings['Domoticz url'] ..
				"/json.htm?description=" .. domoticz.utils.urlEncode(description) ..
				"&scenetype=1" ..
				"&idx=" .. group.id ..
				"&name=".. domoticz.utils.urlEncode(group.name) ..
				"&type=updatescene" ..
				"&onaction=&offaction="
			return domoticz.openURL(url)
			--?type=updatescene&idx=7&scenetype=1&name=gpDescriptionGroup&description=ff&onaction=&offaction=&protected=false

		end

		function group.devices()
			local subData = {}
			local ids = data.deviceIDs ~= nil and data.deviceIDs or {}

			for i, id in pairs(ids) do
				subData[i] = domoticz._getItemFromData('device', id)
			end

			return domoticz._setIterators({}, true, 'device', false , subData)
		end

	end

}
