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

    function detectDeviceInfo(device) {
        if (!device) {
            return { unit: 'kWh', divider: 1000, isP1: false, hasReturn: false };
        }

        var switchTypeVal = device.SwitchTypeVal;
        var isP1 = (device.Type === 'P1 Smart Meter' && device.SubType === 'Energy');

        if (isP1) {
            var hasReturn = device.CounterDeliv !== undefined && device.CounterDeliv !== null
                && parseFloat(device.CounterDeliv) > 0;
            return { unit: 'kWh', divider: 1000, isP1: true, hasReturn: hasReturn };
        }

        switch (switchTypeVal) {
            case SWITCH_TYPE_GAS:
                return { unit: 'm\u00b3', divider: 1, isP1: false, hasReturn: false };
            case SWITCH_TYPE_WATER:
                return { unit: 'L', divider: 0.001, isP1: false, hasReturn: false };
            case SWITCH_TYPE_ENERGY_GENERATED:
                return { unit: 'kWh', divider: 1000, isP1: false, hasReturn: false };
            case SWITCH_TYPE_ENERGY_USED:
                return { unit: 'kWh', divider: 1000, isP1: false, hasReturn: false };
            default:
                // Generic counter incremental / unknown
                if (device.SubType === 'kWh' || device.Type === 'kWh') {
                    return { unit: 'kWh', divider: 1000, isP1: false, hasReturn: false };
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
        day:     { sensor: 'counter', range: 'day' },
        week:    { sensor: 'counter', range: 'week' },
        month:   { sensor: 'counter', range: 'month' },
        year:    { sensor: 'counter', range: 'year' },
        compare: { sensor: 'counter', range: 'compare', groupby: 'month' }
    };

    var CHART_TYPE_LABELS = {
        day:     'Today (hourly)',
        week:    'Last 7 days',
        month:   'Last month',
        year:    'Last year',
        compare: 'Compare years'
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
                    { value: 'day',     label: 'Today (hourly)' },
                    { value: 'week',    label: 'Last 7 days' },
                    { value: 'month',   label: 'Last month' },
                    { value: 'year',    label: 'Last year' },
                    { value: 'compare', label: 'Compare years' }
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
                    var url = 'json.htm?type=command&param=graph&sensor=' + api.sensor +
                              '&idx=' + cfg.deviceIdx + '&range=' + api.range;
                    if (api.groupby) {
                        url += '&groupby=' + api.groupby;
                    }

                    $http.get(url, { timeout: cancelToken.promise })
                        .then(function(resp) {
                            var data      = resp.data.result || [];
                            var firstYear = resp.data.firstYear || null;
                            $timeout(function() {
                                renderChart(data, cfg, chartType, firstYear);
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

                // ----------------------------------------------------------------
                // Render dispatch
                // ----------------------------------------------------------------

                function renderChart(data, cfg, chartType, firstYear) {
                    var container = document.getElementById(ctrl.chartId);
                    if (!container || !window.Highcharts) { return; }

                    destroyChart();

                    var info = deviceInfo || { unit: 'kWh', divider: 1000, isP1: false, hasReturn: false };

                    if (chartType === 'compare') {
                        chart = renderCompare(container, data, cfg, info, firstYear);
                    } else if (info.isP1 && info.hasReturn) {
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
                        var ts  = new Date(d.d).getTime();
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
                        name:  titleForChartType(cfg, chartType) || info.unit,
                        data:  series,
                        color: getThemeColor('--dz-btn-primary-bg', '#337ab7')
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
                        var ts   = new Date(d.d).getTime();
                        var imp  = (parseFloat(d.v1 || 0) + parseFloat(d.v2 || 0)) / 1000;
                        var ret  = (parseFloat(d.r1 || 0) + parseFloat(d.r2 || 0)) / 1000;
                        if (!isNaN(imp)) { importSeries.push([ts, imp]); }
                        if (!isNaN(ret) && ret > 0) { returnSeries.push([ts, ret]); }
                    });

                    var xFmt = (chartType === 'day') ? '%a %d %b %H:%M' : '%a %d %b %Y';
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
                        title:  { text: 'kWh', style: { fontSize: '10px' } },
                        labels: { style: { fontSize: '10px' } }
                    };
                    opts.tooltip = {
                        shared:        true,
                        valueSuffix:   ' kWh',
                        valueDecimals: 3,
                        xDateFormat:   xFmt
                    };
                    opts.series = [
                        {
                            name:  'Import',
                            data:  importSeries,
                            color: getThemeColor('--dz-accent-danger', '#e74c3c')
                        },
                        {
                            name:  'Return',
                            data:  returnSeries,
                            color: getThemeColor('--dz-accent-success', '#27ae60')
                        }
                    ];

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
                    opts.chart.margin = [10, 10, 40, 40];
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
                        return cfg.deviceIdx + '|' + resolveChartType(cfg) + '|' + (cfg.title || '');
                    },
                    function(val, old) {
                        if (val === old) { return; }

                        var cfg      = ctrl.widgetDef && ctrl.widgetDef.config;
                        var oldParts = old.split('|');
                        var newParts = val.split('|');

                        // Only the title changed — update chart title in-place
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
