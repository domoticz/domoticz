define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'battery-monitor',
        label:       'Battery Monitor',
        description: 'Shows all devices with battery level at or below a configurable threshold',
        category:    'System',
        icon:        'fa-solid fa-battery-quarter',
        defaultW:    3,
        defaultH:    4,
        minW:        2,
        minH:        3,
        maxW:        6,
        maxH:        10,
        configSchema: [
            {
                key:      'title',
                type:     'text',
                label:    'Title (optional)',
                required: false
            },
            {
                key:     'threshold',
                type:    'number',
                label:   'Alert threshold % (show devices at or below)',
                default: 25
            },
            {
                key:     'showFull',
                type:    'boolean',
                label:   'Show all devices with a battery (not just low)',
                default: false
            },
            {
                key:     'sortBy',
                type:    'select',
                label:   'Sort by',
                default: 'level',
                options: [
                    { value: 'level', label: 'Battery level (low first)' },
                    { value: 'name',  label: 'Device name (A–Z)' }
                ]
            },
            {
                key:     'refreshInterval',
                type:    'number',
                label:   'Refresh interval (seconds)',
                default: 300
            }
        ]
    });

    app.directive('ddBatteryMonitorWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/battery-monitor.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', '$q', function($scope, $http, $interval, $q) {
                var ctrl = this;

                ctrl.title            = 'Battery Monitor';
                ctrl.devices          = [];
                ctrl.count            = 0;
                ctrl.totalWithBattery = 0;
                ctrl.loading          = false;
                ctrl.loadError        = false;

                var refreshTimer = null;
                var cancelToken  = null;

                function cfg() {
                    return (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                }

                ctrl.levelColor = function(level) {
                    if (level <= 10) { return 'var(--dz-widget-energy-net-neg)'; }
                    if (level <= 25) { return 'var(--dz-widget-sunpv)'; }
                    if (level <= 50) { return 'var(--dz-widget-sunpv)'; }
                    return 'var(--dz-widget-energy-export)';
                };

                ctrl.levelIcon = function(level) {
                    if (level <= 10) { return 'fa-battery-empty'; }
                    if (level <= 25) { return 'fa-battery-quarter'; }
                    if (level <= 50) { return 'fa-battery-half'; }
                    if (level <= 75) { return 'fa-battery-three-quarters'; }
                    return 'fa-battery-full';
                };

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
                            filter: 'all',
                            order:  'Name'
                        },
                        timeout: cancelToken.promise
                    }).then(function(resp) {
                        ctrl.loading = false;
                        var result = resp.data && resp.data.result;
                        if (!result) { return; }

                        var c         = cfg();
                        var threshold = parseInt(c.threshold, 10);
                        if (isNaN(threshold) || threshold < 1)   { threshold = 25; }
                        if (threshold > 100) { threshold = 100; }
                        var showFull  = c.showFull === true;
                        var sortBy    = c.sortBy || 'level';

                        // Filter: only devices that have a battery (level != 255 and defined)
                        var withBattery = result.filter(function(d) {
                            return typeof d.BatteryLevel !== 'undefined' &&
                                   d.BatteryLevel !== 255;
                        });

                        ctrl.totalWithBattery = withBattery.length;

                        // Apply threshold filter unless showFull is true
                        var visible = showFull
                            ? withBattery
                            : withBattery.filter(function(d) {
                                return d.BatteryLevel <= threshold;
                              });

                        // Sort
                        if (sortBy === 'name') {
                            visible.sort(function(a, b) {
                                return a.Name.localeCompare(b.Name);
                            });
                        } else {
                            visible.sort(function(a, b) {
                                return a.BatteryLevel - b.BatteryLevel;
                            });
                        }

                        ctrl.devices = visible.map(function(d) {
                            return {
                                name:       d.Name,
                                level:      d.BatteryLevel,
                                idx:        d.idx,
                                lastUpdate: d.LastUpdate || ''
                            };
                        });

                        ctrl.count = ctrl.devices.length;
                    }).catch(function(err) {
                        if (err.status === -1) { return; }
                        ctrl.loading   = false;
                        ctrl.loadError = true;
                    });
                }

                function scheduleRefresh() {
                    if (refreshTimer) { $interval.cancel(refreshTimer); refreshTimer = null; }
                    var interval = parseInt(cfg().refreshInterval, 10);
                    if (isNaN(interval) || interval <= 0) { interval = 300; }
                    refreshTimer = $interval(load, interval * 1000);
                }

                $scope.$on('dd:widget:refresh', load);

                $scope.$on('$destroy', function() {
                    if (cancelToken)  { cancelToken.resolve(); cancelToken = null; }
                    if (refreshTimer) { $interval.cancel(refreshTimer); refreshTimer = null; }
                });

                ctrl.$onInit = function() {
                    var c      = cfg();
                    ctrl.title = c.title || 'Battery Monitor';
                    load();
                    scheduleRefresh();
                };

                $scope.$watch(
                    function() {
                        var c = cfg();
                        return (c.threshold) + '|' + (c.showFull) + '|' + (c.sortBy) + '|' + (c.refreshInterval);
                    },
                    function(val, old) {
                        if (val !== old) {
                            var c      = cfg();
                            ctrl.title = c.title || 'Battery Monitor';
                            load();
                            scheduleRefresh();
                        }
                    }
                );
            }]
        };
    }]);
});
