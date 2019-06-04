**Note**: This document is maintained on [github](https://github.com/domoticz/domoticz/blob/development/dzVents/documentation/README.md), and the wiki version is automatically generated. Edits should be performed on github, or they may be suggested on the wiki article's [Discussion page](https://www.domoticz.com/wiki/Talk:DzVents:_next_generation_LUA_scripting).

**Breaking change warning!!**: For people using with dzVents prior to version 2.4: Please read the [change log](#Change_log) below as there is an easy-to-fix breaking change regarding the second parameter passed to the execute function (it is no longer `nil` for timer/security triggers).

Documentation for dzVents 2.3.0 (Domoticz v3.5876) can be found [here](https://github.com/domoticz/domoticz/blob/2f6ba5c5a8978a010d6867228ad84eab762c5936/dzVents/documentation/README.md).

Documentation for dzVents 2.2.0 (Domoticz v3.8153) can be found [here](https://github.com/domoticz/domoticz/blob/9f75e45f994f87c8d8ce9cb39eaab85886df0be4/scripts/dzVents/documentation/README.md).

# About dzVents 2.4.x (Domoticz v3.8837+)
dzVents /diː ziː vɛnts/, short for Domoticz Easy Events, brings Lua scripting in Domoticz to a whole new level. Writing scripts for Domoticz has never been so easy. Not only can you define triggers more easily, and have full control over timer-based scripts with extensive scheduling support, dzVents presents you with an easy to use API to all necessary information in Domoticz. No longer do you have to combine all kinds of information given to you by Domoticz in many different data tables. You don't have to construct complex commandArrays anymore. dzVents encapsulates all the Domoticz peculiarities regarding controlling and querying your devices. And on top of that, script performance has increased a lot if you have many scripts because Domoticz will fetch all device information only once for all your device scripts and timer scripts. And ... **it is 100% Lua**! So if you already have a bunch of event scripts for Domoticz, upgrading should be fairly easy.

Let's start with an example. Say you have a switch that when activated, it should activate another switch but only if the room temperature is above a certain level. And when done, it should send a notification. This is how it looks in dzVents:

    return {
       on = {
          devices = { 'Room switch'}
       },
       execute = function(domoticz, roomSwitch)
          if (roomSwitch.active and domoticz.devices('Living room').temperature > 18) then
             domoticz.devices('Another switch').switchOn()
             domoticz.notify('This rocks!',
                             'Turns out that it is getting warm here',
                             domoticz.PRIORITY_LOW)
          end
       end
    }

Or you have a timer script that should be executed every 10 minutes, but only on weekdays, and have it do something with some user variables and only during daytime:


    return {
       on = {
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

Or you want to detect a humidity rise within the past 5 minutes:

    return {
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

Just to give you an idea! Everything in your Domoticz system is now logically available in the domoticz object structure. With this domoticz object, you can get to all the information in your system and manipulate your devices.

# Using dzVents with Domoticz
In Domoticz go to **Setup > Settings > Other**  and in the section EventSystem make sure the checkbox 'dzVents disabled' is not checked.
Also make sure that in the Security section in the settings **(Setup > Settings > System > Local Networks (no username/password)** you allow 127.0.0.1 to not need a password. dzVents uses that port to send certain commands to Domoticz. Finally make sure you have set your current location in **Setup > Settings > System > Location**, otherwise there is no way to determine nighttime/daytime state.

There are two ways of creating dzVents event scripts in Domoticz:

 1. By creating lua scripts in your domoticz instance on your domoticz server: `/path/to/domoticz/scripts/dzVents/scripts`. Make sure that each script has the extension `.lua` and follows the guidelines as described below.
 2. By creating lua scripts inside Domoticz using the internal Domoticz event editor: Go to **Setup > More Options > Events** and set the event type to Lua (dzVents).

**Note: scripts that you write on the filesystem and inside Domoticz using the internal web-editor all share the same namespace. That means that if you have two scripts with the same name, only the one of the filesystem will be used. The log will tell you when this happens.**

## Quickstart
If you made sure that dzVents system is active, we can do a quick test if everything works:

 - Pick a switch in your Domoticz system. Write down the exact name of the switch. If you don't have a switch then you can create a Dummy switch and use that one.
 - Create a new file in the `/path/to/domoticz/scripts/dzVents/scripts/` folder (or using the web-editor in Domoticz, swtich to dzVents mode first.). Call the file `test.lua`. *Note: when you create a script in the web-editor you do **not** add the .lua extension!* Also, valid script names follow the same rules as [filesystem names](https://en.wikipedia.org/wiki/Filename#Reserved_characters_and_words ).
 - Open `test.lua` in an editor and fill it with this code and change `<exact name of the switch>` with the .. you guessed it... exact name of the switch device:
```
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

    return {
        on = {
            devices = { 'My switch' }
        },
        execute = function(domoticz, switch)
            -- your script logic goes here, something like this:

            if (switch.state == 'On') then
                domoticz.log('I am on!', domoticz.LOG_INFO)
            end
        end
    }

Simply said, the `on`-part defines the trigger and the `execute` part is what should be done if the trigger matches with the current Domoticz event. So all your logic is inside the `execute` function.

## Sections in the script
Each dzVents event script has this structure:
```
return {
   active = true, -- optional
   on = { -- at least one of these:
      devices = { ... },
      variables = { ... },
      timer = { ... },
      security = { ... },
      scenes = { ... },
      groups = { ... },
      httpResponses = { ... }
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
The `on` section has five kinds of subsections that *can all be used simultaneously* :

#### devices = { ... }
A list of device-names or indexes. If a device in your system was changed (e.g. switch was triggered or a new temperature was received) and it is listed in this section then the execute function is executed. Each device can be:

 - The name of your device between string quotes. **You can use the asterisk (\*) wild-card here e.g. `PIR_*` or `*_PIR`**.  E.g.: `devices = { 'myDevice', 'anotherDevice', 123, 'pir*' }`
 - The index (idx) of your device (as the name may change, the index will usually stay the same, the index can be found in the devices section in Domoticz). Note that idx is a number;
 - The name or idx of your device followed by a time constraint, such as:
    `['myDevice']  = { 'at 15:*', 'at 22:* on sat, sun' }` The script will be executed if `myDevice` was changed, **and** it is either between 15:00 and 16:00 or between 22:00 and 23:00 in the weekend. See [time trigger rules](#timer_trigger_rules).

#### scenes = { ... }
 A list of one or more scene-names or indexes. The same rules as devices apply. So if your scene is listed in this table then the executed function will be triggered, e.g.: `on = { scenes = { 'myScene', 'anotherScene' } },`.

#### groups = { ...}
A list of one or more group-names or indexes. The same rules as devices apply.

#### timer = { ... }
A list of one ore more time 'rules' like `every minute` or `at 17:*`. See [*timer* trigger rules](#timer_trigger_rules). If any or the rules matches with the current time/date then your execute function is called. E.g.: `on = { timer = { 'between 30 minutes before sunset and 30 minutes after sunrise' } }`.

#### variables = { ... }
A list of one or more user variable-names as defined in Domoticz ( *Setup > More options > User variables*). If any of the variables listed here changes, the script is executed.

#### security = { ... }
A list of one or more of these security states:

 - `domoticz.SECURITY_ARMEDAWAY`,
 - `domoticz.SECURITY_ARMEDHOME`,
 - `domoticz.SECURITY_DISARMED`

If the security state in Domoticz changes and it matches with any of the states listed here, the script will be executed. See `/path/to/domoticz/scripts/dzVents/examples/templates/security.lua` for an example see [Security Panel](#Security_Panel) for information about how to create a security panel device.

#### httpResponses = { ...} <sup>2.4.0</sup>
A list of  one or more http callback triggers. Use this in conjunction with `domoticz.openURL()` where you will provide Domoticz with the callback trigger.  See [Asynchronous HTTP requests](Asynchronous_HTTP_requests) for more information.

### execute = function(**domoticz, item, triggerInfo**) ... end
When all the above conditions are met (active == true and the on section has at least one matching rule), then this `execute` function is called. This is the heart of your script. The function has three parameters:

####1. (**domoticz**, item, triggerInfo)
 The [domoticz object](#Domoticz_object_API_.28Application_Programming_Interface.29). This object gives you access to almost everything in your Domoticz system, including all methods to manipulate them—like modifying switches or sending notifications. More about the [domoticz object](#Domoticz_object_API_.28Application_Programming_Interface.29) below.

#### 2. (domoticz, **item**, triggerInfo)
 Depending on what actually triggered the script, `item` is either a:

 - [device](#Device_object_API),
 - [variable](#Variable_object_API_.28user_variables.29),
 - [scene](#Scene),
 - [group](#Group),
 - timer,
 - [security](#Security_Panel) or
 - [httpResponse](#Asynchronous_HTTP_requests)

Since you can define multiple on-triggers in your script, it is not always clear what the type is of this second parameter. In your code you need to know this in order to properly respond to the different events. To help you inspect the object you can use these attributes like `if (item.isDevice) then ... end`:

 - **isDevice**:  <sup>2.4.0</sup>. returns `true` if the item is a Device object.
 - **isVariable**: <sup>2.4.0</sup>.  returns `true` if the item is a Variable object.
 - **isScene**:  <sup>2.4.0</sup>. returns `true` if the item is a Scene object.
 - **isGroup**:  <sup>2.4.0</sup>. returns `true` if the item is a Group object.
 - **isTimer**: <sup>2.4.0</sup>.  returns `true` if the item is a Timer object.
 - **isSecurity**: <sup>2.4.0</sup>.  returns `true` if the item is a Security object.
 - **isHTTPResponse**: <sup>2.4.0</sup>.  returns `true` if the item is an HTTPResponse object.
 - **trigger**: <sup>2.4.0</sup>.  *string*. the timer rule, the security state or the http response callback string that actually triggered your script. E.g. if you have multiple timer rules can inspect `trigger` which exact timer rule was fired.

#### 3. (domoticz, item, **triggerInfo**)
**Note**: as of version 2.4.0, `triggerInfo` has become more or less obsolete and is left in here for backward compatibility. All information is now available on the `item` parameter (second parameter of the execute function, see point 2 above).

`triggerInfo` holds information about what triggered the script. The object has two attributes:

 1. **type**:  the type of the the event that triggered the execute function, either:
      - domoticz.EVENT_TYPE_TIMER,
      - domoticz.EVENT_TYPE_DEVICE,
      - domoticz.EVENT_TYPE_SECURITY,
      - domoticz.EVENT_TYPE_SCENE,
      - domoticz.EVENT_TYPE_GROUP
      - domoticz.EVENT_TYPE_VARIABLE)
      - domoticz.EVENT_TYPE_HTTPRESPONSE <sup>2.4.0</sup>
 2. **trigger**: the timer rule that triggered the script if the script was called due to a timer event, or the security state that triggered the security trigger rule. See [below](#timer_trigger_rules) for the possible timer trigger rules.
 3. **scriptName**: the name of the current script.

#### Tip: rename the parameters to better fit your needs
The names of the execute parameters are actually something you can change to your convenience. For instance, if you only have one trigger for a specific switch device, you can rename `item` it to `switch`. Or if you think `domoticz` is too long you can rename it to `d` or `dz` (might save you a lot of typing and may make your code more readable):
```
return {
   on = { devices = 'mySwitch' },
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
 - **marker**: A string that is prefixed before each log message. That way you can easily create a filter in the Domoticz log to see just these messages.

Example:
```
logging = {
   level = domoticz.LOG_DEBUG,
   marker = "Hey you"
   },
```

## Some trigger examples

### Device changes
Suppose you have two devices—a smoke detector 'myDetector' and a room temperature sensor 'roomTemp', and you want to send a notification when either the detector detects smoke or the temperature is too high:

```
return {
   on = {
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
```
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
```
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
```
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
```
return {
   on = {
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
```
return {
   on = {
      security = { domoticz.SECURITY_ARMEDAWAY }
   },
   execute = function(domoticz, security)
      domoticz.groups('All lights').switchOff()
   end
}
```

### Asynchronous HTTP Request and handling <sup>2.4.0</sup>
Suppose you have some external web service that will tell you what the current energy consumption is and you want that information in Domoticz:
```
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
See [Asynchronous HTTP requests](Asynchronous_HTTP_requests) for more information.

### Combined rules
Let's say you have a script that checks the status of a lamp and is triggered by motion detector:
```
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

```
    on = {
       timer = {
         'every minute',              -- causes the script to be called every minute
           'every other minute',        -- minutes: xx:00, xx:02, xx:04, ..., xx:58
           'every <xx> minutes',        -- starting from xx:00 triggers every xx minutes
                                        -- (0 > xx < 60)
           'every hour',                -- 00:00, 01:00, ..., 23:00  (24x per 24hrs)
           'every other hour',          -- 00:00, 02:00, ..., 22:00  (12x per 24hrs)
           'every <xx> hours',          -- starting from 00:00, triggers every xx
                                        -- hours (0 > xx < 24)
           'at 13:45',                  -- specific time
           'at *:45',                   -- every 45th minute in the hour
           'at 15:*',                   -- every minute between 15:00 and 16:00
           'at 12:45-21:15',            -- between 12:45 and 21:15. You cannot use '*'!
           'at 19:30-08:20',            -- between 19:30 and 8:20 then next day
           'at 13:45 on mon,tue',       -- at 13:45 only on Mondays and Tuesdays (english)
           'on mon,tue',                -- on Mondays and Tuesdays
           'every hour on sat',         -- you guessed it correctly
           'at sunset',                 -- uses sunset/sunrise info from Domoticz
           'at sunrise',           
           'at civiltwilightstart',     -- uses civil twilight start/end info from Domoticz
           'at civiltwilightend',
           'at sunset on sat,sun',
           'xx minutes before civiltwilightstart',
           'xx minutes after civiltwilightstart',
           'xx minutes before civiltwilightend',
           'xx minutes after civiltwilightend',
           'xx minutes before sunset',
           'xx minutes after sunset',
           'xx minutes before sunrise',
           'xx minutes after sunrise'   -- guess ;-)
           'between aa and bb'          -- aa/bb can be a time stamp like 15:44
                                        -- aa/bb can be sunrise/sunset
                                        -- aa/bb can be 'xx minutes before/after
                                           sunrise/sunset'
           'at civildaytime',           -- between civil twilight start and civil twilight end
           'at civilnighttime',         -- between civil twilight end and civil twilight start
           'at nighttime',              -- between sunset and sunrise
           'at daytime',                -- between sunrise and sunset
           'at daytime on mon,tue',     -- between sunrise and sunset
                                           only on Mondays and Tuesdays
         'in week 12,44'              -- (2.4.0) in week 12 or 44
         'in week 20-25,33-47'        -- (2.4.0) between week 20
                                  and 25 or week 33 and 47
         'in week -12, 33-'           -- (2.4.0) week <= 12 or week >= 33
         'every odd week',
         'every even week',           -- (2.4.0) odd or even numbered weeks
         'on 23/11',                  -- (2.4.0) on 23rd of november (dd/mm)
         'on 23/11-25/12',            -- (2.4.0) between 23/11 and 25/12
         'on 2/3-18/3',11/8,10/10-14/10',
         'on */2,15/*',               -- (2.4.0) every day in February or
                                      -- every 15th day of the month
            'on -3/4,4/7-',              -- (2.4.0) before 3/4 or after 4/7

           -- or if you want to go really wild and combine them:
           'at nighttime at 21:32-05:44 every 5 minutes on sat, sun',
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
The domoticz object passed as the first parameter to the `execute` function contains everything you need to interact with your domotica system. It provides all the information about your devices, scenes, groups, variables and has all the methods needed to inspect and manipulate them. Getting this information is easy:

`domoticz.time.isDayTime` or `domoticz.devices('My sensor').temperature` or `domoticz.devices('My sensor').lastUpdate.minutesAgo`.

The domoticz object consists of a logically arranges structure. It harbors all methods to manipulate Domoticz. dzVents will take care to notify Domoticz with all your intended changes and actions. Earlier you had to fill a commandArray with obscure commands. That's no longer the case.

Some more examples:
 `domoticz.devices(123).switchOn().forMin(5).afterSec(10)` or
 `domoticz.devices('My dummy sensor').updateBarometer(1034, domoticz.BARO_THUNDERSTORM)`.

One tip before you get started:
**Make sure that all your devices have unique names. dzVents will give you warnings in the logs if device names are not unique.**

##  Domoticz object API (Application Programming Interface)
The domoticz object holds all information about your Domoticz system. It has global attributes and methods to query and manipulate your system. It also has a collection of **devices**, **variables** (user variables in Domoticz), **scenes**, **groups**. Each of these collections has three iterator functions: `forEach()`, `filter()` and `reduce()` to make searching for devices easier. See [Looping through the collections: iterators](#Looping_through_the_collections:_iterators).

### Domoticz attributes and methods
 - **devices(idx/name)**: *Function*. A function returning a device by idx or name: `domoticz.devices(123)` or `domoticz.devices('My switch')`. For the device API see [Device object API](#Device_object_API). To iterate over all devices do: `domoticz.devices().forEach(..)`. See [Looping through the collections: iterators](#Looping_through_the_collections:_iterators). Note that you cannot do `for i, j in pairs(domoticz.devices()) do .. end`.
 - **dump()**: *Function*. <sup>2.4.16</sup> Dump all domoticz.settings attributes to the Domoticz log. This ignores the log level setting.
 - **email(subject, message, mailTo)**: *Function*. Send email.
 - **groups(idx/name)**: *Function*: A function returning a group by name or idx. Each group has the same interface as a device. To iterate over all groups do: `domoticz.groups().forEach(..)`. See [Looping through the collections: iterators](#Looping_through_the_collections:_iterators). Note that you cannot do `for i, j in pairs(domoticz.groups()) do .. end`. Read more about [Groups](#Group).
 - **helpers**: *Table*. Collection of shared helper functions available to all your dzVents scripts. See [Shared helper functions](#Shared_helper_functions).
 - **log(message, [level])**: *Function*. Creates a logging entry in the Domoticz log but respects the log level settings. You can provide the loglevel: `domoticz.LOG_INFO`, `domoticz.LOG_DEBUG`, `domoticz.LOG_ERROR` or `domoticz.LOG_FORCE`. In Domoticz settings you can set the log level for dzVents.
 - **notify(subject, message, priority, sound, extra, subsystem)**: *Function*. Send a notification (like Prowl). Priority can be like `domoticz.PRIORITY_LOW, PRIORITY_MODERATE, PRIORITY_NORMAL, PRIORITY_HIGH, PRIORITY_EMERGENCY`. For sound see the SOUND constants below. `subsystem` can be a table containing one or more notification subsystems. See `domoticz.NSS_xxx` types.
 - **openURL(url/options)**: *Function*. Have Domoticz 'call' a URL. If you just pass a url then Domoticz will execute the url after your script has finished but you will not get notified.  If you pass a table with options then you have to possibility to receive the results of the request in a dzVents script. Read more about [asynchronous http requests](#Asynchronous_HTTP_requests)<sup>2.4.0</sup> with dzVents. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **scenes(idx/name)**: *Function*: A function returning a scene by name or id. Each scene has the same interface as a device. See [Device object API](#Device_object_API). To iterate over all scenes do: `domoticz.scenes().forEach(..)`. See [Looping through the collections: iterators]. (#Looping_through_the_collections:_iterators). Note that you cannot do `for i, j in pairs(domoticz.scenes()) do .. end`. Read more about [Scenes](#Scene).
 - **security**: Holds the state of the security system e.g. `Armed Home` or `Armed Away`.
 - **sendCommand(command, value)**: Generic (low-level)command method (adds it to the commandArray) to the list of commands that are being sent back to domoticz. *There is likely no need to use this directly. Use any of the device methods instead (see below).*
 - **settings**:
    - **domoticzVersion**:<sup>2.4.15</sup> domoticz version string.
    - **dzVentsVersion**:<sup>2.4.15</sup> dzVents version string.
    - **location**
        - **latitude**:<sup>2.4.14</sup> domoticz settings locations latitude.
        - **longitude**:<sup>2.4.14</sup> domoticz settings locations longitude.
        - **name**:<sup>2.4.14</sup> domoticz settings location Name. 
    - **serverPort**: webserver listening port.
    - **url**: internal url to access the API service.
    - **webRoot**: `webroot` value as specified when starting the Domoticz service.
 - **sms(message)**: *Function*. Sends an sms if it is configured in Domoticz.
 - **snapshot(cameraID or camera Name<sup>2.4.15</sup>,subject)**:<sup>2.4.11</sup> *Function*. Sends email with a camera snapshot if email is configured and set for attachments in Domoticz.
 - **startTime**: *[Time Object](#Time_object)*. Returns the startup time of the Domoticz service.
 - **systemUptime**: *Number*. Number of seconds the system is up.
 - **time**: *[Time Object](#Time_object)*: Current system time. Additional to Time object attributes:
    - **isDayTime**: *Boolean*
    - **isNightTime**: *Boolean*
    - **isCivilDayTime**: *Boolean*. <sup>2.4.7</sup>
    - **isCivilNightTime**: *Boolean*. <sup>2.4.7</sup>
    - **isToday**: *Boolean*. Indicates if the device was updated today
    - **sunriseInMinutes**: *Number*. Number of minutes since midnight when the sun will rise.
    - **sunsetInMinutes**: *Number*. Number of minutes since midnight when the sun will set.
    - **civTwilightStartInMinutes**: *Number*. <sup>2.4.7</sup> Number of minutes since midnight when the civil twilight will start.
    - **civTwilightEndInMinutes**: *Number*. <sup>2.4.7</sup> Number of minutes since midnight when the civil twilight will end.
 - **triggerIFTTT(makerName [,sValue1, sValue2, sValue3])**: *Function*. <sup>2.4.18</sup> Have Domoticz 'call' an IFTTT maker event. makerName is required, 0-3 sValue's are optional. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **utils**: <sup>2.4.0</sup>. A subset of handy utilities:
    - _: Lodash. This is an entire collection with very handy Lua functions. Read more about [Lodash](#Lodash_for_Lua).  E.g.: `domoticz.utils._.size({'abc', 'def'}))` Returns 2.
    - **dumpTable(table,[levelIndicator])**: *Function*: <sup>2.4.19</sup> print table structure and contents to log
    - **fileExists(path)**: *Function*: <sup>2.4.0</sup> Returns `true` if the file (with full path) exists.
    - **fromJSON(json, fallback <sup>2.4.16</sup>)**: *Function*. Turns a json string to a Lua table. Example: `local t = domoticz.utils.fromJSON('{ "a": 1 }')`. Followed by: `print( t.a )` will print 1. Optional 2nd param fallback will be returned if json is nil or invalid.
    - **inTable(table, searchString)**: *Function*: <sup>2.4.21</sup> Returns `"key"` if table has searchString as a key, `"value"` if table has searchString as value and `false` otherwise.
    - **osExecute(cmd)**: *Function*:  Execute an os command.
    - **round(number, [decimalPlaces])**: *Function*. Helper function to round numbers. Default decimalPlaces is 0.
    - **stringSplit(string, [separator ])**:<sup>2.4.19</sup> *Function*. Helper function to split a line in separate words. Default separator is space. Return is a table with separate words.
    - **toCelsius(f, relative)**: *Function*. Converts temperature from Fahrenheit to Celsius along the temperature scale or when relative==true it uses the fact that 1F==0.56C. So `toCelsius(5, true)` returns 5F*(1/1.8) = 2.78C.
    - **toJSON(luaTable)**: *Function*. <sup>2.4.0</sup> Converts a Lua table to a json string.
    - **urlDecode(s)**: <sup>2.4.13</sup> *Function*. Simple deCoder to convert a string with escaped chars (%20, %3A and the likes) to human readable format
    - **urlEncode(s, [strSub])**: *Functon*. Simple url encoder for string so you can use them in `openURL()`. `strSub` is optional and defaults to + but you can also pass %20 if you like/need.
 - **variables(idx/name)**: *Function*. A function returning a variable by it's name or idx. See  [Variable object API](#Variable_object_API_.28user_variables.29) for the attributes. To iterate over all variables do: `domoticz.variables().forEach(..)`. See [Looping through the collections: iterators](#Looping_through_the_collections:_iterators). **Note that you cannot do `for i, j in pairs(domoticz.variables()) do .. end`**.

### Looping through the collections: iterators
The domoticz object has these collections (tables): devices, scenes, groups, variables, changedDevices and changedVariables. You cannot use the `pairs()` or `ipairs()` functions. Therefore dzVents has three iterator methods:

 1. **find(function)**: Returns the item in the collection for which `function` returns true. When no item is found `find` returns nil.
 2. **forEach(function)**: Executes function once per array element. The function receives the item in the collection (device or variable). If the function returns *false*, the loop is aborted.
 3. **filter(function / table)**: returns items in the collection for which the function returns true. You can also provide a table with names and/or ids.
 4. **reduce(function, initial)**: Loop over all items in the collection and do some calculation with it. You call it with the function and the initial value. Each iteration the function is called with the accumulator and the item in the collection. The function does something with the accumulator and returns a new value for it.

####Examples:

find():
```
   local myDevice = domoticz.devices().find(function(device)
      return device.name == 'myDevice'
   end)
   domoticz.log('Id: ' .. myDevice.id)
```
forEach():
```
    domoticz.devices().forEach(function(device)
       if (device.batteryLevel < 20) then
          -- do something
       end
    end)
```
filter():
```
   local deadDevices = domoticz.devices().filter(function(device)
      return (device.lastUpdate.minutesAgo > 60)
   end)
   deadDevices.forEach(function(zombie)
      -- do something
   end)
```
or
```
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
```
   domoticz.devices().filter(function(device)
      return (device.lastUpdate.minutesAgo > 60)
   end).forEach(function(zombie)
      -- do something with the zombie
   end)
```
Using a reducer to count all devices that are switched on:
```
    local count = domoticz.devices().reduce(function(acc, device)
       if (device.state == 'On') then
          acc = acc + 1 -- increase the accumulator
       end
       return acc -- always return the accumulator
    end, 0) -- 0 is the initial value for the accumulator
```

### Constants
The domoticz object has these constants available for use in your code e.g. `domoticz.LOG_INFO`.

**IMPORTANT:  you have to prefix these constants with `domoticz.<constant>`**:

 - **ALERTLEVEL_GREY**, **ALERTLEVEL_GREEN**, **ALERTLEVEL_ORANGE**, **ALERTLEVEL_RED**, **ALERTLEVEL_YELLOW**: for updating text sensors.
 - **BASETYPE_DEVICE**, **BASETYPE_SCENE**, **BASETYPE_GROUP**, **BASETYPE_VARIABLE**, **BASETYPE_SECURITY**, **BASETYPE_TIMER**, **BASETYPE_HTTP_RESPONSE**: indicators for the various object types that are passed as the second parameter to the execute function. E.g. you can check if an object is a device object: `if (item.baseType == domoticz.BASETYPE_DEVICE) then ... end`.
 - **BARO_CLOUDY**, **BARO_CLOUDY_RAIN**, **BARO_STABLE**, **BARO_SUNNY**, **BARO_THUNDERSTORM**, **BARO_NOINFO**, **BARO_UNSTABLE**: for updating barometric values.
 - **EVENT_TYPE_DEVICE**, **EVENT_TYPE_VARIABLE**, **EVENT_TYPE_SECURITY**,  **EVENT_TYPE_HTTPRESPONSE**<sup>2.4.0</sup>, **EVENT_TYPE_TIMER**: triggerInfo types passed to the execute function in your scripts.
 - **EVOHOME_MODE_AUTO**, **EVOHOME_MODE_TEMPORARY_OVERRIDE**, **EVOHOME_MODE_PERMANENT_OVERRIDE**, **EVOHOME_MODE_FOLLOW_SCHEDULE** <sup>2.4.9</sup>: mode for EvoHome system.
 - **HUM_COMFORTABLE**, **HUM_DRY**, **HUM_NORMAL**, **HUM_WET**: constant for humidity status.
 - **INTEGER**, **FLOAT**, **STRING**, **DATE**, **TIME**: variable types.
 - **LOG_DEBUG**, **LOG_ERROR**, **LOG_INFO**, **LOG_FORCE**: for logging messages. LOG_FORCE is at the same level as LOG_ERROR.
 - **NSS_GOOGLE_CLOUD_MESSAGING**, **NSS_HTTP**, **NSS_KODI**, **NSS_LOGITECH_MEDIASERVER**, **NSS_NMA**,**NSS_PROWL**, **NSS_PUSHALOT**, **NSS_PUSHBULLET**, **NSS_PUSHOVER**, **NSS_PUSHSAFER**, **NSS_TELEGRAM** <sup>2.4.8</sup>: for notification subsystem
 - **PRIORITY_LOW**, **PRIORITY_MODERATE**, **PRIORITY_NORMAL**, **PRIORITY_HIGH**, **PRIORITY_EMERGENCY**: for notification priority.
 - **SECURITY_ARMEDAWAY**, **SECURITY_ARMEDHOME**, **SECURITY_DISARMED**: for security state.
 - **SOUND_ALIEN** , **SOUND_BIKE**, **SOUND_BUGLE**, **SOUND_CASH_REGISTER**, **SOUND_CLASSICAL**, **SOUND_CLIMB** , **SOUND_COSMIC**, **SOUND_DEFAULT** , **SOUND_ECHO**, **SOUND_FALLING**  , **SOUND_GAMELAN**, **SOUND_INCOMING**, **SOUND_INTERMISSION**, **SOUND_MAGIC** , **SOUND_MECHANICAL**, **SOUND_NONE**, **SOUND_PERSISTENT**, **SOUND_PIANOBAR** , **SOUND_SIREN** , **SOUND_SPACEALARM**, **SOUND_TUGBOAT**  , **SOUND_UPDOWN**: for notification sounds.

## Device object API
If you have a dzVents script that is triggered by switching a device in Domoticz then the second parameter passed to the execute function will be a *device* object. Also, each device in Domoticz can be found in the `domoticz.devices()` collection (see above). The device object has a set of fixed attributes like *name* and *idx*. Many devices, however, have different attributes and methods depending on their (hardware)type, subtype and other device specific identifiers. It is possible that you will get an error if you call a method on a device that doesn't support it, so please check the device properties for your specific hardware to see which are supported (can also be done in your script code!).

dzVents recognizes most of the different device types in Domoticz and creates the proper attributes and methods. It is possible that your device type has attributes that are not recognized; if that's the case, please create a ticket in the Domoticz [issue tracker on GitHub](https://github.com/domoticz/domoticz/issues), and an adapter for that device will be added.

If for some reason you miss a specific attribute or data for a device, then likely the `rawData` attribute will have that information.

### Device attributes and methods for all devices
 - **active**: *Boolean*. Is true for some common states like 'On' or 'Open' or 'Motion'. Same as bState.
 - **batteryLevel**: *Number* If applicable for that device then it will be from 0-100.
 - **bState**: *Boolean*. Is true for some common states like 'On' or 'Open' or 'Motion'. Better to use active.
 - **changed**: *Boolean*. True if the device was changed
 - **description**: *String*. Description of the device.
 - **deviceId**: *String*. Another identifier of devices in Domoticz. dzVents uses the id(x) attribute. See device list in Domoticz' settings table.
 - **deviceSubType**: *String*. See Domoticz devices table in Domoticz GUI.
 - **deviceType**: *String*. See Domoticz devices table in Domoticz GUI.
 - **dump()**: *Function*. Dump all attributes to the Domoticz log. This ignores the log level setting.
 - **hardwareName**: *String*. See Domoticz devices table in Domoticz GUI.
 - **hardwareId**: *Number*. See Domoticz devices table in Domoticz GUI.
 - **hardwareType**: *String*. See Domoticz devices table in Domoticz GUI.
 - **hardwareTypeValue**: *Number*. See Domoticz devices table in Domoticz GUI.
 - **icon**: *String*. Name of the icon in Domoticz GUI.
 - **id**: *Number*. Index of the device. You can find the index in the device list (idx column) in Domoticz settings. It's not truly an index but is unique enough for dzVents to be treated as an id.
 - **idx**: *Number*. Same as id: index of the device
 - **lastUpdate**: *[Time Object](#Time_object)*: Time when the device was updated.
 - **name**: *String*. Name of the device.
 - **nValue**: *Number*. Numerical representation of the state.
 - **rawData**: *Table*: All values are *String* types and hold the raw data received from Domoticz.
 - **setDescription(description)**: *Function*. <sup>2.4.16</sup> Generic method to update the description for all devices, groups and scenes. E.g.: device.setDescription(device.description .. '/nChanged by '.. item.trigger .. 'at ' .. domoticz.time.raw). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **setIcon(iconNumber)**: *Function*. <sup>2.4.17</sup> method to update the icon for devices. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **setState(newState)**: *Function*. Generic update method for switch-like devices. E.g.: device.setState('On'). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **setValues(nValue,[ sValue1, sValue2, ...])**: *Function*. <sup>2.4.17</sup> Generic alternative method to update device nValue, sValue. Uses domoticz JSON API to force subsequent events like pushing to influxdb. nValue required but when set to nil it will use current nValue. sValue parms are optional and can be many.
 - **state**: *String*. For switches, holds the state like 'On' or 'Off'. For dimmers that are on, it is also 'On' but there is a level attribute holding the dimming level. **For selector switches** (Dummy switch) the state holds the *name* of the currently selected level. The corresponding numeric level of this state can be found in the **rawData** attribute: `device.rawData[1]`.
 - **signalLevel**: *Number* If applicable for that device then it will be from 0-100.
 - **switchType**: *String*. See Domoticz devices table in Domoticz GUI(Switches tab). E.g. 'On/Off', 'Door Contact', 'Motion Sensor' or 'Blinds' 
 - **switchTypeValue**: *Number*. See Domoticz devices table in Domoticz GUI.
 - **timedOut**: *Boolean*. Is true when the device couldn't be reached.
 - **unit**: *Number*. Device unit. See device list in Domoticz' settings for the unit.
 - **update(< params >)**: *Function*. Generic update method. Accepts any number of parameters that will be sent back to Domoticz. There is no need to pass the device.id here. It will be passed for you. Example to update a temperature: `device.update(0,12)`. This will eventually result in a commandArray entry `['UpdateDevice']='<idx>|0|12'`. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

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

#### Counter, managed Counter <sup>2.4.12</sup>,counter incremental
 - **counter**: *Number*
 - **counterToday**: *Number*. Today's counter value.
 - **updateCounter(value)**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **valueQuantity**: *String*. For counters.
 - **valueUnits**: *String*.

#### Custom sensor
 - **sensorType**: *Number*.
 - **sensorUnit**: *String*:
 - **updateCustomSensor(value)**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Distance sensor
 - **distance**: *Number*.
 - **updateDistance(distance)**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Electric usage
 - **WhActual**: *Number*. Current Watt usage.
 - **updateEnergy(energy)**: *Function*. In Watt. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Evohome (zones)
 - **setPoint**: *Number*.
 - **mode**: *string* <sup>2.4.9</sup>.
 - **untilDate**: *string in ISO 8601 format* or n/a <sup>2.4.9</sup>.
 - **updateSetPoint(setPoint, mode, until)**: *Function*. Update set point. Mode can be domoticz.EVOHOME_MODE_AUTO, domoticz.EVOHOME_MODE_TEMPORARY_OVERRIDE, domoticz.EVOHOME_MODE_FOLLOW_SCHEDULE <sup>2.4.9</sup> or domoticz.EVOHOME_MODE_PERMANENT_OVERRIDE. You can provide an until date (in ISO 8601 format e.g.: `os.date("!%Y-%m-%dT%TZ")`). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Evohome (hotWater) <sup>2.4.9</sup>.
 - **state**: *string*  ('On' or 'Off')
 - **mode**: *string* 
 - **untilDate**: *string in ISO 8601 format* or n/a.
 - **setHotWater(state, mode, until)**: *Function*. set HotWater Mode can be domoticz.EVOHOME_MODE_AUTO, domoticz.EVOHOME_MODE_TEMPORARY_OVERRIDE, domoticz.EVOHOME_MODE_FOLLOW_SCHEDULE or domoticz.EVOHOME_MODE_PERMANENT_OVERRIDE You can provide an until date (in ISO 8601 format for domoticz.EVOHOME_MODE_TEMPORARY_OVERRIDE e.g.: `os.date("!%Y-%m-%dT%TZ")`). 
 
#### Gas
 - **counter**: *Number*. Value in m<sup>3</sup>
 - **counterToday**: *Number*. Value in m<sup>3</sup>
 - **updateGas(usage)**: *Function*. Usage in **dm<sup>3</sup>** (liters). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Group
 - **devices()**: *Function*. Returns the collection of associated devices. Supports the same iterators as for `domoticz.devices()`: `forEach()`, `filter()`, `find()`, `reduce()`. See [Looping through the collections: iterators](#Looping_through_the_collections:_iterators). Note that the function doesn't allow you to get a device by its name or id. Use `domoticz.devices()` for that.
 - **switchOff()**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **switchOn()**: Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **toggleGroup()**: Toggles the state of a group. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

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
 - **counterToday**: *Number*.
 - **updateElectricity(power, energy)**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **usage**: *Number*.
 - **WhToday**: *Number*. Total Wh usage of the day. Note the unit is Wh and not kWh!
 - **WhTotal**: *Number*. Total Wh usage.
 - **WhActual**: *Number*. Actual reading.

#### Leaf wetness
 - **wetness**: *Number*. Wetness value
 - **updateWetness(wetness)**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Logitech Media Server
 - **pause()**: *Function*. <sup>2.4.0</sup> Will pause the device if it was streaming. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **play()**: *Function*. <sup>2.4.0</sup> If the device was off, it will turn on and start streaming. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **playFavorites([position])**: *Function*. <sup>2.4.0</sup>  Will start the favorites playlist. `position` is optional (0 = default). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **playlistID**: *Nubmer*. <sup>2.4.0</sup>
 - **setVolume(level)**: *Function*. <sup>2.4.0</sup> Sets the volume (0 <= level <= 100). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **startPlaylist(name)**: *Function*. <sup>2.4.0</sup> Will start the playlist by its `name`. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **stop()**: *Function*. <sup>2.4.0</sup> Will stop the device (only effective if the device is streaming). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **volumeUp()**: *Function*. <sup>2.4.16</sup> Will turn the device volume up with 2 points. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **volumeDown()**: *Function*. <sup>2.4.16</sup> Will turn the device volume down with 2 points. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Lux sensor
 - **lux**: *Number*. Lux level for light sensors.
 - **updateLux(lux)**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Onkyo receiver
 - **onkyoEISCPCommand(command)**. *Function*. <sup>2.4.0</sup> Sends an EISP command to the Onkyo receiver.

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
 - **rain**: *Number*
 - **rainRate**: *Number*
 - **updateRain(rate, counter)**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### RGBW(W) / Lighting Limitless/Applamp
 - **decreaseBrightness()**: *Function*. <sup>2.4.0</sup> Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **getColor()**; *Function*. <sup>2.4.17</sup> Returns table with color attributes or nil when color field of device is not set.
 - **increaseBrightness()**: *Function*. <sup>2.4.0</sup> Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **setColor(r, g, b, br, cw, ww, m, t)**: *Function*. <sup>2.4.16</sup> Sets the light to requested color.  r, g, b required, others optional. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **setColorBrightness()**: same as setColor
 - **setDiscoMode(modeNum)**: *Function*. <sup>2.4.0</sup> Activate disco mode, `1 =< modeNum <= 9`. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
- **setHex(r, g, b)**: *Function*. <sup>2.4.16</sup> Sets the light to requested color.  r, g, b required (decimal Values 0-255). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
- **setHue(hue, brightness, isWhite)**: *Function*. <sup>2.4.16</sup> Sets the light to requested Hue. Hue and brightness required. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **setKelvin(Kelvin)**: *Function*. <sup>2.4.0</sup> Sets Kelvin level of the light (For RGBWW devices only). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **setNightMode()**: *Function*. <sup>2.4.0</sup> Sets the lamp to night mode. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **setRGB(red, green, blue)**: *Function*. <sup>2.4.0</sup> Set the lamps RGB color. Values are from 0-255. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **setWhiteMode()**: *Function*. <sup>2.4.0</sup> Sets the lamp to white mode. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Scale weight
 - **weight**: *Number*
 - **udateWeight()**: *Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Scene
 - **devices()**: *Function*. Returns the collection of associated devices. Supports the same iterators as for `domoticz.devices()`: `forEach()`, `filter()`, `find()`, `reduce()`. See [Looping through the collections: iterators](#Looping_through_the_collections:_iterators). Note that the function doesn't allow you to get a device by its name or id. Use `domoticz.devices()` for that.
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
 - **quietOn()**: *Function*. <sup>2.4.20</sup> Set deviceStatus to on without physically switching it. Subsequent Events are triggered. Supports some [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **quietOff()**: *Function*. <sup>2.4.20</sup> set deviceStatus to off without physically switching it. Subsequent Events are triggered. Supports some [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **stop()**: *Function*. Set device to Stop if it supports it (e.g. blinds). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **switchOff()**: *Function*. Switch device off it is supports it. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **switchOn()**: *Function*. Switch device on if it supports it. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **switchSelector(<[level]|[levelname]>)**: *Function*. Switches a selector switch to a specific level ( levelname or level(numeric) required ) levelname must be exact, for level the closest fit will be picked. See the edit page in Domoticz for such a switch to get a list of the values). Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
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
 - **updateTempHumBaro(temperature, humidity, status, pressure, forecast)**: *Function*. forecast can be domoticz.BARO_NOINFO, BARO_SUNNY, BARO_PARTLY_CLOUDY, BARO_CLOUDY, BARO_RAIN. status can be domoticz.HUM_NORMAL, HUM_COMFORTABLE, HUM_DRY, HUM_WET. Note: temperature must be in Celsius. Use `domoticz.toCelsius()` to convert a Fahrenheit temperature to Celsius. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Temperature, Humidity
 - **dewPoint**: *Number*
 - **humidity**: *Number*
 - **humidityStatus**: *String*
 - **humidityStatusValue**: *Number*. Value matches with domoticz.HUM_NORMAL, -HUM_DRY, HUM_COMFORTABLE, -HUM_WET.
 - **temperature**: *Number*
 - **updateTempHum(temperature, humidity, status)**: *Function*. status can be domoticz.HUM_NORMAL, HUM_COMFORTABLE, HUM_DRY, HUM_WET. Note: temperature must be in Celsius. Use `domoticz.toCelsius()` to convert a Fahrenheit temperature to Celsius. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Text
 - **text**: *String*
 - **updateText(text)**: Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Thermostat set point
 - **setPoint**: *Number*.
 - **updateSetPoint(setPoint)**:*Function*. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

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
 - **gustMs**: *Number*. Gust ( in meters / second ) <sup>2.4.9</sup>
 - **temperature**: *Number*
 - **speed**: *Number*. Windspeed ( in the unit set in Meters/Counters settings for Wind Meter )
 - **speedMs**: *Number*. Windspeed ( in meters / second ) <sup>2.4.9</sup>
 - **updateWind(bearing, direction, speed, gust, temperature, chill)**: *Function*. Bearing in degrees, direction in N, S, NNW etc, speed in m/s, gust in m/s, temperature and chill in Celsius. Use `domoticz.toCelsius()` to convert a Fahrenheit temperature to Celsius. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

#### Youless meter <sup>2.4.6</sup>
 - **counterDeliveredToday**: *Number*.
 - **counterDeliveredTotal**: *Number*.
 - **powerYield**: *String*.
 - **updateYouless(total, actual)**: *Function*.

#### Zone heating
 - **setPoint**: *Number*.
 - **heatingMode**: *String*.

#### Z-Wave Thermostat mode
 - **mode**: *Number*. Current mode number.
 - **modeString**: *String*. Mode name.
 - **modes**: *Table*. Lists all available modes.
 - **updateMode(modeString)**: *Function*. Set new mode. Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

### Command options (delay, duration, event triggering)
Many dzVents device methods support extra options, like controlling a delay or a duration:

    -- switch on for 2 minutes after 10 seconds
    device.switchOn().afterSec(10).forMin(2)

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

####Options
 - **afterHour(hours), afterMin(minutes), afterSec(seconds)**: *Function*. Activates the command after a certain number of hours, minutes or seconds.
 - **cancelQueuedCommands()**: *Function*. <sup>2.4.0</sup> Cancels queued commands. E.g. you switch on a device after 10 minutes:  `myDevice.switchOn().afterMin(10)`. Within those 10 minutes you can cancel that command by calling:  `myDevice.cancelQueuedCommands()`.
 - **checkFirst()**: *Function*. Checks if the **current** state of the device is different than the desired new state. If the target state is the same, no command is sent. If you do `mySwitch.switchOn().checkFirst()`, then no switch command is sent if the switch is already on. This command only works with switch-like devices. It is not available for toggle and dim commands, either.
 - **forHour(hours), forMin(minutes), forSec(seconds)**: *Function*. Activates the command for the duration of hours, minutes or seconds. See table below for applicability.
 - **withinHour(hours), withinMin(minutes), withinSec(seconds)**: *Function*. Activates the command within a certain period (specified in hours, minutes or seconds) *randomly*. See table below for applicability.
 - **silent()**: *Function*. No follow-up events will be triggered: `mySwitch.switchOff().silent()`.
 - **repeatAfterHour(hours, [number]), repeatAfterMin(minutes, [number]), repeatAfterSec(seconds, [number])**: *Function*. Repeats the sequence *number* times after the specified duration (specified in hours, minutes, or seconds).  If no *number* is provided, 1 is used. **Note that `afterXXX()` and `withinXXX()` are only applied at the beginning of the sequence and not between the repeats!**

Note that the actual switching or changing of the device is done by Domoticz and not by dzVents. dzVents only tells Domoticz what to do; if the options are not carried out as expected, this is likely a Domoticz or hardware issue.

**Important note when using forXXX()**: Let's say you have a light that is triggered by a motion detector.  Currently the light is `Off` and you do this: `light.switchOn().forMin(5)`. What happens inside Domoticz is this:  at t<sub>0</sub> Domoticz issues the `switchOn()` command and schedules a command to restore the **current** state at t<sub>5</sub> which is `Off`. So at t<sub>5</sub> it will switch the light off.

If, however, *before the scheduled `switchOff()` happens at t<sub>5</sub>*, new motion is detected and you send this command again at t<sub>2</sub> then something unpredictable may seem to happen: *the light is never turned off!* This is what happens:

At t<sub>2</sub> Domoticz receives the `switchOn().forMin(5)` command again. It sees a scheduled command at t<sub>5</sub> and deletes that command (it is within the new interval). Then Domoticz performs the (unnecessary, it's already on) `switchOn()` command. Then it checks the current state of the light which is `On`!! and schedules a command to return to that state at t<sub>2+5</sub>=t<sub>7</sub>. So, at t<sub>7</sub> the light is switched on again. And there you have it: the light is not switched off and never will be because future commands will always be checked against the current on-state.

That's just how it works and you will have to deal with it in your script. So, instead of simply re-issuing `switchOn().forMin(5)` you have to check the switch's state first:
```
if (light.active) then
   light.switchOff().afterMin(5)
else
   light.switchOn().forMin(5)
end
```
or issue these two commands *both* as they are mutually exclusive:
```
light.switchOff().checkFirst().afterMin(5)
light.switchOn().checkFirst().forMin(5)
```

####Availability
Some options are not available to all commands. All the options are available to device switch-like commands like `myDevice.switchOff()`, `myGroup.switchOn()` or `myBlinds.open()`.  For updating (usually Dummy ) devices like a text device `myTextDevice.updateText('zork')` you can only use `silent()`. For thermostat setpoint devices and snapshot command silent() is not available.  See table below   

| option                   | state changes            | update commands | user variables | updateSetpoint |  snapshot  | triggerIFTTT |
|--------------------------|:------------------------:|:---------------:|:--------------:|:--------------:|:-----------|:-------------|
| `afterXXX()`             |  •                       |  •              | •              | •              | •          | •            |
| `forXXX()`               |  •                       |  n/a            | n/a            | n/a            | n/a        | n/a          |
| `withinXXX()`            |  •                       |  •              | •              | •              | •          | n/a          |
| `silent()`               |  •                       |  •              | •              | n/a            | n/a        | n/a          |
| `repeatAfterXXX()`       |  •                       |  n/a            | n/a            | n/a            | n/a        | n/a          |
| `checkFirst()`           |  • (switch-like devices) |  n/a            | n/a            | n/a            | n/a        | n/a          |
| `cancelQueuedCommands()` |  •                       |  •              | •              | n/a            | n/a        | n/a          |

**Note**: XXX is a placeholder for `Min/Sec/Hour` affix e.g. `afterMin()`.
**Note**: for `domoticz.openURL()` only `afterXXX()` and `withinXXX()` is available.

#### Follow-up event triggers
Normally if you issue a command, Domoticz will immediately trigger follow-up events, and dzVents will automatically trigger defined event scripts. If you trigger a scene, all devices in that scene will issue a change event. If you have event triggers for these devices, they will be executed by dzVents. If you don't want this to happen, add `.silent()` to your commands (exception is updateSetPoint).

### Create your own device adapter
If your device is not recognized by dzVents, you can still operate it using the generic device attributes and methods. It is much nicer, however, to have device specific attributes and methods. Existing recognized adapters are in `/path/to/domoticz/dzVents/runtime/device-adapters`.  Copy an existing adapter and adapt it to your needs. You can turn debug logging on and inspect the file `domoticzData.lua` in the dzVents folder. There you will find the unique signature for your device type. Usually it is a combination of deviceType and deviceSubType, but you can use any of the device attributes in the `matches` function. The matches function checks if the device type is suitable for your adapter and the `process` function actually creates your specific attributes and methods.
Also, if you call `myDevice.dump()` you will see all attributes and methods and the attribute `_adapters` shows you a list of applied adapters for that device.
Finally, register your adapter in `Adapters.lua`. Please share your adapter when it is ready and working.

## Variable object API (user variables)
User variables created in Domoticz have these attributes and methods:

### Variable attributes and methods
 - **changed**: *Boolean*. Was the device changed.
 - **date**: *Date*. If type is domoticz.DATE. See lastUpdate for the sub-attributes.
 - **id**: *Number*. Index of the variable.
 - **lastUpdate**: *[Time Object](#Time_object)*
 - **name**: *String*. Name of the variable
 - **set(value)**: Tells Domoticz to update the variable. Supports timing options. See [above](#Command_options_.28delay.2C_duration.2C_event_triggering.29).
 - **time**: *Date*. If type is domoticz.TIME. See lastUpdate for the sub-attributes.
 - **type**: *String*. Can be domoticz.INTEGER, domoticz.FLOAT, domoticz.STRING, domoticz.DATE, domoticz.TIME.
 - **value**: *String|Number|Date|Time*. Value of the variable.

## Time object
Many attributes represent a moment in time, like `myDevice.lastUpdate`  or `domoticz.time`. In dzVents, a time-like attribute is an object with properties and methods which make your life easier.

```
   print(myDevice.lastUpdate.minutesAgo)
   print(myDevice.lastUpdate.daysAgo)

   -- compare two times
   print(domoticz.time.compare(myDevice.lastUpdate).secs))
```

You can also create your own:
```
    local Time = require('Time')
    local t = Time('2016-12-12 07:35:00') -- must be this format!!
```
If you don't pass a time string:
```
    local Time = require('Time')
    local currentTime = Time()
```

Use this in combination with the various dzVents time attributes:

```
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
### Time properties and methods

Creation:
```
local Time = require('Time')
local now = Time() -- current time
local someTime = Time('2017-12-31 22:19:15')
local utcTime = Time('2017-12-31 22:19:15', true)
```
 - **civTwilightEndInMinutes**: *Number*. Minutes from midnight until civTwilightEnd.
 - **civTwilightStartInMinutes**:*Number*. Minutes from midnight until civTwilightStart. 
 - **compare(time)**: *Function*. Compares the current time object with another time object. *Make sure you pass a Time object!* Returns a table (all values are *positive*, use the compare property to see if *time* is in the past or future):
    - **milliseconds**: Total difference in milliseconds.
    - **seconds**: Total difference in whole seconds.
    - **minutes**: Total difference in whole minutes.
    - **hours**: Total difference in whole hours.
    - **days**: Total difference in whole days.
    - **compare**: 0 = both are equal, 1 = *time* is in the future, -1 = *time* is in the past.
 - **day**: *Number*
 - **dayAbbrOfWeek**: *String*. sun,mon,tue,wed,thu,fri or sat 
 - **daysAgo**: *Number*
 - **dDate**: *Number*. timestamp (seconds since 01/01/1970 00:00)
 - **getISO**: *Function*. Returns the ISO 8601 formatted date.
 - **hour**: *Number*
 - **hoursAgo**: *Number*. Number of hours since the last update.
 - **isToday**: *Boolean*. Indicates if the device was updated today
 - **isUTC**: *Boolean*.
 - **matchesRule(rule) **: *Function*. Returns true if the rule matches with the time. See [time trigger rules](#timer_trigger_rules) for rule examples.
 - **millisecondsAgo**: *Number*. Number of milliseconds since the last update.
 - **minutes**: *Number*
 - **minutesAgo**: *Number*. Number of minutes since the last update.
 - **month**: *Number*
 - **msAgo**: *Number*. Number of milliseconds since the last update.
 - **raw**: *String*. Generated by Domoticz
 - **rawDate**: *String*. Returns the date part of the raw data.
 - **rawTime**: *String*. Returns the time part of the raw data.
 - **seconds**: *Number*
 - **secondsSinceMidnight**: *Number*
 - **secondsAgo**: *Number*. Number of seconds since the last update.
 - **sunsetInMinutes**: *Number*. Minutes from midnight until sunset.
 - **sunriseInMinutes**: *Number*. Minutes from midnight until sunrise.         
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
 - **wday**: *Number* 
 - **week**: *Number*

**Note: it is currently not possible to change a time object instance.**


## Shared helper functions
It is not unlikely that at some point you want to share Lua code among your scripts. Normally in Lua you would have to create a module and require that module in all you scripts. But dzVents makes that easier for you:
Inside your scripts folder or in Domoticz' GUI web editor, create a `global_data.lua` script (same as for global persistent data) and feed it with this code:
```
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
```
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
```
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
```
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
```
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

```
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

### Re-initializing a variable <sup>2.4.0</sup>
As of dzVents 2.4.0 you can re-initialize a persistent variable and re-apply the initial value as defined in the data section:

```
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

The problem with this is that you have to do a lot of bookkeeping to make sure that there isn't too much data to store (see [below for how it works](#How_does_the_storage_stuff_work.3F)) . Fortunately, dzVents has done this for you:
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

### Defining
Define a script variable or global variable in the data section and set `history = true`:

   …
   data = {
      var1 = { history = true, maxItems = 10, maxHours = 1, maxMinutes = 5 }
   }

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

#### Statistical functions
Statistical functions require *numerical* data in the set. If the set is just numbers you can do this:

    myVar.add(myDevice.temperature) -- adds a number to the set
    myVar.avg() -- returns the average

If, however, you add more complex data or you want to do a computation first, then you have to *tell dzVents how to get a numeric value from these data*. So let's say you do this to add data to the set:

    myVar.add( { 'person' = 'John', waterUsage = u })

Where `u` is a variable that got its value earlier. If you want to calculate the average water usage, then dzVents will not be able to do this because it doesn't know the value is actually in the `waterUsage` attribute! You will get `nil`.

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
 - **delta2( fromIdx, toIdx, [smoothRangeFrom], [smoothRangeTo], [default] )**:  <sup>2.4.0</sup> Same as **delta** but now you can control if the smooth values for the *from-item* and the *to-item* separately. The function also returns the fromItem and the toItem that is used to calculate the delta with: `local delta, from, to = delta2(2, 5, 3, 3)`.
 - **deltaSince([timeAgo](#Time_specification_.28timeAgo.29),  [smoothRange], [default] )**: Same as **delta** but within the `timeAgo` interval. The function also returns the fromItem and the toItem that is used to calculate the delta with: `local delta, from, to = deltaSince('00:00:10', 3)`.
 - **deltaSinceOrOldest([timeAgo](#Time_specification_.28timeAgo.29),  [smoothRangeFrom], [smoothRangeTo], [default] )**:  <sup>2.4.0</sup> Same as **deltaSince** but it will take the oldest value in the set if *timeAgo* is older than the age of the entire set. The function also returns the fromItem and the toItem that is used to calculate the delta with: `local delta, from, to = deltaSinceOrOldest('00:00:10', 3, 3)`.
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
**Again, make sure you don't put too much stuff in your persisted data as it may slow things down too much.**

# Asynchronous HTTP requests
As of 2.4.0 dzVents allows you to make asynchronous HTTP request and handle the results. Asynchronous means that you don't block the system while waiting for the response. Earlier you had to use os functions like `curl` or `wget` and some magic to make sure that you didn't block the system for too long after which Domoticz will terminate the script with a message that it took more than 10 seconds.

dzVents to the rescue. With dzVents there are two ways to make an http call and it is determined by how you use the `domoticz.openURL()` command. The simplest form simply calls `openURL` on the domoticz object with only the url as the parameter (a string value):

```
domoticz.openURL('http://domain/path/to/something?withparameters=1')
```

After your script is finished, Domoticz will make the request that's where it ends. No callback. Nothing.

The second way is different. Instead of passing a url you pass in a table with all the parameters to make the request **and** your provide a *callback trigger* which is just a string or a name:
```
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

```
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
```
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

###Making the request:

 **domoticz.openURL(options)**: *options*  <sup>2.4.0</sup> is a Lua table:

 - **url**: *String*.
 - **method**: *String*. Optional. Either `'GET'` (default) or `'POST'`.
 - **callback**: *String*. Optional. A custom string that will be used by dzVents to find a the callback handler script.
 - **headers**: *Table*. Optional. A Lua table with additions http request-headers.
 - **postData**: Optional. When doing a `POST` this data will be the payload of the request (body). If you provide a Lua table then this will automatically be converted to json and the request-header `application/json` is set. So no need to do that manually.

Supports [command options](#Command_options_.28delay.2C_duration.2C_event_triggering.29).

### The response object
The response object <sup>2.4.0</sup> (second parameter in your execute function) has these attributes:

 - **data**: Raw response data.
 - **headers**: *Table*. Response headers.
 - **isJSON**: *Boolean*. Short for `response.headers['Content-Type'] == 'application/json'`. When true, the data is automatically converted to a Lua table.
 - **json**. *Table*. When the response data is `application/json` then the response data is automatically converted to a Lua table for quick and easy access.
 - **ok**: *Boolean*. `True` when the request was successful. It checks for statusCode to be in range of 200-299.
 - **statusCode**: *Number*. HTTP status codes. See [HTTP response status codes](https://en.wikipedia.org/wiki/List_of_HTTP_status_codes).
 - **statusText**: *String*. <sup>2.4.19</sup> HTTP status message. See [HTTP response status codes](https://en.wikipedia.org/wiki/List_of_HTTP_status_codes).
 - **protocol**: *String*. <sup>2.4.19</sup> HTTP protocol. See [HTTP response status codes](https://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol).
 - **trigger**, **callback**: *String*. The callback string that triggered this response instance. This is useful if you have a script that is triggered by multiple different callback strings.

### More about request and response headers
Whenever you do an http request it is not just some data that is sent. Along with the request a bunch of so-called headers are sent along with it. HTTP headers allow the client and the server to pass additional information with the request or the response. Also, in the response there are also headers (response header). These response headers usually tell you what kind of data is returned, if it is compressed, if the request was successful etc.

#### request headers
dzVents allow you to set custom request headers that will accompany the data in the request. Sometimes it is necessary to set these headers like for instance when a API or webservice require a security token or key or when the service needs to know what the format of the response data is. Check the documentation of the web service.

So, let's say you need to call a web service that requires an api key in the headers and the documentation states it needs to be passed in an x-access-token header. Your openURL command then may look like this:
```
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

```
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

```
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

```
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
			-- we assume you can access your Domoticz using the 1.0.0.127 ip
			-- on port 8080
			domoticz.openURL({
				url = 'http://1.0.0.127:8080/json.htm?type=openzwavenodes&idx=2',
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

# Settings

Settings for dzVents are found in the Domoticz GUI: **Setup > Settings > Other > EventSystem**:

 - **dzVents disabled**: Tick this if you don't want any dzVents script to be executed.
 - **Log level**: (Note that you can override this setting in the logging section of your script)
    - *Errors*,
    - *Errors + minimal execution info*: Errors and some minimal information about which script is being executed and why,
    - *Errors + minimal execution info + generic info*. Even more information about script execution and a bit more log formatting.
    - *Debug*. Shows everything and dzVents will create a file `domoticzData.lua` in the dzVents folder. This is a dump of all the data received by dzVents from Domoticz.. These data are used to create the entire dzVents data structure.
    - *No logging*. As silent as possible.

# Troubleshooting
So, you think if you have done everything correctly but things do not work (in some way) as you expected. Here are a couple steps you can do to find the cause.

### Is dzVents enabled?
Check the settings (see above) and make sure the checkbox **dzVents disabled** is not checked.

### Is your script enabled?
Make sure the active section in your script is set to `true`:  `active = true`. And, on top of that, if you are using the internal web editor to write your script, make sure that your script is active there and you have clicked Save! "Event active" is a separate checkbox that must be ticked for every active script. When not active, the script name will be red in the list on the right.

### Turn on debug logging
Activate debug logging in the settings (see above). This will produce a lot of extra messages in the Domoticz log (don't forget to turn it off when you are done troubleshooting!). It is best to monitor the log through the command line, as the log in the browser sometimes tends to not always show all log messages. See the Domoticz manual for how to do that.

When debug logging is enabled, every time dzVents kicks into action (Domoticz throws an event) it will log it, and it will create a file `/path/to/domoticz/scripts/dzVents/domoticzData.lua` with a dump of the data sent to dzVents. These data lie at the core of the dzVents object model. You can open this file in an editor to see if your device/variable/scene/group is in there. Note that the data in the file are not exactly the same as what you will get when you interact with dzVents, but it is a good start. If your device is not in the data file, then you will not have access to it in dzVents and dzVents will not be able to match triggers with that device. Something's wrong if you expect your device to be in there but it is not (is the device active/used?).

Every time Domoticz starts dzVents and debug logging is enabled you should see these lines:
```
dzVents version: x.y.z
Event trigger type: xxxx
```
Where xxxx can be time, device, uservariable, security or scenegroup. That should give you a clue what kind of event is active. If you don't see this information then dzVents is not active (or debug logging is not active).

### Script is still not executed
If for some reason your script is not executed while all of the above is done, it is possible that your triggers are not correct. Either the time rule is not matching with the current time (try to set the rule to `every minute` or something simple), or the device name is not correct (check casing), or you use an id that doesn't exist. Note that in the `on` section, you cannot use the dzVents domoticz object!

Make sure that if you use an id in the `on` section that this id is correct. Go to the device list in Domoticz and find the device and make sure the device is in the 'used' list!. Unused devices will not create an event. Find the idx for the device and use that as the id.

Also, make sure that your device names are unique! dzVents will throw a warning in the log if multiple device names match. It's best practice to have unique names for your devices. Use some clever naming like prefixing like 'Temperature: living room' or 'Lights: living room'. That also makes using wild cards on your `on` section easier.

If your script is still not triggered, you can try to create a classic Lua event script and see if that does work.

### Debugging your script
A simple way to inspect a device in your script is to dump it to the log: `myDevice.dump()`. This will dump all the attributes (and more) of the device so you can inspect what its state is.
Use print statements or domoticz.log() statements in your script at cricital locations to see if the Lua interpreter reaches that line.
Don't try to print a device object though; use the `myDevice.dump()` method for that. It wil log all attributes of the device in the Domoticz log.

If you place a print statement all the way at the top of your script file it will be printed every time dzVents loads your script (that is done at the beginning of each event cycle). Sometimes that is handy to see if your script gets loaded in the first place. If you don't see your print statement in the log, then likely dzVents didn't load it and it will not work.

### Get help
The Domoticz forum is a great resource for help and solutions. Check the [dzVents forum](https://www.domoticz.com/forum/viewforum.php?f=59).

# Other interesting stuff

## Lodash for Lua

Lodash is a well known and very popular Javascript library filled with dozens of handy helper functions that really make you life a lot easier. Fortunately there is also a Lua version. As of dzVents 2.4.0 this is directly available through the domoticz object:

```
local _ = domoticz.utils._
_.print(_.indexOf({2, 3, 'x', 4}, 'x'))
```

Check out the great documentation [here](http://axmat.github.io/lodash.lua/).


# Migrating from version 1.x.x
As you can read in the change log below there are a couple of changes in 2.0 that will break older scripts.

## The 'on={..}' section.
The on-section needs the items to be grouped based on their type. Prior to 2.0 you had
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
For more information about these iterators see: [Looping through the collections: iterators](#Looping_through_the_collections:_iterators).

## Timed commands
Prior to 2.0, to turn a switch off after 10 seconds:
```
   domoticz.devices['mySwitch'].switchOff().after_sec(10)
```
In 2.x:
```
   domoticz.devices('mySwitch').switchOff().afterSec(10)
```
The same applies for for_min and with_min.

## Device attributes 
Some device attributes are no longer formatted strings with units in there like WhToday. It is possible that you have some scripts that deal with strings instead of values.

## Changed attributes
In 1.x.x you had a changedAttributes collection on a device. That is no longer there. Just remove it and just check for the device to be changed instead.

## Using rawData
Prior to 2.x you likely used the rawData attribute to get to certain device values. With 2.x this is likely not needed anymore. Check the various device types above and check if there isn't a named attribute for you device type. dzVents 2.x uses so called device adapters which should take care of interpreting raw values and put them in named attributes for you. If you miss your device you can file a ticket or create an adapter yourself. See ![Create your own device adapter](#Create_your_own_device_adapter).

## What happened to fetch http data?
In 2.x it is no longer needed to make timed json calls to Domoticz to get extra device information into your scripts. Very handy.
On the other hand, you have to make sure that dzVents can access the json without the need for a password because some commands are issued using json calls by dzVents. Make sure that in Domoticz settings under **Local Networks (no username/password)** you add `127.0.0.1` and you're good to go.

# Change log
##[2.4.22]
- selector.switchSelector method accepts levelNames
- increased selector.switchSelector resilience
- fix wildcard timerule 

##[2.4.21]
- fixed wrong direction for open() and close() for some types of blinds
- Add inTable function to domoticz.utils
- Add sValue attribute to devices

##[2.4.20]
- Add quietOn() and quietOff() method to switchType devices 

##[2.4.19]
- Add stringSplit function to domoticz.utils.
- Add statusText and protocol to HTTPResponse

##[2.4.18]
- Add triggerIFTTT() to domoticz

##[2.4.17]
- Add dumpTable() to domoticz.utils
- Add setValues for devices
- Add setIcon for devices

##[2.4.16]
- Add method dump() to domoticz (dumps settings)
- Add setHue, setColor, setHex, getColor for RGBW(W) devices
- Add setDescription for devices, groups and scenes
- Add volumeUp / volumeDown for Logitech Media Server (LMS)
- Changed domoticz.utils.fromJSON (add optional fallback param) 

##[2.4.15]
- Add option to use camera name in snapshot command
- Add domoticz.settings.domoticzVersion
- Add domoticz.settings.dzVentsVersion

 
##[2.4.14]
- Added domoticz.settings.location.longitude and domoticz.settings.location.latitude 
- Added check for- and message when call to openURL cannot open local (127.0.0.1)
- **BREAKING CHANGE** :Changed domoticz.settings.location to domoticz.settings.location.name (domoticz settings location Name) 
- prevent call to updateCounter with table

##[2.4.13]
- Added domoticz.settings.location (domoticz settings location Name)
- Added domoticz.utils.urlDecode method to convert a string with escaped chars (%20, %3A and the likes) to human readable format

##[2.4.12]
- Added managed Counter (to counter)

##[2.4.11]
- Added snapshot command to send Email with camera snapshot ( afterXXX() and withinXXX() options available)

##[2.4.10]
- Added option to use afterXXX() and withinXXX() functions to updateSetPoint() <sup>needs domoticz V4.10360 or newer</sup>
- Changed function updateMode to display mode as string in domoticz log

##[2.4.9]
- Added evohome hotwater device (state, mode, untilDate and setHotWater function)
- Added mode and untilDate for evohome zone devices 
- Added EVOHOME_MODE_FOLLOW_SCHEDULE as mode for evohome devices
- Add speedMs and gustMs from wind devices 
- bugfix for youless device (0 handling)
- bugfix for time ( twilightstart and twilightend handling)
- Fixed some date-range rule checking

##[2.4.8]
- Added telegram as option for domoticz.notify

##[2.4.7]
- Added support for civil twilight in rules

##[2.4.6]
- Added Youless device
- Added more to the documentation section for http requests
- Made sure global_data is the first module to process. This fixes some unexpected issues if you need some globals initialized before the other scripts are loaded.

##[2.4.5]
- Fixed a bug in date ranges for timer triggers (http://domoticz.com/forum/viewtopic.php?f=59&t=23109).

##[2.4.4]
- Fixed rawTime and rawData so it shows leading zeros when values are below 10.
- Fixed one wildcard issue. Should now work as expected.
- Fixed a problem in Domoticz where you couldn't change the state of some door contact-like switches using the API or dzVents. That seems to work now.

##[2.4.3]
- Fixed trigger wildcards. Now you can do `*aa*bb*cc` or `a*` which will require the target to start with an `a`
- Added more EvoHome device types to the EvoHome device adapter.

##[2.4.2]
- Fixed RGBW device adapter
- Fixed EvoHome device adapter
- Changed param ordering opentherm gateway command (https://www.domoticz.com/forum/viewtopic.php?f=59&t=21620&p=170469#p170469)

##[2.4.1]
- Fixed week number problems on Windows
- Fixed 'on date' rules to support dd/mm format (e.g. 01/02)

##[2.4.0]

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

##[2.3.0]

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

##[2.2.0]

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

##[2.1.1]

 - Fixed typo in the doc WActual > WhActual.
 - Updated switch adapter to match more switch-like devices.
 - Added Z-Wave Thermostat mode device adapter.
 - Fixed a problem with thermostat setpoint devices to issue the proper url when updating.

##[2.1.0]

 - Added support for switching RGB(W) devices (including Philips/Hue) to have toggleSwitch(), switchOn() and switchOff() and a proper level attribute.
 - Added support for Ampère 1 and 3-phase devices
 - Added support for leaf wetness devices
 - Added support for scale weight devices
 - Added support for soil moisture devices
 - Added support for sound level devices
 - Added support for visibility devices
 - Added support for waterflow devices
 - Added missing color attribute to alert sensor devices
 - Added updateEnergy() to electric usage devices
 - Fixed casing for WhTotal, WhActual methods on kWh devices (Watt's in a name?)
 - Added toCelsius() helper method to domoticz object as the various update temperature methods all need Celsius.
 - Added lastLevel for dimmers so you can see the level of the dimmer just before it was switched off (and while is it still on).
 - Added integration tests for full round-trip Domoticz > dzVents > Domoticz > dzVents tests (100 tests). Total tests (unit+integration) now counts 395!
 - Fixed setting uservariables. It still uses json calls to update the variable in Domoticz otherwise you won't get uservariable event scripts triggered in dzVents.
 - Added dzVents version information in the Domoticz settings page for easy checking what dzVents version is being used in your Domoticz built. Eventhough it is integrated with Domoticz, it is handy for dzVents to have it's own heartbeat.
 - avg(), avgSince(), sum() and sumSince() now return 0 instead of nil for empty history sets. Makes more sense.
 - Fixed boiler example to fallback to the current temperature when there is no history data yet when it calculates the average temperature.
 - Use different api command for setting setPoints in the Thermostat setpoint device adapter.

##[2.0.0] Domoticz integration

 - Almost a complete rewrite.
 - **BREAKING CHANGE**: Accessing a device, scene, group, variable, changedDevice, or changedVariable has been changed: instead of doing `domoticz.devices['myDevice']` you now have to call a function: `domoticz.devices('myDevice')`. This applies also for the other collections: `domoticz.scenes(), domoticz.groups(), domoticz.changedDevices(), domoticz.changedVariables()`. If you want to loop over these collection **you can no longer use the standard Lua for..pairs or for..ipairs construct**. You have to use the iterators like forEach, filter and reduce: `domoticz.devices().forEach(function() .. end)` (see [Looping through the collections: iterators](#Looping_through_the_collections:_iterators)). This was a necessary change to make dzVents a whole lot faster in processing your event scripts. **So please change your existing dzVents scripts!**
 - **BREAKING CHANGE**: after_sec, for_min, after_min, within_min methods have been renamed to the camel-cased variants afterSec, forMin, afterMin, withinMin. Please rename the calls in your script.
 - **BREAKING CHANGE**: There is no longer an option to check if an attribute was changed as this was quite useless. The device has a changed flag. You can use that. Please change your existing scripts.
 - **BREAKING CHANGE**: Many device attributes are now in the appropriate type (Number instead of Strings) so you can easily make calculations with them. Units are stripped from the values as much as possible. **Check your scripts as this may break stuff.**
 - **BREAKING CHANGE**: on-section now requires subsections for `devices`, `timer`, `variables`, and `security`. The old way no longer works! Please convert your existing scripts!
 - **BREAKING CHANGE**: Make sure that in Domoticz settings under **Local Networks (no username/password)** you add `127.0.0.1` so dzVents can use Domoticz api when needed.
 - dzVents now works on Windows machines!
 - dzVents is no longer a separate library that you have to get from GitHub. All integrated into Domoticz.
 - Added option to create shared utility/helper functions and have them available in all your scripts. Simply add a `helpers = { myFunction = function() ... end }` to the `global_data.lua` file in your scripts folder and you can access the function everywhere: `domoticz.helpers.myFunction()`.
 - Created a pluggable system for device adapters so people can easily contribute by creating specific adapters for specific devices. An adapter makes sure that you get the proper methods and attributes for that device. See `/path/to/domoticz/scripts/dzVents/runtime/device-adapters`.
 - Added a `reduce()` iterator to the collections in the domoticz object so you can now easily collect data about all your devices. See  [Looping through the collections: iterators](#Looping_through_the_collections:_iterators).
 - Added a `find()` iterator so you can easily find an item in one of the collection (devices, scenes, groups etc.). See  [Looping through the collections: iterators](#Looping_through_the_collections:_iterators).
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

##[1.1.2]

 - More robust way of updating devices.lua
 - Added device level information for non-dimmer-like devices

##[1.1.1]

 - Added support for a devices table in the 'on' section.
 - Added extra log level for only showing information about the execution of a script module.
 - Added example script for System-alive checker notifications (preventing false negatives).

##[1.1]

 - Added example script for controlling the temperature in a room with hysteresis control.
 - Fixed updateLux (thanks to neutrino)
 - Added Kodi commands to the device methods.
 - Fixed updateCounter
 - Added counterToday and counterTotal attributes for counter devices. Only available when http fetching is enabled. See [Using dzVents with Domoticz](#Using_dzVents_with_Domoticz).

##[1.0.2]

 - Added device description attribute.
 - Added support for setting the setpoint for opentherm gateway.
 - Added timedOut boolean attribute to devices. Requires http data fetching to be enabled. See [Using dzVents with Domoticz](#Using_dzVents_with_Domoticz).
 - Properly detects usage devices and their Wattage.

##[1.0.1]

 - Added updateCustomSensor(value) method.
 - Fixed reset() for historical data.

##[1.0][1.0-beta2]

 - Deprecated setNew(). Use add() instead. You can now add multiple values at once in a script by calling multiple add()s in succession.
 - Fixed printing device logs when a value was boolean or nil
 - Fixed WActual/total/today for energy devices.
 - Added updateSetPoint method on devices for dummy thermostat devices and EvoHome setpoint devices
 - Added couple of helper properties on Time object. See README.
 - Renamed the file dzVents_settings.lua to dzVents_settings_example.lua so you don't overwrite your settings when you copy over a new version of dzVents to your system.

##[1.0-beta1]

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

##[0.9.13]

 - Fixed some timer functions

##[0.9.12]

 - Fixed a bug with log level printing. Now errors are printed.
 - Added setPoint, heatingMode, lux, WhTotal, WhToday, WActual attributes to devices that support it. No need to access rawData for these anymore.

##[0.9.11]

 - Added log method to domoticz object. Using this to log message in the Domoticz log will respect the log level setting in the settings file. [dannybloe]
 - Updated readme. Better overview, more attributes described.
 - Added iterator functions (forEach and filter) to domoticz.devices, domoticz.changedDevices and domoticz.variables to iterate or filter more easily over these collections.
 - Added a couple of example scripts.

##[0.9.10]

 - A little less verbose debug logging. Domoticz seems not to print all message in the log. If there are too many they may get lost. [dannybloe]
 - Added method fetchHttpDomoticzData to domoticz object so you can manually trigger getting device information from Domoticz through http. [dannybloe]
 - Added support for sounds in domiticz.notify(). [WebStarVenlo]

##[0.9.9]

 - Fixed a bug where every trigger name was treated as a wild-carded name. Oopsidayzy...

##[0.9.8]

 - Fixed a bug where a device can have underscores in its name.
 - Added dimTo(percentage) method to control dimmers.
 - Modified the switch-like commands to allow for timing options: .for_min(), .within_min(), after_sec() and after_min() methods to control delays and durations. Removed the options argument to the switch functions. See readme.
 - Added switchSelector method on a device to set a selector switch.
 - Added wild-card option for triggers
 - Added http request data from Domoticz to devices. Now you can check the battery level and switch type and more. Make sure to edit dzVents_settings.lua file first and check the readme for install instructions!!!
 - Added log level setting in dzVents_settings.lua

#[0.9.7]

 - Added domoticz object resource structure. Updated readme accordingly. No more (or hardly any) need for juggling with all the Domoticz Lua tables and commandArrays.
