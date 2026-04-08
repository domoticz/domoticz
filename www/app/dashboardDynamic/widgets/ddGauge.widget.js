define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    var RADIUS        = 40;
    var CIRCUMFERENCE = 2 * Math.PI * RADIUS;   // ~251.33
    var ARC_FRACTION  = 220 / 360;               // ~0.6111
    var ARC_LENGTH    = CIRCUMFERENCE * ARC_FRACTION; // ~153.59

    widgetRegistry.register({
        type:        'gauge',
        label:       'Gauge',
        description: 'Circular arc gauge showing the current value of any numeric device',
        category:    'Charts & Data',
        icon:        'fa-solid fa-gauge-high',
        defaultW:    2,
        defaultH:    3,
        minW:        2,
        minH:        3,
        maxW:        4,
        maxH:        4,
        transparentBackground: true,
        configSchema: [
            {
                key:          'deviceIdx',
                type:         'device-picker',
                label:        'Device',
                required:     true,
                deviceFilter: 'numeric'
            },
            {
                key:      'title',
                type:     'text',
                label:    'Title (optional, falls back to device name)',
                required: false
            },
            {
                key:     'min',
                type:    'number',
                label:   'Minimum value',
                default: 0
            },
            {
                key:     'max',
                type:    'number',
                label:   'Maximum value',
                default: 100
            },
            {
                key:     'unit',
                type:    'text',
                label:   'Unit suffix',
                default: '%'
            },
            {
                key:     'thresholdWarn',
                type:    'number',
                label:   'Warning threshold',
                default: 50
            },
            {
                key:     'thresholdCrit',
                type:    'number',
                label:   'Critical threshold',
                default: 80
            },
            {
                key:     'thresholdMode',
                type:    'select',
                label:   'Threshold mode',
                default: 'low-is-good',
                options: [
                    { value: 'low-is-good',  label: 'Low is good (e.g. CPU load)' },
                    { value: 'high-is-good', label: 'High is good (e.g. battery %)' }
                ]
            },
            { key: 'showBackground', type: 'boolean', label: 'Show panel background', default: true }
        ]
    });

    app.directive('ddGaugeWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/gauge.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', '$q', function($scope, $http, $interval, $q) {
                var ctrl      = this;
                ctrl.title    = '';
                ctrl.value    = null;
                var cancelToken = null;

                ctrl.unitStr = function() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    return (cfg.unit !== undefined && cfg.unit !== null) ? cfg.unit : '%';
                };

                ctrl.valueStr = function() {
                    if (ctrl.value === null) { return '--'; }
                    return String(Math.abs(ctrl.value) > 1000 ? Math.round(ctrl.value) : ctrl.value);
                };

                ctrl.gaugeColor = function() {
                    if (ctrl.value === null) { return 'var(--dz-widget-stat-muted)'; }
                    var cfg       = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var warn      = parseFloat(cfg.thresholdWarn);
                    var crit      = parseFloat(cfg.thresholdCrit);
                    var mode      = cfg.thresholdMode || 'low-is-good';
                    if (isNaN(warn)) { warn = 50; }
                    if (isNaN(crit)) { crit = 80; }

                    var v = ctrl.value;

                    if (mode === 'high-is-good') {
                        if (v >= crit) { return 'var(--dz-widget-energy-export)'; }
                        if (v >= warn) { return 'var(--dz-widget-amber)'; }
                        return 'var(--dz-accent-red)';
                    } else {
                        // low-is-good
                        if (v < warn)  { return 'var(--dz-widget-energy-export)'; }
                        if (v < crit)  { return 'var(--dz-widget-amber)'; }
                        return 'var(--dz-accent-red)';
                    }
                };

                ctrl.strokeDasharrayBg = function() {
                    return ARC_LENGTH + ' ' + CIRCUMFERENCE;
                };

                ctrl.strokeDasharray = function() {
                    if (ctrl.value === null) { return '0 ' + CIRCUMFERENCE; }
                    var cfg  = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var min  = parseFloat(cfg.min);
                    var max  = parseFloat(cfg.max);
                    if (isNaN(min)) { min = 0; }
                    if (isNaN(max)) { max = 100; }
                    var pct  = Math.min(1, Math.max(0, (ctrl.value - min) / (max - min)));
                    var fill = pct * ARC_LENGTH;
                    return fill + ' ' + CIRCUMFERENCE;
                };

                function applyDevice(d) {
                    var cfg   = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    ctrl.title = cfg.title || d.Name || '';
                    var raw   = (d.SubType === 'kWh' && d.Usage) ? d.Usage : (d.Data || '');
                    var match = raw.match(/^([-\d.]+)/);
                    ctrl.value = match ? parseFloat(match[1]) : null;
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
                        applyDevice(d);
                    }).catch(function(err) {
                        if (err.status === -1) { return; }
                        ctrl.loadError = true;
                    });
                }

                $scope.$on('device_update', function(e, updated) {
                    var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                    if (cfg && String(updated.idx) === String(cfg.deviceIdx)) {
                        applyDevice(updated);
                    }
                });

                var timer = $interval(load, 30000);

                $scope.$on('$destroy', function() {
                    if (cancelToken) { cancelToken.resolve(); cancelToken = null; }
                    $interval.cancel(timer);
                });

                $scope.$on('dd:widget:refresh', load);

                $scope.$watch(
                    function() {
                        return ctrl.widgetDef && ctrl.widgetDef.config && ctrl.widgetDef.config.deviceIdx;
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
