package.path = package.path ..
		";../?.lua;../device-adapters/?.lua;./data/?.lua;../../../scripts/dzVents/generated_scripts/?.lua;" ..
		"../../../scripts/lua/?.lua"

local _ = require 'lodash'
local File = require('File')
local http = require("socket.http")
local socket = require("socket")
local ltn12 = require("ltn12")
local jsonParser = require('JSON')

local BASE_URL = 'http://localhost:8080'
local API_URL = BASE_URL .. '/json.htm?'
local DUMMY_HW = 15
local SECPANEL_INDEX = 48
local SWITCH_TYPES = {
	BLINDS = 3,
	BLINDS_INVERTED= 6,
	BLINDS_PERCENTAGE= 13,
	BLINDS_PERCENTAGE_INVERTED= 16,
	CONTACT= 2,
	DIMMER= 7,
	DOOR_CONTACT= 11,
	DOOR_LOCK= 19,
	DOORBELL= 1,
	DUSK_SENSOR= 12,
	MEDIA_PLAYER= 17,
	MOTION_SENSOR= 8,
	ON_OFF= 0,
	PUSH_OFF_BUTTON= 10,
	PUSH_ON_BUTTON= 9,
	SELECTOR= 18,
	SMOKE_DETECTOR= 5,
	VENETIAN_BLINDS_EU= 15,
	VENETIAN_BLINDS_US= 14,
	X10_SIREN= 4
}

local VIRTUAL_DEVICES = {
	AIR_QUALITY = {249, 'vdAirQuality'},
	ALERT = {7, 'vdAlert'},
	AMPERE_3_PHASE = {9, 'vdAmpere3'},
	AMPERE_1_PHASE = {19, 'vdAmpere1'},
	BAROMETER = {11, 'vdBarometer'},
	COUNTER = {113, 'vdCounter'},
	COUNTER_INCREMENTAL = {14, 'vdCounterIncremental'},
	CUSTOM_SENSOR = {1004, 'vdCustomSensor'},
	DISTANCE = {13, 'vdDistance'},
	ELECTRIC_INSTANT_COUNTER = {18, 'vdElectricInstanceCounter'},
	GAS = {3, 'vdGas'},
	HUMIDITY = {81, 'vdHumidity'},
	LEAF_WETNESS = {16, 'vdLeafWetness'},
	LUX = {246, 'vdLux'},
	P1_SMART_METER_ELECTRIC = {250, 'vdP1SmartMeterElectric'},
	PERCENTAGE = {2, 'vdPercentage'},
	PRESSURE_BAR = {1, 'vdPressureBar'},
	RAIN = {85, 'vdRain'},
	RGB_SWITCH = {241, 'vdRGBSwitch'},
	RGBW_SWITCH = {1003, 'vdRGBWSwitch'},
	SCALE_WEIGHT = {93, 'vdScaleWeight'},
	SELECTOR_SWITCH = {1002, 'vdSelectorSwitch'},
	SOIL_MOISTURE = {15, 'vdSoilMoisture'},
	SOLAR_RADIATION = {20, 'vdSolarRadiation'},
	SOUND_LEVEL = {10, 'vdSoundLevel'},
	SWITCH = {6, 'vdSwitch'},
	TEMPERATURE = {80, 'vdTemperature'},
	TEMP_HUM = {82, 'vdTempHum'},
	TEMP_HUM_BARO = {84, 'vdTempHumBaro'},
	TEXT = {5, 'vdText'},
	THERMOSTAT_SETPOINT = {8, 'vdThermostatSetpoint'},
	USAGE_ELECTRIC = {248, 'vdUsageElectric'},
	UV = {87, 'vdUV'},
	VISIBILITY = {12, 'vdVisibility'},
	VOLTAGE = {4, 'vdVoltage'},
	WATERFLOW = {1000, 'vdWaterflow'},
	WIND = {86, 'vdWind'},
	WIND_TEMP_CHILL = {1001, 'vdWindTempChill'},
	TEMP_BARO = { 247, 'vdTempBaro' },
	SILENT_SWITCH = { 6, 'vdSilentSwitch' },
	API_TEMP = { 80, 'vdAPITemperature' },
	REPEAT_SWITCH = { 6, 'vdRepeatSwitch' },
}
local VAR_TYPES = {
	INT = {0, 'varInteger', 42},
	FLOAT = {1, 'varFloat', 42.11},
	STRING = {2, 'varString', 'Somestring'},
	DATE = {3, 'varDate', '31/12/2017'},
	TIME = {4, 'varTime', '23:59'},
	SILENT = { 0, 'varSilent', 1 },
}

local scriptSourcePath = './'
local scriptTargetPath = '../../../scripts/dzVents/scripts/'
local generatedScriptTargetPath = '../../../scripts/dzVents/generated_scripts/'
local dataTargetPath = '../../../scripts/dzVents/data/'

function getScriptSourcePath(name)
	return scriptSourcePath .. name
end

function getScriptTargetPath(name)
	return scriptTargetPath .. name
end

function copyScript(name)
	File.remove(getScriptTargetPath(name))
	File.copy(getScriptSourcePath(name), getScriptTargetPath(name))
end

function doAPICall(url)
	local response = {}
	local result, respcode, respheaders, respstatus = http.request {
		method = "GET",
		url = API_URL .. url,
		sink = ltn12.sink.table(response)
	}

	local _json = table.concat(response)

	local json = jsonParser:decode(_json)
	local ok = json.status == 'OK'

	return ok, json, result, respcode, respheaders, respstatus
end

function createHardware(name, type)
	local url = "type=command&param=addhardware&htype=" .. tostring(type) .. "&name=" .. name .. "&enabled=true&datatimeout=0"
	local ok, json, result, respcode, respheaders, respstatus = doAPICall(url)
	local idx = json.idx
	return ok, idx, json, result, respcode, respheaders, respstatus
end

function createVirtualDevice(hw, name, type, options)
	-- type=createvirtualsensor&idx=2&sensorname=s1&sensortype=6

	local url = "type=createvirtualsensor&idx=" .. tostring(hw) .."&sensorname=" .. name .. "&sensortype=" .. tostring(type)

	if (options) then
		url = url .. '&sensoroptions=1;' .. tostring(options)
	end

	local ok, json, result, respcode, respheaders, respstatus = doAPICall(url)
	local idx = json.idx

	return ok, idx, json, result, respcode, respheaders, respstatus
end

function createScene(name)
	-- type=addscene&name=myScene&scenetype=0
	local url = "type=addscene&name=" .. name .. "&scenetype=0"

	local ok, json, result, respcode, respheaders, respstatus = doAPICall(url)
	local idx = json.idx
	return ok, idx, json, result, respcode, respheaders, respstatus
end

function createGroup(name)
	-- type=addscene&name=myGroup&scenetype=1
	local url = "type=addscene&name=" .. name .. "&scenetype=1"

	local ok, json, result, respcode, respheaders, respstatus = doAPICall(url)
	local idx = json.idx

	return ok, idx, json, result, respcode, respheaders, respstatus
end

function createVariable(name, type, value)
	--type=command&param=saveuservariable&vname=myint&vtype=0&vvalue=1
	local url = "type=command&param=saveuservariable&vname=" .. name .."&vtype=" .. tostring(type) .. "&vvalue=" .. tostring(value)

	local ok, json, result, respcode, respheaders, respstatus = doAPICall(url)
	local idx = json.idx

	return ok, idx, json, result, respcode, respheaders, respstatus
end

function deleteDevice(idx)
	-- type=deletedevice&idx=2;1;...
	local url = "type=deletedevice&idx=" .. tostring(idx)
	local ok, json, result, respcode, respheaders, respstatus = doAPICall(url)
	return ok, json
end

function deleteGroupScene(idx)
	-- type=deletescene&idx=1
	local url = "type=deletescene&idx=" .. tostring(idx)
	local ok, json, result, respcode, respheaders, respstatus = doAPICall(url)
	return ok, json
end

function getDevice(idx)
	--type=devices&rid=15
	local url = "type=devices&rid=" .. idx
	local ok, json, result, respcode, respheaders, respstatus = doAPICall(url)
	if (ok and json.result ~= nil and _.size(json.result) == 1) then
		return ok, json.result[1]
	end
	return false, nil
end

function getAllDevices()
	-- type=devices&displayhidden=1&displaydisabled=1&filter=all&used=all
	local url = "type=devices&displayhidden=1&displaydisabled=1&filter=all&used=all"
	local ok, json, result, respcode, respheaders, respstatus = doAPICall(url)
	assert.is_true(ok)
	return ok, json, result
end

function deleteAllDevices()
	local ok, json, result = getAllDevices()

	if (ok) then
		if (_.size(json.result) > 0) then
			for i, device in pairs(json.result) do
				local ok, json
				if (device.Type == 'Scene' or device.Type == 'Group') then
					ok, json = deleteGroupScene(device.idx)
				else
					ok, json = deleteDevice(device.idx)
				end

				if (not ok) then
					print('Failed to remove device: ' .. device.Name)
				end
				assert.is_true(ok)
			end
		end
	end
end

function deleteVariable(idx)
	--type=command&param=deleteuservariable&idx=1
	local url = "type=command&param=deleteuservariable&idx=" .. tostring(idx)
	local ok, json, result, respcode, respheaders, respstatus = doAPICall(url)
	return ok, json
end

function deleteAllVariables()
	-- type=command&param=getuservariables
	local url = "type=command&param=getuservariables"
	local ok, json, result, respcode, respheaders, respstatus = doAPICall(url)
	assert.is_true(ok)
	if (ok) then
		if (_.size(json.result) > 0) then
			for i, variable in pairs(json.result) do
				local ok, json
				ok, json = deleteVariable(variable.idx)

				if (not ok) then
					print('Failed to remove variable: ' .. variable.Name)
				end
				assert.is_true(ok)
			end
		end
	end
end

function deleteHardware(idx)
	-- http://localhost:8080/json.htm?type=command&param=deletehardware&idx=2
	local url = "type=command&param=deletehardware&idx=" .. tostring(idx)
	local ok, json, result, respcode, respheaders, respstatus = doAPICall(url)
	return ok, json
end

function deleteAllHardware()
	-- http://localhost:8080/json.htm?type=hardware
	local url = "type=hardware"
	local ok, json, result, respcode, respheaders, respstatus = doAPICall(url)
	assert.is_true(ok)
	if (ok) then
		if (_.size(json.result) > 0) then
			for i, hw in pairs(json.result) do
				local ok, json = deleteHardware(hw.idx)
				if (not ok) then
					print('Failed to remove hardware: ' .. hw.Name)
				end
				assert.is_true(ok)
			end
		end
	end
end

function deleteScript(idx)
	--type=events&param=delete&event=1
	local url = "type=events&param=delete&event=" .. tostring(idx)
	local ok, json, result, respcode, respheaders, respstatus = doAPICall(url)
	return ok, json
end

function deleteAllScripts()
	-- type=events&param=list
	local url = "type=events&param=list"
	local ok, json, result, respcode, respheaders, respstatus = doAPICall(url)
	assert.is_true(ok)
	if (ok) then
		if (_.size(json.result) > 0) then
			for i, script in pairs(json.result) do
				local ok, json
				ok, json = deleteScript(script.id)

				if (not ok) then
					print('Failed to remove script: ' .. script.Name)
				end
				assert.is_true(ok)
			end
		end
	end
end

function updateSwitch(idx, name, description, switchType)
	--http://localhost:8080/json.htm?type=setused&idx=1&name=vdSwitch&description=des&strparam1=&strparam2=&protected=false&switchtype=0&customimage=0&used=true&addjvalue=0&addjvalue2=0&options=
	local url = "type=setused&idx=" ..
			tostring(idx) ..
			"&name=" .. name ..
			"&description=" .. description ..
			"&strparam1=&strparam2=&protected=false&" ..
			"switchtype=" .. tostring(switchType) .. "&customimage=0&used=true&addjvalue=0&addjvalue2=0&options="
	local ok, json, result, respcode, respheaders, respstatus = doAPICall(url)
	return ok
end

function switch(idx, cmd)
	--type=command&param=switchlight&idx=39&switchcmd=On&level=0&passcode=
	local url = "type=command&param=switchlight&switchcmd=" .. cmd .. "&level=0&idx=" .. tostring(idx)
	local ok, json, result, respcode, respheaders, respstatus = doAPICall(url)
	return ok
end

function dimTo(idx, cmd, level)
	--type=command&param=switchlight&idx=39&switchcmd=On&level=0&passcode=
	local url = "type=command&param=switchlight&switchcmd=" .. cmd .. "&level=" .. tostring(level) .. "&idx=" .. tostring(idx)
	local ok, json, result, respcode, respheaders, respstatus = doAPICall(url)
	return ok
end

function switchGroup(idx, cmd)
	--http://localhost:8080/json.htm?type=command&param=switchscene&idx=2&switchcmd=On&passcode=
	local url = "type=command&param=switchscene&idx=" .. tostring(idx) .. "&switchcmd=" .. cmd .. "&passcode="
	local ok, json, result, respcode, respheaders, respstatus = doAPICall(url)
	return ok
end

function addSceneDevice(sceneIdx, devIdx)
	-- http://localhost:8080/json.htm?type=command&param=addscenedevice&idx=2&isscene=false&devidx=1&command=On&level=100&hue=0&ondelay=&offdelay=
	local url = "type=command&param=addscenedevice&idx=" .. tostring(sceneIdx) .. "&isscene=false&devidx=" .. tostring(devIdx) .. "&command=On&level=100&hue=0&ondelay=&offdelay="
	local ok, json, result, respcode, respheaders, respstatus = doAPICall(url)

	return ok
end

function initSettings()
	local response = {}
	local reqbody = "Language=en&Themes=default&Title=Domoticz&Latitude=52.298665&Longitude=5.629619&DashboardType=0&AllowWidgetOrdering=on&MobileType=0&WebUserName=&WebPassword=d41d8cd98f00b204e9800998ecf8427e&AuthenticationMethod=0&GuestUser=0&SecPassword=1&SecOnDelay=0&ProtectionPassword=d41d8cd98f00b204e9800998ecf8427e&WebLocalNetworks=127.0.0.1&RemoteSharedPort=6144&checkforupdates=on&ReleaseChannel=0&AcceptNewHardware=on&HideDisabledHardwareSensors=on&MyDomoticzUserId=&MyDomoticzPassword=&EnableTabLights=on&EnableTabScenes=on&EnableTabTemp=on&EnableTabWeather=on&EnableTabUtility=on&EnableTabCustom=on&LightHistoryDays=30&ShortLogDays=1&ProwlAPI=&NMAAPI=&PushbulletAPI=&PushsaferAPI=&PushsaferImage=&PushoverUser=&PushoverAPI=&PushALotAPI=&ClickatellAPI=&ClickatellUser=&ClickatellPassword=&ClickatellFrom=&ClickatellTo=&HTTPField1=&HTTPField2=&HTTPField3=&HTTPField4=&HTTPTo=&HTTPURL=https%3A%2F%2Fwww.somegateway.com%2Fpushurl.php%3Fusername%3D%23FIELD1%26password%3D%23FIELD2%26apikey%3D%23FIELD3%26from%3D%23FIELD4%26to%3D%23TO%26message%3D%23MESSAGE&HTTPPostData=&HTTPPostContentType=application%2Fjson&HTTPPostHeaders=&KodiIPAddress=224.0.0.1&KodiPort=9777&KodiTimeToLive=5&LmsPlayerMac=&LmsDuration=5&NotificationSensorInterval=43200&NotificationSwitchInterval=0&EmailFrom=&EmailTo=&EmailServer=&EmailPort=25&EmailUsername=&EmailPassword=&UseEmailInNotifications=on&WindUnit=0&TempUnit=0&DegreeDaysBaseTemperature=18.0&WeightUnit=0&EnergyDivider=1000&CostEnergy=0.2149&CostEnergyT2=0.2149&CostEnergyR1=0.0800&CostEnergyR2=0.0800&GasDivider=100&CostGas=0.6218&WaterDivider=100&CostWater=1.6473&ElectricVoltage=230&CM113DisplayType=0&SmartMeterType=0&FloorplanPopupDelay=750&FloorplanAnimateZoom=on&FloorplanShowSensorValues=on&FloorplanShowSceneNames=on&FloorplanRoomColour=Blue&FloorplanActiveOpacity=25&FloorplanInactiveOpacity=5&RandomSpread=15&SensorTimeout=60&BatterLowLevel=0&LogFilter=&LogFileName=&LogLevel=0&DoorbellCommand=0&RaspCamParams=-w+800+-h+600+-t+1&UVCParams=-S80+-B128+-C128+-G80+-x800+-y600+-q100&LogEventScriptTrigger=on&DzVentsLogLevel=3&EnableEventScriptSystem=on&DisableDzVentsSystem=on"

	local url = BASE_URL .. '/storesettings.webem'
	local result, respcode, respheaders, respstatus = http.request {
		method = "POST",
		source = ltn12.source.string(reqbody),
		url = url,
		headers = {
			['Connection'] = 'keep-alive',
			['Content-Length'] = _.size(reqbody),
			['Pragma'] = 'no-cache',
			['Cache-Control'] = 'no-cache',
			['Origin'] = BASE_URL,
			['User-Agent'] = 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_5) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/59.0.3071.115 Safari/537.36',
			['Content-Type'] = 'application/x-www-form-urlencoded; charset=UTF-8',
			['Accept'] = '*/*',
			['X-Requested-With'] = 'XMLHttpRequest',
			['DNT'] = '1',
			['Referer'] = BASE_URL,
		},
		sink = ltn12.sink.table(response)
	}

	local _json = table.concat(response)
	local ok = (respcode == 200)

	return ok, result, respcode, respheaders, respstatus

end

function setDisarmed()
	-- http://localhost:8080/json.htm?type=command&param=setsecstatus&secstatus=0&seccode=c4ca4238a0b923820dcc509a6f75849b
	local url = "type=command&param=setsecstatus&secstatus=0&seccode=c4ca4238a0b923820dcc509a6f75849b"
	local ok, json, result, respcode, respheaders, respstatus = doAPICall(url)
	return ok
end

function addSecurityPanel()
	--http://localhost:8080/json.htm?type=setused&idx=42&name=secPanel&used=true&maindeviceidx=
	local url = "type=setused&idx=" .. tostring(SECPANEL_INDEX) .. "&name=secPanel&used=true&maindeviceidx="
	local ok, json, result, respcode, respheaders, respstatus = doAPICall(url)
	return ok
end

function createScript(name, code)
	local response = {}
	local reqbody = 'name=' .. name ..'&' ..
			'interpreter=dzVents&' ..
			'eventtype=All&' ..
			'xml=' .. tostring(code) .. '&' ..
			--'xml=return%20%7B%0A%09active%20%3D%20false%2C%0A%09on%20%3D%20%7B%0A%09%09timer%20%3D%20%7B%7D%0A%09%7D%2C%0A%09execute%20%3D%20function(domoticz%2C%20device)%0A%09end%0A%7D&' ..
			'logicarray=&' ..
			'eventid=&' ..
			'eventstatus=1&' ..
			'editortheme=ace%2Ftheme%2Fxcode'
	local url = BASE_URL .. '/event_create.webem'
	local result, respcode, respheaders, respstatus = http.request {
		method = "POST",
		source = ltn12.source.string(reqbody),
		url = url,
		headers = {
			['Connection'] = 'keep-alive',
			['Pragma'] = 'no-cache',
			['Cache-Control'] = 'no-cache',
			['Content-Length'] = _.size(reqbody),
			['Origin'] = BASE_URL,
			['User-Agent'] = 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_5) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/59.0.3071.115 Safari/537.36',
			['Content-Type'] = 'application/x-www-form-urlencoded; charset=UTF-8',
			['Accept'] = '*/*',
			['X-Requested-With'] = 'XMLHttpRequest',
			['DNT'] = '1',
			['Referer'] = BASE_URL,
		},
		sink = ltn12.sink.table(response)
	}

	local _json = table.concat(response)
	local ok = (respcode == 200)

	return ok, result, respcode, respheaders, respstatus

end

describe('Integration test', function ()

	setup(function()
		deleteAllDevices()
		deleteAllVariables()
		deleteAllHardware()
		deleteAllScripts()
	end)

	teardown(function()
		os.remove(generatedScriptTargetPath .. 'stage1.lua')
		os.remove(getScriptTargetPath('stage2.lua'))
		os.remove(getScriptTargetPath('silent.lua'))
		os.remove(getScriptTargetPath('vdSwitchDimmer.lua'))
		os.remove(getScriptTargetPath('vdRepeatSwitch.lua'))
		os.remove(getScriptTargetPath('secArmedAway.lua'))
		os.remove(getScriptTargetPath('varString.lua'))
		os.remove(getScriptTargetPath('scScene.lua'))
		os.remove(getScriptTargetPath('some_module.lua'))
		os.remove(getScriptTargetPath('global_data.lua'))
		os.remove(dataTargetPath .. '__data_global_data.lua')
		os.remove(dataTargetPath .. '__data_secArmedAway.lua')
	end)

	before_each(function()

	end)

	after_each(function() end)

	local dummyIdx
	local stage1TriggerIdx
	local endResultsIdx
	local switchDimmerResultsIdx
	local varStringResultsIdx
	local secArmedAwayIdx
	local scSceneResultsIdx
	local switchSilentResultsIdx

--	it('a', function() end)

	describe('Settings', function()

		it('Should initialize settings', function()

			local ok, result, respcode, respheaders, respstatus = initSettings()

			assert.is_true(ok)

		end)

	end)

	describe('Hardware', function()
		it('should create dummy hardware', function()
			local ok
			ok, dummyIdx = createHardware('dummy', DUMMY_HW)
			assert.is_true(ok)
		end)
	end)

	describe('Devices', function()

		it('should create a virtual switch', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.SWITCH[2], VIRTUAL_DEVICES.SWITCH[1])
			assert.is_true(ok)
			ok = updateSwitch(idx, VIRTUAL_DEVICES.SWITCH[2], 'desc%20' .. VIRTUAL_DEVICES.SWITCH[2], SWITCH_TYPES.ON_OFF)
			assert.is_true(ok)
		end)

		it('should create a air quality device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.AIR_QUALITY[2], VIRTUAL_DEVICES.AIR_QUALITY[1])
			assert.is_true(ok)
		end)

		it('should create an alert device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.ALERT[2], VIRTUAL_DEVICES.ALERT[1])
			assert.is_true(ok)
		end)

		it('should create an AMPERE_3_PHASE device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.AMPERE_3_PHASE[2], VIRTUAL_DEVICES.AMPERE_3_PHASE[1])
			assert.is_true(ok)
		end)
		it('should create an AMPERE_1_PHASE device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.AMPERE_1_PHASE[2], VIRTUAL_DEVICES.AMPERE_1_PHASE[1])
			assert.is_true(ok)
		end)
		it('should create an BAROMETER device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.BAROMETER[2], VIRTUAL_DEVICES.BAROMETER[1])
			assert.is_true(ok)
		end)
		it('should create an COUNTER device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.COUNTER[2], VIRTUAL_DEVICES.COUNTER[1])
			assert.is_true(ok)
		end)
		it('should create an COUNTER_INCREMENTAL device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.COUNTER_INCREMENTAL[2], VIRTUAL_DEVICES.COUNTER_INCREMENTAL[1])
			assert.is_true(ok)
		end)
		it('should create an CUSTOM_SENSOR device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.CUSTOM_SENSOR[2], VIRTUAL_DEVICES.CUSTOM_SENSOR[1], 'axis')
			assert.is_true(ok)
		end)
		it('should create an DISTANCE device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.DISTANCE[2], VIRTUAL_DEVICES.DISTANCE[1])
			assert.is_true(ok)
		end)
		it('should create an ELECTRIC_INSTANT_COUNTER device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.ELECTRIC_INSTANT_COUNTER[2], VIRTUAL_DEVICES.ELECTRIC_INSTANT_COUNTER[1])
			assert.is_true(ok)
		end)
		it('should create an GAS device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.GAS[2], VIRTUAL_DEVICES.GAS[1])
			assert.is_true(ok)
		end)
		it('should create an HUMIDITY device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.HUMIDITY[2], VIRTUAL_DEVICES.HUMIDITY[1])
			assert.is_true(ok)
		end)
		it('should create an LEAF_WETNESS device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.LEAF_WETNESS[2], VIRTUAL_DEVICES.LEAF_WETNESS[1])
			assert.is_true(ok)
		end)
		it('should create an LUX device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.LUX[2], VIRTUAL_DEVICES.LUX[1])
			assert.is_true(ok)
		end)
		it('should create an P1_SMART_METER_ELECTRIC device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.P1_SMART_METER_ELECTRIC[2], VIRTUAL_DEVICES.P1_SMART_METER_ELECTRIC[1])
			assert.is_true(ok)
		end)
		it('should create an PERCENTAGE device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.PERCENTAGE[2], VIRTUAL_DEVICES.PERCENTAGE[1])
			assert.is_true(ok)
		end)
		it('should create an PRESSURE_BAR device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.PRESSURE_BAR[2], VIRTUAL_DEVICES.PRESSURE_BAR[1])
			assert.is_true(ok)
		end)
		it('should create an RAIN device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.RAIN[2], VIRTUAL_DEVICES.RAIN[1])
			assert.is_true(ok)
		end)
		it('should create an RGB_SWITCH device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.RGB_SWITCH[2], VIRTUAL_DEVICES.RGB_SWITCH[1])
			assert.is_true(ok)
		end)
		it('should create an RGBW_SWITCH device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.RGBW_SWITCH[2], VIRTUAL_DEVICES.RGBW_SWITCH[1])
			assert.is_true(ok)
		end)
		it('should create an SCALE_WEIGHT device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.SCALE_WEIGHT[2], VIRTUAL_DEVICES.SCALE_WEIGHT[1])
			assert.is_true(ok)
		end)
		it('should create an SELECTOR_SWITCH device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.SELECTOR_SWITCH[2], VIRTUAL_DEVICES.SELECTOR_SWITCH[1])
			assert.is_true(ok)
		end)
		it('should create an SOIL_MOISTURE device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.SOIL_MOISTURE[2], VIRTUAL_DEVICES.SOIL_MOISTURE[1])
			assert.is_true(ok)
		end)
		it('should create an SOLAR_RADIATION device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.SOLAR_RADIATION[2], VIRTUAL_DEVICES.SOLAR_RADIATION[1])
			assert.is_true(ok)
		end)
		it('should create an SOUND_LEVEL device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.SOUND_LEVEL[2], VIRTUAL_DEVICES.SOUND_LEVEL[1])
			assert.is_true(ok)
		end)
		it('should create an TEMPERATURE device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.TEMPERATURE[2], VIRTUAL_DEVICES.TEMPERATURE[1])
			assert.is_true(ok)
		end)
		it('should create an TEMP_HUM device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.TEMP_HUM[2], VIRTUAL_DEVICES.TEMP_HUM[1])
			assert.is_true(ok)
		end)
		it('should create an TEMP_HUM_BARO device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.TEMP_HUM_BARO[2], VIRTUAL_DEVICES.TEMP_HUM_BARO[1])
			assert.is_true(ok)
		end)
		it('should create an TEXT device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.TEXT[2], VIRTUAL_DEVICES.TEXT[1])
			assert.is_true(ok)
		end)
		it('should create an THERMOSTAT_SETPOINT device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.THERMOSTAT_SETPOINT[2], VIRTUAL_DEVICES.THERMOSTAT_SETPOINT[1])
			assert.is_true(ok)
		end)
		it('should create an USAGE_ELECTRIC device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.USAGE_ELECTRIC[2], VIRTUAL_DEVICES.USAGE_ELECTRIC[1])
			assert.is_true(ok)
		end)
		it('should create an UV device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.UV[2], VIRTUAL_DEVICES.UV[1])
			assert.is_true(ok)
		end)
		it('should create an VISIBILITY device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.VISIBILITY[2], VIRTUAL_DEVICES.VISIBILITY[1])
			assert.is_true(ok)
		end)
		it('should create an VOLTAGE device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.VOLTAGE[2], VIRTUAL_DEVICES.VOLTAGE[1])
			assert.is_true(ok)
		end)
		it('should create an WATERFLOW device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.WATERFLOW[2], VIRTUAL_DEVICES.WATERFLOW[1])
			assert.is_true(ok)
		end)
		it('should create an WIND device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.WIND[2], VIRTUAL_DEVICES.WIND[1])
			assert.is_true(ok)
		end)
		it('should create an WIND_TEMP_CHILL device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.WIND_TEMP_CHILL[2], VIRTUAL_DEVICES.WIND_TEMP_CHILL[1])
			assert.is_true(ok)
		end)
		it('should create a dimmer', function()
			local ok, idx = createVirtualDevice(dummyIdx, 'vdSwitchDimmer', VIRTUAL_DEVICES.SWITCH[1])
			assert.is_true(ok)
			ok = updateSwitch(idx, 'vdSwitchDimmer', 'desc%20vdSwitchDimmer', SWITCH_TYPES.DIMMER)
			assert.is_true(ok)
			ok = dimTo(idx, 'Set%20Level', 34) -- will end up like 33% for some weird reason
			assert.is_true(ok)
		end)
		it('should create an TEMP_BARO device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.TEMP_BARO[2], VIRTUAL_DEVICES.TEMP_BARO[1])
			assert.is_true(ok)
		end)
		it('should create an silent device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.SILENT_SWITCH[2], VIRTUAL_DEVICES.SILENT_SWITCH[1])
			assert.is_true(ok)
		end)
		it('should create an temp sensor device that will be update via the API', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.API_TEMP[2], VIRTUAL_DEVICES.API_TEMP[1])
			assert.is_true(ok)
		end)

		it('should create an repeat device', function()
			local ok, idx = createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.REPEAT_SWITCH[2], VIRTUAL_DEVICES.REPEAT_SWITCH[1])
			assert.is_true(ok)
		end)

	end)

	describe('Groups and scenes', function()

		it('should create a scene', function()
			-- first create switch to be put in the scene
			local ok
			local switchIdx
			local sceneIdx  = 1 -- api doesn't return the idx so we assume this is 1
			ok, switchIdx = createVirtualDevice(dummyIdx, 'sceneSwitch1', 6)
			assert.is_true(ok)

			ok = createScene('scScene')
			assert.is_true(ok)


			ok = addSceneDevice(sceneIdx, switchIdx)
			assert.is_true(ok)
		end)

		it('should create a group', function()
			local ok
			local switchIdx
			local groupIdx = 2

			-- first create switch to be put in the group
			ok, switchIdx = createVirtualDevice(dummyIdx, 'groupSwitch1', 6)
			assert.is_true(ok)

			ok = createGroup('gpGroup')
			assert.is_true(ok)

			ok = addSceneDevice(groupIdx, switchIdx)
			assert.is_true(ok)

		end)

		it('should create a silent scene', function()
			-- scene that will be switched on with .silent()
			-- it should not trigger a script
			local ok
			local switchIdx
			local sceneIdx = 3 -- api doesn't return the idx so we assume this is 1
			ok, switchIdx = createVirtualDevice(dummyIdx, 'sceneSilentSwitch1', 6)
			assert.is_true(ok)

			ok = createScene('scSilentScene')
			assert.is_true(ok)

			ok = addSceneDevice(sceneIdx, switchIdx)
			assert.is_true(ok)
		end)

		it('should create a silent group', function()
			local ok
			local switchIdx
			local groupIdx = 4

			-- first create switch to be put in the group
			ok, switchIdx = createVirtualDevice(dummyIdx, 'groupSilentSwitch1', 6)
			assert.is_true(ok)

			ok = createGroup('gpSilentGroup')
			assert.is_true(ok)

			ok = addSceneDevice(groupIdx, switchIdx)
			assert.is_true(ok)
		end)
	end)

	describe('Variables', function()

		it('should create an integer variable', function()
			local ok, idx = createVariable(VAR_TYPES.INT[2], VAR_TYPES.INT[1], VAR_TYPES.INT[3])
			assert.is_true(ok)
		end)

		it('should create an FLOAT variable', function()
			local ok, idx = createVariable(VAR_TYPES.FLOAT[2], VAR_TYPES.FLOAT[1], VAR_TYPES.FLOAT[3])
			assert.is_true(ok)
		end)

		it('should create an STRING variable', function()
			local ok, idx = createVariable(VAR_TYPES.STRING[2], VAR_TYPES.STRING[1], VAR_TYPES.STRING[3])
			assert.is_true(ok)
		end)

		it('should create an DATE variable', function()
			local ok, idx = createVariable(VAR_TYPES.DATE[2], VAR_TYPES.DATE[1], VAR_TYPES.DATE[3])
			assert.is_true(ok)
		end)

		it('should create an TIME variable', function()
			local ok, idx = createVariable(VAR_TYPES.TIME[2], VAR_TYPES.TIME[1], VAR_TYPES.TIME[3])
			assert.is_true(ok)
		end)

		it('should create an silent variable', function()
			-- doesn't create an event when changed
			local ok, idx = createVariable(VAR_TYPES.SILENT[2], VAR_TYPES.SILENT[1], VAR_TYPES.SILENT[3])
			assert.is_true(ok)
		end)
	end)

	describe('Preparing security panel', function ()
		it('Should create a security panel', function()
			local ok = setDisarmed()
			assert.is_true(ok)

			ok = addSecurityPanel()
			assert.is_true(ok)

		end)

	end)

	describe('Preparing scripts and triggers', function()

		it('Should create a dzVents script', function()
			local stage1Script = File.read('./stage1.lua')

			local ok, result, respcode, respheaders, respstatus = createScript('stage1', stage1Script)
			assert.is_true(ok)
		end)

		it('Should move stage2 script in place', function()
			copyScript('stage2.lua')
		end)

		it('Should move vdSwitchDimmer script in place', function()
			copyScript('vdSwitchDimmer.lua')
		end)

		it('Should move vdRepeatSwitch script in place', function()
			copyScript('vdRepeatSwitch.lua')
		end)

		it('Should move varString script in place', function()
			copyScript('varString.lua')
		end)

		it('Should move scScene script in place', function()
			copyScript('scScene.lua')
		end)

		it('Should move secArmedAway script in place', function()
			copyScript('secArmedAway.lua')
		end)

		it('Should move globaldata script in place', function()
			copyScript('global_data.lua')
		end)

		it('Should move silent script in place', function()
			copyScript('silent.lua')
		end)

		it('Should move a module in place', function()
			copyScript('some_module.lua')
		end)

		it('Should create the stage1 trigger switch', function()
			local ok
			ok, stage1TriggerIdx = createVirtualDevice(dummyIdx, 'stage1Trigger', VIRTUAL_DEVICES.SWITCH[1])
			assert.is_true(ok)
		end)

		it('Should create the stage2 trigger switch', function()
			local ok, idx = createVirtualDevice(dummyIdx, 'stage2Trigger', VIRTUAL_DEVICES.SWITCH[1])
			assert.is_true(ok)
		end)

		it('Should create results for vdSwitchDimmer script ', function()
			ok, switchDimmerResultsIdx = createVirtualDevice(dummyIdx, 'switchDimmerResults', VIRTUAL_DEVICES.TEXT[1])
			assert.is_true(ok)
		end)

		it('Should create results for varString script ', function()
			ok, varStringResultsIdx = createVirtualDevice(dummyIdx, 'varStringResults', VIRTUAL_DEVICES.TEXT[1])
			assert.is_true(ok)
		end)

		it('Should create results for secArmedAway script ', function()
			ok, secArmedAwayIdx = createVirtualDevice(dummyIdx, 'secArmedAwayResults', VIRTUAL_DEVICES.TEXT[1])
			assert.is_true(ok)
		end)

		it('Should create results for scScene script ', function()
			ok, scSceneResultsIdx = createVirtualDevice(dummyIdx, 'scSceneResults', VIRTUAL_DEVICES.TEXT[1])
			assert.is_true(ok)
		end)

		it('Should create results for silent script', function()
			ok, switchSilentResultsIdx = createVirtualDevice(dummyIdx, 'switchSilentResults', VIRTUAL_DEVICES.SWITCH[1])
			assert.is_true(ok)
		end)

		it('Should create the final results text device', function()
			local ok
			ok, endResultsIdx = createVirtualDevice(dummyIdx, 'endResult', VIRTUAL_DEVICES.TEXT[1])
			assert.is_true(ok)
		end)

	end)

	describe('Start the tests', function()

		it('Should all just work fine', function()

			local ok = switch(stage1TriggerIdx, 'On')

			assert.is_true(ok)

		end)

		it('Should have succeeded', function()

			socket.sleep(23) -- the trigger for stage 2 has a delay set to 4 seconds (afterSec(4))

			local switchDimmerResultsDevice
			local varStringResultsDevice
			local secArmedAwayDevice
			local scSceneResultsDevice
			local switchSilentResultsDevice
			local ok = false
			local endResultsDevice

			ok, endResultsDevice = getDevice(endResultsIdx)
			assert.is_true(ok)
			ok = false
			ok, switchDimmerResultsDevice = getDevice(switchDimmerResultsIdx)
			assert.is_true(ok)
			ok = false
			ok, varStringResultsDevice = getDevice(varStringResultsIdx)
			assert.is_true(ok)
			ok = false
			ok, secArmedAwayDevice = getDevice(secArmedAwayIdx)
			assert.is_true(ok)
			ok = false
			ok, scSceneResultsDevice = getDevice(scSceneResultsIdx)
			assert.is_true(ok)
			ok = false
			ok, switchSilentResultsDevice = getDevice(switchSilentResultsIdx)
			assert.is_true(ok)

			assert.is_same('SUCCEEDED', endResultsDevice['Data'])
			assert.is_same('SUCCEEDED', switchDimmerResultsDevice['Data'])
			assert.is_same('SUCCEEDED', varStringResultsDevice['Data'])
			assert.is_same('SUCCEEDED', secArmedAwayDevice['Data'])
			assert.is_same('SUCCEEDED', scSceneResultsDevice['Data'])
			assert.is_same('Off', switchSilentResultsDevice['Status'])

		end)

		it('NOTE', function()
			print('DONT FORGET TO SWITCH OFF TESTMODE IN dVents.lua !!!!!')
		end)


	end)


end);