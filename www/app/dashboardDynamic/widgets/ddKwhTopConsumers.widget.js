define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'kwh-top-consumers',
        label:       'kWh Top Consumers',
        description: 'Lists kWh devices sorted by today\'s energy usage, highest first. Excludes timed-out and P1 meter devices.',
        category:    'System',
        icon:        'fa-solid fa-ranking-star',
        defaultW:    3,
        defaultH:    4,
        minW:        2,
        minH:        3,
        maxW:        6,
        maxH:        10,
        transparentBackground: true,
        configSchema: [
            {
                key:      'title',
                type:     'text',
                label:    'Title (optional)',
                required: false
            },
            {
                key:     'maxDevices',
                type:    'number',
                step:    1,
                label:   'Max devices to show',
                default: 20,
                min:     1
            },
            {
                key:      'excludeIdx',
                type:     'text',
                label:    'Exclude device IDX (semicolon separated)',
                required: false
            },
            { key: 'showBackground', type: 'boolean', label: 'Show panel background', default: true }
        ]
    });

    app.directive('ddKwhTopConsumersWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/kwh-top-consumers.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$q', function($scope, $http, $q) {
                var ctrl = this;

                ctrl.title     = 'kWh Top Consumers';
                ctrl.devices   = [];
                ctrl.count     = 0;
                ctrl.loading   = false;
                ctrl.loadError = false;

                var cancelToken     = null;
                var allKwhDevices   = {};   // idx (string) -> device object; full filtered list for re-ranking

                function cfg() {
                    return (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                }

                function parseKwh(str) {
                    if (!str) { return -1; }
                    var m = String(str).match(/^([\d.,]+)/);
                    if (!m) { return -1; }
                    return parseFloat(m[1].replace(',', '.')) || 0;
                }

                function parseWatt(str) {
                    if (!str) { return null; }
                    var m = String(str).match(/^([\d.,]+)/);
                    return m ? (parseFloat(m[1].replace(',', '.')) || 0) : null;
                }

                function toDeviceObj(d) {
                    return {
                        name:         d.Name,
                        idx:          String(d.idx),
                        counterToday: d.CounterToday || '0 kWh',
                        kwhValue:     parseKwh(d.CounterToday),
                        usageWatt:    parseWatt(d.Usage)
                    };
                }

                // Re-sort allKwhDevices by kwhValue and expose the top N as ctrl.devices.
                // Called after every HTTP load and every WebSocket update so the ranking
                // stays correct even when an off-screen device surpasses a visible one.
                function applyRanking() {
                    var c   = cfg();
                    var max = parseInt(c.maxDevices, 10);
                    if (isNaN(max) || max < 1) { max = 20; }

                    var sorted = Object.keys(allKwhDevices).map(function(k) {
                        return allKwhDevices[k];
                    }).sort(function(a, b) {
                        return b.kwhValue - a.kwhValue;
                    });

                    ctrl.devices = sorted.slice(0, max);
                    ctrl.count   = ctrl.devices.length;
                }

                function load() {
                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();

                    ctrl.loading   = true;
                    ctrl.loadError = false;

                    $http.get('json.htm', {
                        params: {
                            type:   'command',
                            param:  'getdevices',
                            used:   'true',
                            filter: 'utility',
                            order:  'Name'
                        },
                        timeout: cancelToken.promise
                    }).then(function(resp) {
                        ctrl.loading = false;
                        var result = resp.data && resp.data.result;
                        if (!result) { return; }

                        var c = cfg();

                        // Build exclusion set from user-configured IDX list (semicolon separated)
                        var excludeSet = {};
                        (c.excludeIdx || '').split(';').forEach(function(s) {
                            var v = s.trim();
                            if (v) { excludeSet[v] = true; }
                        });

                        // Keep only kWh devices; exclude:
                        //   - P1 hardware (HardwareTypeVal 4=serial, 5=LAN) — filters all
                        //     phase sensors (L1/L2/L3) created by the P1 meter
                        //   - Return/Energy Generated meters (SwitchTypeVal 4 = MTYPE_ENERGY_GENERATED)
                        //   - timed-out devices
                        //   - manually excluded IDXs
                        // Store full set in allKwhDevices for WebSocket-driven re-ranking
                        // (a currently hidden device can overtake a visible one when its usage rises).
                        allKwhDevices = {};
                        result.forEach(function(d) {
                            if (d.HaveTimeout === true) { return; }
                            var hwType = d.HardwareTypeVal;
                            if (hwType === 4 || hwType === 5) { return; }
                            if (d.SwitchTypeVal === 4) { return; }  // Return / Energy Generated
                            if (d.SubType !== 'kWh' && d.Type !== 'kWh') { return; }
                            if (excludeSet[String(d.idx)]) { return; }
                            if (parseKwh(d.CounterToday) <= 0) { return; }
                            allKwhDevices[String(d.idx)] = toDeviceObj(d);
                        });

                        applyRanking();
                    }).catch(function(err) {
                        if (err.status === -1) { return; }
                        ctrl.loading   = false;
                        ctrl.loadError = true;
                    });
                }

                // Live WebSocket update: if the device is in our tracked set, update its
                // values and re-rank immediately — no HTTP round-trip needed.
                $scope.$on('device_update', function(e, updated) {
                    var key = String(updated.idx);
                    if (!allKwhDevices[key]) { return; }

                    if (updated.CounterToday) {
                        var kwhValue = parseKwh(updated.CounterToday);
                        if (kwhValue <= 0) {
                            delete allKwhDevices[key];
                        } else {
                            allKwhDevices[key].counterToday = updated.CounterToday;
                            allKwhDevices[key].kwhValue     = kwhValue;
                        }
                    }
                    if (allKwhDevices[key]) {
                        var w = parseWatt(updated.Usage);
                        if (w !== null) { allKwhDevices[key].usageWatt = w; }
                    }

                    applyRanking();
                });

                $scope.$on('dd:widget:refresh', load);

                $scope.$on('$destroy', function() {
                    if (cancelToken) { cancelToken.resolve(); cancelToken = null; }
                });

                ctrl.$onInit = function() {
                    ctrl.title = cfg().title || 'kWh Top Consumers';
                    load();
                };

                $scope.$watch(
                    function() {
                        var c = cfg();
                        return (c.title || '') + '|' + (c.maxDevices) + '|' + (c.excludeIdx || '');
                    },
                    function(val, old) {
                        if (val !== old) {
                            ctrl.title = cfg().title || 'kWh Top Consumers';
                            load();
                        }
                    }
                );
            }]
        };
    }]);
});
