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
                type:   'group',
                spread: true,
                fields: [
                    { key: 'layout',         type: 'boolean', label: 'List layout',          default: false },
                    { key: 'showBackground', type: 'boolean', label: 'Show panel background', default: true }
                ]
            },
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
                ctrl.actions          = [];
                ctrl.busy             = {};
                ctrl.success          = {};
                ctrl.error            = {};
                ctrl.deviceOn         = {};
                ctrl.levelOptions     = {};
                ctrl.currentLevel     = {};
                ctrl.currentLevelText = {};
                ctrl.showLevelPicker  = {};
                ctrl.dimLevel         = {};
                ctrl.showDimSlider    = {};

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
                            ctrl.currentLevel[a.idx]     = updated.LevelInt;
                            ctrl.currentLevelText[a.idx] = getLevelLabel(a.idx, updated.LevelInt);
                        } else if (a.type === 'group') {
                            ctrl.deviceOn[a.idx] = (updated.Status === 'On');
                        } else if (a.type === 'blind') {
                            ctrl.deviceOn[a.idx] = (updated.Status === 'Open' ||
                                (updated.Status && updated.Status.indexOf('Set ') === 0) ||
                                updated.Status === 'Stopped');
                        } else if (a.type === 'dimmer') {
                            ctrl.deviceOn[a.idx] = (updated.Status === 'On');
                            if (updated.LevelInt !== undefined) {
                                ctrl.dimLevel[a.idx] = updated.LevelInt;
                            }
                        } else {
                            ctrl.deviceOn[a.idx] = (updated.Status === 'On');
                        }
                    });
                });

                function onDocClick() {
                    var any = false;
                    Object.keys(ctrl.showDimSlider).forEach(function(k) {
                        if (ctrl.showDimSlider[k]) { ctrl.showDimSlider[k] = false; any = true; }
                    });
                    if (any && !$scope.$$phase) { $scope.$apply(); }
                }
                document.addEventListener('click', onDocClick);
                $scope.$on('$destroy', function() { document.removeEventListener('click', onDocClick); });

                function decodeLevelNames(d) {
                    var raw;
                    try { raw = b64DecodeUnicode(d.LevelNames); } catch(e) { raw = d.LevelNames || ''; }
                    return raw.split('|').map(function(n, i) { return { value: i * 10, label: n }; });
                }

                function getLevelLabel(idx, levelInt) {
                    var opts = ctrl.levelOptions[idx] || [];
                    var opt  = opts.find(function(o) { return o.value === levelInt; });
                    return opt ? opt.label : '';
                }

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
                    if (action.type === 'dimmer') {
                        $http.get('json.htm', { params: { type: 'command', param: 'getdevices', rid: action.idx } })
                            .then(function(resp) {
                                var item = resp.data && resp.data.result && resp.data.result[0];
                                if (!item) { return; }
                                ctrl.deviceOn[action.idx] = (item.Status === 'On');
                                ctrl.dimLevel[action.idx] = item.LevelInt !== undefined ? item.LevelInt : 100;
                            });
                        return;
                    }
                    if (action.type === 'group') {
                        $http.get('json.htm', { params: { type: 'command', param: 'getscenes' } })
                            .then(function(resp) {
                                var results = (resp.data && resp.data.result) || [];
                                var item = results.find(function(s) { return String(s.idx) === String(action.idx); });
                                if (item) {
                                    ctrl.deviceOn[action.idx] = (item.Status === 'On');
                                }
                            });
                        return;
                    }
                    if (action.switchcmd === 'On' || action.switchcmd === 'Off') { return; }
                    $http.get('json.htm', { params: { type: 'command', param: 'getdevices', rid: action.idx } })
                        .then(function(resp) {
                            var item = resp.data && resp.data.result && resp.data.result[0];
                            if (!item) { return; }
                            if (action.type === 'selector') {
                                if (!ctrl.levelOptions[action.idx]) {
                                    ctrl.levelOptions[action.idx] = decodeLevelNames(item);
                                }
                                ctrl.currentLevel[action.idx]     = item.LevelInt;
                                ctrl.currentLevelText[action.idx] = getLevelLabel(action.idx, item.LevelInt);
                            } else if (action.type === 'blind') {
                                ctrl.deviceOn[action.idx] = (item.Status === 'Open' ||
                                    (item.Status && item.Status.indexOf('Set ') === 0) ||
                                    item.Status === 'Stopped');
                            } else {
                                ctrl.deviceOn[action.idx] = (item.Status === 'On');
                                if (item.SwitchType === 'Dimmer') {
                                    action.type = 'dimmer';
                                    ctrl.dimLevel[action.idx] = item.LevelInt !== undefined ? item.LevelInt : 100;
                                }
                            }
                        });
                }

                ctrl.activeKey = function(action) {
                    return action.idx;
                };

                function loadDeviceStates() {
                    (ctrl.actions || []).forEach(fetchDeviceState);
                }

                ctrl.toggleLevelPicker = function(idx) {
                    ctrl.showLevelPicker[idx] = !ctrl.showLevelPicker[idx];
                };

                ctrl.selectLevel = function(action, level) {
                    ctrl.showLevelPicker[action.idx] = false;
                    var busyKey = action.idx + '_sel';
                    if (ctrl.busy[busyKey]) { return; }
                    ctrl.busy[busyKey]  = true;
                    ctrl.error[busyKey] = false;
                    $http.get('json.htm', { params: { type: 'command', param: 'switchlight', idx: action.idx, switchcmd: 'Set Level', level: level } })
                        .then(function(resp) {
                            var data = resp.data || {};
                            if (data.status === 'OK') {
                                ctrl.currentLevel[action.idx]     = level;
                                ctrl.currentLevelText[action.idx] = getLevelLabel(action.idx, level);
                                ctrl.success[busyKey] = true;
                                $timeout(function() { ctrl.success[busyKey] = false; }, 1200);
                            } else {
                                ctrl.error[busyKey] = true;
                                ddToast.error((action.label || action.idx) + ': ' + (data.message || 'Unknown error'));
                                $timeout(function() { ctrl.error[busyKey] = false; }, 2500);
                            }
                        })
                        .catch(function(err) {
                            ctrl.error[busyKey] = true;
                            ddToast.error((action.label || action.idx) + ': ' + ((err && err.statusText) || 'Request failed'));
                            $timeout(function() { ctrl.error[busyKey] = false; }, 2500);
                        })
                        .finally(function() { ctrl.busy[busyKey] = false; });
                };

                ctrl.toggleDimSlider = function(idx, $event) {
                    if ($event) { $event.stopPropagation(); }
                    ctrl.showDimSlider[idx] = !ctrl.showDimSlider[idx];
                };

                ctrl.applyDimLevel = function(action) {
                    var level = parseInt(ctrl.dimLevel[action.idx], 10);
                    if (isNaN(level)) { return; }
                    $http.get('json.htm', { params: { type: 'command', param: 'switchlight', idx: action.idx, switchcmd: 'Set Level', level: level } })
                        .then(function(resp) {
                            if (resp.data && resp.data.status === 'OK') {
                                ctrl.deviceOn[action.idx] = level > 0;
                                ctrl.success[action.idx + '_dim'] = true;
                                $timeout(function() { ctrl.success[action.idx + '_dim'] = false; }, 1200);
                            } else {
                                ddToast.error((action.label || action.idx) + ': ' + ((resp.data && resp.data.message) || 'Unknown error'));
                            }
                        })
                        .catch(function(err) {
                            ddToast.error((action.label || action.idx) + ': ' + ((err && err.statusText) || 'Request failed'));
                        });
                };

                ctrl.execute = function(action, cmd) {
                    var busyKey = (action.type === 'blind' || action.type === 'group')
                        ? action.idx + '_' + cmd
                        : action.idx;
                    if (ctrl.busy[busyKey]) { return; }
                    ctrl.busy[busyKey]  = true;
                    ctrl.error[busyKey] = false;

                    var params;
                    if (action.type === 'scene') {
                        params = { type: 'command', param: 'switchscene', idx: action.idx, switchcmd: 'On' };
                    } else if (action.type === 'group') {
                        params = { type: 'command', param: 'switchscene', idx: action.idx, switchcmd: cmd };
                    } else if (action.type === 'blind') {
                        params = { type: 'command', param: 'switchlight', idx: action.idx, switchcmd: cmd };
                    } else if (action.type === 'dimmer') {
                        params = { type: 'command', param: 'switchlight', idx: action.idx, switchcmd: action.switchcmd || 'Toggle' };
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
