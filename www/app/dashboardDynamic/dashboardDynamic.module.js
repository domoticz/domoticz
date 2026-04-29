define(['app'], function(app) {
    'use strict';

    /**
     * Dashboard 2.0 AngularJS module.
     * All dashboardDynamic services, directives, and controllers are registered here.
     */

    // Module-level constant: supported grid columns
    app.constant('DD_GRID_COLUMNS', 12);

    // Module-level constant: list of all widget types (used by Feature 05)
    app.constant('DD_WIDGET_CATALOG', []);  // populated by Feature 05

    /**
     * ddDeviceClassifier
     *
     * Determines which existing AngularJS widget directive should render a
     * given device object returned by the /json.htm API.
     *
     * Returns one of:
     *   'dz-light-widget'   — switches, lights, blinds, color devices, fans, …
     *   'dz-scene-widget'   — scenes and groups
     *   'dz-utility-widget' — temperature, humidity, weather, meters, counters, …
     */
    app.factory('ddDeviceClassifier', [function() {

        function getDirective(device) {
            if (!device) { return null; }

            var type = device.Type || '';

            // Scenes and Groups
            if (type.indexOf('Scene') === 0 || type.indexOf('Group') === 0) {
                return 'dz-scene-widget';
            }

            // Light / Switch / Blind / Color / RFY / Fan / Chime / Security / Thermostat
            if (
                type.indexOf('Light')       === 0 ||
                type.indexOf('Blind')       === 0 ||
                type.indexOf('Curtain')     === 0 ||
                type.indexOf('Color Switch') === 0 ||
                type.indexOf('Chime')       === 0 ||
                type.indexOf('Thermostat 2') === 0 ||
                type.indexOf('Thermostat 3') === 0 ||
                type.indexOf('RFY')         === 0 ||
                type.indexOf('ASA')         === 0 ||
                type.indexOf('Fan')         === 0 ||
                type === 'Security'
            ) {
                return 'dz-light-widget';
            }

            // Everything else (Temp, Humidity, Weather, Utility, Energy, …)
            return 'dz-utility-widget';
        }

        var IMAGE_ICON_MAP = {
            'Fan':             'fa-solid fa-fan',
            'Fireplace':       'fa-solid fa-fire',
            'Heating':         'fa-solid fa-fire-flame-curved',
            'Media':           'fa-solid fa-tv',
            'Phone':           'fa-solid fa-phone',
            'Speaker':         'fa-solid fa-volume-high',
            'WallSocket':      'fa-solid fa-plug',
            'Siren':           'fa-solid fa-bell',
            'Door':            'fa-solid fa-door-open',
            'doorbell':        'fa-solid fa-bell',
            'Blinds':          'fa-solid fa-blinds',
            'blinds':          'fa-solid fa-blinds',
            'Security':        'fa-solid fa-shield-halved',
            'Water':           'fa-solid fa-faucet',
            'Generic':         'fa-solid fa-circle-dot',
            'Light':           'fa-solid fa-lightbulb',
            'ChristmasTree':   'fa-solid fa-tree',
            'Computer':        'fa-solid fa-desktop',
            'Printer':         'fa-solid fa-print',
            'Washing':         'fa-solid fa-shirt'
        };

        function autoDeviceIcon(d) {
            var type = (d.Type || '').toLowerCase();
            if (d.Temp !== undefined)                                           { return 'fa-solid fa-temperature-half'; }
            if (d.Humidity !== undefined)                                       { return 'fa-solid fa-droplet'; }
            if (type.indexOf('wind') >= 0)                                      { return 'fa-solid fa-wind'; }
            if (type.indexOf('rain') >= 0)                                      { return 'fa-solid fa-cloud-rain'; }
            if (d.SwitchType === 'Selector')                                    { return 'fa-solid fa-sliders'; }
            if (d.SwitchType === 'Dimmer')                                      { return 'fa-solid fa-lightbulb'; }
            if (type.indexOf('light') >= 0 || type.indexOf('switch') >= 0) {
                if (d.Image && IMAGE_ICON_MAP[d.Image])                         { return IMAGE_ICON_MAP[d.Image]; }
                return 'fa-solid fa-power-off';
            }
            if (d.SubType === 'kWh')                                            { return 'fa-solid fa-bolt'; }
            if (type.indexOf('scene') >= 0 || type.indexOf('group') >= 0)      { return 'fa-solid fa-play'; }
            return 'fa-solid fa-circle-dot';
        }

        function roundIfLarge(str) {
            var m = String(str || '').match(/^([\d.]+)(\s*.+)$/);
            if (!m) { return str; }
            var n = parseFloat(m[1]);
            return (!isNaN(n) && n > 1000) ? Math.round(n) + m[2] : str;
        }

        function extractDeviceValue(d) {
            var type = (d.Type || '').toLowerCase();
            if (d.SwitchType === 'Selector') {
                var levelNames = [];
                try { levelNames = b64DecodeUnicode(d.LevelNames).split('|'); } catch(e) { levelNames = (d.LevelNames || '').split('|'); }
                var levelIdx  = (d.LevelInt !== undefined) ? Math.round(d.LevelInt / 10) : 0;
                var levelName = levelNames[levelIdx] || d.Status || d.Data || '—';
                return { value: levelName, isOn: d.LevelInt > 0, unit: '', unit2: null, secondValue: null, typeClass: 'switch' };
            }
            if (d.SwitchType !== undefined || type.indexOf('light') >= 0 || type.indexOf('switch') >= 0) {
                var statusStr = d.Status || d.Data || '';
                var isOn = (d.SwitchType === 'Dimmer')
                    ? (statusStr !== '' && statusStr !== 'Off')
                    : (statusStr === 'On');
                return { value: $.t(statusStr) || statusStr, isOn: isOn, unit: '', unit2: null, secondValue: null, typeClass: 'switch' };
            }
            if (type.indexOf('scene') >= 0 || type.indexOf('group') >= 0) {
                var sceneStatus = d.Status || d.Data || '';
                return { value: $.t(sceneStatus) || sceneStatus, isOn: sceneStatus === 'On', unit: '', unit2: null, secondValue: null, typeClass: 'switch' };
            }
            if (type.indexOf('wind') >= 0) {
                var dirSpeed = ((d.DirectionStr || '') + ' ' + (d.Speed || '')).trim();
                var wTemp    = (d.Temp !== undefined && parseFloat(d.Temp) !== -999) ? String(d.Temp) : null;
                return { value: dirSpeed || d.Data || '\u2014', secondValue: wTemp, isOn: false,
                         unit: dirSpeed ? 'km/h' : '', unit2: wTemp !== null ? '\u00b0C' : null, typeClass: 'wind' };
            }
            if (d.Temp !== undefined && d.Humidity !== undefined) {
                return { value: String(d.Temp), secondValue: String(d.Humidity), isOn: false,
                         unit: '\u00b0C', unit2: '%', typeClass: 'temp-hum' };
            }
            if (d.Temp !== undefined) {
                return { value: String(d.Temp), isOn: false, unit: '\u00b0C', unit2: null, secondValue: null, typeClass: 'temp' };
            }
            if (d.Humidity !== undefined) {
                return { value: String(d.Humidity), isOn: false, unit: '%', unit2: null, secondValue: null, typeClass: 'humidity' };
            }
            // P1 Smart Meter — has both CounterToday (import) and CounterDelivToday (export)
            if (d.CounterToday !== undefined && d.CounterDelivToday !== undefined) {
                var p1Usage  = (d.Usage  || '').trim() || '0 Watt';
                var p1Value  = 'Usage: ' + d.CounterToday + ', Actual: ' + p1Usage;
                var p1Second = 'Return: ' + d.CounterDelivToday;
                return { value: p1Value, secondValue: p1Second, isOn: false, unit: '', unit2: null, typeClass: 'p1' };
            }
            // kWh energy counter — show counter value + current Watt (rounded when > 1000)
            if (d.SubType === 'kWh') {
                var kwhData = roundIfLarge(d.Data || '\u2014');
                var kwhVal  = (d.Usage !== undefined) ? kwhData + ' / ' + roundIfLarge(d.Usage) : kwhData;
                return { value: kwhVal, secondValue: null, isOn: false, unit: '', unit2: null, typeClass: 'generic' };
            }
            return { value: roundIfLarge(d.Data || '\u2014'), isOn: false, unit: '', unit2: null, secondValue: null, typeClass: 'generic' };
        }

        return { getDirective: getDirective, autoDeviceIcon: autoDeviceIcon, extractDeviceValue: extractDeviceValue };
    }]);

    // Small helper directive: auto-focuses an input when it appears in the DOM
    app.directive('ddAutofocus', ['$timeout', function($timeout) {
        return {
            restrict: 'A',
            link: function(scope, element) {
                $timeout(function() { element[0].focus(); element[0].select(); }, 30);
            }
        };
    }]);

    return app;
});
