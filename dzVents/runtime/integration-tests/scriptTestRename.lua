return {
			on = { devices = { 'vdRename*' }},

			logging = { level = domoticz.LOG_DEBUG, marker = 'rename'},

	execute = function(dz, item )

		local ok 
		if item.name == 'vdRenameSwitch' then
				item.rename('vdRenameSwitch renamed').afterSec(3)
				item.switchOn().afterSec(5)
				dz.scenes('scRenameScene').rename('scRenameScene renamed').afterSec(1)
				dz.groups('gpRenameGroup').rename('gpRenameGroup renamed').afterSec(2)
				dz.variables('varRename').rename('varRename renamed')
				return
		elseif item.name == 'vdRenameSwitch renamed' then
				ok = dz.scenes('scRenameScene renamed') and
					 dz.groups('gpRenameGroup renamed') and
					 dz.variables('varRename renamed')
		end

		if ok then
				dz.devices('switchRenameResults').updateText('RENAME SUCCEEDED')
		else
				dz.devices('switchRenameResults').updateText('RENAME FAILED')
		end
	end
}
