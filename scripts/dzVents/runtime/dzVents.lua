-- make sure we can find our modules

local currentPath = globalvariables['script_path']
local triggerReason = globalvariables['script_reason']

scriptsFolderPath = currentPath .. 'scripts' -- global

package.path = package.path .. ';' .. currentPath .. '?.lua'
package.path = package.path .. ';' .. currentPath .. 'runtime/?.lua'
package.path = package.path .. ';' .. currentPath .. 'runtime/device-adapters/?.lua'
package.path = package.path .. ';' .. currentPath .. 'dzVents/?.lua'
package.path = package.path .. ';' .. currentPath .. 'scripts/?.lua'
package.path = package.path .. ';' .. currentPath .. 'scripts/storage/?.lua'

local EventHelpers = require('EventHelpers')
local helpers = EventHelpers()

_ = require 'lodash' -- todo remove
local persistence = require('persistence')

-- _.print(domoticzData)

if (_G.TESTMODE == nil) then
--	persistence.store(currentPath .. '/domoticzData.lua', domoticzData)
	print(triggerReason)
end

--print('----------------------------')
--for idx, device in pairs(domoticzData) do
--
--	print(idx)
--
--	for i, attr in pairs(device) do
--
--
--
--		if (i == 'data' or i == 'rawData') then
--			_.print('>    >' .. i .. '=' .. tostring(attr))
--			for ii, attrr in pairs(attr) do
--				_.print('>         >' .. ii .. '=' .. tostring(attrr))
--			end
--		else
--			_.print('>    >' .. i .. '=' .. tostring(attr))
--		end
--
--
--	end
--
--
--end
--print('----------------------------')

if triggerReason == "time" then
	commandArray = helpers.dispatchTimerEventsToScripts()
elseif triggerReason == "device" then
	commandArray = helpers.dispatchDeviceEventsToScripts()
else
	print ("Unknown trigger: ", triggerReason)
end

return commandArray

