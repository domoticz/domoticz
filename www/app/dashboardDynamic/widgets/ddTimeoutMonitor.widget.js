define([
    'app',
    'dashboardDynamic/widgetRegistry.service',
    'dashboardDynamic/ddVisibility.service'
], function(app, widgetRegistry, ddVisibility) {
    'use strict';

    widgetRegistry.register({
        type:        'timeout-monitor',
        label:       'Timeout Monitor',
        description: 'Shows all devices that have not reported within their expected interval',
        category:    'System',
        icon:        'fa-solid fa-clock-rotate-left',
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
                key:     'sortBy',
                type:    'select',
                label:   'Sort by',
                default: 'lastUpdate',
                options: [
                    { value: 'lastUpdate', label: 'Last update (oldest first)' },
                    { value: 'name',       label: 'Device name (A–Z)' }
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

    app.directive('ddTimeoutMonitorWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/timeout-monitor.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', '$q', 'ddVisibility', function($scope, $http, $interval, $q, ddVisibility) {
                var ctrl = this;

                ctrl.title          = 'Timeout Monitor';
                ctrl.devices        = [];
                ctrl.count          = 0;
                ctrl.totalMonitored = 0;
                ctrl.loading        = false;
                ctrl.loadError      = false;

                var refreshTimer = null;
                var cancelToken  = null;

                function cfg() {
                    return (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                }

                ctrl.timeAgo = function(lastUpdate) {
                    if (!lastUpdate) { return '–'; }
                    var d = new Date(lastUpdate.replace(' ', 'T'));
                    if (isNaN(d.getTime())) { return '–'; }
                    var diff = Math.floor((Date.now() - d.getTime()) / 1000);
                    if (diff < 0)     { return 'just now'; }
                    if (diff < 60)    { return 'just now'; }
                    if (diff < 3600)  { return Math.floor(diff / 60) + 'm ago'; }
                    if (diff < 86400) { return Math.floor(diff / 3600) + 'h ago'; }
                    return Math.floor(diff / 86400) + 'd ago';
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

                        var c      = cfg();
                        var sortBy = c.sortBy || 'lastUpdate';

                        ctrl.totalMonitored = result.filter(function(d) {
                            return typeof d.HaveTimeout !== 'undefined';
                        }).length;

                        var timedOut = result.filter(function(d) {
                            return d.HaveTimeout === true;
                        });

                        if (sortBy === 'name') {
                            timedOut.sort(function(a, b) {
                                return a.Name.localeCompare(b.Name);
                            });
                        } else {
                            timedOut.sort(function(a, b) {
                                var da = new Date((a.LastUpdate || '').replace(' ', 'T'));
                                var db = new Date((b.LastUpdate || '').replace(' ', 'T'));
                                return da - db;
                            });
                        }

                        ctrl.devices = timedOut.map(function(d) {
                            return {
                                name:       d.Name,
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

                function stopTimer() {
                    if (refreshTimer) { $interval.cancel(refreshTimer); refreshTimer = null; }
                }

                function startTimer() {
                    stopTimer();
                    var interval = parseInt(cfg().refreshInterval, 10);
                    if (isNaN(interval) || interval <= 0) { interval = 300; }
                    refreshTimer = $interval(load, interval * 1000);
                }

                $scope.$on('dd:page:hidden',  function() { stopTimer(); });
                $scope.$on('dd:page:visible', function() { load(); startTimer(); });

                $scope.$on('dd:widget:refresh', load);

                $scope.$on('$destroy', function() {
                    if (cancelToken)  { cancelToken.resolve(); cancelToken = null; }
                    if (refreshTimer) { $interval.cancel(refreshTimer); refreshTimer = null; }
                });

                ctrl.$onInit = function() {
                    var c      = cfg();
                    ctrl.title = c.title || 'Timeout Monitor';
                    load();
                    if (!ddVisibility.isHidden()) { startTimer(); }
                };

                $scope.$watch(
                    function() {
                        var c = cfg();
                        return (c.sortBy) + '|' + (c.refreshInterval);
                    },
                    function(val, old) {
                        if (val !== old) {
                            var c      = cfg();
                            ctrl.title = c.title || 'Timeout Monitor';
                            load();
                            startTimer();
                        }
                    }
                );
            }]
        };
    }]);
});
