define([
    'app',
    'dashboardDynamic/widgetRegistry.service',
    'dashboardDynamic/ddVisibility.service'
], function(app, widgetRegistry, ddVisibility) {
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
        transparentBackground: true,
        configSchema: [
            { key: 'showBackground', type: 'boolean', label: 'Show panel background', default: true }
        ]
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
            controller: ['$scope', '$http', '$interval', '$q', 'ddVisibility', function($scope, $http, $interval, $q, ddVisibility) {
                var ctrl = this;
                ctrl.version       = null;
                ctrl.hardwareCount = null;
                ctrl.deviceCount   = null;
                var cancelToken = null;
                var timer       = null;

                function load() {
                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();
                    var thisToken = cancelToken;
                    var t = thisToken.promise;

                    $http.get('json.htm?type=command&param=getversion', { timeout: t })
                        .then(function(resp) {
                            if (thisToken !== cancelToken) { return; }
                            var d = resp.data;
                            ctrl.version = d && (d.version || d.build_time || null);
                        })
                        .catch(function(err) {
                            if (err.status === -1) { return; }
                            console.error('SystemStatus: failed to load version', err);
                        });

                    $http.get('json.htm?type=command&param=gethardware', { timeout: t })
                        .then(function(resp) {
                            if (thisToken !== cancelToken) { return; }
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
                        params: { type: 'command', param: 'getdevices', filter: 'all', used: 'true', order: 'Name' },
                        timeout: t
                    }).then(function(resp) {
                        if (thisToken !== cancelToken) { return; }
                        var result = resp.data && resp.data.result;
                        ctrl.deviceCount = result ? result.length : 0;
                    }).catch(function(err) {
                        if (err.status === -1) { return; }
                        console.error('SystemStatus: failed to load devices', err);
                    });
                }

                function stopTimer() {
                    if (timer) { $interval.cancel(timer); timer = null; }
                }

                function startTimer() {
                    stopTimer();
                    // Refresh every 5 minutes
                    timer = $interval(load, 300000);
                }

                $scope.$on('dd:page:hidden',  function() { stopTimer(); });
                $scope.$on('dd:page:visible', function() { load(); startTimer(); });

                $scope.$on('$destroy', function() {
                    if (cancelToken) { cancelToken.resolve(); }
                    stopTimer();
                });
                $scope.$on('dd:widget:refresh', load);

                ctrl.$onInit = function() {
                    load();
                    if (!ddVisibility.isHidden()) { startTimer(); }
                };
            }]
        };
    }]);
});
