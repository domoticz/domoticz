define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
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
                key:      'deviceIdx',
                type:     'device-picker',
                label:    'Device',
                required: true
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
            { key: 'showBackground', type: 'boolean', label: 'Show panel background', default: true }
        ]
    });

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
            controller: ['$scope', '$http', '$q', function($scope, $http, $q) {
                var ctrl = this;
                ctrl.label             = '';
                ctrl.value             = '—';
                ctrl.unit              = '';
                ctrl.numVal            = NaN;
                ctrl.counterTotal      = null;
                ctrl.valueDeliv        = null;
                ctrl.unitDeliv         = '';
                ctrl.counterDelivTotal = null;
                var cancelToken = null;

                function formatTotal(str, fallbackUnit) {
                    if (str === null || str === undefined || str === '') return null;
                    var m = String(str).trim().match(/^([\d.\-]+)\s*(.*)?$/);
                    if (!m) return String(str) || null;
                    var num  = String(parseFloat(m[1]));
                    var unit = m[2] || fallbackUnit;
                    return unit ? num + ' ' + unit : num;
                }

                function applyDevice(d, labelOverride) {
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
                    $(document).off('dz:setpoint:saved', onSetpointSaved);
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
