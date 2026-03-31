define([
    'app',
    'dashboard2/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'quick-actions',
        label:       'Quick Actions',
        description: 'One-click scene and device action buttons',
        category:    'Custom Content',
        icon:        'fa-solid fa-bolt',
        defaultW:    4,
        defaultH:    2,
        minW:        2,
        minH:        2,
        maxW:        12,
        maxH:        4,
        configSchema: [
            {
                key:      'actions',
                type:     'textarea',
                label:    'Actions (JSON array)',
                required: false,
                help:     'Array of {type:"scene"|"switch", idx:"5", label:"Name", icon:"fa-solid fa-power-off"}'
            }
        ]
    });

    app.directive('db2QuickActionsWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboard2/widgets/quick-actions.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$timeout', function($scope, $http, $timeout) {
                var ctrl = this;
                ctrl.actions  = [];
                ctrl.busy     = {};
                ctrl.success  = {};

                ctrl.$onInit = function() {
                    parseActions();
                };

                $scope.$watch(
                    function() {
                        return ctrl.widgetDef && ctrl.widgetDef.config &&
                               ctrl.widgetDef.config.actions;
                    },
                    function(val, old) {
                        if (val !== old) { parseActions(); }
                    }
                );

                function parseActions() {
                    var raw = ctrl.widgetDef && ctrl.widgetDef.config &&
                              ctrl.widgetDef.config.actions;
                    if (!raw) { ctrl.actions = []; return; }
                    try {
                        ctrl.actions = JSON.parse(raw);
                    } catch (e) {
                        ctrl.actions = [];
                    }
                }

                ctrl.execute = function(action) {
                    if (ctrl.busy[action.idx]) { return; }
                    ctrl.busy[action.idx] = true;

                    var params;
                    if (action.type === 'scene') {
                        params = {
                            type:      'command',
                            param:     'switchscene',
                            idx:       action.idx,
                            switchcmd: 'On'
                        };
                    } else {
                        params = {
                            type:      'command',
                            param:     'switchlight',
                            idx:       action.idx,
                            switchcmd: 'Toggle'
                        };
                    }

                    $http.get('json.htm', { params: params })
                        .then(function() {
                            ctrl.success[action.idx] = true;
                            $timeout(function() {
                                ctrl.success[action.idx] = false;
                            }, 1200);
                        })
                        .finally(function() {
                            ctrl.busy[action.idx] = false;
                        });
                };
            }]
        };
    }]);
});
