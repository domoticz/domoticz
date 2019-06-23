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
			adapterManager.addDummyMethod(device, 'rename')
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

		function group.setDescription(newDescription)
			local url = domoticz.settings['Domoticz url'] .. 
						'/json.htm?type=updatescene&scenetype=1' ..
						'&idx=' .. group.id ..
						'&name='.. utils.urlEncode(group.name) ..
						'&description=' .. utils.urlEncode(newDescription) 
			return domoticz.openURL(url)
		end

		function group.rename(newName)
			local url = domoticz.settings['Domoticz url'] .. '/json.htm?type=updatescene&scenetype=1' ..
						'&idx=' .. group.id ..
						'&name='.. utils.urlEncode(newName) ..
						'&description=' .. utils.urlEncode(group.description) 
			return domoticz.openURL(url)
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
