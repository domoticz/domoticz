return {
			on = { devices = { 'vdProtect*' }},

			logging = { level = domoticz.LOG_DEBUG, marker = 'protect'},

	execute = function(dz, item )
		local resultTextSwitch = dz.devices('switchProtectResults')
		local OK = 'PROTECTION'
		local NOK = 'PROTECTION FAILED'
		local On = 'On'
		local Off = 'Off'

		local function switch(action, idx, delay)
			local url = dz.settings['Domoticz url'] .. '/json.htm?type=command&param=switchlight&switchcmd=' .. action .. '&idx=' .. idx
			dz.openURL(url).afterSec(delay)
		end

		dz.log(item.protected,dz.LOG_DEBUG)
		
		if item.active and resultTextSwitch.text ~= OK  then 
			resultTextSwitch.updateText(OK)
			item.protectionOn()
			switch(On, item.idx, 3)	-- Should not be allowed
			item.protectionOff().afterSec(5)
			switch(Off, item.idx, 7)	-- Should be allowed
		elseif item.active then
			resultTextSwitch.updateText(NOK)
		elseif resultTextSwitch.text ~= NOK then
			resultTextSwitch.updateText(OK .. ' SUCCEEDED')
		end
	end
}
