return {
	active = true,
	on = {
		scenes = {'myscene1'}
	},
	data = {},
	execute = function(domoticz, scene, triggerInfo)
		scene.switchOff()
		return scene.name
	end
}