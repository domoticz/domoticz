[3.0.2]
- Add `PUT` and `DELETE` support to `openURL`
- Ensure sending integer in nValue in update function
- Fix sValue for custom sensor

[3.0.1] 
- Add option 'at' to the various commands/methods
 
[3.0.0] 
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
	 - MQTT: {"command" : "customevent", "event" :    "MyEvent" , "data" : "myData" } ( data = opt ional )
 - Add method  domoticz.emitEvent()
 - Add attribute `mode` to Evohome controller
 - Add option to dumpTable() and ([device][uservariable][scene][group].dump() to os file

[2.5.7]
- Add option checkFirst to switchSelector method

[2.5.6]
- Add dayName, monthName, monthAbbrName and time as time attributes
- Add _.round, _.isEqual functions to lodash.lua
- Add testLodash.lua as testModule for lodash 

[2.5.5]
- Add Zwave fan to Zwave mode device adapter

[2.5.4]
- Add minutesSinceMidnight to domoticz Time object
- Add domoticz.time.addSeconds(), -.addMinutes(), -.addHours(), -.addDays() , -.makeTime()
- Add string.sMatch to utils
- Made wildcard handling more resilient when magic chars are part of script triggers

[2.5.3] 
- Add timealert / errors for long running scripts
- Add triggerHTTPResponse()

[2.5.2]
- Add actualWatt to replace WhActual (left in WhActual for compatibility reasons)
- Add toBase64 and fromBase64 function in utils
- Add setLogMarker function in utils
- Deprecated increaseBrightness(), decreaseBrightness(), discomode methods (only available for Yeelight and when used leave the device stateless)

[2.5.1]
- Added `toXML` and `fromXML` methods to domoticz.utils.
- Add attributes isXML, xmlVersion, xmlEncoding

[2.5.0]
- Prepared for Lua 5.3

[2.4.29]
- Add error message including affected module when module got corrupted on disk.
- Add setLevel method for switchTypes.
- Increased resilience against badly formatted type Time user-variables.
- Use native domoticz command for increaseCounter method.
- Set inverse of "set color" to Off to enable use of toggleSwitch for RGB type of devices.

[2.4.28]
- Add deviceExists(), groupExists(), sceneExists(), variableExists(), cameraExists() methods
- increased httpResponse resilience against different use of Upper-, Lowercase in headers['content-type'] to ensure JSON conversion to Lua table

[2.4.27]

- Add protect attribute for devices / scenes and groups
- Add methods protectOn and protectOff for devices / scenes and groups
- Add functions rightPad, leftPad, centerPad, leadingZeros, numDecimals in utils

[2.4.26]
- Add Smoke detector

[2.4.25]
- Add rawDateTime
- fix for combined device / civil[day|night]time trigger rule 
- fix for checkFirst on stopped status

[2.4.24]
- Add method rename for devices, user-variables , scenes and groups

[2.4.23]
- Add method setMode for evohome device
- Add method incrementCounter for incremental counter
- Prepared for Firebase notifications. Firebase (fcm) is the replacement for Google Cloud Messaging gcm)
- fix wildcard device 

[2.4.22]
- selector.switchSelector method accepts levelNames
- increased selector.switchSelector resilience
- fix wildcard timerule 

[2.4.21]
- fixed wrong direction for open() and close() for some types of blinds
- Add inTable function to domoticz.utils
- Add sValue attribute to devices

[2.4.20]
- Add quietOn() and quietOff() method to switchType devices 

[2.4.19]
- Add stringSplit function to domoticz.utils.
- Add statusText and protocol to HTTPResponse

[2.4.18]
- Add triggerIFTTT()

[2.4.17]
- Add function dumpTable() to domoticz.utils
- Add setValues() method to devices 
- Add setIcon() method for devices

[2.4.16]
- Add option dump() to domoticz.settings
- Add domoticz.hsbToRGB method for converting hue to rgb
- Add setHue, setColor, setHex, getColor for RGBW(W) devices
- Add setDescription for devices, groups and scenes
- Add volumeUp / volumeDown for Logitech Media Server (LMS)
- Changed domoticz.utils.fromJSON (add optional fallback param) 

[2.4.15] 
- Add option to use camera name in snapshot command
- Add domoticz.settings.domoticzVersion
- Add domoticz.settings.dzVentsVersion

[2.4.14]
- Added domoticz.settings.location.longitude and domoticz.settings.location.latitude 
- Added check for- and message when call to openURL cannot open local (127.0.0.1)
- **BREAKING CHANGE** :Changed domoticz.settings.location to domoticz.settings.location.name (domoticz settings location Name) 
- prevent call to updateCounter with table

[2.4.13]
- Added domoticz.settings.location (domoticz settings location Name) 
- Added domoticz.urlDecode method to convert an urlEncoded string to human readable format

[2.4.12]
- Added Managed counter (to counter)

[2.4.11]
- Added snapshot command to send Email with camera snapshot ( afterXXX() and withinXXX() options available)

[2.4.10]
- Added option to use afterXXX() and withinXXX() functions to updateSetPoint()
- Changed parm from integer to string in call to device.update in function updateMode of device adapter zwave_thermostat_mode_device

[2.4.9]
- Added evohome hotwater device (state, mode, untilDate and setHotWater function)
- Added mode and untilDate for evohome zone devices 
- Added EVOHOME_MODE_FOLLOW_SCHEDULE as mode for evohome devices
- Add speedMs and gustMs from wind devices 
- bugfix for youless device (0 handling)
- bugfix for time ( twilightstart and twilightend handling)
- Added tests for twilight and device functions (hotwater) and attributes (evohome- and wind devices)
- Fixed some date-range rule checking
- Fixed integration tests by changing API call for creating a domoticz variable (saveuservariable was changed to adduservariable)
- Fixed combined timer where one part of the combination is even or odd week. 
- Added tests for combined timer with even/odd week 

[2.4.8]
- Added telegram as option for domoticz.notify

[2.4.7]
- Added support for civil twilight in rules

[2.4.6]
- Added Youless device
- Added more to the documentation section for http requests
- Made sure global_data is the first module to process. This fixes some unexpected issues if you need some globals initialized before the other scripts are loaded.

[2.4.5]
- Fixed a bug in date ranges for timer triggers (http://domoticz.com/forum/viewtopic.php?f=59&t=23109).

[2.4.4]
- Fixed rawTime and rawData so it shows leading zeros when values are below 10.
- Fixed one wildcard issue. Should now work as expected.
- Fixed a problem in Domoticz where you couldn't change the state of some door contact-like switches using the API or dzVents. That seems to work now.

[2.4.3]
- Fixed trigger wildcards. Now you can do `*aa*bb*cc` or `a*` which will require the target to start with an `a`
- Added more EvoHome device types to the EvoHome device adapter.

[2.4.2]
- Fixed RGBW device adapter
- Fixed EvoHome device adapter
- Changed param ordering opentherm gateway command (https://www.domoticz.com/forum/viewtopic.php?f=59&t=21620&p=170469#p170469)

[2.4.1]
- Fixed week number problems on Windows
- Fixed 'on date' rules to support dd/mm format (e.g. 01/02)

[2.4.0]
- **BREAKING CHANGE**: The second parameter passed to the execute function is no longer `nil` when the script was triggered by a timer or a security event. Please check your scripts. The second parameter has checks to determine the type. E.g. `execute = function(domoticz, item) .. end`. You can inspect `item` using: `item.isDevice`, `item.isTimer`, `item.isVariable`, `item.isScene`, `item.isGroup`, `item.isSecurity`, `item.isHTTPResponse`. Please read the documentation about the execute function.
- Added ``.cancelQueuedCommands()`` to devices, groups, scenes and variables. Calling this method will cancel any scheduled future commands issued using for instance `.afterMin(10)` or `.repeatAfterMin(1, 4)`
- Added `.devices()` collection to scenes and groups to iterate (`forEach`, `filter`, `reduce`, `find`) over the associated devices.
- Added http response event triggers to be used in combination with `openURL`. You can now do `GET` and `POST` request and handle the response in your dzVents scripts **ASYNCHRONICALLY**. See the documentation. No more json parsing needed or complex `curl` shizzle.
- Added a more consistent second parameter sent to the execute function. When a timer is triggered then the second parameter is a Timer object instead of nil. This way you can check the baseType of the second parameter and makes third parameter (triggerInfo) kind of obsolete. Every object bound to the second parameter now has a baseType.
- Added locked/unlocked support for door-locks (locked == active).
- Moved utility functions from the domoticz object to `domoticz.utils` object. You will see a deprecation warning when using the old function like `round()`, `toCelsius()` etc.
- Added `lodash` as a method to `domoticz.utils`: `domoticz.utils._`
- Added `toJSON` and `fromJSON` methods to domoticz.utils.
- Added `afterXXX()` and `withinXXX()` support for device-update commands. E.g.: `myTextDevice.updateText('Zork').afterMin(2)`.
- Added support for Logitech Media Server devices (thanks to Eoreh).
- Added new timer rules: date rules: `on 13/07`, `on */03`, `on 12/*`, `on 12/04-22/09`, `on -24/03`, `on 19/11-`, week rules: `in week 12,15,19-23,-48,53-`, `every even week`, `every odd week`. See documentation.
- Added historical data helper `delta2(fromIndex, toIndex, smoothRangeFrom, smoothRangeTo, default)` to have a bit more control over smoothing. You can specify if want to smooth either the start value (reference) and/or the to value (compared value).
- Added historical data helper `deltaSinceOrOldest(timeAgo, smoothRangeFrom, smoothRangeTo, default)`. This will use the oldest data value when the data set is shorter than timeAgo.
- Added support for Lighting Limitless/Applamp RGBW devices. You can now set Kelvin and RGB values, NightMode, WhiteMode and increase and decrease the brightness and discoMode. See the documentation.
- Added device adapter for Onkyo receiver hardware.
- Added `scriptName` to the triggerInfo object passed as the third parameter to the execute function. This holds the name of the script being executed.
- Fixed bug in Domoticz where using forXXX() with selector switches didn't always work.
- Fixed bug in Domoticz where improper states were passed to the event scripts. This may happen on slower machines where several devices may have been updated before the event-system had a change to operate on them. In that case the event scripts received the current final state instead of the state at the moment of the actual event.
- Added support for webroot. dzVents will now use the proper API url when domoticz is started with the -webroot switch.
- Added support for event-bursts. If (on slower machines) events get queued up in Domoticz, they will be sent to dzVents in one-package. This makes event processing significantly faster.
- Added `domoticz.data.initialize(varName)`. This will re-initialize non-historical variables to the initial value as defined in the data section.
- You now no longer have to provide an initial value for a persistent variable when you declare it. So you can do `data = { 'a', 'b', 'c'}` instead of `data = {a={initial = nil}, b={initial=nil}, c={initial=nil} }`. You have to quote the names though.
- A filter can now accept a table with names/id's instead of only functions. You can now do `domoticz.devices().filter({ 'mySwitch', 'myPIR', 34, 35, 'livingLights'})` which will give you a collection of devices with these names and ids.
- Added and documented `domoticz.settings.url`, `domoticz.settings.webRoot` and `domoticz.settings.serverPort`.
- Removed fixed limit on historical variables if there is a limit specified.

[2.3.0]

 - Added `active` attribute to devices (more logical naming than the attribute 'bState'). `myDevice.active` is true or false depending on a set of known state values (like On, Off, Open, Closed etc). Use like `if mySwitch.active then .. end`
 - Added `domoticz.urlEncode` method on the `domoticz` object so you can prepare a string before using it with `openURL()`.
 - Updating devices, user variables, scenes and groups now always trigger the event system for follow-up events.
 - Added support for groups and scenes change-event scripts. Use `on = { scenes = { 'myScene1', 'myScene2' }, groups = {'myGroup1'} }`
 - Added adapter for the new Temperature+Barometer device.
 - Added `domoticz.startTime` giving you the time at which the Domoticz service was started. Returns a Time object.
 - Added `domoticz.systemUptime` (in seconds) indicating the number of seconds the machine is running. Returns a Time object.
 - Added more options to the various commands/methods: e.g. `myDevice.switchOff().silent()`, `.forSec()`, `.forHour()`, `.afterHour()`, `.repeatAfterSec()`, `.repeatAfterMin()`, `.repeatAfterHour()`, `.withinHour()` and `.checkFirst()`. This now works not only for devices but also scenes, groups and user variables. See documentation about command options.
 - Added `.silent()` option to commands like `switchOn().afterSec(4).silent()` causing no follow-up events to be triggered. This also works when updating non-switch-like devices, scenes, groups and user variables. If you do not call `silent()``, a follow-up event is *always* triggered (for devices, scenes, groups and user variables).
 - Added time rule `between xx and yy`. You can now do things like: `between 16:34 and sunset` or `between 10 minutes before sunset and sunrise`. See the doc.
 - Added support for milliseconds in `Time` object. This also give a ms part in the time-stamp for historical persistent data. You can now do `msAgo()` on time stamp when you inspect a data point from an history variable.
 - Added `daysAgo()` to `Time` object.
 - Added `compare(time)` method to `Time` object to calculate the difference between them. Returns a table. See documentation.
 - Added `domoticz.round()` method to `domoticz` object.
 - Added `text` property to alert sensor.
 - `active` section is now optional in your dzVents script. If you don't specify an `active = true/false` then true is assumed (script is active). Handy for when you use Domoticz' GUI script editor as it has its own way of activating and deactivating scripts.
 - Added `humidityStatusValue` for humidity devices. This value matches with the values used for setting the humidity status.
 - `Time` object will initialize to current time if nothing is passed: `local current = Time()`.
 - Added the lua Lodash library (http://axmat.github.io/lodash.lua, MIT license).
 - Fixed documentation about levelNames for selector switches and added the missing levelName.
 - Moved dzVents runtime code away from the `/path/to/domoticz/scripts/dzVents` folder as this scripts folder contains user stuff.
 - Added more trigger examples in the documentation.
 - You can now put your own non-dzVents modules in your scripts folder or write them with the Domoticz GUI editor. dzVents is no longer bothered by it. You can require your modules in Lua's standard way.
 - Added `/path/to/domoticz/scripts/dzVents/scripts/modules` and `/path/to/domoticz/scripts/dzVents/modules` to the Lua package path for custom modules. You can now place your modules in there and require them from anywhere in your scripts.
 - Added dzVents-specific boilerplate/templates for internal web editor.
 - Fixed the confusing setting for enabling/disabling dzVents event system in Domoticz settings.
 - Fixed a problem where if you have two scripts for a device and one script uses the name and the other uses the id as trigger, the id-based script wasn't executed.

[2.2.0]

 - Fixed typo in the doc WActual > WhActual.
 - Updated switch adapter to match more switch-like devices.
 - Added Z-Wave Thermostat mode device adapter.
 - Fixed a problem with thermostat setpoint devices to issue the proper url when updating.
 - Added secondsSinceMidnight to time attributes (e.g. lastUpdate.secondsSinceMidnight)
 - Added 4 new time-rules: xx minutes before/after sunset/sunrise.
 - Added example script to fake user presence.
 - Fixed support for uservariables with spaces in their names.
 - Show a warning when an item's name isn't unique in the collection.
 - Added timing options for security methods armAway, armHome and disarm like device.armAway().afterSec(10).
 - Added idx, deviceId and unit to device objects. Don't confuse deviceId with device.id(x) which is actually the index of the device.
 - Added instructions on how to create a security panel device in Domoticz. This device now has a state that has the same value as domoticz.security.
 - Fixed bug in check battery levels example.
 - Fixed some irregularities with dimmer levels.

[2.1.0]

 - Added support for switching RGB(W) devices (including Philips/Hue) to have toggleSwitch(), switchOn() and switchOff() and a proper level attribute.
 - Added support for Ampère 1 and 3-phase devices
 - Added support for leaf wetness devices
 - Added support for scale weight devices
 - Added support for soil moisture devices
 - Added support for sound level devices
 - Added support for visibility devices
 - Added support for waterflow devices
 - Added missing color attribute to alert sensor devices
 - Added updateEnergy() to electric us./dage devices
 - Fixed casing for WhTotal, WhActual methods on kWh devices (Watt's in a name?)
 - Added toCelsius() helper method to domoticz object as the various update temperature methods all need celsius.
 - Added lastLevel for dimmers so you can see the level of the dimmer just before it was switched off (and while is it still on).
 - Added integration tests for full round-trip Domoticz > dzVents > Domoticz > dzVents tests (100 tests). Total tests (unit+integration) now counts 395!
 - Fixed setting uservariables. It still uses json calls to update the variable in Domoticz otherwise you won't get uservariable event scripts triggered in dzVents.
 - Added dzVents version information in the Domoticz settings page for easy checking what dzVents version is being used in your Domoticz built. Eventhough it is integrated with Domoticz, it is handy for dzVents to have it's own heartbeat.
 - avg(), avgSince(), sum() and sumSince() now return 0 instead of nil for empty history sets. Makes more sense.
 - Fixed boiler example to fallback to the current temperature when there is no history data yet when it calculates the average temperature.
 - Use different api command for setting setPoints in the Thermostat setpoint device adapter.

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
