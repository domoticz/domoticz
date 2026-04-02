define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'p1-electricity',
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
            }
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
            controller: ['$scope', '$http', '$interval', '$q', function($scope, $http, $interval, $q) {
                var ctrl = this;
                ctrl.title              = '';
                ctrl.importWatt         = null;
                ctrl.exportWatt         = null;
                ctrl.counterToday       = null;
                ctrl.counterDelivToday  = null;
                ctrl.price              = null;
                ctrl.isExporting        = false;
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
                    ctrl.price             = (d.PriceToday !== undefined && d.PriceToday !== null)
                                                ? d.PriceToday
                                                : null;
                    ctrl.isExporting       = ctrl.exportWatt > 0;
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
