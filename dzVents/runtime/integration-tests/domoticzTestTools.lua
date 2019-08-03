package.path = package.path ..
		";../?.lua;../device-adapters/?.lua;./data/?.lua;../../../scripts/dzVents/generated_scripts/?.lua;" ..
		"../../../scripts/lua/?.lua"

local _ = require 'lodash'
local File = require('File')
local http = require("socket.http")

local ltn12 = require("ltn12")
local jsonParser = require('JSON')
local scriptSourcePath = './'
local scriptTargetPath = '../../../scripts/dzVents/scripts/'
local generatedScriptTargetPath = '../../../scripts/dzVents/generated_scripts/'
local dataTargetPath = '../../../scripts/dzVents/data/'
local DUMMY_HW = 15
local DUMMY_HW_ID = 2


local function DomoticzTestTools(port, debug, webroot)

	if (port == nil) then port = 8080 end

	local BASE_URL = 'http://localhost:' .. port

	if (webroot ~= '' and webroot ~= nil) then
		BASE_URL = BASE_URL .. '/' .. webroot
	end

	local API_URL = BASE_URL .. '/json.htm?'


	local self = {
		['port'] = port,
		url = BASE_URL,
		apiUrl = API_URL
	}

	function self.getScriptSourcePath(name)
		return scriptSourcePath .. name
	end

	function self.getScriptTargetPath(name)
		return scriptTargetPath .. name
	end

	function self.copyScript(name)
		File.remove(self.getScriptTargetPath(name))
		File.copy(self.getScriptSourcePath(name), self.getScriptTargetPath(name))
	end

	function self.removeFSScript(name)
		os.remove(self.getScriptTargetPath(name))
	end

	function self.removeGUIScript(name)
		os.remove(dataTargetPath .. name)
	end

	function self.removeDataFile(name)
		os.remove(dataTargetPath .. name)
	end

	function self.doAPICall(url)
		local response = {}
		local _url = self.apiUrl .. url

		local result, respcode, respheaders, respstatus = http.request {
			method = "GET",
			url = _url,
			sink = ltn12.sink.table(response)
		}

		local _json = table.concat(response)

		local json = jsonParser:decode(_json)

		local ok = json.status == 'OK'

		if (debug and not ok) then
			_.print('--------------')
			_.print(_url)
			_.print(ok)
			_.print(json)
			_.print(debug.traceback())
			_.print('--------------')
		end


		return ok, json, result, respcode, respheaders, respstatus
	end

	function self.createHardware(name, type)
		local url = "param=addhardware&type=command&htype=" .. tostring(type) .. "&name=" .. name .. "&enabled=true&datatimeout=0"
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		local idx = json.idx
		return ok, idx, json, result, respcode, respheaders, respstatus
	end

	function self.createDummyHardware()
		local ok, dummyIdx = self.createHardware('dummy', DUMMY_HW)
		return ok, dummyIdx
	end

	function self.createManagedCounter(name)
		local url = "type=createdevice&idx=" .. DUMMY_HW_ID .."&sensorname=" .. name .. "&sensormappedtype=0xF321"
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		return ok, json, result, respcode, respheaders, respstatus
	end

	function self.createCamera()
		local url = "type=command&param=addcamera&address=192.168.192.123&port=8083&name=camera1&enabled=true&imageurl=aW1hZ2UuanBn&protocol=0"
		--&username=&password=&imageurl=aW1hZ2UuanBn&protocol=0
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		return ok, json, result, respcode, respheaders, respstatus
	end

	function self.createVirtualDevice(hw, name, type, options)

		local url = "type=createvirtualsensor&idx=" .. tostring(hw) .."&sensorname=" .. name .. "&sensortype=" .. tostring(type)
		if tostring(type) == "33" then
			url = "type=createdevice&idx=" .. tostring(hw) .."&sensorname=" .. name .. "&sensormappedtype=0xF321"
		end

		if (options) then
			url = url .. '&sensoroptions=1;' .. tostring(options)
		end

		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		local idx = json.idx

		return ok, idx, json, result, respcode, respheaders, respstatus
	end

	function self.createScene(name)
		-- type=addscene&name=myScene&scenetype=0
		local url = "type=addscene&name=" .. name .. "&scenetype=0"

		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		local idx = json.idx
		return ok, idx, json, result, respcode, respheaders, respstatus
	end

	function self.createGroup(name)
		-- type=addscene&name=myGroup&scenetype=1
		local url = "type=addscene&name=" .. name .. "&scenetype=1"

		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		local idx = json.idx

		return ok, idx, json, result, respcode, respheaders, respstatus
	end

	function self.createVariable(name, type, value)
		-- todo, encode value
		--type=command&param=adduservariable&vname=myint&vtype=0&vvalue=1
		local url = "param=adduservariable&type=command&vname=" .. name .."&vtype=" .. tostring(type) .. "&vvalue=" .. tostring(value)

		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)

		local idx = json.idx

		return ok, idx, json, result, respcode, respheaders, respstatus
	end

	function self.deleteDevice(idx)
		-- type=deletedevice&idx=2;1;...
		local url = "type=deletedevice&idx=" .. tostring(idx)
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		return ok, json
	end

	function self.deleteGroupScene(idx)
		-- type=deletescene&idx=1
		local url = "type=deletescene&idx=" .. tostring(idx)
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		return ok, json
	end

	function self.getDevice(idx)
		--type=devices&rid=15
		local url = "type=devices&rid=" .. idx
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		if (ok and json.result ~= nil and _.size(json.result) == 1) then
			return ok, json.result[1]
		end
		return false, nil
	end

	function self.getAllDevices()
		-- type=devices&displayhidden=1&displaydisabled=1&filter=all&used=all
		local url = "type=devices&displayhidden=1&displaydisabled=1&filter=all&used=all"
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		return ok, json, result
	end

	function self.deleteAllDevices()
		local ok, json, result = self.getAllDevices()
		local res = true
		if (ok) then
			if (_.size(json.result) > 0) then
				for i, device in pairs(json.result) do
					local ok, json
					if (device.Type == 'Scene' or device.Type == 'Group') then
						ok, json = self.deleteGroupScene(device.idx)
					else
						ok, json = self.deleteDevice(device.idx)
					end

					if (not ok) then
						print('Failed to remove device: ' .. device.Name)
						res = false
					end
				end
			end
		else
			return false
		end
		return res
	end

	function self.deleteVariable(idx)
		--type=command&param=deleteuservariable&idx=1
		local url = "param=deleteuservariable&type=command&idx=" .. tostring(idx)
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		return ok, json
	end

	function self.deleteAllVariables()
		-- type=commandparam=onkyoeiscm=getuservariables
		local url = "param=getuservariables&type=command"
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		local res = true
		if (ok) then
			if (_.size(json.result) > 0) then
				for i, variable in pairs(json.result) do
					local ok, json
					ok, json = self.deleteVariable(variable.idx)
					if (not ok) then
						print('Failed to remove variable: ' .. variable.Name)
						res = false
					end
				end
			end
		else
			return false
		end
		return res
	end

	function self.deleteCamera(idx)
		-- http://localhost:8080/json.htm?type=command&param=deletehardware&idx=2
		local url = "type=command&param=deletecamera&idx=" .. tostring(idx)
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		return ok, json
	end

	function self.deleteAllCameras()
		local url = "type=cameras"
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		local res = true
		if (ok) then
			if (_.size(json.result) > 0) then
				for i, camera in pairs(json.result) do
					local ok, json = self.deleteCamera(camera.idx)
					if (not ok) then
						print('Failed to remove cameras: ' .. camera.Name)
						res = false
					end
				end
			end
		else
			return false
		end
		return res
	end


	function self.deleteHardware(idx)
		-- http://localhost:8080/json.htm?type=command&param=deletehardware&idx=2
		local url = "param=deletehardware&type=command&idx=" .. tostring(idx)
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		return ok, json
	end

	function self.deleteAllHardware()
		-- http://localhost:8080/json.htm?type=hardware
		local url = "type=hardware"
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		local res = true
		if (ok) then
			if (_.size(json.result) > 0) then
				for i, hw in pairs(json.result) do
					local ok, json = self.deleteHardware(hw.idx)
					if (not ok) then
						print('Failed to remove hardware: ' .. hw.Name)
						res = false
					end
				end
			end
		else
			return false
		end
		return res
	end

	function self.deleteScript(idx)
		--type=events&param=delete&event=1
		local url = "param=delete&type=events&event=" .. tostring(idx)
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		return ok, json
	end

	function self.deleteAllScripts()
		-- type=events&param=list
		local url = "param=list&type=events"
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		local res = true
		if (ok) then
			if (_.size(json.result) > 0) then
				for i, script in pairs(json.result) do
					local ok, json
					ok, json = self.deleteScript(script.id)

					if (not ok) then
						print('Failed to remove script: ' .. script.Name)
						res = false
					end
					return true

				end
			end
		else
			return false
		end
		return res
	end

	function self.updateSwitch(idx, name, description, switchType)
		--http://localhost:8080/json.htm?type=setused&idx=1&name=vdSwitch&description=des&strparam1=&strparam2=&protected=false&switchtype=0&customimage=0&used=true&addjvalue=0&addjvalue2=0&options=
		local url = "type=setused&idx=" ..
				tostring(idx) ..
				"&name=" .. name ..
				"&description=" .. description ..
				"&strparam1=&strparam2=&protected=false&" ..
				"switchtype=" .. tostring(switchType) .. "&customimage=0&used=true&addjvalue=0&addjvalue2=0&options="
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		return ok
	end

	function self.switch(idx, cmd)
		--type=command&param=switchlight&idx=39&switchcmd=On&level=0&passcode=
		local url = "param=switchlight&type=command&switchcmd=" .. cmd .. "&level=0&idx=" .. tostring(idx)
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		return ok
	end

	function self.dimTo(idx, cmd, level)
		--type=command&param=switchlight&idx=39&switchcmd=On&level=0&passcode=
		local url = "param=switchlight&type=command&switchcmd=" .. cmd .. "&level=" .. tostring(level) .. "&idx=" .. tostring(idx)
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		return ok
	end


	function self.switchSelector(idx, level)
		--type=command&param=switchlight&idx=39&switchcmd=On&level=0&passcode=
		local url = "param=switchlight&type=command&switchcmd=Set%20Level&level=" .. tostring(level) .. "&idx=" .. tostring(idx)
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		return ok
	end


	function self.switchGroup(idx, cmd)
		--http://localhost:8080/json.htm?type=command&param=switchscene&idx=2&switchcmd=On&passcode=
		local url = "param=switchscene&type=command&idx=" .. tostring(idx) .. "&switchcmd=" .. cmd .. "&passcode="
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		return ok
	end

	function self.addSceneDevice(sceneIdx, devIdx)
		-- http://localhost:8080/json.htm?type=command&param=addscenedevice&idx=2&isscene=false&devidx=1&command=On&level=100&hue=0&ondelay=&offdelay=
		local url = "param=addscenedevice&type=command&idx=" .. tostring(sceneIdx) .. "&isscene=false&devidx=" .. tostring(devIdx) .. "&command=On&level=100&hue=0&ondelay=&offdelay="
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		return ok
	end

	function self.initSettings(IFTTTEnabled)
		local IFTTTEnabled = IFTTTEnabled or "%20"
		local response = {}
		local reqbody = "Language=en&Themes=default&Title=Domoticz&Latitude=52.278870&Longitude=5.665849&DashboardType=0&AllowWidgetOrdering=on&MobileType=0&WebUserName=&WebPassword=d41d8cd98f00b204e9800998ecf8427e&AuthenticationMethod=0&GuestUser=0&SecPassword=1&SecOnDelay=0&ProtectionPassword=d41d8cd98f00b204e9800998ecf8427e&WebLocalNetworks=127.0.0.1&RemoteSharedPort=6144&WebRemoteProxyIPs=127.0.0.1&checkforupdates=on&ReleaseChannel=0&AcceptNewHardware=on&HideDisabledHardwareSensors=on&MyDomoticzUserId=&MyDomoticzPassword=&EnableTabLights=on&EnableTabScenes=on&EnableTabTemp=on&EnableTabWeather=on&EnableTabUtility=on&EnableTabCustom=on&LightHistoryDays=30&ShortLogDays=1&ProwlAPI=&NMAAPI=&PushbulletAPI=&PushsaferAPI=&PushsaferImage=&PushoverUser=&PushoverAPI=&PushALotAPI=&ClickatellAPI=&ClickatellUser=&ClickatellPassword=&ClickatellFrom=&ClickatellTo=&IFTTTAPI=thisIsnotTheRealAPI&IFTTTEnabled=" .. IFTTTEnabled ..
"&HTTPField1=&HTTPField2=&HTTPField3=&HTTPField4=&HTTPTo=&HTTPURL=https%3A%2F%2Fwww.somegateway.com%2Fpushurl.php%3Fusername%3D%23FIELD1%26password%3D%23FIELD2%26apikey%3D%23FIELD3%26from%3D%23FIELD4%26to%3D%23TO%26message%3D%23MESSAGE&HTTPPostData=&HTTPPostContentType=application%2Fjson&HTTPPostHeaders=&KodiIPAddress=224.0.0.1&KodiPort=9777&KodiTimeToLive=5&LmsPlayerMac=&LmsDuration=5&TelegramAPI=&TelegramChat=&NotificationSensorInterval=43200&NotificationSwitchInterval=0&EmailEnabled=on&EmailFrom=&EmailTo=&EmailServer=&EmailPort=25&EmailUsername=&EmailPassword=&UseEmailInNotifications=on&TempUnit=0&DegreeDaysBaseTemperature=18.0&WindUnit=0&WeightUnit=0&EnergyDivider=1000&CostEnergy=0.2149&CostEnergyT2=0.2149&CostEnergyR1=0.0800&CostEnergyR2=0.0800&GasDivider=100&CostGas=0.6218&WaterDivider=100&CostWater=1.6473&ElectricVoltage=230&CM113DisplayType=0&SmartMeterType=0&FloorplanPopupDelay=750&FloorplanAnimateZoom=on&FloorplanShowSensorValues=on&FloorplanShowSceneNames=on&FloorplanRoomColour=Blue&FloorplanActiveOpacity=25&FloorplanInactiveOpacity=5&RandomSpread=15&SensorTimeout=60&BatterLowLevel=0&LogFilter=&LogFileName=&LogLevel=0&DoorbellCommand=0&RaspCamParams=-w+800+-h+600+-t+1&UVCParams=-S80+-B128+-C128+-G80+-x800+-y600+-q100&EnableEventScriptSystem=on&LogEventScriptTrigger=on&DisableDzVentsSystem=on&DzVentsLogLevel=3"

		local url = BASE_URL .. '/storesettings.webem'
		local result, respcode, respheaders, respstatus = http.request {
			method = "POST",
			source = ltn12.source.string(reqbody),
			url = url,
			headers = {
				['Connection'] = 'keep-alive',
				['Content-Length'] = _.size(reqbody),
				['Pragma'] = 'no-cache',
				['Cache-Control'] = 'no-cache',
				['Origin'] = BASE_URL,
				['User-Agent'] = 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_5) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/59.0.3071.115 Safari/537.36',
				['Content-Type'] = 'application/x-www-form-urlencoded; charset=UTF-8',
				['Accept'] = '*/*',
				['X-Requested-With'] = 'XMLHttpRequest',
				['DNT'] = '1',
				['Referer'] = BASE_URL,
			},
			sink = ltn12.sink.table(response)
		}

		local _json = table.concat(response)
		local ok = (respcode == 200)
		return ok, result, respcode, respheaders, respstatus

	end

	function self.reset()
		local ok = true
		ok = ok and self.deleteAllDevices()
		ok = ok and self.deleteAllVariables()
		ok = ok and self.deleteAllHardware()
		ok = ok and self.deleteAllScripts()
		ok = ok and self.initSettings()
		return ok
	end

	function self.installFSScripts(scripts)
		for i,script in pairs(scripts) do
			self.createFSScript(script)
		end
	end

	function self.cleanupFSScripts(scripts)
		for i,script in pairs(scripts) do
			self.removeFSScript(script)
			self.removeDataFile('__data_' .. script)
		end
	end

	function self.setDisarmed()
		-- http://localhost:8080/json.htm?type=command&param=setsecstatus&secstatus=0&seccode=c4ca4238a0b923820dcc509a6f75849b
		local url = "param=setsecstatus&type=command&secstatus=0&seccode=c4ca4238a0b923820dcc509a6f75849b"
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		return ok
	end

	function self.addSecurityPanel(idx)
		local url = "type=setused&idx=" .. tostring(idx) .. "&name=secPanel&used=true&maindeviceidx="
		local ok, json, result, respcode, respheaders, respstatus = self.doAPICall(url)
		return ok
	end

	function self.createScript(name, code)
		local response = {}
		local reqbody = 'name=' .. name ..'&' ..
				'interpreter=dzVents&' ..
				'eventtype=All&' ..
				'xml=' .. tostring(code) .. '&' ..
				--'xml=return%20%7B%0A%09active%20%3D%20false%2C%0A%09on%20%3D%20%7B%0A%09%09timer%20%3D%20%7B%7D%0A%09%7D%2C%0A%09execute%20%3D%20function(domoticz%2C%20device)%0A%09end%0A%7D&' ..
				'logicarray=&' ..
				'eventid=&' ..
				'eventstatus=1&' ..
				'editortheme=ace%2Ftheme%2Fxcode'
		local url = BASE_URL .. '/event_create.webem'
		local result, respcode, respheaders, respstatus = http.request {
			method = "POST",
			source = ltn12.source.string(reqbody),
			url = url,
			headers = {
				['Connection'] = 'keep-alive',
				['Pragma'] = 'no-cache',
				['Cache-Control'] = 'no-cache',
				['Content-Length'] = _.size(reqbody),
				['Origin'] = BASE_URL,
				['User-Agent'] = 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_5) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/59.0.3071.115 Safari/537.36',
				['Content-Type'] = 'application/x-www-form-urlencoded; charset=UTF-8',
				['Accept'] = '*/*',
				['X-Requested-With'] = 'XMLHttpRequest',
				['DNT'] = '1',
				['Referer'] = BASE_URL,
			},
			sink = ltn12.sink.table(response)
		}

		local _json = table.concat(response)
		local ok = (respcode == 200)

		return ok, result, respcode, respheaders, respstatus
	end

	function self.createGUIScriptFromFile(name)
		local stage1Script = File.read(name)
		local ok, result, respcode, respheaders, respstatus = self.createScript('stage1', stage1Script)
		return ok
	end

	function self.createFSScript(name)
		self.copyScript(name)
	end

	function self.tableEntries(t)
		local count = 0
		for _ in pairs(t) do count = count + 1 end
		return count
	end

	return self


end

return DomoticzTestTools
