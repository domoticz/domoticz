-- Example of XML parser handling data with the following structure

-- Data are coming from the OpenWeatherMap API : See http://www.openweathermap.org/current

-- <current>
-- 	<city id="12345" name="Paris">
-- 		<coord lon="2.35" lat="48.85"/>
-- 		<country>FR</country>
-- 		<sun rise="2016-04-22T04:45:07" set="2016-04-22T18:53:47"/>
-- 	</city>
-- 	<temperature value="14.94" min="14.94" max="14.94" unit="metric"/>
-- 	<humidity value="75" unit="%"/>
-- 	<pressure value="1020.69" unit="hPa"/>
-- 	<wind>
-- 		<speed value="1.96" name="Light breeze"/>
-- 		<gusts/>
-- 		<direction value="268.505" code="W" name="West"/>
-- 	</wind>
-- 	<clouds value="92" name="overcast clouds"/>
-- 	<visibility/>
-- 	<precipitation value="0.18" mode="rain" unit="3h"/>
-- 	<weather number="500" value="light rain" icon="10d"/>
-- 	<lastupdate value="2016-04-22T08:02:45"/>
-- </current>

-- Retrieve the request content
s = request['content'];

-- Update some devices (index are here for this example)

local temp = domoticz_applyXPath(s,'//current/temperature/@value')
local humidity = domoticz_applyXPath(s,'//current/humidity/@value')
local pressure = domoticz_applyXPath(s,'//current/pressure/@value')
domoticz_updateDevice(2988507,'',temp .. ";" .. humidity .. ";0;" .. pressure .. ";0")
