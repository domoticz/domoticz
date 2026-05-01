define([
    'app',
    'dashboardDynamic/widgetRegistry.service',
    'dashboardDynamic/ddVisibility.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'activity-log',
        label:       'Activity Log',
        description: 'Recent device state changes',
        category:    'System',
        icon:        'fa-solid fa-list',
        defaultW:    3,
        defaultH:    4,
        minW:        2,
        minH:        3,
        maxW:        8,
        maxH:        12,
        configSchema: [
            { key: 'maxItems', type: 'number', label: 'Max items', default: 15, min: 5, max: 50 }
        ]
    });

    app.directive('ddActivityLogWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/activity-log.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$q', '$timeout', 'ddVisibility', function($scope, $http, $q, $timeout, ddVisibility) {
                var ctrl = this;
                ctrl.items    = [];
                ctrl.loading  = false;
                var cancelToken    = null;
                var _debounceTimer = null;

                function load() {
                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();

                    var cfg      = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var maxItems = parseInt(cfg.maxItems) || 15;
                    ctrl.loading = true;

                    $http.get('json.htm', {
                        params: {
                            type:  'command',
                            param: 'getdevices',
                            used:  'true',
                            order: 'LastUpdate',
                            filter: 'all'
                        },
                        timeout: cancelToken.promise
                    }).then(function(resp) {
                        ctrl.items   = (resp.data.result || []).slice(0, maxItems);
                        ctrl.loading = false;
                    }).catch(function(err) {
                        if (err.status === -1) { return; }
                        ctrl.loading = false;
                    });
                }

                function debouncedLoad() {
                    if (ddVisibility.isHidden()) { return; }
                    if (_debounceTimer) { $timeout.cancel(_debounceTimer); }
                    _debounceTimer = $timeout(load, 2000);
                }

                var deregDeviceUpdate = $scope.$on('device_update', debouncedLoad);
                $scope.$on('dd:page:hidden',  function() {
                    if (_debounceTimer) { $timeout.cancel(_debounceTimer); _debounceTimer = null; }
                });
                $scope.$on('dd:page:visible', function() { load(); });

                $scope.$on('$destroy', function() {
                    deregDeviceUpdate();
                    if (cancelToken)    { cancelToken.resolve(); cancelToken = null; }
                    if (_debounceTimer) { $timeout.cancel(_debounceTimer); _debounceTimer = null; }
                });
                $scope.$on('dd:widget:refresh', load);

                ctrl.$onInit = load;
            }]
        };
    }]);
});
