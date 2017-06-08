-- make sure we can find our modules
        
local currentPath = globalvariables['script_path']
local triggerReason = globalvariables['script_reason']

scriptsFolderPath = currentPath .. 'scripts'
package.path = package.path .. ';' .. currentPath .. '?.lua'
package.path = package.path .. ';' .. currentPath .. 'runtime/?.lua'
package.path = package.path .. ';' .. currentPath .. 'dzVents/?.lua'
package.path = package.path .. ';' .. currentPath .. 'scripts/?.lua'
package.path = package.path .. ';' .. currentPath .. 'scripts/storage/?.lua'

local EventHelpers = require('EventHelpers')
local helpers = EventHelpers()

if triggerReason == "time" then
    print ("Time trigger.")
    commandArray = helpers.dispatchTimerEventsToScripts()
elseif triggerReason == "device" then
    print ("Device trigger.")
    commandArray = helpers.dispatchDeviceEventsToScripts()
else
    print ("Unknown trigger: ", triggerReason)
end

return commandArray

