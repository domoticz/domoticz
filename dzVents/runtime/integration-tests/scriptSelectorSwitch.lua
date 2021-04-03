return
{

	on =
	{
		devices =
		{
			'vdSelectorSwitch',
			'vdScriptStart',
			'vdScriptEnd',
		},
	},
	data =
	{
		switchStates = {initial = ''},
	},

	logging =
	{
		level = domoticz.LOG_DEBUG,
		marker = 'Selector test',
	},

	execute = function(dz, item)

		local switch = dz.devices('vdSelectorSwitch')
		dz.log(item.name .. ' ==>> Selector state: ' .. item.state .. ' ==>> level: ' .. item.level .. ' ==>> date: ' .. dz.data.switchStates, dz.LOG_DEBUG )

		if item.name == 'vdScriptStart' then
			switch.switchSelector(10)
			switch.switchSelector(30).afterSec(1).forSec(1)
			dz.devices('vdScriptEnd').switchOn().afterSec(3)
		elseif item.name == 'vdScriptEnd' then
			if dz.data.switchStates ~= '103010' then
				dz.log('Error: ' .. dz.data.switchStates .. ' should be 103010', dz.LOG_DEBUG)
			else
				dz.devices('vdScriptOK').switchOn()
			end
		elseif item == switch then
			dz.data.switchStates = dz.data.switchStates  .. item.level
			dz.log(item.name .. ' ==>> Selector state: ' .. item.state .. ' ==>> level: ' .. item.level .. ' ==>> date: ' .. dz.data.switchStates , dz.LOG_DEBUG )
		end

	end
}
