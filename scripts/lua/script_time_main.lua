-- make sure we can find our modules
local currentPath = globalvariables['script_path']
scriptsFolderPath = currentPath .. 'scripts'
package.path = package.path .. ';' .. currentPath .. '?.lua'
package.path = package.path .. ';' .. currentPath .. 'dzVents/?.lua'
package.path = package.path .. ';' .. currentPath .. 'scripts/?.lua'
package.path = package.path .. ';' .. currentPath .. 'scripts/storage/?.lua'

print("1")
print(currentPath)
print(debug.getinfo(1).source:match("@?(.*/)"))
print(package.path)
print("2")

local EventHelpers = require('EventHelpers')
local helpers = EventHelpers()

commandArray = helpers.dispatchTimerEventsToScripts()

return commandArray

