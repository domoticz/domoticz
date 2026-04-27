define(['app'], function (app) {
    'use strict';

    var TRIGGERS = [
        { id: 'device',   icon: 'icon-off',      label: 'Device State',  desc: 'When a device turns on, off, or changes value' },
        { id: 'time',     icon: 'icon-time',      label: 'Time Schedule', desc: 'At a specific time with optional day filter' },
        { id: 'sun',      icon: 'icon-star',      label: 'Sun Event',     desc: 'At sunrise or sunset with optional offset' },
        { id: 'interval', icon: 'icon-refresh',   label: 'Interval',      desc: 'Repeat every N minutes or hours' },
        { id: 'security', icon: 'icon-lock',      label: 'Security',      desc: 'When the security panel state changes' },
        { id: 'variable', icon: 'icon-list-alt',  label: 'Variable',      desc: 'When a user variable is updated' },
    ];

    var ACTIONS = [
        { id: 'switch',   icon: 'icon-off',       label: 'Switch Device', desc: 'Turn on, off, toggle or dim a device' },
        { id: 'notify',   icon: 'icon-bell',       label: 'Notification',  desc: 'Send a push notification or alert' },
        { id: 'scene',    icon: 'icon-th-large',   label: 'Scene',         desc: 'Activate or deactivate a scene' },
        { id: 'variable', icon: 'icon-edit',       label: 'Set Variable',  desc: 'Update a user variable value' },
        { id: 'http',     icon: 'icon-globe',      label: 'HTTP Request',  desc: 'Call a webhook or external service' },
        { id: 'custom',   icon: 'icon-file',       label: 'Custom Code',   desc: 'Write your own dzVents Lua snippet' },
    ];

    var DEVICE_CONDITIONS = {
        switch:      [{ id: 'any', label: 'Any change' }, { id: 'on', label: 'Turns On' }, { id: 'off', label: 'Turns Off' }],
        dimmer:      [{ id: 'any', label: 'Any change' }, { id: 'on', label: 'Turns On' }, { id: 'off', label: 'Turns Off' },
                      { id: 'level_above', label: 'Level above', hasValue: true, unit: '%', def: 50 },
                      { id: 'level_below', label: 'Level below', hasValue: true, unit: '%', def: 50 }],
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
                      { id: 'above', label: 'CO\u2082 above', hasValue: true, unit: 'ppm', def: 1000 },
                      { id: 'below', label: 'CO\u2082 below', hasValue: true, unit: 'ppm', def: 1000 }],
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

    function getDeviceCategory(dev) {
        if (!dev) return 'switch';
        var type = (dev.Type       || '').toLowerCase();
        var sub  = (dev.SubType    || '').toLowerCase();
        var sw   = (dev.SwitchType || '').toLowerCase();

        if (sw === 'dimmer' || sw === 'blinds' || sw === 'venetian blinds') return 'dimmer';
        if (sw === 'motion sensor')    return 'motion';
        if (/door/.test(sw))           return 'door';
        if (type === 'temp')           return 'temperature';
        if (/^temp\s*\+\s*hum/.test(type)) return 'temphum';
        if (type === 'humidity')       return 'humidity';
        if (type === 'wind')           return 'wind';
        if (type === 'rain')           return 'rain';
        if (type === 'uv')             return 'uv';
        if (type === 'air quality')    return 'co2';
        if (type === 'lux')            return 'lux';
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

    app.component('automationWizard', {
        bindings: {
            isOpen:    '<',
            onClose:   '&',
            onCreated: '&'
        },
        templateUrl: 'app/events/AutomationWizard.html',
        controller: function ($scope, $http, $timeout) {
            var vm = this;

            vm.triggers     = TRIGGERS;
            vm.actionDefs   = ACTIONS;
            vm.stepCount    = 4;
            vm.stepLabels   = ['Trigger', 'Configure', 'Actions', 'Review'];

            // — public state —
            vm.step             = 1;
            vm.triggerType      = null;
            vm.triggerConfig    = {};
            vm.actions          = [];
            vm.name             = '';
            vm.showActionPicker = false;
            vm.deviceSearch     = '';
            vm.devices          = [];
            vm.scenes           = [];
            vm.devicesLoaded    = false;
            vm.scenesLoaded     = false;
            vm.deviceConditions = DEVICE_CONDITIONS['switch'];
            vm.conditionNeedsValue = false;

            // — lifecycle —
            vm.$onChanges = function (changes) {
                if (changes.isOpen && changes.isOpen.currentValue) {
                    resetState();
                    loadDevices();
                }
            };

            // — navigation —
            vm.selectTrigger = function (id) {
                if (vm.triggerType !== id) {
                    vm.triggerType   = id;
                    vm.triggerConfig = {};
                    initTriggerDefaults(id);
                }
            };

            vm.canGoNext = function () {
                if (vm.step === 1) return !!vm.triggerType;
                if (vm.step === 2 && vm.triggerType === 'device') return !!vm.triggerConfig.device;
                return true;
            };

            vm.goBack = function () {
                if (vm.step > 1) { vm.step--; }
            };

            vm.goNext = function () {
                if (!vm.canGoNext()) return;
                if (vm.step === 3) loadScenes(); // pre-load for action scene pickers
                vm.step++;
                if (vm.step === 4) {
                    vm.generatedCode = generateCode();
                    // Seed name from trigger if still empty
                    if (!vm.name) vm.name = defaultName();
                    vm.generatedCode = generateCode(); // regenerate with name
                }
            };

            vm.close = function () {
                vm.onClose();
            };

            vm.closeOnBackdrop = function (e) {
                if (e.target === e.currentTarget) vm.onClose();
            };

            // — Step 2: device trigger —
            vm.selectDevice = function (dev) {
                vm.triggerConfig.device         = dev.Name;
                vm.triggerConfig.deviceCategory = getDeviceCategory(dev);
                vm.deviceConditions             = DEVICE_CONDITIONS[vm.triggerConfig.deviceCategory] || DEVICE_CONDITIONS['switch'];
                // reset condition if no longer valid
                var valid = vm.deviceConditions.some(function (c) { return c.id === vm.triggerConfig.condition; });
                if (!valid) { vm.triggerConfig.condition = 'any'; }
                vm.updateConditionValue();
            };

            vm.onConditionChange = function () {
                vm.updateConditionValue();
            };

            vm.updateConditionValue = function () {
                var cond = vm.deviceConditions.find(function (c) { return c.id === vm.triggerConfig.condition; }) || {};
                vm.conditionNeedsValue = !!cond.hasValue;
                vm.conditionUnit       = cond.unit || '';
                if (cond.hasValue && vm.triggerConfig.conditionValue == null) {
                    vm.triggerConfig.conditionValue = cond.def;
                }
                if (!cond.hasValue) {
                    vm.triggerConfig.conditionValue = undefined;
                }
            };

            // — Step 3: actions —
            vm.openActionPicker = function () { vm.showActionPicker = true; };
            vm.closeActionPicker = function () { vm.showActionPicker = false; };

            vm.pickAction = function (actionDef) {
                vm.actions.push({ type: actionDef.id, config: {} });
                vm.showActionPicker = false;
                loadScenes(); // pre-fetch in case scene action was picked
            };

            vm.removeAction = function (idx) {
                vm.actions.splice(idx, 1);
            };

            vm.getActionDef = function (type) {
                return ACTIONS.find(function (a) { return a.id === type; }) || { icon: 'icon-cog', label: type };
            };

            // — Step 4: review / create —
            vm.onNameChange = function () {
                vm.generatedCode = generateCode();
            };

            vm.createAutomation = function () {
                var code = vm.generatedCode || generateCode();
                var name = (vm.name || 'MyAutomation').trim();
                var event = {
                    id:           name,
                    eventstatus:  '1',
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

            // — internals —
            function resetState() {
                vm.step             = 1;
                vm.triggerType      = null;
                vm.triggerConfig    = {};
                vm.actions          = [];
                vm.name             = '';
                vm.deviceSearch     = '';
                vm.showActionPicker = false;
                vm.deviceConditions = DEVICE_CONDITIONS['switch'];
                vm.conditionNeedsValue = false;
                vm.generatedCode    = '';
            }

            function initTriggerDefaults(type) {
                var tc = vm.triggerConfig;
                if (type === 'time')     { tc.time = tc.time || '07:00'; tc.days = tc.days || ''; }
                if (type === 'sun')      { tc.event = tc.event || 'sunrise'; tc.offset = tc.offset != null ? tc.offset : 0; }
                if (type === 'interval') { tc.value = tc.value || 5; tc.unit = tc.unit || 'minutes'; }
                if (type === 'security') { tc.state = tc.state || 'SECURITY_ARMED_AWAY'; }
                if (type === 'device')   { tc.condition = tc.condition || 'any'; }
            }

            function defaultName() {
                var t = TRIGGERS.find(function (x) { return x.id === vm.triggerType; });
                var label = t ? t.label : 'Automation';
                if (vm.triggerType === 'device' && vm.triggerConfig.device) {
                    return vm.triggerConfig.device + ' automation';
                }
                return label + ' automation';
            }

            function loadDevices() {
                if (vm.devicesLoaded) return;
                $http.get('json.htm?type=command&param=getdevices&filter=all&used=true&order=Name')
                    .then(function (resp) {
                        vm.devices       = (resp.data && resp.data.result) ? resp.data.result : [];
                        vm.devicesLoaded = true;
                    });
            }

            function loadScenes() {
                if (vm.scenesLoaded) return;
                $http.get('json.htm?type=command&param=getscenes')
                    .then(function (resp) {
                        vm.scenes       = (resp.data && resp.data.result) ? resp.data.result : [];
                        vm.scenesLoaded = true;
                    });
            }

            function buildConditionLine(tc, indent) {
                var c   = tc.condition;
                var cat = tc.deviceCategory || 'switch';
                var v   = tc.conditionValue != null ? tc.conditionValue : 0;
                if (!c || c === 'any') return null;

                var expr = COND_EXPR[c];
                if (typeof expr === 'object') expr = expr[cat];
                if (!expr) return null;
                return indent + 'if (' + expr.replace('{v}', v) + ') then';
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

                if (t === 'device') {
                    L.push(i8 + 'devices = {');
                    L.push(i12 + "'" + luaEsc(tc.device || 'Your Device') + "'");
                    L.push(i8 + '},');
                } else if (t === 'time') {
                    var days = tc.days ? ' on ' + tc.days : '';
                    L.push(i8 + 'timer = {');
                    L.push(i12 + "'at " + (tc.time || '07:00') + days + "'");
                    L.push(i8 + '},');
                } else if (t === 'sun') {
                    var ev  = tc.event || 'sunrise';
                    var off = parseInt(tc.offset) || 0;
                    var sun = off === 0 ? 'at ' + ev
                            : Math.abs(off) + ' minutes ' + (off < 0 ? 'before' : 'after') + ' ' + ev;
                    L.push(i8 + 'timer = {');
                    L.push(i12 + "'" + sun + "'");
                    L.push(i8 + '},');
                } else if (t === 'interval') {
                    var n  = parseInt(tc.value) || 5;
                    var un = tc.unit || 'minutes';
                    var iv = (n === 1 && un === 'hours') ? 'every hour' : 'every ' + n + ' ' + un;
                    L.push(i8 + 'timer = {');
                    L.push(i12 + "'" + iv + "'");
                    L.push(i8 + '},');
                } else if (t === 'security') {
                    L.push(i8 + 'security = {');
                    L.push(i12 + 'domoticz.' + (tc.state || 'SECURITY_ARMED_AWAY'));
                    L.push(i8 + '},');
                } else if (t === 'variable') {
                    L.push(i8 + 'variables = {');
                    L.push(i12 + "'" + luaEsc(tc.varName || 'YourVariable') + "'");
                    L.push(i8 + '},');
                }

                L.push(i4 + '},');

                var param = t === 'device'   ? 'device'
                          : t === 'variable' ? 'variable'
                          : t === 'security' ? 'security' : 'timer';
                L.push(i4 + 'execute = function(domoticz, ' + param + ')');

                var bi       = i8;
                var condLine = (t === 'device') ? buildConditionLine(tc, i8) : null;
                if (condLine) { L.push(condLine); bi = i12; }

                if (!vm.actions.length) {
                    L.push(bi + '-- Add your actions here');
                } else {
                    vm.actions.forEach(function (a) {
                        var c = a.config || {};
                        if (a.type === 'switch') {
                            var act = c.action || 'switchOn';
                            if (act === 'dim') {
                                L.push(bi + "domoticz.devices('" + luaEsc(c.device || 'Device') + "').dimTo(" + (parseInt(c.level) || 50) + ")");
                            } else {
                                L.push(bi + "domoticz.devices('" + luaEsc(c.device || 'Device') + "')." + act + "()");
                            }
                        } else if (a.type === 'notify') {
                            L.push(bi + "domoticz.notify('" + luaEsc(c.title || 'Alert') + "', '" + luaEsc(c.message || '') + "', domoticz.PRIORITY_NORMAL)");
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
                            L.push(bi + i4 + "method = '" + (c.method || 'GET') + "'");
                            L.push(bi + '})');
                        } else if (a.type === 'custom') {
                            (c.code || '-- your code here').split('\n').forEach(function (line) { L.push(bi + line); });
                        }
                    });
                }

                if (condLine) L.push(i8 + 'end');
                L.push(i4 + 'end');
                L.push('}');
                return L.join('\n');
            }
        }
    });
});
