local socket = require("socket");
return {
	active = true,
	on = {
		devices = {
			'vdRepeatSwitch',
			'vdScriptStart',
			'vdScriptEnd'
		},

	},
	data = { states = {initial = ''}},
	execute = function(dz, item)

		if (item.name == 'vdScriptStart') then
			dz.devices('vdRepeatSwitch').switchOn().afterSec(2).forSec(1).repeatAfterSec(1, 1)
			dz.devices('vdScriptEnd').switchOn().afterSec(8)
		end

		if (item.name == 'vdScriptEnd') then
			print(dz.data.states)
			if (dz.data.states == 'OnOffOnOff') then
				dz.devices('vdScriptOK').switchOn()
			else
				print('Error: ' .. dz.data.states)
			end

		end

		if (item.name == 'vdRepeatSwitch') then
			--socket.sleep(3)
			dz.data.states = dz.data.states  .. item.state
		end

	end
}
