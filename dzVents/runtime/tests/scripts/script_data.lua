return {
	active = true,
	on = {
		devices = {'somedevice'}
	},
	data = {
		a = { initial = '' },
		b = { initial = 1 },
		c = { initial = {x = 1, y = 2} },
		d = { history = true, maxItems = 15},
		e = { history = true, maxHours = 1, getValue = function(item) return item.data.num end },
		g = { initial = 666 } -- this one is local (there's also a global with this name)
	},
	execute = function(domoticz, device)
		domoticz.data.a = 'this is set from script'
		domoticz.data.b = 245
		domoticz.data.c.x = 10
		domoticz.data.c.y = 20
		domoticz.data.g = 87

		domoticz.data.d.setNew(123)
		domoticz.data.e.setNew({num=456})

		domoticz.globalData.g = domoticz.data.e.min(1,1) -- should be 456 through the getValue function above
		domoticz.globalData.h = false
	end
}