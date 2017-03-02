-- make sure we can find our modules

local currentPath = globalvariables['script_path']
print ("1")
print (currentPath)
print (debug.getinfo(1).source:match("@?(.*/)"))
print ("2")
scriptsFolderPath = currentPath .. 'scripts'
package.path = package.path .. ';' .. currentPath .. '?.lua'
package.path = package.path .. ';' .. currentPath .. 'dzVents/?.lua'
package.path = package.path .. ';' .. currentPath .. 'scripts/?.lua'
package.path = package.path .. ';' .. currentPath .. 'scripts/storage/?.lua'

local EventHelpers = require('EventHelpers')
local helpers = EventHelpers()

commandArray = helpers.dispatchDeviceEventsToScripts()

return commandArray

