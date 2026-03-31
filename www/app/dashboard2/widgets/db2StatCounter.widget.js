define([
    'app',
    'dashboard2/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'stat-counter',
        label:       'Stat Counter',
        description: 'Single large KPI number from any device',
        category:    'Charts & Data',
        icon:        'fa-solid fa-gauge',
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
                required: true
            },
            {
                key:      'label',
                type:     'text',
                label:    'Label',
                required: false
            }
        ]
    });

    app.directive('db2StatCounterWidget', ['$q', function($q) {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboard2/widgets/stat-counter.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', '$q', function($scope, $http, $interval, $q) {
                var ctrl = this;
                ctrl.label = '';
                ctrl.value = '\u2014';
                ctrl.unit  = '';
                var cancelToken = null;

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

                        // Parse "23.5 °C" or "1234 Wh" style Data string
                        var match = (d.Data || '').match(/^([\d.\-]+)\s*(.*)?$/);
                        ctrl.value = match ? match[1] : (d.Data || '\u2014');
                        ctrl.unit  = match ? (match[2] || '') : '';
                        ctrl.label = cfg.label || d.Name || '';
                    }).catch(function(err) {
                        if (err.status === -1) { return; }
                        ctrl.error = 'Failed to load data';
                        ctrl.loading = false;
                    });
                }

                var timer = $interval(load, 30000);

                $scope.$on('$destroy', function() {
                    if (cancelToken) { cancelToken.resolve(); cancelToken = null; }
                    $interval.cancel(timer);
                });
                $scope.$on('db2:widget:refresh', load);

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
