define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    var _verifiedDevices = {};

    widgetRegistry.register({
        type:                  'setpoint',
        transparentBackground: true,
        label:       'Setpoint',
        description: 'Display and control a Domoticz setpoint device with +/− buttons and click-to-edit',
        category:    'Controls',
        icon:        'fa-solid fa-sliders',
        defaultW:    2,
        defaultH:    3,
        minW:        2,
        minH:        2,
        maxW:        4,
        maxH:        4,
        configSchema: [
            {
                key:          'deviceIdx',
                type:         'device-picker',
                label:        'Device',
                deviceFilter: 'setpoint',
                required:     true
            },
            {
                key:      'title',
                type:     'text',
                label:    'Title (optional, falls back to device name)',
                required: false
            },
            { key: 'showBackground', type: 'boolean', label: 'Show panel background', default: true },
            { key: 'ranges', type: 'range-list', label: 'Bar ranges (optional)' }
        ]
    });

    app.directive('ddSetpointWidget', ['bootbox', function(bootbox) {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/setpoint.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$q', function($scope, $http, $q) {
                var ctrl = this;
                ctrl.title           = '';
                ctrl.value           = null;
                ctrl.sending         = false;
                ctrl.loadError       = false;
                ctrl.deviceStep      = 0.5;
                ctrl.deviceMin       = 5;
                ctrl.deviceMax       = 35;
                ctrl.deviceUnit      = '°C';
                ctrl.deviceProtected = false;
                var cancelToken = null;

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
                    // Round to same decimal precision as step
                    var s = step();
                    var decimals = (s.toString().split('.')[1] || '').length;
                    return parseFloat(v.toFixed(decimals));
                }

                function applyDevice(d) {
                    ctrl.title           = cfg().title || d.Name || '';
                    ctrl.deviceProtected = d.Protected;
                    if (d.step  !== undefined) { ctrl.deviceStep = parseFloat(d.step) || 0.5; }
                    if (d.min   !== undefined) { ctrl.deviceMin  = parseFloat(d.min); }
                    if (d.max   !== undefined) { ctrl.deviceMax  = parseFloat(d.max); }
                    if (d.vunit !== undefined && d.vunit !== '') { ctrl.deviceUnit = d.vunit; }
                    var raw = d.SetPoint !== undefined ? d.SetPoint : d.Data;
                    var parsed = parseFloat(raw);
                    ctrl.value = isNaN(parsed) ? null : parsed;
                }

                function load() {
                    var c = cfg();
                    if (!c.deviceIdx) { return; }

                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();

                    $http.get('json.htm', {
                        params:  { type: 'command', param: 'getdevices', rid: c.deviceIdx },
                        timeout: cancelToken.promise
                    }).then(function(resp) {
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
                        ctrl.value   = newVal;
                        ctrl.sending = false;
                        $(document).trigger('dz:setpoint:saved', { idx: c.deviceIdx, value: newVal });
                    }).catch(function() {
                        ctrl.sending = false;
                    });
                }

                ctrl.increment = function() {
                    if (ctrl.value === null || ctrl.sending) { return; }
                    var idx = cfg().deviceIdx;
                    if (ctrl.deviceProtected && !_verifiedDevices[idx]) {
                        if (typeof HandleProtection === 'function') {
                            HandleProtection(ctrl.deviceProtected, function() {
                                _verifiedDevices[idx] = true;
                                var newVal = round(clamp(ctrl.value + step()));
                                if (newVal !== ctrl.value) { sendValue(newVal); }
                            });
                        }
                        return;
                    }
                    var newVal = round(clamp(ctrl.value + step()));
                    if (newVal !== ctrl.value) { sendValue(newVal); }
                };

                ctrl.decrement = function() {
                    if (ctrl.value === null || ctrl.sending) { return; }
                    var idx = cfg().deviceIdx;
                    if (ctrl.deviceProtected && !_verifiedDevices[idx]) {
                        if (typeof HandleProtection === 'function') {
                            HandleProtection(ctrl.deviceProtected, function() {
                                _verifiedDevices[idx] = true;
                                var newVal = round(clamp(ctrl.value - step()));
                                if (newVal !== ctrl.value) { sendValue(newVal); }
                            });
                        }
                        return;
                    }
                    var newVal = round(clamp(ctrl.value - step()));
                    if (newVal !== ctrl.value) { sendValue(newVal); }
                };

                ctrl.atMin = function() {
                    return ctrl.value !== null && ctrl.value <= minVal();
                };

                ctrl.atMax = function() {
                    return ctrl.value !== null && ctrl.value >= maxVal();
                };

                function onSetpointSaved(e, data) {
                    var c = cfg();
                    if (c && String(data.idx) === String(c.deviceIdx)) {
                        $scope.$applyAsync(function() { ctrl.value = data.value; });
                    }
                }
                $(document).on('dz:setpoint:saved', onSetpointSaved);

                ctrl.clickToEdit = function(event) {
                    if (ctrl.sending) { return; }
                    var c = cfg();
                    if (!c.deviceIdx) { return; }
                    if (typeof ShowSetpointPopup === 'function') {
                        ShowSetpointPopup(event, c.deviceIdx, ctrl.deviceProtected, ctrl.value, false,
                            ctrl.deviceStep, ctrl.deviceMin, ctrl.deviceMax);
                    }
                };

                $scope.$on('device_update', function(e, updated) {
                    var c = cfg();
                    if (c && String(updated.idx) === String(c.deviceIdx)) {
                        applyDevice(updated);
                    }
                });

                $scope.$on('$destroy', function() {
                    if (cancelToken) { cancelToken.resolve(); cancelToken = null; }
                    $(document).off('dz:setpoint:saved', onSetpointSaved);
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
