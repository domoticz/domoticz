define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    var _verifiedDevices = {};

    widgetRegistry.register({
        type:                  'thermostat6',
        transparentBackground: true,
        label:       'Thermostat6',
        description: 'Shows Temp, Humidity, Barometer and Setpoint controls for a Thermostat6 device',
        category:    'Controls',
        icon:        'fa-solid fa-thermometer',
        defaultW:    3,
        defaultH:    3,
        minW:        2,
        minH:        2,
        maxW:        6,
        maxH:        5,
        configSchema: [
            {
                key:          'deviceIdx',
                type:         'device-picker',
                label:        'Device',
                deviceFilter: 'thermostat6',
                required:     true
            },
            {
                key:      'title',
                type:     'text',
                label:    'Title (optional, falls back to device name)',
                required: false
            },
            { key: 'showBackground', type: 'boolean', label: 'Show panel background', default: true }
        ]
    });

    app.directive('ddThermostat6Widget', ['bootbox', function(bootbox) {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/thermostat6.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', '$q', function($scope, $http, $interval, $q) {
                var ctrl = this;
                ctrl.title           = '';
                ctrl.deviceName      = '';
                ctrl.temp            = null;
                ctrl.humidity        = null;
                ctrl.humidityStatus  = null;
                ctrl.barometer       = null;
                ctrl.setpoint        = null;
                ctrl.sending         = false;
                ctrl.loadError       = false;
                ctrl.deviceStep      = 0.5;
                ctrl.deviceMin       = 5;
                ctrl.deviceMax       = 35;
                ctrl.deviceProtected = false;
                var cancelToken      = null;

                function cfg() {
                    return (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                }

                function step()  { return ctrl.deviceStep; }
                function minVal(){ return ctrl.deviceMin; }
                function maxVal(){ return ctrl.deviceMax; }

                function clamp(v) {
                    return Math.max(minVal(), Math.min(maxVal(), v));
                }

                function round(v) {
                    var s = step();
                    var decimals = (s.toString().split('.')[1] || '').length;
                    return parseFloat(v.toFixed(decimals));
                }

                function applyDevice(d) {
                    ctrl.deviceName      = d.Name || '';
                    ctrl.title           = cfg().title || ctrl.deviceName;
                    ctrl.deviceProtected = d.Protected;
                    if (d.step  !== undefined) { ctrl.deviceStep = parseFloat(d.step) || 0.5; }
                    if (d.min   !== undefined) { ctrl.deviceMin  = parseFloat(d.min); }
                    if (d.max   !== undefined) { ctrl.deviceMax  = parseFloat(d.max); }

                    var t = parseFloat(d.Temp);
                    ctrl.temp = isNaN(t) ? null : t;

                    var h = parseInt(d.Humidity, 10);
                    ctrl.humidity = isNaN(h) ? null : h;

                    ctrl.humidityStatus = (d.HumidityStatus !== undefined && d.HumidityStatus !== '') ? d.HumidityStatus : null;

                    var b = parseFloat(d.Barometer);
                    ctrl.barometer = isNaN(b) ? null : b;

                    var raw    = d.SetPoint !== undefined ? d.SetPoint : d.Data;
                    var parsed = parseFloat(raw);
                    ctrl.setpoint = isNaN(parsed) ? null : parsed;
                }

                function load() {
                    var c = cfg();
                    if (!c.deviceIdx) { return; }

                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();

                    $http.get('json.htm', {
                        params: {
                            type:  'command',
                            param: 'getdevices',
                            rid:   c.deviceIdx
                        },
                        timeout: cancelToken.promise
                    }).then(function(resp) {
                        ctrl.loadError = false;
                        var d = resp.data.result && resp.data.result[0];
                        if (!d) { return; }
                        applyDevice(d);
                    }).catch(function(err) {
                        if (err.status === -1) { return; }
                        ctrl.loadError = true;
                    });
                }

                function sendValue(newVal) {
                    var c = cfg();
                    if (!c.deviceIdx) { return; }
                    ctrl.sending = true;
                    $http.get('json.htm', {
                        params: {
                            type:     'command',
                            param:    'setsetpoint',
                            idx:      c.deviceIdx,
                            setpoint: newVal
                        }
                    }).then(function() {
                        ctrl.setpoint = newVal;
                        ctrl.sending  = false;
                    }).catch(function() {
                        ctrl.sending = false;
                    });
                }

                ctrl.increment = function() {
                    if (ctrl.setpoint === null || ctrl.sending) { return; }
                    var idx = cfg().deviceIdx;
                    if (ctrl.deviceProtected && !_verifiedDevices[idx]) {
                        if (typeof HandleProtection === 'function') {
                            HandleProtection(ctrl.deviceProtected, function() {
                                _verifiedDevices[idx] = true;
                                var newVal = round(clamp(ctrl.setpoint + step()));
                                if (newVal !== ctrl.setpoint) { sendValue(newVal); }
                            });
                        }
                        return;
                    }
                    var newVal = round(clamp(ctrl.setpoint + step()));
                    if (newVal !== ctrl.setpoint) { sendValue(newVal); }
                };

                ctrl.decrement = function() {
                    if (ctrl.setpoint === null || ctrl.sending) { return; }
                    var idx = cfg().deviceIdx;
                    if (ctrl.deviceProtected && !_verifiedDevices[idx]) {
                        if (typeof HandleProtection === 'function') {
                            HandleProtection(ctrl.deviceProtected, function() {
                                _verifiedDevices[idx] = true;
                                var newVal = round(clamp(ctrl.setpoint - step()));
                                if (newVal !== ctrl.setpoint) { sendValue(newVal); }
                            });
                        }
                        return;
                    }
                    var newVal = round(clamp(ctrl.setpoint - step()));
                    if (newVal !== ctrl.setpoint) { sendValue(newVal); }
                };

                ctrl.atMin = function() {
                    return ctrl.setpoint !== null && ctrl.setpoint <= minVal();
                };

                ctrl.atMax = function() {
                    return ctrl.setpoint !== null && ctrl.setpoint >= maxVal();
                };

                ctrl.clickToEdit = function(event) {
                    if (ctrl.sending) { return; }
                    var c = cfg();
                    if (!c.deviceIdx) { return; }
                    if (typeof ShowSetpointPopup === 'function') {
                        ShowSetpointPopup(event, c.deviceIdx, ctrl.deviceProtected, ctrl.setpoint, false,
                            ctrl.deviceStep, ctrl.deviceMin, ctrl.deviceMax);
                    }
                };

                $scope.$on('device_update', function(e, updated) {
                    var c = cfg();
                    if (c && String(updated.idx) === String(c.deviceIdx)) {
                        applyDevice(updated);
                    }
                });

                var timer = $interval(load, 30000);

                $scope.$on('$destroy', function() {
                    if (cancelToken) { cancelToken.resolve(); cancelToken = null; }
                    $interval.cancel(timer);
                });

                $scope.$on('dd:widget:refresh', load);

                $scope.$watch(
                    function() {
                        return ctrl.widgetDef && ctrl.widgetDef.config && ctrl.widgetDef.config.deviceIdx;
                    },
                    function(val, old) {
                        if (val !== old) { load(); }
                    }
                );

                ctrl.$onInit = load;
            }]
        };
    }]);
});
