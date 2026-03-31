define([
    'app',
    'dashboard2/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'energy-chart',
        label:       'Energy Chart',
        description: 'kWh consumption bar chart',
        category:    'Charts & Data',
        icon:        'fa-solid fa-bolt',
        defaultW:    4,
        defaultH:    3,
        minW:        3,
        minH:        2,
        maxW:        12,
        maxH:        8,
        configSchema: [
            {
                key:      'deviceIdx',
                type:     'device-picker',
                label:    'Device',
                required: true
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

    app.directive('db2EnergyChartWidget', ['$http', '$timeout', '$q', function($http, $timeout, $q) {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboard2/widgets/energy-chart.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$timeout', '$q', function($scope, $http, $timeout, $q) {
                var ctrl = this;
                ctrl.chartId = 'db2-energy-chart-' + (++chartIdCounter);
                var chart = null;
                var cancelToken = null;

                function getThemeColor(varName, fallback) {
                    var val = getComputedStyle(document.documentElement).getPropertyValue(varName).trim();
                    return val || fallback;
                }

                function load() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    if (!cfg.deviceIdx) { return; }

                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();

                    var range = cfg.range || 'week';
                    var url = 'json.htm?type=command&param=graph&sensor=counter' +
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

                    // Energy data: divide Wh by 1000 to display kWh
                    var usage = data.map(function(d) {
                        var ts  = new Date(d.d).getTime();
                        var val = parseFloat(d.v || d.v1 || 0) / 1000;
                        return [ts, val];
                    }).filter(function(pt) {
                        return !isNaN(pt[1]);
                    });

                    if (chart) {
                        chart.destroy();
                        chart = null;
                    }

                    chart = window.Highcharts.chart(ctrl.chartId, {
                        chart: {
                            type:            'column',
                            animation:       false,
                            backgroundColor: 'transparent',
                            margin:          [10, 10, 30, 40],
                            style:           { fontFamily: 'inherit' }
                        },
                        title:   { text: null },
                        legend:  { enabled: false },
                        credits: { enabled: false },
                        xAxis: {
                            type:   'datetime',
                            labels: { style: { fontSize: '10px' } }
                        },
                        yAxis: {
                            title:  { text: 'kWh', style: { fontSize: '10px' } },
                            labels: { style: { fontSize: '10px' } }
                        },
                        tooltip: {
                            valueSuffix: ' kWh',
                            valueDecimals: 2,
                            xDateFormat:  '%a %d %b %H:%M'
                        },
                        series: [{
                            name:  cfg.title || 'Energy',
                            data:  usage,
                            color: getThemeColor('--dz-btn-primary-bg', '#337ab7')
                        }]
                    });
                }

                $scope.$on('$destroy', function() {
                    if (cancelToken) { cancelToken.resolve(); cancelToken = null; }
                    if (chart) { chart.destroy(); chart = null; }
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
