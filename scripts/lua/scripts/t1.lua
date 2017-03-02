return {
	active = true,
	on = {
		't1'
},
execute = function(domoticz, switch)
	if (switch.state == 'On') then
		domoticz.notify('Hey!','I am on!',
		domoticz.PRIORITY_NORMAL)
	else
		domoticz.notify('Hey','I am off',
		domoticz.PRIORITY_NORMAL)
	end
end
}

