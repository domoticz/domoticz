-- Example of XML parser handling data with the following structure
--<?xml version="1.0" encoding="UTF-8"?>
--<sensor>
--       <id>13</id>
--        <value>29.99</value>
--</sensor>

--}

-- A test with curl would be : curl -X POST -d "@test.xml" 'http://192.168.1.17:8080/json.htm?type=command&param=udevices&script=example_xml.lua'

-- Retrieve the request content
s = request['content'];

-- Update some devices (index are here for this example)
local id = domoticz_applyXPath(s,'//id/text()')
local s = domoticz_applyXPath(s,'//value/text()')
domoticz_updateDevice(id,'',s)
