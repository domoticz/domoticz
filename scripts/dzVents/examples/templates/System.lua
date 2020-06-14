return {
	on = {
		system = {
			'start',
			'stop',
			'manualBackupFinished',
            'dailyBackupFinished',
            'hourlyBackupFinished',
            'monthlyBackupFinished'
		}
	},
	data = {},
	logger = {},
	execute = function(domoticz, triggeredItem)
		domoticz.log('Domoticz has started')
	end
}
