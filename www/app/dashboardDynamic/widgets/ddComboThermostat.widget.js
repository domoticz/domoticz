define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    var _verifiedDevices = {};

    widgetRegistry.register({
        type:        'combo-thermostat',
        label:       'Thermostat',
        description: 'Shows current temperature from a sensor alongside a setpoint control with +/− buttons',
        category:    'Controls',
        icon:        'fa-solid fa-temperature-half',
        defaultW:    2,
        defaultH:    3,
        minW:        2,
        minH:        2,
        maxW:        4,
        maxH:        4,
        configSchema: [
            {
                key:          'tempDeviceIdx',
                type:         'device-picker',
                label:        'Temperature sensor',
                deviceFilter: 'temp',
                required:     true
            },
            {
                key:          'setpointDeviceIdx',
                type:         'device-picker',
                label:        'Setpoint device',
                deviceFilter: 'setpoint',
                required:     true
            },
            {
                key:      'title',
                type:     'text',
                label:    'Title (optional, falls back to device name)',
                required: false
            }
        ]
    });

    app.directive('ddComboThermostatWidget', ['bootbox', function(bootbox) {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/combo-thermostat.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', '$q', function($scope, $http, $interval, $q) {
                var ctrl = this;
                ctrl.title           = '';
                ctrl.currentTemp     = null;
                ctrl.setpoint        = null;
                ctrl.sending         = false;
                ctrl.loadError       = false;
                ctrl.deviceStep      = 0.5;
                ctrl.deviceMin       = 5;
                ctrl.deviceMax       = 35;
                ctrl.deviceUnit      = '°C';
                ctrl.deviceProtected = false;
                var cancelToken  = null;

                function cfg() {
                    return (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                }

                function step()  { return ctrl.deviceStep; }
                function minVal(){ return ctrl.deviceMin; }
                function maxVal(){ return ctrl.deviceMax; }
                function unit()  { return ctrl.deviceUnit; }

                function clamp(v) {
                    return Math.max(minVal(), Math.min(maxVal(), v));
                }

                function round(v) {
                    var s = step();
                    var decimals = (s.toString().split('.')[1] || '').length;
                    return parseFloat(v.toFixed(decimals));
                }

                function applyDevices(results) {
                    var c = cfg();
                    var tempIdx      = String(c.tempDeviceIdx);
                    var setpointIdx  = String(c.setpointDeviceIdx);

                    (results || []).forEach(function(d) {
                        var id = String(d.idx);
                        if (id === tempIdx) {
                            ctrl.title       = cfg().title || d.Name || '';
                            var t = parseFloat(d.Temp);
                            ctrl.currentTemp = isNaN(t) ? null : t;
                        }
                        if (id === setpointIdx) {
                            ctrl.deviceProtected = d.Protected;
                            if (d.step  !== undefined) { ctrl.deviceStep = parseFloat(d.step) || 0.5; }
                            if (d.min   !== undefined) { ctrl.deviceMin  = parseFloat(d.min); }
                            if (d.max   !== undefined) { ctrl.deviceMax  = parseFloat(d.max); }
                            if (d.vunit !== undefined && d.vunit !== '') { ctrl.deviceUnit = d.vunit; }
                            var raw    = d.SetPoint !== undefined ? d.SetPoint : d.Data;
                            var parsed = parseFloat(raw);
                            ctrl.setpoint = isNaN(parsed) ? null : parsed;
                        }
                    });
                }

                function load() {
                    var c = cfg();
                    if (!c.tempDeviceIdx || !c.setpointDeviceIdx) { return; }

                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();

                    $http.get('json.htm', {
                        params: {
                            type:  'command',
                            param: 'getdevices',
                            rid:   c.tempDeviceIdx + ',' + c.setpointDeviceIdx
                        },
                        timeout: cancelToken.promise
                    }).then(function(resp) {
                        ctrl.loadError = false;
                        applyDevices(resp.data.result);
                    }).catch(function(err) {
                        if (err.status === -1) { return; }
                        ctrl.loadError = true;
                    });
                }

                function sendValue(newVal) {
                    var c = cfg();
                    if (!c.setpointDeviceIdx) { return; }
                    ctrl.sending = true;
                    $http.get('json.htm', {
                        params: {
                            type:     'command',
                            param:    'setsetpoint',
                            idx:      c.setpointDeviceIdx,
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
                    var idx = cfg().setpointDeviceIdx;
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
                    var idx = cfg().setpointDeviceIdx;
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
                    if (!c.setpointDeviceIdx) { return; }
                    if (typeof ShowSetpointPopup === 'function') {
                        ShowSetpointPopup(event, c.setpointDeviceIdx, ctrl.deviceProtected, ctrl.setpoint, false,
                            ctrl.deviceStep, ctrl.deviceMin, ctrl.deviceMax);
                    }
                };

                $scope.$on('device_update', function(e, updated) {
                    var c = cfg();
                    if (!c) { return; }
                    var updIdx = String(updated.idx);
                    if (updIdx === String(c.tempDeviceIdx) || updIdx === String(c.setpointDeviceIdx)) {
                        load();
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
                        var c = ctrl.widgetDef && ctrl.widgetDef.config;
                        return c && (c.tempDeviceIdx + '|' + c.setpointDeviceIdx);
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
