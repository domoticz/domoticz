define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:                  'gas-summary',
        transparentBackground: true,
        label:       'Gas Summary',
        description: 'Compact stat card showing today\'s gas usage and total counter',
        category:    'Energy',
        icon:        'fa-solid fa-fire',
        defaultW:    2,
        defaultH:    2,
        minW:        2,
        minH:        2,
        maxW:        4,
        maxH:        3,
        configSchema: [
            {
                key:      'deviceIdx',
                type:     'device-picker',
                label:    'Device',
                deviceFilter: 'gas',
                required: true
            },
            {
                key:      'title',
                type:     'text',
                label:    'Title (optional, falls back to device name)',
                required: false
            }
        ]
    });

    app.directive('ddGasSummaryWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/gas-summary.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', '$q', function($scope, $http, $interval, $q) {
                var ctrl = this;
                ctrl.title        = '';
                ctrl.counterToday = null;
                ctrl.counterTotal = null;
                var cancelToken   = null;

                function applyDevice(d) {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    ctrl.title = cfg.title || d.Name || '';

                    // CounterToday: "0.995 m3" — keep full string for display
                    ctrl.counterToday = d.CounterToday || null;

                    // Counter: total counter string
                    ctrl.counterTotal = d.Counter || null;
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

                var timer = $interval(load, 60000);

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
