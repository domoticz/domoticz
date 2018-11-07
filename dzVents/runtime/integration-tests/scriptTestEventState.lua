local socket = require("socket");

local checkInitialLastUpate = function(item, expected)
	if (item.lastUpdate.secondsAgo < expected) then
		print('error initial lastUpdate for ' .. item.name .. ', expected less than ' .. expected .. ', got ' .. item.lastUpdate.secondsAgo)
	else
		print('initial lastUpdate for ' .. item.name .. ' ok: ' .. item.lastUpdate.secondsAgo)
	end
end

return {
	active = true,
	on = {
		devices = {
			'vdRepeatSwitch',
			'vdScriptStart',
			'vdScriptEnd',
			'vdDelay',
			'vdTempHumBaro'
		},
		variables = { 'varInt' },
		scenes = { 'scScene' }
	},
	data = {
		switchStates = {initial = ''},
		switchSecs = { initial = ''},
		temp = { initial = ''},
		tempSecs = { initial = ''},
		varStates = {initial = ''},
		varSecs = { initial = ''},
		sceneStates = {initial = ''},
		sceneSecs = { initial = ''}
	},
	execute = function(dz, item)

		local switch = dz.devices('vdRepeatSwitch')
		local scene = dz.scenes('scScene')
		local var = dz.variables('varInt')
		local temp = dz.devices('vdTempHumBaro')

		if (item.name == 'vdScriptStart') then

			-- this will trigger the sleep delay
			dz.devices('vdDelay').switchOn()
			dz.devices('vdScriptEnd').switchOn().afterSec(12)

			-- check lastUpdates
			checkInitialLastUpate(switch, 2)
			checkInitialLastUpate(var, 2)
			checkInitialLastUpate(scene, 2)
			checkInitialLastUpate(temp, 2)

			switch.switchOn().afterSec(1).forSec(1).repeatAfterSec(1, 1)
			scene.switchOn().afterSec(1).forSec(1).repeatAfterSec(1,1)
			var.set(dz.variables('varInt').value + 10).afterSec(1)
			var.set(dz.variables('varInt').value + 20).afterSec(2)
			var.set(dz.variables('varInt').value + 30).afterSec(3)
			var.set(dz.variables('varInt').value + 40).afterSec(4)

			temp.updateTempHumBaro(10, 10, dz.HUM_WET, 10, dz.BARO_STABLE).afterSec(1)
			temp.updateTempHumBaro(20, 20, dz.HUM_DRY, 20, dz.BARO_SUNNY).afterSec(2)
			temp.updateTempHumBaro(30, 30, dz.HUM_NORMAL, 30, dz.BARO_CLOUDY).afterSec(3)
			temp.updateTempHumBaro(30, 30, dz.HUM_COMFORTABLE, 30, dz.BARO_UNSTABLE).afterSec(4)
			print('initial scene state: ' .. scene.state)
		end

		if (item.name == 'vdDelay') then
			print('Delay start >>>>>>>>>>>>>>>>>>>>>')
			socket.sleep(9)
			print('Delay end >>>>>>>>>>>>>>>>>>>>>')
		end

		if (item.name == 'vdScriptEnd') then
			print('switch states: ' .. dz.data.switchStates)
			print('switch secs: ' .. dz.data.switchSecs)
			print('----')

			print('varStates: ' .. dz.data.varStates)
			print('varSecs: ' .. dz.data.varSecs)
			print('----')

			print('sceneStates: ' .. dz.data.sceneStates)
			print('sceneSecs: ' .. dz.data.sceneSecs)
			print('----')

			print('temp: ' .. dz.data.temp)
			print('tempSecs: ' .. dz.data.tempSecs)
			print('----')

			local ok = true

			if (dz.data.switchStates ~= 'OnOffOnOff') then
				print('Error: ' .. dz.data.switchStates)
				ok = false
			end
			if (dz.data.sceneStates ~= 'OnOffOnOff') then
				print('Error: ' .. dz.data.sceneStates)
				ok = false
			end
			if (dz.data.varStates ~= '10203040') then
				print('Error: ' .. dz.data.varStates)
				ok = false
			end
			if (dz.data.temp ~= '101010Wet202020Dry303030Normal303030Comfortable') then
				print('Error: ' .. dz.data.temp)
				ok = false
			end

			if ok then
				dz.devices('vdScriptOK').switchOn()
			end

		end

		if (item.name == 'varInt') then
			if (dz.data.varStates ~= '') then
				dz.data.varSecs = dz.data.varSecs .. tostring(item.lastUpdate.secondsAgo)
			end
			dz.data.varStates = dz.data.varStates  .. tostring(item.value)
		end

		if (item.name == 'scScene') then
			if (dz.data.sceneStates ~= '') then
				dz.data.sceneSecs = dz.data.sceneSecs .. tostring(item.lastUpdate.secondsAgo)
			end
			dz.data.sceneStates = dz.data.sceneStates  .. tostring(item.state)
		end

		if (item.name == 'vdRepeatSwitch') then
			if (dz.data.switchStates ~= '') then
				dz.data.switchSecs = dz.data.switchSecs .. tostring(item.lastUpdate.secondsAgo)
			end
			dz.data.switchStates = dz.data.switchStates  .. item.state
		end

		if (item.name == 'vdTempHumBaro') then
			if (dz.data.temp ~= '') then
				dz.data.tempSecs = dz.data.tempSecs .. tostring(item.lastUpdate.secondsAgo)
			end
			dz.data.temp = dz.data.temp  .. item.temperature .. item.humidity .. item.barometer .. item.humidityStatus
		end

	end
}
