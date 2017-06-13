return {
	active = true,
	on = {
		devices = {
			['deviceZork'] = { 'on mon, tue' },
			['deviceDork'] = { 'on sun' },
			'deviceGork'
		}
	},
	execute = function(domoticz, device)
		return 'script_with_active_method'
	end
}