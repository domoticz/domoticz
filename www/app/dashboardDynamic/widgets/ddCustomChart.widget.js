define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    function detectSensor(device) {
        var type = device.Type || '';
        var sub  = device.SubType || '';
        var stv  = device.SwitchTypeVal;

        if (type === 'Wind')  { return { sensor: 'wind',    field: function(d){ return d.sp; },  unit: device.WindSign || 'm/s' }; }
        if (type === 'Rain')  { return { sensor: 'rain',    field: function(d){ return d.mm; },  unit: 'mm' }; }
        if (type === 'UV')    { return { sensor: 'uv',      field: function(d){ return d.uvi; }, unit: 'UVI' }; }
        if (type === 'Lux' || sub === 'Lux') { return { sensor: 'lux', field: function(d){ return d.lux; }, unit: 'lux' }; }
        if (type === 'Humidity') { return { sensor: 'temp', field: function(d){ return d.hu; },  unit: '%' }; }
        if (type.indexOf('Temp') >= 0 || sub.indexOf('Temp') >= 0) {
            return { sensor: 'temp', field: function(d){ return d.te !== undefined ? d.te : d.v; }, unit: '°' };
        }
        if (sub === 'Percentage') {
            // Day range returns `v`; month/year aggregates return v_min/v_max/v_avg.
            return {
                sensor: 'Percentage',
                field: function(d) { return d.v !== undefined ? d.v : d.v_avg; },
                unit: '%'
            };
        }
        if (type === 'General' && sub === 'Voltage') {
            return { sensor: 'counter', field: function(d){ return d.v }, unit: 'V' };
        }
        if (type === 'General' && sub === 'Current') {
            return { sensor: 'counter', field: function(d){ return d.v }, unit: 'A' };
        }
        if (type.indexOf('Meter') >= 0 || type === 'Cube Electric' ||
            (type === 'General' && (sub === 'kWh' || sub === 'Counter Incremental' || sub === 'Managed Counter'))) {
            if (stv === 1) { return { sensor: 'counter', field: function(d){ return d.v; }, unit: 'm³' }; }
            if (stv === 2) { return { sensor: 'counter', field: function(d){ return d.v; }, unit: 'L' }; }
            return { sensor: 'counter', field: function(d){ return d.v !== undefined ? d.v : d.v1; }, unit: 'kWh' };
        }
        if (type === 'P1 Smart Meter') {
            return { sensor: 'counter', field: function(d){ return d.v !== undefined ? d.v : d.v1; }, unit: 'kWh' };
        }
        // Usage devices (instantaneous Watts — P1 phase power, smart plugs, etc.)
        // The graph endpoint returns the wattage in field `u` for these devices.
        if (type === 'Usage') {
            return { sensor: 'counter', field: function(d){ return d.u !== undefined ? d.u : d.v; }, unit: 'W' };
        }
        if (sub === 'SetPoint' || type === 'Thermostat') {
            return { sensor: 'temp', field: function(d){ return d.se !== undefined ? d.se : d.te; }, unit: '°' };
        }
        return { sensor: 'temp', field: function(d){ return d.te !== undefined ? d.te : d.v; }, unit: '?' };
    }

    function buildYAxisIndex(sensorInfos) {
        var unitToAxis = {};
        var axisCount  = 0;
        sensorInfos.forEach(function(info) {
            if (unitToAxis[info.unit] === undefined) {
                unitToAxis[info.unit] = axisCount++;
            }
        });
        return unitToAxis;
    }

    var DEVICE_KEYS = ['device1','device2','device3','device4','device5',
                       'device6','device7','device8','device9','device10'];

    widgetRegistry.register({
        type:                  'custom-chart',
        transparentBackground: true,
        label:       'Custom Chart',
        description: 'Free-form chart combining multiple sensors',
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
                key:     'range',
                type:    'select',
                label:   'Time range',
                options: [
                    { value: 'day',   label: 'Last 24h' },
                    { value: 'week',  label: 'Last 7 days' },
                    { value: 'month', label: 'Last month' },
                    { value: 'year',  label: 'Last year' }
                ],
                default: 'day'
            },
            {
                key:      'title',
                type:     'text',
                label:    'Title',
                required: false
            },
            {
                key:     'showLegend',
                type:    'boolean',
                label:   'Show legend',
                default: true
            },
            { key: 'device1',  type: 'device-picker', label: 'Device 1',  required: false },
            { key: 'device2',  type: 'device-picker', label: 'Device 2',  required: false },
            { key: 'device3',  type: 'device-picker', label: 'Device 3',  required: false },
            { key: 'device4',  type: 'device-picker', label: 'Device 4',  required: false },
            { key: 'device5',  type: 'device-picker', label: 'Device 5',  required: false },
            { key: 'device6',  type: 'device-picker', label: 'Device 6',  required: false },
            { key: 'device7',  type: 'device-picker', label: 'Device 7',  required: false },
            { key: 'device8',  type: 'device-picker', label: 'Device 8',  required: false },
            { key: 'device9',  type: 'device-picker', label: 'Device 9',  required: false },
            { key: 'device10', type: 'device-picker', label: 'Device 10', required: false }
        ]
    });

    var chartIdCounter = 0;

    app.directive('ddCustomChartWidget', ['$http', '$timeout', '$q', function($http, $timeout, $q) {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/custom-chart.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$timeout', '$q', function($scope, $http, $timeout, $q) {
                var ctrl = this;
                ctrl.chartId = 'dd-custom-chart-' + (++chartIdCounter);
                ctrl.isEmpty = false;
                ctrl.error   = null;

                var chart              = null;
                var infoTokens         = [];
                var graphTokens        = [];
                var resizeObserver     = null;

                // ----------------------------------------------------------------
                // Helpers
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

                function getThemeColor(varName, fallback) {
                    var val = getComputedStyle(document.documentElement).getPropertyValue(varName).trim();
                    return val || fallback;
                }

                function cancelAll() {
                    infoTokens.forEach(function(t)  { t.resolve(); });
                    graphTokens.forEach(function(t) { t.resolve(); });
                    infoTokens  = [];
                    graphTokens = [];
                }

                function destroyChart() {
                    if (chart) { chart.destroy(); chart = null; }
                }

                // ----------------------------------------------------------------
                // Data loading
                // ----------------------------------------------------------------

                function load() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};

                    ctrl.isEmpty = false;
                    ctrl.error   = null;

                    var idxList = DEVICE_KEYS
                        .map(function(k) { return cfg[k]; })
                        .filter(function(v) { return !!v; });

                    if (idxList.length === 0) {
                        ctrl.isEmpty = true;
                        destroyChart();
                        return;
                    }

                    cancelAll();

                    var range = cfg.range || 'day';

                    // Fetch device info for all selected devices in parallel.
                    var infoRequests = idxList.map(function(idx) {
                        var token = $q.defer();
                        infoTokens.push(token);
                        var url = 'json.htm?type=command&param=getdevices&rid=' + encodeURIComponent(idx);
                        return $http.get(url, { timeout: token.promise })
                            .then(function(resp) {
                                var result = resp.data && resp.data.result;
                                return (result && result[0]) ? result[0] : null;
                            })
                            .catch(function(err) {
                                if (err.status === -1) { return undefined; }
                                return null;
                            });
                    });

                    $q.all(infoRequests).then(function(devices) {
                        // undefined means cancelled — abort entirely.
                        if (devices.some(function(d) { return d === undefined; })) { return; }

                        var sensorInfos = devices.map(function(device) {
                            return device ? detectSensor(device) : { sensor: 'temp', field: function(d){ return d.te; }, unit: '?' };
                        });

                        var unitToAxis = buildYAxisIndex(sensorInfos);

                        // Fetch graph data for each device in parallel.
                        var graphRequests = idxList.map(function(idx, i) {
                            var token = $q.defer();
                            graphTokens.push(token);
                            var sensor = sensorInfos[i].sensor;
                            var url = 'json.htm?type=command&param=graph&sensor=' + encodeURIComponent(sensor) +
                                      '&idx=' + encodeURIComponent(idx) +
                                      '&range=' + encodeURIComponent(range);
                            return $http.get(url, { timeout: token.promise })
                                .then(function(resp) {
                                    return {
                                        device:     devices[i],
                                        sensorInfo: sensorInfos[i],
                                        data:       resp.data.result || []
                                    };
                                })
                                .catch(function(err) {
                                    if (err.status === -1) { return undefined; }
                                    return { device: devices[i], sensorInfo: sensorInfos[i], data: [] };
                                });
                        });

                        $q.all(graphRequests).then(function(results) {
                            if (results.some(function(r) { return r === undefined; })) { return; }
                            $timeout(function() { renderChart(results, unitToAxis, cfg); }, 0);
                        });
                    });
                }

                // ----------------------------------------------------------------
                // Chart rendering
                // ----------------------------------------------------------------

                function renderChart(results, unitToAxis, cfg) {
                    var container = document.getElementById(ctrl.chartId);
                    if (!container || !window.Highcharts) { return; }

                    destroyChart();

                    var textColor = getThemeColor('--dz-body-text', '#ccc');

                    var axisCount = Object.keys(unitToAxis).length;
                    var yAxes = [];
                    // Build unit→axis label map (unit string becomes the axis title).
                    var unitList = [];
                    Object.keys(unitToAxis).forEach(function(unit) {
                        unitList[unitToAxis[unit]] = unit;
                    });
                    // Axis 0 → left side; axes 1+ → right side, each offset by 55px
                    for (var ai = 0; ai < axisCount; ai++) {
                        var isOpposite = ai > 0;
                        var axisOffset = ai > 1 ? (ai - 1) * 55 : 0;
                        var axisEntry = {
                            title:    { text: unitList[ai], style: { fontSize: '10px', color: textColor } },
                            labels:   { style: { fontSize: '10px', color: textColor } },
                            opposite: isOpposite || undefined
                        };
                        if (axisOffset) { axisEntry.offset = axisOffset; }
                        yAxes.push(axisEntry);
                    }

                    var hcSeries = results.map(function(result, i) {
                        var info      = result.sensorInfo;
                        var fieldFn   = info.field;
                        var axisIndex = unitToAxis[info.unit];
                        var name      = (result.device && result.device.Name) ? result.device.Name : ('Series ' + (i + 1));

                        var data = result.data
                            .map(function(d) {
                                // Parse "YYYY-MM-DD[ HH:MM[:SS]]" as LOCAL time —
                                // ISO strings without a TZ suffix are interpreted as UTC
                                // by many engines, which shifts the chart by the local offset.
                                var parts = (d.d || '').split(' ');
                                var ymd   = parts[0].split('-');
                                var hms   = (parts[1] || '0:0:0').split(':');
                                var ts    = new Date(+ymd[0], +ymd[1] - 1, +ymd[2],
                                                     +hms[0], +hms[1] || 0, +hms[2] || 0).getTime();
                                var val   = parseFloat(fieldFn(d));
                                return [ts, val];
                            })
                            .filter(function(pt) {
                                return isFinite(pt[0]) && !isNaN(pt[1]);
                            });

                        return {
                            name:  name,
                            data:  data,
                            yAxis: axisIndex,
                            type:  'spline'
                        };
                    });

                    var showLegend = cfg.showLegend !== false;

                    var opts = {
                        time: {
                            useUTC:   false,
                            timezone: Intl.DateTimeFormat().resolvedOptions().timeZone
                        },
                        chart: {
                            animation:       false,
                            backgroundColor: 'transparent',
                            marginTop:       10,
                            style:           { fontFamily: 'inherit' },
                            height:          container.offsetHeight || 200,
                            width:           container.offsetWidth  || null,
                            zoomType: 'x'
                        },
                        title: {
                            text:  cfg.title || null,
                            align: 'center',
                            style: { fontSize: '11px', fontWeight: '600', color: textColor }
                        },
                        legend: {
                            enabled:   showLegend,
                            itemStyle: { fontSize: '10px', color: textColor }
                        },
                        credits:   { enabled: false },
                        exporting: { enabled: false },
                        xAxis: {
                            type:      'datetime',
                            crosshair: true,
                            labels:    { style: { fontSize: '10px', color: textColor } },
                            min: (function() {
                                var now = Date.now();
                                if (cfg.range === 'day')   { return now - 24 * 3600 * 1000; }
                                if (cfg.range === 'week')  { return now -  7 * 24 * 3600 * 1000; }
                                if (cfg.range === 'month') { return now - 30 * 24 * 3600 * 1000; }
                                return undefined;
                            }()),
                            max: Date.now()
                        },
                        yAxis:   yAxes,
                        tooltip: {
                            shared:    true,
                            formatter: function() {
                                var x = this.x;
                                // Use the chart's own time object so per-chart useUTC/timezone
                                // is honored (Highcharts.dateFormat is global and ignores it).
                                var chartTime = this.points[0].series.chart.time;
                                var s = '<b>' + chartTime.dateFormat('%a %d %b %H:%M', x) + '</b>';
                                var pointMap = {};
                                this.points.forEach(function(p) { pointMap[p.series.name] = p; });
                                this.points[0].series.chart.series.forEach(function(series) {
                                    var p = pointMap[series.name];
                                    var val;
                                    if (p !== undefined && p.y !== null) {
                                        val = p.y;
                                    } else {
                                        // Find last known value at or before x
                                        var pts = series.data;
                                        var last = null;
                                        for (var i = pts.length - 1; i >= 0; i--) {
                                            if (pts[i].x <= x && pts[i].y !== null) {
                                                last = pts[i].y;
                                                break;
                                            }
                                        }
                                        val = last;
                                    }
                                    var unit = unitList[series.options.yAxis] || '';
                                    s += '<br/><span style="color:' + series.color + '">\u25CF</span> ' +
                                         series.name + ': ';
                                    s += (val !== null && val !== undefined)
                                        ? ('<b>' + window.Highcharts.numberFormat(val, 1) + (unit ? ' ' + unit : '') + '</b>')
                                        : '<i>—</i>';
                                });
                                return s;
                            }
                        },
                        series: hcSeries
                    };

                    chart = window.Highcharts.chart(ctrl.chartId, opts);
                    setupResizeObserver(container);
                }

                // ----------------------------------------------------------------
                // Reactivity / lifecycle
                // ----------------------------------------------------------------

                $scope.$on('$destroy', function() {
                    if (resizeObserver) { resizeObserver.disconnect(); resizeObserver = null; }
                    cancelAll();
                    destroyChart();
                });

                $scope.$on('dd:widget:refresh', load);

                $scope.$watch(
                    function() {
                        var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                        if (!cfg) { return ''; }
                        var devicePart = DEVICE_KEYS.map(function(k) { return cfg[k] || ''; }).join(',');
                        return devicePart + '|' +
                               (cfg.range || 'day') + '|' +
                               (cfg.title || '') + '|' +
                               String(cfg.showLegend !== false);
                    },
                    function(val, old) {
                        if (val === old) { return; }
                        var cfg      = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                        var oldParts = old.split('|');
                        var newParts = val.split('|');

                        // If only display options changed (same devices + range), update chart in-place.
                        if (chart && oldParts[0] === newParts[0] && oldParts[1] === newParts[1]) {
                            var showLegend = cfg.showLegend !== false;
                            chart.setTitle({ text: cfg.title || null });
                            chart.legend.update({ enabled: showLegend });
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
