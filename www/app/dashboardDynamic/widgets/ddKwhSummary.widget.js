define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:                  'kwh-summary',
        transparentBackground: true,
        label:       'kWh Summary',
        description: 'Compact stat card showing current power (W) and today\'s energy total (kWh)',
        category:    'Energy',
        icon:        'fa-solid fa-solar-panel',
        defaultW:    2,
        defaultH:    2,
        minW:        2,
        minH:        2,
        maxW:        4,
        maxH:        3,
        configSchema: [
            {
                key:          'deviceIdx',
                type:         'device-picker',
                label:        'Device',
                deviceFilter: 'kwh',
                required:     true
            },
            {
                key:      'title',
                type:     'text',
                label:    'Title (optional, falls back to device name)',
                required: false
            },
            {
                key:     'colorToken',
                type:    'select',
                label:   'Color',
                default: 'solar',
                options: [
                    { value: 'solar',  label: 'Solar (yellow)' },
                    { value: 'import', label: 'Import (amber)' },
                    { value: 'export', label: 'Export (green)' },
                    { value: 'gas',    label: 'Gas (orange-red)' }
                ]
            }
        ]
    });

    app.directive('ddKwhSummaryWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/kwh-summary.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', '$q', function($scope, $http, $interval, $q) {
                var ctrl = this;
                ctrl.title        = '';
                ctrl.usageWatt    = null;
                ctrl.counterToday = null;
                var cancelToken   = null;

                function applyDevice(d) {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    ctrl.title = cfg.title || d.Name || '';

                    // Usage: "1234 Watt" or "1234 W" — extract numeric part
                    var wMatch = (d.Usage || '').match(/^([\d.]+)/);
                    ctrl.usageWatt = wMatch ? wMatch[1] : null;

                    // CounterToday: "3.456 kWh" — keep full string for display
                    ctrl.counterToday = d.CounterToday || null;
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

                ctrl.colorClass = function() {
                    var cfg  = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var token = cfg.colorToken || 'solar';
                    return 'dd-energy-' + token;
                };

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
