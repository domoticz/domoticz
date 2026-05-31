define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:                  'gas-summary',
        transparentBackground: true,
        label:       'Gas Summary',
        description: 'Compact stat card showing today\'s gas usage and total counter',
        category:    'Energy',
        icon:        'fa-solid fa-fire',
        defaultW:    2,
        defaultH:    2,
        minW:        2,
        minH:        2,
        maxW:        4,
        maxH:        3,
        configSchema: [
            {
                key:      'deviceIdx',
                type:     'device-picker',
                label:    'Device',
                deviceFilter: 'gas',
                required: true
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
                    { from: 0, to: 1,  color: '#66bb6a' },
                    { from: 1, to: 10, color: '#DF2D3A' }
                ]
            },
            { key: 'showBackground', type: 'boolean', label: 'Show panel background', default: true }
        ]
    });

    app.directive('ddGasSummaryWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/gas-summary.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$q', function($scope, $http, $q) {
                var ctrl = this;
                ctrl.title        = '';
                ctrl.counterToday = null;
                ctrl.counterTotal = null;
                ctrl.price        = null;
                ctrl.numVal       = NaN;
                var cancelToken   = null;
                var _ytdBase      = null;  // sum of completed days this year (excl. today); null = not yet loaded
                var _lastDate     = null;  // 'YYYY-MM-DD' of last year-graph fetch

                function todayStr() {
                    var n = new Date();
                    var mo = n.getMonth() + 1;
                    var d  = n.getDate();
                    return n.getFullYear() + '-' + (mo < 10 ? '0' + mo : mo) + '-' + (d < 10 ? '0' + d : d);
                }

                function applyDevice(d) {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    ctrl.title = cfg.title || d.Name || '';
                    ctrl.counterToday = d.CounterToday || null;

                    var raw = parseFloat(d.price);
                    ctrl.price = (!isNaN(raw) && raw !== 1000 && raw !== 0) ? raw : null;

                    var todayMatch = (d.CounterToday || '').match(/^([\d.]+)/);
                    ctrl.numVal = todayMatch ? parseFloat(todayMatch[1]) : NaN;
                }

                function computeTotal() {
                    if (_ytdBase === null) { return; }
                    var todayMatch = (ctrl.counterToday || '').match(/^([\d.]+)\s*(.*)/);
                    var todayVal = todayMatch ? parseFloat(todayMatch[1]) : 0;
                    var unit = (todayMatch && todayMatch[2]) ? todayMatch[2].trim() : 'm3';
                    ctrl.counterTotal = (_ytdBase + todayVal).toFixed(3) + ' ' + unit;
                }

                function applyYearData(yearData) {
                    var today = todayStr();
                    _lastDate = today;
                    if (!yearData || !yearData.length) {
                        _ytdBase = null;
                        return;
                    }
                    var base = 0;
                    yearData.forEach(function(item) {
                        if (item.d !== today) { base += parseFloat(item.v) || 0; }
                    });
                    _ytdBase = base;
                    computeTotal();
                }

                function fetchYearBase() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    if (!cfg.deviceIdx) { return; }
                    $http.get('json.htm', {
                        params: { type: 'command', param: 'graph', sensor: 'counter', idx: cfg.deviceIdx, range: 'year', actyear: new Date().getFullYear() }
                    }).then(function(resp) {
                        applyYearData(resp.data && resp.data.result);
                    }).catch(function(err) {
                        if (err.status === -1) { return; }
                    });
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
                        applyYearData(results[1].data && results[1].data.result);
                    }).catch(function(err) {
                        if (err.status === -1) { return; }
                        ctrl.loadError = true;
                    });
                }

                $scope.$on('device_update', function(e, updated) {
                    var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                    if (cfg && String(updated.idx) === String(cfg.deviceIdx)) {
                        applyDevice(updated);
                        if (_ytdBase !== null) { computeTotal(); }
                    }
                });

                $scope.$on('time_update', function(e, data) {
                    var today = todayStr();
                    if (_lastDate && _lastDate !== today) {
                        fetchYearBase();
                    }
                });

                $scope.$on('$destroy', function() {
                    if (cancelToken) { cancelToken.resolve(); cancelToken = null; }
                });

                $scope.$on('dd:widget:refresh', load);

                $scope.$watch(
                    function() {
                        return ctrl.widgetDef && ctrl.widgetDef.config && ctrl.widgetDef.config.deviceIdx;
                    },
                    function(val, old) {
                        if (val !== old) { _ytdBase = null; load(); }
                    }
                );

                ctrl.$onInit = load;
            }]
        };
    }]);
});
