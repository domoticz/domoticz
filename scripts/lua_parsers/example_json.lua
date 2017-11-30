-- Example of JSON parser handling data with the following structure
--{
--  "id": 13,
--  "name": "outside",
--  "temperature": 12.50,
--  "tags": ["France", "winter"]
--}

-- A test with curl would be : curl -X POST -d "@test.json" 'http://192.168.1.17:8080/json.htm?type=command&param=udevices&script=example_json.lua'

-- Retrieve the request content
s = request['content'];

-- Update some devices (index are here for this example)
local id = domoticz_applyJsonPath(s,'.id')
local s = domoticz_applyJsonPath(s,'.temperature')
domoticz_updateDevice(id,'',s)
