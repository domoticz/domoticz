return {
	active = true,
	on = {
		scenes = {'myscene2'}
	},
	data = {},
	execute = function(domoticz, scene, triggerInfo)
		return scene.name
	end
}