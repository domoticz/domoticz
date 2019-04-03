return {
	on =    {
		devices  = { 'vdSetIconSwitch' },
		httpResponses  = { 'setIcon' },
	},

	execute = function(dz, item, info)
		
		iconSwitch = dz.devices('vdSetIconSwitch')
		
		local function getCustomImage()
			local url = dz.settings['Domoticz url'] ..  '/json.htm?type=devices&rid=' .. item.id
			dz.openURL  ({
				url = url,
				method = 'GET',
				callback = 'setIcon' ,
			}).afterSec(4)
		end

		if item.isHTTPResponse then
			rt = item.json.result[1]
			dz.log("CustomImage".. rt.Name .. " is " .. rt.CustomImage,dz.LOG_FORCE)
			iconSwitch.setDescription(rt.CustomImage)
			return
		end
		
		getCustomImage()
	end
}
