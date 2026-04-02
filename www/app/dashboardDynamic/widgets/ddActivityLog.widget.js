define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'activity-log',
        label:       'Activity Log',
        description: 'Recent device state changes',
        category:    'System & Logs',
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
            controller: ['$scope', '$http', '$interval', function($scope, $http, $interval) {
                var ctrl = this;
                ctrl.items    = [];
                ctrl.loading  = false;

                function load() {
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
                        }
                    }).then(function(resp) {
                        ctrl.items   = (resp.data.result || []).slice(0, maxItems);
                        ctrl.loading = false;
                    }).catch(function() {
                        ctrl.loading = false;
                    });
                }

                $scope.$on('device_update', load);

                var timer = $interval(load, 15000);

                $scope.$on('$destroy', function() { $interval.cancel(timer); });
                $scope.$on('dd:widget:refresh', load);

                ctrl.$onInit = load;
            }]
        };
    }]);
});
