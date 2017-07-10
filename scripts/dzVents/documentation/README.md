**For people working with dzVents prior to version 2.0: Please read the [change log](#Change_log) below as there are a couple of (easy-to-fix) breaking changes. Or check [Migrating from version 1.x.x](#Migrating_from_version_1.x.x)**

# About dzVents 2.0.0
dzVents (|diː ziː vɛnts| short for Domoticz Easy Events) brings Lua scripting in Domoticz to a whole new level. Writing scripts for Domoticz has never been so easy. Not only can you define triggers more easily, and have full control over timer-based scripts with extensive scheduling support, dzVents presents you with an easy to use API to all necessary information in Domoticz. No longer do you have to combine all kinds of information given to you by Domoticzs in many different data tables. You don't have to construct complex commandArrays anymore. dzVents encapsulates all the Domoticz peculiarities regarding controlling and querying your devices. And on top of that, script performance has increased a lot if you have many scripts because Domoticz will fetch all device information only once for all your device scripts and timer scripts.

Let's start with an example. Let's say you have a switch that when activated, it should activate another switch but only if the room temperature is above a certain level. And when done, it should send a notification. This is how it looks like in dzVents:

    return {
    	active = true,
    	on = {
    		devices = {
	    		'Room switch'
    		}
    	},
    	execute = function(domoticz, roomSwitch)
    		if (roomSwitch.state == 'On' and domoticz.devices('Living room').temperature > 18) then
    			domoticz.devices('Another switch').switchOn()
    			domoticz.notify('This rocks!',
    			                'Turns out that it is getting warm here',
    			                domoticz.PRIORITY_LOW)
    		end
    	end
    }

Or you have a timer script that should be executed every 10 minutes but only on weekdays and have it do something with some user variables and only during daytime:


    return {
    	active = true,
    	on = {
    		timer = {'Every 10 minutes on mon,tue,wed,thu,fri'}
    	},
    	execute = function(domoticz)
    		-- check time of the day
    		if (domoticz.time.isDayTime and domoticz.variables('myVar').value == 10) then
    			domoticz.variables('anotherVar').set(15)
    			--activate my scene
    			domoticz.setScene('Evening lights', 'On')
    			if (domoticz.devices('My PIR').lastUpdate.minutesAgo > 5) then
                    domoticz.devices('Bathroom lights').switchOff()
                end
    		end
    	end
    }

Or you want to detect a humidity rise within the past 5 minutes:

    return {
    	active = true,
    	on = {
	    	timer = {'every 5 minutes'}
    	},
    	data = {
	    	previousHumidity = { initial = 100 }
    	},
    	execute = function(domoticz)
    		local bathroomSensor = domoticz.devices('BathroomSensor')
    		if (bathroomSensor.humidity - domoticz.data.previousHumidity) >= 5) then
    			-- there was a significant rise
    			domoticz.devices('Ventilator').switchOn()
    		end
    		-- store current value for next cycle
    		domoticz.data.previousHumidity = bathroomSensor.humidity
    	end
    }

Just to give you an idea! Everything in your Domoticz system is now logically available in the domoticz object structure. With this domoticz object you can get to all the information in your system and manipulate your devices.

# Using dzVents with Domoticz
In Domoticz go to **Setup > Settings > Other**  and in the section EventSystem make sure the checkbox 'dzVents disabled' is not checked.
Also make sure that in the Security section in the settings you allow `127.0.0.1` to not need a password. dzVents uses that port to send certain commands to Domoticz. Finally make sure you have set your current location in **Setup > Settings > System > Location** otherwise there is no way to determine nighttime/daytime state.

There are two ways of creating dzVents event script in Domoticz:

 1. By creating lua scripts in your domoticz instance on your domoticz server: `/path/to/domoticz/scripts/dzVents/scripts`. Make sure that each script has the extension `.lua` and follows the guidelines as described below.
 2. By creating lua scripts inside Domoticz using the internal Domoticz event editor: Go to **Setup > More Options > Events** and set the event type to Lua (dzVents).

**Note: scripts that you write on the filesystem and inside Domoticz using the internal web-editor all share the same namespace. That means that if you have two scripts with the same name, only the one of the filesystem will be used. The log will tell you when this happens.**

## Quickstart
If you made sure that dzVents system is active we can do a quick test if everything works:

 - Pick a switch in your Domoticz system. Write down the exact name of the switch. If you don't have a switch then you can create a Dummy switch and use that one.
 - Create a new file in the `/path/to/domoticz/scripts/dzVents/scripts/` folder (or using the web-editor in Domoticz, swtich to dzVents mode first.). Call the file `test.lua`. *Note: when you create a script in the web-editor you do **not** add the .lua extension!*
 - Open `test.lua` in an editor and fill it with this code and change `<exact name of the switch>` with the .. you guessed it... exact name of the switch device:
```
     return {
    	active = true,
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

See the examples folder `/path/to/domoticz/scripts/dzVents/examples` for more examples.

# Writing scripts
In order for your scripts to work with dzVents, they have to be turned into a Lua module. Basically you make sure it returns a Lua table (object) with a couple of predefined keys like `active`, `on` and `execute`.. Here is an example:

    return {
        active = true,
        on = {
            devices = {
	            'My switch'
            }
        },
        execute = function(domoticz, device)
            -- your script logic goes here, something like this:

            if (device.state == 'On') then
                domoticz.notify('I am on!', '', domoticz.PRIORITY_LOW)
            end
        end
    }

Simply said, if you want to turn your existing script into a script that can be used with dzVents, you put it inside the execute function.

So, the module returns a table with these sections (keys):

    return {
        active = ... ,
        on = {
	        devices = { ... },
	        variables = { ... },
	        timer = { ... },
	        security = { ... }
        },
        data = { ... },
        execute = function(domoticz, device, triggerInfo)
    		...
        end
    }

## Sections in the script
As you can see in the example above, the script has a couple of sections necessary for the script to do when and what you want:

### active = { ... }
If the active-setting is false then the script is not activated. dzVents will ignore the file completely.
The active setting can either be:

 - **true**: the script file will be processed.
 - **false**: the script file is ignored.
 - **function**: A function returning `true` or `false`. The function will receive the domoticz object with all the information about you domoticz instance: `active = function(domoticz) .... end`. So for example you could check for a Domoticz variable or switch and prevent the script from being executed. **However, be aware that for *every script* in your scripts folder, this active function will be called, every cycle!! So, it is better to put all your logic in the execute function instead of in the active function.**

### on = { ... }
The on-section holds all the events/triggers that are monitored by dzVents. If any of the events or triggers matches with the current event coming from Domoticz then the `execute` part of the script is executed.
The on-section has four kinds of subsections that *can all be used simultaneously!* :

 - **devices = { ...}**: this is a list of devices. Each device can be:
	 -  The name of your device between string quotes. **You can use the asterisk (\*) wild-card here e.g. `PIR_*` or `*_PIR`**.  E.g.: `devices = { 'myDevice', 'anotherDevice', 123, 'pir*' }`
	 - The index of your device (the name may change, the index will usually stay the same, the index can be found in the devices section in Domoticz),
	 - The name/id of your device followed by a time constraint (similar to what you can do):
		 - `['myDevice']  = { 'at 15:*', 'at 22:** on sat, sun' }` The script will be executed if `myDevice` was changed and it is either between 15:00 and 16:00 or between 22:00 and 23:00 in the weekend.
 - **timer = { ... }**: a list of time triggers like `every minute` or `at 17:*`. See [below](#timer_trigger_options).
 - **variables = { ... }**: a list of user variable-names as defined in Domoticz ( Setup > More options > User variables). If any of the variables listed here changes, the script is executed.
 - **security = { domoticz.SECURITY_ARMEDAWAY, domoticz.SECURITY_ARMEDHOME, domoticz.SECURITY_DISARMED}**: a list of one or more security states. If the security state in Domoticz changes and it matches with any of the states listed here, the script will be executed.

### execute = function(domoticz, device/variable, triggerInfo) ... end
When al the above conditions are met (active == true and the on section has at least one matching rule) then this function is called. This is the heart of your script. The function receives three possible parameters:

 - the [domoticz object](#Domoticz_object_API). This gives access to almost everything in your Domoticz system including all methods to manipulate them like modifying switches or sending notifications. More about the domoticz object below.
 - the actual [device](#Device_object_API) or [variable](#Variable_object_API) that was defined in the **on** part and caused the script to be called. **Note: of course, if the script was triggered by a timer event, or a security-change event this parameter is *nil*! You may have to test this in your code if your script is triggered by timer events AND device events**
 - information about what triggered the script. This is a small table with two keys:
		* **triggerInfo.type**: (either domoticz.EVENT_TYPE_TIMER, domoticz.EVENT_TYPE_DEVICE, domoticz.EVENT_TYPE_SECURITY or domoticz.EVENT_TYPE_VARIABLE): was the script executed due to a timer event, device-change event, security-change or user variable-change event?
		* **triggerInfo.trigger**: which timer rule triggered the script in case the script was called due to a timer event. or the security state that triggered the security trigger rule. See [below](#timer_trigger_options) for the possible timer trigger options. Note that dzVents lists the first timer definition that matches the current time so if there are more timer triggers that could have been triggering the script, dzVents only picks the first for this trigger property.

### data = { ... }
The optional data section allows you to define variables that will be persisted between script runs. These variables can get a value in your execute function (e.g. `domoticz.data.previousTemperature = device.temperature`) and the next time the script is executed this value is again available in your code (e.g. `if (domoticz.data.previousTemperature < 20) then ...`. For more info see the section [persistent data](#Persistent_data). *Note that this functionality is kind of experimental. It can be a bit fragile so don't let your entire domotica system depend on the current state of these variables.*

### logging = { ... }
The optional logging setting allows you to override the global logging setting of dzVents as set in Setup > Settings > Other > EventSystem > dzVents Log Level. This can be handy when you only want this script to have extensive debug logging while the rest of your script execute silently. You have these options:

 - **level**: This is the log-level you want for this script. Can be either domoticz.LOG_INFO, domoticz.LOG_MODULE_EXEC_INFO, domoticz.LOG_DEBUG or domoticz.LOG_ERROR
 - **marker**: A string that is prefixed before each log message. That way you can easily create a filter in the Domoticz log to just see these messages.

E.g:
```
logging = {
	level = domoticz.LOG_DEBUG,
	marker = "Hey you"
	},
```

## *timer* trigger options
There are several options for time triggers. It is important to know that Domoticz timer events are only trigger once every minute. So that is the smallest interval for you timer scripts. However, dzVents gives you great many options to have full control over when and how often your timer scripts are called (all times are in 24hr format!). :

    on = {
	    timer = {
			'every minute',        -- causes the script to be called every minute
	        'every other minute',  -- minutes: xx:00, xx:02, xx:04, ..., xx:58
	        'every <xx> minutes',  -- starting from xx:00 triggers every xx minutes
	                               -- (0 > xx < 60)
	        'every hour',          -- 00:00, 01:00, ..., 23:00  (24x per 24hrs)
	        'every other hour',    -- 00:00, 02:00, ..., 22:00  (12x per 24hrs)
	        'every <xx> hours',    -- starting from 00:00, triggers every xx
	                               -- hours (0 > xx < 24)
	        'at 13:45',            -- specific time
	        'at *:45',             -- every 45th minute in the hour
	        'at 15:*',             -- every minute between 15:00 and 16:00
	        'at 12:45-21:15',      -- between 12:45 and 21:15. You cannot use '*'!
	        'at 19:30-08:20',      -- between 19:30 and 8:20 then next day
	        'at 13:45 on mon,tue', -- at 13:45 only on Monday en Tuesday (english)
	        'every hour on sat',   -- you guessed it correctly
	        'at sunset',           -- uses sunset/sunrise info from Domoticz
	        'at sunrise',
	        'at sunset on sat,sun',
	        'at nighttime',         -- between sunset and sunrise
	        'at daytime',           -- between sunrise and sunset
	        'at daytime on mon,tue', -- between sunrise and sunset
	                                   only on monday and tuesday

	        -- or if you want to go really wild:
	        'at nighttime at 21:32-05:44 every 5 minutes on sat, sun',

			-- or just do it yourself:
	        function(domoticz)
		        -- you can use domoticz.time to get the current time
		        -- note that this function is called every minute!
		        -- custom code that either returns true or false
		        ...
	        end
	   },
   }

**One important note: if Domoticz, for whatever reason, skips a beat (skips a timer event) then you may miss the trigger! So you may have to build in some fail-safe checks or some redundancy if you have critical time-based stuff to control. There is nothing dzVents can do about it**

Another important issue: the way it is implemented right now the `every xx minutes` and `every xx hours` is a bit limited. The interval resets at every \*:00  (for minutes) or 00:* for hours. So you need an interval that is an integer divider of 60 (or 24 for the hours). So you can do every 1, 2, 3, 4, 5, 6, 10, 12, 15, 20 and 30 minutes only.

# The domoticz object
And now the most interesting part. Without dzVents all the device information was scattered around in a dozen global Lua tables like `otherdevices` or `devicechanged`. You had to write a lot of code to collect all this information and build your logic around it. And, when you want to update switches and stuff you had to fill the commandArray with often low-level stuff in order to make it work.
Fear no more: introducing the **domoticz object**.

The domoticz object contains everything that you need to know in your scripts and all the methods to manipulate your devices and sensors. Getting this information has never been more easy:

`domoticz.time.isDayTime` or `domoticz.devices('My sensor').temperature` or `domoticz.devices('My sensor').lastUpdate.minutesAgo`.

So this object structure contains all the information logically arranged where you would expect it to be. Also, it harbors methods to manipulate Domoticz or devices. dzVents will create the commandArray contents for you and all you have to do is something like `domoticz.devices(123).switchOn().forMin(5).afterSec(10)` or `domoticz.devices('My dummy sensor').updateBarometer(1034, domoticz.BARO_THUNDERSTORM)`.

*The intention is that you don't have to construct low-level commandArray-commands for Domoticz anymore!* The domoticz object tries to encapsulate every low-level commandArray-command into a nice API.  If there is still something missing, please file a report in the Domoticz issue-tracker on GitHub and until then you can always use the sendCommand method which will dispatch the command to the commandArray

One tip:
**Make sure that all your devices have unique names!! dzVents doesn't check for duplicates!!**

## dzVents API
The domoticz object holds all information about your Domoticz system. It has a couple of global attributes and methods to query and manipulate your system. It also has a collection of **devices**, **variables** (user variables in Domoticz), **scenes**, **groups** and when applicable, a collection of **changedDevices** and **changedVariables**. All these collections each have three iterator functions: `forEach(function)`, `filter(function)` and `reduce(function)` to make searching for devices easier. See [iterators](#Iterators) below.

### Domoticz object API:

 - **changedDevices(id/name)**: *Function*. A function returning the device by id (or name).  To iterate over all changed devices do: `domoticz.changedDevices().forEach(..)`. See [Iterators](#Iterators). Note that you cannot do `for i, j in pairs(domoticz.changedDevices()) do .. end`.
 - **changedVariables(id/name)**: *Function*. A function returning the user variable by id or name. To iterate over all changed variables do: `domoticz.changedVariables().forEach(..)`. See [Iterators](#Iterators). Note that you cannot do `for i, j in pairs(domoticz.changedVariables()) do .. end`.
 - **devices(id/name)**: *Function*. A function returning a device  by id or name: `domoticz.devices(123)` or `domoticz.devices('My switch')`. See [Device object API](#Device_object_API) below. To iterate over all devices do: `domoticz.devices().forEach(..)`. See [Iterators](#Iterators). Note that you cannot do `for i, j in pairs(domoticz.devices()) do .. end`.
 - **email(subject, message, mailTo)**: *Function*. Send email.
 - **groups(id/name)**: *Function*: A function returning a group by name or id. Each group has the same interface as a device. To iterate over all groups do: `domoticz.groups().forEach(..)`. See [Iterators](#Iterators). Note that you cannot do `for i, j in pairs(domoticz.groups()) do .. end`.
 - **helpers**: *Table*. Collection of shared helper functions available to all your dzVents scripts. See [Creating shared helper functions](#Creating_shared_helper_functions).
 - **log(message, [level])**: *Function*. Creates a logging entry in the Domoticz log but respects the log level settings. You can provide the loglevel: `domoticz.LOG_INFO`, `domoticz.LOG_DEBUG`, `domoticz.LOG_ERROR` or `domoticz.LOG_FORCE`. In Domoticz settings you can set the log level for dzVents.
 - **notify(subject, message, priority, sound, extra, subsystem)**: *Function*. Send a notification (like Prowl). Priority can be like `domoticz.PRIORITY_LOW, PRIORITY_MODERATE, PRIORITY_NORMAL, PRIORITY_HIGH, PRIORITY_EMERGENCY`. For sound see the SOUND constants below. `subsystem` can be a table containing one or more notification subsystems. See `domoticz.NSS_xxx` types.
 - **openURL(url)**: *Function*. Have Domoticz 'call' a URL.
 - **scenes(id/name)**: *Function*: A function returning a scene by name or id. Each scene has the same interface as a device. See [Device object API](#Device_object_API). To iterate over all scenes do: `domoticz.scenes().forEach(..)`. See [Iterators](#Iterators). Note that you cannot do `for i, j in pairs(domoticz.scenes()) do .. end`.
 - **security**: Holds the state of the security system e.g. `Armed Home` or `Armed Away`.
 - **sendCommand(command, value)**: Generic (low-level)command method (adds it to the commandArray) to the list of commands that are being sent back to domoticz. *There is likely no need to use this directly. Use any of the device methods instead (see below).*
 - **sms(message)**: *Function*. Sends an sms if it is configured in Domoticz.
 - **time**: Current system time:
	 - **day**: *Number*
	 - **getISO**: *Function*. Returns the ISO 8601 formatted date.
	 - **hour**: *Number*
	 - **isDayTime**
	 - **isNightTime**
	 - **isToday**: *Boolean*. Indicates if the device was updated today
	 - **month**: *Number*
	 - **min**: *Number*
 	 - **raw**: *String*. Generated by Domoticz
 	 - **rawDate**: *String*. Returns the date part of the raw data.
 	 - **rawTime**: *String*. Returns the time part of the raw data.
	 - **sec**: *Number*
	 - **sunsetInMinutes**
	 - **sunriseInMinutes**
	 - **wday**: *Number*. Day of the week ( 0 == sunday)
	 - **year**: *Number*
 - **toCelsius(f, relative)**: *Function*. Converts temperature from Fahrenheit to Celsius along the temperature scale or when relative==true it uses the fact that 1F==0.56C. So `toCelsius(5, true)` returns 5F*(1/1.8) = 2.78C.
 - **variables(id/name)**: *Function*. A function returning a variable by it's name or id. See  [Variable object API](#Variable_object_API) for the attributes. To iterate over all variables do: `domoticz.variables().forEach(..)`. See [Iterators](#Iterators). Note that you cannot do `for i, j in pairs(domoticz.variables()) do .. end`.

### Iterators
The domoticz object has a couple of collections (tables): devices, scenes, groups, variables, changedDevices and changedVariables. In order to make iterating over these collections easier, dzVents has three iterator methods so you don't need to use the `pair()` or `ipairs()` function anymore (less code to write):

 1. **find(function)**: Returns the item in the collection for which `function` returns true. When no item is found `find` returns nil.
 2. **forEach(function)**: Executes a provided function once per array element. The function receives the item in the collection (device or variable). If you return *false* in the function then the loop is aborted.
 3. **filter(function)**: returns items in the collection for which the function returns true.
 4. **reduce(function, initial)**: Loop over all items in the collection and do some calculation with it. You call it with the function and the initial value. Each iteration the function is called with the accumulator and the item in the collection. The function does something with the accumulator and returns a new value for it.

Best to illustrate with an example:

find():

	local myDevice = domoticz.devices().find(function(device)
		return device.name == 'myDevice'
	end)
	domoticz.log('Id: ' .. myDevice.id)

forEach():

    domoticz.devices().forEach(function(device)
    	if (device.batteryLevel < 20) then
    		-- do something
    	end
    end)

filter():

	local deadDevices = domoticz.devices().filter(function(device)
		return (device.lastUpdate.minutesAgo > 60)
	end)
	deadDevices.forEach(function(zombie)
		-- do something
	end)

Of course you can chain:

	domoticz.devices().filter(function(device)
		return (device.lastUpdate.minutesAgo > 60)
	end).forEach(function(zombie)
		-- do something with the zombie
	end)

Using a reducer to count all devices that are switched on:

    local count = domoticz.devices().reduce(function(acc, device)
	    if (device.state == 'On') then
		    acc = acc + 1 -- increase the accumulator
	    end
	    return acc -- always return the accumulator
    end, 0) -- 0 is the initial value for the accumulator


### Contants

 - **ALERTLEVEL_GREY**, **ALERTLEVEL_GREEN**, **ALERTLEVEL_ORANGE**, **ALERTLEVEL_RED**, **ALERTLEVEL_YELLOW**: For updating text sensors.
 - **BARO_CLOUDY**, **BARO_CLOUDY_RAIN**, **BARO_STABLE**, **BARO_SUNNY**, **BARO_THUNDERSTORM**, **BARO_NOINFO**, **BARO_UNSTABLE**: For updating barometric values.
 - **EVENT_TYPE_DEVICE**, **EVENT_TYPE_VARIABLE**, **EVENT_TYPE_SECURITY**, **EVENT_TYPE_TIMER**: triggerInfo types passed to the execute function in your scripts.
 - **EVOHOME_MODE_AUTO**, **EVOHOME_MODE_TEMPORARY_OVERRIDE**, **EVOHOME_MODE_PERMANENT_OVERRIDE**: mode for EvoHome system.
 - **HUM_COMFORTABLE**, **HUM_DRY**, **HUM_NORMAL**, **HUM_WET**: Constant for humidity status.
 - **INTEGER**, **FLOAT**, **STRING**, **DATE**, **TIME**: variable types.
 - **LOG_DEBUG**, **LOG_ERROR**, **LOG_INFO**, **LOG_FORCE**: For logging messages. LOG_FORCE is at the same level as LOG_ERROR.
 - **NSS_GOOGLE_CLOUD_MESSAGING**, **NSS_HTTP**,
**NSS_KODI**, **NSS_LOGITECH_MEDIASERVER**, **NSS_NMA**,**NSS_PROWL**, **NSS_PUSHALOT**, **NSS_PUSHBULLET**, **NSS_PUSHOVER**, **NSS_PUSHSAFER**: for notification subsystem
 - **PRIORITY_LOW**, **PRIORITY_MODERATE**, **PRIORITY_NORMAL**, **PRIORITY_HIGH**, **PRIORITY_EMERGENCY**: For notification priority.
 - **SECURITY_ARMEDAWAY**, **SECURITY_ARMEDHOME**, **SECURITY_DISARMED**: For security state.
 - **SOUND_ALIEN** , **SOUND_BIKE**, **SOUND_BUGLE**, **SOUND_CASH_REGISTER**, **SOUND_CLASSICAL**, **SOUND_CLIMB** , **SOUND_COSMIC**, **SOUND_DEFAULT** , **SOUND_ECHO**, **SOUND_FALLING**  , **SOUND_GAMELAN**, **SOUND_INCOMING**, **SOUND_INTERMISSION**, **SOUND_MAGIC** , **SOUND_MECHANICAL**, **SOUND_NONE**, **SOUND_PERSISTENT**, **SOUND_PIANOBAR** , **SOUND_SIREN** , **SOUND_SPACEALARM**, **SOUND_TUGBOAT**  , **SOUND_UPDOWN**: For notification sounds.

## Device object API
Each device in Domoticz can be found in the `domoticz.devices()` collection as listed above. The device object has a set of fixed attributes like *name* and *id*. Many devices though have different attributes and methods depending on there (hardware)type, subtype and other device specific identifiers. So it is possible that you will get an error if you call a method on a device that doesn't support it. If that is the case please check the device properties (can also be done in your script code!).

dzVents already recognizes most of the devices and creates the proper attributes and methods. It is always possible that your device-type has attributes that are not recognized. If that's the case please create a ticket in Domoticz issue-tracker on GitHub and an adapter for that device will be added.

Most of the time when your device is not recognized you can always use the `rawData` attribute as that will almost always hold all the data that is available in Domoticz.

### Device attributes and methods for all devices
 - **batteryLevel**: *Number* If applicable for that device then it will be from 0-100.
 -  **bState**: *Boolean*. Is true for some common states like 'On' or 'Open' or 'Motion'.
 - **changed**: *Boolean*. True if the device was changed
 - **description**: *String*. Description of the device.
 - **deviceSubType**: *String*. See Domoticz devices table in Domoticz GUI.
 - **deviceType**: *String*. See Domoticz devices table in Domoticz GUI.
 - **dump()**: *Function*. Dump all attributes to the Domoticz log. This ignores the log level setting.
 - **hardwareName**: *String*. See Domoticz devices table in Domoticz GUI.
 - **hardwareId**: *Number*. See Domoticz devices table in Domoticz GUI.
 - **hardwareType**: *String*. See Domoticz devices table in Domoticz GUI.
 - **hardwareTypeValue**: *Number*. See Domoticz devices table in Domoticz GUI.
 - **icon**: *String*. Name of the icon in Domoticz GUI.
 - **id**: *Number*. Id of the device
 - **lastUpdate**:
	 - **day**: *Number*
	 - **getISO**: *Function*. Returns the ISO 8601 formatted date.
	 - **hour**: *Number*
 	 - **hoursAgo**: *Number*. Number of hours since the last update.
	 - **isToday**: *Boolean*. Indicates if the device was updated today
	 - **month**: *Number*
	 - **min**: *Number*
	 - **minutesAgo**: *Number*. Number of minutes since the last update.
	 - **raw**: *String*. Generated by Domoticz
  	 - **rawDate**: *String*. Returns the date part of the raw data.
 	 - **rawTime**: *String*. Returns the time part of the raw data.
	 - **sec**: *Number*
	 - **secondsAgo**: *Number*. Number of seconds since the last update.
	 - **year**: *Number*
 - **name**: *String*. Name of the device.
 - **nValue**: *Number*. Numerical representation of the state.
 - **rawData**: *Table*: All values are *String* types and hold the raw data received from Domoticz.
 - **setState(newState)**: *Function*. Generic update method for switch-like devices. E.g.: device.setState('On'). Supports timing options. See [below](#Switch_timing_options_.28delay.2C_duration.29).
- **state**: *String*. For switches this holds the state like 'On' or 'Off'. For dimmers that are on, it is also 'On' but there is a level attribute holding the dimming level. **For selector switches** (Dummy switch) the state holds the *name* of the currently selected level. The corresponding numeric level of this state can be found in the **rawData** attribute: `device.rawData[1]`.
 - **signalLevel**: *Number* If applicable for that device then it will be from 0-100.
 - **switchType**: *String*. See Domoticz devices table in Domoticz GUI.
 - **switchTypeValue**: *Number*. See Domoticz devices table in Domoticz GUI.
 - **timedOut**: *Boolean*. Is true when the device couldn't be reached.
 - **update(< params >)**: *Function*. Generic update method. Accepts any number of parameters that will be sent back to Domoticz. There is no need to pass the device.id here. It will be passed for you. Example to update a temperature: `device.update(0,12)`. This will eventually result in a commandArray entry `['UpdateDevice']='<idx>|0|12'`

### Device attributes and methods for specific devices
Note that if you do not find your specific device type here you can always inspect what is in the `rawData` attribute. Please lets us know that it is missing so we can write an adapter for it (or you can write your own and submit it). You can dump all attributes for your device by calling `myDevice.dump()`. That will show you all the attributes and their values in the Domoticz. log.

#### Air quality
  - **co2**: *Number*. PPM
  - **quality**: *String*. Air quality string.
  - **updateAirQuality(ppm)**: Pass the CO2 concentration.

#### Alert sensor
 - **updateAlertSensor(level, text)**: *Function*. Level can be domoticz.ALERTLEVEL_GREY, ALERTLEVEL_GREEN, ALERTLEVEL_YELLOW, ALERTLEVEL_ORANGE, ALERTLEVEL_RED

#### Barometer sensor
 - **barometer**. *Number*. Barometric pressure.
 - **forecast**: *Number*.
 - **forecastString**: *String*.
 - **updateBarometer(pressure, forecast)**: *Function*. Update barometric pressure. Forecast can be domoticz.BARO_STABLE, BARO_SUNNY, BARO_CLOUDY, BARO_UNSTABLE, BARO_THUNDERSTORM.

#### Counter and counter incremental
 - **counter**: *Number*
 - **counterToday**: *Number*. Today's counter value.
 - **updateCounter(value)**: *Function*.
 - **valueQuantity**: *String*. For counters.
 - **valueUnits**: *String*.

#### Custom sensor
 - **sensorType**: *Number*.
 - **sensorUnit**: *String*:
 - **updateCustomSensor(value)**: *Function*.

#### Distance sensor
 - **distance**: *Number*.
 - **updateDistance(distance)**: *Function*.

#### Electric usage
 - **WActual**: *Number*. Current Watt usage.

#### Evohome
 - **setPoint**: *Number*.
 - **updateSetPoint(setPoint, mode, until)**: *Function*. Update set point. Mode can be domoticz.EVOHOME_MODE_AUTO, EVOHOME_MODE_TEMPORARY_OVERRIDE or EVOHOME_MODE_PERMANENT_OVERRIDE. You can provide an until date (in ISO 8601 format e.g.: `os.date("!%Y-%m-%dT%TZ")`).

#### Gas
 - **counter**: *Number*
 - **counterToday**: *Number*.
 - **updateGas(usage)**: *Function*.

#### Group
 - **toggleGroup()**: Toggles the state of a group.
 - **switchOff()**: *Function*. Supports timing options. See [below](#Switch_timing_options_.28delay.2C_duration.29).
 - **switchOn()**: Supports timing options. See [below](#Switch_timing_options_.28delay.2C_duration.29).

#### Humidity sensor
 - **humidity**: *Number*
 - **humidityStatus**: *String*
 - **updateHumidity(humidity, status)**: Update humidity. status can be domoticz.HUM_NORMAL, HUM_COMFORTABLE, HUM_DRY, HUM_WET

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
 - **counterToday**: *Number*.
 - **updateElectricity(power, energy)**: *Function*.
 - **usage**: *Number*.
 - **WhToday**: *Number*. Total Wh usage of the day. Note the unit is Wh and not kWh!

#### Lux sensor
 - **lux**: *Number*. Lux level for light sensors.
 - **updateLux(lux)**: *Function*.

#### OpenTherm gateway
 - **setPoint**: *Number*.
 - **updateSetPoint(setPoint)**: *Function*.
 -
#### P1 Smart meter
 - **counterDeliveredToday**: *Number*.
 - **counterToday**: *Number*.
 - **usage1**, **usage2**: *Number*.
 - **return1**, **return2**: *Number*.
 - **updateP1(usage1, usage2, return1, return2, cons, prod)**: *Function*. Updates the device.
 - **usage**: *Number*.
 - **usageDelivered**: *Number*.

#### Percentage
 - **percentage**: *Number*.
 - **updatePercentage(percentage)**: *Function*.

#### Philips Hue Light
 - **level**: *Number*. For dimmers and other 'Set Level..%' devices this holds the level like selector switches.
 - **maxDimLevel**: *Number*.
 - **switchOff()**: *Function*. Switch device off it is supports it. Supports timing options. See [below](#Switch_timing_options_.28delay.2C_duration.29).
 - **switchOn()**: *Function*. Switch device on if it supports it. Supports timing options. See [below](#Switch_timing_options_.28delay.2C_duration.29).
(#Switch_timing_options_.28delay.2C_duration.29).
 - **toggleSwitch()**: *Function*. Toggles the state of the switch (if it is toggle-able) like On/Off, Open/Close etc.

#### Pressure
 - **pressure**: *Number*.
 - **updatePressure(pressure)**: *Function*.

#### Rain meter
 - **rain**: *Number*
 - **rainRate**: *Number*
 - **updateRain(rate, counter)**: *Function*.

#### Scene
 - **switchOn()**: *Function*. Supports timing options. See [below](#Switch_timing_options_.28delay.2C_duration.29).

#### Security
 - **armAway()**: Sets a security device to Armed Away.
 - **armHome()**: Sets a security device to Armed Home.
 - **disarm()**: Disarms a security device.

#### Solar radiation
 - **radiation**. *Number*
 - **updateRadiation(radiation)**: *Function*.

#### Switch (dimmer, selector etc.)
There are many switch-like devices. Not all methods are applicable for all switch-like devices. You can always use the generic `setState(newState)` method.

 - **close()**: *Function*. Set device to Close if it supports it. Supports timing options. See [below](#Switch_timing_options_.28delay.2C_duration.29).
 - **dimTo(percentage)**: *Function*. Switch a dimming device on and/or dim to the specified level. Supports timing options. See [below](#Switch_timing_options_.28delay.2C_duration.29).
 - **level**: *Number*. For dimmers and other 'Set Level..%' devices this holds the level like selector switches.
 - **levelActions**: *String*. |-separated list of url actions for selector switches.
 - **levelName**: *Table*. Table holding the level names for selector switch devices.
 - **maxDimLevel**: *Number*.
 - **open()**: *Function*. Set device to Open if it supports it. Supports timing options. See [below](#Switch_timing_options_.28delay.2C_duration.29).
 - **stop()**: *Function*. Set device to Stop if it supports it (e.g. blinds). Supports timing options. See [below](#Switch_timing_options_.28delay.2C_duration.29).
 - **switchOff()**: *Function*. Switch device off it is supports it. Supports timing options. See [below](#Switch_timing_options_.28delay.2C_duration.29).
 - **switchOn()**: *Function*. Switch device on if it supports it. Supports timing options. See [below](#Switch_timing_options_.28delay.2C_duration.29).
 - **switchSelector(level)**: *Function*. Switches a selector switch to a specific level (numeric value, see the edit page in Domoticz for such a switch to get a list of the values). Supports timing options. See [below](#Switch_timing_options_.28delay.2C_duration.29).
 - **toggleSwitch()**: *Function*. Toggles the state of the switch (if it is togglable) like On/Off, Open/Close etc.

#### Temperature sensor
 - **temperature**: *Number*
 - **updateTemperature(temperature)**: *Function*. Note: temperature must be in Celsius. Use `domoticz.toCelsius()` to convert a Fahrenheit temperature to Celsius.

#### Temperature, Humidity, Barometer sensor
 - **barometer**: *Number*
 - **dewPoint**: *Number*
 - **forecast**: *Number*.
 - **forecastString**: *String*.
 - **humidity**: *Number*
 - **humidityStatus**: *String*
 - **temperature**: *Number*
 - **updateTempHumBaro(temperature, humidity, status, pressure, forecast)**: *Function*. forecast can be domoticz.BARO_NOINFO, BARO_SUNNY, BARO_PARTLY_CLOUDY, BARO_CLOUDY, BARO_RAIN. status can be domoticz.HUM_NORMAL, HUM_COMFORTABLE, HUM_DRY, HUM_WET. Note: temperature must be in Celsius. Use `domoticz.toCelsius()` to convert a Fahrenheit temperature to Celsius.

#### Temperature, Humidity
 - **dewPoint**: *Number*
 - **humidity**: *Number*
 - **humidityStatus**: *String*
 - **temperature**: *Number*
 - **updateTempHum(temperature, humidity, status)**: *Function*. status can be domoticz.HUM_NORMAL, HUM_COMFORTABLE, HUM_DRY, HUM_WET. Note: temperature must be in Celsius. Use `domoticz.toCelsius()` to convert a Fahrenheit temperature to Celsius.

#### Text
 - **text**: *String*
 - **updateText(text)**: Function*.

#### Thermostat set point
- **setPoint**: *Number*.
- **updateSetPoint(setPoint)**:*Function*.

#### UV sensor
 - **uv**: *Number*.
 - **updateUV(uv)**: *Function*.

#### Voltage
 - **voltage**: *Number*.
 - **updateVoltage(voltage)**:Function*.

#### Wind
 - **chill**: *Number*.
 - **direction**: *Number*. Degrees.
 - **directionString**: *String*. Formatted wind direction like N, SE.
 - **gust**: *Number*.
 - **temperature**: *Number*
 - **speed**: *Number*.
 - **updateWind(bearing, direction, speed, gust, temperature, chill)**: *Function*. Note: temperature must be in Celsius. Use `domoticz.toCelsius()` to convert a Fahrenheit temperature to Celsius.

#### Zone heating
 - **setPoint**: *Number*.
 - **heatingMode**: *String*.

### Switch timing options (delay, duration)
To specify a duration or a delay for the various switch command you can do this:

    -- switch on for 2 minutes after 10 seconds
    device.switchOn().afterSec(10).forMin(2)

    -- switch on for 2 minutes after a randomized delay of 1-10 minutes
    device.switchOff().withinMin(10).forMin(2)
    device.close().forMin(15)
    device.open().afterSec(20)
    device.open().afterMin(2)

 - **afterSec(seconds)**: *Function*. Activates the command after a certain amount of seconds.
 - **afterMin(minutes)**: *Function*. Activates the command after a certain amount of minutes.
 - **forMin(minutes)**: *Function*. Activates the command for the duration of a certain amount of minutes (cannot be specified in seconds).
 - **withinMin(minutes)**: *Function*. Activates the command within a certain period *randomly*.

Note that **dimTo()** doesn't support **forMin()**.

### Create your own device adapter
It is possible that your device is not recognized by dzVents. You can still operate it using the generic device attributes and methods but it is much nicer if you have device specific attributes and methods. Take a look at the existing adapters. You can find them in `/path/to/domoticz/scripts/dzVents/runtime/device-adapters`.  Just copy an existing adapter and adapt it to your needs. You can turn debug logging on and inspect the file `domoticzData.lua` in the dzVents folder. There you can see what the unique signature is for your device type. Usually it is a combination of deviceType and deviceSubType. But you can use any of the device attributes in the `matches` function. The matches function checks if the device type is suitable for your adapter and the `process` function actually creates your specific attributes and methods.
Also, if you call `myDevice.dump()` you will see all attributes and methods and the attribute `_adapters` shows you a list of applied adapters for that device.
Finally register your adapter in `Adapters.lua`. Please share your adapter when it is ready and working.

## Variable object API
User variables created in Domoticz have these attributes and methods:

### Variable attributes
 - **changed**: *Boolean*. Was the device changed.
 - **date**: *Date*. If type is domoticz.DATE. See lastUpdate for the sub-attributes.
 - **id**: *Number*. Index of the variable.
 - **lastUpdate**:
	 - **day**: *Number*
	 - **getISO**: *Function*. Returns the ISO 8601 formatted date.
	 - **hour**: *Number*
	 - **hoursAgo**: *Number*. Number of hours since the last update.
	 - **isToday**: *Boolean*. Indicates if the device was updated today
	 - **min**: *Number*
	 - **minutesAgo**: *Number*. Number of minutes since the last update.
	 - **month**: *Number*
	 - **raw**: *String*. Generated by Domoticz
	 - **rawDate**: *String*. Returns the date part of the raw data.
 	 - **rawTime**: *String*. Returns the time part of the raw data.
	 - **sec**: *Number*
	 - **year**: *Number*
 - **name**: *String*. Name of the variable
 - **time**: *Date*. If type is domoticz.TIME. See lastUpdate for the sub-attributes.
 - **type**: *String*. Can be domoticz.INTEGER, domoticz.FLOAT, domoticz.STRING, domoticz.DATE, domoticz.TIME.
 - **value**: *String|Number|Date|Time*. value of the variable.

### Variable methods

 - **set(value)**: Tells Domoticz to update the variable.

## Create your own Time object
dzVents comes with a Time object that may be useful to you. This object is used for the various time attributes like `domoticz.time`, `device.lastUpdate`. You can easily create such an object yourself:

```
    local Time = require('Time')
    local t = Time('2016-12-12 07:35:00') -- must be this format!!
```

You can use this in combination with the various dzVents time attributes:

```
    local Time = require('Time')
    local t = Time('2016-12-12 07:35:00')

	local tonight = Time(domoticz.time.rawDate .. ' 20:00:00')
	print (tonight.getISO())
	-- will print something like: 2016-12-12T20:00:00Z
	print(t.minutesAgo()) -- difference from 'now' in minutes

	-- and you can feed it with all same rules as you use
	-- for the timer = { .. } section:
	if (t.matchesRule('at 16:00-21:00')) then
		-- t is in between 16:00 and 21:00
	end
```

### Time properties and methods:
Creation:
```
local Time = require('Time')
local myTime = Time('yyy-mm-dd hh:mm:ss', [isUTC: Boolean])
```
When `isUTC` is true then the given date is treated as a UTC time.

 - **day**: *Number*
 - **getISO**: *Function*. Returns the ISO 8601 formatted date.
 - **hour**: *Number*
 - **hoursAgo**: *Number*. Number of hours since the last update.
 - **isToday**: *Boolean*. Indicates if the device was updated today
 - **matchesRule(rule) **: *Function*. Returns true if the rule matches with the time. See [time trigger options](#timer_trigger_options) for rule examples.
 - **min**: *Number*
 - **minutesAgo**: *Number*. Number of minutes since the last update.
 - **month**: *Number*
 - **raw**: *String*. Generated by Domoticz
 - **rawDate**: *String*. Returns the date part of the raw data.
 - **rawTime**: *String*. Returns the time part of the raw data.
 - **sec**: *Number*
 - **secondsAgo**: *Number*. Number of seconds since the last update.
 - **utcSystemTime**: *Table*. Current system time (Lua table) in UTC format.
 - **utcTime**: *Table*. Time (Lua table) of the object in UTC format.
 - **year**: *Number*

## Creating shared helper functions
It is not unlikely that at some point you want to share Lua code among your scripts. Normally in Lua you would have to create a module and require that module in all you scripts. But dzVents makes that easier for you:
Inside your scripts folder create a `global_data.lua` script (same as for global persistent data) and feed it with this code:
```
return {
	helpers = {
		myHandyFunction = function(param1, param2)
			-- do your stuff
		end
	}
}
```
Save the file and from then on you can use myHandyFunction everywhere in your event scripts:
```
return {
	...
	execute = function(domoticz, device)
		local results = domoticz.helpers.myHandyFunction('bla', 'boo')
	end
}
```
No requires or dofiles needed.  Simple as that.


# Persistent data

In many situations you need to store some device state or other information in your scripts for later use. Like knowing what the state was of a device the previous time the script was executed or what the temperature in a room was 10 minutes ago. Without dzVents you had to resort to user variables. These are global variables that you create in the Domoticz GUI and that you can access in your scripts like: domoticz.variables('previousTemperature').

Now, for some this is rather inconvenient and they want to control this state information in the event scripts themselves or want to use all of Lua's variable types. dzVents has a solution for that: **persistent script data**. This can either be on the script level or on a global level.

## Script level persistent variables

Persistent script variables are available in your scripts and whatever value put in them is persisted and can be retrieved in the next script run.

Here is an example. Let's say you want to send a notification if some switch has been actived 5 times:
```
    return {
        active = true,
        on = {
			devices = { 'MySwitch' }
    	},
        data = {
    	    counter = {initial=0}
    	},
        execute = function(domoticz, switch)
    		if (domoticz.data.counter = 5) then
    			domoticz.notify('The switch was pressed 5 times!')
    			domoticz.data.counter = 0 -- reset the counter
    		else
    			domoticz.data.counter = domoticz.data.counter + 1
    		end
        end
    }
```
Here you see the `data` section defining a persistent variable called `counter`. It also defines an initial value.  From then on you can read and set the variable in your script.

You can define as many variables as you like and put whatever value in there that you like. It doesn't have to be just a number,  you can even put the entire device state in it:

```
    return {
        active = true,
        on = {
    	    devices = { 'MySwitch' }
    	},
        data = {
    	    previousState = {initial=nil}
    	},
        execute = function(domoticz, switchDevice)
    	    -- set the previousState:
    		domoticz.data.previousState = switchDevice

    		-- read something from the previousState:
    		if (domoticz.data.previousState.temperature > .... ) then
    		end
        end
    }
```
**Note that you cannot call methods on previousState like switchOn(). Only the data is persisted.**

### Size matters and watch your speed!!
If you decide to put tables in the persistent data (or arrays) beware to not let them grow as it will definitely slow down script execution because dzVents has to serialize and deserialize the data back and from the file system. Better is to use the historical option as described below.

## Global persistent variables

Next to script level variables you can also define global variables. As script level variables are only available in the scripts that define them, global variables can be accessed and changed in every script. All you have to do is create a script file called `global_data.lua` in your scripts folder with this content (same file where you can also put your shared helper functions):

```
    return {
	    helpers = {},
    	data = {
    		peopleAtHome = { initial = false },
    		heatingProgramActive = { initial = false }
    	}
    }
```

Just define the variables that you need and access them in your scripts:
```
    return {
        active = true,
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

## A special kind of persistent variables: *history = true*

In some situation, storing a previous value for a sensor is not enough and you would like to have more previous values for example when you want to calculate an average over several readings or see if there was a constant rise or decrease. Of course you can define a persistent variable holding a table:

```
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

The problem with this is that you have to do a lot of bookkeeping yourself to make sure that there isn't too much data to store (see [below how it works](#How_does_the_storage_stuff_work.3F)) and many statistical stuff requires a lot of code. Fortunately, dzVents has done this for you:
```
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

### Historical variables API
#### Defining
You define a script variable or global variable in the data section and set `history = true`:

	..
	data = {
		var1 = { history = true, maxItems = 10, maxHours = 1, maxMinutes = 5 }
	}

 - **maxItems**: *Number*. Controls how many items are stored in the variable. maxItems wins over maxHours and maxMinutes.
 - **maxHours**: *Number*. Data older than `maxHours` from now will be discarded.  So if you set it to 2 than data older than 2 hours will be removed at the beginning of the script.
 - **maxMinutes**: *Number*. Same as maxHours but, you guessed it: for minutes this time..
 All these options can be combined but maxItems wins. **And again: don't store too much data. Just put only in there what you really need!**


#### Add
When you defined your historical variable you can new values to the list like this:

    domoticz.data.myVar.add(value)

As soon as you do that, this new value is put on top of the list and shifts the older values one place down the line. If `maxItems` was reached then the oldest value will be discarded.  *All methods like calculating averages or sums will immediately use this new value!* So, if you don't want this to happen set the new value at the end of your script or after you have done your analysis.

Basically you can put any kind of data in the historical variable. It can be a numbers, strings but also more complex data like tables. However, in order to be able to use the statistical methods you will have to set numeric values or tell dzVents how to get a numeric value from you data. More on that later.

#### Getting. It's all about time!
Getting values from a historical variable is basically done by using an index where 1 is the newest value , 2 is the second to newest and so on:

    local item = domoticz.data.myVar.get(5)
    print(item.data)

However, all data in the storage is time-stamped so getting something from the internal storage will get you this:

	local item = domoticz.data.myVar.getLatest()
	print(item.time.secondsAgo) -- access the time stamp
	print(item.data) -- access the data

The time attribute by itself is a table with many properties that help you inspect the data points more easily:

 - **day**: *Number*.
 - **getISO**: *Function*. Returns the ISO 8601 formatted date.
 - **hour**: *Number*
 - **isToday**: *Boolean*.
 - **month**: *Number*
 - **min**: *Number*
 - **minutesAgo**: *Number*.  How many minutes ago from the current time the data was stored.
 - **raw**: *String*. Formatted time.
 - **rawDate**: *String*. Returns the date part of the raw data.
 - **rawTime**: *String*. Returns the time part of the raw data.
 - **sec**: *Number*
 - **secondsAgo**: *Number*. Number of seconds since the last update.
 - **year**: *Number*
 - **utcSystemTime**: *Table*. UTC system time:
	 - **day**: *Number*
	 - **hour**: *Number*
	 - **month**: *Number*
	 - **min**: *Number*
	 - **sec**: *Number*
	 - **year**: *Number*
 - **utcTime**: *Table*. Time stamp in UTC time:
	 - **day**: *Number*
	 - **hour**: *Number*
	 - **month**: *Number*
	 - **min**: *Number*
	 - **sec**: *Number*
	 - **year**: *Number*


#### Interacting with your data: statistics!
Once you have data points in your historical variable you can interact with it and get all kinds of statistical information from the set. Many of the functions needed for this interaction require one or two indexes or a time specification (time ago):

##### Index
All data in the set can be addressed using an index. The item with index = 1 is the youngest and the item with the highest index is the oldest (and beware, if you have called `add()` first, then the first item is that new value!). You can always check the size of the set by inspecting `myVar.size`.

##### Time specification (*timeAgo*)
Every datapoint in the set has a time stamp and of course the set is always ordered so that the youngest item is the first and the oldest item the last. Many functions require you to specify a moment in the past. You do this by passing a string in this format:

    hh:mm:ss

 Where hh is the amount of hours ago, mm the amount of minutes and ss the amount of seconds. They will all be added together and you don't have to consider 60 minute boundaries etc. So this is a valid time specification:

    12:88:03

Which will point to the data point at or around `12*3600 + 88*60 + 3 = 48.483 seconds` in the past.
Example:

	-- get average since the past 30 minutes:
	local avg = myVar.avgSince('00:30:00')

##### Getting data points

 - **get( [idx] )**: Returns the idx-th item in the set. Same as `myVar.storage[idx]`.
 - **getAtTime( [timeAgo](#Time_specification_.28timeAgo.29) )**: Returns the data point *closest* to the moment as specified by `timeAgo`. So `myVar.getAtTime('1:00:00')` returns the item that is closest to one hour old. So it may be a bit younger or a bit older than 1 hour.
 - **getLatest( ):** Returns the youngest item in the set. Same as `print(myVar.get(1).data)`.
 - **getOldest( )**: Returns the oldest item in the set. Same as `print(myVar.get(myVar.size).data)`.
 - **size**: Return the amount of data points in the set.
 - **subset( [fromIdx], [toIdx] )**: Returns a subset of the stored data. If you omit `fromIdx` then it starts at 1. If you omit `toIdx` then it takes all items until the end of the set (oldest). So `myVar.subset()` returns all data. The result set supports [iterators](#Looping_through_the_data:_iterators) `forEach`, `filter`, `find` and `reduce`.
 - **subsetSince( [[timeAgo](#Time_specification_.28timeAgo.29)] )**: Returns a subset of the stored data since the relative time specified by timeAgo. So calling `myVar.subsetSince('00:60:00')` returns all items that have been added to the list in the past 60 minutes. The result set supports [iterators](#Looping_through_the_data:_iterators) `forEach`, `filter`, `find` and `reduce`.
 - **reset( ):** Removes all the items from the set. Could be handy if you want to start over. It could be a good practice to do this often when you know you don't need older data. For instance when you turn on a heater and you just want to monitor rising temperatures starting from this moment when the heater is activated. If you don't need data points from before, then you may call reset.

###### Looping through the data: iterators
There are a couple of convenience methods to make looping through the data set easier. This is similar to the iterators as described [above](#Iterators):

 - **forEach(function)**: Loop over all items in the set: E.g.: `myVar.forEach( function( item, index, collection) ... end )`
 - **filter(function)**: Create a filtered set of items. The function receives the item and returns true if the item should be in the result set. E.g. get a set with item values larger than 20: `subset = myVar.filter( function (item) return (item.data > 20) end )`.
 - **find(function)**: Search for a specific item in the set: E.g. find the first item with a value higher than 20: `local item = myVar.find( function (item) return (item.data > 20) end )`.
 - **reduce(function, initial)**: Loop over all items in the set and do some calculation with it. You call reduce with the function and the initial value. Each iteration the function is called with the accumulator. The function does something with the accumulator and returns a new value for it. Once you get the hang of it, it is very powerful. Best to give an example. Suppose you want to sum all values:

    	local sum = myVar.reduce(function(acc, item)
			local value = item.data
			return acc + value
		end, 0)

Suppose you want to get data points older than 45 minutes and count the values that are higher than 20 (of course there are more ways to do this):

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

##### Statistical functions
In order to use the statistical functions you have to put *numerical* data in the set. Or you have to provide a function for getting this data. So, if it is just numbers you can just do this:

    myVar.add(myDevice.temperature) -- adds a number to the set
    myVar.avg() -- returns the average

If, however you add more complex data or you want to do a computation first, then you have to tell dzVents how to get to this data. So let's say you do this to add data to the set:

    myVar.add( { 'person' = 'John', waterUsage = u })

Where `u` is some variable that got its value earlier. Now if you want to calculate the average water usage then dzVents will not be able to do this because it doesn't know the value is actually in the `waterUsage` attribute! You will get `nil`.

To make this work you have to provide a **getValue function** in the data section when you define the historical variable:

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

This function tells dzVents how to get the numeric value for a data item. **Note: the `getValue` function has to return a number!**.

Of course, if you don't intend to use any of these statistical functions you can put whatever you want in the set. Even mixed data. No-one cares but you.

###### Functions

 - **avg( [fromIdx], [toIdx], [default] )**: Calculates the average of all item values within the range `fromIdx` to `toIdx`. You can specify a `default` value for when there is no data in the set.
 - **avgSince( [timeAgo](#Time_specification_.28timeAgo.29), default )**: Calculates the average of all data points since `timeAgo`. Returns `default` if there is no data. E.g.: `local avg = myVar.avgSince('00:30:00')` returns the average over the past 30 minutes.
 - **min( [fromIdx], [toIdx] )**: Returns the lowest value in the range defined by fromIdx and toIdx.
 - **minSince( [timeAgo](#Time_specification_.28timeAgo.29) )**: Same as **min** but now within the `timeAgo` interval.
 - **max( [fromIdx], [toIdx] )**: Returns the highest value in the range defined by fromIdx and toIdx.
 - **maxSince( [timeAgo](#Time_specification_.28timeAgo.29) )**: Same as **max** but now within the `timeAgo` interval.
 - **sum( [fromIdx], [toIdx] )**: Returns the summation of all values in the range defined by fromIdx and toIdx.
 - **sumSince( [timeAgo](#Time_specification_.28timeAgo.29) )**: Same as **sum** but now within the `timeAgo` interval.
 - **delta( fromIdx, toIdx, [smoothRange], [default] )**: Returns the delta (difference) between items specified by `fromIdx` and `toIdx`. You have to provide a valid range (no `nil` values). [Supports data smoothing](#About_data_smoothing) when providing a `smoothRange` value. Returns `default` if there is not enough data.
 - **deltaSince( [timeAgo](#Time_specification_.28timeAgo.29),  [smoothRange], [default] )**: Same as **delta** but now within the `timeAgo` interval.
 - **localMin( [smoothRange], default )**: Returns the first minimum value (and the item holding the minimal value) in the past. [Supports data smoothing](#About_data_smoothing) when providing a `smoothRange` value. So if you have this range of values in the data set (from new to old): `10 8 7 5 3 4 5 6`.  Then it will return `3` because older values *and* newer values are higher: a local minimum. You can use this if you want to know at what time a temperature started to rise after have been dropping. E.g.:

		local value, item = myVar.localMin()
		print(' minimum was : ' .. value .. ': ' .. item.time.secondsAgo .. ' seconds ago' )
 - **localMax([smoothRange], default)**: Same as **localMin** but now for the maximum value. [Supports data smoothing](#About_data_smoothing) when providing a `smoothRange` value.
 - **smoothItem(itemIdx, [smoothRange])**: Returns a the value of `itemIdx` in the set but smoothed by averaging with its neighbors. The amount of neighbors is set by `smoothRange`. See [About data smoothing](#About_data_smoothing).

##### About data smoothing
Suppose you store temperatures in the historical variable. These temperatures my have extremes. Sometimes these extremes could be due to sensor reading errors. In order to reduce the effect of these so called spikes, you could smooth out values. It is like blurring the data. Here is an example. The Raw column could be your temperatures. The other columns is calculated by averaging the neighbors. So for item 10 the calucations are:

    Range=1 for t10 = (25 + 31 + 29) / 3 = 28,3
    Range=2 for t10 = (16 + 25 + 31 + 29 + 26) / 5 = 25,4

| Time | Raw | Range=1 | Range=2 |
|------|-----|---------|---------|
| 1    | 18  | 20,0    | 21,7    |
| 2    | 22  | 21,7    | 25,0    |
| 3    | 25  | 27,3    | 24,6    |
| 4    | 35  | 27,7    | 26,6    |
| 5    | 23  | 28,7    | 27,6    |
| 6    | 28  | 26,0    | 25,8    |
| 7    | 27  | 23,7    | 23,8    |
| 8    | 16  | 22,7    | 25,4    |
| 9    | 25  | 24,0    | 25,6    |
| 10   | 31  | 28,3    | 25,4    |
| 11   | 29  | 28,7    | 29,2    |
| 12   | 26  | 30,0    | 30,2    |
| 13   | 35  | 30,3    | 30,2    |
| 14   | 30  | 32,0    | 30,4    |
| 15   | 31  | 30,3    | 30,8    |
| 16   | 30  | 29,7    | 28,2    |
| 17   | 28  | 26,7    | 28,6    |
| 18   | 22  | 27,3    | 26,2    |
| 19   | 32  | 24,3    | 25,3    |
| 20   | 19  | 25,5    | 24,3    |

If you make a chart you can make it even more visible. The red line is not smoothed and clearly has more spikes than the others:

[[File:dzvents-smoothing.png|frame|none|alt=|Smoothing]]

So, some of the statistical function allow you to provide a smoothing range. Usually a range of 1 or 2 is sufficient. I suggest to use smoothing when checking detlas, local minumums and maximums.


## How does the storage stuff work?
For every script file that defines persisted variables (using the `data={ .. }` section) dzVents will create storage file inside a subfolder called `data` with the name `__data_scriptname.lua`. You can always delete these data files or the entire storage folder if there is a problem with it:

    domoticz/
    	scripts/
			dzVents/
				data/
					__data_yourscript1.lua
					__data_yourscript2.lua
					__data_global_data.lua
				documentation/
				examples/
				generated_scripts/
				runtime/
				scripts/
					yourscript1.lua
					yourscript2.lua
					global_data.lua


If you dare to you can watch inside these files. Every time some data is changed, dzVents will stream the changes back into the data files.
**And again: make sure you don't put too much stuff in your persisted data as it may slows things down too much.**

# Settings

There are a couple of settings for dzVents. They can be found in Domoticz GUI: **Setup > Settings > Other > EventSystem**:

 - **dzVents disabled**: Tick this if you don't want any dzVents script to be executed.
 - **Log level**: Note that you can override this setting in the logging section of your script:
    - *Errors*,
    - *Errors + minimal execution info*: Errors and some minimal information about which script is being executed and why,
    - *Errors + minimal execution info + generic info*. Even more information about script execution and a bit more log formatting.
    - *Debug*. Shows everything and dzVents will create a file `domoticzData.lua` in the dzVents folder. This is a dump of all the data that is received by dzVents from Domoticz.. That data is used to create the entire dzVents data structure.
    - *No logging*. As silent as possible.

# Migrating from version 1.x.x
As you can read in the change log below there are a couple of changes in 2.0 that will break older scrtips.

## The 'on={..}' section.
The on-section needs the items to be grouped based on their type. So prior to 2.0 you had
```
	on = {
		'myDevice',
		'anotherDevice'
	}
```
In 2.x you have:
```
	on = {
		devices = {
			'myDevice',
			'anotherDevice'
		}
	}
```
The same for timer options, in 1.x.x:
```
	on = {
		['timer'] = 'every 10 minutes on mon,tue'
	}
```
2.x:
```
	on = {
		timer = {
			'every 10 minutes on mon,tue'
		}
	}
```
Or when you have a combination, in 1.x.x
```
	on = {
			'myDevice',
			['timer'] = 'every 10 minutes on mon,tue'
		}
	}

```
2.x:
```
	on = {
		devices = {
			'myDevice'
		}
		timer = {
			'every 10 minutes on mon,tue'
		}
	}

```
## Getting devices, groups, scenes etc.
Prior to 2.x you did this to get a device:
```
	domoticz.devices['myDevice']
	domoticz.groups['myGroup']
	domoticz.scenes['myScene']
	domoticz.variables['myVariable']
	domoticz.changedDevices['myDevices']
	domoticz.changeVariables['myVariable']
```
Change that to:
```
	domoticz.devices('myDevice') -- a function call
	domoticz.groups('myGroup')
	domoticz.scenes('myScene')
	domoticz.variables('myVariable')
	domoticz.changedDevices('myDevices')
	domoticz.changeVariables('myVariable')
```
## Looping through the devices (and other dzVents collections), iterators
Earlier you could do this:
```
	for i, device in pairs(domoticz.devices) do
		domoticz.log(device.name)
	end
```
In 2.x that is no longer possible. You now have to do this:
```
	domoticz.devices().forEach(function(device)
		domoticz.log(device.name)
	end)
```
The same applies for the other collections like groups, scenes, variables, changedDevices and changedVariables.
Note that you can easily search for a device using iterators as well:
```
	local myDevice = domoticz.devices().find(function(device)
		return device.name == 'deviceImLookingFor'
	end)
```
For more information about these iterators see: [Iterators](#Iterators).

## Timed commands
Prior to 2.0 you did this if you wanted to turn a switch off after a couple seconds:
```
	domoticz.devices['mySwitch'].switchOff().after_sec(10)
```
In 2.x:
```
	domoticz.devices('mySwitch').switchOff().afterSec(10)
```
The same applies for for_min and with_min.

## Device attributes
One thing you have to check is that some device attributes are no longer formatted strings with units in there like WhToday. It is possible that you have some scripts that deal with strings instead of values.

## Changed attributes
In 1.x.x you had a changedAttributes collection on a device. That is no longer there. Just remove it and just check for the device to be changed instead.

## Using rawData
Prior to 2.x you likely used the rawData attribute to get to certain device values. With 2.x this is likely not needed anymore. Check the various device types above and check if there isn't a named attribute for you device type. dzVents 2.x uses so called device adapters which should take care of interpreting raw values and put them in named attributes for you. If you miss your device you can file a ticket or create an adapter yourself. See ![Create your own device adapter](#Create_your_own_device_adapter).

## What happened to fetch http data?
In 2.x it is no longer needed to make timed json calls to Domoticz to get extra device information into your scripts. Very handy.
On the other hand, you have to make sure that dzVents can access the json without the need for a password because some commands are issued using json calls by dzVents. Make sure that in Domoticz settings under **Local Networks (no username/password)** you add `127.0.0.1` and you're good to go.

# Change log
[2.0.1] Domoticz integration
 - Added support for switching Lighting Limitless/Applamp (Hue etc) devices.
 - Added toCelsius() helper method to domoticz object as the various update temperature methods all need celsius.

[2.0.0] Domoticz integration

 - Almost a complete rewrite.
 - **BREAKING CHANGE**: Accessing a device, scene, group, variable, changedDevice, or changedVariable has been changed: instead of doing `domoticz.devices['myDevice']` you now have to call a function: `domoticz.devices('myDevice')`. This applies also for the other collections: `domoticz.scenes(), domoticz.groups(), domoticz.changedDevices(), domoticz.changedVariables()`. If you want to loop over these collection **you can no longer use the standard Lua for..pairs or for..ipairs construct**. You have to use the iterators like forEach, filter and reduce: `domoticz.devices().forEach(function() .. end)` (see [Iterators](#Iterators)). This was a necessary change to make dzVents a whole lot faster in processing your event scripts. **So please change your existing dzVents scripts!**
 - **BREAKING CHANGE**: after_sec, for_min, after_min, within_min methods have been renamed to the camel-cased variants afterSec, forMin, afterMin, withinMin. Please rename the calls in your script.
 - **BREAKING CHANGE**: There is no longer an option to check if an attribute was changed as this was quite useless. The device has a changed flag. You can use that. Please change your existing scripts.
 - **BREAKING CHANGE**: Many device attributes are now in the appropriate type (Number instead of Strings) so you can easily make calculations with them. Units are stripped from the values as much as possible. **Check your scripts as this may break stuff.**
 - **BREAKING CHANGE**: on-section now requires subsections for `devices`, `timer`, `variables`, and `security`. The old way no longer works! Please convert your existing scripts!
 - **BREAKING CHANGE**: Make sure that in Domoticz settings under **Local Networks (no username/password)** you add `127.0.0.1` so dzVents can use Domoticz api when needed.
 - dzVents now works on Windows machines!
 - dzVents is no longer a separate library that you have to get from GitHub. All integrated into Domoticz.
 - Added option to create shared utility/helper functions and have them available in all your scripts. Simply add a `helpers = { myFunction = function() ... end }` to the `global_data.lua` file in your scripts folder and you can access the function everywhere: `domoticz.helpers.myFunction()`.
 - Created a pluggable system for device adapters so people can easily contribute by creating specific adapters for specific devices. An adapter makes sure that you get the proper methods and attributes for that device. See `/path/to/domoticz/scripts/dzVents/runtime/device-adapters`.
 - Added a `reduce()` iterator to the collections in the domoticz object so you can now easily collect data about all your devices. See  [Iterators](#Iterators).
 - Added a `find()` iterator so you can easily find an item in one of the collection (devices, scenes, groups etc.). See  [Iterators](#Iterators).
 - Variables (uservariables) have more attributes. The `value` is now the same type as it is defined in Domoticz. So no more need for a converted nValue attribute. You can inspect the type using `myVar.type`. If it is a time variable or date variable you an extra `date` or `time` attribute with the same methods as with all other date/time related attributes like `lastUpdate`. .E.g. `myVar.date.minutesAgo`.
 - Settings are now moved to the Domoticz GUI (**Setup > Settings > Other**) and no longer in a settings file.
 - You can now override the log settings per script. So you can turn-off logging globally (see log level in the settings) and still have debug-level logging for that one script you are working on. You can even add a prefix string to the log messages for easy filtering in the Domoticz log. See the documentation about the `logging = { .. }` section.
 - No more need to do http-calls to get extended data from Domoticz. All relevant internal Domoticz state-data is now available inside your dzVents scripts. Thanks Scotty!!
 - Support for many many more device types and their specific methods. See the documentation for the list of attributes and events that are available. Note that device-type specific attributes are only available for those type of devices. You will receive an error in the log if you try to access an attribute that doesn't exist for that particular device. Hopefully you don't have to use the rawData table anymore. If you still do please file a report or create a device adapter yourself.
 - You can now write dzVents scripts using the internal editor in Domoticz. These scripts will be automatically exported to the filesystem (one-way only) so dzVents can execute them (generated_scripts folder).  Thanks again Scotty!
 - Support for variable change events (`on = { variables = { 'varA', 'varB'} }`)
 - Support for security change events (`on = { security = { domoticz.SECURITY_ARMEDAWAY } }`)
 - The triggerInfo passed to the execute function now includes information about which security state triggered the script if it was a security event.
 - Extended the timer-rule with time range e.g. `at 16:45-21:00` and `at nighttime` and `at daytime` and you can provide a custom function. See documentation for examples. The timer rules can be combined as well.
 - Timer rules for `every xx minutes` or `every xx hours` are now limited to intervals that will reach *:00 minutes or hours. So for minutes you can only do these intervals: 1, 2, 3, 4, 5, 6, 10, 12, 15, 20 and 30. Likewise for hours.
 - The Time object (e.g. domoticz.time) now has a method `matchesRule(rule)`. `rule` is a string same as you use for timer options: `if (domoticz.time.matchesRule('at 16:32-21:33 on mon,tue,wed')) then ... end`. The rule matches if the current system time matches with the rule.
 - A device trigger can have a time-rule constraint: ` on = { devices = { ['myDevice'] = 'at nighttime' } }`. This only triggers the script when myDevice was changed **and** the time is after sunset and before sunrise.
 - Add support for subsystem selection for domoticz.notify function.
 - Fixed a bug where a new persistent variable wasn't picked up when that variable was added to an already existing data section.

[1.1.2]

 - More robust way of updating devices.lua
 - Added device level information for non-dimmer-like devices

[1.1.1]

 - Added support for a devices table in the 'on' section.
 - Added extra log level for only showing information about the execution of a script module.
 - Added example script for System-alive checker notifications (preventing false negatives).

[1.1]

 - Added example script for controlling the temperature in a room with hysteresis control.
 - Fixed updateLux (thanks to neutrino)
 - Added Kodi commands to the device methods.
 - Fixed updateCounter
 - Added counterToday and counterTotal attributes for counter devices. Only available when http fetching is enabled.

[1.0.2]

 - Added device description attribute.
 - Added support for setting the setpoint for opentherm gateway.
 - Added timedOut boolean attribute to devices. Requires http data fetching to be anabled.
 - Properly detects usage devices and their Wattage.

[1.0.1]

 - Added updateCustomSensor(value) method.
 - Fixed reset() for historical data.

[1.0][1.0-beta2]

 - Deprecated setNew(). Use add() instead. You can now add multiple values at once in a script by calling multiple add()s in succession.
 - Fixed printing device logs when a value was boolean or nil
 - Fixed WActual/total/today for energy devices.
 - Added updateSetPoint method on devices for dummy thermostat devices and EvoHome setpoint devices
 - Added couple of helper properties on Time object. See README.
 - Renamed the file dzVents_settings.lua to dzVents_settings_example.lua so you don't overwrite your settings when you copy over a new version of dzVents to your system.

[1.0-beta1]

 - Added data persistence for scripts between script runs (see readme for more info)
 - Added a time-line based data type for you scripts with historical information and many statistical functions for retreiving information like average, minumum, maximum, delta, data smoothing (averaging values over neighbours) etc. See readme for more information.
 - Added SMS method to the domoticz object.
 - Added toggleSwitch() method to devices that support it.
 - Added more switch states that control device.bState (e.g. on == true, open == true 'all on' == true)
 - Added secondsAgo to the lastUpdate attribute
 - Added tests (test code coverage is above 96%!)
 - Refactored code significantly.
 - Made sure differently formulated but equal triggers in one script only execute the script only once (like MySensor and MySensor_Temperature).
 - Added trigger info as a third parameter (Lua table) that is passed to the execute method of a script containing information about what exactly triggered the script (type = EVENT_TYPE_TIMER/EVENT_TYPE_DEVICE, trigger=<timer rule>). See readme.
 - Added Lua time properties to domoticz.time property with all information about the current time (hours, minutes, seconds, etc.)
 - Added option to return false in a forEach iteratee function which will abort the forEach loop.
 - All devices not delivered by Domoticz to the event scripts are now added to domoticz.devices using the http data that is fetched every 30 minutes (by default).
 - Added scenes and groups collections to the domoticz object
 - Added Quick Reference Guide.

[0.9.13]

 - Fixed some timer functions

[0.9.12]

 - Fixed a bug with log level printing. Now errors are printed.
 - Added setPoint, heatingMode, lux, WhTotal, WhToday, WActual attributes to devices that support it. No need to access rawData for these anymore.

[0.9.11]

 - Added log method to domoticz object. Using this to log message in the Domoticz log will respect the log level setting in the settings file. [dannybloe]
 - Updated readme. Better overview, more attributes described.
 - Added iterator functions (forEach and filter) to domoticz.devices, domoticz.changedDevices and domoticz.variables to iterate or filter more easily over these collections.
 - Added a couple of example scripts.

[0.9.10]

 - A little less verbose debug logging. Domoticz seems not to print all message in the log. If there are too many they may get lost. [dannybloe]
 - Added method fetchHttpDomoticzData to domoticz object so you can manually trigger getting device information from Domoticz through http. [dannybloe]
 - Added support for sounds in domiticz.notify(). [WebStarVenlo]

[0.9.9]

 - Fixed a bug where every trigger name was treated as a wild-carded name. Oopsidayzy...

[0.9.8]

 - Fixed a bug where a device can have underscores in its name.
 - Added dimTo(percentage) method to control dimmers.
 - Modified the switch-like commands to allow for timing options: .for_min(), .within_min(), after_sec() and after_min() methods to control delays and durations. Removed the options argument to the switch functions. See readme.
 - Added switchSelector method on a device to set a selector switch.
 - Added wild-card option for triggers
 - Added http request data from Domoticz to devices. Now you can check the battery level and switch type and more. Make sure to edit dzVents_settings.lua file first and check the readme for install instructions!!!
 - Added log level setting in dzVents_settings.lua

[0.9.7]

 - Added domoticz object resource structure. Updated readme accordingly. No more (or hardly any) need for juggling with all the Domoticz Lua tables and commandArrays.