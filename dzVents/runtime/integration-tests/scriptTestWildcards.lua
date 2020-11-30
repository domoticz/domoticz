return {
			on = { devices = { '*Wild*card*' }},

			logging = { level = domoticz.LOG_DEBUG, marker = 'wildcard'},

			data = {	lastAcessed = { initial = 0 },
						counter = { initial = 0 }},

	execute = function(dz, item )
		local funnyNames = {"_-Wildcard-_","-Wildcard-","[-Wildcard-]","-Wild-card-card-","(-Wildcard-/","\\Wild-card-\\","wildcard_endState"}

		if dz.data.lastAcessed > ( dz.time.dDate - 10 )  then
			dz.data.counter = dz.data.counter + 1
		else
			dz.data.counter = 1
		end
		dz.data.lastAcessed = dz.time.dDate

		local function renameDevice(id, newName)
			local url = dz.settings['Domoticz url'] ..  "/json.htm?type=command&param=renamedevice" ..
						"&idx=" .. item.idx ..
						"&name=" .. dz.utils.urlEncode(newName)
			dz.openURL(url)
		end

		renameDevice( item.idx, funnyNames[dz.data.counter] or funnyNames[#funnyNames])
		
		if dz.data.counter == #funnyNames then
			dz.devices('switchWildCardsResults').updateText("WILDCARD SUCCEEDED")
		else
			dz.devices(item.id).dimTo(dz.data.counter).afterSec(2)
		end
	end
}