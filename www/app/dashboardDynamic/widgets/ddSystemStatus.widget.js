define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'system-status',
        label:       'System Status',
        description: 'Domoticz server version and device counts',
        category:    'System',
        icon:        'fa-solid fa-server',
        defaultW:    3,
        defaultH:    2,
        minW:        2,
        minH:        2,
        maxW:        6,
        maxH:        4,
        configSchema: []
    });

    app.directive('ddSystemStatusWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/system-status.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', function($scope, $http, $interval) {
                var ctrl = this;
                ctrl.version       = null;
                ctrl.hardwareCount = null;
                ctrl.deviceCount   = null;

                function load() {
                    $http.get('json.htm?type=command&param=getversion')
                        .then(function(resp) {
                            var d = resp.data;
                            ctrl.version = d && (d.version || d.build_time || null);
                        })
                        .catch(function(err) {
                            if (err.status === -1) { return; }
                            console.error('SystemStatus: failed to load version', err);
                        });

                    $http.get('json.htm?type=command&param=gethardware')
                        .then(function(resp) {
                            var hw = (resp.data && resp.data.result) || [];
                            ctrl.hardwareCount = hw.filter(function(h) {
                                return h.Enabled === 'true';
                            }).length;
                        })
                        .catch(function(err) {
                            if (err.status === -1) { return; }
                            console.error('SystemStatus: failed to load hardware', err);
                        });

                    $http.get('json.htm', {
                        params: { type: 'command', param: 'getdevices', filter: 'all', used: 'true', order: 'Name' }
                    }).then(function(resp) {
                        var result = resp.data && resp.data.result;
                        ctrl.deviceCount = result ? result.length : 0;
                    }).catch(function(err) {
                        if (err.status === -1) { return; }
                        console.error('SystemStatus: failed to load devices', err);
                    });
                }

                // Refresh every 5 minutes
                var timer = $interval(load, 300000);

                $scope.$on('$destroy', function() { $interval.cancel(timer); });
                $scope.$on('dd:widget:refresh', load);

                ctrl.$onInit = load;
            }]
        };
    }]);
});
