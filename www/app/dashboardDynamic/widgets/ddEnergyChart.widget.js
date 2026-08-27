define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    // ----------------------------------------------------------------
    // Device-type detection helpers
    // ----------------------------------------------------------------

    // SwitchTypeVal constants (mirrors chart.deviceTypes used in CounterLog)
    var SWITCH_TYPE_ENERGY_USED      = 0;
    var SWITCH_TYPE_GAS              = 1;
    var SWITCH_TYPE_WATER            = 2;
    var SWITCH_TYPE_ENERGY_GENERATED = 4;

    function parseDateLocal(str) {
        var s = str.replace('T', ' ');
        var dp = s.split(' ');
        var ymd = dp[0].split('-');
        if (dp.length === 1) {
            return new Date(+ymd[0], +ymd[1] - 1, +ymd[2]).getTime();
        }
        var hms = dp[1].split(':');
        return new Date(+ymd[0], +ymd[1] - 1, +ymd[2], +hms[0], +hms[1], +(hms[2] || 0)).getTime();
    }

    function detectDeviceInfo(device) {
        if (!device) {
            return { unit: 'kWh', divider: 1, isP1: false, hasReturn: false };
        }

        var switchTypeVal = device.SwitchTypeVal;
        var isP1 = (device.Type === 'P1 Smart Meter' && device.SubType === 'Energy');

        if (isP1) {
            var hasReturn = device.CounterDeliv !== undefined && device.CounterDeliv !== null
                && parseFloat(device.CounterDeliv) > 0;
            return { unit: 'kWh', divider: 1, isP1: true, hasReturn: hasReturn };
        }

        switch (switchTypeVal) {
            case SWITCH_TYPE_GAS:
                return { unit: 'm\u00b3', divider: 1, isP1: false, hasReturn: false };
            case SWITCH_TYPE_WATER:
                // API returns m³ (raw / backend-divider); widget converts to Litres (÷ 0.001 = × 1000)
                return { unit: 'L', divider: 0.001, isP1: false, hasReturn: false };
            case SWITCH_TYPE_ENERGY_GENERATED:
                // API already applies the energy divider (Wh → kWh), so no further division needed
                return { unit: 'kWh', divider: 1, isP1: false, hasReturn: false };
            case SWITCH_TYPE_ENERGY_USED:
                return { unit: 'kWh', divider: 1, isP1: false, hasReturn: false };
            default:
                // Generic counter incremental / unknown
                if (device.SubType === 'kWh' || device.Type === 'kWh') {
                    return { unit: 'kWh', divider: 1, isP1: false, hasReturn: false };
                }
                return { unit: device.Unit || '', divider: 1, isP1: false, hasReturn: false };
        }
    }

    // ----------------------------------------------------------------
    // Chart-type configuration
    // ----------------------------------------------------------------

    // Map legacy `range` values (saved in old widgets) to new chartType keys.
    var RANGE_TO_CHART_TYPE = {
        day:   'day',
        week:  'week',
        month: 'month',
        year:  'year'
    };

    // API parameters for each chartType
    var CHART_TYPE_API = {
        day:      { sensor: 'counter', range: 'day' },
        shortlog: { sensor: 'counter', range: 'day' },
        week:     { sensor: 'counter', range: 'week' },
        month:    { sensor: 'counter', range: 'month' },
        year:     { sensor: 'counter', range: 'year' },
        compare:  { sensor: 'counter', range: 'compare', groupby: 'month' }
    };

    var CHART_TYPE_LABELS = {
        day:      'Today (hourly)',
        shortlog: 'Short Log',
        week:     'Last 7 days',
        month:    'Last month',
        year:     'Last year',
        compare:  'Compare years'
    };

    var MONTH_NAMES = ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];

    widgetRegistry.register({
        type:        'energy-chart',
        label:       'Counter / Energy Chart',
        description: 'kWh, Gas, Water, P1 and counter bar charts',
        category:    'Charts & Data',
        icon:        'fa-solid fa-bolt',
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
                deviceFilter: 'counter'
            },
            {
                key:     'chartType',
                type:    'select',
                label:   'Chart type',
                options: [
                    { value: 'shortlog', label: 'Short Log' },
                    { value: 'day',      label: 'Today (hourly)' },
                    { value: 'week',     label: 'Last 7 days' },
                    { value: 'month',    label: 'Last month' },
                    { value: 'year',     label: 'Last year' },
                    { value: 'compare',  label: 'Compare years' }
                ],
                default: 'shortlog'
            },
            {
                key:      'title',
                type:     'text',
                label:    'Custom title',
                required: false
            },
            {
                key:     'barColor',
                type:    'color',
                label:   'Bar color',
                help:    'Applies to single-series counter charts (gas, water, kWh non-P1).',
                default: '#03befc'
            },
            { key: 'showBackground', type: 'boolean', label: 'Show panel background', default: true }
        ]
    });

    var chartIdCounter = 0;

    app.directive('ddEnergyChartWidget', ['$http', '$timeout', '$q', function($http, $timeout, $q) {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/energy-chart.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$timeout', '$q', function($scope, $http, $timeout, $q) {
                var ctrl = this;
                ctrl.chartId    = 'dd-energy-chart-' + (++chartIdCounter);
                ctrl.deviceName = '';
                ctrl.noData     = false;
                var chart        = null;
                var cancelToken  = null;
                var lastDeviceIdx = null;
                var deviceInfo   = null;
                var resizeObserver = null;

                // ----------------------------------------------------------------
                // Resize observer
                // ----------------------------------------------------------------

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

                // ----------------------------------------------------------------
                // Theme helper
                // ----------------------------------------------------------------

                function getThemeColor(varName, fallback) {
                    var val = getComputedStyle(document.documentElement).getPropertyValue(varName).trim();
                    return val || fallback;
                }

                // ----------------------------------------------------------------
                // Resolve chartType (handles legacy `range` field)
                // ----------------------------------------------------------------

                function resolveChartType(cfg) {
                    if (cfg.chartType && CHART_TYPE_API[cfg.chartType]) {
                        return cfg.chartType;
                    }
                    if (cfg.range && RANGE_TO_CHART_TYPE[cfg.range]) {
                        return RANGE_TO_CHART_TYPE[cfg.range];
                    }
                    return 'day';
                }

                // ----------------------------------------------------------------
                // Load
                // ----------------------------------------------------------------

                function load() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    if (!cfg.deviceIdx) { return; }

                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();

                    var deviceChanged = cfg.deviceIdx !== lastDeviceIdx;
                    if (deviceChanged) {
                        lastDeviceIdx = cfg.deviceIdx;
                        deviceInfo    = null;

                        $http.get('json.htm?type=command&param=getdevices&rid=' + cfg.deviceIdx)
                            .then(function(resp) {
                                var d = resp.data && resp.data.result && resp.data.result[0];
                                ctrl.deviceName = cfg.title || (d && d.Name) || '';
                                deviceInfo = detectDeviceInfo(d);
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
                    var info = deviceInfo || { isP1: false };
                    var url;

                    if (info.isP1 && chartType === 'day') {
                        url = 'json.htm?type=command&param=graph&sensor=counter&idx=' + cfg.deviceIdx + '&range=hour&resolution=60';
                    } else if (info.isP1 && chartType === 'compare') {
                        url = 'json.htm?type=command&param=graph&sensor=counter&idx=' + cfg.deviceIdx + '&range=compare&groupby=month&sensorarea=usage';
                    } else {
                        url = 'json.htm?type=command&param=graph&sensor=' + api.sensor +
                              '&idx=' + cfg.deviceIdx + '&range=' + api.range;
                        if (api.groupby) {
                            url += '&groupby=' + api.groupby;
                        }
                    }

                    $http.get(url, { timeout: cancelToken.promise })
                        .then(function(resp) {
                            var data      = resp.data.result || [];
                            var firstYear = resp.data.firstYear || null;
                            var meta      = {
                                p1DisplayType: resp.data.P1DisplayType,
                                delivered:     resp.data.delivered === true
                            };
                            ctrl.noData   = (data.length === 0);
                            $timeout(function() {
                                renderChart(data, cfg, chartType, firstYear, meta);
                            }, 0);
                        })
                        .catch(function(err) {
                            if (err.status === -1) { return; }
                            ctrl.error = 'Failed to load data';
                        });
                }

                // ----------------------------------------------------------------
                // Shared chart helpers
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
                            margin:          [22, 10, 30, 65],
                            style:           { fontFamily: 'inherit' },
                            height:          h,
                            width:           container.offsetWidth || null,
                            zoomType:        'x'
                        },
                        title: {
                            text:  null,
                            align: 'center',
                            style: { fontSize: '11px', fontWeight: '600', color: getThemeColor('--dz-body-text', '#ccc') }
                        },
                        plotOptions: {
                            column: { borderWidth: 0, pointPadding: 0.1, groupPadding: 0, minPointLength: 2 },
                            // Don't fade non-hovered bars (Highcharts default inactive opacity is 0.2)
                            series: { states: { inactive: { opacity: 1 } } }
                        },
                        time: {
                            useUTC:   false,
                            timezone: Intl.DateTimeFormat().resolvedOptions().timeZone
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

                // ----------------------------------------------------------------
                // Render dispatch
                // ----------------------------------------------------------------

                function renderChart(data, cfg, chartType, firstYear, meta) {
                    var container = document.getElementById(ctrl.chartId);
                    if (!container || !window.Highcharts) { return; }

                    destroyChart();

                    var info = deviceInfo || { unit: 'kWh', divider: 1, isP1: false, hasReturn: false };
                    meta = meta || {};

                    if (chartType === 'compare') {
                        chart = renderCompare(container, data, cfg, info, firstYear);
                    } else if (chartType === 'shortlog') {
                        chart = info.isP1
                            ? renderP1ShortLog(container, data, cfg, info, meta)
                            : renderSimpleShortLog(container, data, cfg, info);
                    } else if (info.isP1 && chartType === 'day') {
                        chart = renderP1HourBars(container, data, cfg, info, chartType);
                    } else if (info.isP1) {
                        chart = renderP1Bars(container, data, cfg, info, chartType);
                    } else {
                        chart = renderSimpleBars(container, data, cfg, info, chartType);
                    }

                    setupResizeObserver(container);
                }

                // ----------------------------------------------------------------
                // Chart: simple column (day / week / month / year, single series)
                // ----------------------------------------------------------------

                function renderSimpleBars(container, data, cfg, info, chartType) {
                    var series = data.map(function(d) {
                        var ts  = parseDateLocal(d.d);
                        var raw = parseFloat(d.v !== undefined ? d.v : (d.v1 || 0));
                        // Divide by info.divider: kWh devices use 1000 (Wh→kWh),
                        // Water uses 0.001 (m³→L, i.e. multiply by 1000), Gas uses 1.
                        var val = raw / info.divider;
                        return [ts, isNaN(val) ? null : val];
                    }).filter(function(pt) { return pt[1] !== null; });

                    var xFmt = (chartType === 'day') ? '%a %d %b %H:%M' : '%a %d %b %Y';

                    var opts = baseChartOptions(container);
                    opts.chart.type = 'column';
                    opts.title.text = titleForChartType(cfg, chartType);
                    opts.xAxis = {
                        type:   'datetime',
                        labels: { style: { fontSize: '10px' } }
                    };
                    // chartType=day means "Today (hourly)" — same as the Domoticz log
                    // page: a rolling 24-hour window. Counter short-log returns ~5 days
                    // of data, so clip the x-axis to [now-24h .. now].
                    if (chartType === 'day') {
                        var nowMs = Date.now();
                        opts.xAxis.min = nowMs - 24 * 60 * 60 * 1000;
                        opts.xAxis.max = nowMs;
                    }
                    opts.yAxis = {
                        title:  { text: info.unit, style: { fontSize: '10px' } },
                        labels: { style: { fontSize: '10px' } }
                    };
                    opts.tooltip = {
                        valueSuffix:   ' ' + info.unit,
                        valueDecimals: info.unit === 'L' ? 0 : 3,
                        xDateFormat:   xFmt
                    };
                    opts.series = [{
                        // The chart title already carries the range ("Last month"); the
                        // tooltip names the series only, so a single day's value is
                        // not labelled with the whole range.
                        name:  ctrl.deviceName || info.unit,
                        data:  series,
                        // User-configurable; defaults to the same cyan as the P1 "Usage" series
                        color: cfg.barColor || '#03befc'
                    }];

                    return window.Highcharts.chart(ctrl.chartId, opts);
                }

                // ----------------------------------------------------------------
                // Chart: P1 dual series (import + export/return)
                // ----------------------------------------------------------------

                function renderP1Bars(container, data, cfg, info, chartType) {
                    var importSeries = [];
                    var returnSeries = [];

                    data.forEach(function(d) {
                        var ts  = parseDateLocal(d.d);
                        // v1/v2 from the backend are already in kWh (divided by the energy divider)
                        var imp = parseFloat(d.v1 || 0) + parseFloat(d.v2 || 0);
                        var ret = parseFloat(d.r1 || 0) + parseFloat(d.r2 || 0);
                        if (!isNaN(imp)) { importSeries.push([ts, imp]); }
                        if (info.hasReturn && !isNaN(ret) && ret > 0) { returnSeries.push([ts, ret]); }
                    });

                    var xFmt      = '%a %d %b %Y';
                    var textColor = getThemeColor('--dz-body-text', '#ccc');

                    var opts = baseChartOptions(container);
                    opts.chart.type = 'column';
                    opts.title.text = titleForChartType(cfg, chartType);
                    opts.xAxis = {
                        type:   'datetime',
                        labels: { style: { fontSize: '10px' } }
                    };
                    opts.yAxis = {
                        title:  { text: 'kWh', style: { fontSize: '10px' } },
                        labels: { style: { fontSize: '10px' } }
                    };

                    if (info.hasReturn) {
                        opts.chart.marginBottom = 75;
                        opts.legend = {
                            enabled:       true,
                            verticalAlign: 'bottom',
                            itemStyle:     { fontSize: '10px', color: textColor }
                        };
                        opts.tooltip = {
                            shared:        true,
                            valueSuffix:   ' kWh',
                            valueDecimals: 3,
                            xDateFormat:   xFmt
                        };
                        opts.series = [
                            {
                                name:  'Usage',
                                data:  importSeries,
                                color: 'rgba(3,190,252,0.8)'
                            },
                            {
                                name:  'Return',
                                data:  returnSeries,
                                color: 'rgba(3,252,190,0.8)'
                            }
                        ];
                    } else {
                        opts.tooltip = {
                            valueSuffix:   ' kWh',
                            valueDecimals: 3,
                            xDateFormat:   xFmt
                        };
                        opts.series = [{
                            name:  'Usage',
                            data:  importSeries,
                            color: 'rgba(3,190,252,0.8)'
                        }];
                    }

                    return window.Highcharts.chart(ctrl.chartId, opts);
                }

                // ----------------------------------------------------------------
                // Chart: P1 hourly bars (day / week) — reads v (usage Wh) + r (return Wh)
                // ----------------------------------------------------------------

                function renderP1HourBars(container, data, cfg, info, chartType) {
                    var importSeries = [];
                    var returnSeries = [];
                    var hasReturn    = info.hasReturn;

                    data.forEach(function(d) {
                        var ts  = parseDateLocal(d.d);
                        var imp = parseFloat(d.v || 0);
                        var ret = parseFloat(d.r || 0);
                        if (!isNaN(imp)) { importSeries.push([ts, imp]); }
                        if (hasReturn && !isNaN(ret) && ret > 0) { returnSeries.push([ts, ret]); }
                    });

                    var xFmt      = '%a %d %b %H:%M';
                    var textColor = getThemeColor('--dz-body-text', '#ccc');

                    var opts = baseChartOptions(container);
                    opts.chart.type = 'column';
                    opts.title.text = titleForChartType(cfg, chartType);
                    opts.xAxis = {
                        type:   'datetime',
                        labels: { style: { fontSize: '10px' } }
                    };
                    opts.yAxis = {
                        title:  { text: 'Wh', style: { fontSize: '10px' } },
                        labels: { style: { fontSize: '10px' } }
                    };

                    if (hasReturn) {
                        opts.chart.marginBottom = 75;
                        opts.legend = {
                            enabled:       true,
                            verticalAlign: 'bottom',
                            itemStyle:     { fontSize: '10px', color: textColor }
                        };
                        opts.tooltip = {
                            shared:        true,
                            valueSuffix:   ' Wh',
                            valueDecimals: 0,
                            xDateFormat:   xFmt
                        };
                        opts.series = [
                            {
                                name:  'Usage',
                                data:  importSeries,
                                color: 'rgba(3,190,252,0.8)'
                            },
                            {
                                name:  'Return',
                                data:  returnSeries,
                                color: 'rgba(3,252,190,0.8)'
                            }
                        ];
                    } else {
                        opts.tooltip = {
                            valueSuffix:   ' Wh',
                            valueDecimals: 0,
                            xDateFormat:   xFmt
                        };
                        opts.series = [{
                            name:  'Usage',
                            data:  importSeries,
                            color: 'rgba(3,190,252,0.8)'
                        }];
                    }

                    return window.Highcharts.chart(ctrl.chartId, opts);
                }

                // ----------------------------------------------------------------
                // Chart: P1 short log (range=day) — area, power in W, threshold=0
                // ----------------------------------------------------------------

                function renderP1ShortLog(container, data, cfg, info, meta) {
                    var p1DisplayType = (meta.p1DisplayType !== undefined) ? meta.p1DisplayType : 1;
                    var delivered     = meta.delivered === true;
                    var textColor     = getThemeColor('--dz-body-text', '#ccc');
                    var series        = [];

                    var opts = baseChartOptions(container);
                    opts.chart.type = 'area';
                    opts.title.text = titleForChartType(cfg, 'shortlog');
                    opts.xAxis = {
                        type:   'datetime',
                        labels: { style: { fontSize: '10px' } }
                    };
                    opts.yAxis = {
                        title:  { text: 'W', style: { fontSize: '10px' } },
                        labels: { style: { fontSize: '10px' } },
                        min:    0
                    };
                    opts.tooltip = {
                        shared:        true,
                        valueSuffix:   ' W',
                        valueDecimals: 0,
                        xDateFormat:   '%a %d %b %H:%M'
                    };
                    opts.plotOptions = {
                        area: { marker: { enabled: false }, threshold: 0 }
                    };

                    if (p1DisplayType === 1) {
                        // Dynamic mode: v = usage W (positive), r = return W (negative in data, show as positive)
                        var usageSeries  = [];
                        var returnSeries = [];
                        data.forEach(function(d) {
                            var ts = parseDateLocal(d.d);
                            var v  = parseFloat(d.v);
                            var r  = parseFloat(d.r);
                            usageSeries.push([ts, isNaN(v) ? null : Math.max(0, v)]);
                            if (delivered) {
                                returnSeries.push([ts, isNaN(r) ? null : Math.abs(r)]);
                            }
                        });
                        series.push({
                            name:  'Usage',
                            data:  usageSeries,
                            color: {
                                linearGradient: { x1: 0, y1: 0, x2: 0, y2: 1 },
                                stops: [
                                    [0, 'rgb(160,30,252,1)'],
                                    [1, 'rgb(3,190,252,0.8)']
                                ]
                            }
                        });
                        if (delivered) {
                            series.push({
                                name:  'Return',
                                data:  returnSeries,
                                color: {
                                    linearGradient: { x1: 0, y1: 0, x2: 0, y2: 1 },
                                    stops: [
                                        [0, 'rgb(3,152,90)'],
                                        [0.8, 'rgb(3,252,190)']
                                    ]
                                }
                            });
                        }
                    } else {
                        // Low/High tariff mode: v1+v2 stacked for usage, r1+r2 stacked for return
                        var u1 = [], u2 = [], r1 = [], r2 = [];
                        data.forEach(function(d) {
                            var ts = parseDateLocal(d.d);
                            u1.push([ts, parseFloat(d.v1) || null]);
                            u2.push([ts, parseFloat(d.v2) || null]);
                            if (delivered) {
                                r1.push([ts, Math.abs(parseFloat(d.r1)) || null]);
                                r2.push([ts, Math.abs(parseFloat(d.r2)) || null]);
                            }
                        });
                        opts.plotOptions.area.stacking = 'normal';
                        series.push({ name: 'Usage 1',  data: u1, color: 'rgba(60,130,252,0.8)',  fillOpacity: 0.5, stack: 'susage'  });
                        series.push({ name: 'Usage 2',  data: u2, color: 'rgba(3,190,252,0.8)',   fillOpacity: 0.5, stack: 'susage'  });
                        if (delivered) {
                            series.push({ name: 'Return 1', data: r1, color: 'rgba(30,242,110,0.8)', fillOpacity: 0.5, stack: 'sreturn' });
                            series.push({ name: 'Return 2', data: r2, color: 'rgba(3,252,190,0.8)',  fillOpacity: 0.5, stack: 'sreturn' });
                        }
                    }

                    opts.series = series;

                    if (series.length > 1) {
                        opts.chart.marginBottom = 65;
                        opts.legend = {
                            enabled:       true,
                            verticalAlign: 'bottom',
                            y:             10,
                            itemStyle:     { fontSize: '10px', color: textColor }
                        };
                    }

                    return window.Highcharts.chart(ctrl.chartId, opts);
                }

                // ----------------------------------------------------------------
                // Chart: non-P1 short log (range=day) — column, raw v values
                // ----------------------------------------------------------------

                function renderSimpleShortLog(container, data, cfg, info) {
                    // Shortlog `v` for kWh devices is in W (instant power) — no conversion.
                    // For all other types (water, gas, etc.) apply the divider just like simpleBars.
                    var isKwh = (info.unit === 'kWh');
                    var series = data.map(function(d) {
                        var ts  = parseDateLocal(d.d);
                        var raw = parseFloat(d.v);
                        var val = isKwh ? raw : raw / info.divider;
                        return [ts, isNaN(val) || val === 0 ? null : val];
                    }).filter(function(pt) { return pt[1] !== null; });

                    var unit = isKwh ? 'W' : info.unit;

                    // Compute "Last X Days" from data span
                    var chartTitle;
                    if (series.length >= 2) {
                        var spanMs = series[series.length - 1][0] - series[0][0];
                        var days   = Math.max(1, Math.round(spanMs / 86400000));
                        var suffix = days === 1 ? 'Last Day' : 'Last ' + days + ' Days';
                        chartTitle = cfg.title || (ctrl.deviceName ? ctrl.deviceName + ' \u2014 ' + suffix : suffix);
                    } else {
                        chartTitle = titleForChartType(cfg, 'shortlog');
                    }

                    var opts = baseChartOptions(container);
                    opts.chart.type = 'column';
                    opts.title.text = chartTitle;
                    opts.xAxis = {
                        type:   'datetime',
                        labels: { style: { fontSize: '10px' } }
                    };
                    opts.yAxis = {
                        title:  { text: unit, style: { fontSize: '10px' } },
                        labels: { style: { fontSize: '10px' } }
                    };
                    opts.tooltip = {
                        valueSuffix:   ' ' + unit,
                        valueDecimals: 0,
                        xDateFormat:   '%a %d %b %H:%M'
                    };
                    opts.series = [{
                        name:  ctrl.deviceName || unit,
                        data:  series,
                        color: getThemeColor('--dz-btn-primary-bg', '#337ab7')
                    }];

                    return window.Highcharts.chart(ctrl.chartId, opts);
                }

                // ----------------------------------------------------------------
                // Chart: compare years (grouped columns per month category)
                // API returns: [ { y: 2023, c: "01", s: 45.2 }, ... ]
                // firstYear: 2021
                // ----------------------------------------------------------------

                function renderCompare(container, data, cfg, info, firstYear) {
                    var currentYear = new Date().getFullYear();
                    var startYear   = firstYear ? parseInt(firstYear, 10) : currentYear;

                    // Build year range
                    var years = [];
                    for (var y = startYear; y <= currentYear; y++) { years.push(y); }

                    // Categories: month labels 01-12
                    var categories = MONTH_NAMES;

                    // Group data by year → array of 12 values (null if missing)
                    var seriesByYear = {};
                    years.forEach(function(yr) {
                        seriesByYear[yr] = new Array(12).fill(null);
                    });

                    data.forEach(function(d) {
                        var yr  = parseInt(d.y, 10);
                        var cat = parseInt(d.c, 10) - 1; // 0-based month index
                        var val = parseFloat(d.s);
                        if (!isNaN(yr) && cat >= 0 && cat < 12 && !isNaN(val)) {
                            if (!seriesByYear[yr]) { seriesByYear[yr] = new Array(12).fill(null); }
                            seriesByYear[yr][cat] = val;
                        }
                    });

                    // Color palette — cycle through accent colors
                    var PALETTE = [
                        getThemeColor('--dz-btn-primary-bg',  '#337ab7'),
                        getThemeColor('--dz-accent-danger',   '#e74c3c'),
                        getThemeColor('--dz-accent-success',  '#27ae60'),
                        getThemeColor('--dz-accent-warning',  '#f39c12'),
                        getThemeColor('--dz-accent-info',     '#3498db'),
                        getThemeColor('--dz-accent-primary',  '#2980b9')
                    ];

                    var seriesArr = years.map(function(yr, i) {
                        return {
                            name:  String(yr),
                            data:  seriesByYear[yr],
                            color: PALETTE[i % PALETTE.length]
                        };
                    });

                    var textColor = getThemeColor('--dz-body-text', '#ccc');

                    var opts = baseChartOptions(container);
                    opts.chart.type   = 'column';
                    opts.chart.margin = [10, 10, 40, 55];
                    opts.title.text   = titleForChartType(cfg, 'compare');
                    opts.legend       = {
                        enabled:   years.length > 1,
                        itemStyle: { fontSize: '10px', color: textColor }
                    };
                    opts.xAxis = {
                        categories: categories,
                        labels:     { style: { fontSize: '10px' } }
                    };
                    opts.yAxis = {
                        title:  { text: info.unit, style: { fontSize: '10px' } },
                        labels: { style: { fontSize: '10px' } }
                    };
                    opts.tooltip = {
                        shared:        true,
                        valueSuffix:   ' ' + info.unit,
                        valueDecimals: info.unit === 'L' ? 0 : 3
                    };
                    opts.plotOptions = {
                        column: { grouping: true, borderWidth: 0 }
                    };
                    opts.series = seriesArr;

                    return window.Highcharts.chart(ctrl.chartId, opts);
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
                    if (cancelToken) { cancelToken.resolve(); cancelToken = null; }
                    if (chart) { chart.destroy(); chart = null; }
                    if (refreshDebounce) { $timeout.cancel(refreshDebounce); }
                });

                $scope.$on('dd:widget:refresh', load);

                $scope.$watch(
                    function() {
                        var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                        if (!cfg) { return ''; }
                        return cfg.deviceIdx + '|' + resolveChartType(cfg) + '|' +
                               (cfg.title || '') + '|' + (cfg.barColor || '');
                    },
                    function(val, old) {
                        if (val === old) { return; }

                        var cfg      = ctrl.widgetDef && ctrl.widgetDef.config;
                        var oldParts = old.split('|');
                        var newParts = val.split('|');

                        // Only the title changed — update chart title in-place
                        if (chart && oldParts[0] === newParts[0] && oldParts[1] === newParts[1] &&
                            oldParts[3] === newParts[3]) {
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
