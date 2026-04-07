define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    // API parameters for each chartType
    var CHART_TYPE_API = {
        day:   { sensor: 'rain', range: 'day' },
        week:  { sensor: 'rain', range: 'week' },
        month: { sensor: 'rain', range: 'month' },
        year:  { sensor: 'rain', range: 'year' }
    };

    var CHART_TYPE_LABELS = {
        day:   'Last 24h',
        week:  'Last Week',
        month: 'Last Month',
        year:  'Last Year'
    };

    widgetRegistry.register({
        type:        'rain-chart',
        label:       'Rain Chart',
        description: 'Rain rate and rainfall total charts',
        category:    'Charts & Data',
        icon:        'fa-solid fa-cloud-rain',
        defaultW:    4,
        defaultH:    3,
        minW:        3,
        minH:        2,
        maxW:        12,
        maxH:        8,
        transparentBackground: true,
        configSchema: [
            {
                key:          'deviceIdx',
                type:         'device-picker',
                label:        'Device',
                required:     true,
                deviceFilter: 'rain'
            },
            {
                key:     'chartType',
                type:    'select',
                label:   'Chart type',
                options: [
                    { value: 'day',   label: 'Last 24h' },
                    { value: 'week',  label: 'Last Week' },
                    { value: 'month', label: 'Last Month' },
                    { value: 'year',  label: 'Last Year' }
                ],
                default: 'day'
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

    app.directive('ddRainChartWidget', ['$http', '$timeout', '$q', function($http, $timeout, $q) {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/rain-chart.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$timeout', '$q', function($scope, $http, $timeout, $q) {
                var ctrl = this;
                ctrl.chartId    = 'dd-rain-chart-' + (++chartIdCounter);
                ctrl.deviceName = '';
                var chart        = null;
                var cancelToken  = null;
                var lastDeviceIdx = null;
                var resizeObserver = null;

                function setupResizeObserver(container) {
                    if (resizeObserver) { resizeObserver.disconnect(); }
                    if (!window.ResizeObserver || !container) { return; }
                    resizeObserver = new ResizeObserver(function(entries) {
                        if (!chart) { return; }
                        var r = entries[0] && entries[0].contentRect;
                        if (r && r.width > 0 && r.height > 0) {
                            chart.setSize(r.width, r.height, false);
                        }
                    });
                    resizeObserver.observe(container);
                }

                function getThemeColor(varName, fallback) {
                    var val = getComputedStyle(document.documentElement).getPropertyValue(varName).trim();
                    return val || fallback;
                }

                function resolveChartType(cfg) {
                    if (cfg.chartType && CHART_TYPE_API[cfg.chartType]) {
                        return cfg.chartType;
                    }
                    return 'day';
                }

                function load() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    if (!cfg.deviceIdx) { return; }

                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();

                    if (cfg.deviceIdx !== lastDeviceIdx) {
                        lastDeviceIdx = cfg.deviceIdx;
                        $http.get('json.htm?type=command&param=getdevices&rid=' + cfg.deviceIdx)
                            .then(function(resp) {
                                var d = resp.data && resp.data.result && resp.data.result[0];
                                ctrl.deviceName = cfg.title || (d && d.Name) || '';
                                fetchChartData(cfg);
                            });
                    } else {
                        ctrl.deviceName = cfg.title || ctrl.deviceName;
                        fetchChartData(cfg);
                    }
                }

                function fetchChartData(cfg) {
                    var chartType = resolveChartType(cfg);
                    var api = CHART_TYPE_API[chartType];
                    var url = 'json.htm?type=command&param=graph&sensor=' + api.sensor +
                              '&idx=' + cfg.deviceIdx + '&range=' + api.range;

                    $http.get(url, { timeout: cancelToken.promise })
                        .then(function(resp) {
                            $timeout(function() {
                                renderChart(resp.data, cfg, chartType);
                            }, 0);
                        })
                        .catch(function(err) {
                            if (err.status === -1) { return; }
                            ctrl.error = 'Failed to load data';
                        });
                }

                function destroyChart() {
                    if (chart) { chart.destroy(); chart = null; }
                }

                function baseChartOptions(container) {
                    var h = container.offsetHeight || 200;
                    return {
                        chart: {
                            animation:       false,
                            backgroundColor: 'transparent',
                            margin:          [10, 10, 30, 40],
                            style:           { fontFamily: 'inherit' },
                            height:          h,
                            width:           container.offsetWidth || null
                        },
                        title: {
                            text:  null,
                            align: 'center',
                            style: { fontSize: '11px', fontWeight: '600', color: getThemeColor('--dz-body-text', '#ccc') }
                        },
                        legend:    { enabled: false },
                        credits:   { enabled: false },
                        exporting: { enabled: false }
                    };
                }

                function titleForChartType(cfg, chartType) {
                    if (cfg.title) { return cfg.title; }
                    var base  = ctrl.deviceName || '';
                    var label = CHART_TYPE_LABELS[chartType] || '';
                    return base && label ? base + ' \u2014 ' + label : (base || label || null);
                }

                function renderChart(responseData, cfg, chartType) {
                    var container = document.getElementById(ctrl.chartId);
                    if (!container || !window.Highcharts) { return; }

                    destroyChart();

                    switch (chartType) {
                        case 'week':
                        case 'month':
                        case 'year':
                            chart = renderBarChart(container, responseData.result || [], cfg, chartType);
                            break;
                        default:
                            chart = renderDayChart(container, responseData.result || [], cfg);
                            break;
                    }

                    setupResizeObserver(container);
                }

                // ----------------------------------------------------------------
                // Chart: day — rain rate (mm/h) spline + cumulative area over 24h
                // ----------------------------------------------------------------

                function renderDayChart(container, data, cfg) {
                    var rateSeries       = [];
                    var cumulativeSeries = [];
                    var cumulative       = 0;

                    data.forEach(function(d) {
                        var ts = new Date(d.d).getTime();
                        var r  = parseFloat(d.r);
                        var v  = parseFloat(d.v);
                        if (!isNaN(r)) { rateSeries.push([ts, r]); }
                        if (!isNaN(v)) {
                            cumulative += v;
                            cumulativeSeries.push([ts, Math.round(cumulative * 10) / 10]);
                        }
                    });

                    var textColor = getThemeColor('--dz-body-text', '#ccc');
                    var opts = baseChartOptions(container);
                    opts.chart.margin = [10, 50, 30, 40];
                    opts.title.text   = titleForChartType(cfg, 'day');
                    opts.legend       = { enabled: true, itemStyle: { fontSize: '10px', color: textColor } };
                    opts.xAxis = {
                        type:   'datetime',
                        labels: { style: { fontSize: '10px' } }
                    };
                    opts.yAxis = [
                        {
                            title:  { text: 'mm/h', style: { fontSize: '10px' } },
                            labels: { style: { fontSize: '10px' } },
                            min:    0
                        },
                        {
                            title:     { text: 'mm', style: { fontSize: '10px' } },
                            labels:    { style: { fontSize: '10px' } },
                            min:       0,
                            opposite:  true
                        }
                    ];
                    opts.tooltip = {
                        shared:      true,
                        xDateFormat: '%a %d %b %H:%M'
                    };
                    opts.series = [
                        {
                            type:    'spline',
                            name:    'Rain rate',
                            data:    rateSeries,
                            color:   'rgba(3,190,252,0.9)',
                            yAxis:   0,
                            tooltip: { valueSuffix: ' mm/h', valueDecimals: 1 },
                            marker:  { enabled: false }
                        },
                        {
                            type:      'area',
                            name:      'Cumulative',
                            data:      cumulativeSeries,
                            color:     'rgba(39,174,96,0.85)',
                            fillColor: {
                                linearGradient: { x1: 0, y1: 0, x2: 0, y2: 1 },
                                stops: [
                                    [0, 'rgba(39,174,96,0.35)'],
                                    [1, 'rgba(39,174,96,0.02)']
                                ]
                            },
                            yAxis:      1,
                            tooltip:    { valueSuffix: ' mm', valueDecimals: 1 },
                            lineWidth:  1.5,
                            marker:     { enabled: false }
                        }
                    ];

                    return window.Highcharts.chart(ctrl.chartId, opts);
                }

                // ----------------------------------------------------------------
                // Chart: week / month / year — daily or monthly rainfall bars
                // ----------------------------------------------------------------

                function renderBarChart(container, data, cfg, chartType) {
                    var totalSeries = [];

                    data.forEach(function(d) {
                        var ts = new Date(d.d).getTime();
                        var mm = parseFloat(d.mm !== undefined ? d.mm : d.v);
                        if (!isNaN(mm)) { totalSeries.push([ts, mm]); }
                    });

                    var opts = baseChartOptions(container);
                    opts.chart.type   = 'column';
                    opts.chart.margin = [10, 10, 30, 40];
                    opts.title.text   = titleForChartType(cfg, chartType);
                    opts.xAxis = {
                        type:   'datetime',
                        labels: { style: { fontSize: '10px' } }
                    };
                    opts.yAxis = {
                        title:  { text: 'mm', style: { fontSize: '10px' } },
                        labels: { style: { fontSize: '10px' } },
                        min:    0
                    };
                    opts.tooltip = {
                        valueSuffix:   ' mm',
                        valueDecimals: 1,
                        xDateFormat:   chartType === 'year' ? '%b %Y' : '%a %d %b'
                    };
                    opts.plotOptions = {
                        column: { borderWidth: 0, groupPadding: 0.05 }
                    };
                    opts.series = [
                        {
                            name:  'Rainfall',
                            data:  totalSeries,
                            color: 'rgba(3,190,252,0.85)'
                        }
                    ];

                    return window.Highcharts.chart(ctrl.chartId, opts);
                }

                // ----------------------------------------------------------------
                // Empty state chart (no data)
                // ----------------------------------------------------------------

                function renderEmpty(container, cfg, chartType) {
                    var opts = baseChartOptions(container);
                    opts.title.text = titleForChartType(cfg, chartType);
                    opts.series = [];
                    opts.xAxis  = {};
                    opts.yAxis  = {};
                    return window.Highcharts.chart(ctrl.chartId, opts);
                }

                // ----------------------------------------------------------------
                // Reactivity / lifecycle
                // ----------------------------------------------------------------

                var refreshDebounce = null;
                $scope.$on('device_update', function(e, updated) {
                    var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                    if (!cfg || String(updated.idx) !== String(cfg.deviceIdx)) { return; }
                    if (refreshDebounce) { $timeout.cancel(refreshDebounce); }
                    refreshDebounce = $timeout(load, 60000);
                });

                $scope.$on('$destroy', function() {
                    if (resizeObserver) { resizeObserver.disconnect(); resizeObserver = null; }
                    if (cancelToken)    { cancelToken.resolve();       cancelToken    = null; }
                    if (chart)          { chart.destroy();             chart          = null; }
                    if (refreshDebounce) { $timeout.cancel(refreshDebounce); }
                });

                $scope.$on('dd:widget:refresh', load);

                $scope.$watch(
                    function() {
                        var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                        if (!cfg) { return ''; }
                        return cfg.deviceIdx + '|' + resolveChartType(cfg) + '|' + (cfg.title || '');
                    },
                    function(val, old) {
                        if (val === old) { return; }

                        var cfg      = ctrl.widgetDef && ctrl.widgetDef.config;
                        var oldParts = old.split('|');
                        var newParts = val.split('|');

                        if (chart && oldParts[0] === newParts[0] && oldParts[1] === newParts[1]) {
                            ctrl.deviceName = cfg.title || ctrl.deviceName;
                            chart.setTitle({ text: ctrl.deviceName || null });
                        } else {
                            load();
                        }
                    }
                );

                ctrl.$onInit = load;
            }]
        };
    }]);
});
