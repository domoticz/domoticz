return {
	on = {
		scenes = {
			'myScene'
		}
	},
	execute = function(domoticz, scene)
		domoticz.log('Scene ' .. scene.name .. ' was changed', domoticz.LOG_INFO)
	end
}