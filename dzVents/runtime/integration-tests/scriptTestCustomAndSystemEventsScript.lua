return
{
	on =
	{
		system = { 'manualBackupFinished', 'st*', 'reset*' },
		customEvents = { [ 'myEvents*' ] = { 'at 04:00-03:45'} },
		shellCommandResponses = { '*' },
	},

	execute = function(domoticz, item)

		dz = domoticz

		local filename = 'eventsIntegrationtests.triggers'

		if item.trigger == 'start' then
			os.execute('rm -f ' .. _G.dataFolderPath .. '/../dumps/' .. filename)
		end

		item.dump()

		dz.utils.dumpTable({ item },nil ,filename)

		if item.isShellCommandResponse then
			dz.log(item,dz.log_FORCE)
		end
	end
}
