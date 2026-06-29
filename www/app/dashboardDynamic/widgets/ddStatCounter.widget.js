define([
    'app',
    'dashboardDynamic/widgetRegistry.service',
    'dashboardDynamic/ddSparkline.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:                  'stat-counter',
        label:                 'Stat Counter',
        description:           'Single large KPI number from any device',
        category:              'Charts & Data',
        icon:                  'fa-solid fa-gauge',
        transparentBackground: true,
        defaultW:    2,
        defaultH:    2,
        minW:        2,
        minH:        1,
        maxW:        4,
        maxH:        3,
        configSchema: [
            {
                key:       'deviceIdx',
                type:      'device-picker',
                label:     'Device',
                required:  true,
                metricKey: 'metric'
            },
            {
                key:      'label',
                type:     'text',
                label:    'Label',
                required: false
            },
            {
                key:          'ranges',
                type:         'range-list',
                label:        'Bar ranges',
                help:         'Add value ranges to show a gradient bar. Bar auto-scales to the combined min/max of all ranges.',
                seedDefaults: [
                    { from: 0,  to: 50,  color: '#66bb6a' },
                    { from: 50, to: 100, color: '#DF2D3A' }
                ]
            },
            { key: 'showGraph', type: 'boolean', label: 'Show trend graph (last 24h)', default: false },
            { key: 'showBackground', type: 'boolean', label: 'Show panel background', default: true }
        ]
    });

    // Multi-value temp-family metric → device field (current value) and graph field.
    var METRIC_FIELDS = {
        te: { deviceField: 'Temp',      graph: function(x){ return x.te !== undefined ? x.te : x.v; } },
        hu: { deviceField: 'Humidity',  graph: function(x){ return x.hu; } },
        ba: { deviceField: 'Barometer', graph: function(x){ return x.ba; } },
        se: { deviceField: 'SetPoint',  graph: function(x){ return x.se !== undefined ? x.se : x.te; } }
    };

    function metricUnit(metric) {
        if (metric === 'te' || metric === 'se') { return '°' + (($.myglobals && $.myglobals.tempsign) || 'C'); }
        if (metric === 'hu') { return '%'; }
        if (metric === 'ba') { return 'hPa'; }
        return '';
    }

    // Map a device to the graph sensor + value field for its trend sparkline.
    // An explicit metric (te/hu/ba/se) overrides the auto-detection.
    function detectGraph(d, metric) {
        if (metric && METRIC_FIELDS[metric]) {
            return { sensor: 'temp', field: METRIC_FIELDS[metric].graph };
        }
        var type = d.Type || '', sub = d.SubType || '';
        if (type.indexOf('Temp') >= 0 || sub.indexOf('Temp') >= 0) {
            return { sensor: 'temp', field: function(x){ return x.te !== undefined ? x.te : x.v; } };
        }
        if (type === 'Humidity')                 { return { sensor: 'temp',       field: function(x){ return x.hu; } }; }
        if (type === 'Rain')                     { return { sensor: 'rain',       field: function(x){ return x.mm; } }; }
        if (type === 'Wind')                     { return { sensor: 'wind',       field: function(x){ return x.sp; } }; }
        if (type === 'UV')                       { return { sensor: 'uv',         field: function(x){ return x.uvi; } }; }
        if (sub === 'Lux' || type === 'Lux')     { return { sensor: 'lux',        field: function(x){ return x.lux; } }; }
        if (sub === 'Percentage')                { return { sensor: 'Percentage', field: function(x){ return x.v !== undefined ? x.v : x.v_avg; } }; }
        if (type === 'Usage')                    { return { sensor: 'counter',    field: function(x){ return x.u !== undefined ? x.u : x.v; } }; }
        // Default: counters (kWh, gas, water, general counters)
        return { sensor: 'counter', field: function(x){ return x.v !== undefined ? x.v : x.v1; } };
    }

    app.directive('ddStatCounterWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/stat-counter.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$q', '$timeout', 'ddSparkline',
                function($scope, $http, $q, $timeout, ddSparkline) {
                var ctrl = this;
                ctrl.label             = '';
                ctrl.value             = '—';
                ctrl.unit              = '';
                ctrl.numVal            = NaN;
                ctrl.counterTotal      = null;
                ctrl.valueDeliv        = null;
                ctrl.unitDeliv         = '';
                ctrl.counterDelivTotal = null;
                ctrl.showGraph         = false;
                ctrl.sparkId           = 'dd-statc-spark-' + $scope.$id;
                var cancelToken   = null;
                var sparkToken    = null;
                var sparkChart    = null;
                var renderTimeout = null;

                function formatTotal(str, fallbackUnit) {
                    if (str === null || str === undefined || str === '') return null;
                    var m = String(str).trim().match(/^([\d.\-]+)\s*(.*)?$/);
                    if (!m) return String(str) || null;
                    var num  = String(parseFloat(m[1]));
                    var unit = m[2] || fallbackUnit;
                    return unit ? num + ' ' + unit : num;
                }

                function applyMetric(d, metric, labelOverride) {
                    var raw = d[METRIC_FIELDS[metric].deviceField];
                    var num = parseFloat(raw);
                    ctrl.value             = isNaN(num) ? (raw !== undefined ? String(raw) : '—') : String(num);
                    ctrl.unit              = metricUnit(metric);
                    ctrl.numVal            = isNaN(num) ? NaN : num;
                    ctrl.label             = labelOverride || d.Name || '';
                    ctrl.counterTotal      = null;
                    ctrl.valueDeliv        = null;
                    ctrl.unitDeliv         = '';
                    ctrl.counterDelivTotal = null;
                }

                function applyDevice(d, labelOverride) {
                    var metric = ((ctrl.widgetDef && ctrl.widgetDef.config) || {}).metric;
                    if (metric && METRIC_FIELDS[metric] && d[METRIC_FIELDS[metric].deviceField] !== undefined) {
                        applyMetric(d, metric, labelOverride);
                        return;
                    }

                    var fallbackUnit = d.vunit || d.ValueUnits || '';
                    var hasToday     = !!d.CounterToday;
                    var primary      = hasToday ? d.CounterToday : (d.Counter || d.Data || '—');

                    var match   = (primary || '').match(/^([\d.\-]+)\s*(.*)?$/);
                    var unit    = match ? (match[2] || fallbackUnit) : fallbackUnit;
                    ctrl.value  = match ? String(parseFloat(match[1])) : (primary || '—');
                    ctrl.unit   = unit;
                    ctrl.label  = labelOverride || d.Name || '';
                    ctrl.numVal = match ? parseFloat(match[1]) : NaN;

                    ctrl.counterTotal = hasToday ? formatTotal(d.Counter || d.Data, unit) : null;

                    var hasDeliv = typeof d.CounterDeliv !== 'undefined' && d.CounterDeliv != 0;
                    if (hasDeliv) {
                        var dm          = (d.CounterDelivToday || '').match(/^([\d.\-]+)\s*(.*)?$/);
                        var delivUnit   = dm ? (dm[2] || unit) : unit;
                        ctrl.valueDeliv        = dm ? String(parseFloat(dm[1])) : (d.CounterDelivToday || '—');
                        ctrl.unitDeliv         = delivUnit;
                        ctrl.counterDelivTotal = formatTotal(d.CounterDeliv, delivUnit);
                    } else {
                        ctrl.valueDeliv        = null;
                        ctrl.unitDeliv         = '';
                        ctrl.counterDelivTotal = null;
                    }
                }

                function load() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    if (!cfg.deviceIdx) { return; }

                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();

                    $http.get('json.htm', {
                        params:  { type: 'command', param: 'getdevices', rid: cfg.deviceIdx },
                        timeout: cancelToken.promise
                    }).then(function(resp) {
                        var d = resp.data.result && resp.data.result[0];
                        if (!d) { return; }
                        applyDevice(d, cfg.label);
                        loadSparkline(d);
                    }).catch(function(err) {
                        if (err.status === -1) { return; }
                    });
                }

                function destroySpark() {
                    if (sparkChart && sparkChart.destroy) { sparkChart.destroy(); }
                    sparkChart = null;
                }

                function loadSparkline(d) {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    ctrl.showGraph = cfg.showGraph === true;
                    if (!ctrl.showGraph || !d || !window.Highcharts) { destroySpark(); return; }

                    var g = detectGraph(d, cfg.metric);
                    if (sparkToken) { sparkToken.resolve(); }
                    sparkToken = $q.defer();
                    $http.get('json.htm', {
                        params:  { type: 'command', param: 'graph', sensor: g.sensor, idx: cfg.deviceIdx, range: 'day' },
                        timeout: sparkToken.promise
                    }).then(function(resp) {
                        var rows = (resp.data && resp.data.result) || [];
                        var data = [];
                        rows.forEach(function(r) {
                            var v = parseFloat(g.field(r));
                            if (isNaN(v)) { return; }
                            data.push([ddSparkline.parseLocal(r.d), v]);
                        });
                        if (renderTimeout) { $timeout.cancel(renderTimeout); }
                        renderTimeout = $timeout(function() {
                            renderTimeout = null;
                            destroySpark();
                            // Re-check: the option may have been toggled off while fetching
                            if (((ctrl.widgetDef && ctrl.widgetDef.config) || {}).showGraph === true) {
                                sparkChart = ddSparkline.render(ctrl.sparkId, data);
                            }
                        }, 0);
                    }).catch(function(err) {
                        if (err.status === -1) { return; }
                    });
                }

                $scope.$on('device_update', function(e, updated) {
                    var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                    if (cfg && String(updated.idx) === String(cfg.deviceIdx)) {
                        applyDevice(updated, cfg.label);
                    }
                });

                function onSetpointSaved(e, data) {
                    var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                    if (cfg && String(data.idx) === String(cfg.deviceIdx)) {
                        $scope.$applyAsync(function() {
                            ctrl.value  = String(data.value);
                            ctrl.numVal = data.value;
                        });
                    }
                }
                $(document).on('dz:setpoint:saved', onSetpointSaved);

                $scope.$on('$destroy', function() {
                    if (cancelToken) { cancelToken.resolve(); cancelToken = null; }
                    if (sparkToken)  { sparkToken.resolve();  sparkToken = null; }
                    if (renderTimeout) { $timeout.cancel(renderTimeout); renderTimeout = null; }
                    destroySpark();
                    $(document).off('dz:setpoint:saved', onSetpointSaved);
                });
                $scope.$on('dd:widget:refresh', load);

                $scope.$watch(
                    function() {
                        var c = ctrl.widgetDef && ctrl.widgetDef.config;
                        return c ? (c.deviceIdx + '|' + (c.showGraph === true) + '|' + (c.metric || '') + '|' + (c.label || '')) : '';
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
