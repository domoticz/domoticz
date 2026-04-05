define([
    'app',
    'dashboardDynamic/dashboardDynamic.module',
    'dashboardDynamic/widgetRegistry.service'
], function(app) {
    'use strict';

    /**
     * DdWidgetSettingsCtrl
     *
     * Modal controller for the widget configuration dialog.
     * Opened via $uibModal with:
     *   resolve: {
     *     widget:     function() { return widgetObject; },
     *     descriptor: function() { return descriptorObject; }
     *   }
     *
     * On save, closes the modal with the updated config object.
     * The caller is responsible for writing the config back to the widget.
     */
    app.controller('DdWidgetSettingsCtrl', [
        '$scope', '$uibModalInstance', '$http', 'widget', 'descriptor',
        function($scope, $uibModalInstance, $http, widget, descriptor) {

        $scope.descriptor        = descriptor;
        $scope.config            = angular.copy(widget.config || {});
        $scope.deviceList        = [];
        $scope.deviceListByField = {};
        $scope.scenes            = [];
        $scope.cameras           = [];
        $scope.pickerOptions     = {};

        // Pre-populate schema defaults for keys not yet set in config
        (descriptor.configSchema || []).forEach(function(field) {
            if ($scope.config[field.key] === undefined && field.default !== undefined) {
                $scope.config[field.key] = field.default;
            }
        });

        // Pre-populate defaults for sub-fields inside group fields
        (descriptor.configSchema || []).forEach(function(field) {
            if (field.type === 'group' && field.fields) {
                field.fields.forEach(function(subField) {
                    if ($scope.config[subField.key] === undefined && subField.default !== undefined) {
                        $scope.config[subField.key] = subField.default;
                    }
                });
            }
        });

        // Lazy-load device list only if a device-picker field is present
        var devicePickerFields = (descriptor.configSchema || []).filter(function(f) {
            return f.type === 'device-picker';
        });
        if (devicePickerFields.length) {
            $http.get('json.htm?type=command&param=getdevices&order=Name&displayhidden=1')
                .then(function(resp) {
                    var all = (resp.data && resp.data.result) || [];
                    $scope.deviceList = all;

                    function deviceLabel(d) {
                        var typeStr = (d.SubType && d.SubType !== d.Type)
                            ? d.Type + '/' + d.SubType
                            : d.Type;
                        return d.Name + (typeStr ? ' (' + typeStr + ')' : '');
                    }

                    // Build per-field filtered + sorted lists
                    devicePickerFields.forEach(function(field) {
                        var filter = field.deviceFilter;
                        var filtered = filter ? all.filter(function(d) {
                            if (filter === 'temp') {
                                return d.Type && d.Type.indexOf('Temp') >= 0;
                            }
                            if (filter === 'wind') {
                                return d.Type === 'Wind';
                            }
                            if (filter === 'baro') {
                                return typeof d.Barometer !== 'undefined' ||
                                       (d.Type && (d.Type.indexOf('Baro') >= 0 || d.Type === 'Barometer')) ||
                                       (d.SubType && d.SubType.indexOf('Baro') >= 0);
                            }
                            if (filter === 'rain') {
                                return d.Type === 'Rain';
                            }
                            if (filter === 'setpoint') {
                                var sub = (d.SubType || '').toLowerCase();
                                return sub === 'setpoint' || sub === 'set point';
                            }
                            if (filter === 'thermostat6') {
                                return (d.Type || '') === 'Thermostat 6';
                            }
                            if (filter === 'kwh') {
                                var t = d.Type || '';
                                var st = d.SubType || '';
                                return (t === 'General' && st === 'kWh') ||
                                       (t.indexOf('Meter') >= 0 && (d.SwitchTypeVal === 0 || d.SwitchTypeVal === 4));
                            }
                            if (filter === 'gas') {
                                return (d.Type || '').indexOf('Meter') >= 0 && d.SwitchTypeVal === 1;
                            }
                            if (filter === 'water') {
                                return d.SwitchTypeVal === 2 &&
                                       ((d.Type || '').indexOf('Meter') >= 0 ||
                                        ((d.Type || '') === 'General' &&
                                         (d.SubType === 'Counter Incremental' || d.SubType === 'Managed Counter')));
                            }
                            if (filter === 'p1') {
                                return d.Type === 'P1 Smart Meter';
                            }
                            if (filter === 'text') {
                                return (d.SubType || '') === 'Text';
                            }
                            if (filter === 'numeric') {
                                // Exclude switches/lights — identified by a non-empty SwitchType string
                                // (sensors/meters return SwitchType as empty or absent)
                                if (d.SwitchType && d.SwitchType !== '') { return false; }
                                var t = d.Type || '';
                                return t.indexOf('Lighting') < 0 &&
                                       t !== 'Light/Switch' &&
                                       t !== 'Color Switch' &&
                                       t !== 'Blinds' &&
                                       t !== 'RFY' &&
                                       t !== 'Security';
                            }
                            if (filter === 'counter') {
                                var t = d.Type || '';
                                var st = d.SubType || '';
                                return t.indexOf('Meter') >= 0 ||
                                       t === 'Cube Electric' ||
                                       (t === 'General' && (
                                           st === 'kWh' ||
                                           st === 'Counter Incremental' ||
                                           st === 'Managed Counter'
                                       ));
                            }
                            return true;
                        }) : all;
                        $scope.deviceListByField[field.key] = filtered.slice().sort(function(a, b) {
                            var la = filter ? a.Name : deviceLabel(a);
                            var lb = filter ? b.Name : deviceLabel(b);
                            return la.localeCompare(lb);
                        });
                        $scope.pickerOptions[field.key] = $scope.deviceListByField[field.key].map(function(d) {
                            return { value: String(d.idx), label: field.deviceFilter ? d.Name : deviceLabel(d) };
                        });
                    });

                    // Helper so the template can compute the label
                    $scope.deviceLabel = deviceLabel;
                });
        }

        // Lazy-load scene list only if a scene-picker field is present
        var needsScenes = (descriptor.configSchema || []).some(function(f) {
            return f.type === 'scene-picker';
        });
        if (needsScenes) {
            $http.get('json.htm?type=command&param=getscenes')
                .then(function(resp) {
                    $scope.scenes = (resp.data && resp.data.result) || [];
                    $scope.sceneOptions = $scope.scenes.map(function(s) {
                        return { value: String(s.idx), label: s.Name };
                    });
                });
        }

        // Lazy-load plan list only if a plan-picker field is present
        var needsPlans = (descriptor.configSchema || []).some(function(f) {
            return f.type === 'plan-picker';
        });
        if (needsPlans) {
            $http.get('json.htm?type=command&param=getplans&order=name&used=true')
                .then(function(resp) {
                    $scope.plans = (resp.data && resp.data.result) || [];
                    $scope.planOptions = $scope.plans.map(function(p) {
                        return { value: String(p.idx), label: p.Name };
                    });
                })
                .catch(function() { $scope.plans = []; });
        }

        // Lazy-load camera list only if a camera-picker field is present
        var needsCameras = (descriptor.configSchema || []).some(function(f) {
            return f.type === 'camera-picker';
        });
        if (needsCameras) {
            $http.get('json.htm', { params: { type: 'command', param: 'getcameras', order: 'Name' } })
                .then(function(resp) {
                    $scope.cameras = (resp.data && resp.data.result) || [];
                    $scope.cameraOptions = $scope.cameras.map(function(c) {
                        return { value: String(c.idx), label: c.Name };
                    });
                })
                .catch(function() { $scope.cameras = []; });
        }

        // For action-list fields: load devices + scenes and set up helpers
        var actionListFields = (descriptor.configSchema || []).filter(function(f) {
            return f.type === 'action-list';
        });
        if (actionListFields.length) {
            // Parse legacy JSON string to array
            actionListFields.forEach(function(field) {
                var val = $scope.config[field.key];
                if (typeof val === 'string') {
                    try { $scope.config[field.key] = JSON.parse(val); } catch(e) { $scope.config[field.key] = []; }
                }
                if (!Array.isArray($scope.config[field.key])) {
                    $scope.config[field.key] = [];
                }
            });

            $scope.actionDeviceOptions  = [];
            $scope.actionSceneOptions   = [];

            var actionDeviceFilter = actionListFields[0] && actionListFields[0].deviceFilter;

            function actionDeviceHasStop(d) {
                return (d.SubType === 'RAEX') ||
                    (d.SubType && (d.SubType.indexOf('A-OK') === 0 || d.SubType.indexOf('Hasta') >= 0 ||
                                   d.SubType.indexOf('Media Mount') === 0 || d.SubType.indexOf('Forest') === 0 ||
                                   d.SubType.indexOf('Chamberlain') === 0 || d.SubType.indexOf('Sunpery') === 0 ||
                                   d.SubType.indexOf('Dolat') === 0 || d.SubType.indexOf('ASP') === 0 ||
                                   d.SubType === 'Harrison' || d.SubType.indexOf('RFY') === 0 ||
                                   d.SubType.indexOf('ASA') === 0 || d.SubType.indexOf('DC106') === 0 ||
                                   d.SubType.indexOf('Confexx') === 0)) ||
                    (d.SwitchType && (d.SwitchType.indexOf('Venetian Blinds') === 0 ||
                                      d.SwitchType.indexOf('Stop') >= 0));
            }

            function lookupActionDevice(idx) {
                return ($scope.actionDevices || []).find(function(d) { return String(d.idx) === String(idx); });
            }

            // Load devices
            $http.get('json.htm?type=command&param=getdevices&order=Name&displayhidden=1&used=true')
                .then(function(resp) {
                    var all = (resp.data && resp.data.result) || [];
                    $scope.actionDevices = actionDeviceFilter === 'switch'
                        ? all.filter(function(d) { return d.SwitchType && d.SwitchType !== ''; })
                        : all;
                    $scope.actionDeviceOptions = $scope.actionDevices.map(function(d) {
                        return { value: String(d.idx), label: d.Name };
                    });
                });

            // Load scenes
            $http.get('json.htm?type=command&param=getscenes')
                .then(function(resp) {
                    $scope.actionScenes = (resp.data && resp.data.result) || [];
                    $scope.actionSceneOptions = $scope.actionScenes.map(function(s) {
                        return { value: String(s.idx), label: s.Name };
                    });
                });

            $scope.getActionOptions = function() {
                return $scope.newAction.type === 'scene'
                    ? $scope.actionSceneOptions
                    : $scope.actionDeviceOptions;
            };

            $scope.newActionDevice = function() {
                if (!$scope.newAction.idx || $scope.newAction.type !== 'switch') { return null; }
                return lookupActionDevice($scope.newAction.idx);
            };

            // New action form state
            $scope.newAction = { type: 'switch', idx: '', label: '' };

            $scope.$watch('newAction.type', function(val, old) {
                if (val !== old) {
                    $scope.newAction.idx = '';
                }
            });

            $scope.actionAddItem = function(fieldKey) {
                var a = $scope.newAction;
                if (!a.idx) { return; }
                var d     = a.type === 'switch' ? lookupActionDevice(a.idx) : null;
                var label = (a.label || '').trim();

                if (a.type === 'scene') {
                    if (!label) {
                        var s = ($scope.actionScenes || []).find(function(x) { return String(x.idx) === String(a.idx); });
                        label = s ? s.Name : a.idx;
                    }
                    $scope.config[fieldKey].push({ type: 'scene', idx: String(a.idx), label: label, icon: 'fa-solid fa-play' });

                } else if (d && d.SwitchType === 'Selector') {
                    if (!label) { label = d.Name; }
                    $scope.config[fieldKey].push({ type: 'selector', idx: String(a.idx), label: label });

                } else if (d && d.SwitchType && d.SwitchType.indexOf('Blinds') >= 0) {
                    if (!label) { label = d.Name; }
                    $scope.config[fieldKey].push({ type: 'blind', idx: String(a.idx), label: label, hasStop: actionDeviceHasStop(d) });

                } else {
                    if (!label) { label = d ? d.Name : a.idx; }
                    var action = { type: 'switch', idx: String(a.idx), label: label, icon: 'fa-solid fa-power-off' };
                    if (d && d.SwitchType === 'Push On Button')  { action.switchcmd = 'On'; }
                    if (d && d.SwitchType === 'Push Off Button') { action.switchcmd = 'Off'; }
                    $scope.config[fieldKey].push(action);
                }

                $scope.newAction = { type: a.type, idx: '', label: '' };
            };

            $scope.actionRenameItem = function(fieldKey, index) {
                var action = $scope.config[fieldKey][index];
                bootbox.prompt({
                    title: 'Rename action',
                    inputType: 'text',
                    value: action.label || '',
                    callback: function(result) {
                        if (result === null) { return; }
                        $scope.$apply(function() {
                            action.label = result.trim() || action.label;
                        });
                    }
                });
            };

            $scope.actionRemoveItem = function(fieldKey, index) {
                $scope.config[fieldKey].splice(index, 1);
            };
        }

        $scope.save = function() {
            if ($scope.settingsForm.$invalid) {
                $scope.settingsForm.$setSubmitted();
                return;
            }
            $uibModalInstance.close($scope.config);
        };

        $scope.cancel = function() {
            $uibModalInstance.dismiss('cancel');
        };
    }]);

    app.directive('ddSortableList', function() {
        return {
            restrict: 'A',
            link: function(scope, element, attrs) {
                var dragSrcIdx = null;

                element.on('dragstart', '> .dd-al-item', function(e) {
                    dragSrcIdx = parseInt($(this).data('sortIndex'), 10);
                    e.originalEvent.dataTransfer.effectAllowed = 'move';
                    $(this).addClass('dd-al-dragging');
                });

                element.on('dragover', '> .dd-al-item', function(e) {
                    if (dragSrcIdx === null) { return; }
                    e.preventDefault();
                    e.originalEvent.dataTransfer.dropEffect = 'move';
                    element.find('> .dd-al-item').removeClass('dd-al-drag-over');
                    $(this).addClass('dd-al-drag-over');
                });

                element.on('dragleave', '> .dd-al-item', function() {
                    $(this).removeClass('dd-al-drag-over');
                });

                element.on('drop', '> .dd-al-item', function(e) {
                    e.preventDefault();
                    var dropIdx = parseInt($(this).data('sortIndex'), 10);
                    element.find('> .dd-al-item').removeClass('dd-al-drag-over');
                    if (isNaN(dragSrcIdx) || isNaN(dropIdx) || dragSrcIdx === dropIdx) {
                        dragSrcIdx = null;
                        return;
                    }
                    var from = dragSrcIdx;
                    dragSrcIdx = null;
                    scope.$apply(function() {
                        var arr = scope.$eval(attrs.ddSortableList);
                        var item = arr.splice(from, 1)[0];
                        arr.splice(dropIdx, 0, item);
                    });
                });

                element.on('dragend', '> .dd-al-item', function() {
                    element.find('> .dd-al-item')
                        .removeClass('dd-al-drag-over')
                        .removeClass('dd-al-dragging');
                    dragSrcIdx = null;
                });

                scope.$on('$destroy', function() {
                    element.off('dragstart dragover dragleave drop dragend');
                });
            }
        };
    });
});
