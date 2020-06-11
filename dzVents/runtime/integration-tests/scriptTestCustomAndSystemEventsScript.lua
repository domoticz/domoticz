return
{
	on =
	{
		system = { 'manualBackupFinished', 'st*' },
		customEvents = { [ 'myEvents*' ] = { 'at 04:00-03:45'} },
	},

	execute = function(domoticz, item)

		dz = domoticz

		local filename = 'eventsIntegrationtests.triggers'

		if item.trigger == 'start' then
			os.execute('rm -f ' .. _G.dataFolderPath .. '/../dumps/' .. filename)
		end

		item.dump()

		dz.utils.dumpTable({ item },nil ,filename)
	end
}
