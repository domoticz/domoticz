package.path = package.path ..
		";../?.lua;../device-adapters/?.lua;./data/?.lua;../../../scripts/dzVents/generated_scripts/?.lua;" ..
		"../../../scripts/lua/?.lua"

local TestTools = require('domoticzTestTools')('8080', true)
local socket = require("socket")

local _ = require 'lodash'

local DUMMY_HW = 15

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
	AMPERE_1_PHASE = {19, 'vdAmpere1'},
	AMPERE_3_PHASE = {9, 'vdAmpere3'},
	API_TEMP = { 80, 'vdAPITemperature' },
	BAROMETER = {11, 'vdBarometer'},
	CANCELLED_REPEAT_SWITCH = { 6, 'vdCancelledRepeatSwitch' },
	COUNTER = {113, 'vdCounter'},
	COUNTER_INCREMENTAL = {14, 'vdCounterIncremental'},
	CUSTOM_SENSOR = {1004, 'vdCustomSensor'},
	DESCRIPTION_SWITCH = { 6, 'vdDescriptionSwitch' },
	DISTANCE = {13, 'vdDistance'},
	ELECTRIC_INSTANT_COUNTER = {18, 'vdElectricInstanceCounter'},
	GAS = {3, 'vdGas'},
	HTTP_SWITCH = { 6, 'vdHTTPSwitch' },
	HUMIDITY = {81, 'vdHumidity'},
	LEAF_WETNESS = {16, 'vdLeafWetness'},
	LUX = {246, 'vdLux'},
	P1_SMART_METER_ELECTRIC = {250, 'vdP1SmartMeterElectric'},
	PERCENTAGE = {2, 'vdPercentage'},
	PRESSURE_BAR = {1, 'vdPressureBar'},
	RAIN = {85, 'vdRain'},
	REPEAT_SWITCH = { 6, 'vdRepeatSwitch' },
	RGB_SWITCH = {241, 'vdRGBSwitch'},
	RGBW_SWITCH = {1003, 'vdRGBWSwitch'},
	SCALE_WEIGHT = {93, 'vdScaleWeight'},
	SELECTOR_SWITCH = {1002, 'vdSelectorSwitch'},
	SETICON_SWITCH = {1002, 'vdSetIconSwitch'},
	SETVALUES_SENSOR = {1004, 'vdSetValueSensor'},
	SILENT_SWITCH = { 6, 'vdSilentSwitch' },
	SOIL_MOISTURE = {15, 'vdSoilMoisture'},
	SOLAR_RADIATION = {20, 'vdSolarRadiation'},
	SOUND_LEVEL = {10, 'vdSoundLevel'},
	SWITCH = {6, 'vdSwitch'},
	QUIET_ON_SWITCH = {6, 'vdQuietOnSwitch'},
	QUIET_OFF_SWITCH = {6, 'vdQuietOffSwitch'},
	TEMP_BARO = { 247, 'vdTempBaro' },
	TEMP_HUM = {82, 'vdTempHum'},
	TEMP_HUM_BARO = {84, 'vdTempHumBaro'},
	TEMPERATURE = {80, 'vdTemperature'},
	TEXT = {5, 'vdText'},
	THERMOSTAT_SETPOINT = {8, 'vdThermostatSetpoint'},
    UPDATEDOCUMENT_SWITCH = { 6, 'vdDocumentationSwitch' },
	USAGE_ELECTRIC = {248, 'vdUsageElectric'},
	UV = {87, 'vdUV'},
	VISIBILITY = {12, 'vdVisibility'},
	VOLTAGE = {4, 'vdVoltage'},
	WATERFLOW = {1000, 'vdWaterflow'},
	WIND = {86, 'vdWind'},
	WIND_TEMP_CHILL = {1001, 'vdWindTempChill'},
	-- increment SECPANEL_INDEX_CHECK when adding a new one !!!!!!!!!!
}

local SECPANEL_INDEX_CHECK = 60
local SECPANEL_INDEX = TestTools.tableEntries(VIRTUAL_DEVICES) + 10 -- 10 is number of groups / scenes + managed counter + dimmer switch + ?

local VAR_TYPES = {
	INT = {0, 'varInteger', 42},
	FLOAT = {1, 'varFloat', 42.11},
	STRING = {2, 'varString', 'Somestring'},
	DATE = {3, 'varDate', '31/12/2017'},
	TIME = {4, 'varTime', '23:59'},
	SILENT = { 0, 'varSilent', 1 },
	CANCELLED = { 0, 'varCancelled', 0},
	UPDATEDOCUMENT = { 0, 'varUpdateDocument', 88},
}

local getResultsFromfile = function(file)
	local utils = require('Utils')
	local results = assert(io.open(file, "r"))
	resTable = utils.fromJSON(results:read("*all"))
	results:close()
	return resTable
end


describe('Integration test', 
	function ()
		setup(function()
		assert.is_true(TestTools.deleteAllDevices())
		assert.is_true(TestTools.deleteAllVariables())
		assert.is_true(TestTools.deleteAllHardware())
		assert.is_true(TestTools.deleteAllCameras())
		assert.is_true(TestTools.deleteAllScripts())
	end)

	teardown(function()
		TestTools.removeDataFile('__data_global_data.lua')
		TestTools.removeDataFile('__data_secArmedAway.lua')
		TestTools.removeFSScript('descriptionScript.lua')
		TestTools.removeFSScript('IconScript.lua')
		TestTools.removeFSScript('global_data.lua')
		TestTools.removeFSScript('httpResponseScript.lua')
		TestTools.removeFSScript('scCancelledScene.lua')
		TestTools.removeFSScript('scScene.lua')
		TestTools.removeFSScript('secArmedAway.lua')
		TestTools.removeFSScript('silent.lua')
		TestTools.removeFSScript('some_module.lua')
		TestTools.removeFSScript('stage2.lua')
		TestTools.removeFSScript('varCancelled.lua')
		TestTools.removeFSScript('varString.lua')
		TestTools.removeFSScript('vdCancelledRepeatSwitch.lua')
		TestTools.removeFSScript('vdRepeatSwitch.lua')
		TestTools.removeFSScript('vdSwitchDimmer.lua')
		TestTools.removeFSScript('scriptTestUpdatedDocumentation.lua')
		TestTools.removeGUIScript('stage1.lua')
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
	local switchQuietResultsIdx

	-- it('a', function() end)
	describe('Settings', function()
		it('Should initialize settings with enabled IFTTT', function()
			local IFTTTEnabled = "on"
			local ok, result, respcode, respheaders, respstatus = TestTools.initSettings(IFTTTEnabled)
			assert.is_true(ok)
		end)
	end)
	
	describe('Hardware', function()
		it('should create dummy hardware', function()
			local ok
			ok, dummyIdx = TestTools.createHardware('dummy', DUMMY_HW)
			assert.is_true(ok)
		end)
	end)

	describe('Camera ID', function()
		it('should have proper Camera ID', function()
			assert.are.equal(SECPANEL_INDEX, SECPANEL_INDEX_CHECK)
		end)
	end)

	describe('Camera', function()
		it('should create a camera', function()
			local ok
			ok = TestTools.createCamera('dummy', DUMMY_HW)
			assert.is_true(ok)
		end)
	end)

	describe('Devices', function()
		it('should create a virtual switch', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.SWITCH[2], VIRTUAL_DEVICES.SWITCH[1])
			assert.is_true(ok)
			ok = TestTools.updateSwitch(idx, VIRTUAL_DEVICES.SWITCH[2], 'desc%20' .. VIRTUAL_DEVICES.SWITCH[2], SWITCH_TYPES.ON_OFF)
			assert.is_true(ok)
		end)

		it('should create a air quality device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.AIR_QUALITY[2], VIRTUAL_DEVICES.AIR_QUALITY[1])
			assert.is_true(ok)
		end)

		it('should create an alert device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.ALERT[2], VIRTUAL_DEVICES.ALERT[1])
			assert.is_true(ok)
		end)

		it('should create an AMPERE_3_PHASE device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.AMPERE_3_PHASE[2], VIRTUAL_DEVICES.AMPERE_3_PHASE[1])
			assert.is_true(ok)
		end)

		it('should create an AMPERE_1_PHASE device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.AMPERE_1_PHASE[2], VIRTUAL_DEVICES.AMPERE_1_PHASE[1])
			assert.is_true(ok)
		end)

		it('should create a BAROMETER device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.BAROMETER[2], VIRTUAL_DEVICES.BAROMETER[1])
			assert.is_true(ok)
		end)

		it('should create a COUNTER device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.COUNTER[2], VIRTUAL_DEVICES.COUNTER[1])
			assert.is_true(ok)
		end)

		it('should create a COUNTER_INCREMENTAL device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.COUNTER_INCREMENTAL[2], VIRTUAL_DEVICES.COUNTER_INCREMENTAL[1])
			assert.is_true(ok)
		end)

		it('should create a CUSTOM_SENSOR device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.CUSTOM_SENSOR[2], VIRTUAL_DEVICES.CUSTOM_SENSOR[1], 'axis')
			assert.is_true(ok)
		end)

		it('should create a DISTANCE device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.DISTANCE[2], VIRTUAL_DEVICES.DISTANCE[1])
			assert.is_true(ok)
		end)

		it('should create an ELECTRIC_INSTANT_COUNTER device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.ELECTRIC_INSTANT_COUNTER[2], VIRTUAL_DEVICES.ELECTRIC_INSTANT_COUNTER[1])
			assert.is_true(ok)
		end)

		it('should create a GAS device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.GAS[2], VIRTUAL_DEVICES.GAS[1])
			assert.is_true(ok)
		end)

		it('should create a HUMIDITY device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.HUMIDITY[2], VIRTUAL_DEVICES.HUMIDITY[1])
			assert.is_true(ok)
		end)

		it('should create a LEAF_WETNESS device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.LEAF_WETNESS[2], VIRTUAL_DEVICES.LEAF_WETNESS[1])
			assert.is_true(ok)
		end)

		it('should create a LUX device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.LUX[2], VIRTUAL_DEVICES.LUX[1])
			assert.is_true(ok)
		end)

		it('should create a P1_SMART_METER_ELECTRIC device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.P1_SMART_METER_ELECTRIC[2], VIRTUAL_DEVICES.P1_SMART_METER_ELECTRIC[1])
			assert.is_true(ok)
		end)

		it('should create a PERCENTAGE device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.PERCENTAGE[2], VIRTUAL_DEVICES.PERCENTAGE[1])
			assert.is_true(ok)
		end)

		it('should create a PRESSURE_BAR device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.PRESSURE_BAR[2], VIRTUAL_DEVICES.PRESSURE_BAR[1])
			assert.is_true(ok)
		end)

		it('should create a RAIN device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.RAIN[2], VIRTUAL_DEVICES.RAIN[1])
			assert.is_true(ok)
		end)

		it('should create an RGB_SWITCH device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.RGB_SWITCH[2], VIRTUAL_DEVICES.RGB_SWITCH[1])
			assert.is_true(ok)
		end)

		it('should create an RGBW_SWITCH device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.RGBW_SWITCH[2], VIRTUAL_DEVICES.RGBW_SWITCH[1])
			assert.is_true(ok)
		end)

		it('should create a SCALE_WEIGHT device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.SCALE_WEIGHT[2], VIRTUAL_DEVICES.SCALE_WEIGHT[1])
			assert.is_true(ok)
		end)

		it('should create a SELECTOR_SWITCH device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.SELECTOR_SWITCH[2], VIRTUAL_DEVICES.SELECTOR_SWITCH[1])
			assert.is_true(ok)
		end)

		it('should create a SETICON_SWITCH device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.SETICON_SWITCH[2], VIRTUAL_DEVICES.SETICON_SWITCH[1])
			assert.is_true(ok)
		end)

		it('should create a SOIL_MOISTURE device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.SOIL_MOISTURE[2], VIRTUAL_DEVICES.SOIL_MOISTURE[1])
			assert.is_true(ok)
		end)

		it('should create a SOLAR_RADIATION device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.SOLAR_RADIATION[2], VIRTUAL_DEVICES.SOLAR_RADIATION[1])
			assert.is_true(ok)
		end)

		it('should create a SOUND_LEVEL device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.SOUND_LEVEL[2], VIRTUAL_DEVICES.SOUND_LEVEL[1])
			assert.is_true(ok)
		end)

		it('should create a TEMPERATURE device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.TEMPERATURE[2], VIRTUAL_DEVICES.TEMPERATURE[1])
			assert.is_true(ok)
		end)

		it('should create a TEMP_HUM device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.TEMP_HUM[2], VIRTUAL_DEVICES.TEMP_HUM[1])
			assert.is_true(ok)
		end)

		it('should create a TEMP_HUM_BARO device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.TEMP_HUM_BARO[2], VIRTUAL_DEVICES.TEMP_HUM_BARO[1])
			assert.is_true(ok)
		end)

		it('should create a TEXT device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.TEXT[2], VIRTUAL_DEVICES.TEXT[1])
			assert.is_true(ok)
		end)

		it('should create a THERMOSTAT_SETPOINT device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.THERMOSTAT_SETPOINT[2], VIRTUAL_DEVICES.THERMOSTAT_SETPOINT[1])
			assert.is_true(ok)
		end)

		it('should create a USAGE_ELECTRIC device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.USAGE_ELECTRIC[2], VIRTUAL_DEVICES.USAGE_ELECTRIC[1])
			assert.is_true(ok)
		end)

		it('should create an UV device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.UV[2], VIRTUAL_DEVICES.UV[1])
			assert.is_true(ok)
		end)

		it('should create a VISIBILITY device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.VISIBILITY[2], VIRTUAL_DEVICES.VISIBILITY[1])
			assert.is_true(ok)
		end)

		it('should create a VOLTAGE device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.VOLTAGE[2], VIRTUAL_DEVICES.VOLTAGE[1])
			assert.is_true(ok)
		end)

		it('should create a WATERFLOW device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.WATERFLOW[2], VIRTUAL_DEVICES.WATERFLOW[1])
			assert.is_true(ok)
		end)

		it('should create a WIND device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.WIND[2], VIRTUAL_DEVICES.WIND[1])
			assert.is_true(ok)
		end)

		it('should create a WIND_TEMP_CHILL device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.WIND_TEMP_CHILL[2], VIRTUAL_DEVICES.WIND_TEMP_CHILL[1])
			assert.is_true(ok)
		end)

		it('should create a DIMMER device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, 'vdSwitchDimmer', VIRTUAL_DEVICES.SWITCH[1])
			assert.is_true(ok)
			ok = TestTools.updateSwitch(idx, 'vdSwitchDimmer', 'desc%20vdSwitchDimmer', SWITCH_TYPES.DIMMER)
			assert.is_true(ok)
			ok = TestTools.dimTo(idx, 'Set%20Level', 34) -- will end up like 33% for some weird reason
			assert.is_true(ok)
		end)

		it('should create a TEMP_BARO device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.TEMP_BARO[2], VIRTUAL_DEVICES.TEMP_BARO[1])
			assert.is_true(ok)
		end)

		it('should create a SILENT device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.SILENT_SWITCH[2], VIRTUAL_DEVICES.SILENT_SWITCH[1])
			assert.is_true(ok)
		end)

		it('should create a TEMP SENSOR sensor device that will be updated via the API', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.API_TEMP[2], VIRTUAL_DEVICES.API_TEMP[1])
			assert.is_true(ok)
		end)

		it('should create a REPEAT device', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.REPEAT_SWITCH[2], VIRTUAL_DEVICES.REPEAT_SWITCH[1])
			assert.is_true(ok)
		end)

		it('should create an REPEAT device that will be canceled', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.CANCELLED_REPEAT_SWITCH[2], VIRTUAL_DEVICES.CANCELLED_REPEAT_SWITCH[1])
			assert.is_true(ok)
		end)

		it('should create an HTTP switch to trigger HTTP requests', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.HTTP_SWITCH[2], VIRTUAL_DEVICES.HTTP_SWITCH[1])
			assert.is_true(ok)
		end)
        
        it('should create an Update Documentationswitch to trigger test UpdatedDocumentation script', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.UPDATEDOCUMENT_SWITCH[2], VIRTUAL_DEVICES.UPDATEDOCUMENT_SWITCH[1])
			assert.is_true(ok)
		end)

		it('should create a DESCRIPTION switch to trigger description script', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.DESCRIPTION_SWITCH[2], VIRTUAL_DEVICES.DESCRIPTION_SWITCH[1])
			assert.is_true(ok)
		end)

		it('should create a SETVALUE sensor', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.SETVALUES_SENSOR[2], VIRTUAL_DEVICES.SETVALUES_SENSOR[1], 'values')
			assert.is_true(ok)
		end)

		it('should create a quietOn switch', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.QUIET_ON_SWITCH[2], VIRTUAL_DEVICES.QUIET_ON_SWITCH[1])
			assert.is_true(ok)
		end)

		it('should create a quietOff switch', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, VIRTUAL_DEVICES.QUIET_OFF_SWITCH[2], VIRTUAL_DEVICES.QUIET_OFF_SWITCH[1])
			assert.is_true(ok)
		end)

	end)

	describe('ManagedCounter', function()
	 it('should create a Managed counter', function()
			local ok
			ok = TestTools.createManagedCounter('vdManagedCounter')
			assert.is_true(ok)
		end)
	end)

	describe('Groups and scenes', function()
		-- increment SECPANEL_INDEX when adding a new one !!!!!!!!!!
		it('should create a scene', function()
			-- first create switch to be put in the scene
			local ok
			local switchIdx
			local sceneIdx = 1 -- api doesn't return the idx so we assume this is 1
			ok, switchIdx = TestTools.createVirtualDevice(dummyIdx, 'sceneSwitch1', 6)
			assert.is_true(ok)

			ok = TestTools.createScene('scScene')
			assert.is_true(ok)

			ok = TestTools.addSceneDevice(sceneIdx, switchIdx)
			assert.is_true(ok)
		end)

		it('should create a group', function()
			local ok
			local switchIdx
			local groupIdx = 2

			-- first create switch to be put in the group
			ok, switchIdx = TestTools.createVirtualDevice(dummyIdx, 'groupSwitch1', 6)
			assert.is_true(ok)

			ok = TestTools.createGroup('gpGroup')
			assert.is_true(ok)

			ok = TestTools.addSceneDevice(groupIdx, switchIdx)
			assert.is_true(ok)

		end)

		it('should create a silent scene', function()
			-- scene that will be switched on with .silent()
			-- it should not trigger a script
			local ok
			local switchIdx
			local sceneIdx = 3 -- api doesn't return the idx so we assume this is 1
			ok, switchIdx = TestTools.createVirtualDevice(dummyIdx, 'sceneSilentSwitch1', 6)
			assert.is_true(ok)

			ok = TestTools.createScene('scSilentScene')
			assert.is_true(ok)

			ok = TestTools.addSceneDevice(sceneIdx, switchIdx)
			assert.is_true(ok)
		end)

		it('should create a silent group', function()
			local ok
			local switchIdx
			local groupIdx = 4

			-- first create switch to be put in the group
			ok, switchIdx = TestTools.createVirtualDevice(dummyIdx, 'groupSilentSwitch1', 6)
			assert.is_true(ok)

			ok = TestTools.createGroup('gpSilentGroup')
			assert.is_true(ok)

			ok = TestTools.addSceneDevice(groupIdx, switchIdx)
			assert.is_true(ok)
		end)

		it('should create a scene which update will be cancelled', function()
			local ok
			local switchIdx
			local sceneIdx = 5 -- api doesn't return the idx so we assume this is 1
			ok, switchIdx = TestTools.createVirtualDevice(dummyIdx, 'sceneCancelledSwitch1', 6)
			assert.is_true(ok)

			ok = TestTools.createScene('scCancelledScene')
			assert.is_true(ok)

			ok = TestTools.addSceneDevice(sceneIdx, switchIdx)
			assert.is_true(ok)
		end)

		it('should create a scene which will get a new description', function()
			local ok
			local switchIdx
			local sceneIdx = 6
			ok, switchIdx = TestTools.createVirtualDevice(dummyIdx, 'sceneDescriptionSwitch1', 6)
			assert.is_true(ok)

			ok = TestTools.createScene('scDescriptionScene')
			assert.is_true(ok)

			ok = TestTools.addSceneDevice(sceneIdx, switchIdx)
			assert.is_true(ok)
		end)

		it('should create a group which will get a new description', function()
			local ok
			local switchIdx
			local groupIdx = 7

			ok, switchIdx = TestTools.createVirtualDevice(dummyIdx, 'groupDescriptionSwitch1', 6)
			assert.is_true(ok)

			ok = TestTools.createGroup('gpDescriptionGroup')
			assert.is_true(ok)

			ok = TestTools.addSceneDevice(groupIdx, switchIdx)
			assert.is_true(ok)
		end)
		-- increment SECPANEL_INDEX when adding a new group or scene !!!!!!!!!!
	end)

	describe('Variables', function()

		it('should create an integer variable', function()
			local ok, idx = TestTools.createVariable(VAR_TYPES.INT[2], VAR_TYPES.INT[1], VAR_TYPES.INT[3])
			assert.is_true(ok)
		end)

		it('should create an FLOAT variable', function()
			local ok, idx = TestTools.createVariable(VAR_TYPES.FLOAT[2], VAR_TYPES.FLOAT[1], VAR_TYPES.FLOAT[3])
			assert.is_true(ok)
		end)

		it('should create an STRING variable', function()
			local ok, idx = TestTools.createVariable(VAR_TYPES.STRING[2], VAR_TYPES.STRING[1], VAR_TYPES.STRING[3])
			assert.is_true(ok)
		end)

		it('should create an DATE variable', function()
			local ok, idx = TestTools.createVariable(VAR_TYPES.DATE[2], VAR_TYPES.DATE[1], VAR_TYPES.DATE[3])
			assert.is_true(ok)
		end)

		it('should create an TIME variable', function()
			local ok, idx = TestTools.createVariable(VAR_TYPES.TIME[2], VAR_TYPES.TIME[1], VAR_TYPES.TIME[3])
			assert.is_true(ok)
		end)

		it('should create an silent variable', function()
			-- doesn't create an event when changed
			local ok, idx = TestTools.createVariable(VAR_TYPES.SILENT[2], VAR_TYPES.SILENT[1], VAR_TYPES.SILENT[3])
			assert.is_true(ok)
		end)

		it('should create an variable which after-update will be cancelled', function()
			-- doesn't create an event when changed
			local ok, idx = TestTools.createVariable(VAR_TYPES.CANCELLED[2], VAR_TYPES.CANCELLED[1], VAR_TYPES.CANCELLED[3])
			assert.is_true(ok)
		end)
        
        it('should create an integer variable to hold status of updateDocumentation check', function()
			local ok, idx = TestTools.createVariable(VAR_TYPES.UPDATEDOCUMENT[2], VAR_TYPES.UPDATEDOCUMENT[1], VAR_TYPES.UPDATEDOCUMENT[3])
			assert.is_true(ok)
		end)
	end)

	describe('Preparing security panel', function ()
		it('Should create a security panel', function()
			local ok = TestTools.setDisarmed()
			assert.is_true(ok)

			ok = TestTools.addSecurityPanel(SECPANEL_INDEX)
			assert.is_true(ok)

		end)

	end)

	describe('Preparing scripts and triggers', function()

		it('Should create a dzVents script', function()
			assert.is_true(TestTools.createGUIScriptFromFile('./stage1.lua'))
		end)

		it('Should move stage2 script in place', function()
			TestTools.createFSScript('stage2.lua')
		end)

		it('Should move vdSwitchDimmer script in place', function()
			TestTools.createFSScript('vdSwitchDimmer.lua')
		end)

		it('Should move vdRepeatSwitch script in place', function()
			TestTools.createFSScript('vdRepeatSwitch.lua')
		end)

		it('Should move vdCancelledRepeatSwitch script in place', function()
			TestTools.createFSScript('vdCancelledRepeatSwitch.lua')
		end)

		it('Should move varString script in place', function()
			TestTools.createFSScript('varString.lua')
		end)

		it('Should move varCancelled script in place', function()
			TestTools.createFSScript('varCancelled.lua')
		end)

		it('Should move scScene script in place', function()
			TestTools.createFSScript('scScene.lua')
		end)

		it('Should move scCancelledScene script in place', function()
			TestTools.createFSScript('scCancelledScene.lua')
		end)

		it('Should move secArmedAway script in place', function()
			TestTools.createFSScript('secArmedAway.lua')
		end)

		it('Should move globaldata script in place', function()
			TestTools.createFSScript('global_data.lua')
		end)

		it('Should move silent script in place', function()
			TestTools.createFSScript('silent.lua')
		end)

		it('Should move quiet script in place', function()
			TestTools.createFSScript('quiet.lua')
		end)
		  
		it('Should move description script in place', function()
			TestTools.createFSScript('descriptionScript.lua')
		end)

		it('Should move IconScript in place', function()
			TestTools.createFSScript('IconScript.lua')
		end)

		it('Should move a module in place', function()
			TestTools.createFSScript('some_module.lua')
		end)

		it('Should move a httpResponse event script in place', function()
			TestTools.createFSScript('httpResponseScript.lua')
		end)
        
        it('Should move a updateDocumentation event script in place', function()
			TestTools.createFSScript('scriptTestUpdatedDocumentation.lua')
		end)

		it('Should create the stage1 trigger switch', function()
			local ok
			ok, stage1TriggerIdx = TestTools.createVirtualDevice(dummyIdx, 'stage1Trigger', VIRTUAL_DEVICES.SWITCH[1])
			assert.is_true(ok)
		end)

		it('Should create the stage2 trigger switch', function()
			local ok, idx = TestTools.createVirtualDevice(dummyIdx, 'stage2Trigger', VIRTUAL_DEVICES.SWITCH[1])
			assert.is_true(ok)
		end)

		it('Should create results for vdSwitchDimmer script ', function()
			ok, switchDimmerResultsIdx = TestTools.createVirtualDevice(dummyIdx, 'switchDimmerResults', VIRTUAL_DEVICES.TEXT[1])
			assert.is_true(ok)
		end)

		it('Should create results for varString script ', function()
			ok, varStringResultsIdx = TestTools.createVirtualDevice(dummyIdx, 'varStringResults', VIRTUAL_DEVICES.TEXT[1])
			assert.is_true(ok)
		end)

		it('Should create results for secArmedAway script ', function()
			ok, secArmedAwayIdx = TestTools.createVirtualDevice(dummyIdx, 'secArmedAwayResults', VIRTUAL_DEVICES.TEXT[1])
			assert.is_true(ok)
		end)

		it('Should create results for scScene script ', function()
			ok, scSceneResultsIdx = TestTools.createVirtualDevice(dummyIdx, 'scSceneResults', VIRTUAL_DEVICES.TEXT[1])
			assert.is_true(ok)
		end)

		it('Should create results for silent script', function()
			ok, switchSilentResultsIdx = TestTools.createVirtualDevice(dummyIdx, 'switchSilentResults', VIRTUAL_DEVICES.SWITCH[1])
			assert.is_true(ok)
		end)
		  
		  it('Should create results for quiet script', function()
			ok, switchQuietResultsIdx = TestTools.createVirtualDevice(dummyIdx, 'switchQuietResults', VIRTUAL_DEVICES.SWITCH[1])
			assert.is_true(ok)
		end)

		it('Should create the final results text device', function()
			local ok
			ok, endResultsIdx = TestTools.createVirtualDevice(dummyIdx, 'endResult', VIRTUAL_DEVICES.TEXT[1])
			assert.is_true(ok)
		end)

	end)

	describe('Start the tests', function()
		it('Should all just work fine', function()
			socket.sleep(1)
			local ok = TestTools.switch(stage1TriggerIdx, 'On')
			assert.is_true(ok)
		end)
	end)

	describe('waiting...', function()
		secondsToWait = 25
		for i=1,secondsToWait do
			it('sleeping ' .. tostring(i) .. ' of ' .. secondsToWait .. ' seconds '   , function()
				socket.sleep(1) -- 25 because of repeatAfter tests , the trigger for stage 2 has a delay set to 4 seconds (afterSec(4))
				assert.is_true(true)
			end)
			if i == 10 then 
				it('Should reinitialize settings with disabled IFTTT', function()
					local IFTTTEnabled = "%20"
					local ok, result, respcode, respheaders, respstatus = TestTools.initSettings(IFTTTEnabled)
					assert.is_true(ok)
				end)
			end
		end
	end)

	describe('Stage2', function()

		resTable = getResultsFromfile("/tmp/Stage2results.json")
		for key, value in pairs (resTable)do
			it(key, function()
				assert.is_same(true, value)
			end)
		end

		it('Devices with results should have succeeded', function()

			local switchDimmerResultsDevice
			local varStringResultsDevice
			local secArmedAwayDevice
			local scSceneResultsDevice
			local switchSilentResultsDevice
			local switchQuietResultsDevice
			local ok = false
			local endResultsDevice

			ok, endResultsDevice = TestTools.getDevice(endResultsIdx)
			assert.is_true(ok)
			ok = false
			ok, switchDimmerResultsDevice = TestTools.getDevice(switchDimmerResultsIdx)
			assert.is_true(ok)
			ok = false
			ok, varStringResultsDevice = TestTools.getDevice(varStringResultsIdx)
			assert.is_true(ok)
			ok = false
			ok, secArmedAwayDevice = TestTools.getDevice(secArmedAwayIdx)
			assert.is_true(ok)
			ok = false
			ok, scSceneResultsDevice = TestTools.getDevice(scSceneResultsIdx)
			assert.is_true(ok)
			ok = false
			ok, switchSilentResultsDevice = TestTools.getDevice(switchSilentResultsIdx)
			assert.is_true(ok)
				ok = false
			ok, switchQuietResultsDevice = TestTools.getDevice(switchQuietResultsIdx)
			assert.is_true(ok)

			assert.is_same('ENDRESULT SUCCEEDED', endResultsDevice['Data'])
			assert.is_same('DIMMER SUCCEEDED', switchDimmerResultsDevice['Data'])
			assert.is_same('STRING VARIABLE SUCCEEDED', varStringResultsDevice['Data'])
			assert.is_same('SECURITY SUCCEEDED', secArmedAwayDevice['Data'])
			assert.is_same('SCENE SUCCEEDED', scSceneResultsDevice['Data'])
			assert.is_same('Off', switchSilentResultsDevice['Status'])
			assert.is_same('On', switchQuietResultsDevice['Status'])

		end)

		-- it('NOTE', function()
		-- 	print('DONT FORGET TO SWITCH OFF TESTMODE IN dVents.lua !!!!!')
		-- end)
	end)

end);