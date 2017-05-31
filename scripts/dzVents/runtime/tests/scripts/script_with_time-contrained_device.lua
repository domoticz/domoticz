return {
	active = true,
	on = {
		['deviceZork'] = { 'on mon, tue' },
		['devices'] = {
			['deviceDork'] = { 'on sun' },
			'deviceGork'
		}
	},
	execute = function(domoticz, device)
		return 'script_with_active_method'
	end
}