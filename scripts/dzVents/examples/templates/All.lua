-- Check the wiki for dzVents
-- remove what you don't need
return {

	-- optional active section,
	-- when left out the script is active
	-- note that you still have to check this script
	-- as active in the side panel
	active = {

		true,  -- either true or false, or you can specify a function

		--function(domoticz)
			-- return true/false
		--end
	},
	-- trigger
	-- can be a combination:
	on = {

		-- device triggers
		devices = {
			-- scripts is executed if the device that was updated matches with one of these triggers
			'device name', -- device name
			'abc*', -- triggers for all devices which name begins with abc
			258, -- id
		},

		-- timer riggers
		timer = {
			-- timer triggers.. if one matches with the current time then the script is executed
			'at 13:45',
			'at 18:37',
			'every 3 minutes on mon,tue,fri at 16:00-15:00',
			function(domoticz)
				-- return true or false
			end
		},

		-- user variable triggers
		variables = {
			'myUserVariable'
		},

		-- security triggers
		security = {
			domoticz.SECURITY_ARMEDAWAY,
			domoticz.SECURITY_ARMEHOME,
		},

		-- scene triggers
		scenes = {
			'myScene'
		},

		-- group triggers
		groups = {
			'myGroup'
		},

		-- http http responses
		httpResponses = {
			'some callback string'
		},

		-- system events
		system = {
			'start',
			'stop',
			'manualBackupFinished',
			'dailyBackupFinished',
			'hourlyBackupFinished',
			'monthlyBackupFinished'
		},

		customEvents = {
			'myCustomEvent'
		}
	},

	-- persistent data
	-- see documentation about persistent variables
	data = {
		myVar = { initial = 5 },
		myHistoryVar = { maxItems = 10 },
	},

	-- custom logging level for this script
	logging = {
        level = domoticz.LOG_DEBUG,
        marker = "Cool script"
    },

	-- actual event code
	-- the second parameter is depending on the trigger
	-- when it is a device change, the second parameter is the device object
	-- similar for variables, scenes and groups and httpResponses
	-- inspect the type like: triggeredItem.isDevice
	execute = function(domoticz, triggeredItem, info)
		--[[

		The domoticz object holds all information about your Domoticz system. E.g.:

		local myDevice = domoticz.devices('myDevice')
		local myVariable = domoticz.variables('myUserVariable')
		local myGroup = domoticz.groups('myGroup')
		local myScene = domoticz.scenes('myScene')

		The device object is the device that was triggered due to the device in the 'on' section above.
		]] --

		-- example

		if (triggerdItem.active) then -- state == 'On'
			triggerdItem.switchOff().afterMin(2) -- if it is a switch
			domoticz.notify('Light info', 'The light ' .. triggerdItem.name .. ' will be switched off soon')
		end
	end
}
