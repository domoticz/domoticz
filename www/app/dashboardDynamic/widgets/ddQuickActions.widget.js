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

        configSchema: [
            {
                key:          'actions',
                type:         'action-list',
                label:        'Actions',
                deviceFilter: 'switch'
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
                ctrl.actions   = [];
                ctrl.busy      = {};
                ctrl.success   = {};
                ctrl.error     = {};
                ctrl.deviceOn  = {};

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

                $scope.$on('device_update', function(e, updated) {
                    (ctrl.actions || []).forEach(function(a) {
                        if (String(a.idx) !== String(updated.idx)) { return; }
                        if (a.switchcmd === 'On' || a.switchcmd === 'Off') { return; }
                        if (a.type === 'selector') {
                            ctrl.deviceOn[a.idx + '_' + a.level] = (updated.LevelInt === a.level);
                        } else if (a.type === 'blind') {
                            ctrl.deviceOn[a.idx] = (updated.Status === 'Open' ||
                                (updated.Status && updated.Status.indexOf('Set ') === 0) ||
                                updated.Status === 'Stopped');
                        } else {
                            ctrl.deviceOn[a.idx] = (updated.Status === 'On');
                        }
                    });
                });

                function parseActions() {
                    var raw = ctrl.widgetDef && ctrl.widgetDef.config &&
                              ctrl.widgetDef.config.actions;
                    if (!raw) { ctrl.actions = []; return; }
                    if (Array.isArray(raw)) {
                        ctrl.actions = raw;
                        loadDeviceStates();
                        return;
                    }
                    try {
                        ctrl.actions = JSON.parse(raw);
                    } catch (e) {
                        ctrl.actions = [];
                    }
                    loadDeviceStates();
                }

                function fetchDeviceState(action) {
                    if (action.type === 'scene') { return; }
                    if (action.switchcmd === 'On' || action.switchcmd === 'Off') { return; }
                    $http.get('json.htm', { params: { type: 'command', param: 'getdevices', rid: action.idx } })
                        .then(function(resp) {
                            var item = resp.data && resp.data.result && resp.data.result[0];
                            if (!item) { return; }
                            if (action.type === 'selector') {
                                ctrl.deviceOn[action.idx + '_' + action.level] = (item.LevelInt === action.level);
                            } else if (action.type === 'blind') {
                                ctrl.deviceOn[action.idx] = (item.Status === 'Open' ||
                                    (item.Status && item.Status.indexOf('Set ') === 0) ||
                                    item.Status === 'Stopped');
                            } else {
                                ctrl.deviceOn[action.idx] = (item.Status === 'On');
                            }
                        });
                }

                ctrl.activeKey = function(action) {
                    return action.type === 'selector' ? action.idx + '_' + action.level : action.idx;
                };

                function loadDeviceStates() {
                    (ctrl.actions || []).forEach(fetchDeviceState);
                }

                ctrl.execute = function(action, blindCmd) {
                    var busyKey = action.type === 'blind'
                        ? action.idx + '_' + blindCmd
                        : ctrl.activeKey(action);
                    if (ctrl.busy[busyKey]) { return; }
                    ctrl.busy[busyKey]  = true;
                    ctrl.error[busyKey] = false;

                    var params;
                    if (action.type === 'scene') {
                        params = { type: 'command', param: 'switchscene', idx: action.idx, switchcmd: 'On' };
                    } else if (action.type === 'selector') {
                        params = { type: 'command', param: 'switchlight', idx: action.idx, switchcmd: 'Set Level', level: action.level };
                    } else if (action.type === 'blind') {
                        params = { type: 'command', param: 'switchlight', idx: action.idx, switchcmd: blindCmd };
                    } else {
                        params = { type: 'command', param: 'switchlight', idx: action.idx, switchcmd: action.switchcmd || 'Toggle' };
                    }

                    $http.get('json.htm', { params: params })
                        .then(function(resp) {
                            var data = resp.data || {};
                            if (data.status === 'OK') {
                                fetchDeviceState(action);
                                ctrl.success[busyKey] = true;
                                $timeout(function() { ctrl.success[busyKey] = false; }, 1200);
                            } else {
                                var msg = data.message || data.status || 'Unknown error';
                                ctrl.error[busyKey] = true;
                                ddToast.error((action.label || action.idx) + ': ' + msg);
                                $timeout(function() { ctrl.error[busyKey] = false; }, 2500);
                            }
                        })
                        .catch(function(err) {
                            var msg = (err && err.statusText) ? err.statusText : 'Request failed';
                            ctrl.error[busyKey] = true;
                            ddToast.error((action.label || action.idx) + ': ' + msg);
                            $timeout(function() { ctrl.error[busyKey] = false; }, 2500);
                        })
                        .finally(function() { ctrl.busy[busyKey] = false; });
                };
            }]
        };
    }]);
});
