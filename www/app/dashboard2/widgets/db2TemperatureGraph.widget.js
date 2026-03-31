define([
    'app',
    'dashboard2/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'temperature-graph',
        label:       'Temperature Graph',
        description: 'Temperature/humidity history chart',
        category:    'Charts & Data',
        icon:        'fa-solid fa-chart-line',
        defaultW:    4,
        defaultH:    3,
        minW:        3,
        minH:        2,
        maxW:        12,
        maxH:        8,
        configSchema: [
            {
                key:          'deviceIdx',
                type:         'device-picker',
                label:        'Device',
                required:     true,
                deviceFilter: 'temp'
            },
            {
                key:     'range',
                type:    'select',
                label:   'Time range',
                options: [
                    { value: 'day',   label: 'Last 24h' },
                    { value: 'week',  label: 'Last 7 days' },
                    { value: 'month', label: 'Last month' },
                    { value: 'year',  label: 'Last year' }
                ],
                default: 'week'
            },
            {
                key:      'title',
                type:     'text',
                label:    'Custom title',
                required: false
            }
        ]
    });

    var chartIdCounter = 0;

    app.directive('db2TemperatureGraphWidget', ['$http', '$timeout', '$q', function($http, $timeout, $q) {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboard2/widgets/temperature-graph.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$timeout', '$q', function($scope, $http, $timeout, $q) {
                var ctrl = this;
                ctrl.chartId    = 'db2-temp-chart-' + (++chartIdCounter);
                ctrl.deviceName = '';
                var chart = null;
                var cancelToken = null;
                var lastDeviceIdx = null;

                function getThemeColor(varName, fallback) {
                    var val = getComputedStyle(document.documentElement).getPropertyValue(varName).trim();
                    return val || fallback;
                }

                function load() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    if (!cfg.deviceIdx) { return; }

                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();

                    // Fetch device name if device changed
                    if (cfg.deviceIdx !== lastDeviceIdx) {
                        lastDeviceIdx = cfg.deviceIdx;
                        $http.get('json.htm?type=command&param=getdevices&rid=' + cfg.deviceIdx)
                            .then(function(resp) {
                                var d = resp.data && resp.data.result && resp.data.result[0];
                                ctrl.deviceName = cfg.title || (d && d.Name) || '';
                            });
                    } else {
                        ctrl.deviceName = cfg.title || ctrl.deviceName;
                    }

                    var range = cfg.range || 'week';
                    var url = 'json.htm?type=command&param=graph&sensor=temp' +
                              '&idx=' + cfg.deviceIdx + '&range=' + range;

                    $http.get(url, { timeout: cancelToken.promise })
                        .then(function(resp) {
                            var data = resp.data.result || [];
                            $timeout(function() { renderChart(data, cfg); }, 0);
                        })
                        .catch(function(err) {
                            if (err.status === -1) { return; }
                            ctrl.error = 'Failed to load data';
                            ctrl.loading = false;
                        });
                }

                function renderChart(data, cfg) {
                    var container = document.getElementById(ctrl.chartId);
                    if (!container || !window.Highcharts) { return; }

                    var temps = data.map(function(d) {
                        return [new Date(d.d).getTime(), parseFloat(d.te !== undefined ? d.te : d.v)];
                    }).filter(function(pt) {
                        return !isNaN(pt[1]);
                    });

                    if (chart) {
                        chart.destroy();
                        chart = null;
                    }

                    chart = window.Highcharts.chart(ctrl.chartId, {
                        chart: {
                            type:            'spline',
                            animation:       false,
                            backgroundColor: 'transparent',
                            margin:          [10, 10, 30, 40],
                            style:           { fontFamily: 'inherit' },
                            height:          '100%'
                        },
                        title:   { text: null },
                        legend:  { enabled: false },
                        credits: { enabled: false },
                        xAxis: {
                            type:   'datetime',
                            labels: { style: { fontSize: '10px' } }
                        },
                        yAxis: {
                            title:  { text: '\u00b0C', style: { fontSize: '10px' } },
                            labels: { style: { fontSize: '10px' } }
                        },
                        tooltip: {
                            valueSuffix:  ' \u00b0C',
                            xDateFormat: '%a %d %b %H:%M'
                        },
                        series: [{
                            name:  cfg.title || 'Temperature',
                            data:  temps,
                            color: getThemeColor('--dz-accent-danger', '#e74c3c')
                        }]
                    });
                }

                // Debounced reload on device update — charts show historical data so
                // reloading more than once per minute is wasteful
                var refreshDebounce = null;
                $scope.$on('device_update', function(e, updated) {
                    var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                    if (!cfg || String(updated.idx) !== String(cfg.deviceIdx)) { return; }
                    if (refreshDebounce) { $timeout.cancel(refreshDebounce); }
                    refreshDebounce = $timeout(load, 60000);
                });

                $scope.$on('$destroy', function() {
                    if (cancelToken) { cancelToken.resolve(); cancelToken = null; }
                    if (chart) { chart.destroy(); chart = null; }
                    if (refreshDebounce) { $timeout.cancel(refreshDebounce); }
                });

                $scope.$on('db2:widget:refresh', load);

                $scope.$watch(
                    function() {
                        var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                        return cfg ? (cfg.deviceIdx + '|' + cfg.range) : '';
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
