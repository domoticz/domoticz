define([
    'app',
    'dashboardDynamic/widgetRegistry.service',
    'dashboardDynamic/dashboardDynamic.module',
    'widgets/dzLightWidget',
    'widgets/dzUtilityWidget',
    'widgets/dzSceneWidget'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'dz-room',
        label:       'Room / Plan',
        description: 'All devices in a specific room or plan',
        category:    'Devices',
        icon:        'fa-solid fa-house',
        defaultW:    6,
        defaultH:    6,
        minW:        3,
        minH:        4,
        maxW:        12,
        maxH:        20,
        configSchema: [
            { key: 'planIdx', type: 'plan-picker', label: 'Room / Plan',  required: true },
            { key: 'layout',  type: 'boolean',     label: 'List layout',  default: false },
            { key: 'fontSize', type: 'select', label: 'Font size',
              options: [
                  { value: '',       label: 'Default' },
                  { value: 'larger', label: 'Larger' },
                  { value: 'large',  label: 'Large' },
                  { value: 'xl',     label: 'Extra large' }
              ],
              default: ''
            },
            { key: 'title',   type: 'text',         label: 'Custom Title', required: false }
        ]
    });

    app.directive('ddDzRoomWidget', [function() {
        return {
            restrict:         'EA',
            templateUrl:      'views/dashboardDynamic/widgets/dz-room.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', '$timeout', '$q', '$location', 'ddDeviceClassifier', 'ddToast',
                function($scope, $http, $interval, $timeout, $q, $location, ddDeviceClassifier, ddToast) {
                var ctrl = this;
                ctrl.devices           = [];
                ctrl.listItems         = [];
                ctrl.loading           = false;
                ctrl.loadError         = null;
                ctrl.busy              = {};
                ctrl.success           = {};
                ctrl.itemError         = {};
                ctrl.deviceOn          = {};
                ctrl.dimLevel          = {};
                ctrl.showDimSlider     = {};
                ctrl.showLevelPicker   = {};
                ctrl.currentLevel      = {};
                ctrl.currentLevelText  = {};
                ctrl.levelOptions      = {};
                ctrl.blindStatus       = {};
                ctrl.securityStatus    = {};

                ctrl.isLight = function(d) {
                    return ddDeviceClassifier.getDirective(d) === 'dz-light-widget';
                };

                ctrl.isScene = function(d) {
                    return ddDeviceClassifier.getDirective(d) === 'dz-scene-widget';
                };

                function getActionType(d) {
                    var type = (d.Type || '').toLowerCase();
                    if (type.indexOf('scene') >= 0)                                         { return 'scene'; }
                    if (type.indexOf('group') >= 0)                                         { return 'group'; }
                    if (d.SwitchType === 'Security Panel' || d.Type === 'Security')        { return 'security'; }
                    if (d.SwitchType && d.SwitchType.indexOf('Blinds') >= 0)               { return 'blind'; }
                    if (d.SwitchType === 'Selector')                                        { return 'selector'; }
                    if (d.SwitchType === 'Dimmer')                                          { return 'dimmer'; }
                    if (d.SwitchType !== undefined || type.indexOf('light') >= 0 ||
                        type.indexOf('switch') >= 0)                                        { return 'switch'; }
                    return 'stat';
                }

                function decodeLevelNames(d) {
                    var raw;
                    try { raw = b64DecodeUnicode(d.LevelNames); } catch(e) { raw = d.LevelNames || ''; }
                    var levels = raw.split('|').map(function(n, i) { return { value: i * 10, label: n }; });
                    if (d.LevelOffHidden) {
                        levels = levels.filter(function(l) { return l.value !== 0; });
                    }
                    return levels;
                }

                function getBlindStatus(status) {
                    if (status === 'Open' || (status && status.indexOf('Set ') === 0)) { return 'open'; }
                    if (status === 'Stopped') { return 'stopped'; }
                    return 'closed';
                }

                function getLevelLabel(idx, levelInt) {
                    var opts = ctrl.levelOptions[idx] || [];
                    var opt  = opts.find(function(o) { return o.value === levelInt; });
                    return opt ? opt.label : '';
                }

                function buildListItems() {
                    ctrl.listItems = ctrl.devices.map(function(d) {
                        var extracted  = ddDeviceClassifier.extractDeviceValue(d);
                        var actionType = getActionType(d);
                        var item = {
                            idx:         String(d.idx),
                            label:       d.Name || String(d.idx),
                            icon:        ddDeviceClassifier.autoDeviceIcon(d),
                            type:        actionType,
                            hasStop:     !!(d.SwitchType && d.SwitchType.indexOf('Blinds') >= 0 &&
                                           d.SwitchType !== 'Blinds Percentage' && d.SwitchType !== 'Venetian Blinds EU' && d.SwitchType !== 'Venetian Blinds US'),
                            switchcmd:   (d.SwitchType === 'Push On Button' ? 'On' : d.SwitchType === 'Push Off Button' ? 'Off' : null),
                            value:       extracted.value ? String(extracted.value).replace(/\n/g, ' ') : extracted.value,
                            secondValue: extracted.secondValue ? String(extracted.secondValue).replace(/\n/g, ' ') : extracted.secondValue,
                            isOn:        extracted.isOn,
                            unit:        extracted.unit,
                            unit2:       extracted.unit2,
                            typeClass:   extracted.typeClass
                        };
                        if (actionType === 'blind') {
                            ctrl.blindStatus[item.idx] = getBlindStatus(d.Status);
                        } else if (actionType === 'security') {
                            ctrl.securityStatus[item.idx] = d.Status;
                        } else if (actionType !== 'stat' && actionType !== 'scene') {
                            var isLocked = d.SwitchType === 'Door Lock' || d.SwitchType === 'Door Lock Inverted';
                            ctrl.deviceOn[item.idx] = isLocked ? (d.Status === 'Unlocked') : extracted.isOn;
                        }
                        if (actionType === 'dimmer') {
                            ctrl.dimLevel[item.idx] = d.LevelInt !== undefined ? d.LevelInt : 100;
                        }
                        if (actionType === 'selector') {
                            if (!ctrl.levelOptions[item.idx]) {
                                ctrl.levelOptions[item.idx] = decodeLevelNames(d);
                            }
                            ctrl.currentLevel[item.idx]     = d.LevelInt;
                            ctrl.currentLevelText[item.idx] = getLevelLabel(item.idx, d.LevelInt);
                        }
                        return item;
                    });
                }

                ctrl.goToLog = function(item, $event) {
                    if ($event) { $event.stopPropagation(); }
                    $location.path('/Devices/' + item.idx + '/Log');
                };

                ctrl.execute = function(item, cmd) {
                    var busyKey = (item.type === 'blind' || item.type === 'group')
                        ? item.idx + '_' + cmd
                        : (item.type === 'security' ? item.idx + '_sec_' + cmd : item.idx);
                    if (ctrl.busy[busyKey]) { return; }
                    ctrl.busy[busyKey]      = true;
                    ctrl.itemError[busyKey] = false;
                    var params;
                    if (item.type === 'scene') {
                        params = { type: 'command', param: 'switchscene', idx: item.idx, switchcmd: 'On' };
                    } else if (item.type === 'group') {
                        params = { type: 'command', param: 'switchscene', idx: item.idx, switchcmd: cmd };
                    } else if (item.type === 'blind') {
                        params = { type: 'command', param: 'switchlight', idx: item.idx, switchcmd: cmd };
                    } else if (item.type === 'security') {
                        params = { type: 'command', param: 'setsecstatus', secstatus: cmd, udsecstatus: 0 };
                    } else {
                        params = { type: 'command', param: 'switchlight', idx: item.idx, switchcmd: item.switchcmd || 'Toggle' };
                    }
                    $http.get('json.htm', { params: params })
                        .then(function(resp) {
                            var data = resp.data || {};
                            if (data.status === 'OK') {
                                if (item.type !== 'security') {
                                    ctrl.deviceOn[item.idx] = !ctrl.deviceOn[item.idx];
                                }
                                ctrl.success[busyKey] = true;
                                $timeout(function() { ctrl.success[busyKey] = false; }, 1200);
                            } else {
                                ctrl.itemError[busyKey] = true;
                                ddToast.error((item.label || item.idx) + ': ' + (data.message || 'Unknown error'));
                                $timeout(function() { ctrl.itemError[busyKey] = false; }, 2500);
                            }
                        })
                        .catch(function(err) {
                            ctrl.itemError[busyKey] = true;
                            ddToast.error((item.label || item.idx) + ': ' + ((err && err.statusText) || 'Request failed'));
                            $timeout(function() { ctrl.itemError[busyKey] = false; }, 2500);
                        })
                        .finally(function() { ctrl.busy[busyKey] = false; });
                };

                ctrl.toggleDimSlider = function(idx, $event) {
                    if ($event) { $event.stopPropagation(); }
                    ctrl.showDimSlider[idx] = !ctrl.showDimSlider[idx];
                };

                ctrl.applyDimLevel = function(item) {
                    var level = parseInt(ctrl.dimLevel[item.idx], 10);
                    if (isNaN(level)) { return; }
                    $http.get('json.htm', { params: { type: 'command', param: 'switchlight', idx: item.idx, switchcmd: 'Set Level', level: level } })
                        .then(function(resp) {
                            if (resp.data && resp.data.status === 'OK') {
                                ctrl.deviceOn[item.idx] = level > 0;
                            } else {
                                ddToast.error((item.label || item.idx) + ': ' + ((resp.data && resp.data.message) || 'Unknown error'));
                            }
                        })
                        .catch(function(err) {
                            ddToast.error((item.label || item.idx) + ': ' + ((err && err.statusText) || 'Request failed'));
                        });
                };

                ctrl.toggleLevelPicker = function(idx, $event) {
                    if ($event) { $event.stopPropagation(); }
                    ctrl.showLevelPicker[idx] = !ctrl.showLevelPicker[idx];
                };

                ctrl.selectLevel = function(item, level) {
                    var busyKey = item.idx + '_sel';
                    if (ctrl.busy[busyKey]) { return; }
                    ctrl.busy[busyKey]      = true;
                    ctrl.itemError[busyKey] = false;
                    $http.get('json.htm', { params: { type: 'command', param: 'switchlight', idx: item.idx, switchcmd: 'Set Level', level: level } })
                        .then(function(resp) {
                            var data = resp.data || {};
                            if (data.status === 'OK') {
                                ctrl.showLevelPicker[item.idx]  = false;
                                ctrl.currentLevel[item.idx]     = level;
                                ctrl.currentLevelText[item.idx] = getLevelLabel(item.idx, level);
                                ctrl.success[busyKey] = true;
                                $timeout(function() { ctrl.success[busyKey] = false; }, 1200);
                            } else {
                                ctrl.itemError[busyKey] = true;
                                ddToast.error((item.label || item.idx) + ': ' + (data.message || 'Unknown error'));
                                $timeout(function() { ctrl.itemError[busyKey] = false; }, 2500);
                            }
                        })
                        .catch(function(err) {
                            ctrl.itemError[busyKey] = true;
                            ddToast.error((item.label || item.idx) + ': ' + ((err && err.statusText) || 'Request failed'));
                            $timeout(function() { ctrl.itemError[busyKey] = false; }, 2500);
                        })
                        .finally(function() { ctrl.busy[busyKey] = false; });
                };

                function onDocClick() {
                    var any = false;
                    Object.keys(ctrl.showDimSlider).forEach(function(k) {
                        if (ctrl.showDimSlider[k]) { ctrl.showDimSlider[k] = false; any = true; }
                    });
                    Object.keys(ctrl.showLevelPicker).forEach(function(k) {
                        if (ctrl.showLevelPicker[k]) { ctrl.showLevelPicker[k] = false; any = true; }
                    });
                    if (any && !$scope.$$phase) { $scope.$apply(); }
                }
                document.addEventListener('click', onDocClick);

                var loadCancel = null;

                function load() {
                    var cfg    = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var planId = cfg.planIdx;

                    if (!planId) {
                        ctrl.devices   = [];
                        ctrl.loading   = false;
                        ctrl.loadError = null;
                        return;
                    }

                    if (loadCancel) { loadCancel.resolve(); }
                    loadCancel = $q.defer();

                    ctrl.loading   = true;
                    ctrl.loadError = null;

                    var url = 'json.htm?type=command&param=getdevices&filter=all&used=true&plan=' + planId + '&order=Name';

                    $http.get(url, { timeout: loadCancel.promise })
                        .then(function(resp) {
                            ctrl.loading = false;
                            ctrl.devices = (resp.data && resp.data.result) || [];
                            buildListItems();
                        })
                        .catch(function(err) {
                            if (err && err.status === -1) { return; }
                            ctrl.loading   = false;
                            ctrl.loadError = 'Failed to load room devices';
                        });
                }

                $scope.$on('device_update', function(e, updated) {
                    var idx = String(updated.idx);
                    for (var i = 0; i < ctrl.devices.length; i++) {
                        if (String(ctrl.devices[i].idx) === idx) {
                            ctrl.devices[i] = updated;
                            buildListItems();
                            return;
                        }
                    }
                    load();
                });
                $scope.$on('scene_update', load);
                $scope.$on('dd:widget:refresh', load);

                $scope.$watch(
                    function() { return ctrl.widgetDef && ctrl.widgetDef.config && ctrl.widgetDef.config.planIdx; },
                    function(val, old) { if (val !== old) { load(); } }
                );

                var timer = $interval(load, 60000);
                $scope.$on('$destroy', function() {
                    $interval.cancel(timer);
                    if (loadCancel) { loadCancel.resolve(); loadCancel = null; }
                    document.removeEventListener('click', onDocClick);
                });

                ctrl.$onInit = load;
            }]
        };
    }]);
});
