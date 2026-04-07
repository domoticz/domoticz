define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    // Map legacy `range` values to the new `chartType` values so saved dashboards
    // that still have range=day/week/month/year continue to work unchanged.
    var RANGE_TO_CHART_TYPE = {
        day:   'last24h',
        week:  'last7days',
        month: 'last_month',
        year:  'last_year'
    };

    // For each chartType: which sensor/range to request from the API.
    var CHART_TYPE_API = {
        last24h:       { sensor: 'temp', range: 'day' },
        last7days:     { sensor: 'temp', range: 'week' },
        last_month:    { sensor: 'temp', range: 'month' },
        last_year:     { sensor: 'temp', range: 'year' },
        dewpoint:      { sensor: 'temp', range: 'day' },
        temp_humidity: { sensor: 'temp', range: 'day' },
        comfort_zone:  { sensor: 'temp', range: 'week' }
    };

    var CHART_TYPE_LABELS = {
        last24h:       'Last 24 hours',
        last7days:     'Last 7 days',
        last_month:    'Last month',
        last_year:     'Last year',
        dewpoint:      'Dew Point',
        temp_humidity: 'Temp vs Humidity',
        comfort_zone:  'Comfort Zone'
    };

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
        transparentBackground: true,
        configSchema: [
            {
                key:          'deviceIdx',
                type:         'device-picker',
                label:        'Device',
                required:     true,
                deviceFilter: 'temp'
            },
            {
                key:     'chartType',
                type:    'select',
                label:   'Chart type',
                options: [
                    { value: 'last24h',       label: 'Last 24 hours' },
                    { value: 'last7days',     label: 'Last 7 days' },
                    { value: 'last_month',    label: 'Last month' },
                    { value: 'last_year',     label: 'Last year' },
                    { value: 'dewpoint',      label: 'Dew Point' },
                    { value: 'temp_humidity', label: 'Temp vs Humidity' },
                    { value: 'comfort_zone',  label: 'Comfort Zone' }
                ],
                default: 'last24h'
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

    app.directive('ddTemperatureGraphWidget', ['$http', '$timeout', '$q', function($http, $timeout, $q) {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/temperature-graph.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$timeout', '$q', function($scope, $http, $timeout, $q) {
                var ctrl = this;
                ctrl.chartId    = 'dd-temp-chart-' + (++chartIdCounter);
                ctrl.deviceName = '';
                var chart = null;
                var cancelToken = null;
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

                // Resolve the effective chartType, applying the legacy `range` mapping.
                function resolveChartType(cfg) {
                    if (cfg.chartType) {
                        return cfg.chartType;
                    }
                    if (cfg.range && RANGE_TO_CHART_TYPE[cfg.range]) {
                        return RANGE_TO_CHART_TYPE[cfg.range];
                    }
                    return 'last24h';
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

                    var chartType = resolveChartType(cfg);
                    var api = CHART_TYPE_API[chartType] || { sensor: 'temp', range: 'day' };
                    var url = 'json.htm?type=command&param=graph&sensor=' + api.sensor +
                              '&idx=' + cfg.deviceIdx + '&range=' + api.range;

                    $http.get(url, { timeout: cancelToken.promise })
                        .then(function(resp) {
                            var data = resp.data.result || [];
                            $timeout(function() { renderChart(data, cfg, chartType); }, 0);
                        })
                        .catch(function(err) {
                            if (err.status === -1) { return; }
                            ctrl.error = 'Failed to load data';
                        });
                }

                // ----------------------------------------------------------------
                // Shared chart setup helpers
                // ----------------------------------------------------------------

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
                            text:  ctrl.deviceName || null,
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
                    var base = ctrl.deviceName || '';
                    var label = CHART_TYPE_LABELS[chartType] || '';
                    return base && label ? base + ' \u2014 ' + label : (base || label || null);
                }

                // ----------------------------------------------------------------
                // Render dispatch
                // ----------------------------------------------------------------

                function renderChart(data, cfg, chartType) {
                    var container = document.getElementById(ctrl.chartId);
                    if (!container || !window.Highcharts) { return; }

                    destroyChart();

                    switch (chartType) {
                        case 'dewpoint':
                            chart = renderDewPoint(container, data, cfg);
                            break;
                        case 'temp_humidity':
                            chart = renderTempHumidity(container, data, cfg);
                            break;
                        case 'comfort_zone':
                            chart = renderComfortZone(container, data, cfg);
                            break;
                        default:
                            chart = renderSimpleLine(container, data, cfg, chartType);
                            break;
                    }

                    setupResizeObserver(container);
                }

                // ----------------------------------------------------------------
                // Chart type: simple spline (last24h, last7days, last_month, last_year)
                // ----------------------------------------------------------------

                function renderSimpleLine(container, data, cfg, chartType) {
                    var series = data.map(function(d) {
                        return [new Date(d.d).getTime(), parseFloat(d.te !== undefined ? d.te : d.v)];
                    }).filter(function(pt) { return !isNaN(pt[1]); });

                    var opts = baseChartOptions(container);
                    opts.chart.type = 'spline';
                    opts.title.text = titleForChartType(cfg, chartType);
                    opts.xAxis = {
                        type:   'datetime',
                        labels: { style: { fontSize: '10px' } }
                    };
                    opts.yAxis = {
                        title:  { text: '\u00b0C', style: { fontSize: '10px' } },
                        labels: { style: { fontSize: '10px' } }
                    };
                    opts.tooltip = {
                        valueSuffix:  ' \u00b0C',
                        xDateFormat: '%a %d %b %H:%M'
                    };
                    opts.series = [{
                        name:  'Temperature',
                        data:  series,
                        color: getThemeColor('--dz-accent-danger', '#e74c3c')
                    }];

                    return window.Highcharts.chart(ctrl.chartId, opts);
                }

                // ----------------------------------------------------------------
                // Chart type: dewpoint — temperature + dew point on same axis
                // ----------------------------------------------------------------

                function renderDewPoint(container, data, cfg) {
                    var tempSeries = [];
                    var dewSeries  = [];

                    data.forEach(function(d) {
                        var ts = new Date(d.d).getTime();
                        var te = parseFloat(d.te);
                        var td = parseFloat(d.td);
                        if (!isNaN(te)) { tempSeries.push([ts, te]); }
                        if (!isNaN(td)) { dewSeries.push([ts, td]); }
                    });

                    var opts = baseChartOptions(container);
                    opts.chart.type = 'spline';
                    opts.title.text = titleForChartType(cfg, 'dewpoint');
                    opts.legend = { enabled: true, itemStyle: { fontSize: '10px', color: getThemeColor('--dz-body-text', '#ccc') } };
                    opts.xAxis = {
                        type:   'datetime',
                        labels: { style: { fontSize: '10px' } }
                    };
                    opts.yAxis = {
                        title:  { text: '\u00b0C', style: { fontSize: '10px' } },
                        labels: { style: { fontSize: '10px' } }
                    };
                    opts.tooltip = {
                        shared:       true,
                        valueSuffix:  ' \u00b0C',
                        xDateFormat: '%a %d %b %H:%M'
                    };
                    opts.series = [
                        {
                            name:  'Temperature',
                            data:  tempSeries,
                            color: getThemeColor('--dz-accent-danger', '#e74c3c')
                        },
                        {
                            name:       'Dew Point',
                            data:       dewSeries,
                            color:      getThemeColor('--dz-accent-info', '#3498db'),
                            dashStyle:  'ShortDash'
                        }
                    ];

                    return window.Highcharts.chart(ctrl.chartId, opts);
                }

                // ----------------------------------------------------------------
                // Chart type: temp_humidity — dual y-axis (temp + humidity)
                // ----------------------------------------------------------------

                function renderTempHumidity(container, data, cfg) {
                    var tempSeries = [];
                    var humSeries  = [];

                    data.forEach(function(d) {
                        var ts = new Date(d.d).getTime();
                        var te = parseFloat(d.te);
                        var hu = parseFloat(d.hu);
                        if (!isNaN(te)) { tempSeries.push([ts, te]); }
                        if (!isNaN(hu)) { humSeries.push([ts, hu]); }
                    });

                    var textColor = getThemeColor('--dz-body-text', '#ccc');

                    var opts = baseChartOptions(container);
                    opts.chart.type  = 'spline';
                    opts.chart.margin = [10, 45, 30, 40];
                    opts.title.text  = titleForChartType(cfg, 'temp_humidity');
                    opts.legend = { enabled: true, itemStyle: { fontSize: '10px', color: textColor } };
                    opts.xAxis = {
                        type:   'datetime',
                        labels: { style: { fontSize: '10px' } }
                    };
                    opts.yAxis = [
                        {
                            title:  { text: '\u00b0C', style: { fontSize: '10px' } },
                            labels: { style: { fontSize: '10px' } }
                        },
                        {
                            title:    { text: '%', style: { fontSize: '10px' } },
                            labels:   { style: { fontSize: '10px' } },
                            opposite: true,
                            min:      0,
                            max:      100
                        }
                    ];
                    opts.tooltip = {
                        shared:      true,
                        xDateFormat: '%a %d %b %H:%M'
                    };
                    opts.series = [
                        {
                            name:         'Temperature',
                            data:         tempSeries,
                            yAxis:        0,
                            tooltip:      { valueSuffix: ' \u00b0C' },
                            color:        getThemeColor('--dz-accent-danger', '#e74c3c')
                        },
                        {
                            name:         'Humidity',
                            data:         humSeries,
                            yAxis:        1,
                            tooltip:      { valueSuffix: ' %' },
                            color:        getThemeColor('--dz-accent-info', '#3498db')
                        }
                    ];

                    return window.Highcharts.chart(ctrl.chartId, opts);
                }

                // ----------------------------------------------------------------
                // Chart type: comfort_zone — scatter of humidity (x) vs temp (y)
                // with a plotBand shading the comfort zone (18-24°C / 40-60% RH)
                // ----------------------------------------------------------------

                function renderComfortZone(container, data, cfg) {
                    var scatterData = [];

                    data.forEach(function(d) {
                        var te = parseFloat(d.te);
                        var hu = parseFloat(d.hu);
                        if (!isNaN(te) && !isNaN(hu)) {
                            scatterData.push([hu, te]);
                        }
                    });

                    var textColor = getThemeColor('--dz-body-text', '#ccc');

                    var opts = baseChartOptions(container);
                    opts.chart.type   = 'scatter';
                    opts.chart.margin = [10, 10, 40, 45];
                    opts.title.text   = titleForChartType(cfg, 'comfort_zone');
                    opts.legend       = { enabled: false };
                    opts.xAxis = {
                        title:  { text: 'Humidity (%)', style: { fontSize: '10px' } },
                        labels: { style: { fontSize: '10px' } },
                        min:    0,
                        max:    100,
                        // Shade the comfort-zone humidity band (40-60 %)
                        plotBands: [{
                            from:  40,
                            to:    60,
                            color: 'rgba(39,174,96,0.12)',
                            label: { text: 'Comfort', style: { color: '#27ae60', fontSize: '9px' } }
                        }]
                    };
                    opts.yAxis = {
                        title:  { text: '\u00b0C', style: { fontSize: '10px' } },
                        labels: { style: { fontSize: '10px' } },
                        // Shade the comfort-zone temperature band (18-24 °C)
                        plotBands: [{
                            from:  18,
                            to:    24,
                            color: 'rgba(39,174,96,0.12)',
                            label: { text: '18\u201324\u00b0C', style: { color: '#27ae60', fontSize: '9px' }, align: 'right', x: -4 }
                        }]
                    };
                    opts.tooltip = {
                        formatter: function() {
                            return 'Humidity: <b>' + this.x + ' %</b><br>Temp: <b>' + this.y + ' \u00b0C</b>';
                        }
                    };
                    opts.plotOptions = {
                        scatter: {
                            marker: {
                                radius: 3,
                                symbol: 'circle'
                            }
                        }
                    };
                    opts.series = [{
                        name:   'Temp vs Humidity',
                        data:   scatterData,
                        color:  getThemeColor('--dz-accent-primary', '#2980b9'),
                        marker: { fillColor: getThemeColor('--dz-accent-primary', '#2980b9') }
                    }];

                    return window.Highcharts.chart(ctrl.chartId, opts);
                }

                // ----------------------------------------------------------------
                // Reactivity / lifecycle
                // ----------------------------------------------------------------

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
                    if (resizeObserver) { resizeObserver.disconnect(); resizeObserver = null; }
                    if (cancelToken) { cancelToken.resolve(); cancelToken = null; }
                    if (chart) { chart.destroy(); chart = null; }
                    if (refreshDebounce) { $timeout.cancel(refreshDebounce); }
                });

                $scope.$on('dd:widget:refresh', load);

                $scope.$watch(
                    function() {
                        var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                        if (!cfg) { return ''; }
                        // Watch both chartType (new) and range (legacy) so either change triggers a reload.
                        return cfg.deviceIdx + '|' + resolveChartType(cfg) + '|' + (cfg.title || '');
                    },
                    function(val, old) {
                        if (val === old) { return; }

                        var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                        var oldParts = old.split('|');
                        var newParts = val.split('|');

                        // If only the title changed (same device + same chart type), update in-place.
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
