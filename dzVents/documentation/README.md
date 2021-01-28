**Note**: This document is maintained on [github](https://github.com/domoticz/domoticz/blob/development/dzVents/documentation/README.md), and the wiki version is automatically generated. Edits should be performed on github, or they may be suggested on the wiki article's [Discussion page](https://www.domoticz.com/wiki/Talk:DzVents:_next_generation_LUA_scripting).
Editing can be done by any editor but if you are looking for a specialized markDown editor; [stackedit.io](https://stackedit.io/app#) would be a good choice.

Documentation for dzVents 2.4.0 - 2.5.7 (Domoticz v4.11652) can be found [here](https://github.com/domoticz/domoticz/blob/a0a6069e40744df222e889474032439476b7ecfb/dzVents/documentation/README.md).

Documentation for dzVents 2.3.0 (Domoticz v3.8153) can be found [here](https://github.com/domoticz/domoticz/blob/2f6ba5c5a8978a010d6867228ad84eab762c5936/dzVents/documentation/README.md).

Documentation for dzVents 2.2.0 (Domoticz v3.5876) can be found [here](https://github.com/domoticz/domoticz/blob/9f75e45f994f87c8d8ce9cb39eaab85886df0be4/scripts/dzVents/documentation/README.md).

# About dzVents
dzVents /diː ziː vɛnts/, short for Domoticz Easy Events, brings Lua scripting in Domoticz to a whole new level. Writing scripts for Domoticz has never been so easy. Not only can you define triggers more easily, and have full control over timer-based scripts with extensive scheduling support, dzVents presents you with an easy to use API to all necessary information in Domoticz. No longer do you have to combine all kinds of information given to you by Domoticz in many different data tables. You don't have to construct complex commandArrays anymore. dzVents encapsulates all the Domoticz peculiarities regarding controlling and querying your devices. And on top of that, script performance has increased a lot if you have many scripts because Domoticz will fetch all device information only once for all your device scripts and timer scripts. And ... **it is 100% Lua**! So if you already have a bunch of event scripts for Domoticz, upgrading should be fairly easy.

Let's start with an example. Say you have a switch that when activated, it should activate another switch but only if the room temperature is above a certain level. And when done, it should send a notification. This is how it looks in dzVents:

```Lua
return
{
	on =
	{
		devices = { 'Room switch'}
	},

	execute = function(domoticz, roomSwitch)
		if (roomSwitch.active and domoticz.devices('Living room').temperature > 18) then
		domoticz.devices('Another switch').switchOn()
		domoticz.notify('This rocks!',
						'Turns out that it is getting warm here',
						domoticz.PRIORITY_LOW)
		end

}
```

Or you have a timer script that should be executed every 10 minutes, but only on weekdays, and have it do something with some user variables and only during daytime:

```Lua
return
{
	on =
	{
		timer = {'Every 10 minutes on mon,tue,wed,thu,fri'}
	},

	execute = function(domoticz)
		-- check time of the day
		if (domoticz.time.isDayTime and domoticz.variables('myVar').value == 10) then
			domoticz.variables('anotherVar').set(15)
			--activate my scene
			domoticz.scenes('Evening lights').switchOn()
			if (domoticz.devices('My PIR').lastUpdate.minutesAgo > 5) then
				domoticz.devices('Bathroom lights').switchOff()
			end
		end
	end
}
```

Or you want to detect a humidity rise within the past 5 minutes:

```Lua
return
{
	on =
	{
		timer = {'every 5 minutes'}
	},
	data =
	{
		previousHumidity = { initial = 100 }
	},
	execute = function(domoticz)
		local bathroomSensor = domoticz.devices('BathroomSensor')
		if (bathroomSensor.humidity - domoticz.data.previousHumidity) >= 5 then
			-- there was a significant rise
			domoticz.devices('Ventilator').switchOn()
		end
		-- store current value for next cycle
		domoticz.data.previousHumidity = bathroomSensor.humidity
	end
}
```

Just to give you an idea! Everything in your Domoticz system is now logically available in the domoticz object structure. With this domoticz object, you can get to all the information in your system and manipulate your devices.

# Using dzVents with Domoticz
In Domoticz go to **Setup > Settings > Other**  and in the section EventSystem make sure the check-box 'dzVents disabled' is not checked.
Also make sure that in the Security section in the settings **(Setup > Settings > System > Local Networks (no username/password)** you allow 127.0.0.1 (and / or ::1 when using IPv6 ) to not need a password. dzVents does use this port to get the location settings and to send certain commands to Domoticz. Finally make sure you have set your current location in **Setup > Settings > System > Location**, otherwise there is no way to determine nighttime/daytime state.

There are two ways of creating dzVents event scripts in Domoticz:

 1. By creating scripts in your domoticz instance on your domoticz server: `/path/to/domoticz/scripts/dzVents/scripts`. Make sure that each script has the extension `.lua` and follows the guidelines as described below.
 2. By creating scripts inside Domoticz using the internal Domoticz event editor: Go to **Setup > More Options > Events**. Press the + button and choose dzVents. You must then choose a template. They are there just for convenience during writing the script; the actual trigger for the script is determined by what you entered in the on = section. The internal event editor have a help button when writing dzVents scripts, next to Save and Delete. This button opens this wiki in a separate browser tab when clicked. Name your script to your liking but leave out the extension .lua

**Note: scripts that you write on the file-system and inside Domoticz using the internal web-editor all share the same name-space. That means that if you have two scripts with the same name, only the one of the file-system will be used. The log will tell you when this happens.**

**Note: Scripts created in the internal editor are stored in the domoticz database and are all written to `/path/to/domoticz/scripts/dzVents/generated_scripts` on every domoticz restart and every "EventSystem: reset all events...". Scripts in `/path/to/domoticz/scripts/dzVents/scripts` are not stored in the database and should be backed-up separately. **

## Quickstart
If you made sure that dzVents system is active, we can do a quick test if everything works:

 - Pick a switch in your Domoticz system. Write down the exact name of the switch. If you don't have a switch then you can create a Dummy switch and use that one.
 - Create a new file in the `/path/to/domoticz/scripts/dzVents/scripts/` folder (or using the web-editor in Domoticz, switch to dzVents mode first.). Call the file `test.lua`. *Note: when you create a script in the web-editor you do **not** add the .lua extension!* Also, valid script names follow the same rules as [filesystem names](https://en.wikipedia.org/wiki/Filename#Reserved_characters_and_words ).
 - Open `test.lua` in an editor and fill it with this code and change `<exact name of the switch>` with the .. you guessed it... exact name of the switch device:
```Lua
	return {
		on = {
			devices = {
			'<exact name of the switch>'
			}
		},
		execute = function(domoticz, switch)
			if (switch.state == 'On') then
				domoticz.log('Hey! I am on!')
			else
				domoticz.log('Hey! I am off!')
			end
		end
	}
```
 - Save the script
 - Open the Domoticz log in the browser
 - In Domoticz  GUI (perhaps in another browser tab) press the switch.
 - You can watch the log in Domoticz and it should show you that indeed it triggered your script and you should see the log messages.

See the examples folder `/path/to/domoticz/scripts/dzVents/examples` for more examples. This folder includes templates you can use to get started as well. And, if you use the GUI web editor to write your script, you will find boilerplate examples in the drop-down below the script type setting.

# Writing scripts
In order for your scripts to work with dzVents, they have to be turned into a Lua module with a specific structure. Basically you make sure it returns a Lua table (object) with predefined keys like `on` and `execute`. Here is an example:

```Lua
return
{
	on =
	{
		devices = { 'My switch' }
	},

	execute = function(domoticz, switch)
		-- your script logic goes here, something like this:
		if (switch.state == 'On') then
			domoticz.log('I am on!', domoticz.LOG_INFO)
		end
	end
}
```

Simply said, the `on`-part defines the trigger and the `execute` part is what should be done if the trigger matches with the current Domoticz event. So all your logic is inside the `execute` function.

## Sections in the script
Each dzVents event script has this structure:
```Lua
return
{
	active = true, -- optional
	on =
	{
		-- at least one of these:
		customEvents = { ... },
		devices = { ... },
		groups = { ... },
		httpResponses = { ... },
		scenes = { ... },
		security = { ... },
		shellCommandResponses = { ... },
		system = { ... },
		timer = { ... },
		variables = { ... },

	},
	data = { ... }, -- optional
	logging = { ... }, -- optional
	execute = function(domoticz, item, triggerInfo)
	-- your code here
	end
}
```

### active = true/false (optional)
If `active = false`, then the script is not activated: *dzVents will ignore the script completely*. If you don't have `active` specified, `true` is assumed (default value). If you write scripts with Domoticz' internal script editor then you don't need this part as the editor has its own way of enabling and disabling scripts.

So, `active` can either be:

 - **true**: (default) the script file will be processed.
 - **false**: the script file is ignored.
 - **function(domoticz) ... end**: A function returning `true` or `false`. The function will receive the domoticz object with all the information about your domoticz instance: `active = function(domoticz) .... end`. For example you could check for a Domoticz variable or switch and prevent the script from being executed. **However, be aware that for *every script* in your scripts folder, this active function will be called, every cycle!! Therefore, it is better to put all your logic in the execute function instead of in the active function.**

### on = { ... }
The `on` section tells dzVents *when* the execute function has to be executed. It holds all the events/**triggers** that are monitored by dzVents. If any of the events or triggers match with the current event coming from Domoticz, then the `execute` part of the script is executed by dzVents.
The `on` section has many kinds of subsections that *can all be used simultaneously* :

#### customEvents = { ... } <sup>3.0.0</sup>
A list of one or more custom event triggers. This eventTrigger can be activate by a json/api call, a MQTT message (when domoticz is setup to listen to such messages on the hardware tab) or by the dzVents internal command domoticz.emitEvent
 - The name of the custom-event
 - The name of the custom-event followed by a time constraint, such as:
	`['start']  = { 'at 15:*', 'at 22:* on sat, sun' }` The script will be executed if domoticz is started, **and** it is either between 15:00 and 16:00 or between 22:00 and 23:00 in the weekend. See [See time trigger rules](#timer_trigger_rules).

##### API
	- curl: curl -d "{ 'a':10, 'b':20, 'some':'text', 'sub' : { 'x':10, 'y':20 } }" "http://<domoticzIP:domoticz port>/json.htm?type=command&param=customevent&event=<myCustomEvent>"
	- JSON: **< domoticzIP : domoticz port >**/json.htm?type=command&param=customevent&event=<MyEvent>&data=myData
	- MQTT simple:  {"command":"customevent", "event":"MyEvent","data":"myData"}
	- MQTT complex: {"command":"customevent","event":"MyEvent","data":"{\"idx\":29,\"test\":\"ok\"}" }
	- emitEvent: domoticz.emitEvent('myCustomEvent' [,])
		`domoticz.emitEvent('myEvent') -- no data`
		`domoticz.emitEvent('another event', 'some data')`
		`domoticz.emitEvent('hugeEvent', { a = 10, b = 20, some = 'text', sub = { x = 10, y = 20 } })`

##### Attributes
The customEvent object (second parameter in your execute function) has these attributes:

 - **data**: Raw customEvent data.
 - **hasLines**: *Boolean*. <sup>3.1.0</sup> When true, the data is automatically converted to a Lua table. (isJSON and isXML have preference)
 - **isJSON**: *Boolean*.<sup>3.0.3</sup> true when the customEvent data is a valid json string. The data is then automatically converted to a Lua table.
 - **isXML**: *Boolean*. <sup>3.0.3</sup> true when the customEvent data is a valid xml string. When true, the data is automatically converted to a Lua table.
 - **json**. *Table*. <sup>3.0.3</sup> When the customEvent data is a valid json string, the response data is automatically converted to a Lua table for quick and easy access. nil otherwise
 - **lines**: *Table*. <sup>3.1.0</sup> When the response data has multiple lines but is not a JSON or XML string then the response data is automatically converted to a table for quick and easy access. nil otherwise
 - **trigger**, **customEvent**: *String*.<sup>3.0.3</sup> The string that triggered this customEvent instance. This is useful if you have a script that can be triggered by multiple different customEvent strings.
 - **xml**. *Table*. <sup>3.0.3</sup> When the response data is a valid xml string, the customEvent data is automatically converted to a Lua table for quick and easy access. nil otherwise
 - **xmlEncoding**. *String*. When the response data is `text/xml` See [ xml encoding] ( https://en.wikipedia.org/wiki/XML ).
 - **xmlVersion**. *String*. When the response data is `text/xml` See [ xml versions ] ( https://en.wikipedia.org/wiki/XML ).

#### devices = { ... }
A list of device-names or indexes. If a device in your system was updated (e.g. a switch was triggered or a new temperature was received) and it is listed in this section then the execute function is executed. **Note**: update does not necessarily means the device state or value has changed. Each device can be:

 - The name of your device between string quotes. **You can use the asterisk (\*) wild-card here e.g. `PIR_*` or `*_PIR`**.  E.g.: `devices = { 'myDevice', 'anotherDevice', 123, 'pir*' }`
 - The index (idx) of your device (as the name may change, the index will usually stay the same, the index can be found in the devices section in Domoticz). **Note that idx is a number;**
 - The name or idx of your device followed by a time constraint, such as:
	`['myDevice']  = { 'at 15:*', 'at 22:* on sat, sun' }` The script will be executed if `myDevice` was changed, **and** it is either between 15:00 and 16:00 or between 22:00 and 23:00 in the weekend. See [time trigger rules](#timer_trigger_rules).

#### groups = { ...}
A list of one or more group-names or indexes. The same rules as devices apply.

#### httpResponses = { ...}
A list of  one or more http callback triggers. Use this in conjunction with `domoticz.openURL()` where you will provide Domoticz with the callback trigger.  See [asynchronous http requests](#Asynchronous_HTTP_requests) for more information.

#### shellCommandResponses = { ...} <sup>3.1.0</sup>
A list of  one or more scriptcommand callback triggers. Use this in conjunction with `domoticz.executeShellCommand()` where you will provide Domoticz with the callback trigger.  See  [Asynchronous shell commands](#Asynchronous_shell_command_execution) for more information.

#### scenes = { ... }
 A list of one or more scene-names or indexes. The same rules as devices apply. So if your scene is listed in this table then the executed function will be triggered, e.g.: `on = { scenes = { 'myScene', 'anotherScene' } },`.

#### security = { ... }
A list of one or more of these security states:

 - `domoticz.SECURITY_ARMEDAWAY`,
 - `domoticz.SECURITY_ARMEDHOME`,
 - `domoticz.SECURITY_DISARMED`

If the security state in Domoticz changes and it matches with any of the states listed here, the script will be executed. See `/path/to/domoticz/scripts/dzVents/examples/templates/security.lua` for an example see [Security Panel](#Security_Panel) for information about how to create a security panel device.

#### system = { ...} <sup>3.0.0</sup>
A list of  one or more system triggers.

- `stop`,
`start`,
`manualBackupFinished`,
`dailyBackupFinished`,
`hourlyBackupFinished`,
`monthlyBackupFinished`,

 - The name of the system-event followed by a time constraint, such as:
	`['start']  = { 'at 15:*', 'at 22:* on sat, sun' }` The script will be executed if domoticz is  started, **and** it is either between 15:00 and 16:00 or between 22:00 and 23:00 in the weekend. See [time trigger rules](#timer_trigger_rules).
	-   **start**  - fired when Domoticz has started.
	-   **stop**  - fired when Domoticz is shutting down. As you probably can imagine you have only a limited amount of time - also depending on the load on your system -  to have Domoticz do stuff when your script has been completed. Some commands will probably not be executed. Just give it a try.
	-  **Backups** - the trigger item (2nd parameter of the execute function) is a table that holds information about the newly created backup (location,  duration in seconds and type).  You could use this information to copy the file to some other location or for another purpose.
		-	**dailyBackupFinished**	- automatic backup when set in domoticz
		-	**hourlyBackupFinished**	 -							  " "
		-	**monthlyBackupFinished** - " "
		-	**manualBackupFinished**  - fired when you start a backup using the Domoticz GUI or via **< domoticz IP:domoticz port >**/backupdatabase.php

#### timer = { ... }
A list of one ore more time 'rules' like `every minute` or `at 17:*`. See [*timer* trigger rules](#timer_trigger_rules). If any or the rules matches with the current time/date then your execute function is called. E.g.: `on = { timer = { 'between 30 minutes before sunset and 30 minutes after sunrise' } }`.

#### variables = { ... }
A list of one or more user variable-names as defined in Domoticz ( *Setup > More options > User variables*). If any of the variables listed here is updated, ( **Note**: update does not necessarily means the variable value or type changed ).the script is executed. **Note**: Script will only execute when this variable is updated directly by dzVents or by a JSON. If updated with a standard Lua script (using commandArray) or in combination with a time option like afterAAA no event will be triggered.

### execute = function(**domoticz, item, triggerInfo**) ... end
When all the above conditions are met (active == true and the on section has at least one matching rule), then this `execute` function is called. This is the heart of your script. The function has three parameters:

#### 1. (**domoticz**, item, triggerInfo)
 The [domoticz object](#Domoticz_object_API_.28Application_Programming_Interface.29). This object gives you access to almost everything in your Domoticz system, including all methods to manipulate them—like modifying switches or sending notifications. More about the [domoticz object](#Domoticz_object_API_.28Application_Programming_Interface.29) below.

#### 2. (domoticz, **item**, triggerInfo)
 Depending on what actually triggered the script, `item` is either a:

 - [customEvent](#Custom_event_API) <sup>3.0.0</sup>
 - [device](#Device_object_API),
 - [variable](#Variable_object_API_.28user_variables.29),
 - [scene](#Scene),
 - [group](#Group),
 - [timer](#timer_=_{_..._} ),
 - [system](#System_event_API), <sup>3.0.0</sup>
 - [security](#Security_Panel) or
 - [httpResponse](#Asynchronous_HTTP_requests)
 - [shellCommandResponse](#Asynchronous_shell_command_executio) <sup>3.1.0</sup>

Since you can define multiple on-triggers in your script, it is not always clear what the type is of this second parameter. In your code you need to know this in order to properly respond to the different events. To help you inspect the object you can use these attributes like `if (item.isDevice) then ... end`:

 - **isCustomEvent**: <sup>3.0.0</sup>. returns `true` if the item is a customEvent object.
 - **isDevice**: . returns `true` if the item is a Device object.
 - **isGroup**: . returns `true` if the item is a Group object.
 - **isHTTPResponse**: .  returns `true` if the item is an HTTPResponse object.
 - **isShellCommandResponse**: . <sup>3.1.0</sup>  returns `true` if the item is a shellcommandResponse object.
 - **isScene**: . returns `true` if the item is a Scene object.
 - **isSecurity**: .  returns `true` if the item is a Security object.
 - **isSystemEvent**: <sup>3.0.0</sup>.  returns `true` if the item is a system object.
 - **isTimer**: .  returns `true` if the item is a Timer object.
-  **isVariable**: .  returns `true` if the item is a Variable object.

 - **trigger**: .  *string*. the timer rule, the security state, the customEvent or the http response callback string that actually triggered your script. E.g. if you have multiple timer rules can inspect `trigger` which exact timer rule was fired.

#### 3. (domoticz, item, **triggerInfo**)
**Note**: as of version 2.4.0, `triggerInfo` has become more or less obsolete and is left in here for backward compatibility. All information is now available on the `item` parameter (second parameter of the execute function, see point 2 above).

`triggerInfo` holds information about what triggered the script. The object has two attributes:

 1. **type**:  the type of the the event that triggered the execute function, either:
		- domoticz.EVENT_TYPE_CUSTOM,<sup>3.0.0</sup>
		- domoticz.EVENT_TYPE_DEVICE,
		- domoticz.EVENT_TYPE_GROUP
		- domoticz.EVENT_TYPE_HTTPRESPONSE
		- domoticz.EVENT_TYPE_SCENE,
		- domoticz.EVENT_TYPE_SECURITY,
		- domoticz.EVENT_TYPE_SYSTEM,<sup>3.0.0</sup>
		- domoticz.EVENT_TYPE_TIMER,
		- domoticz.EVENT_TYPE_VARIABLE)

 2. **trigger**: the timer rule that triggered the script if the script was called due to a timer event, or the security state that triggered the security trigger rule. See [below](#timer_trigger_rules) for the possible timer trigger rules.
 3. **scriptName**: the name of the current script.

#### Tip: rename the parameters to better fit your needs
The names of the execute parameters are actually something you can change to your convenience. For instance, if you only have one trigger for a specific switch device, you can rename `item` it to `switch`. Or if you think `domoticz` is too long you can rename it to `d` or `dz` (might save you a lot of typing and may make your code more readable):
```Lua
return
{
	on =
	{
		devices = 'mySwitch'
	},
	execute = function(dz, mySwitch)
		dz.log(mySwitch.state, dz.LOG_INFO)
	end
}
```

### data = { ... } (optional)
The optional data section allows you to define local variables that will hold their values *between* script runs. These variables can get a value in your execute function and the next time the script is executed these variables will have their previous state restored. For more information see the section [Persistent data](#Persistent_data).

### logging = { ... } (optional)
The optional logging section allows you to override the global logging setting of dzVents as set in *Setup > Settings > Other > EventSystem > dzVents Log Level*. This can be handy when you only want this script to have extensive debug logging while the rest of your script executes silently. You have these options:

 - **level**: This is the log level you want for this script. Can be domoticz.LOG_INFO, domoticz.LOG_MODULE_EXEC_INFO, domoticz.LOG_DEBUG or domoticz.LOG_ERROR
 - **marker**: A string that is prefixed before each log message. That way you can easily create a filter in the Domoticz log to see just these messages. **marker** defaults to scriptname

Example:
```Lua
logging = {
	level = domoticz.LOG_DEBUG,
	marker = "Hey you"
	},
```

## Some trigger examples
### Custom events
Suppose you want to trigger a script not on the minute but 30 seconds later.

```Lua
return
{
	on =
	{
		timer =
		{
			'every minute',
		},

		customEvents =
		{
			'delayed',
		},
},
	execute = function(domoticz, item)
		if item.isTimer then
			domoticz.emitEvent('delayed', domoticz.time.rawTime ).afterSec(30)
		else
			domoticz.notify('Delayed', 'Event was emitted at ' .. item.data, domoticz.PRIORITY_LOW)
		end
	end
}
```

### Device changes
Suppose you have two devices—a smoke detector 'myDetector' and a room temperature sensor 'roomTemp', and you want to send a notification when either the detector detects smoke or the temperature is too high:

```Lua
return
{
	on =
	{
		devices = {
			'myDetector',
			'roomTemp'
	}
},
	execute = function(domoticz, device)
		if ((device.name == 'myDetector' and device.active) or
			(device.name == 'roomTemp' and device.temperature >= 45)) then
			domoticz.notify('Fire', 'The room is on fire', domoticz.PRIORITY_EMERGENCY)
		end
	end
}
```

### Scene / group changes
Suppose you have a scene 'myScene' and a group 'myGroup', and you want to turn on the group as soon as myScene is activated:
```Lua
return {
	on = {
		scenes = { 'myScene' }
	},
	execute = function(domoticz, scene)
		if (scene.state == 'On') then
			domoticz.groups('myGroup').switchOn()
		end
	end
}
```
Or, if you want to send an email when a group is activated at night:
```Lua
return {
	on = {
		groups = { ['myGroup'] = {'at nighttime'} }
	},
	execute = function(domoticz, group)
		if (group.state == 'On') then
			domoticz.email('Hey', 'The group is on', 'someone@the.world.org')
		end
	end
}
```
### Timer events
Suppose you want to check the soil humidity every 30 minutes during the day and every hour during the night:
```Lua
return {
	on = {
		timer = {
			'every 30 minutes at daytime',
			'every 60 minutes at nighttime'
		}
	},
	execute = function(domoticz, timer)
		domoticz.log('The rule that triggered the event was: ' .. timer.trigger')
		if (domoticz.devices('soil').moisture > 100) then
			domoticz.devices('irrigation').switchOn().forMin(60)
		end
	end
}
```
### Variable changes
Suppose you have a script that updates a variable 'myAmountOfMoney', and if that variable reaches a certain level you want to be notified:
```Lua
return
{
	on =
	{
		variables = { 'myAmountOfMoney' }
	},
	execute = function(domoticz, variable)
		-- variable is the variable that's triggered
		if (variable.value > 1000000) then
			domoticz.notify('Rich', 'You can stop working now', domoticz.PRIORITY_HIGH)
		end
	end
}
```

### Security changes
Suppose you have a group holding all the lights in your house, and you want to switch it off as soon as the alarm is activated:
```Lua
return
{
	on =
	{
		security = { domoticz.SECURITY_ARMEDAWAY }
	},
	execute = function(domoticz, security)
		domoticz.groups('All lights').switchOff()
	end
}
```

### Asynchronous HTTP Request and handling
Suppose you have some external web service that will tell you what the current energy consumption is and you want that information in Domoticz:
```Lua
return {
	on = {
		timer = { 'every 5 minutes' },
		httpResponses = { 'energyRetrieved' }
	},
	execute = function(domoticz, item)
		if (item.isTimer) then
			domoticz.openURL({
			url = 'http://url/to/service',
			method = 'GET',
			callback = 'energyRetrieved'
			})
		elseif (item.isHTTPResponse) then
			if (item.ok) then -- statusCode == 2xx
				local current = item.json.consumption
				domoticz.devices('myCurrentUsage').updateEnergy(current)
			end
		end
	end
}
```
See[ asynchronous http requests](#Asynchronous_HTTP_requests) for more information.

### Asynchronous shell command execution <sup>3.1.0</sup>
Suppose you have some local shellcommand to measure the speed of your internet connection and you want that information in Domoticz:
```Lua
return {
	on = {
		timer = { 'every 5 minutes' },
		shellCommandResponses = { 'internetspeedRetrieved' }
	},
	execute = function(domoticz, item)
		if (item.isTimer) then
		domoticz.executeShellCommand({
			command = 'speedtest-cli --json',      -- just an example
			callback = 'internetspeedRetrieved',   -- see shellCommandResponses above.
			timeout = 20,                          -- max execution time in seconds
		})
		elseif (item.isShellCommandResponse) then
			if (item.statusCode==0) then
				domoticz.devices('myCurrentdownloadspeed').updateCustomSensor(item.json.download)
			end
		end
	end
}
```
See[ asynchronous shell commands](#Asynchronous_shell_command_execution) for more information.

### System changes
Do some initial work when domoticz starts

```Lua
return
{
	on =
	{
		system =
		{
			'start',
			'backupDoneDaily',
		},

	},

	execute = function(domoticz, system)
		domoticz.log('Domoticz sends ' ..  system.type, domoticz.LOG_INFO)

		if system.type == 'start' then
			domoticz.device('initial device').switchOff().silent
			domoticz.variables('special startVar').set('domoticz started at ' .. domoticz.time.rawDateTime )
		elseif system.type == 'backupDoneDaily' then
			domoticz.notify('Domoticz backup', 'Ended. Backupfile ' .. system.data.location , domoticz.PRIORITY_LOW)
		end
	end
}
```

### Combined rules
Let's say you have a script that checks the status of a lamp and is triggered by motion detector:
```Lua
return {
	on = {
			timer = { 'every 5 minutes' },
			devices = { 'myDetector' }
	},
	execute = function(domoticz, item)
		if (item.isTimer) then
			-- the timer was triggered
			domoticz.devices('myLamp').switchOff()
		elseif (item.isDevice and item.active) then
			-- it must be the detector
			domoticz.devices('myLamp').switchOn()
		end
	end
}
```

## *timer* trigger rules
There are several options for time triggers. It is important to know that Domoticz timer events only trigger once every minute, so one minute is the smallest interval for your timer scripts. However, dzVents gives you many options to have full control over when and how often your timer scripts are called (all times are in 24hr format and all dates in dd/mm):
Keywords recognized are "at, between, every, except, in, on " ( except supported from version <sup>3.0.16</sup> onwards ).

```Lua
	on = {
		timer = {
			'every minute',				-- causes the script to be called every minute
			'every other minute',		-- minutes: xx:00, xx:02, xx:04, ..., xx:58
			'every <xx> minutes',		-- starting from xx:00 triggers every xx minutes
										-- (0 > xx < 60)
			'every hour',				-- 00:00, 01:00, ..., 23:00  (24x per 24hrs)
			'every other hour',			-- 00:00, 02:00, ..., 22:00  (12x per 24hrs)
			'every <xx> hours',			-- starting from 00:00, triggers every xx
										-- hours (0 > xx < 24)
			'at 13:45',					-- specific time
			'at *:45',					-- every 45th minute in the hour
			'at 15:*',					-- every minute between 15:00 and 16:00
			'at 12:45-21:15',			-- between 12:45 and 21:15. You cannot use '*'!
			'at 19:30-08:20',			-- between 19:30 and 8:20 (next day)
			'at 13:45 on mon,tue',		-- at 13:45 only on Mondays and Tuesdays (english)
			'on mon,tue',				-- on Mondays and Tuesdays
			'every hour on sat',		-- you guessed it correctly
			'at sunset',				-- uses sunset/sunrise/solarnoon info from Domoticz
			'at sunrise',
			'at solarnoon',				-- <sup>3.0.11</sup>
			'at civiltwilightstart',	-- uses civil twilight start/end info from Domoticz
			'at civiltwilightend',
			'at sunset on sat,sun',
			'xx minutes before civiltwilightstart',		--
			'xx minutes after civiltwilightstart',		-- Please note that these relative times
			'xx minutes before civiltwilightend',		-- cannot cross dates
			'xx minutes after civiltwilightend',		--
			'xx minutes before sunset',
			'xx minutes after sunset',
			'xx minutes before sunrise',
			'xx minutes after sunrise'
			'xx minutes before solarnoon',	-- <sup>3.0.11</sup>
			'xx minutes after solarnoon',	--<sup>3.0.11</sup>
			'between aa and bb'			-- aa/bb can be a time stamp like 15:44 (if aa > bb will cross dates)
										-- aa/bb can be sunrise/sunset/solarnoon ('between sunset and sunrise' and 'between solarnoon and sunrise' will cross dates)
										-- aa/bb can be 'xx minutes before/after sunrise/sunset/solarnoon'
			'at civildaytime',			-- between civil twilight start and civil twilight end
			'at civilnighttime',		-- between civil twilight end and civil twilight start
			'at nighttime',				-- between sunset and sunrise
			'at daytime',				-- between sunrise and sunset
			'at daytime on mon,tue',	-- between sunrise and sunset only on Mondays and Tuesdays
			'in week 1-7,44'			-- in week 1-7 or 44
			'in week 20-25,33-47'		-- between week 20 and 25 or week 33 and 47
			'in week -12, 33-'			-- week <= 12 or week >= 33
			'every odd week',
			'every even week',			-- odd or even numbered weeks
			'on 23/11',					-- on 23rd of November (dd/mm)
			'on 23/11-25/12',			-- between 23/11 and 25/12
			'on 2/3-8/3,7/8,6/10-14/10',-- between march 2 and 8, on august 7 and between October 6 and 14.
			'on */2,15/*',				-- every day in February or
										-- every 15th day of the month
			'on -3/4,4/7-',				-- before 3/4 or after 4/7

			'at 12:45-21:15 except at 18:00-18:30',	-- between 12:45 and 21:15 but not between 18:00 and 18:30 ( except supported from 3.0.16 onwards )
			'at daytime except on sun',				-- between sunrise and sunset but not on Sundays

			-- or if you want to go really wild and combine them:
				'at nighttime at 21:32-05:44 every 5 minutes on sat, sun except at 04:00', -- except supported from 3.0.16 onwards
				'every 10 minutes between 20 minutes before sunset and 30 minutes after sunrise on mon,fri,tue on 20/5-18/8'

			-- or just do it yourself:
				function(domoticz)
				-- you can use domoticz.time to get the current time
				-- note that this function is called every minute!
				-- custom code that either returns true or false
				...
			end
		},
	}
```

The timer events are triggered every minute. If such a trigger occurs, dzVents will scan all timer-based events and will evaluate the timer rules. So, if you have a rule `on mon` then the script will be executed *every minute* but only on Monday.

Be mindful of the logic if using multiple types of timer triggers. It may not make sense to combine a trigger for a specific or instantaneous time with a trigger for a span or sequence of times (like 'at sunset' with 'every 6 minutes') in the same construct. Similarly, `'between aa and bb'` only makes sense with instantaneous times for `aa` and `bb`.

**One important note: if Domoticz, for whatever reason, skips a timer event then you may miss the trigger! Therefore, you should build in some fail-safe checks or some redundancy if you have critical time-based stuff to control. There is nothing dzVents can do about it**

Another important issue: the way it is implemented right now, the `every xx minutes` and `every xx hours` is a bit limited. The interval resets at every \*:00  (for minutes) or 00:* for hours. You need an interval that is an integer divider of 60 (or 24 for the hours). So you can do every 1, 2, 3, 4, 5, 6, 10, 12, 15, 20 and 30 minutes only.

# The domoticz object
The domoticz object passed as the first parameter to the `execute` function contains everything you need to interact with your domotica system. It provides all information made available by the domoticz event system about your custom- and system events, devices, scenes, groups, hardware modules and variables and has methods needed to inspect and manipulate them. Getting this information is easy:

`domoticz.time.isDayTime` or `domoticz.devices('My sensor').temperature` or `domoticz.devices('My sensor').lastUpdate.minutesAgo`.

The domoticz object consists of a logically arranges structure. It harbors all methods to manipulate Domoticz. dzVents will take care to notify Domoticz with all your intended changes and actions. Earlier you had to fill a commandArray with obscure commands. That's no longer the case.

Some more examples:
 `domoticz.devices(123).switchOn().forMin(5).afterSec(10)` or
 `domoticz.devices('My dummy sensor').updateBarometer(1034, domoticz.BARO_THUNDERSTORM)`.

One tip before you get started:
**Make sure that all your devices have unique names. dzVents will give you warnings in the logs if device names are not unique.**

##  Domoticz object API (Application Programming Interface)
The domoticz object holds all information about your Domoticz system. It has global attributes and methods to query and manipulate your system. It also has a collection of **devices**, **variables** (user variables in Domoticz), **scenes**, **groups**, **hardware**. Each of these collections has four iterator functions: `find(), forEach()`, `filter()` and `reduce()` to make searching for devices easier. See [Looping through the collections: iterators](#Looping_through_the_collections:_iterators).

### Domoticz attributes and methods
 - **devices(idx/name)**: *Function*. A function returning a device by idx or name: `domoticz.devices(123)` or `domoticz.devices('My switch')`. For the device API see [Device object API](#Device_object_API). To iterate over all devices do: `domoticz.devices().forEach(..)`. See [Looping through the collections: iterators](#Looping_through_the_collections:_iterators). Note that you cannot do `for i, j in pairs(domoticz.devices()) do .. end`.
 - **dump([osfile]<sup>3.0.0</sup>)**: *Function*. Dump all domoticz.settings attributes to the Domoticz log. This ignores the log level setting.
 - **email(subject, message, mailTo [, delay]<sup>3.0.10</sup>)**: *Function*. Send email. Optional parm delay is delay in seconds.
 - **emitEvent(name,[extra data ])**:*Function*. <sup>3.0.0</sup> Have Domoticz 'call' a customEvent. If you just pass a name then Domoticz will execute the script(s) that subscribed to the named customEvent after your script has finished. You can optionally pass extra information as a string or table. A table will be automatically converted into a json string and converted back to a table in the subscribed script(s). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **executeShellCommand(command/options)**: *Function*. <sup>3.1.0</sup> Have Domoticz 'call' a command. If you just pass a command then Domoticz will execute the command after your script has finished but you will not get notified.  If you pass options table with a callback then you have the possibility to receive the results of the request in a dzVents script. Read more about [asynchronous shell commands](#Asynchronous_shell_command_execution) with dzVents. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
**Note Make sure that when you use this function on a Windows system and use one or more of the special chars &<>()@^| outside double quotes, you will have to escape these with a ^ char **
- **groups(idx/name)**: *Function*: A function returning a group by name or idx. Each group has the same interface as a device. To iterate over all groups do: `domoticz.groups().forEach(..)`. See [Looping through the collections: iterators](#Looping_through_the_collections:_iterators). Note that you cannot do `for i, j in pairs(domoticz.groups()) do .. end`. Read more about [Groups](#Group).
 - **hardwareInfo(idx/name)**: <sup>3.0.6</sup> *Function*: A function returning hardwareInfo of a hardware module by name or idx. The return of the function is a table with attributes name, type, typeValue, deviceNames (table with names of all active devices defined on this hardware) and deviceIds (table with idx of all active devices defined on this hardware)
 - **hardware(idx/name)**:  <sup>3.0.7</sup> *Function*: A function returning a hardware module by name or idx. Each hardware has an interface comparable to group. To iterate over all hardware do: `domoticz.hardware().forEach(..)`. See [Looping through the collections: iterators](#Looping_through_the_collections:_iterators). Note that you cannot do `for i, j in pairs(domoticz.hardware()) do .. end`. Read more about [Hardware](#Hardware).
 - **helpers**: *Table*. Collection of shared helper functions available to all your dzVents scripts. See [Shared helper functions](#Shared_helper_functions).
 - **log(message, [level])**: *Function*. Creates a logging entry in the Domoticz log but respects the log level settings. You can provide the loglevel: `domoticz.LOG_INFO`, `domoticz.LOG_DEBUG`, `domoticz.LOG_ERROR` or `domoticz.LOG_FORCE`. In Domoticz settings you can set the log level for dzVents.
- **moduleLabel**: <sup>3.0.3</sup> Module (script) name without extension.
 - **notify(subject, message [,priority][,sound][,extra][,subsystem][,delay]<sup>3.0.10</sup> )**: *Function*. Send a notification (like Prowl). Priority can be like `domoticz.PRIORITY_LOW, PRIORITY_MODERATE, PRIORITY_NORMAL, PRIORITY_HIGH, PRIORITY_EMERGENCY`. `extra` is notification subsystem specific. For NSS_FIREBASE you can use this field to specify the target mobile ('midx_1', midx_2, etc..). For sound see [list of dzVents Constants](#Constants) for the SOUND constants below. `subsystem` defaults to all subsystems but can be one subsystem or a table containing one or more notification subsystems. See [list of dzVents Constants](#Constants) for `domoticz.NSS_subsystem` types. Delay is delay in seconds
 - **openURL(url/options)**: *Function*. Have Domoticz 'call' a URL. If you just pass a url then Domoticz will execute the url after your script has finished but you will not get notified.  If you pass a table with options then you have to possibility to receive the results of the request in a dzVents script. Read more about [asynchronous http requests](#Asynchronous_HTTP_requests) with dzVents. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **scenes(idx/name)**: *Function*: A function returning a scene by name or id. Each scene has the same interface as a device. See [Device object API](#Device_object_API). To iterate over all scenes do: `domoticz.scenes().forEach(..)`. See [Looping through the collections: iterators]. (#Looping_through_the_collections:_iterators). Note that you cannot do `for i, j in pairs(domoticz.scenes()) do .. end`. Read more about [Scenes](#Scene).
 - **security**: Holds the state of the security system e.g. `Armed Home` or `Armed Away`.
 - **sendCommand(command, value[, delay] <sup>3.0.10</sup>)**: Generic (low-level)command method (adds it to the commandArray) to the list of commands that are being sent back to domoticz. Optional parm delay is delay in seconds *There is likely no need to use this directly. Use any of the device methods instead (see below).*
 - **settings**:
	- **domoticzVersion**: domoticz version string.
	- **dzVentsVersion**: dzVents version string.
	- **location**
		- **latitude**: domoticz settings locations latitude.
		- **longitude**: domoticz settings locations longitude.
		- **name**: domoticz settings location Name.
	- **secureServer**: *boolean* <sup>3.1.1</sup> true when domoticz can be accessed via https.
	- **serverPort**: webserver listening port used by dzVents to access domoticz API.
	- **url**: internal url to access the API service.
	- **webRoot**: `webroot` <sup>3.1.1</sup> value as specified when starting the Domoticz service (or default).
	- **wwwBind**: `wwwbind` <sup>3.1.4</sup> value as specified when starting the Domoticz service (or default).
 - **sms(message [, delay] <sup>3.0.10</sup> )**: *Function*. Sends an sms if it is configured in Domoticz. Optional parm delay is delay in seconds.
 - **snapshot(cameraID(s) or camera Name(s)<sup>3.0.13</sup>,subject)**: *Function*. Sends email with a camera snapshots if email is configured and set for attachments in Domoticz. Send 1 or multiple camerIDs -names in ; separated string or array.

```Lua
	dz.snapshot()                  -- defaults to id = 1, subject domoticz
	dz.snapshot(1,'dz')            -- subject dz
	dz.snapshot('1;2;3')           -- ; separated string
	dz.snapshot({1,4,7})           --  table
	dz.snapshot('test;test2')      -- ; separated string
	dz.snapshot({'test', 'test2'}) -- table
```

 - **startTime**: *[Time Object](#Time_object)*. Returns the startup time of the Domoticz service.
 - **systemUptime**: *Number*. Number of seconds the system is up.
 - **time**: *[Time Object](#Time_object)*: Current system time. Additional to Time object attributes:
	- **isDayTime**: *Boolean*
	- **isNightTime**: *Boolean*
	- **isCivilDayTime**: *Boolean*.
	- **isCivilNightTime**: *Boolean*.
	- **isToday**: *Boolean*. Indicates if the device was updated today
	- **sunriseInMinutes**: *Number*. Number of minutes since midnight when the sun will rise.
	- **sunsetInMinutes**: *Number*. Number of minutes since midnight when the sun will set.
	- **civTwilightStartInMinutes**: *Number*.  Number of minutes since midnight when the civil twilight will start.
	- **civTwilightEndInMinutes**: *Number*. Number of minutes since midnight when the civil twilight will end.
 - **triggerHTTPResponse([httpResponse], [delay], [message])**: *Function*. Creates a callback by sending a logmessage. httpResponse defaults to scriptname, delay defaults to 0 (immediate), message defaults to httpResponse. Left in for backward compatibility only. You can use emitEvent / customEvents now.
 - **triggerIFTTT(makerName [,sValue1, sValue2, sValue3])**: *Function*. Have Domoticz 'call' an IFTTT maker event. makerName is required, 0-3 sValue's are optional. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **utils**: . A subset of handy utilities:
		Note that these functions must be preceded by domoticz.utils. If you use more then a few declare something like local _u = domoticz.utils at the beginning of your script and use _u.functionName in the remainder. Example:

	```{.lua}
		_u = domoticz.utils
		print(_u.rightPad('test',10) .. '|||') -- =>  test		|||
	```

	- **\_.lodashFunctions**: This is an entire collection with very handy Luafunctions. Read more about
		[lodash](#lodash_for_Lua "wikilink").  E.g.:
		``` {.lua}
		domoticz.utils._.size({'abc', 'def'}))` Returns 2.
		```

	- **cameraExists(parm)**: *Function*: returns name when entered with a valid camera ^3.0.12^ or cameraID and return ID when entered with valid cameraName or false when not a camera, cameraID or cameraName of an existing camera
	- **deviceExists(parm)**: *Function*: returns name when entered with a valid device ^3.0.12^ or deviceID and returns ID when entered with valid deviceName or false when not a device, deviceID or deviceName of an existing (and active) device.
		Example:

		``` {.lua}
		local dz = domoticz
		local _u = dz.utils
		if _u.deviceExists('myDevice') then
			dz.devices('myDevice').switchOn()
		else
			dz.log('Device myDevice does not exist!', dz.LOG_ERROR)
		end

		local myName = _u.deviceExists(123)
		if myName then
			dz.log('Device 123 is ' .. myName, dz.LOG_INFO)
		else
			dz.log('Device 123 does not exist', dz.LOG_ERROR)
		end
		```

	- **dumpTable(table,[levelIndicator],[osfile]<sup>3.0.0</sup>))**: *Function*: print table structure and contents to log
	- **fileExists(path)**: *Function*:  Returns `true` if the file (with full path) exists.
	- **fromBase64(string)**: *Function*: Decode a base64 string
	- **fromJSON(json, fallback)**: *Function*. Turns a json string to a Lua table. Example: `local t = domoticz.utils.fromJSON('{ "a": 1 }')`. Followed by: `print( t.a )` will print 1. Optional 2nd param fallback will be returned if json is nil or invalid.
	- **fromXML(xml, fallback )**: *Function*: Turns a xml string to a Lua table. Example: `local t = domoticz.utils.fromXML('<testtag>What a nice feature!</testtag>') Followed by: `print( t.texttag)` will print What a nice feature! Optional 2nd param fallback will be returned if xml is nil or invalid.
	- **fuzzyLookup([string|array of strings], parm)**: *Function*: <sup>3.0.14</sup>. Search fuzzy matching string in parm. If parm is string it returns a number (lower is better match). If parm is array of strings it returns the best matching string.
	- **groupExists(parm)**: *Function*: returns name when entered with a valid group ^3.0.12^ or groupID and return ID when entered with valid groupName or false when not a group, groupID or groupName of an existing group
	- **hardwareExists(parm)**: *Function*: <sup>3.0.7</sup> returns name when entered with valid hardwareID or ID when entered with valid hardwareName or false when not a hardwareID or hardwareName of an existing (and active  )hardware module
	- **inTable(table, searchString)**: *Function*: Returns `"key"` if table has searchString as a key, `"value"` if table has searchString as value and `false` otherwise.
	- **isJSON(string[, content])**: *Function*: <sup>3.0.4</sup> Returns `true` if content is 'application/json' or string is enclosed in {} and `false` otherwise.
	- **isXML(string[, content])**: *Function*: <sup>3.0.4</sup> Returns `true` if content is 'text/xml' or 'application/xml' or string is enclosed in <> and `false` otherwise.
	- **leftPad(string, length [, character])**: *Function*: Precede string with given character(s) (default = space) to given length.
	- **centerPad(string, length [, character])**: *Function*: Center string by preceding and succeeding with given character(s) (default = space) to given length.
	- **numDecimals(number [, integer [, decimals ]])**: *Function*: Format number to float representation.
	*Examples:*
	```Lua
			domoticz.utils.numDecimals(12.23, 4, 4) -- => 12.2300,
			domoticz.utils.numDecimals (12.23,1,1) -- => 12.2,
			domoticz.utils.leadingZeros(domoticz.utils.numDecimals (12.23,4,4),9) -- => 0012.2300
	```
	- **osCommand(cmd)**: *Function*: <sup>3.0.13</sup> Execute an OS command and return result and returncode. Note: For long running scripts `domoticz.executeShellCommand()` should be used, See [Asynchronous shell commands](#Asynchronous_shell_command_execution) for more information.
	- **osExecute(cmd)**: *Function*: Execute an OS command (no return). Note: For long running scripts `executeShellCommand()` should be used. See [Asynchronous shell commands](#Asynchronous_shell_command_execution) for more information.
	- **rightPad(string, length [, character])**: *Function*: Succeed string with given character(s) (default = space) to given length.
	- **round(number, [decimalPlaces])**: *Function*. Helper function to round numbers. Default decimalPlaces is 0.
	- **sceneExists(parm)**: *Function*: returns name when entered with valid scene ^3.0.12^ or sceneID and return ID when entered with valid sceneName or false when not a scene, sceneID or sceneName of an existing scene
	- **setLogMarker([marker])**: *Function*: set logMarker to 'marker'. Defaults to scriptname. Can be used to change logMarker based on flow in script
	- **stringSplit(string [,separator ] [,convertNumber ][,convertNil ]<sup>3.0.17</sup>)**:*Function*. Helper function to split a line in separate words. Default separator is space. Return is a table with separated words. Default convertNumber is false when set to true it will convert strings to number where possible. Word is ignored when nil, unless convertNil is set; in that case word will be set to the convertNil value.
	- **stringToSeconds(str)**:  *Function*: <sup>3.0.1</sup>  Returns number of seconds between now and str.
	*Examples:*

		```Lua
		domoticz.utils.stringToSeconds('09:00')					-- number of seconds between now and 09:00 hr.
		domoticz.utils.stringToSeconds('08:53:30 on fri')		-- number of seconds between now and Friday at 08:53:30
		domoticz.utils.stringToSeconds('08:53:30 on sat, sun')	-- number of seconds between now and Saturday or Sunday at 08:53:30 ;whatever comes first
		```
	- **toBase64(string)**: *Function*: ) Encode a string to base64
	- **toCelsius(f, relative)**: *Function*. Converts temperature from Fahrenheit to Celsius along the temperature scale or when relative == true it uses the fact that 1F = 0.56C. So `toCelsius(5, true)` returns 5F*(1/1.8) = 2.78C.
	- **toJSON(luaTable)**: *Function*.  Converts a Lua table to a json string.
	- **toXML(luaTable, [header])**: *Function*. Converts a Lua table to a xml string.
	- **urlDecode(s)**: *Function*. Simple deCoder to convert a string with escaped chars (%20, %3A and the likes) to human readable format.
	- **urlEncode(s, [strSub])**: *Function*. Simple url encoder for string so you can use them in `openURL()`. `strSub` is optional and defaults to + but you can also pass %20 if you like/need.
	- **variableExists(parm)**: *Function*: returns name when entered with a valid variable ^3.0.12^ or variableID and return ID when entered with valid variableName or false when not a variable, variableID or variableName of an existing variable
	- **leadingZeros(number, length)**: *Function*: Precede number with given zeros to given length.
	- **variables(idx/name)**: *Function*. A function returning a variable by it's name or idx. See  [Variable object API]
(#Variable_object_API_.28user_variables.29) for the attributes. To iterate over all variables do: `domoticz.variables().forEach(..)`. See [Looping through the collections: iterators](#Looping_through_the_collections:_iterators). **Note that you cannot do `for i, j in pairs(domoticz.variables()) do .. end`**.

### Looping through the collections: iterators
The domoticz object has these collections (tables): devices, scenes, groups, variables, changedDevices and changedVariables. You cannot use the `pairs()` or `ipairs()` functions. Therefore dzVents has four iterator methods:

 1. **find(function)**: Returns the item in the collection for which `function` returns true. When no item is found `find` returns nil.
 2. **forEach(function)**: Executes function once per array element. The function receives the item in the collection (device or variable). If the function returns *false*, the loop is aborted.
 3. **filter(function / table)**: returns items in the collection for which the function returns true. You can also provide a table with names and/or ids.
 4. **reduce(function, initial)**: Loop over all items in the collection and do some calculation with it. You call it with the function and the initial value. Each iteration the function is called with the accumulator and the item in the collection. The function does something with the accumulator and returns a new value for it.

#### Examples:

find():
```Lua
	local myDevice = domoticz.devices().find(function(device)
		return device.name == 'myDevice'
	end)
	domoticz.log('Id: ' .. myDevice.id)
```
forEach():
```Lua
	domoticz.devices().forEach(function(device)
		if (device.batteryLevel < 20) then
		-- do something
		end
	end)
```
filter():
```Lua
	local deadDevices = domoticz.devices().filter(function(device)
		return (device.lastUpdate.minutesAgo > 60)
	end)
	deadDevices.forEach(function(zombie)
		-- do something
	end)
```
or
```Lua
	local livingLights = {
		'window',
		'couch',
		33, -- kitchen light id
	}
	local lights = domoticz.devices().filter(livingLights)
	lights.forEach(function(light)
		-- do something
		light.switchOn()
	end)
```

Of course you can chain:
```Lua
	domoticz.devices().filter(function(device)
		return (device.lastUpdate.minutesAgo > 60)
	end).forEach(function(zombie)
		-- do something with the zombie
	end)
```
Using a reducer to count all devices that are switched on:
```Lua
	local count = domoticz.devices().reduce(function(acc, device)
		if (device.state == 'On') then
			acc = acc + 1 -- increase the accumulator
		end
		return acc -- always return the accumulator
	end, 0) -- 0 is the initial value for the accumulator
```

### Constants
The domoticz object has these constants available for use in your code e.g. `domoticz.LOG_INFO`.

**IMPORTANT:  you have to prefix these constants with the name of your domoticz object. Example: `domoticz.ALERTLEVEL_RED`**:

 - **ALERTLEVEL_GREY, ALERTLEVEL_GREEN, ALERTLEVEL_ORANGE, ALERTLEVEL_RED, ALERTLEVEL_YELLOW**: for updating text sensors.
 - **BASETYPE_CUSTOM_EVENT <sup>3.0.0</sup>,BASETYPE_DEVICE, BASETYPE_SCENE, BASETYPE_GROUP, BASETYPE_HARDWARE** <sup>3.0.7</sup>, **BASETYPE_VARIABLE, BASETYPE_SECURITY, BASETYPE_TIMER, BASETYPE_HTTP_RESPONSE, BASETYPE_SYSTEM**<sup>3.0.0</sup>: indicators for the various object types that are passed as the second parameter to the execute function. E.g. you can check if an object is a device object:
	```Lua
	if (item.baseType == domoticz.BASETYPE_DEVICE) then ... end
	```

 - **BARO_CLOUDY, BARO_CLOUDY_RAIN, BARO_STABLE, BARO_SUNNY, BARO_THUNDERSTORM, BARO_NOINFO, BARO_UNSTABLE, BARO_COMPUTE** : for updating barometric values.
 - **EVENT_TYPE_DEVICE, EVENT_TYPE_VARIABLE, EVENT_TYPE_CUSTOM**, <sup>3.0.0</sup> **EVENT_TYPE_SECURITY,  EVENT_TYPE_HTTPRESPONSE, EVENT_TYPE_SYSTEM,** <sup>3.0.0</sup> **EVENT_TYPE_TIMER**: triggerInfo types passed to the execute function in your scripts.
 - **EVOHOME_MODE_AUTO, EVOHOME_MODE_TEMPORARY_OVERRIDE, EVOHOME_MODE_PERMANENT_OVERRIDE, EVOHOME_MODE_FOLLOW_SCHEDULE**: mode for EvoHome system.
 - **EVOHOME_MODE_AUTO, EVOHOME_MODE_AUTOWITHRESET, EVOHOME_MODE_AUTOWITHECO, EVOHOME_MODE_AWAY, EVOHOME_MODE_DAYOFF, EVOHOME_MODE_CUSTOM, EVOHOME_MODE_HEATINGOFF** : mode for EvoHome controller
 - **HUM_COMFORTABLE, HUM_DRY, HUM_NORMAL, HUM_WET, HUM_COMPUTE** <sup>3.0.15</sup>: constant for humidity status.
 - **INTEGER, FLOAT, STRING, DATE, TIME**: variable types.
 - **LOG_DEBUG, LOG_ERROR, LOG_INFO, LOG_FORCE: for logging messages. LOG_FORCE is at the same level as LOG_ERROR.
 - **NSS_CLICKATELL** <sup>3.1.3</sup>, **NSS_FIREBASE, NSS_FIREBASE_CLOUD_MESSAGING, NSS_GOOGLE_DEVICES,** <sup>3.0.10</sup> <sup>Only with installed casting plugin</sup>, **NSS_HTTP, NSS_KODI, NSS_LOGITECH_MEDIASERVER, NSS_NMA,NSS_PROWL, NSS_PUSHALOT, NSS_PUSHBULLET, NSS_PUSHOVER, NSS_PUSHSAFER, NSS_TELEGRAM, NSS_GOOGLE_CLOUD_MESSAGING** <sup>deprecated by Google and replaced by firebase</sup>: for notification subsystem
 - **PRIORITY_LOW, PRIORITY_MODERATE, PRIORITY_NORMAL, PRIORITY_HIGH, PRIORITY_EMERGENCY**: for notification priority.
 - **SECURITY_ARMEDAWAY, SECURITY_ARMEDHOME, SECURITY_DISARMED**: for security state.
 - **STATE_IDLE, STATE_COOLING, STATE_HEATING**: for thermostat operating state.
 - **SOUND_ALIEN , SOUND_BIKE, SOUND_BUGLE, SOUND_CASH_REGISTER, SOUND_CLASSICAL, SOUND_CLIMB , SOUND_COSMIC, SOUND_DEFAULT, SOUND_ECHO, SOUND_FALLING, SOUND_GAMELAN, SOUND_INCOMING, SOUND_INTERMISSION, SOUND_MAGIC, SOUND_MECHANICAL, SOUND_NONE, SOUND_PERSISTENT, SOUND_PIANOBAR , SOUND_SIREN, SOUND_SPACEALARM, SOUND_TUGBOAT, SOUND_UPDOWN**: for notification sounds.

## Custom event API
If you have a dzVents script that is triggered by a customEvent in Domoticz then the second parameter passed to the execute function will be a *notification* object. The object.type = customEvent (isCustomEvent: true) and object.data contains the passed data to this script. object.trigger is the name of the customEvent that triggered the script.

## Device object API
If you have a dzVents script that is triggered by switching a device in Domoticz then the second parameter passed to the execute function will be a *device* object. Also, each device in Domoticz can be found in the `domoticz.devices()` collection (see above). The device object has a set of fixed attributes like *name* and *idx*. Many devices, however, have different attributes and methods depending on their (hardware)type, subtype and other device specific identifiers. It is possible that you will get an error if you call a method on a device that doesn't support it, so please check the device properties for your specific hardware to see which are supported (can also be done in your script code!).

dzVents recognizes most of the different device types in Domoticz and creates the proper attributes and methods. It is possible that your device type has attributes that are not recognized; if that's the case, please create a ticket in the Domoticz [issue tracker on GitHub](https://github.com/domoticz/domoticz/issues), and an adapter for that device will be added.

If for some reason you miss a specific attribute or data for a device, then likely the `rawData` attribute will have that information.

## Device attributes and methods for all devices
 - **active**: *Boolean*. Is true for some common states like 'On' or 'Open' or 'Motion'. Same as bState.
 - **baseType** *String*. 'camera', 'device', 'group', 'scene', 'hardware', 'uservariable' or 'security'
 - **batteryLevel**: *Number* If applicable for that device then it will be from 0-100.
 - **bState**: *Boolean*. Is true for some common states like 'On' or 'Open' or 'Motion'. Better to use active.
 - **changed**: *Boolean*. True if the device was updated. **Note**: This does not necessarily means the device state or value changed.
 - **customImage**: *Number*. When customImage is used for the device-icon this will be the icon-number. It will be 0 if the icon was not changed.
 - **description**: *String*. Description of the device.
 - **deviceId**: *String*. Another identifier of devices in Domoticz. dzVents uses the id(x) attribute. See device list in Domoticz' settings table.
 - **deviceSubType**: *String*. See Domoticz devices table in Domoticz GUI.
 - **deviceType**: *String*. See Domoticz devices table in Domoticz GUI.
 - **dump()**: *Function*. Dump all attributes to the Domoticz log. This ignores the log level setting.
 - **dumpSelection([{'attributes'} 'functions' 'tables'])**: *Function*. <sup>3.0.5</sup>  Dump attributes, function-names or table-names to the Domoticz log. This ignores the log level setting.
 - **hardwareName**: *String*. See Domoticz devices table in Domoticz GUI.
 - **hardwareId**: *Number*. See Domoticz devices table in Domoticz GUI.
 - **hardwareType**: *String*. See Domoticz devices table in Domoticz GUI.
 - **hardwareTypeValue**: *Number*. See Domoticz devices table in Domoticz GUI.
 - **icon**: *String*. Name of the icon in Domoticz GUI.
 - **id**: *Number*. Index of the device. You can find the index in the device list (idx column) in Domoticz settings. It's not truly an index but is unique enough for dzVents to be treated as an id.
 - **idx**: *Number*. Same as id: index of the device
 - **inActive**: *Boolean*. <sup>3.1.0</sup> true if active is false and vice versa. If active is nil inActive is also nil
 - **lastUpdate**: *[Time Object](#Time_object)*: Time when the device was updated. **Note: The lastUpdate for devices that triggered the script at hand is in fact the previousUpdate time. The real lastUpdate time for these "script triggering" devices is the current time.
 - **name**: *String*. Name of the device.
 - **nValue**: *Number*. Numerical representation of the state.
 - **protected**: *Boolean*. True when device / scene / group is protected. False otherwise.
 - **protectionOff()**: *Function*. switch protection to off. Supports some [command options]
 - **protectionOn()**: *Function*. switch protection to on. Supports some [command options] !! **Note: domoticz protects against GUI and API access only. switchOn / switchOff type Blockly / Lua / dzVents commands are not influenced (because they are executed as admin user)
 - **rawData**: *Table*: All values are *String* types and hold the raw data received from Domoticz.
 - **rename(newName)**: *Function*. Change current devicename to new devicename Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **setDescription(description)**: *Function*. Generic method to update the description for all devices, groups and scenes. E.g.: device.setDescription(device.description .. '/nChanged by '.. item.trigger .. 'at ' .. domoticz.time.raw). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **setIcon(iconNumber)**: *Function*. method to update the icon for devices. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **setState(newState)**: *Function*. Generic update method for switch-like devices. E.g.: device.setState('On'). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **setValues(nValue,[ sValue1, sValue2, ...])**: *Function*. Generic alternative method to update device nValue, sValues. Uses domoticz JSON API to force subsequent pushes like influxdb and MQTT. nValue required but when set to nil it defaults to current nValue. sValue parms are optional and can be many. <sup>3.0.8</sup> If one of sValue parms is 'parsetrigger', subsequent eventscripts will be triggered.
 - **state**: *String*. For switches, holds the state like 'On' or 'Off'. For dimmers that are on, it is also 'On' but there is a level attribute holding the dimming level. **For selector switches** (Dummy switch) the state holds the *name* of the currently selected level. The corresponding numeric level of this state can be found in the **rawData** attribute: `device.rawData[1]`.
 - **signalLevel**: *Number* If applicable for that device then it will be from 0-100.
 - **switchType**: *String*. See Domoticz devices table in Domoticz GUI(Switches tab). E.g. 'On/Off', 'Door Contact', 'Motion Sensor' or 'Blinds'
 - **sValue**: *String*. Returns the sValue (string Value) of a device.
 - **switchTypeValue**: *Number*. See Domoticz devices table in Domoticz GUI.
 - **timedOut**: *Boolean*. Is true when the device couldn't be reached.
 - **unit**: *Number*. Device unit. See device list in Domoticz' settings for the unit.
 - **update(< params >)**: *Function*. Generic update method. Accepts any number of parameters that will be sent back to Domoticz. There is no need to pass the device.id here. It will be passed for you. Example to update a temperature: `device.update(0,12)`. This will eventually result in a commandArray entry `['UpdateDevice']='<idx>|0|12'`. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
- **updateQuiet(nValue, sValue)**: *Function*. <sup>3.0.18</sup> Uses the JSON/API to send nValue and/or sValue to domoticz (at least one should be supplied). Will update the device status in the GUI and trigger events but will not execute the action on the device. Only useful when the status in the GUI is not in line with the actual status. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

### Device attributes and methods for specific devices
Note that if you do not find your specific device type here you can always inspect what is in the `rawData` attribute. Please let us know that it is missing so we can write an adapter for it (or you can write your own and submit it). Calling `myDevice.dump()` will dump all attributes and values for myDevice to the Domoticz log.

#### Air quality
 - **co2**: *Number*. PPM
 - **quality**: *String*. Air quality string.
 - **updateAirQuality(ppm)**: Pass the CO<sub>2</sub> concentration. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Alert sensor
 - **color**: *Number*. Color of the alert. See domoticz color constants for possible values.
 - **text**: *String*
 - **updateAlertSensor(level, text)**: *Function*. Level can be domoticz.ALERTLEVEL_GREY, ALERTLEVEL_GREEN, ALERTLEVEL_YELLOW, ALERTLEVEL_ORANGE, ALERTLEVEL_RED. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Ampère, 1-phase
 - **current**: *Number*. Ampère.
 - **updateCurrent(current)**: *Function*. Current in Ampère. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Ampère, 3-phase
 - **current1**, **current2**, **current3** : *Number*. Ampère.
 - **updateCurrent(current1, current2, current3)**: *Function*. Currents in Ampère. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Barometer sensor
 - **barometer**. *Number*. Barometric pressure.
 - **forecast**: *Number*.
 - **forecastString**: *String*.
 - **updateBarometer(pressure, forecast)**: *Function*. Update barometric pressure. Forecast can be domoticz.BARO_STABLE, BARO_SUNNY, BARO_CLOUDY, BARO_UNSTABLE, BARO_THUNDERSTORM. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Counter, managed Counter, counter incremental
 - **counter**: *Number*
 - **counterToday**: *Number*. Today's counter value.
 - **updateCounter(value)**: *Function*. **Overwrite current value for managed and standard counters; increment for incremental counters !!**. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **updateHistory( date, sValues )**: *Strings* <sup>3.1.3</sup> *function* Managed counter only! Used to write values to short or long term storage.
```Lua
	local mCounter = domoticz.devices('device name')
	mCounter.updateHistory('2021-01-22', 10;1777193') -- Write to long term storage (meter_calendar table)
	mCounter.updateHistory('2021-01-22 10:05:02', 10;1777193') -- Write to short term storage (meter table)
```
 - **incrementCounter(value)**: *Function*. (counter incremental) Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29). To update with a complete new value you will have to do some calculation and take the counter divider into account.
```Lua
	local newValue = value
	local iCounter = domoticz.devices('device name')
	local counterDivider = x -- x is depending on the counter divider set for the device
	iCounter.incrementCounter( ( -1 * iCounter.counter * counterDivider ) + newValue )
```
 - **valueQuantity**: *String*. For counters.
 - **valueUnits**: *String*.

#### Custom sensor
 - **sensorType**: *Number*.
 - **sensorUnit**: *String*:
 - **sensorValue**: <sup>3.0.11</sup> *Number* where applicable; else *String*:
 - **updateCustomSensor(value)**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Distance sensor
 - **distance**: *Number*.
 - **updateDistance(distance)**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Electric usage
 - **actualWatt**: *Number*. Current Watt usage.
 - **WhActual**: *Number*. Current Watt usage. (please use actualWatt)
 - **updateEnergy(energy)**: *Function*. In Watt. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Evohome (controller)
 - **mode**: *string* <sup>3.0.0</sup>.
 - **setMode(mode, dparm, action, ooc )**: *Function*. set mode for controller. Mode can be domoticz.EVOHOME_MODE_AUTO, domoticz.EVOHOME_MODE_AUTOWITHRESET, domoticz.EVOHOME_MODE_AUTOWITHECO, domoticz.EVOHOME_MODE_AWAY, domoticz.EVOHOME_MODE_DAYOFF, domoticz.EVOHOME_MODE_CUSTOM or domoticz.EVOHOME_MODE_HEATINGOFF. dParm <optional> can be a future time string (in ISO 8601 format e.g.: `os.date("!%Y-%m-%dT%TZ")`), a future time object, a future time as number of seconds since epoch or a number representing a positive offset in minutes (max 1 year). action <optional> (1 = run on action script, 0 = disable), ooc <optional> (1 = only trigger the event & log on change, 0 = always trigger & log)

#### Evohome (hotWater)
 - **state**: *string*  ('On' or 'Off')
 - **mode**: *string*
 - **untilDate**: *string in ISO 8601 format* or n/a.
 - **setHotWater(state, mode, until)**: *Function*. set HotWater Mode can be domoticz.EVOHOME_MODE_AUTO, domoticz.EVOHOME_MODE_TEMPORARY_OVERRIDE, domoticz.EVOHOME_MODE_FOLLOW_SCHEDULE or domoticz.EVOHOME_MODE_PERMANENT_OVERRIDE You can provide an until date (in ISO 8601 format for domoticz.EVOHOME_MODE_TEMPORARY_OVERRIDE e.g.: `os.date("!%Y-%m-%dT%TZ")`).

#### Evohome (relay)
 - **state**: *string*  ('On' or 'Off')
 - **level**: *number*  (level percentage)
 - **active**: *boolean* (true when state is not equal to 'Off', false otherwise)

#### Evohome (zones)
 - **setPoint**: *Number*.
 - **mode**: *string* .
 - **untilDate**: *string in ISO 8601 format* or n/a .
 - **updateSetPoint(setPoint, mode, until)**: *Function*. Update set point. Mode can be domoticz.EVOHOME_MODE_AUTO, domoticz.EVOHOME_MODE_TEMPORARY_OVERRIDE, domoticz.EVOHOME_MODE_FOLLOW_SCHEDULE or domoticz.EVOHOME_MODE_PERMANENT_OVERRIDE. You can provide an until date (in ISO 8601 format e.g.: `os.date("!%Y-%m-%dT%TZ")`). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Gas
 - **counter**: *Number*. Value in m<sup>3</sup>
 - **counterToday**: *Number*. Value in m<sup>3</sup>
 - **updateGas(usage)**: *Function*. Usage in **dm<sup>3</sup>** (liters). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Group
 - **devices()**: *Function*. Returns the collection of associated devices. Supports the same iterators as for `domoticz.devices()`: `forEach()`, `filter()`, `find()`, `reduce()`. See [Looping through the collections: iterators](#Looping_through_the_collections:_iterators). Note that the function doesn't allow you to get a device by its name or id. Use `domoticz.devices()` for that.
 - **protectionOff()**: *Function*. switch protection to off. Supports some [command options]
 - **protectionOn()**: *Function*. switch protection to on. Supports some [command options]
 - **rename(newName)**: *Function*. Change current group-name to new group-name Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **switchOff()**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **switchOn()**: Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **toggleGroup()**: Toggles the state of a group. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Hardware <sup>3.0.7</sup>
 - **devices()**: *Function*. Returns the collection of associated devices. Supports the same iterators as for `domoticz.devices()`: `forEach()`, `filter()`, `find()`, `reduce()`. See [Looping through the collections: iterators](#Looping_through_the_collections:_iterators). Note that the function doesn't allow you to get a device by its name or id. Use `domoticz.devices()` for that.
 - **isHardware**:  *Boolean*
 - **isPythonPlugin**: *Boolean*
 - **type**: : *String*
 - **typeValue**: : *Number*

#### Humidity sensor
 - **humidity**: *Number*
 - **humidityStatus**: *String*
 - **humidityStatusValue**: *Number*. Value matches with domoticz.HUM_NORMAL, -HUM_DRY, HUM_COMFORTABLE, -HUM_WET.
 - **updateHumidity(humidity, status)**: Update humidity. status can be domoticz.HUM_NORMAL, HUM_COMFORTABLE, HUM_DRY, HUM_WET. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Kodi
 - **kodiExecuteAddOn(addonId)**: *Function*. Will send an Execute Addon command sending no parameters. Addon IDs are embedded in the addon configuration and are not to be confused with the Addon Name. For example: http://forums.homeseer.com/showthread.php?p=1213403.
 - **kodiPause()**: *Function*. Will send a Pause command, only effective if the device is streaming.
 - **kodiPlay()**: *Function*. Will send a Play command, only effective if the device was streaming and has been paused.
 - **kodiPlayFavorites([position])**: *Function*. Will play an item from the Kodi's Favorites list optionally starting at *position*. Favorite positions start from 0 which is the default.
 - **kodiPlayPlaylist(name, [position])**: *Function*. Will play a music or video Smart Playlist with *name* optionally starting at *position*. Playlist positions start from 0 which is the default.
 - **kodiSetVolume(level)**: *Function*. Set the volume for a Kodi device, 0 <= level <= 100.
 - **kodiStop()**: *Function*. Will send a Stop command, only effective if the device is streaming.
 - **kodiSwitchOff()**: *Function*. Will turn the device off if this is supported in settings on the device.

#### kWh, Electricity (instant and counter)
 - **actualWatt**: *Number*. Actual usage in Watt.
 - **counterToday**: *Number*.
 - **updateElectricity(power, energy)**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **usage**: *Number*.
 - **WhToday**: *Number*. Total Wh usage of the day. Note the unit is Wh and not kWh!
 - **WhTotal**: *Number*. Total Wh usage.
 - **WhActual**: *Number*. Actual reading in Watt. Please use actualWatt

#### Leaf wetness
 - **wetness**: *Number*. Wetness value
 - **updateWetness(wetness)**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Logitech Media Server
 - **pause()**: *Function*.  Will pause the device if it was streaming. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **play()**: *Function*.  If the device was off, it will turn on and start streaming. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **playFavorites([position])**: *Function*.	Will start the favorites playlist. `position` is optional (0 = default). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **playlistID**: *Nubmer*.
 - **setVolume(level)**: *Function*.  Sets the volume (0 <= level <= 100). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **startPlaylist(name)**: *Function*.  Will start the playlist by its `name`. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **stop()**: *Function*.  Will stop the device (only effective if the device is streaming). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **volumeUp()**: *Function*. Will turn the device volume up with 2 points. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **volumeDown()**: *Function*. Will turn the device volume down with 2 points. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Lux sensor
 - **lux**: *Number*. Lux level for light sensors.
 - **updateLux(lux)**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Onkyo receiver
 - **onkyoEISCPCommand(command)**. *Function*.  Sends an EISP command to the Onkyo receiver.

#### OpenTherm gateway
 - **setPoint**: *Number*.
 - **updateSetPoint(setPoint)**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### P1 Smart meter
 - **counterDeliveredToday**: *Number*.
 - **counterToday**: *Number*.
 - **usage1**, **usage2**: *Number*.
 - **return1**, **return2**: *Number*.
 - **updateP1(usage1, usage2, return1, return2, cons, prod)**: *Function*. Updates the device. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **usage**: *Number*.
 - **usageDelivered**: *Number*.

#### Percentage
 - **percentage**: *Number*.
 - **updatePercentage(percentage)**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Philips Hue Light
See switch below.

#### Pressure
 - **pressure**: *Number*.
 - **updatePressure(pressure)**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Rain meter
 - **rain**: *Number* (please note that this does return the rain total for today)
 - **rainRate**: *Number*
 - **updateRain(rate, counter)**: *Function*. (rate in mm * 100 per hour, counter is total in mm) Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### RGBW(W) / Lighting Limitless/Applamp
 - **decreaseBrightness()**: deprecated because only very limited supported and will be removed from API
 - **getColor()**; *Function*. Returns table with color attributes or nil when color field of device is not set.
 - **increaseBrightness()**: deprecated because only very limited supported and will be removed from API
 - **setColor(r, g, b, br, cw, ww, m, t)**: *Function*. Sets the light to requested color.  r, g, b required, others optional. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29). Meaning and limits of parms can be found [here](https://www.domoticz.com/wiki/Domoticz_API/JSON_URL's#Set_a_light_to_a_certain_color_or_color_temperature).
 - **setColorBrightness()**: same as setColor
 - **setDiscoMode(modeNum)**: deprecated because only very limited supported and will be removed from API
- **setHex(r, g, b)**: *Function*. Sets the light to requested color.  r, g, b required (decimal Values 0-255). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
- **setHue(hue, brightness, isWhite)**: *Function*. Sets the light to requested Hue. Hue and brightness required. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **setKelvin(Kelvin)**: *Function*.  Sets Kelvin level of the light (For RGBWW devices only). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **setNightMode()**: *Function*.  Sets the lamp to night mode. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **setRGB(red, green, blue)**: *Function*.  Set the lamps RGB color. Values are from 0-255. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **setWhiteMode()**: *Function*.  Sets the lamp to white mode. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Scale weight
 - **weight**: *Number*
 - **udateWeight()**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Scene
 - **devices()**: *Function*. Returns the collection of associated devices. Supports the same iterators as for `domoticz.devices()`: `forEach()`, `filter()`, `find()`, `reduce()`. See [Looping through the collections: iterators](#Looping_through_the_collections:_iterators). Note that the function doesn't allow you to get a device by its name or id. Use `domoticz.devices()` for that.
 - **protectionOff()**: *Function*. switch protection to off. Supports some [command options]
 - **protectionOn()**: *Function*. switch protection to on. Supports some [command options]
 - **rename(newName)**: *Function*. Change current scene-name to new scene-name Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **switchOn()**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **switchOff()**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Security Panel
Create a security device:

 1. Set a security panel password in Domoticz settings.
 2. Activate the security panel (**Setup > More Options > Security Panel** and set it to **Disarm** (using your password)).
 3. Go to the device list in (**Setup > Devices**) where it should show the security panel device under 'Not used'.
 4. Add the device and give it a name.

Methods/attributes:

 - **armAway()**: Sets a security device to Armed Away. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **armHome()**: Sets a security device to Armed Home. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **disarm()**: Disarms a security device. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **state**: *String*: Same values as domoticz.security value.

#### Smoke detection
 - **activate()**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **reset()**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Solar radiation
 - **radiation**. *Number*. In Watt/m<sup>2</sup>.
 - **updateRadiation(radiation)**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Soil moisture
 - **moisture**. *Number*. In centibars (cB).
 - **updateSoilMoisture(moisture)**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Sound level
 - **level**. *Number*. In dB.
 - **updateSoundLevel(level)**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Switch (dimmer, selector etc.)
There are many switch-like devices. Not all methods are applicable for all switch-like devices. You can always use the generic `setState(newState)` method.

 - **close()**: *Function*. Set device to Close if it supports it. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **dimTo(percentage)**: *Function*. Switch a dimming device on and/or dim to the specified level. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **lastLevel**: Number. The level of a dimmer just before it was switched off. **Note: lastLevel only shows you the previous level if you have a trigger for the device itself. So for instance you have a on-trigger for 'myDimmer' and the dimmer is set from 20% to 30%, in your script `myDimmer.level == 30` and `myDimmer.lastLevel == 20`. **
 - **level**: *Number*. For dimmers and other 'Set Level..%' devices this holds the level like selector switches.
 - **levelActions**: *String*. |-separated list of url actions for selector switches.
 - **levelName**: *String*. Current level name for selector switches.
 - **levelNames**: *Table*. Table holding the level names for selector switch devices.
 - **maxDimLevel**: *Number*.
 - **open()**: *Function*. Set device to Open if it supports it. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **quietOn()**: *Function*. Set deviceStatus to on without physically switching it. Subsequent Events are triggered. Supports some [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **quietOff()**: *Function*. set deviceStatus to off without physically switching it. Subsequent Events are triggered. Supports some [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **setLevel(percentage)**: *Function*. Set device to a given level if it supports it (e.g. blinds percentage). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **stop()**: *Function*. Set device to Stop if it supports it (e.g. blinds). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **switchOff()**: *Function*. Switch device off it is supports it. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **switchOn()**: *Function*. Switch device on if it supports it. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **switchSelector(<[level]|[levelname] >)** : *Function*. Switches a selector switch to a specific level ( levelname or level(numeric) required ) levelname must be exact, for level the closest fit will be picked. See the edit page in Domoticz for such a switch to get a list of the values). Levelname is only supported when level 0 ("Off") is not removed Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **toggleSwitch()**: *Function*. Toggles the state of the switch (if it is togglable) like On/Off, Open/Close etc.

#### Temperature sensor
 - **temperature**: *Number*
 - **updateTemperature(temperature)**: *Function*. Note: temperature must be in Celsius. Use `domoticz.toCelsius()` to convert a Fahrenheit temperature to Celsius. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Temperature, Barometer sensor
 - **barometer**: *Number*
 - **forecast**: *Number*.
 - **forecastString**: *String*.
 - **temperature**: *Number*
 - **updateTempBaro(temperature, pressure, forecast)**: *Function*. Forecast can be domoticz.BARO_STABLE, BARO_SUNNY, BARO_CLOUDY, BARO_UNSTABLE, BARO_THUNDERSTORM. Note: temperature must be in Celsius. Use `domoticz.toCelsius()` to convert a Fahrenheit temperature to Celsius. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Temperature, Humidity, Barometer sensor
 - **barometer**: *Number*
 - **dewPoint**: *Number*
 - **forecast**: *Number*.
 - **forecastString**: *String*.
 - **humidity**: *Number*
 - **humidityStatus**: *String*
 - **humidityStatusValue**: *Number*. Value matches with domoticz.HUM_NORMAL, -HUM_DRY, HUM_COMFORTABLE, -HUM_WET.
 - **temperature**: *Number*
 - **updateTempHumBaro(temperature, humidity, status, pressure, forecast)**: *Function*. forecast can be domoticz.BARO_NOINFO, BARO_SUNNY, BARO_PARTLY_CLOUDY, BARO_CLOUDY, BARO_RAIN. status can be domoticz.HUM_NORMAL, HUM_COMFORTABLE, HUM_DRY, HUM_WET, HUM_COMPUTE (let dzVents do the math)<sup>3.0.15</sup>. Note: temperature must be in Celsius. Use `domoticz.toCelsius()` to convert a Fahrenheit temperature to Celsius. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Temperature, Humidity
 - **dewPoint**: *Number*
 - **humidity**: *Number*
 - **humidityStatus**: *String*
 - **humidityStatusValue**: *Number*. Value matches with domoticz.HUM_NORMAL, -HUM_DRY, HUM_COMFORTABLE, -HUM_WET.
 - **temperature**: *Number*
 - **updateTempHum(temperature, humidity [, status] )**: *Function*. status can be domoticz.HUM_NORMAL, HUM_COMFORTABLE, HUM_DRY, HUM_WET or HUM_COMPUTE (let dzVents do the math)<sup>3.0.15</sup>. Note: temperature must be in Celsius. Use `domoticz.toCelsius()` to convert a Fahrenheit temperature to Celsius. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Text
 - **text**: *String*
 - **updateText(text)**: Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Thermostat set point
 - **setPoint**: *Number*.
 - **updateSetPoint(setPoint)**:*Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Thermostat Operating state <sup>3.0.19</sup>
 - **mode**: *Number*. Current mode
 - **modes**: *String*. List of all modes
 - **modeString**: *String*. Current mode
 - **updateMode(mode)**:*Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Thermostat type 3 (Mertik) <sup>3.0.15</sup>
 - **mode**: *Number*. Current mode
 - **modes**: *String*. List of all modes
 - **modeString**: *String*. Current mode
 - **updateMode(mode)**:*Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### UV sensor
 - **uv**: *Number*. UV index.
 - **updateUV(uv)**: *Function*. UV index. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Visibility
 - **visibility**: *Number*. In km.
 - **updateVisibility(distance)**:Function*. In km. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Voltage
 - **voltage**: *Number*.
 - **updateVoltage(voltage)**:Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Waterflow
 - **flow**: *Number*. In L/m.
 - **updateWaterflow(flow)**:Function*. In L/m. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Wind
 - **chill**: *Number*.
 - **direction**: *Number*. Degrees.
 - **directionString**: *String*. Formatted wind direction like N, SE.
 - **gust**: *Number*. ( in meters / second, might change in future releases to Meters/Counters settings for Wind Meter )
 - **gustMs**: *Number*. Gust ( in meters / second )
 - **temperature**: *Number*
 - **speed**: *Number*. Windspeed ( in the unit set in Meters/Counters settings for Wind Meter )
 - **speedMs**: *Number*. Windspeed ( in meters / second )
 - **updateWind(bearing, direction, speed, gust, temperature, chill)**: *Function*. Bearing in degrees, direction in N, S, NNW etc, speed in m/s, gust in m/s, temperature and chill in Celsius. Use `domoticz.toCelsius()` to convert a Fahrenheit temperature to Celsius. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Youless meter
 - **counterDeliveredToday**: *Number*.
 - **counterDeliveredTotal**: *Number*.
 - **powerYield**: *String*.
 - **updateYouless(total, actual)**: *Function*.

#### Zone heating
 - **setPoint**: *Number*.
 - **heatingMode**: *String*.

#### Z-Wave Fan mode
 - **mode**: *Number*. Current mode number.
 - **modeString**: *String*. Mode name.
 - **modes**: *Table*. Lists all available modes.
 - **updateMode(modeString)**: *Function*. Set new mode. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Z-Wave Thermostat mode
 - **mode**: *Number*. Current mode number.
 - **modeString**: *String*. Mode name.
 - **modes**: *Table*. Lists all available modes.
 - **updateMode(modeString)**: *Function*. Set new mode. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

### Command options (delay, duration, event triggering)
Many dzVents device methods support extra options, like controlling a delay or a duration:
```Lua

	-- switch on for 2 minutes after 10 seconds
	device.switchOn().afterSec(10).forMin(2)

	-- switch on at a specic time / day
	device.switchOn().at('09:00')					-- earliest moment it will be 09:00 hr.
	device.switchOn().at('08:53:30 on fri')			-- earliest moment it will be Friday at 08:53:30
	device.switchOn().at('08:53:30 on sat, sun')	-- earliest moment it will be Saturday or Sunday at 08:53:30 (whatever comes first)

	-- switch on for 2 minutes after a randomized delay of 1-10 minutes
	device.switchOff().withinMin(10).forMin(2)
	device.close().forMin(15)
	device.open().afterSec(20)
	device.open().afterMin(2)

	-- switch on but do not trigger follow up events
	device.switchOn().silent()

	-- flash a light for 3 times
	device.switchOn().forSec(2).repeatAfterSec(1, 3)

	-- switch the device on but only if the current state isn't already on:
	device.switchOn().checkFirst()
	-- this is a short for:
	if (device.state == 'Off') then
		devices.switchOn()
	end
```

#### Options
 - **at(hh:mm[:ss][ on [ ddd|dddd ] )**: *Function*.<sup>3.0.1</sup> Activates the command at a certain time [ on a certain day]
 - **afterHour(hours), afterMin(minutes), afterSec(seconds)**: *Function*. Activates the command after a certain number of hours, minutes or seconds.
 - **cancelQueuedCommands()**: *Function*.  Cancels queued commands. E.g. you switch on a device after 10 minutes:  `myDevice.switchOn().afterMin(10)`. Within those 10 minutes you can cancel that command by calling:  `myDevice.cancelQueuedCommands()`.
 - **checkFirst()**: *Function*. Checks if the **current** state of the device is different than the desired new state. If the target state is the same, no command is sent. If you do `mySwitch.switchOn().checkFirst()`, then no switch command is sent if the switch is already on. This command only works with switch-like devices. It is not available for toggle and dim commands, either.
 - **forHour(hours), forMin(minutes), forSec(seconds)**: *Function*. Activates the command for the duration of hours, minutes or seconds. See table below for applicability and the warning on unexpected behavior of these functions.
 - **withinHour(hours), withinMin(minutes), withinSec(seconds)**: *Function*. Activates the command within a certain period (specified in hours, minutes or seconds) *randomly*. See table below for applicability.
 - **silent()**: *Function*. No follow-up events will be triggered: `mySwitch.switchOff().silent()`.
 - **repeatAfterHour(hours, [number]), repeatAfterMin(minutes, [number]), repeatAfterSec(seconds, [number])**: *Function*. Repeats the sequence *number* times after the specified duration (specified in hours, minutes, or seconds).  If no *number* is provided, 1 is used. **Note that `afterAAA()` and `withinAAA()` are only applied at the beginning of the sequence and not between the repeats!**

Note that the actual switching or changing of the device is done by Domoticz and not by dzVents. dzVents only tells Domoticz what to do; if the options are not carried out as expected, this is likely a Domoticz or hardware issue.

**Important note when using forAAA()**: Let's say you have a light that is triggered by a motion detector.  Currently the light is `Off` and you do this: `light.switchOn().forMin(5)`. What happens inside Domoticz is this:  at t<sub>0</sub> Domoticz issues the `switchOn()` command and schedules a command to restore the **current** state at t<sub>5</sub> which is `Off`. So at t<sub>5</sub> it will switch the light off.

If, however, *before the scheduled `switchOff()` happens at t<sub>5</sub>*, new motion is detected and you send this command again at t<sub>2</sub> then something unpredictable may seem to happen: *the light is never turned off!* This is what happens:

At t<sub>2</sub> Domoticz receives the `switchOn().forMin(5)` command again. It sees a scheduled command at t<sub>5</sub> and deletes that command (it is within the new interval). Then Domoticz performs the (unnecessary, it's already on) `switchOn()` command. Then it checks the current state of the light which is `On`!! and schedules a command to return to that state at t<sub>2+5</sub>=t<sub>7</sub>. So, at t<sub>7</sub> the light is switched on again. And there you have it: the light is not switched off and never will be because future commands will always be checked against the current on-state.

That's just how it works and you will have to deal with it in your script. So, instead of simply re-issuing `switchOn().forMin(5)` you have to check the switch's state first:
```Lua
if (light.active) then
	light.switchOff().afterMin(5)
else
	light.switchOn().forMin(5)
end
```
or issue these two commands *both* as they are mutually exclusive:
```Lua
light.switchOff().checkFirst().afterMin(5)
light.switchOn().checkFirst().forMin(5)
```

#### Availability
Some options are not available to all commands. All the options are available to device switch-like commands like `myDevice.switchOff()`, `myGroup.switchOn()` or `myBlinds.open()`.  For updating (usually Dummy ) devices like a text device `myTextDevice.updateText('zork')` you can only use `silent()`. For thermostat setpoint devices and snapshot command silent() is not available.  For commands for which dzVents must use openURL, only  `at()` and  `afterAAA()` methods are available. These commands are mainly the setAaaaa() commands for RGBW type devices.

See table below

```{=mediawiki}

{| class="wikitable"
!width="17%"| option
!align="center" width="12%"| state changes
!align="center" width="12%"| update commands
!align="center" width="12%"| user variables
!align="center" width="12%"| updateSetpoint
!align="center" width="12%"| snapshot
!align="center" width="12%"| triggerIFTTT
!align="center" width="12%"| emitEvent
|-
| <code>afterAAA()</code><sup>1</sup>
|align="center"| •
|align="center"| •
|align="center"| •
|align="center"| •
|align="center"| •
|align="center"| •
|align="center"| •
|-
| <code>at()</code>
|align="center"| •
|align="center"| •
|align="center"| •
|align="center"| •
|align="center"| •
|align="center"| •
|align="center"| •
|-
| <code>forAAA()</code>
|align="center"| •
|align="center"| n/a
|align="center"| n/a
|align="center"| n/a
|align="center"| n/a
|align="center"| n/a
|align="center"| n/a
|-
| <code>withinAAA()</code>
|align="center"| •
|align="center"| •
|align="center"| •
|align="center"| •
|align="center"| •
|align="center"| n/a
|align="center"| •
|-
| <code>silent()</code>
|align="center"| •
|align="center"| •
|align="center"| •
|align="center"| n/a
|align="center"| n/a
|align="center"| n/a
|align="center"| n/a
|-
| <code>repeatAfterAAA()</code>
|align="center"| •
|align="center"| n/a
|align="center"| n/a
|align="center"| n/a
|align="center"| n/a
|align="center"| n/a
|align="center"| n/a
|-
| <code>checkFirst()</code>
|align="center"| • <sup>2</sup>
|align="center"| n/a
|align="center"| n/a
|align="center"| n/a
|align="center"| n/a
|align="center"| n/a
|align="center"| n/a
|-
| <code>cancelQueuedCommands()</code>
|align="center"| •
|align="center"| •
|align="center"| •
|align="center"| n/a
|align="center"| n/a
|align="center"| n/a
|align="center"| n/a

|}
```

#### Notes on table
 - **Note 1**: AAA is a placeholder for `Min/Sec/Hour` affix e.g. `afterMin()`.
 - **Note 2**: for `domoticz.openURL()` only `at()`, `afterAAA()` and `withinAAA()` is available.
 - **Note 3**: Note 2 also applies for all commands depending on openURL (like rgbwwDevice.setAaa() commands).
 - **Note 4**: Including dimTo, switchSelector, setLevel and similar methods.

#### Follow-up event triggers
Normally if you issue a command, Domoticz will immediately trigger follow-up events, and dzVents will automatically trigger defined event scripts. If you trigger a scene, all devices in that scene will issue a change event. If you have event triggers for these devices, they will be executed by dzVents. If you don't want this to happen, add `.silent()` to your commands (exception is updateSetPoint).

### Create your own device adapter
If your device is not recognized by dzVents, you can still operate it using the generic device attributes and methods. It is much nicer, however, to have device specific attributes and methods. Existing recognized adapters are in `/path/to/domoticz/dzVents/runtime/device-adapters`.  Copy an existing adapter and adapt it to your needs. You can turn debug logging on and inspect the file `domoticzData.lua` in the dzVents folder. There you will find the unique signature for your device type. Usually it is a combination of deviceType and deviceSubType, but you can use any of the device attributes in the `matches` function. The matches function checks if the device type is suitable for your adapter and the `process` function actually creates your specific attributes and methods.
Also, if you call `myDevice.dump()` you will see all attributes and methods and the attribute `_adapters` shows you a list of applied adapters for that device.
Finally, register your adapter in `Adapters.lua`. Please share your adapter when it is ready and working.

## Variable object API (user variables)
User variables created in Domoticz have these attributes and methods:

### Variable attributes and methods
 - **changed**: *Boolean*. Was the variable updated. **Note**: update does not necessarily means the variable value or type changed.
 - **date**: *Date*. If type is domoticz.DATE. See lastUpdate for the sub-attributes.
 - **id**: *Number*. Index of the variable.
 - **lastUpdate**: *[Time Object](#Time_object)*
 - **name**: *String*. Name of the variable
 - **rename(newName)**: *Function*. Change current variable name to new variable name. (does not support timing options)
 - **set(value)**: Tells Domoticz to update the variable. Supports timing options. See [above](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **time**: *Date*. If type is domoticz.TIME. See lastUpdate for the sub-attributes.
 - **type**: *String*. Can be domoticz.INTEGER, domoticz.FLOAT, domoticz.STRING, domoticz.DATE, domoticz.TIME.
 - **value**: *String|Number|Date|Time*. Value of the variable.

## Time object
Many attributes represent a moment in time, like `myDevice.lastUpdate`  or `domoticz.time`. In dzVents, a time-like attribute is an object with properties and methods which make your life easier.

```Lua
	print(myDevice.lastUpdate.minutesAgo)
	print(myDevice.lastUpdate.daysAgo)

	-- compare two times
	print(domoticz.time.compare(myDevice.lastUpdate).secs))
```

You can also create your own:
```Lua
	local Time = require('Time')
	local t = Time('2016-12-12 07:35:00') -- must be this format!!
```
If you don't pass a time string:
```Lua
	local Time = require('Time')
	local currentTime = Time()
```

Use this in combination with the various dzVents time attributes:

```Lua
	local Time = require('Time')
	local t = Time('2016-12-12 07:35:00')

	local tonight = Time(domoticz.time.rawDate .. ' 20:00:00')
	print (tonight.getISO())
	-- will print something like: 2016-12-12T20:00:00Z
	print(t.minutesAgo) -- difference from 'now' in minutes

	-- and you can feed it with all same rules as you use
	-- for the timer = { .. } section:
	if (t.matchesRule('at 16:00-21:00')) then
		-- t is in between 16:00 and 21:00
	end

	-- very powerful if you want to compare two time instances:
	local anotherTime = Time('...') -- fill-in some time here
	print(t.compare(anotherTime).secs) -- diff in seconds between t and anotherTime.
```

### System event API
If you have a dzVents script that is triggered by a system-event in Domoticz then the second parameter passed to the execute function will be a *system* object. The object.type = system-event that triggered the script. ( object.isSystemEvent: true) For backups the object also contains location and durationin seconds.

### Time properties and methods

Creation:
```Lua
local Time = require('Time')
local now = Time() -- current time
local someTime = Time('2017-12-31 22:19:15')
local utcTime = Time('2017-12-31 22:19:15', true)
```

Creation:
```Lua
local someTime = domoticz.time.addMinutes(30) -- someTime = domoticz time object for now + 30 minutes
local someTime = domoticz.time.makeTime() -- someTime = new domoticz time object.
```

 - **addAAA(offset)**: *time object*. time object with given offset (positive or negative. AAA is a placeholder for `Seconds, Minutes, Hours or Days` affix e.g. `addHours(-2)
 - **civTwilightEndInMinutes**: *Number*. Minutes from midnight until civTwilightEnd.
 - **civTwilightStartInMinutes**:*Number*. Minutes from midnight until civTwilightStart.
 - **compare(time)**: *Function*. Compares the current time object with another time object. *Make sure you pass a Time object!* Returns a table (all values are *positive*, use the compare property to see if *time* is in the past or future):
	- **milliseconds**: Total difference in milliseconds.
	- **seconds**: Total difference in whole seconds.
	- **minutes**: Total difference in whole minutes.
	- **hours**: Total difference in whole hours.
	- **days**: Total difference in whole days.
	- **compare**: 0 = both are equal, 1 = *time* is in the future, -1 = *time* is in the past.
 - **dateToDate( dateString, sourceFormat, targetFormat[ , offSet] )**:<sup>3.0.17</sup> *Function*: Returns a reformatted datestring. dateString is a formatted date and source- and targetFormat are representations of the dateString elements and their locations in the string (see examples in dateToTimestamp) . Offset is a number (default = 0 )
```Lua
		-- examples:
		domoticz.time.dateToDate('31/12/2021 23:31:05','dd/mm/yyyy hh:MM:ss', 'dddd dd mmmm hh:MM:ss',  1 )				-- > 'Friday 31 December 23:31:06'
		domoticz.time.dateToDate('31/12/2021 23:31:05','dd/mm/yyyy hh:MM:ss', 'ddd dd mmm hh:MM:ss',  -3600 * 24 - 3 ))	-- > 'Thu 30 Dec 23:31:02'
```

 - **dateToTimestamp([ dateString [,control] ] )**: *Function*:  Returns *Number* representing time since epoch in seconds. dateString is a formatted date and control is a representation of the dateString elements and their location in the string. Default control is 'yyyy-mm-dd hh:MM'.
	recognized controls are either a Lua pattern string or a string containing following codes:
```Lua
		dddd -- full weekdayname(e.g. Wednesday) language depends on locale
		ddd  -- abbreviated weekdayname(e.g. Wed) language depends on locale
		dd   -- day of the month {[01-31]
		mmmm -- full monthname(e.g September) language depends on locale
		mmm  -- abbreviated monthname(e.g. Sep) language depends on locale
		mm   -- month [01-12]
		yyyy -- 4 digit year
		yy   -- 2 digit year
		hh   -- hour 24-hour clock
		ii   -- hour 12-hour clock
		MM   -- minute
		ss   -- second
		W    -- weeknumber [01-53]
		w    -- weekday{[0-6] Sunday-Saturday
		datm -- date and time (e.g. 09/16/98 23:48:10) format depends on locale
		mer  -- either "AM" or "PM" locale
		date -- date(e.g. 09/16/98) format depends on locale
		time -- time (e.g. 23:48:10)

		-- examples:
		domoticz.time.dateToTimestamp('2020-12-31 23:59'))                                                      -- > 1609455540
		domoticz.time.dateToTimestamp('2020-12-31 23:59:01','yyyy-mm-dd hh:MM:ss'))                             -- > 1609455541
		domoticz.time.dateToTimestamp('December 31, 23:59:02','mmmm dd, hh:MM:ss'))                             -- > 1609455542
		domoticz.time.dateToTimestamp('2020-12-31 23:59:03','(%d+)%D+(%d+)%D+(%d+)%D+(%d+)%D+(%d+)%D+(%d+)' ))  -- > 1609455543
		domoticz.time.dateToTimestamp('20201231235904','(%d%d%d%d)(%d%d)(%d%d)(%d%d)(%d%d)(%d%d)'))	            -- > 1609455544
```

 - **day**: *Number*
 - **day**: *Number*
 - **dayAbbrOfWeek**: *String*. sun,mon,tue,wed,thu,fri or sat
 - **daysAgo**: *Number*
 - **dayName**: *String*. complete name Sunday, Monday, etc
 - **dDate**: *Number*. timestamp (seconds since 01/01/1970 00:00)
 - **getISO**: *Function*. Returns the ISO 8601 formatted date.
 - **hour**: *Number*
 - **hoursAgo**: *Number*. Number of hours since the last update.
 - **isdst**: *Boolean*. <sup>3.0.18</sup> Indicates if time is in the daylight saving period
 - **isToday**: *Boolean*. Indicates if time is today
 - **isUTC**: *Boolean*.
 - **makeTime(string | table,[isUTC])**: *domoticz time object*. returns domoticz time object based on first parameter (time as table or string) string format must be 'yyyy-mm-dd hh:mm:ss'. isUTC defaults to false.
 - **matchesRule(rule) **: *Function*. Returns true if the rule matches with the time. See [time trigger rules](#timer_trigger_rules) for rule examples.
 - **millisecondsAgo**: *Number*. Number of milliseconds since the last update.
 - **minutes**: *Number*
 - **minutesAgo**: *Number*. Number of minutes since the last update.
 - **minutesSinceMidnight**: *Number*
 - **month**: *Number*
 - **monthAbbrName**: String. jan, feb, mar, etc
 - **monthName**: *String*.  January, February, etc
 - **msAgo**: *Number*. Number of milliseconds since the last update.
 - **raw**: *String*. Generated by Domoticz
 - **rawDate**: *String*. Returns the date part of the raw data.
 - **rawDateTime**: *String*. Combined date / time formatted as domoticz does in API / JSON returns (rawDate .. ' '.. rawTime )
 - **rawTime**: *String*. Returns the time part of the raw data as HH:MM:SS
 - **seconds**: *Number*
 - **secondsSinceMidnight**: *Number*
 - **secondsAgo**: *Number*. Number of seconds since the last update.
 - **sunsetInMinutes**: *Number*. Minutes from midnight until sunset.
 - **sunriseInMinutes**: *Number*. Minutes from midnight until sunrise.
 - **solarnoonInMinutes**: *Number*. <sup>3.0.11</sup> Minutes from midnight until solarnoon.
 - **time**: *String*. Returns the time part of the raw data as HH:MM
 - **timestampToDate([ [ timestamp ] , [ pattern ] ,[ offSet ]  )**: *function*. Returns a datestring. timestamp is number of seconds since epoch (default = now) , pattern is a string as explained in this wiki at the description of the domoticz.time.dateToTimestamp function (default = 'yyyy-mm-dd hh:MM:ss'). Offset is a number (default = 0 )
```Lua
		-- examples:
		domoticz.time.timestampToDate(10,'date time')                               --> '01/01/70 01:00:10'
		domoticz.time.timestampToDate(60,'ddd mm mmm yyyy ii:MM mer','date time')   --> 'Thu 01 Jan 1970 01:01 AM''
		domoticz.time.timestampToDate(1598077445,'dd/mm/yy ii:MM mer nZero')        --> '22/08/20 8:24 AM'
```

 - **toUTC(string | table,[offset])**: *domoticz time object*. <sup>3.0.9</sup> returns domoticz time object based on first parameter (time as table or string) string format must be 'yyyy-mm-dd hh:mm:ss'. offset defaults to 0.
 - **utcSystemTime**: *Table*. UTC system time (only when in UTC mode):
	- **day**: *Number*
	- **hour**: *Number*
	- **month**: *Number*
	- **minutes**: *Number*
	- **seconds**: *Number*
	- **year**: *Number*
 - **utcTime**: *Table*. Time stamp in UTC time:
	- **day**: *Number*
	- **hour**: *Number*
	- **month**: *Number*
	- **minutes**: *Number*
	- **seconds**: *Number*
	- **year**: *Number*
 - **year**: *Number*
 - **wday**: *Number* day of the week. Sunday = 1, Saturday = 7
 - **week**: *Number* week of the year

**Note: it is currently not possible to change the domoticz.time object instance.** ( but you can create a new one with- or without an offset)

## Shared helper functions
It is not unlikely that at some point you want to share Lua code among your scripts. Normally in Lua you would have to create a module and require that module in all you scripts. But dzVents makes that easier for you:
Inside your scripts folder or in Domoticz' GUI web editor, create a `global_data.lua` script (same as for global persistent data) and feed it with this code:
```Lua
return {
	helpers = {
		myHandyFunction = function(param1, param2)
			-- do your stuff
		end,
		MY_CONSTANT = 100 -- doesn't have to be a function
	}
}
```
Save the file and then you can use myHandyFunction everywhere in your event scripts:
```Lua
return {
	...
	execute = function(domoticz, device)
		local results = domoticz.helpers.myHandyFunction('bla', 'boo')
		print(domoticz.helpers.MY_CONSTANT)
	end
}
```
No `require` or `dofile` is needed.

**Important note: if you need to access the domoticz object in your helper function you have to pass it as a parameter:**

Example:
```Lua
return {
	helpers = {
		myHandyFunction = function(domoticz, param1, param2)
			-- do your stuff
			domoticz.log('Hey')
		end
	}
}
```
And pass it along:
```Lua
return {
	...
	execute = function(domoticz, device)
		local results = domoticz.helpers.myHandyFunction(domoticz, 'bla', 'boo')
	end
}
```
**Note**: there can be only **one** `global_data.lua` on your system. Either in `/path/to/domoticz/scripts/dzVents/script` or in Domoticz' internal GUI web editor.

## Requiring your own modules
If you don't want to use the helpers support, but you want to require your own modules, place them either in `/path/to/domoticz/scripts/dzVents/modules` or `/path/to/domoticz/scripts/dzVents/scripts/modules` as these folders are already added to the Lua package path. You can just require those modules: `local myModule = require('myModule')`

# Persistent data

In many situations you need to store a device state or other information in your scripts for later use. Like knowing what the state was of a device the previous time the script was executed or what the temperature in a room was 10 minutes ago. Without dzVents, you had to resort to user variables. These are global variables that you create in the Domoticz GUI and that you can access in your scripts like: domoticz.variables('previousTemperature').

Now, for some this is rather inconvenient and they want to control this state information in the event scripts themselves or want to use all of Lua's variable types. dzVents has a solution for that: **persistent script data**. This can either be on the script level or on a global level.

## Script level persistent variables

The values in persistent script variables persist and can be retrieved in the next script run.

For example, send a notification if a switch has been activated 5 times:
```Lua
	return {
		on = {
			devices = { 'MySwitch' }
		},
		data = {
			counter = { initial = 0 }
		},
		execute = function(domoticz, switch)
			if (domoticz.data.counter == 5) then
			domoticz.notify('The switch was pressed 5 times!')
			domoticz.data.counter = 0 -- reset the counter
		else
			domoticz.data.counter = domoticz.data.counter + 1
		end
	end
	}
```
The `data` section defines a persistent variable called `counter`. It also defines an initial value.  From then on you can read and set the variable in your script.

You do not have to provide an initial value though. In that case the initial value is *nil*:

```Lua
	return {
		--
		data = {
			'x', 'y', 'z' -- note the quotes
		},
		execute = function(domoticz, item)
			print(tostring(domoticz.data.x)) -- prints nil
		end
	}
```

You can define as many variables as you like and put whatever value in there that you like. It doesn't have to be just a number. Just don't put something in there with functions (well, they are filtered out actually).

### Size matters and watch your speed!!
If you include tables (or arrays) in the persistent data, beware to not let them grow. It will definitely slow down script execution because dzVents has to serialize and deserialize the data. It is better to use the historical option as described below.

### Re-initializing a variable
You can re-initialize a persistent variable and re-apply the initial value as defined in the data section:

```Lua
return {
	on = { .. },
	data = {
		x = { initial = 'initial value' }
	},
	execute = function(domoticz, item)
		if (domoticz.data.x ~= 'initial value') then
			domoticz.data.initialize('x')
			print(domoticz.data.x) -- will print 'initial value'
		end
	end
}
```
Note that `domoticz.data.initialize('<varname>')` is just a convenience method. You can of course create a local variable in your module holding the initial value and use it in your data section and your execute function.

## Global persistent variables

Script level variables are only available in the scripts that define them, but global variables can be accessed and changed in every script. To utilize global persistent variables, create a script file called `global_data.lua` in your scripts folder with this content (same file where you can also put your shared helper functions):

```Lua
	return {
		helpers = {},
		data = {
			peopleAtHome = { initial = false },
			heatingProgramActive = { initial = false }
		}
	}
```

Just define the variables that you need and access them in your scripts:
```Lua
	return {
		on = {
			devices = {'WindowSensor'}
		},
		execute = function(domoticz, windowSensor)
			if (domoticz.globalData.heatingProgramActive
				and windowSensor.state == 'Open') then
				domoticz.notify("Hey don't open the window when the heating is on!")
			end
		end
	}
```
**Note**: there can be only one `global_data.lua` on your system. Either in `/path/to/domoticz/scripts/dzVents/script` or in Domoticz' internal GUI web editor.

You can use `domoticz.globalData.initialize('<varname>')` just as like `domoticz.data.initialize('<varname>')`.

## A special kind of persistent variables: *history = true* (Historical variables API)

In some situations, storing a previous value for a sensor is not enough, and you would like to have more previous values. For example, you want to calculate an average over several readings. Of course you can define a persistent variable holding a table:

```Lua
	return {
		active = true,
		on = {
			devices = {'MyTempSensor'}
		},
		data = {
			previousData = { initial = {} }
		},
		execute = function(domoticz, sensor)
			-- add new data
			table.insert(domoticz.data.previousData, sensor.temperature)

			-- calculate the average
			local sum = 0, count = 0
			for i, temp in pairs(domoticz.data.previousData) do
				sum = sum + temp
				count = count + 1
			end
			local average = sum / count
		end
	}
```

The problem with this is that you have to do a lot of bookkeeping to make sure that there isn't too much data to store (see [below for how it works](#How_does_the_storage_stuff_work.3F)) . Fortunately, dzVents has done this for you:
```Lua
	return {
		active = true,
		on = {
			devices = {'MyTempSensor'}
		},
		data = {
			temperatures = { history = true, maxItems = 10 }
		},
		execute = function(domoticz, sensor)
			-- add new data
			domoticz.data.temperatures.add(sensor.temperature)

			-- average
			local average = domoticz.data.temperatures.avg()

			-- maximum value in the past hour:
			local max = domoticz.data.temperatures.maxSince('01:00:00')
		end
	}
```

### Defining
Define a script variable or global variable in the data section and set `history = true`:

```Lua
	…
	data = {
		var1 = { history = true, maxItems = 10, maxHours = 1, maxMinutes = 5 }
	}
```

 - **maxItems**: *Number*. Controls how many items are stored in the variable. maxItems has precedence over maxHours and maxMinutes.
 - **maxHours**: *Number*. Data older than `maxHours` from now will be discarded.  E.g., if set to 2, then data older than 2 hours will be removed at the beginning of the script.
 - **maxMinutes**: *Number*. Same as maxHours but, you guessed it: for minutes this time..
 All these options can be combined. **Reminder: don't store too much data. Only put in what you really need!**

### Adding data
Add new values to a historical variable:

	domoticz.data.myVar.add(value)

As soon as this new value is put on top of the list, the older values shift one place down the line. If `maxItems` is reached, then the oldest value will be discarded.  *All methods, like calculating averages or sums, will immediately use this new value!* If you don't want this to happen, set the new value at the end of your script or after you have done your analysis.

Any kind of data may be stored in the historical variable: numbers, strings, but also more complex data like tables. However, in order to use the [statistical methods](#Statistical_functions), numeric values are required.

### Accessing values in historical variables
Values in a historical variable are indexed, where index 1 is the newest value, 2 is the second newest, and so on:

	local item = domoticz.data.myVar.get(5)
	print(item.data)

However, all data in the storage are time-stamped:

	local item = domoticz.data.myVar.getLatest()
	print(item.time.secondsAgo) -- access the time stamp
	print(item.data) -- access the data

The time attribute by itself is a table with many properties that help you inspect the data points more easily. See [Time Object](#Time_object) for all attributes and methods.

#### Index
All data in the set can be addressed using an index. The item with index = 1 is the youngest, and the item with the highest index is the oldest. Beware, if you have called `add()` first, then the first item is that new value! You can always check the size of the set by inspecting `myVar.size`.

#### Time specification (*timeAgo*)
Every data point in the set has a timestamp and the set is ordered so that the youngest item is the first and the oldest item the last. Many functions require a moment in the past to be specified by passing a string in the format **`hh:mm:ss`**, where *hh* is the number of hours ago, *mm* the number of minutes and *ss* the number of seconds. The times are added together, so you don't have to consider 60 minute boundaries, etc. A time specification of `12:88:03` is valid, and points to the data point at or around `12*3600 + 88*60 + 3 = 48.483 seconds` in the past.

Example:

	-- get average for the past 30 minutes:
	local avg = myVar.avgSince('00:30:00')

#### Getting data points

 - **get(idx)**: Returns the idx-th item in the set. Same as `myVar.storage[idx]`.
 - **getAtTime([timeAgo](#Time_specification_.28timeAgo.29))**: Returns the data point *closest* to the moment as specified by `timeAgo` and the index in the set. So, `myVar.getAtTime('1:00:00')` returns the item that is closest to one hour old. It may be a bit younger or a bit older than 1 hour. The second return value is the index in the set: `local item, index = myVar.getAtTime('1:00:00')`
 - **getLatest( )**: Returns the youngest item in the set. Same as `print(myVar.get(1).data)`.
 - **getOldest( )**: Returns the oldest item in the set and the index. Same as `print(myVar.get(myVar.size).data)`. Note that the index is always the same as the total size. `local item, index = myVar.getOldest()`.
 - **size**: Return the number of data points in the set.
 - **subset( [fromIdx], [toIdx] )**: Returns a subset of the stored data. Default value, if omitted, of `fromIdx` is 1. Omitting `toIdx` takes all items until the end of the set (oldest). E.g., `myVar.subset()` returns all data. The result set supports [iterators](#Looping_through_the_data:_iterators) `forEach`, `filter`, `find` and `reduce`.
 - **subsetSince( [timeAgo](#Time_specification_.28timeAgo.29) )**: Returns a subset of the stored data since the relative time specified by timeAgo. So calling `myVar.subsetSince('00:60:00')` returns all items that have been added to the list in the past 60 minutes. The result set supports [iterators](#Looping_through_the_data:_iterators) `forEach`, `filter`, `find` and `reduce`.
 - **reset( )**: Removes all the items from the set. It could be a good practice to do this often when you know you don't need older data. For instance, when you turn on a heater and you just want to monitor rising temperatures starting from this moment when the heater is activated. If you don't need data points from before, then you may call reset.

#### Looping through the data: iterators
Similar to the iterators as described [above](#Looping_through_the_collections:_iterators), there are convenience methods to make looping through the data set easier.

 - **forEach(function)**: Loop over all items in the set: E.g.: `myVar.forEach( function( item, index, collection) ... end )`
 - **filter(function)**: Create a filtered set of items. The function receives the item and returns true if the item is in the result set. E.g. get a set with item values larger than 20: `subset = myVar.filter( function (item) return (item.data > 20) end )`.
 - **find(function)**: Search for a specific item in the set: E.g. find the first item with a value higher than 20: `local item = myVar.find( function (item) return (item.data > 20) end )`.
 - **reduce(function, initial)**: Loop over all items in the set and do some calculation with it. You call reduce with the function and the initial value. Each iteration the function is called with the accumulator. The function does something with the accumulator and returns a new value for it. Once you get the hang of it, it is very powerful. Best to give an example. To sum all values:

```Lua
		local sum = myVar.reduce(function(acc, item)
			local value = item.data
		return acc + value
		end, 0)
```

Suppose you want to get data points older than 45 minutes and count the values that are higher than 20 (of course there are more ways to do this):
```Lua
	local myVar = domoticz.data.myVar

	local olderItems = myVar.filter(function (item)
		return (item.time.minutesAgo > 45)
	end)

	local count = olderItems.reduce(function(acc, item)
		if (item.data > 20) then
			acc = acc + 1
		end
		return acc
	end, 0)

	print('Found ' .. tostring(count) .. ' items')
```

#### Statistical functions
Statistical functions require *numerical* data in the set. If the set is just numbers you can do this:

	myVar.add(myDevice.temperature) -- adds a number to the set
	myVar.avg() -- returns the average

If, however, you add more complex data or you want to do a computation first, then you have to *tell dzVents how to get a numeric value from these data*. So let's say you do this to add data to the set:

	myVar.add( { 'person' = 'John', waterUsage = u })

Where `u` is a variable that got its value earlier. If you want to calculate the average water usage, then dzVents will not be able to do this because it doesn't know the value is actually in the `waterUsage` attribute! You will get `nil`.

To make this work you have to provide a **getValue function** in the data section when you define the historical variable:

```Lua
	return {
		active = true,
		on = {...},
		data = {
		myVar = {
			history = true,
			maxItems = 10,
			getValue = function(item)
				return item.data.waterUsage -- return number!!
			end
		}
		},
		execute = function()...end
	}
```

This function tells dzVents how to get the numeric value for a data item. **Note: the `getValue` function has to return a number!**.

Of course, if you don't intend to use any of these statistical functions you can put whatever you want in the set.

 - **avg( [fromIdx], [toIdx], [default] )**: Calculates the average of all item values within the range `fromIdx` to `toIdx`. If no data are in the set, the value `default` will be returned instead of `0`.
 - **avgSince([timeAgo](#Time_specification_.28timeAgo.29), default )**: Calculates the average of all data points since `timeAgo`. Returns `default` if there are no data, otherwise 0. E.g.: `local avg = myVar.avgSince('00:30:00')` returns the average over the past 30 minutes.
 - **min( [fromIdx], [toIdx] )**: Returns the lowest value in the range defined by fromIdx and toIdx.
 - **minSince([timeAgo](#Time_specification_.28timeAgo.29))**: Same as **min** but now within the `timeAgo` interval.
 - **max( [fromIdx], [toIdx] )**: Returns the highest value in the range defined by fromIdx and toIdx.
 - **maxSince([timeAgo](#Time_specification_.28timeAgo.29))**: Same as **max** but within the `timeAgo` interval.
 - **sum( [fromIdx], [toIdx] )**: Returns the summation of all values in the range defined by fromIdx and toIdx. It will return 0 when there are no data.
 - **sumSince([timeAgo](#Time_specification_.28timeAgo.29))**: Same as **sum** but within the `timeAgo` interval.  It will return 0 when there are no data in the interval.
 - ** delta(fromIdx, toIdx, [smoothRange], [default] )**: Returns the delta (difference) between items specified by `fromIdx` and `toIdx`. Provide a valid range (no `nil` values). [Supports data smoothing]. (#Data_smoothing) when providing a `smoothRange` value. Returns `default` if there is not enough data. The function also returns the fromItem and the toItem that is used to calculate the delta with: `local delta, from, to = delta(2, 5, 3)`.
 - **delta2( fromIdx, toIdx, [smoothRangeFrom], [smoothRangeTo], [default] )**:	Same as **delta** but now you can control if the smooth values for the *from-item* and the *to-item* separately. The function also returns the fromItem and the toItem that is used to calculate the delta with: `local delta, from, to = delta2(2, 5, 3, 3)`.
 - **deltaSince([timeAgo](#Time_specification_.28timeAgo.29),  [smoothRange], [default] )**: Same as **delta** but within the `timeAgo` interval. The function also returns the fromItem and the toItem that is used to calculate the delta with: `local delta, from, to = deltaSince('00:00:10', 3)`.
 - **deltaSinceOrOldest([timeAgo](#Time_specification_.28timeAgo.29),  [smoothRangeFrom], [smoothRangeTo], [default] )**:	Same as **deltaSince** but it will take the oldest value in the set if *timeAgo* is older than the age of the entire set. The function also returns the fromItem and the toItem that is used to calculate the delta with: `local delta, from, to = deltaSinceOrOldest('00:00:10', 3, 3)`.
 - **localMin( [smoothRange], default )**:
 - **localMin( [smoothRange], default )**: Returns the first minimum value (and the item holding the minimal value) in the past. [Supports data smoothing](#Data_smoothing) when providing a `smoothRange` value. For example, given this range of values in the data set (from new to old): `10 8 7 5 3 4 5 6`, it will return `3` because older values *and* newer values are higher: a local minimum. Use this if you want to know at what time a temperature started to rise after it had been dropping. E.g.:

		local value, item = myVar.localMin()
		print(' minimum was : ' .. value .. ': ' .. item.time.secondsAgo .. ' seconds ago' )
 - **localMax([smoothRange], default)**: Same as **localMin** but for the maximum value. [Supports data smoothing](#Data_smoothing) when providing a `smoothRange` value.
 - **smoothItem(itemIdx, [smoothRange])**: Returns a the value of `itemIdx` in the set but smoothed by averaging with its neighbors. The number of neighbors is set by `smoothRange`. See [Data smoothing](#Data_smoothing).

#### Data smoothing

Example:
Suppose you store temperatures in the historical variable. These temperatures may have extremes. Perhaps these extremes could be due to sensor reading errors. In order to reduce the effect of these so-called spikes, you could smooth out values. It is like blurring the data. The Raw column is your temperatures. The other columns are calculated by averaging the neighbors. For item 10 the calculations are:

	Range=1 for time10 = (25 + 31 + 29) / 3 = 28,3
	Range=2 for time10 = (16 + 25 + 31 + 29 + 26) / 5 = 25,4

```{=mediawiki}

{| class="wikitable" style="text-align: center; align="center" width="30% height:10px;"
! style="text-align: center; background:darkblue; color:white" align="center" width="12%"| time
! style="text-align: center; background:darkblue; color:white" align="center" width="12%"align="center" width="12%"| raw
! style="text-align: center; background:darkblue; color:white" align="center" width="12%"align="center" width="12%"| range=1
! style="text-align: center; background:darkblue; color:white" align="center" width="12%"align="center" width="12%"| range=2
|-
| '''1'''
| 18
| 20,0
| 21,7
|-
| '''2'''
| 22
| 21,7
| 25,0
|-
| '''3'''
| 25
| 27,3
| 24,6
|-
| '''4'''
| 35
| 27,7
| 26,6
|-
| '''5'''
| 23
| 28,7
| 27,6
|-
| '''6'''
| 28
| 26,0
| 25,8
|-
| '''7'''
| 27
| 23,7
| 23,8
|-
| '''8'''
| 16
| 22,7
| 25,4
|-
| '''9'''
| 25
| 24,0
| 25,6
|-
| '''10'''
| 31
| 28,3
| 25,4
|-
| '''11'''
| 29
| 28,7
| 29,2
|-
| '''12'''
| 26
| 30,0
| 30,2
|-
| '''13'''
| 35
| 30,3
| 30,2
|-
| '''14'''
| 30
| 32,0
| 30,4
|-
| '''15'''
| 31
| 30,3
| 30,8
|-
| '''16'''
| 30
| 29,7
| 28,2
|-
| '''17'''
| 28
| 26,7
| 28,6
|-
| '''18'''
| 22
| 27,3
| 26,2
|-
| '''19'''
| 32
| 24,3
| 25,3
|-
| '''20'''
| 19
| 25,5
| 24,3
|}

```

A chart illustrates it more clearly. The red line is not smoothed and has more spikes than the others:

[[File:dzvents-smoothing.png|frame|none|alt=|Smoothing]]

Usually a range of 1 or 2 is sufficient when providing a smoothing range to statistical functions. I suggest smoothing when checking deltas, local minimums and maximums.

## How does the storage stuff work?
For every script file that defines persisted variables (using the `data={ … }` section), dzVents will create storage file with the name `__data_scriptname.lua` in a subfolder called `data`. You can always delete these data files or the entire storage folder if there is a problem with it:

	domoticz/
		scripts/
			dzVents/
				data/
					__data_yourscript1.lua
					__data_yourscript2.lua
					__data_global_data.lua
				examples/
				generated_scripts/
				scripts/
					yourscript1.lua
					yourscript2.lua
					global_data.lua

If you dare to, you can watch inside these files. Every time some data are changed, dzVents will stream the changes back into the data files.
**Again, make sure you don't put too much stuff in your persistent data as it may slow things down too much.**

# Asynchronous HTTP requests
dzVents allows you to make asynchronous HTTP request and handle the results. Asynchronous means that you don't block the system while waiting for the response. Earlier you had to use os functions like `curl` or `wget` and some magic to make sure that you didn't block the system for too long after which Domoticz will terminate the script with a message that it took more than 10 seconds.

dzVents to the rescue. With dzVents there are two ways to make an http call and it is determined by how you use the `domoticz.openURL()` command. The simplest form simply calls `openURL` on the domoticz object with only the url as the parameter (a string value):

```Lua
domoticz.openURL('http://domain/path/to/something?withparameters=1')
```

After your script is finished, Domoticz will make the request that's where it ends. No callback. Nothing.

The second way is different. Instead of passing a url you pass in a table with all the parameters to make the request **and** your provide a *callback trigger* which is just a string or a name:
```Lua
return {
	on = { ... }, -- some trigger
	execute = function(domoticz)
	domoticz.openURL({
		url = 'http://domain/path/to/something',
		method = 'POST',
		callback = 'mycallbackstring',
		postData = {
			paramA = 'something',
			paramB = 'something else'
		}
	})
	end
}
```
In this case, Domoticz will make the request (a POST in this case), and when done it will trigger an event. dzVents will capture that event and will execute all scripts listening for this callback trigger (*mycallbackstring*):

```Lua
return {
	on = {
	httpResponses = { 'mycallbackstring' }
	},
	execute = function(domoticz, response)
	if (response.ok) then -- success
		if (response.isJSON) then
			domoticz.log(response.json.some.value)
		end
	else
		domoticz.log('There was an error', domoticz.LOG_ERROR)
	end
	end
}
```
Of course you can combine the script that issues the request and handles the response in one script:
```Lua
return {
	on = {
	timer = {'every 5 seconds'},
	httpResponses = { 'trigger' }
	},
	execute = function(domoticz, item)
	if (item.isTimer) then
		domoticz.openURL({
			url = '...',
			callback = 'trigger'
		})
	end
	if (item.isHTTPResponse) then
		if (item.ok) then
			...
		end
	end
	end
}
```
## API

### Making the request:

 **domoticz.openURL(options)**: *options*	is a Lua table:

 - **url**: *String*.
 - **method**: *String*. Optional. Either `'GET'` (default), `'POST'`, `'PUT'`<sup>3.0.2</sup>  or `'DELETE'`<sup>3.0.2</sup> .
 - **callback**: *String*. Optional. A custom string that will be used by dzVents to trigger the callback handler script(s).
 - **headers**: *Table*. Optional. A Lua table with additions http request-headers.
 - **postData**: Optional. When doing a `POST`, `PUT` <sup>3.0.2</sup> or `DELETE`<sup>3.0.2</sup> this data will be the payload of the request (body). If you provide a Lua table then this will automatically be converted to json and the request-header `application/json` is set. So no need to do that manually.

Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

### The response object
The response object  (second parameter in your execute function) has these attributes:

 - **data**: Raw response data.
 - **hasLines**: *Boolean*. <sup>3.1.0</sup> When true, the data is automatically converted to a Lua table. (isJSON and isXML have preference)
 - **headers**: *Table*. Response headers.
 - **isJSON**: *Boolean*. Short for `response.headers['Content-Type'] == 'application/json'`. When true, the data is automatically converted to a Lua table.
 - **isXML**: *Boolean*. Short for `response.headers['Content-Type'] == 'text/xml'`. When true, the data is automatically converted to a Lua table.
  - **json**. *Table*. When the response data is `application/json` then the response data is automatically converted to a Lua table for quick and easy access.
 - **lines**: *Table*. <sup>3.1.0</sup> When the response data has multiple lines but is not a JSON or XML string then the response data is automatically converted to a Lua table for quick and easy access.
 - **ok**: *Boolean*. `True` when the request was successful. It checks for statusCode to be in range of 200-299.
 - **statusCode**: *Number*. HTTP status codes. See [HTTP response status codes](https://en.wikipedia.org/wiki/List_of_HTTP_status_codes ).
 - **statusText**: *String*. HTTP status message. See [HTTP response status codes]( https://en.wikipedia.org/wiki/List_of_HTTP_status_codes ).
 - **protocol**: *String*. HTTP protocol. See [HTTP response status codes]( https://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol ).
 - **trigger**, **callback**: *String*. The callback string that triggered this response instance. This is useful if you have a script that is triggered by multiple different callback strings.
 - **xml**. *Table*. When the response data is `text/xml` , the response data is automatically converted to a Lua table for quick and easy access.
 - **xmlEncoding**. *String*. When the response data is `text/xml` See [ xml encoding] ( https://en.wikipedia.org/wiki/XML ).
 - **xmlVersion**. *String*. When the response data is `text/xml` See [ xml versions ] ( https://en.wikipedia.org/wiki/XML ).

### More about request and response headers
Whenever you do an http request it is not just some data that is sent. Along with the request a bunch of so-called headers are sent along with it. HTTP headers allow the client and the server to pass additional information with the request or the response. Also, in the response there are also headers (response header). These response headers usually tell you what kind of data is returned, if it is compressed, if the request was successful etc.

#### request headers
dzVents allow you to set custom request headers that will accompany the data in the request. Sometimes it is necessary to set these headers like for instance when a API or webservice require a security token or key or when the service needs to know what the format of the response data is. Check the documentation of the web service.

So, let's say you need to call a web service that requires an api key in the headers and the documentation states it needs to be passed in an x-access-token header. Your openURL command then may look like this:
```Lua
domoticz.openURL({
	url = 'https://somedomain.com/service/getInfo',
	headers = { ['x-access-token'] = '<api-key>' },
	method = 'GET',
	callback = 'info'
})
```
Check google for more information about request headers. All you need to know here is that dzVents allow you to set these headers.

#### response headers
As said earlier, the response also contains a bunch of headers. You can inspect those headers with dzVents but ususally you don't have to. For starters, dzVents already checks the `Content-Type` header which usually states what kind of data format the response is. If it is `application/json` then it automatically converts the json data to a Lua table. Also, it checks the `Status` header to see if the request was successful. If so, then it sets the `ok` attribute on the response object. So normally you don't have to inspect the headers. However, sometimes the web service puts a session token in the response header that you have to use in follow-up requests.

So here an example where we have to log in before we can fetch data. It uses two requests: the first performs the log-in and the second grabs the session token from the login response data and uses that token in the second request to get the data we need. In this example we do this every hour.

That would look like this:

```Lua
return {
	on = {
		timer = {'every hour'},
		httpResponses = { 'loggedin', 'data' }
	},
	execute = function(domoticz, item)
		if (item.isTimer) then
			-- login
			domoticz.openURL({
				url = 'https://somedomain.com/login',
				method = 'POST',
				postData = { ['username'] = 'Luke Skywalker', ['password'] = 'theforce' }
				callback = 'loggedin'
			})
		end
		if (item.isHTTPResponse and item.ok) then
			-- check to which request this response was the result
			if (item.trigger == 'loggedin') then
				-- we are logged in, now grab the session token from the header and
				-- fetch our data
				local token = item.headers['x-session-token']
				-- now we have the token, put it in the headers:
				domoticz.openURL({
					url = 'https://somedomain.com/getData',
					method = 'GET',
					headers = { ['x-session-token'] = token },
					callback = 'data'
				})
			else
				-- it must the data we requested
				local data = item.data
				-- do something with it
			end
		end
	end
}
```

Some remarks about the response header `Content-Type`. If a service is a good web-citizen then it tells you what the format of the data is in this header. So, if the data is a json object then the header should be `application/json`. Unfortunately, there are lot of lazy programmers out there who don't set this header properly. If that is the case, dzVents cannot detect the format and will not turn it into a Lua table for you automatically. So, if you know it is json but the header is not properly set, then you can easily convert it into a Lua table in your code:

```Lua
return {
	on = {
		timer = {'every hour'},
		httpResponses = { 'trigger' }
	},
	execute = function(domoticz, item)
		if (item.isTimer) then
			-- login
			domoticz.openURL({
				url = 'https://somedomain.com/getData',
				callback = 'trigger'
			})
		end
		if (item.isHTTPResponse and item.ok) then
			-- we know it is json but dzVents cannot detect this
			-- convert to Lua
			local json = domoticz.utils.fromJSON(item.data)
			-- json is now a Lua table
			print(json.result.title) -- just an example
		end
	end
}
```

### Fetching data from Domoticz itself
Most of the things you need to do in your dzVents script is already exposed somewhere in the dzVents object hierarchy. Sometimes however you need some data that is not available in dzVents. Just to give an example, let's assume you have some zwave hardware and you want to know the `last seen` information of zwave devices or the status. This information is available in the node overview on the hardware page in Domoticz GUI. So, here is an example of how you can get that information into dzVents:

```Lua
return {
	on = {
		timer = { 'every hour' },
		httpResponses = {
			'zwaveInfo'
		}
	},
	execute = function(domoticz, item)

		if (item.isTimer) then
			-- check the index of your zwave hardware in the GUI
			-- in this example it is 2
			-- we assume you can access your Domoticz using the 127.0.0.1 ip
			-- on port 8080
			domoticz.openURL({
				url = 'http://127.0.0.1:8080/json.htm?type=openzwavenodes&idx=2',
				method = 'GET',
				callback = 'zwaveInfo',
			})
		end

		if (item.isHTTPResponse and item.ok) then
			local Time = require('Time')
			local results = item.json.result
			-- loop through the nodes and print some info
			for i, node in pairs(results) do
				-- convert the time stamp in the raw data into a
				-- dzVents Time object
				local lastUpdate = Time(node.LastUpdate)
				print(node.Name)
				print('Hours ago: ' .. lastUpdate.hoursAgo)
				print('State: ' .. node.State')
			end
		end
	end
}

```

# Asynchronous shell command execution

dzVents allows you to make asynchronous shell commands and handle the results. Asynchronous means that you don't block the system while waiting for the response. Earlier you had to use functions like `utils.osCommand()` or `os.Execute()` and some magic to make sure that you didn't block the system for too long after which Domoticz comes with a message that the script took more than 10 seconds.

dzVents to the rescue. With dzVents there are two ways to execute a shell command asynchronously and it is determined by how you use the `domoticz.executeShellCommand()` command. The simplest form simply calls `executeShellCommand` on the domoticz object with only the command as the parameter (a string value):

```Lua
domoticz.executeShellCommand('some shell command')
```

After your script is finished, Domoticz will execute the request in a separate thread and that's where it ends. No callback. Nothing.

The second way is different. Instead of passing a command you pass in a table with the command to be executed **and** a *callback trigger* which is just a string or a name:
```Lua
return {
	on = { ... }, -- some trigger
	execute = function(domoticz)
		domoticz.executeShellCommand({
			command = 'some shell command',
			callback = 'mycallbackstring',
			timeout = timeoutinseconds,
		})
	end
}
```
In this case, Domoticz will execute the command and when done it will trigger an event. dzVents will capture that event and will execute all scripts listening for this callback trigger (*mycallbackstring*):

```Lua
return {
	on = {
		shellCommandResponses = { 'mycallbackstring' }
	},
	execute = function(domoticz, response)
		if response.ok then
			if (response.isJSON) then
				domoticz.log(response.json.some.value)
			end
		else
			domoticz.log('There was an error', domoticz.LOG_ERROR)
		end
	end
}
```
Of course you can combine the script that issues the request and handles the response in one script:
```Lua
return
{
	on =
	{
		timer =
		{
			'every 5 minutes',
		},
		shellCommandResponses =
		{
			scriptVar,
		},
	},

	execute = function(domoticz, item)
		if item.isTimer then
			domoticz.executeShellCommand(
			{
				command = 'some shell command',
				callback = scriptVar,
				timeout = timeoutinseconds,
			})
		elseif item.isShellCommandResponse and then
			if item.statusCode == 0  then
				... -- process result (in item.json, -item.xml, -item.lines or item.data)
			end
		end
	end
}

```

## API

### Making the request:

 **domoticz.executeShellCommand(options)**: *options*	is a Lua table:

 - **command**: *String*.
 - **callback**: *String*. Optional. A custom string that will be used by dzVents to find a the callback handler script.
 - **timeout**: *Integer*. Optional. Max execution time in seconds. defaults to 10 seconds. A value of 0 disables the timeout.

Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

### The response object
The response object  (second parameter in your execute function) has these attributes:

 - **data**: output from the command.  (stdout)
 - **errorText**: Errors generated by the command (stderr)
 - **statusCode**: *Int*.  exitcode generated by the command
 - **hasLines**: *Boolean*. When true, the data is automatically converted to a Lua table. (isJSON and isXML have preference)
 - **isJSON**: *Boolean*. When true, the data is automatically converted to a Lua table.
 - **isXML**: *Boolean*. When true, the data is automatically converted to a Lua table.
 - **json**: *Table*. When the response data is `application/json` then the response data is automatically converted to a Lua table for quick and easy access.
 - **lines**: *Table*. When the response data has multiple lines but is not a JSON or XML string then the response data is automatically converted to a Lua table for quick and easy access.
 - **ok**: *Boolean*. `True` when the request was successful. It checks for statusCode to be 0.
 - **timeoutOccurred** *Boolean*  `True` when the request was aborted by a timeout.
 - **trigger**, **callback**: *String*. The callback string that triggered this response instance. This is useful if you have a script that is triggered by multiple different callback strings.
 - **xml**. *Table*. When the response data is `text/xml` , the response data is automatically converted to a Lua table for quick and easy access.
 - **xmlEncoding**. *String*. When the response data is `text/xml` See [ xml encoding] ( https://en.wikipedia.org/wiki/XML ).
 - **xmlVersion**. *String*. When the response data is `text/xml` See [ xml versions ] ( https://en.wikipedia.org/wiki/XML ).

# Settings

Settings for dzVents are found in the Domoticz GUI: **Setup > Settings > Other > EventSystem**:

 - **dzVents disabled**: Tick this if you don't want any dzVents script to be executed.
 - **Log level**: (Note that you can override this setting in the logging section of your script)
	- *Errors*,
	- *Errors + minimal execution info*: Errors and some minimal information about which script is being executed and why,
	- *Errors + minimal execution info + generic info*. Even more information about script execution and a bit more log formatting.
	- *Debug*. Shows everything and dzVents will create the files `domoticzData.lua` and `module.log` in the scripts/dzVents folder. `domoticzData.lua` is a dump of all the data received by dzVents from Domoticz and `module.log` is a summary of the execution time and CPU usage of the user scripts active during the debug period.
	- *No logging*. As silent as possible.

# Troubleshooting
So, you think if you have done everything correctly but things do not work (in some way) as you expected. Here are a couple steps you can do to find the cause.

### Is dzVents enabled?
Check the settings (see above) and make sure the checkbox **dzVents disabled** is not checked.

### Is your script enabled?
Make sure the active section in your script is set to `true`:  `active = true`. And, on top of that, if you are using the internal web editor to write your script, make sure that your script is active there and you have clicked Save! "Event active" is a separate checkbox that must be ticked for every active script. When not active, the script name will be red in the list on the right.

### Turn on debug logging
Activate debug logging in the settings (see above). This will produce a lot of extra messages in the Domoticz log (don't forget to turn it off when you are done troubleshooting!). It is best to monitor the log through the command line, as the log in the browser sometimes tends to not always show all log messages. See the Domoticz manual for how to do that.

When debug logging is enabled, every time dzVents kicks into action (Domoticz throws an event) it will log it, and it will create the files `domoticzData.lua` and `module.log` in `/path/to/domoticz/scripts/dzVents`. `domoticzData.lua` is a dump of all the data received by dzVents from Domoticz. This data lie at the core of the dzVents object model. You can open this file in an editor to see if your device/variable/scene/group/hardware is in there. Note that the data in the file are not exactly the same as what you will get when you interact with dzVents, but it is a good start. If your object is not in the data file, then you will not have access to it in dzVents and dzVents will not be able to match triggers with that object. Something's wrong if you expect your object to be in there but it is not (is the object active/used?).
`module.log` is a summary of the execution time and CPU usage of the user scripts active during the debug period.with a dump of the data sent to dzVents.

Example content of `module.log`
```
	Startdate time	- End date time	 (clk - CPU  )                 dule / scriptname << type: "trigger"
	---------------------------------------------------------------------------------------------------------------
	11/27/20 18:40:00 - 11/27/20 18:40:00 (00 - 0.0029)                     waqi.lua << timer: "every 20 minutes"
	11/27/20 18:40:00 - 11/27/20 18:40:00 (00 - 0.0016)                   Washer.lua << timer: "every 5 minutes"
	11/27/20 18:40:01 - 11/27/20 18:40:01 (00 - 0.0215)   getWaterFromHomewizard.lua << HTTPResponse: "WaterfromHomewizard"
	11/27/20 18:40:03 - 11/27/20 18:40:03 (00 - 0.2472)     updateWeatherSensors.lua << HTTPResponse: "WUS_buienradarResponse"
	11/27/20 18:40:03 - 11/27/20 18:40:03 (00 - 0.0290)                     waqi.lua << HTTPResponse: "waqi_nearby"
	11/27/20 18:41:00 - 11/27/20 18:41:00 (00 - 0.0583)        Bathroom humidity.lua << timer: "every minute between 05:15 and 23:30"
```

Every time Domoticz starts dzVents and debug logging is enabled you should see these lines in the domoticz log:
```
dzVents version: x.y.z
Event trigger type: aaaa
```
Where aaaa can be time, custom event, system event, device, uservariable, security or scenegroup. That should give you a clue what kind of event is active. If you don't see this information then dzVents is not active (or debug logging is not active).

### Script is still not executed
If for some reason your script is not executed while all of the above is done, it is possible that your triggers are not correct. Either the time rule is not matching with the current time (try to set the rule to `every minute` or something simple), or the device name is not correct (check casing), or you use an id that doesn't exist. Note that in the `on` section, you cannot use the dzVents domoticz object!

Make sure that if you use an id in the `on` section that this id is correct. Go to the device list in Domoticz and find the device and make sure the device is in the 'used' list!. Unused devices will not create an event. Find the idx for the device and use that as the id.

Also, make sure that your device names are unique! dzVents will throw a warning in the log if multiple device names match. It's best practice to have unique names for your devices. Use some clever naming like prefixing like 'Temperature: living room' or 'Lights: living room'. That also makes using wild cards on your `on` section easier.

If your script is still not triggered, you can try to create a classic Lua event script and see if that does work.

### Debugging your script
A simple way to inspect a device in your script is to dump it to the log: `myDevice.dump()`. This will dump everything known to dzVents of the device so you can inspect what its state is. If you only need to see a subset you can use dumpSelection('attributes'), dumpSelection('functions') or dumpSelection('tables')
Use print statements or domoticz.log() statements in your script at cricital locations to see if the Lua interpreter reaches that line.
Don't try to print a device object though; use the `myDevice.dump()` method for that. It wil log all attributes of the device in the Domoticz log.

If you place a print statement all the way at the top of your script file it will be printed every time dzVents loads your script (that is done at the beginning of each event cycle). Sometimes that is handy to see if your script gets loaded in the first place. If you don't see your print statement in the log, then likely dzVents didn't load it and it will not work.

### Get help
The Domoticz forum is a great resource for help and solutions. Check the [dzVents forum](https://www.domoticz.com/forum/viewforum.php?f=59).

# Other interesting stuff

## lodash for Lua

lodash is a well known and very popular Javascript library filled with dozens of handy helper functions that really make you life a lot easier. Fortunately there is also a Lua version. This is directly available through the domoticz object:

```Lua
local _ = domoticz.utils._
_.print(_.indexOf({2, 3, 'x', 4}, 'x'))
```

<!--- (Removed because link is dead. Hopefully only temporarily  ) Check out the great documentation [here](http://axmat.github.io/lodash.lua/).  --->
Check out the documentation [here](https://htmlpreview.github.io/?https://github.com/rwaaren/lodash.lua/blob/master/doc/index.html).

# History [link to changes in previous versions](https://www.domoticz.com/wiki/DzVents_version_History).

## [3.1.4] ##
- Fixed issue that prevented dzVents from accessing the domoticz API when using -wwwbind

## [3.1.3] ##
- Add method updateHistory for managed counter devices
- Added NSS_CLICKATELL as notification subsystem

## [3.1.2] ##
- Fixed issue with icon name
- Add attribute customImage (icon number or 0)
- Use level as brightness in getColor function
- Allow booleans as value in header field of openURL

## [3.1.1] ##
- Fixed issue that prevented dzVents from accessing the domoticz API when used in sslwww only mode

## [3.1.0]
- Added shell command event triggers to be used in combination with `executeShellCommand`. You can now execute shell commands and handle the response in your dzVents scripts **ASYNCHRONICALLY**. See the documentation. No more json parsing needed or complex `popen()` or 'system()'  shizzle.
- add inActive attribute to devices

## [3.0.19]
- Add thermostat Operating State device adapter

## [3.0.18]
- Add isdst as (boolean) attribute to time object
- Add updateQuiet() method to generic device
- Fixed bug in dumpTable that caused infinite loop for table with self-reference

## [3.0.17]
- Add timestampTodate, dateToTimestamp and dateToDate functions in Time

## [3.0.16]
- Add except as keyword in timeRules

## [3.0.15]
- Fixed bug in domoticz.time.matchesRule (daterange was ignored when "on mon, tue" was also part of the rule)
- Add device adapter for Thermostat type 3 devices (Mertik)
- Add utils.humidityStatus
- Add option to have dzVents compute humidity status

## [3.0.14]
- Add utils.fuzzyLookup
- Made eventHelpers more resilient to coding errors in the on = section

## [3.0.13]
- Add utils.osCommand (caption)
- Improved utils.stringSplit (implicit conversion to number on request)
- Made log function more resilient when logitem is table with embedded function(s)
- Improved domoticz.snapshot() to enable snapshot of multiple cameras (in one Email)

## [3.0.12]
- Add option to use device, camera, group, scene and variable objects as parm to deviceExists(), groupExists(), sceneExists(), variableExists(), cameraExists() methods.

## [3.0.11]
- Add sensorValue attribute to custom sensor
- Add solarnoon as moment in time (like sunrise / sunset )

## [3.0.10]
- Add NSS_GOOGLE_DEVICES for notification casting to Google home / Google chromecast
- Add optional parm delay to domoticz.sendCommand, domoticz.email, domoticz.sms and domoticz.notify

## [3.0.9]
- Add dump() as function to object types: camera-, customEvent, hardware, systemEvent, HTTPResponse, security and time.
- Add function toUTC to time object.
- Allow table as parm to function makeTime

## [3.0.8]
- Allow IPv6 ::1 as localhost in domoticz settings
- Fixed bug that occurred when using a decimal number in afterSec (openURL and emitEvent)
- Implement optional use of parsetrigger parm in setValues to trigger any subsequent eventscripts
- Updated round.utils to correctly handle negative numbers and round to zero decimals

## [3.0.7]
- Add domoticz.hardware() as separate object class

## [3.0.6]
- Add hardwareInfo() function

## [3.0.5]
- Add dumpSelection() method
- Fixed settings.url

## [3.0.4]
- Convert HTTPResponse data to JSON / XML even when HTTPResponse does not fully comply with RFC
- add isJSON, isXML functions to Utils

## [3.0.3]
- add isJSON, isXML, json, xml and customEvent attributes to customEvent object (consistent with response object)

## [3.0.2]
- Add `PUT` and `DELETE` support to `openURL`
- Ensure sending integer in nValue in update function
- Fix sValue for custom sensor

## [3.0.1]
- Add option `at()` to the various commands/methods
- Add stringToSeconds() function

## [3.0.0]
 - Add system-events triggers as option to the on = { ... } section. Scripts can now be triggered based on these system-events:
	 - start
	 - stop
	 - manualBackupFinished,
	 - dailyBackupFinished
	 - hourlyBackupFinished
	 - monthlyBackupFinished

 - Add custom-events triggers as option to the on = { ... } section. You can now send an event trigger to start subscribed dzVents scripts. customEvents can be triggered by:
	 - dzVents domoticz.emitEvent(name, data )  --  command  (data = optional)
	 - JSON: json.htm?type=command&param=customevent&event=MyEvent&data=myData ( data = optional )
	 - MQTT: {"command" : "customevent", "event" :	"MyEvent" , "data" : "myData" } ( data = opt ional )
 - Add method  domoticz.emitEvent()
 - Add attribute `mode` to Evohome controller
 - Add option to dumpTable() and ([device][uservariable][scene][group].dump() to os file
