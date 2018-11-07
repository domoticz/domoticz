local socket = require("socket");

return {
	active = true,
	on = {
		devices = {
			'vdSelectorSwitch',
			'vdScriptStart',
			'vdScriptEnd',
		},
	},
	data = {
		switchStates = {initial = ''},
	},
	execute = function(dz, item)

		local switch = dz.devices('vdSelectorSwitch')

		if (item.name == 'vdScriptStart') then
			--switch.switchSelector(10).silent()
			switch.switchSelector(30).forSec(2)
			dz.devices('vdScriptEnd').switchOn().afterSec(4)
		end


		if (item.name == 'vdScriptEnd') then
			print(dz.data.switchStates)
			print('----')

			local ok = true

			if (dz.data.switchStates ~= '3010') then
				print('Error: ' .. dz.data.switchStates .. ' should be 3010')
				ok = false
			end

			if ok then
				dz.devices('vdScriptOK').switchOn()
			end

		end

		if (item.name == 'vdSelectorSwitch') then
			dz.data.switchStates = dz.data.switchStates  .. item.level
		end

	end
}
