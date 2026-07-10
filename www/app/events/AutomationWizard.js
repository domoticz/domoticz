define(['app'], function (app) {
    'use strict';

    var TRIGGERS = [
        { id: 'device',   icon: 'fa-solid fa-toggle-off',    label: 'Device State',  desc: 'When a device turns on, off, or changes value' },
        { id: 'scene',    icon: 'fa-solid fa-layer-group',   label: 'Scene',         desc: 'When a scene is activated or deactivated' },
        { id: 'group',    icon: 'fa-solid fa-object-group',  label: 'Group',         desc: 'When a group changes state' },
        { id: 'time',     icon: 'fa-solid fa-clock',         label: 'Time Schedule', desc: 'At a specific time with optional day filter' },
        { id: 'interval', icon: 'fa-solid fa-rotate',        label: 'Interval',      desc: 'Repeat every N minutes or hours' },
        { id: 'variable', icon: 'fa-solid fa-sliders',       label: 'Variable',      desc: 'When a user variable is updated' },
        { id: 'sun',      icon: 'fa-solid fa-sun',           label: 'Sun Event',     desc: 'At sunrise or sunset with optional offset' },
        { id: 'security', icon: 'fa-solid fa-lock',          label: 'Security',      desc: 'When the security panel state changes' },
        { id: 'system',   icon: 'fa-solid fa-power-off',     label: 'System',        desc: 'When Domoticz starts or shuts down' },
    ];

    var ACTIONS = [
        { id: 'switch',   icon: 'fa-solid fa-toggle-off',    label: 'Device',        desc: 'Control any device (switch, dimmer, setpoint, sensor, …)' },
        { id: 'notify',   icon: 'fa-solid fa-bell',          label: 'Notification',  desc: 'Send a push notification or alert' },
        { id: 'email',    icon: 'fa-solid fa-envelope',      label: 'Email',         desc: 'Send an email notification' },
        { id: 'scene',    icon: 'fa-solid fa-layer-group',   label: 'Scene',         desc: 'Activate or deactivate a scene' },
        { id: 'group',    icon: 'fa-solid fa-object-group', label: 'Group',         desc: 'Activate, deactivate or toggle a group' },
        { id: 'variable', icon: 'fa-solid fa-pen',           label: 'Set Variable',  desc: 'Update a user variable value' },
        { id: 'http',     icon: 'fa-solid fa-globe',         label: 'HTTP Request',  desc: 'Call a webhook or external service' },
        { id: 'delay',    icon: 'fa-solid fa-hourglass-half',label: 'Delay',         desc: 'Wait before continuing (using domoticz.after)' },
        { id: 'log',      icon: 'fa-solid fa-list',           label: 'Log Message',   desc: 'Write a message to the Domoticz log' },
        { id: 'custom',   icon: 'fa-solid fa-code',          label: 'Custom Code',   desc: 'Write your own dzVents Lua snippet' },
    ];

    var ACTION_OPTIONS = {
        'switch':      [
            { id: 'switchOn',          label: 'Turn On' },
            { id: 'switchOff',         label: 'Turn Off' },
            { id: 'toggleSwitch',      label: 'Toggle' }
        ],
        'dimmer':      [
            { id: 'switchOn',          label: 'Turn On' },
            { id: 'switchOff',         label: 'Turn Off' },
            { id: 'toggleSwitch',      label: 'Toggle' },
            { id: 'dimTo',             label: 'Dim to %',         hasValue: true, unit: '%',  def: 50,  min: 0,   max: 100, step: 1 }
        ],
        'blinds':      [
            { id: 'open',              label: 'Open' },
            { id: 'close',             label: 'Close' },
            { id: 'stop',              label: 'Stop' },
            { id: 'setLevel',          label: 'Set position (%)', hasValue: true, unit: '%',  def: 50,  min: 0,   max: 100, step: 1 }
        ],
        'selector':    [
            { id: 'switchSelector',    label: 'Set level', hasValue: true, unit: '', def: 10, min: 0, step: 10 }
        ],
        'setpoint':    [
            { id: 'updateSetPoint',    label: 'Set to value',     hasValue: true, unit: '',   def: 20,  step: 0.5 }
        ],
        'motion':      [
            { id: 'switchOn',          label: 'Trigger motion' },
            { id: 'switchOff',         label: 'Clear motion' }
        ],
        'door':        [
            { id: 'switchOn',          label: 'Open' },
            { id: 'switchOff',         label: 'Close' }
        ],
        'temperature': [
            { id: 'updateTemperature', label: 'Set temperature',  hasValue: true, unit: '°', def: 20, step: 0.1 }
        ],
        'temphum':     [
            { id: 'updateTempHum',     label: 'Set temperature',  hasValue: true, unit: '°', def: 20, step: 0.1 }
        ],
        'humidity':    [
            { id: 'updateHumidity',    label: 'Set humidity',     hasValue: true, unit: '%',  def: 50,  min: 0, max: 100 }
        ],
        'percentage':  [
            { id: 'updatePercentage',  label: 'Set percentage',   hasValue: true, unit: '%',  def: 0,   min: 0, max: 100 }
        ],
        'counter':     [
            { id: 'updateCounter',     label: 'Set counter',      hasValue: true, unit: '',   def: 0 },
            { id: 'incrementCounter',  label: 'Increment by',     hasValue: true, unit: '',   def: 1, min: 1 }
        ],
        'lux':         [
            { id: 'updateLux',         label: 'Set lux',          hasValue: true, unit: 'lux', def: 0 }
        ],
        'co2':         [
            { id: 'updateAirQuality',  label: 'Set CO₂',     hasValue: true, unit: 'ppm', def: 400 }
        ],
        'custom':      [
            { id: 'updateCustomSensor',label: 'Set value',        hasValue: true, unit: '',   def: 0 }
        ],
        'uv':          [
            { id: 'updateUV',          label: 'Set UV index',     hasValue: true, unit: '',   def: 0, min: 0, step: 0.1 }
        ],
        'energy':      [
            { id: 'updateEnergy',      label: 'Set power (W)',    hasValue: true, unit: 'W',  def: 0, min: 0 }
        ]
    };

    var DEVICE_CONDITIONS = {
        switch:      [{ id: 'any', label: 'Any change' }, { id: 'on', label: 'Turns On' }, { id: 'off', label: 'Turns Off' }],
        dimmer:      [{ id: 'any', label: 'Any change' }, { id: 'on', label: 'Turns On' }, { id: 'off', label: 'Turns Off' },
                      { id: 'level_above', label: 'Level above', hasValue: true, unit: '%', def: 50 },
                      { id: 'level_below', label: 'Level below', hasValue: true, unit: '%', def: 50 }],
        blinds:      [{ id: 'any', label: 'Any change' }, { id: 'open', label: 'Opens' }, { id: 'closed', label: 'Closes' }],
        selector:    [{ id: 'any', label: 'Any change' },
                      { id: 'level_is', label: 'Level is', hasValue: true, unit: '', def: 0 }],
        setpoint:    [{ id: 'any', label: 'Any change' }],
        motion:      [{ id: 'any', label: 'Any change' }, { id: 'on', label: 'Motion detected' }, { id: 'off', label: 'No motion' }],
        door:        [{ id: 'any', label: 'Any change' }, { id: 'open', label: 'Opens' }, { id: 'closed', label: 'Closes' }],
        temperature: [{ id: 'any', label: 'Any change' },
                      { id: 'above', label: 'Above', hasValue: true, unit: '°', def: 20 },
                      { id: 'below', label: 'Below', hasValue: true, unit: '°', def: 20 }],
        temphum:     [{ id: 'any', label: 'Any change' },
                      { id: 'temp_above', label: 'Temperature above', hasValue: true, unit: '°', def: 20 },
                      { id: 'temp_below', label: 'Temperature below', hasValue: true, unit: '°', def: 20 },
                      { id: 'hum_above',  label: 'Humidity above',    hasValue: true, unit: '%', def: 60 },
                      { id: 'hum_below',  label: 'Humidity below',    hasValue: true, unit: '%', def: 60 }],
        humidity:    [{ id: 'any', label: 'Any change' },
                      { id: 'above', label: 'Above', hasValue: true, unit: '%', def: 60 },
                      { id: 'below', label: 'Below', hasValue: true, unit: '%', def: 60 }],
        percentage:  [{ id: 'any', label: 'Any change' },
                      { id: 'above', label: 'Above', hasValue: true, unit: '%', def: 50 },
                      { id: 'below', label: 'Below', hasValue: true, unit: '%', def: 50 }],
        power:       [{ id: 'any', label: 'Any change' },
                      { id: 'above', label: 'Usage above', hasValue: true, unit: 'W', def: 100 },
                      { id: 'below', label: 'Usage below', hasValue: true, unit: 'W', def: 100 }],
        wind:        [{ id: 'any', label: 'Any change' }, { id: 'above', label: 'Speed above', hasValue: true, unit: 'bft', def: 5 }],
        rain:        [{ id: 'any', label: 'Any change' }, { id: 'above', label: 'Rate above', hasValue: true, unit: 'mm/h', def: 5 }],
        uv:          [{ id: 'any', label: 'Any change' }, { id: 'above', label: 'UV index above', hasValue: true, unit: '', def: 5 }],
        co2:         [{ id: 'any', label: 'Any change' },
                      { id: 'above', label: 'CO₂ above', hasValue: true, unit: 'ppm', def: 1000 },
                      { id: 'below', label: 'CO₂ below', hasValue: true, unit: 'ppm', def: 1000 }],
        lux:         [{ id: 'any', label: 'Any change' },
                      { id: 'above', label: 'Above', hasValue: true, unit: 'lux', def: 500 },
                      { id: 'below', label: 'Below', hasValue: true, unit: 'lux', def: 500 }],
        voltage:     [{ id: 'any', label: 'Any change' },
                      { id: 'above', label: 'Above', hasValue: true, unit: 'V', def: 12 },
                      { id: 'below', label: 'Below', hasValue: true, unit: 'V', def: 12 }],
        counter:     [{ id: 'any', label: 'Any change' }, { id: 'above', label: 'Counter above', hasValue: true, unit: '', def: 100 }],
        custom:      [{ id: 'any', label: 'Any change' },
                      { id: 'above', label: 'Value above', hasValue: true, unit: '', def: 0 },
                      { id: 'below', label: 'Value below', hasValue: true, unit: '', def: 0 }],
    };

    var COND_EXPR = {
        on:          "device.state == 'On'",
        off:         "device.state == 'Off'",
        open:        "device.state == 'Open'",
        closed:      "device.state == 'Closed'",
        level_above: 'device.level > {v}',
        level_below: 'device.level < {v}',
        level_is:    'device.level == {v}',
        above: {
            temperature: 'device.temperature > {v}', temphum: 'device.temperature > {v}',
            humidity: 'device.humidity > {v}', percentage: 'device.percentage > {v}',
            power: 'device.usage > {v}', wind: 'device.speed > {v}', rain: 'device.rain > {v}',
            uv: 'device.uv > {v}', co2: 'device.co2 > {v}', lux: 'device.lux > {v}',
            voltage: 'device.voltage > {v}', counter: 'device.counter > {v}',
            custom: 'device.sensorValue > {v}'
        },
        below: {
            temperature: 'device.temperature < {v}', temphum: 'device.temperature < {v}',
            humidity: 'device.humidity < {v}', percentage: 'device.percentage < {v}',
            power: 'device.usage < {v}', co2: 'device.co2 < {v}', lux: 'device.lux < {v}',
            voltage: 'device.voltage < {v}', custom: 'device.sensorValue < {v}'
        },
        temp_above: 'device.temperature > {v}',
        temp_below: 'device.temperature < {v}',
        hum_above:  'device.humidity > {v}',
        hum_below:  'device.humidity < {v}',
    };

    function getSelectorLevels(dev) {
        if (!dev || !dev.LevelNames) return [];
        try {
            var names = atob(dev.LevelNames).split('|');
            var hiddenOff = dev.LevelOffHidden === true || dev.LevelOffHidden === 'true'; // API may return boolean or string
            return names
                .map(function (name, i) { return { level: i * 10, name: name }; })
                .filter(function (opt) { return !(hiddenOff && opt.level === 0); });
        } catch (e) {
            console.error('Failed to parse selector levels:', e);
            return [];
        }
    }

    function defaultLevelValue(levelOptions, fallbackDef) {
        if (levelOptions && levelOptions.length) {
            return levelOptions[0].level;
        }
        return fallbackDef !== undefined ? fallbackDef : 0;
    }

    function getDeviceCategory(dev) {
        if (!dev) return 'switch';
        var type = (dev.Type       || '').toLowerCase();
        var sub  = (dev.SubType    || '').toLowerCase();
        var sw   = (dev.SwitchType || '').toLowerCase();

        if (sw === 'dimmer')             return 'dimmer';
        if (/blind/.test(sw))          return 'blinds';
        if (sw === 'selector')         return 'selector';
        if (sw === 'motion sensor')    return 'motion';
        if (/door/.test(sw))           return 'door';
        if (type === 'thermostat')     return 'setpoint';
        if (sub === 'setpoint')        return 'setpoint';
        if (type === 'temp')           return 'temperature';
        if (/^temp\s*\+\s*hum/.test(type)) return 'temphum';
        if (type === 'humidity')       return 'humidity';
        if (type === 'wind')           return 'wind';
        if (type === 'rain')           return 'rain';
        if (type === 'uv')             return 'uv';
        if (type === 'air quality')    return 'co2';
        if (type === 'lux')            return 'lux';
        if (type === 'usage' && sub === 'electric') return 'energy';
        if (/p1 smart meter/.test(type) || sub === 'kwh') return 'power';
        if (type === 'rfxmeter')       return 'counter';
        if (sub === 'temperature')     return 'temperature';
        if (sub === 'humidity')        return 'humidity';
        if (sub === 'percentage')      return 'percentage';
        if (sub === 'custom sensor')   return 'custom';
        if (sub === 'kwh')             return 'power';
        if (sub === 'voltage')         return 'voltage';
        if (sub === 'lux')             return 'lux';
        return 'switch';
    }

    function luaEsc(s) {
        return String(s).replace(/\\/g, '\\\\').replace(/'/g, "\\'");
    }

    function numVal(v, fallback) { var n = parseFloat(v); return isNaN(n) ? fallback : n; }
    function intVal(v, fallback)  { var n = parseInt(v, 10); return isNaN(n) ? fallback : n; }

    function formatTime(v) {
        if (!v) return '07:00';
        if (v instanceof Date) {
            return ('0' + v.getHours()).slice(-2) + ':' + ('0' + v.getMinutes()).slice(-2);
        }
        var s = String(v);
        if (/^\d{2}:\d{2}$/.test(s)) return s;
        var d = new Date(s);
        if (!isNaN(d)) return ('0' + d.getHours()).slice(-2) + ':' + ('0' + d.getMinutes()).slice(-2);
        return '07:00';
    }

    app.component('automationWizard', {
        bindings: {
            isOpen:    '<',
            onClose:   '&',
            onCreated: '&',
            events:    '<'
        },
        templateUrl: 'app/events/AutomationWizard.html',
        controller: function ($scope, $http, $timeout) {
            var vm = this;

            vm.triggers     = TRIGGERS;
            vm.actionDefs   = ACTIONS;
            vm.stepCount    = 5;
            vm.stepLabels   = ['Trigger', 'Configure', 'Condition', 'Actions', 'Review'];

            vm.step             = 1;
            vm.triggerType      = null;
            vm.triggerConfig    = {};
            vm.actions          = [];
            vm.name             = '';
            vm.enabled          = true;
            vm.showActionPicker = false;
            vm.deviceSearch     = '';
            vm.devices           = [];
            vm.scenes            = [];
            vm.groups            = [];
            vm.variables         = [];
            vm.devicesLoaded     = false;
            vm.scenesLoaded      = false;
            vm.variablesLoaded   = false;
            vm.deviceConditions = DEVICE_CONDITIONS['switch'];
            vm.conditionNeedsValue = false;
            vm.wizardCondition  = {};
            vm.aceEditor        = null;

            $scope.$on('$destroy', function () {
                if (vm.aceEditor) { vm.aceEditor.destroy(); vm.aceEditor = null; }
                var aceEl = document.getElementById('aw-ace-editor');
                if (aceEl) aceEl.innerHTML = '';
            });

            vm.hourOptions = (function () {
                var opts = [];
                for (var i = 0; i < 24; i++) opts.push({ label: ('0' + i).slice(-2), value: i });
                return opts;
            })();
            vm.minuteOptions = (function () {
                var opts = [];
                for (var i = 0; i < 60; i++) opts.push({ label: ('0' + i).slice(-2), value: i });
                return opts;
            })();

            vm.$onChanges = function (changes) {
                if (changes.isOpen && changes.isOpen.currentValue) {
                    resetState();
                    loadDevices();
                }
            };

            function onKeyDown(e) {
                if ((e.key === 'Escape' || e.keyCode === 27) && vm.isOpen) {
                    $scope.$apply(function () { vm.close(); });
                }
            }
            document.addEventListener('keydown', onKeyDown);
            $scope.$on('$destroy', function () {
                document.removeEventListener('keydown', onKeyDown);
            });

            vm.selectTrigger = function (id) {
                if (vm.triggerType !== id) {
                    vm.triggerType   = id;
                    vm.triggerConfig = {};
                    initTriggerDefaults(id);
                }
                vm.goNext();
            };

            function isValidUrl(s) {
                try { var u = new URL(s); return u.protocol === 'http:' || u.protocol === 'https:'; } catch (e) { return false; }
            }

            function isActionValid(a) {
                var c = a.config || {};
                if (a.type === 'http')     return isValidUrl(c.url) && (!c.handleResponse || !!(c.callback && c.callback.trim()));
                if (a.type === 'delay')    return c.seconds !== undefined && c.seconds !== '' && Number(c.seconds) > 0;
                if (a.type === 'custom')   return !!(c.code && c.code.trim());
                if (a.type === 'variable') return !!(c.varName && c.varName.trim()) && c.value !== undefined && c.value !== '';
                if (a.type === 'switch')   return !!(c.deviceIdx && c.action);
                if (a.type === 'scene')    return !!(c.scene && c.action);
                if (a.type === 'group')    return !!(c.group && c.action);
                if (a.type === 'email')    return !!(c.subject && c.subject.trim()) || !!(c.message && c.message.trim());
                if (a.type === 'notify')   return !!(c.title && c.title.trim()) || !!(c.message && c.message.trim());
                if (a.type === 'log')      return !!(c.message && c.message.trim());
                return true;
            }

            vm.isActionValid = isActionValid;

            vm.canGoNext = function () {
                if (vm.step === 1) return !!vm.triggerType;
                if (vm.step === 2) {
                    if (vm.triggerType === 'device')   return !!(vm.triggerConfig.devices && vm.triggerConfig.devices.length);
                    if (vm.triggerType === 'scene')    return !!(vm.triggerConfig.scenes && vm.triggerConfig.scenes.length);
                    if (vm.triggerType === 'group')    return !!(vm.triggerConfig.groups && vm.triggerConfig.groups.length);
                    if (vm.triggerType === 'variable') return !!vm.triggerConfig.varName;
                    if (vm.triggerType === 'system')   return !!(vm.triggerConfig.onStart || vm.triggerConfig.onStop);
                }
                if (vm.step === 3) return true;
                if (vm.step === 4) return vm.actions.length > 0 && vm.actions.every(isActionValid);
                return true;
            };

            vm.goBack = function () {
                if (vm.step > 1) { vm.step--; }
            };

            vm.goNext = function () {
                if (!vm.canGoNext()) return;
                vm.step++;
                if (vm.step === 2 && vm.triggerType === 'device') {
                    $timeout(function () {
                        var el = document.getElementById('aw-device-search');
                        if (el) el.focus();
                    });
                }
                if (vm.step === 2 && vm.triggerType === 'variable') {
                    loadVariables();
                }
                if (vm.step === 2 && (vm.triggerType === 'scene' || vm.triggerType === 'group')) {
                    loadScenes();
                }
                if (vm.step === 3) {
                    if (!vm.wizardCondition.conditionStates || !vm.wizardCondition.conditionStates.length) {
                        initWizardCondition();
                    }
                    if (!vm.devicesLoaded) loadDevices();
                }
                if (vm.step === 4) {
                    loadScenes();
                }
                if (vm.step === 5) {
                    if (!vm.name) vm.name = defaultName();
                    vm.generatedCode = generateCode();
                    $timeout(initAceEditor);
                }
            };

            vm.close = function () {
                vm.onClose();
            };

            vm.selectDevice = function (dev) {
                var arr = vm.triggerConfig.devices;
                var idx = arr.indexOf(dev.idx);
                if (idx >= 0) {
                    arr.splice(idx, 1);
                } else {
                    arr.push(dev.idx);
                    vm.triggerConfig.deviceCategory = getDeviceCategory(dev);
                    vm.triggerConfig.conditionLevelOptions = getSelectorLevels(dev);
                    vm.deviceConditions             = DEVICE_CONDITIONS[vm.triggerConfig.deviceCategory] || DEVICE_CONDITIONS['switch'];
                    var valid = vm.deviceConditions.some(function (c) { return c.id === vm.triggerConfig.condition; });
                    if (!valid) { vm.triggerConfig.condition = 'any'; }
                    vm.updateConditionValue();
                }
            };

            vm.isDeviceSelected = function (dev) {
                return vm.triggerConfig.devices && vm.triggerConfig.devices.indexOf(dev.idx) >= 0;
            };

            vm.deviceLabel = function (dev) {
                if (!dev) return '';
                var meta = dev.Type || '';
                if (dev.SubType) meta += (meta ? '/' : '') + dev.SubType;
                return meta ? (dev.Name + ' (' + meta + ')') : dev.Name;
            };

            function findDeviceByIdx(idx) {
                return vm.devices.find(function (d) { return d.idx === idx; });
            }

            function deviceNameDuplicated(name) {
                return vm.devices.filter(function (d) { return d.Name === name; }).length > 1;
            }

            vm.deviceRef = function (idx) {
                var dev = findDeviceByIdx(idx);
                if (!dev) return "domoticz.devices('Device')";
                return deviceNameDuplicated(dev.Name) ? ('domoticz.devices(' + dev.idx + ')')
                                                      : ("domoticz.devices('" + luaEsc(dev.Name) + "')");
            };

            vm.deviceNameByIdx = function (idx) {
                var dev = findDeviceByIdx(idx);
                return dev ? dev.Name : (idx || '');
            };

            vm.deviceOnToken = function (idx) {
                var dev = findDeviceByIdx(idx);
                if (!dev) return "''";
                return deviceNameDuplicated(dev.Name) ? String(dev.idx)
                                                      : ("'" + luaEsc(dev.Name) + "'");
            };

            vm.toggleTriggerScene = function (sc) {
                var arr = vm.triggerConfig.scenes;
                var idx = arr.indexOf(sc.Name);
                if (idx >= 0) arr.splice(idx, 1); else arr.push(sc.Name);
            };
            vm.isTriggerSceneSelected = function (sc) {
                return vm.triggerConfig.scenes && vm.triggerConfig.scenes.indexOf(sc.Name) >= 0;
            };
            vm.toggleTriggerGroup = function (gr) {
                var arr = vm.triggerConfig.groups;
                var idx = arr.indexOf(gr.Name);
                if (idx >= 0) arr.splice(idx, 1); else arr.push(gr.Name);
            };
            vm.isTriggerGroupSelected = function (gr) {
                return vm.triggerConfig.groups && vm.triggerConfig.groups.indexOf(gr.Name) >= 0;
            };

            vm.setDayPreset = function (preset) {
                var days = vm.triggerConfig.days;
                var all = ['mon','tue','wed','thu','fri','sat','sun'];
                all.forEach(function(k) { days[k] = false; });
                if (preset === 'weekdays') { ['mon','tue','wed','thu','fri'].forEach(function(k) { days[k] = true; }); }
                else if (preset === 'weekend') { ['sat','sun'].forEach(function(k) { days[k] = true; }); }
            };

            vm.setConditionDayPreset = function (preset) {
                var days = vm.wizardCondition.days;
                var all = ['mon','tue','wed','thu','fri','sat','sun'];
                all.forEach(function(k) { days[k] = false; });
                if (preset === 'weekdays') { ['mon','tue','wed','thu','fri'].forEach(function(k) { days[k] = true; }); }
                else if (preset === 'weekend') { ['sat','sun'].forEach(function(k) { days[k] = true; }); }
            };

            vm.onConditionChange = function () {
                vm.updateConditionValue();
            };

            vm.updateConditionValue = function () {
                var cond = vm.deviceConditions.find(function (c) { return c.id === vm.triggerConfig.condition; }) || {};
                vm.conditionNeedsValue = !!cond.hasValue;
                vm.conditionUnit       = cond.unit || '';
                if (cond.hasValue && vm.triggerConfig.conditionValue == null) {
                    vm.triggerConfig.conditionValue = defaultLevelValue(vm.triggerConfig.conditionLevelOptions, cond.def);
                }
                if (!cond.hasValue) {
                    vm.triggerConfig.conditionValue = undefined;
                }
            };

            vm.onConditionTypeChange = function () {
                if (vm.wizardCondition.type === 'device' && vm.wizardCondition.deviceIdx) {
                    vm.onConditionDeviceChange();
                }
            };

            vm.onConditionDeviceChange = function () {
                var dev = findDeviceByIdx(vm.wizardCondition.deviceIdx);
                vm.wizardCondition.category = getDeviceCategory(dev);
                vm.wizardCondition.levelOptions = getSelectorLevels(dev);
                vm.wizardCondition.conditionStates = DEVICE_CONDITIONS[vm.wizardCondition.category] || DEVICE_CONDITIONS['switch'];
                var valid = vm.wizardCondition.conditionStates.some(function (c) { return c.id === vm.wizardCondition.state; });
                if (!valid) vm.wizardCondition.state = 'any';
                var sel = vm.wizardCondition.conditionStates.find(function (c) { return c.id === vm.wizardCondition.state; }) || {};
                vm.wizardCondition.needsValue = !!sel.hasValue;
                vm.wizardCondition.unit = sel.unit || '';
                if (sel.hasValue && vm.wizardCondition.value == null) {
                    vm.wizardCondition.value = defaultLevelValue(vm.wizardCondition.levelOptions, sel.def);
                }
            };

            vm.onConditionStateChange = function () {
                var sel = vm.wizardCondition.conditionStates.find(function (c) { return c.id === vm.wizardCondition.state; }) || {};
                vm.wizardCondition.needsValue = !!sel.hasValue;
                vm.wizardCondition.unit = sel.unit || '';
                if (sel.hasValue && vm.wizardCondition.value == null) {
                    vm.wizardCondition.value = defaultLevelValue(vm.wizardCondition.levelOptions, sel.def);
                }
                if (!sel.hasValue) vm.wizardCondition.value = null;
            };

            vm.openActionPicker = function () { vm.showActionPicker = true; };
            vm.closeActionPicker = function () { vm.showActionPicker = false; };

            vm.pickAction = function (actionDef) {
                var config = {};
                if (actionDef.id === 'switch') {
                    config.action = 'switchOn';
                    config.deviceIdx = '';
                    config.deviceCategory = 'switch';
                }
                if (actionDef.id === 'notify') {
                    config.priority = 'PRIORITY_NORMAL';
                }
                if (actionDef.id === 'delay') {
                    config.seconds = 5;
                }
                if (actionDef.id === 'http') {
                    config.method = 'GET';
                    config.handleResponse = false;
                    config.callback = 'myCallback';
                }
                if (actionDef.id === 'variable') {
                    loadVariables();
                }
                if (actionDef.id === 'group') { config.action = 'on'; }
                if (actionDef.id === 'log')   { config.level = 'LOG_INFO'; }
                vm.actions.push({ type: actionDef.id, config: config });
                vm.showActionPicker = false;
                loadScenes();
            };

            vm.removeAction = function (idx) {
                vm.actions.splice(idx, 1);
            };

            vm.getActionDef = function (type) {
                return ACTIONS.find(function (a) { return a.id === type; }) || { icon: 'icon-cog', label: type };
            };

            vm.onActionDeviceChange = function (action) {
                var dev = findDeviceByIdx(action.config.deviceIdx);
                action.config.deviceCategory = getDeviceCategory(dev);
                action.config.levelOptions = null;

                if (action.config.deviceCategory === 'selector') {
                    action.config.levelOptions = getSelectorLevels(dev);
                    if (action.config.levelOptions.length) {
                        action.config.actionValue = action.config.levelOptions[0].level;
                    }
                }

                var opts = ACTION_OPTIONS[action.config.deviceCategory] || ACTION_OPTIONS['switch'];
                var valid = opts.some(function (o) { return o.id === action.config.action; });
                if (!valid) {
                    action.config.action = opts[0] ? opts[0].id : '';
                    if (!action.config.levelOptions) { action.config.actionValue = undefined; }
                }
            };

            vm.actionHasOptions = function (action) {
                return !!(action.config && action.config.levelOptions && action.config.levelOptions.length);
            };

            vm.triggerConditionHasLevelOptions = function () {
                var opts = vm.triggerConfig.conditionLevelOptions;
                return vm.triggerConfig.deviceCategory === 'selector' && !!(opts && opts.length);
            };

            vm.wizardConditionHasLevelOptions = function () {
                var opts = vm.wizardCondition.levelOptions;
                return !!(opts && opts.length);
            };

            vm.getDeviceActionOptions = function (action) {
                var cat = (action.config && action.config.deviceCategory) || 'switch';
                return ACTION_OPTIONS[cat] || ACTION_OPTIONS['switch'];
            };

            vm.getSelectedActionOption = function (action) {
                var opts = vm.getDeviceActionOptions(action);
                return opts.find(function (o) { return o.id === action.config.action; }) || {};
            };

            vm.actionNeedsValue = function (action) {
                return !!vm.getSelectedActionOption(action).hasValue;
            };

            vm.actionValueUnit = function (action) {
                return vm.getSelectedActionOption(action).unit || '';
            };

            vm.actionValueLabel = function (action) {
                return 'Value';
            };

            vm.nameIsDuplicate = function () {
                if (!vm.name || !vm.events || !vm.events.length) return false;
                var n = vm.name.trim().toLowerCase();
                return vm.events.some(function (e) { return e.name.toLowerCase() === n; });
            };

            vm.onNameChange = function () {
                vm.generatedCode = generateCode();
                if (vm.aceEditor) vm.aceEditor.setValue(vm.generatedCode, -1);
            };

            vm.createAutomation = function () {
                var code = vm.generatedCode || generateCode();
                var name = (vm.name || 'MyAutomation').trim();
                var event = {
                    id:           name,
                    eventstatus:  vm.enabled ? '1' : '0',
                    name:         name,
                    interpreter:  'dzVents',
                    type:         'All',
                    xmlstatement: code,
                    logicarray:   '',
                    isChanged:    true,
                    isNew:        true
                };
                vm.onCreated({ event: event });
                vm.onClose();
            };

            function resetState() {
                vm.step             = 1;
                vm.triggerType      = null;
                vm.triggerConfig    = {};
                vm.actions          = [];
                vm.name             = '';
                vm.enabled          = true;
                vm.deviceSearch     = '';
                vm.showActionPicker = false;
                vm.deviceConditions = DEVICE_CONDITIONS['switch'];
                vm.conditionNeedsValue = false;
                vm.generatedCode    = '';
                vm.wizardCondition  = {};
                if (vm.aceEditor) { vm.aceEditor.destroy(); vm.aceEditor = null; }
                var aceEl = document.getElementById('aw-ace-editor');
                if (aceEl) aceEl.innerHTML = '';
            }

            function initWizardCondition() {
                vm.wizardCondition = {
                    type: 'none',
                    deviceIdx: '',
                    category: 'switch',
                    conditionStates: [],
                    state: 'any',
                    value: null,
                    fromHour: 8,
                    fromMin: 0,
                    toHour: 22,
                    toMin: 0,
                    days: { mon: true, tue: true, wed: true, thu: true, fri: true, sat: false, sun: false },
                    dayPart: 'daytime'
                };
            }

            function initAceEditor() {
                require(['ace'], function () {
                    $timeout(function () {
                        var el = document.getElementById('aw-ace-editor');
                        if (!el) return;
                        if (vm.aceEditor) {
                            vm.aceEditor.setValue(vm.generatedCode || '', -1);
                            return;
                        }
                        ace.config.set('workerPath', '../js/ace');
                        var isDark = !document.body.classList.contains('dz-light');
                        var storedTheme = localStorage.getItem('dz-ace-theme');
                        var theme = storedTheme || (isDark ? 'ace/theme/tomorrow_night_blue' : 'ace/theme/github');
                        var editor = ace.edit(el);
                        editor.$blockScrolling = Infinity;
                        editor.setTheme(theme);
                        editor.session.setMode('ace/mode/lua');
                        editor.setOption('showPrintMargin', false);
                        editor.setOption('fontSize', '13px');
                        editor.setValue(vm.generatedCode || '', -1);
                        editor.on('change', function () {
                            $scope.$applyAsync(function () {
                                vm.generatedCode = editor.getValue();
                            });
                        });
                        vm.aceEditor = editor;
                    });
                });
            }

            function initTriggerDefaults(type) {
                var tc = vm.triggerConfig;
                if (type === 'time')     { tc.hour = tc.hour != null ? tc.hour : 7; tc.min = tc.min != null ? tc.min : 0; tc.days = tc.days || { mon: false, tue: false, wed: false, thu: false, fri: false, sat: false, sun: false }; }
                if (type === 'sun')      { tc.event = tc.event || 'sunrise'; tc.offset = tc.offset != null ? tc.offset : 0; }
                if (type === 'interval') { tc.value = tc.value || 5; tc.unit = tc.unit || 'minutes'; }
                if (type === 'security') { tc.state = tc.state || 'SECURITY_ARMEDAWAY'; }
                if (type === 'device')   { tc.devices = tc.devices || []; tc.condition = tc.condition || 'any'; tc.deviceCategory = tc.deviceCategory || 'switch'; }
                if (type === 'scene')    { tc.scenes = tc.scenes || []; }
                if (type === 'group')    { tc.groups = tc.groups || []; }
                if (type === 'system')   { tc.onStart = tc.onStart !== false; tc.onStop = tc.onStop || false; }
            }

            function defaultName() {
                var t = TRIGGERS.find(function (x) { return x.id === vm.triggerType; });
                var label = t ? t.label : 'Automation';
                if (vm.triggerType === 'device' && vm.triggerConfig.devices && vm.triggerConfig.devices.length)
                    return vm.deviceNameByIdx(vm.triggerConfig.devices[0]) + ' automation';
                if (vm.triggerType === 'scene' && vm.triggerConfig.scenes && vm.triggerConfig.scenes.length)
                    return vm.triggerConfig.scenes[0] + ' automation';
                if (vm.triggerType === 'group' && vm.triggerConfig.groups && vm.triggerConfig.groups.length)
                    return vm.triggerConfig.groups[0] + ' automation';
                return label + ' automation';
            }

            function loadDevices() {
                if (vm.devicesLoaded) return;
                $http.get('json.htm?type=command&param=getdevices&filter=all&used=true&order=Name')
                    .then(function (resp) {
                        var list = (resp.data && resp.data.result) ? resp.data.result : [];
                        list.sort(function (a, b) {
                            return (a.Name || '').toLowerCase().localeCompare((b.Name || '').toLowerCase());
                        });
                        vm.devices       = list;
                        vm.devicesLoaded = true;
                    })
                    .catch(function () { vm.devices = []; vm.devicesLoaded = true; });
            }

            function loadScenes() {
                if (vm.scenesLoaded) return;
                $http.get('json.htm?type=command&param=getscenes')
                    .then(function (resp) {
                        var list = (resp.data && resp.data.result) ? resp.data.result : [];
                        vm.scenes       = list.filter(function(s) { return s.Type === 'Scene'; });
                        vm.groups       = list.filter(function(s) { return s.Type === 'Group'; });
                        vm.scenesLoaded = true;
                    })
                    .catch(function () { vm.scenes = []; vm.groups = []; vm.scenesLoaded = true; });
            }

            function loadVariables() {
                if (vm.variablesLoaded) return;
                $http.get('json.htm?type=command&param=getuservariables')
                    .then(function (resp) {
                        var list = (resp.data && resp.data.result) ? resp.data.result : [];
                        list.sort(function (a, b) {
                            return (a.Name || '').toLowerCase().localeCompare((b.Name || '').toLowerCase());
                        });
                        vm.variables       = list;
                        vm.variablesLoaded = true;
                    })
                    .catch(function () { vm.variables = []; vm.variablesLoaded = true; });
            }

            function generateCode() {
                var i4 = '    ', i8 = '        ', i12 = '            ';
                var t  = vm.triggerType;
                var tc = vm.triggerConfig;
                var L  = [];

                L.push('return {');
                L.push(i4 + 'active = true,');
                L.push(i4 + 'logging = {');
                L.push(i8 + 'level = domoticz.LOG_INFO,');
                L.push(i8 + "marker = '" + luaEsc(vm.name || 'MyAutomation') + "'");
                L.push(i4 + '},');
                L.push(i4 + 'on = {');

                function pushLuaArray(key, items) {
                    L.push(i8 + key + ' = {');
                    items.forEach(function(name, idx) {
                        L.push(i12 + "'" + luaEsc(name) + "'" + (idx < items.length - 1 ? ',' : ''));
                    });
                    L.push(i8 + '},');
                }

                function pushLuaTokens(key, tokens) {
                    L.push(i8 + key + ' = {');
                    tokens.forEach(function(tok, idx) {
                        L.push(i12 + tok + (idx < tokens.length - 1 ? ',' : ''));
                    });
                    L.push(i8 + '},');
                }

                if (t === 'device') {
                    pushLuaTokens('devices', (tc.devices || []).map(function (idx) { return vm.deviceOnToken(idx); }));
                } else if (t === 'scene') {
                    pushLuaArray('scenes', tc.scenes || []);
                } else if (t === 'group') {
                    pushLuaArray('groups', tc.groups || []);
                } else if (t === 'time') {
                    var DAY_KEYS = ['mon','tue','wed','thu','fri','sat','sun'];
                    var selectedDays = DAY_KEYS.filter(function(d) { return tc.days && tc.days[d]; });
                    var daysStr = '';
                    if (selectedDays.length > 0 && selectedDays.length < 7) {
                        if (selectedDays.join(',') === 'mon,tue,wed,thu,fri') daysStr = ' on weekdays';
                        else if (selectedDays.join(',') === 'sat,sun') daysStr = ' on weekends';
                        else daysStr = ' on ' + selectedDays.join(',');
                    }
                    L.push(i8 + 'timer = {');
                    var timeStr = ('0' + (tc.hour || 0)).slice(-2) + ':' + ('0' + (tc.min || 0)).slice(-2);
                    L.push(i12 + "'at " + timeStr + daysStr + "'");
                    L.push(i8 + '},');
                } else if (t === 'sun') {
                    var ev  = tc.event || 'sunrise';
                    var off = intVal(tc.offset, 0);
                    var sun = off === 0 ? 'at ' + ev
                            : Math.abs(off) + ' minutes ' + (off < 0 ? 'before' : 'after') + ' ' + ev;
                    L.push(i8 + 'timer = {');
                    L.push(i12 + "'" + sun + "'");
                    L.push(i8 + '},');
                } else if (t === 'interval') {
                    var n  = intVal(tc.value, 5);
                    var un = tc.unit || 'minutes';
                    var iv = n === 1 ? ('every ' + (un === 'hours' ? 'hour' : 'minute'))
                                     : ('every ' + n + ' ' + un);
                    L.push(i8 + 'timer = {');
                    L.push(i12 + "'" + iv + "'");
                    L.push(i8 + '},');
                } else if (t === 'security') {
                    L.push(i8 + 'security = {');
                    L.push(i12 + 'domoticz.' + (tc.state || 'SECURITY_ARMEDAWAY'));
                    L.push(i8 + '},');
                } else if (t === 'variable') {
                    L.push(i8 + 'variables = {');
                    L.push(i12 + "'" + luaEsc(tc.varName || 'YourVariable') + "'");
                    L.push(i8 + '},');
                } else if (t === 'system') {
                    var evts = [];
                    if (tc.onStart) evts.push("'start'");
                    if (tc.onStop)  evts.push("'stop'");
                    if (!evts.length) evts.push("'start'");
                    L.push(i8 + 'system = {');
                    evts.forEach(function(e) { L.push(i12 + e); });
                    L.push(i8 + '},');
                }

                var httpCallbacks = vm.actions
                    .filter(function(a) { return a.type === 'http' && a.config && a.config.handleResponse && a.config.callback && a.config.callback.trim(); })
                    .map(function(a) { return a.config.callback.trim(); });
                if (httpCallbacks.length) {
                    L.push(i8 + 'httpResponses = {');
                    httpCallbacks.forEach(function(cb) { L.push(i12 + "'" + luaEsc(cb) + "'"); });
                    L.push(i8 + '},');
                }

                L.push(i4 + '},');

                var param = (httpCallbacks.length || t === 'system') ? 'item'
                          : t === 'device'   ? 'device'
                          : t === 'variable' ? 'variable'
                          : t === 'security' ? 'security'
                          : t === 'scene'    ? 'scene'
                          : t === 'group'    ? 'group'
                          : 'timer';
                L.push(i4 + 'execute = function(domoticz, ' + param + ')');

                var condExprs = [];
                if (t === 'device' && tc.condition && tc.condition !== 'any' && tc.devices && tc.devices.length) {
                    var expr1 = COND_EXPR[tc.condition];
                    if (typeof expr1 === 'object') expr1 = expr1[tc.deviceCategory || 'switch'];
                    if (expr1) condExprs.push(expr1.replace('{v}', tc.conditionValue != null ? tc.conditionValue : 0));
                }
                var wc = vm.wizardCondition;
                if (wc && wc.type === 'device' && wc.deviceIdx && wc.state && wc.state !== 'any') {
                    var wcExpr = COND_EXPR[wc.state];
                    if (typeof wcExpr === 'object') wcExpr = wcExpr[wc.category || 'switch'];
                    if (wcExpr) {
                        wcExpr = wcExpr.replace(/\bdevice\./g, vm.deviceRef(wc.deviceIdx) + '.');
                        condExprs.push(wcExpr.replace('{v}', wc.value != null ? wc.value : 0));
                    }
                } else if (wc && wc.type === 'time') {
                    var from = ('0'+(wc.fromHour != null ? wc.fromHour : 8)).slice(-2)+':'+('0'+(wc.fromMin != null ? wc.fromMin : 0)).slice(-2);
                    var to   = ('0'+(wc.toHour   != null ? wc.toHour   : 22)).slice(-2)+':'+('0'+(wc.toMin   != null ? wc.toMin   : 0)).slice(-2);
                    condExprs.push("domoticz.time.matchesRule('between " + from + " and " + to + "')");
                } else if (wc && wc.type === 'days') {
                    var DAY_KEYS2 = ['mon','tue','wed','thu','fri','sat','sun'];
                    var selDays = DAY_KEYS2.filter(function(d) { return wc.days && wc.days[d]; });
                    if (selDays.length && selDays.length < 7) {
                        var dayRule = selDays.join(',') === 'mon,tue,wed,thu,fri' ? 'weekdays'
                                    : selDays.join(',') === 'sat,sun'             ? 'weekends'
                                    : selDays.join(',');
                        condExprs.push("domoticz.time.matchesRule('at * on " + dayRule + "')");
                    }
                } else if (wc && wc.type === 'daytime') {
                    condExprs.push(wc.dayPart === 'nighttime' ? 'domoticz.time.isNightTime' : 'domoticz.time.isDayTime');
                }

                var i16 = i12 + i4;
                var i20 = i16 + i4;
                if (httpCallbacks.length) {
                    L.push(i8 + 'if (item.isHTTPResponse) then');
                    httpCallbacks.forEach(function (cb, idx) {
                        var kw = idx === 0 ? 'if' : 'elseif';
                        L.push(i12 + kw + " (item.trigger == '" + luaEsc(cb) + "') then");
                        L.push(i16 + 'if (item.ok) then');
                        L.push(i20 + '-- handle response: item.json, item.data, item.statusCode');
                        L.push(i16 + 'end');
                    });
                    L.push(i12 + 'end');
                    L.push(i8 + 'else');
                }

                var combinedCond = condExprs.length ? i8 + 'if (' + condExprs.join(' and ') + ') then' : null;
                var bi = httpCallbacks.length ? i12 : i8;
                if (combinedCond) {
                    L.push(httpCallbacks.length ? i12 + combinedCond.trimStart() : combinedCond);
                    bi = httpCallbacks.length ? i16 : i12;
                }

                if (!vm.actions.length) {
                    L.push(bi + '-- Add your actions here');
                } else {
                    vm.actions.forEach(function (a) {
                        var c = a.config || {};
                        if (a.type === 'switch') {
                            var act = c.action || 'switchOn';
                            var dev = vm.deviceRef(c.deviceIdx);
                            if (act === 'dimTo') {
                                L.push(bi + dev + ".dimTo(" + numVal(c.actionValue, 50) + ")");
                            } else if (act === 'updateSetPoint') {
                                L.push(bi + dev + ".updateSetPoint(" + numVal(c.actionValue, 20) + ")");
                            } else if (act === 'switchSelector') {
                                L.push(bi + dev + ".switchSelector(" + intVal(c.actionValue, 0) + ")");
                            } else if (act === 'updateTemperature') {
                                L.push(bi + dev + ".updateTemperature(" + numVal(c.actionValue, 20) + ")");
                            } else if (act === 'updateTempHum') {
                                L.push(bi + dev + ".updateTempHum(" + numVal(c.actionValue, 20) + ", 50, domoticz.HUM_COMPUTE)");
                            } else if (act === 'updateHumidity') {
                                L.push(bi + dev + ".updateHumidity(" + intVal(c.actionValue, 50) + ", domoticz.HUM_NORMAL)");
                            } else if (act === 'updatePercentage') {
                                L.push(bi + dev + ".updatePercentage(" + numVal(c.actionValue, 0) + ")");
                            } else if (act === 'updateCounter') {
                                L.push(bi + dev + ".updateCounter(" + intVal(c.actionValue, 0) + ")");
                            } else if (act === 'updateLux') {
                                L.push(bi + dev + ".updateLux(" + intVal(c.actionValue, 0) + ")");
                            } else if (act === 'updateAirQuality') {
                                L.push(bi + dev + ".updateAirQuality(" + intVal(c.actionValue, 400) + ")");
                            } else if (act === 'updateCustomSensor') {
                                L.push(bi + dev + ".updateCustomSensor(" + numVal(c.actionValue, 0) + ")");
                            } else if (act === 'setLevel') {
                                L.push(bi + dev + ".setLevel(" + intVal(c.actionValue, 50) + ")");
                            } else if (act === 'incrementCounter') {
                                L.push(bi + dev + ".incrementCounter(" + intVal(c.actionValue, 1) + ")");
                            } else if (act === 'updateUV') {
                                L.push(bi + dev + ".updateUV(" + numVal(c.actionValue, 0) + ")");
                            } else if (act === 'updateEnergy') {
                                L.push(bi + dev + ".updateEnergy(" + numVal(c.actionValue, 0) + ")");
                            } else {
                                L.push(bi + dev + "." + act + "()");
                            }
                        } else if (a.type === 'group') {
                            var ga = c.action === 'off' ? 'switchOff' : c.action === 'toggle' ? 'toggleGroup' : 'switchOn';
                            L.push(bi + "domoticz.groups('" + luaEsc(c.group || 'Group') + "')." + ga + "()");
                        } else if (a.type === 'email') {
                            var mailArgs = "'" + luaEsc(c.subject || '') + "', '" + luaEsc(c.message || '') + "'";
                            if (c.to && c.to.trim()) mailArgs += ", '" + luaEsc(c.to.trim()) + "'";
                            L.push(bi + 'domoticz.email(' + mailArgs + ')');
                        } else if (a.type === 'notify') {
                            L.push(bi + "domoticz.notify('" + luaEsc(c.title || 'Alert') + "', '" + luaEsc(c.message || '') + "', domoticz." + (c.priority || 'PRIORITY_NORMAL') + ")");
                        } else if (a.type === 'scene') {
                            var sa = c.action === 'off' ? 'deActivate' : 'activate';
                            L.push(bi + "domoticz.scenes('" + luaEsc(c.scene || 'Scene') + "')." + sa + "()");
                        } else if (a.type === 'variable') {
                            var vv  = (c.value !== undefined && c.value !== '') ? c.value : '0';
                            var vvs = isNaN(Number(vv)) ? "'" + luaEsc(String(vv)) + "'" : vv;
                            L.push(bi + "domoticz.variables('" + luaEsc(c.varName || 'MyVar') + "').set(" + vvs + ")");
                        } else if (a.type === 'http') {
                            L.push(bi + 'domoticz.openURL({');
                            L.push(bi + i4 + "url = '" + luaEsc(c.url || 'https://example.com') + "',");
                            L.push(bi + i4 + "method = '" + luaEsc(c.method || 'GET') + "'" + (c.handleResponse && c.callback ? ',' : ''));
                            if (c.handleResponse && c.callback && c.callback.trim()) {
                                L.push(bi + i4 + "callback = '" + luaEsc(c.callback.trim()) + "'");
                            }
                            L.push(bi + '})');
                        } else if (a.type === 'delay') {
                            var secs = intVal(c.seconds, 5);
                            L.push(bi + 'domoticz.after(' + secs + ', function()');
                            L.push(bi + i4 + '-- actions after delay go here');
                            L.push(bi + 'end)');
                        } else if (a.type === 'log') {
                            L.push(bi + "domoticz.log('" + luaEsc(c.message || '') + "', domoticz." + (c.level || 'LOG_INFO') + ")");
                        } else if (a.type === 'custom') {
                            (c.code || '-- your code here').split('\n').forEach(function (line) { L.push(bi + line); });
                        }
                    });
                }

                if (combinedCond) L.push((httpCallbacks.length ? i12 : i8) + 'end');
                if (httpCallbacks.length) L.push(i8 + 'end');
                L.push(i4 + 'end');
                L.push('}');
                return L.join('\n');
            }
        }
    });
});
