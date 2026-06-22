define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    // API parameters for each chartType
    var CHART_TYPE_API = {
        shortlog:  { sensor: 'wind',    range: 'day' },
        week:      { sensor: 'wind',    range: 'day' },
        direction: { sensor: 'winddir', range: 'day' },
        frequency: { sensor: 'wind',    range: 'day' },
        month:     { sensor: 'wind',    range: 'month' },
        year:      { sensor: 'wind',    range: 'year' }
    };

    var CHART_TYPE_LABELS = {
        shortlog:  'Last 24h',
        week:      'Last 7 days',
        direction: 'Wind Direction',
        frequency: 'Speed Frequency',
        month:     'Last Month',
        year:      'Last Year'
    };

    var WIND_DIRECTIONS = ['N', 'NNE', 'NE', 'ENE', 'E', 'ESE', 'SE', 'SSE',
                           'S', 'SSW', 'SW', 'WSW', 'W', 'WNW', 'NW', 'NNW'];

    // Beaufort scale thresholds in m/s (internal Domoticz wind unit)
    // The API returns speed in the device's configured unit; we use the numeric
    // value as-is and annotate the tooltip with a Beaufort description.
    var BEAUFORT_LABELS = [
        'Calm',           // 0
        'Light Air',      // 1
        'Light Breeze',   // 2
        'Gentle Breeze',  // 3
        'Moderate Breeze',// 4
        'Fresh Breeze',   // 5
        'Strong Breeze',  // 6
        'Near Gale',      // 7
        'Gale',           // 8
        'Strong Gale',    // 9
        'Storm',          // 10
        'Violent Storm',  // 11
        'Hurricane'       // 12
    ];

    // Domoticz stores wind speed in m/s internally; Beaufort thresholds in m/s
    var BEAUFORT_THRESHOLDS_MS = [0.3, 1.6, 3.4, 5.5, 8.0, 10.8, 13.9, 17.2, 20.8, 24.5, 28.5, 32.7];

    function beaufortFromMs(ms) {
        for (var i = BEAUFORT_THRESHOLDS_MS.length - 1; i >= 0; i--) {
            if (ms >= BEAUFORT_THRESHOLDS_MS[i]) { return i + 1; }
        }
        return 0;
    }

    widgetRegistry.register({
        type:        'wind-chart',
        label:       'Wind Chart',
        description: 'Wind speed, direction and frequency charts',
        category:    'Charts & Data',
        icon:        'fa-solid fa-wind',
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
                deviceFilter: 'wind'
            },
            {
                key:     'chartType',
                type:    'select',
                label:   'Chart type',
                options: [
                    { value: 'shortlog',  label: 'Last 24h' },
                    { value: 'week',      label: 'Last 7 days' },
                    { value: 'direction', label: 'Wind Direction' },
                    { value: 'frequency', label: 'Speed Frequency' },
                    { value: 'month',     label: 'Last Month' },
                    { value: 'year',      label: 'Last Year' }
                ],
                default: 'shortlog'
            },
            {
                key:      'title',
                type:     'text',
                label:    'Custom title',
                required: false
            },
            { key: 'showBackground', type: 'boolean', label: 'Show panel background', default: true }
        ]
    });

    var chartIdCounter = 0;

    app.directive('ddWindChartWidget', ['$http', '$timeout', '$q', function($http, $timeout, $q) {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/wind-chart.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$timeout', '$q', function($scope, $http, $timeout, $q) {
                var ctrl = this;
                ctrl.chartId    = 'dd-wind-chart-' + (++chartIdCounter);
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
                    return 'shortlog';
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
                        time: {
                            useUTC:   false,
                            timezone: Intl.DateTimeFormat().resolvedOptions().timeZone
                        },
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
                        case 'direction':
                            chart = renderDirectionChart(container, responseData, cfg);
                            break;
                        case 'frequency':
                            chart = renderFrequencyChart(container, responseData.result || [], cfg);
                            break;
                        case 'month':
                        case 'year':
                            chart = renderLongChart(container, responseData.result || [], cfg, chartType);
                            break;
                        default:
                            chart = renderShortlogChart(container, responseData.result || [], cfg);
                            break;
                    }

                    setupResizeObserver(container);
                }

                // ----------------------------------------------------------------
                // Chart: shortlog — speed + gust splines over 24h
                // ----------------------------------------------------------------

                function renderShortlogChart(container, data, cfg) {
                    var speedSeries = [];
                    var gustSeries  = [];

                    data.forEach(function(d) {
                        var ts = new Date(d.d).getTime();
                        var sp = parseFloat(d.sp);
                        var gu = parseFloat(d.gu);
                        if (!isNaN(sp)) { speedSeries.push([ts, sp]); }
                        if (!isNaN(gu)) { gustSeries.push([ts, gu]); }
                    });

                    var textColor = getThemeColor('--dz-body-text', '#ccc');
                    var opts = baseChartOptions(container);
                    opts.chart.type   = 'spline';
                    opts.chart.margin = [10, 10, 30, 40];
                    opts.title.text   = titleForChartType(cfg, 'shortlog');
                    opts.legend       = { enabled: true, itemStyle: { fontSize: '10px', color: textColor } };
                    opts.xAxis = {
                        type:   'datetime',
                        labels: { style: { fontSize: '10px' } },
                        min:    cfg.chartType === 'week' ? Date.now() - 7 * 24 * 3600 * 1000 : Date.now() - 24 * 3600 * 1000,
                        max:    Date.now()
                    };
                    opts.yAxis = {
                        title:  { text: 'm/s', style: { fontSize: '10px' } },
                        labels: { style: { fontSize: '10px' } },
                        min:    0
                    };
                    opts.tooltip = {
                        shared:      true,
                        xDateFormat: '%a %d %b %H:%M',
                        formatter: function() {
                            var chartTime = this.points[0].series.chart.time;
                            var s = '<b>' + chartTime.dateFormat('%a %d %b %H:%M', this.x) + '</b>';
                            this.points.forEach(function(pt) {
                                var bf = beaufortFromMs(pt.y);
                                s += '<br><span style="color:' + pt.color + '">\u25CF</span> ' +
                                     pt.series.name + ': <b>' + pt.y.toFixed(1) + ' m/s</b>' +
                                     ' (Bft ' + bf + ' \u2014 ' + BEAUFORT_LABELS[bf] + ')';
                            });
                            return s;
                        }
                    };
                    opts.series = [
                        {
                            name:  'Speed',
                            data:  speedSeries,
                            color: 'rgba(3,190,252,0.9)'
                        },
                        {
                            name:       'Gust',
                            data:       gustSeries,
                            color:      'rgba(255,127,39,0.9)',
                            dashStyle:  'ShortDash'
                        }
                    ];

                    return window.Highcharts.chart(ctrl.chartId, opts);
                }

                // ----------------------------------------------------------------
                // Chart: direction — polar column (wind rose)
                // API endpoint: sensor=winddir, returns result_speed[]
                // ----------------------------------------------------------------

                function renderDirectionChart(container, responseData, cfg) {
                    var textColor = getThemeColor('--dz-body-text', '#ccc');
                    var opts = baseChartOptions(container);
                    opts.chart.polar  = true;
                    opts.chart.type   = 'column';
                    opts.chart.margin = [10, 10, 30, 10];
                    opts.title.text   = titleForChartType(cfg, 'direction');
                    opts.legend       = {
                        enabled:   true,
                        align:     'right',
                        verticalAlign: 'top',
                        y:         80,
                        layout:    'vertical',
                        itemStyle: { fontSize: '10px', color: textColor }
                    };
                    opts.pane = { size: '85%' };
                    opts.xAxis = {
                        tickmarkPlacement: 'on',
                        tickWidth:    1,
                        tickPosition: 'outside',
                        tickLength:   5,
                        tickColor:    '#999',
                        tickInterval: 1,
                        categories:   WIND_DIRECTIONS,
                        labels:       { style: { fontSize: '10px' } }
                    };
                    opts.yAxis = {
                        min:             0,
                        showLastLabel:   true,
                        reversedStacks:  false,
                        title:           { text: 'Frequency (%)', style: { fontSize: '10px' } },
                        labels: {
                            style:    { fontSize: '10px' },
                            formatter: function() { return this.value + '%'; }
                        }
                    };
                    opts.tooltip = {
                        formatter: function() {
                            return this.x + ': <b>' + this.y + ' %</b>';
                        }
                    };
                    opts.plotOptions = {
                        series: {
                            stacking:      'normal',
                            shadow:        false,
                            groupPadding:  0,
                            pointPlacement:'on'
                        }
                    };

                    var seriesData = [];
                    if (responseData.result_speed) {
                        var PALETTE = [
                            'rgba(3,190,252,0.85)',
                            'rgba(255,127,39,0.85)',
                            'rgba(39,174,96,0.85)',
                            'rgba(155,89,182,0.85)',
                            'rgba(231,76,60,0.85)',
                            'rgba(241,196,15,0.85)'
                        ];
                        responseData.result_speed.forEach(function(item, i) {
                            var pts = [];
                            for (var j = 0; j < 16; j++) {
                                pts.push(parseFloat(item.sp[j]));
                            }
                            seriesData.push({
                                name:  item.label,
                                data:  pts,
                                color: PALETTE[i % PALETTE.length]
                            });
                        });
                    }
                    opts.series = seriesData;

                    return window.Highcharts.chart(ctrl.chartId, opts);
                }

                // ----------------------------------------------------------------
                // Chart: frequency — histogram of wind speed occurrences + Weibull
                // ----------------------------------------------------------------

                function renderFrequencyChart(container, data, cfg) {
                    var speeds = data.map(function(d) {
                        return parseFloat(d.sp);
                    }).filter(function(v) { return !isNaN(v); });

                    if (speeds.length === 0) {
                        return renderEmpty(container, cfg, 'frequency');
                    }

                    var maxSpeed = Math.max.apply(null, speeds);
                    var numBins  = Math.max(10, Math.min(30, Math.round(Math.sqrt(speeds.length))));
                    var binWidth = maxSpeed > 0 ? maxSpeed / numBins : 1;
                    var magnitude = Math.pow(10, Math.floor(Math.log10(binWidth)));
                    binWidth = Math.ceil(binWidth / magnitude) * magnitude;
                    if (binWidth === 0) { binWidth = 1; }
                    numBins = Math.ceil(maxSpeed / binWidth) + 1;

                    var bins = new Array(numBins).fill(0);
                    speeds.forEach(function(speed) {
                        var bin = Math.floor(speed / binWidth);
                        if (bin >= numBins) { bin = numBins - 1; }
                        bins[bin]++;
                    });

                    var categories = [];
                    var histData   = [];
                    for (var i = 0; i < numBins; i++) {
                        categories.push(Math.round(i * binWidth * 10) / 10);
                        histData.push(Math.round(bins[i] / speeds.length * 10000) / 100);
                    }

                    // Weibull fit via method-of-moments
                    var mean = speeds.reduce(function(a, b) { return a + b; }, 0) / speeds.length;
                    var variance = speeds.reduce(function(a, b) { return a + (b - mean) * (b - mean); }, 0) / speeds.length;
                    var stddev = Math.sqrt(variance);
                    var cv = stddev / mean;
                    var k  = Math.pow(cv, -1.086);
                    var c  = mean / weibullGamma(1 + 1 / k);

                    var weibullData = [];
                    for (var j = 0; j < numBins; j++) {
                        var x   = (j + 0.5) * binWidth;
                        var pdf = (k / c) * Math.pow(x / c, k - 1) * Math.exp(-Math.pow(x / c, k));
                        weibullData.push(Math.round(pdf * binWidth * 10000) / 100);
                    }

                    var textColor = getThemeColor('--dz-body-text', '#ccc');
                    var opts = baseChartOptions(container);
                    opts.chart.type   = 'column';
                    opts.chart.margin = [10, 10, 40, 40];
                    opts.title.text   = titleForChartType(cfg, 'frequency');
                    opts.legend       = {
                        enabled:   true,
                        align:     'right',
                        verticalAlign: 'top',
                        y:         40,
                        layout:    'vertical',
                        itemStyle: { fontSize: '10px', color: textColor }
                    };
                    opts.xAxis = {
                        categories: categories,
                        title:      { text: 'Wind Speed (m/s)', style: { fontSize: '10px' } },
                        crosshair:  true,
                        labels:     { style: { fontSize: '10px' } }
                    };
                    opts.yAxis = {
                        min:    0,
                        title:  { text: 'Frequency (%)', style: { fontSize: '10px' } },
                        labels: { style: { fontSize: '10px' } }
                    };
                    opts.tooltip  = { shared: true };
                    opts.plotOptions = {
                        column: {
                            groupPadding: 0,
                            pointPadding: 0,
                            borderWidth:  1
                        }
                    };
                    opts.series = [
                        {
                            type:  'column',
                            name:  'Histogram',
                            data:  histData,
                            color: 'rgba(3,190,252,0.8)'
                        },
                        {
                            type:       'spline',
                            name:       'Weibull',
                            data:       weibullData,
                            color:      'rgba(255,80,80,0.9)',
                            lineWidth:  2,
                            marker:     { enabled: false },
                            tooltip:    { valueSuffix: ' %' }
                        }
                    ];

                    return window.Highcharts.chart(ctrl.chartId, opts);
                }

                // ----------------------------------------------------------------
                // Chart: month / year — daily/monthly avg speed bar chart
                // ----------------------------------------------------------------

                function renderLongChart(container, data, cfg, chartType) {
                    var speedSeries = [];
                    var gustSeries  = [];

                    data.forEach(function(d) {
                        var ts = new Date(d.d).getTime();
                        var sp = parseFloat(d.sp);
                        var gu = parseFloat(d.gu);
                        if (!isNaN(sp)) { speedSeries.push([ts, sp]); }
                        if (!isNaN(gu)) { gustSeries.push([ts, gu]); }
                    });

                    var textColor = getThemeColor('--dz-body-text', '#ccc');
                    var opts = baseChartOptions(container);
                    opts.chart.type   = 'column';
                    opts.chart.margin = [10, 10, 30, 40];
                    opts.title.text   = titleForChartType(cfg, chartType);
                    opts.legend       = { enabled: true, itemStyle: { fontSize: '10px', color: textColor } };
                    opts.xAxis = {
                        type:   'datetime',
                        labels: { style: { fontSize: '10px' } }
                    };
                    opts.yAxis = {
                        title:  { text: 'm/s', style: { fontSize: '10px' } },
                        labels: { style: { fontSize: '10px' } },
                        min:    0
                    };
                    opts.tooltip = {
                        shared:        true,
                        valueSuffix:   ' m/s',
                        valueDecimals: 1,
                        xDateFormat:   chartType === 'year' ? '%b %Y' : '%a %d %b'
                    };
                    opts.plotOptions = {
                        column: { grouping: true, borderWidth: 0, groupPadding: 0.1 }
                    };
                    opts.series = [
                        {
                            name:  'Speed',
                            data:  speedSeries,
                            color: 'rgba(3,190,252,0.85)'
                        },
                        {
                            name:  'Gust',
                            data:  gustSeries,
                            color: 'rgba(255,127,39,0.85)'
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
                // Weibull gamma function (Lanczos approximation)
                // ----------------------------------------------------------------

                function weibullGamma(z) {
                    if (z < 0.5) {
                        return Math.PI / (Math.sin(Math.PI * z) * weibullGamma(1 - z));
                    }
                    z -= 1;
                    var g    = 7;
                    var coef = [
                        0.99999999999980993, 676.5203681218851, -1259.1392167224028,
                        771.32342877765313, -176.61502916214059, 12.507343278686905,
                        -0.13857109526572012, 9.9843695780195716e-6, 1.5056327351493116e-7
                    ];
                    var x = coef[0];
                    for (var i = 1; i < g + 2; i++) { x += coef[i] / (z + i); }
                    var t = z + g + 0.5;
                    return Math.sqrt(2 * Math.PI) * Math.pow(t, z + 0.5) * Math.exp(-t) * x;
                }

                // ----------------------------------------------------------------
                // Reactivity / lifecycle
                // ----------------------------------------------------------------

                var refreshDebounce = null;
                $scope.$on('device_update', function(e, updated) {
                    var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                    if (!cfg || String(updated.idx) !== String(cfg.deviceIdx)) { return; }
                    if (!refreshDebounce) {
                        refreshDebounce = $timeout(function() {
                            refreshDebounce = null;
                            load();
                        }, 60000);
                    }
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
