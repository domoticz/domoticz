-- Example of parser handling data with the following format
-- TEMPERATURE,HUMIDITY,HUMIDITY_STATUS

-- A test with curl would be : curl -X POST -d "28,48,2" 'http://192.168.1.17:8080/json.htm?type=command&param=udevices&script=example.lua'

-- This function split a string according to a defined separator
-- Entries are returned into an associative array with key values 1,2,3,4,5,6...
local function split(str, sep)
	if sep == nil then
		sep = "%s"
	end
	local t={} ; i=1
	local regex = string.format("([^%s]+)", sep)
		for str in str:gmatch(regex) do
			t[i] = str
			i = i + 1
		end
	return t
end

-- Retrieve the request content
s = request['content'];

-- Split the content into an array of values
local values = split(s, ",");

-- Update some devices (index are here for this example)
domoticz_updateDevice(19,'',values[1])
domoticz_updateDevice(20,values[2],values[2] .. ";" .. values[3])
