define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:                  'p1-electricity',
        transparentBackground: true,
        label:       'P1 Electricity',
        description: 'Dutch P1 smart meter widget showing current import/export power and today\'s totals',
        category:    'Energy',
        icon:        'fa-solid fa-plug',
        defaultW:    3,
        defaultH:    2,
        minW:        2,
        minH:        2,
        maxW:        6,
        maxH:        4,
        configSchema: [
            {
                key:          'deviceIdx',
                type:         'device-picker',
                label:        'Device',
                deviceFilter: 'p1',
                required:     true
            },
            {
                key:      'title',
                type:     'text',
                label:    'Title (optional, falls back to device name)',
                required: false
            },
            {
                key:     'showPrice',
                type:    'boolean',
                label:   'Show price',
                default: true
            },
            {
                key:          'ranges',
                type:         'range-list',
                label:        'Bar ranges',
                help:         'Add value ranges to show a gradient bar on current power (W). Bar auto-scales to the combined min/max of all ranges.',
                seedDefaults: [
                    { from: -5000, to: -2500, color: '#DF2D3A' },
                    { from: -2500, to: -500,  color: '#ffa726' },
                    { from: -500,  to: 500,   color: '#42a5f5' },
                    { from: 500,   to: 2500,  color: '#ffa726' },
                    { from: 2500,  to: 5000,  color: '#DF2D3A' }
                ]
            },
            { key: 'showBackground', type: 'boolean', label: 'Show panel background', default: true }
        ]
    });

    app.directive('ddP1ElectricityWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/p1-electricity.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$q', function($scope, $http, $q) {
                var ctrl = this;
                ctrl.title              = '';
                ctrl.importWatt         = null;
                ctrl.exportWatt         = null;
                ctrl.counterToday       = null;
                ctrl.counterDelivToday  = null;
                ctrl.price              = null;
                ctrl.isExporting        = false;
                ctrl.numVal             = NaN;
                ctrl.loadError          = false;
                var cancelToken         = null;

                function parseWatt(str) {
                    var m = (str || '').match(/^([\d.]+)/);
                    return m ? parseFloat(m[1]) : 0;
                }

                function applyDevice(d) {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    ctrl.title = cfg.title || d.Name || '';

                    ctrl.importWatt        = parseWatt(d.Usage);
                    ctrl.exportWatt        = parseWatt(d.UsageDeliv);
                    ctrl.counterToday      = d.CounterToday      || null;
                    ctrl.counterDelivToday = d.CounterDelivToday || null;
                    // d.price (lowercase) is today's grid cost; 1000 = sentinel meaning "not configured"
                    var raw = parseFloat(d.price);
                    ctrl.price = (!isNaN(raw) && raw !== 1000 && raw !== 0) ? raw : null;
                    ctrl.isExporting       = ctrl.exportWatt > 0;
                    ctrl.numVal            = ctrl.isExporting ? -ctrl.exportWatt : ctrl.importWatt;
                    ctrl.loadError         = false;
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

                $scope.$on('$destroy', function() {
                    if (cancelToken) { cancelToken.resolve(); cancelToken = null; }
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
