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

        $scope.descriptor      = descriptor;
        $scope.config          = angular.copy(widget.config || {});
        $scope.deviceList      = [];
        $scope.deviceListByField = {};
        $scope.scenes          = [];
        $scope.cameras         = [];

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
                                       (d.SubType && d.SubType.indexOf('Barometer') >= 0);
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

            // Load devices
            $http.get('json.htm?type=command&param=getdevices&order=Name&displayhidden=1&used=true')
                .then(function(resp) {
                    $scope.actionDevices = (resp.data && resp.data.result) || [];
                });

            // Load scenes
            $http.get('json.htm?type=command&param=getscenes')
                .then(function(resp) {
                    $scope.actionScenes = (resp.data && resp.data.result) || [];
                });

            // New action form state
            $scope.newAction = { type: 'switch', idx: '', label: '', icon: '' };

            $scope.actionAddItem = function(fieldKey) {
                var a = $scope.newAction;
                if (!a.idx) { return; }
                // Auto-fill label from device/scene name if blank
                var label = (a.label || '').trim();
                if (!label) {
                    var list = a.type === 'scene' ? $scope.actionScenes : $scope.actionDevices;
                    var found = list && list.find(function(x) { return String(x.idx) === String(a.idx); });
                    label = found ? found.Name : a.idx;
                }
                $scope.config[fieldKey].push({
                    type:  a.type,
                    idx:   String(a.idx),
                    label: label,
                    icon:  a.icon || (a.type === 'scene' ? 'fa-solid fa-play' : 'fa-solid fa-power-off')
                });
                $scope.newAction = { type: a.type, idx: '', label: '', icon: '' };
            };

            $scope.actionRemoveItem = function(fieldKey, index) {
                $scope.config[fieldKey].splice(index, 1);
            };

            $scope.actionMoveUp = function(fieldKey, index) {
                if (index === 0) { return; }
                var arr = $scope.config[fieldKey];
                var tmp = arr[index - 1];
                arr[index - 1] = arr[index];
                arr[index] = tmp;
            };

            $scope.actionMoveDown = function(fieldKey, index) {
                var arr = $scope.config[fieldKey];
                if (index >= arr.length - 1) { return; }
                var tmp = arr[index + 1];
                arr[index + 1] = arr[index];
                arr[index] = tmp;
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
});
