define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:                  'water-summary',
        transparentBackground: true,
        label:       'Water Summary',
        description: 'Compact stat card showing today\'s water usage and total counter',
        category:    'Energy',
        icon:        'fa-solid fa-droplet',
        defaultW:    2,
        defaultH:    2,
        minW:        2,
        minH:        2,
        maxW:        4,
        maxH:        3,
        configSchema: [
            {
                key:          'deviceIdx',
                type:         'device-picker',
                label:        'Device',
                deviceFilter: 'water',
                required:     true
            },
            {
                key:      'title',
                type:     'text',
                label:    'Title (optional, falls back to device name)',
                required: false
            },
            {
                key:          'ranges',
                type:         'range-list',
                label:        'Bar ranges',
                help:         'Add value ranges to show a gradient bar on today\'s usage. Bar auto-scales to the combined min/max of all ranges.',
                seedDefaults: [
                    { from: 0,   to: 100,  color: '#29b6f6' },
                    { from: 100, to: 1000, color: '#DF2D3A' }
                ]
            },
            { key: 'showBackground', type: 'boolean', label: 'Show panel background', default: true }
        ]
    });

    app.directive('ddWaterSummaryWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/water-summary.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', '$q', function($scope, $http, $interval, $q) {
                var ctrl = this;
                ctrl.title        = '';
                ctrl.counterToday = null;
                ctrl.counterTotal = null;
                ctrl.price        = null;
                ctrl.numVal       = NaN;
                var cancelToken   = null;

                function applyDevice(d) {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    ctrl.title = cfg.title || d.Name || '';

                    // CounterToday: "0.123 m3" or "123 Liter" — keep full string for display
                    ctrl.counterToday = d.CounterToday || null;

                    // d.price (lowercase): today's water cost; 1000 = sentinel meaning "not configured"
                    var raw = parseFloat(d.price);
                    ctrl.price = (!isNaN(raw) && raw !== 1000 && raw !== 0) ? raw : null;

                    var todayMatch = (d.CounterToday || '').match(/^([\d.]+)/);
                    ctrl.numVal = todayMatch ? parseFloat(todayMatch[1]) : NaN;
                }

                function load() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    if (!cfg.deviceIdx) { return; }

                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();

                    var t = cancelToken.promise;
                    $q.all([
                        $http.get('json.htm', {
                            params:  { type: 'command', param: 'getdevices', rid: cfg.deviceIdx },
                            timeout: t
                        }),
                        $http.get('json.htm', {
                            params:  { type: 'command', param: 'graph', sensor: 'counter', idx: cfg.deviceIdx, range: 'year', actyear: new Date().getFullYear() },
                            timeout: t
                        })
                    ]).then(function(results) {
                        var d = results[0].data.result && results[0].data.result[0];
                        if (!d) { return; }
                        applyDevice(d);

                        // Year-to-date total: sum daily 'v' values from the counter year graph
                        var yearData = results[1].data && results[1].data.result;
                        if (yearData && yearData.length) {
                            var sum = yearData.reduce(function(acc, item) {
                                return acc + (parseFloat(item.v) || 0);
                            }, 0);
                            // Match the unit from today's reading (m3 or Liter)
                            var isLiter = ctrl.counterToday && ctrl.counterToday.indexOf('Liter') >= 0;
                            ctrl.counterTotal = isLiter
                                ? Math.round(sum * 1000) + ' Liter'
                                : sum.toFixed(3) + ' m3';
                        } else {
                            ctrl.counterTotal = null;
                        }
                    }).catch(function(err) {
                        if (err.status === -1) { return; }
                        ctrl.loadError = true;
                    });
                }

                $scope.$on('device_update', function(e, updated) {
                    var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                    if (cfg && String(updated.idx) === String(cfg.deviceIdx)) {
                        applyDevice(updated);
                    }
                });

                var timer = $interval(load, 60000);

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
