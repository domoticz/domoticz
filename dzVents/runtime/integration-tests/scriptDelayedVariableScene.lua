local socket = require("socket");

local testLastUpdate = function(dz, trigger, expectedDifference)
	local now = dz.time.secondsSinceMidnight
	local lastUpdate = trigger.lastUpdate.secondsSinceMidnight
	local diff = (now - lastUpdate)
	print('current: ' .. now .. ', lastUpdate: ' .. lastUpdate .. ', diff: ' .. diff .. ', expected: ' .. expectedDifference)

	if ( diff ~= expectedDifference) then
		print('Difference is: ' .. tostring(diff) .. ', expected to be: ' .. tostring(expectedDifference))
		return false
	end
	print('lastUpdate is as expected')
	return true
end

return {
	active = true,
	on = {
		devices = {
			'vdScriptStart',
			'vdScriptEnd',
		},
		variables = { 'var' },
		scenes = { 'scScene' }
	},
	data = {
		varOk = {initial = false},
		sceneOk = {initial = false}
	},
	execute = function(dz, item)

		local var = dz.variables('var')
		local scene = dz.scenes('scScene')

		if (item.name == 'vdScriptStart') then

			if (not testLastUpdate(dz, var, 2)) then
				-- oops
				return
			end

			if (not testLastUpdate(dz, scene, 2)) then
				-- oops
				return
			end

			var.set('Zork is a dork').afterSec(2)
			scene.switchOn().afterSec(3)
			dz.devices('vdScriptEnd').switchOn().afterSec(4)

		end

		if (item.name == 'vdScriptEnd') then
			if (dz.data.varOk and dz.data.sceneOk) then
				dz.devices('vdScriptOK').switchOn()
			end
		end

		if (item.isVariable) then
			local res = testLastUpdate(dz, item, 4)
			if (res) then
				dz.data.varOk = true
			end
		end

		if (item.isScene) then
			local res = testLastUpdate(dz, item, 5)
			if (res) then
				dz.data.sceneOk = true
			end
		end
	end
}
