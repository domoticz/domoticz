define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
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
                key:   'actions',
                type:  'action-list',
                label: 'Actions'
            }
        ]
    });

    app.directive('ddQuickActionsWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/quick-actions.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$timeout', 'ddToast', function($scope, $http, $timeout, ddToast) {
                var ctrl = this;
                ctrl.actions  = [];
                ctrl.busy     = {};
                ctrl.success  = {};
                ctrl.error    = {};

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
                    if (Array.isArray(raw)) { ctrl.actions = raw; return; }
                    try {
                        ctrl.actions = JSON.parse(raw);
                    } catch (e) {
                        ctrl.actions = [];
                    }
                }

                ctrl.execute = function(action) {
                    if (ctrl.busy[action.idx]) { return; }
                    ctrl.busy[action.idx]   = true;
                    ctrl.error[action.idx]  = false;

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
                        .then(function(resp) {
                            var data = resp.data || {};
                            if (data.status === 'OK') {
                                ctrl.success[action.idx] = true;
                                $timeout(function() {
                                    ctrl.success[action.idx] = false;
                                }, 1200);
                            } else {
                                var msg = data.message || data.status || 'Unknown error';
                                ctrl.error[action.idx] = true;
                                ddToast.error((action.label || action.idx) + ': ' + msg);
                                $timeout(function() {
                                    ctrl.error[action.idx] = false;
                                }, 2500);
                            }
                        })
                        .catch(function(err) {
                            var msg = (err && err.statusText) ? err.statusText : 'Request failed';
                            ctrl.error[action.idx] = true;
                            ddToast.error((action.label || action.idx) + ': ' + msg);
                            $timeout(function() {
                                ctrl.error[action.idx] = false;
                            }, 2500);
                        })
                        .finally(function() {
                            ctrl.busy[action.idx] = false;
                        });
                };
            }]
        };
    }]);
});
