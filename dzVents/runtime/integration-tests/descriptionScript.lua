return {
	on = {
		devices = { 'vdDescriptionSwitch' },
		groups = { 'gpDescriptionGroup' },
		scenes = { 'scDescriptionScene' },
		httpResponses = { '*Description*' }
	},

	execute = function(domoticz, triggerItem, info)

		dz = domoticz
		log = dz.log
		local res = true

		local function getScenes(description)
			local url = dz.settings['Domoticz url'] ..  "/json.htm?type=scenes"
			dz.openURL({
				url = url,
				method = "GET",
				callback = description,
			}).afterSec(3)
		end

		if triggerItem.isGroup or triggerItem.isScene then
			getScenes(triggerItem.name)
		elseif triggerItem.isHTTPResponse then
			rt = triggerItem.json.result
			for i,j in ipairs(rt) do
				if rt[i].Name == 'gpDescriptionGroup' then
					dz.devices('groupDescriptionSwitch1').setDescription(rt[i].Description)
				elseif rt[i].Name == 'scDescriptionScene' then
					dz.devices('sceneDescriptionSwitch1').setDescription(rt[i].Description)
				end
				dz.log("Description in ".. rt[i].Name .. " is " .. rt[i].Description,dz.LOG_FORCE)
			end
			return
		end
		triggerItem.setDescription('Ieder nadeel heb zn voordeel')
	end
}
