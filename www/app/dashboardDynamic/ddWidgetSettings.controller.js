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
        var _metricCache         = {};   // per-device metric options cache (Custom Chart)
        var deviceListLoaded     = false; // true once getdevices has returned

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
                    deviceListLoaded = true;
                    _metricCache = {};   // device data arrived — drop any empty cached results
                    reconcileMetrics();  // now that devices are known, seed/validate metric choices

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
                        $scope.pickerOptions[field.key] =
                        [{ value: '', label: '-- None --' }]
                        .concat(
                            $scope.deviceListByField[field.key].map(function(d) {
                                return {
                                    value: String(d.idx),
                                    label: field.deviceFilter ? d.Name : deviceLabel(d)
                                };
                            })
                        );
                    });

                    // Helper so the template can compute the label
                    $scope.deviceLabel = deviceLabel;
                });
        }

        // ── Per-device metric selector (e.g. Custom Chart) ───────────────────
        // Fields carrying a `metricKey` get an extra dropdown to pick which value
        // of a multi-value sensor to plot (Temperature/Humidity/Barometer/Setpoint).
        var EMPTY_METRICS = [];
        var TEMP_METRIC_DEFS = [
            { value: 'te', label: 'Temperature', needs: 'Temp' },
            { value: 'hu', label: 'Humidity',    needs: 'Humidity' },
            { value: 'ba', label: 'Barometer',   needs: 'Barometer' },
            { value: 'se', label: 'Setpoint',    needs: 'SetPoint' }
        ];
        var metricFields = (descriptor.configSchema || []).filter(function(f) {
            return f.type === 'device-picker' && f.metricKey;
        });

        function deviceByIdx(idx) {
            var list = $scope.deviceList || [];
            for (var i = 0; i < list.length; i++) {
                if (String(list[i].idx) === String(idx)) { return list[i]; }
            }
            return null;
        }

        function computeMetricOptions(idx) {
            var d = deviceByIdx(idx);
            if (!d) { return EMPTY_METRICS; }
            // Wind sensors may carry Temp/Chill but are charted via the wind sensor,
            // so they are not temp-routed and offer no metric choice here.
            if ((d.Type || '') === 'Wind') { return EMPTY_METRICS; }
            var opts = [];
            TEMP_METRIC_DEFS.forEach(function(m) {
                if (d[m.needs] !== undefined) { opts.push({ value: m.value, label: m.label }); }
            });
            if (opts.length <= 1) { return EMPTY_METRICS; }
            // 'Auto' (empty value) = the device's default combined view (no override).
            return [{ value: '', label: 'Auto' }].concat(opts);
        }

        // Cached + stable-reference per (field, device) so ng-options/ng-if don't
        // churn the digest by receiving a fresh array every cycle.
        $scope.metricOptionsFor = function(field) {
            if (!field || !field.metricKey) { return EMPTY_METRICS; }
            var key = field.key + '|' + ($scope.config[field.key] || '');
            if (!_metricCache[key]) { _metricCache[key] = computeMetricOptions($scope.config[field.key]); }
            return _metricCache[key];
        };

        // Keep each stored metric valid for its currently-selected device.
        // A metric value of '' (Auto) or undefined both mean "device default / no
        // override" — the widgets treat any falsy metric as Auto.
        //   - multi-value sensor: default to 'Auto' (''), preserve a still-valid choice
        //   - single-value device: clear to undefined (no dropdown shown)
        //   - device missing: leave untouched until the list has loaded (avoids wiping
        //     a saved metric mid-load); once loaded, a truly-gone device is cleared.
        function reconcileMetrics() {
            metricFields.forEach(function(f) {
                var idx = $scope.config[f.key];
                var dev = idx ? deviceByIdx(idx) : null;
                if (!dev) {
                    if (deviceListLoaded && $scope.config[f.metricKey] !== undefined) {
                        $scope.config[f.metricKey] = undefined;
                    }
                    return;
                }
                var opts = $scope.metricOptionsFor(f);
                if (opts.length > 1) {
                    var cur = $scope.config[f.metricKey];
                    var valid = opts.some(function(o) { return o.value === cur; });
                    if (!valid) { $scope.config[f.metricKey] = ''; }
                } else if ($scope.config[f.metricKey] !== undefined) {
                    $scope.config[f.metricKey] = undefined;
                }
            });
        }

        if (metricFields.length) {
            // Re-validate when the user changes a device selection.
            $scope.$watch(
                function() {
                    return metricFields.map(function(f) { return $scope.config[f.key]; }).join(',');
                },
                reconcileMetrics
            );
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

            function actionDeviceHasLevel(d) {
                return !!(d.SwitchType && (d.SwitchType.indexOf('Percentage') >= 0 ||
                                           d.SwitchType.indexOf('%') >= 0));
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
                    var s = ($scope.actionScenes || []).find(function(x) { return String(x.idx) === String(a.idx); });
                    if (!label) {
                        label = s ? s.Name : a.idx;
                    }
                    var isGroup = s && s.Type === 'Group';
                    if (isGroup) {
                        $scope.config[fieldKey].push({ type: 'group', idx: String(a.idx), label: label, icon: 'fa-solid fa-toggle-on' });
                    } else {
                        $scope.config[fieldKey].push({ type: 'scene', idx: String(a.idx), label: label, icon: 'fa-solid fa-play' });
                    }

                } else if (d && (d.SwitchType === 'Security Panel' || d.Type === 'Security')) {
                    if (!label) { label = d.Name; }
                    $scope.config[fieldKey].push({ type: 'security', idx: String(a.idx), label: label, icon: 'fa-solid fa-shield-halved' });

                } else if (d && d.SwitchType === 'Selector') {
                    if (!label) { label = d.Name; }
                    $scope.config[fieldKey].push({ type: 'selector', idx: String(a.idx), label: label });

                } else if (d && d.SwitchType && d.SwitchType.indexOf('Blinds') >= 0) {
                    if (!label) { label = d.Name; }
                    $scope.config[fieldKey].push({ type: 'blind', idx: String(a.idx), label: label, hasStop: actionDeviceHasStop(d), hasLevel: actionDeviceHasLevel(d) });

                } else if (d && d.SwitchType === 'Dimmer') {
                    if (!label) { label = d.Name; }
                    $scope.config[fieldKey].push({ type: 'dimmer', idx: String(a.idx), label: label, icon: 'fa-solid fa-power-off' });

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
                var dlg = window.bootbox.prompt('Rename action', function(result) {
                    if (result === null) { return; }
                    $scope.$apply(function() {
                        action.label = result.trim() || action.label;
                    });
                });
                dlg.find('input').attr('type', 'text').val(action.label || '');
            };

            $scope.actionRemoveItem = function(fieldKey, index) {
                $scope.config[fieldKey].splice(index, 1);
            };
        }

        // For device-list fields: load all devices and set up add/remove helpers
        var deviceListFields = (descriptor.configSchema || []).filter(function(f) {
            return f.type === 'device-list';
        });
        if (deviceListFields.length) {
            // Parse legacy JSON string to array
            deviceListFields.forEach(function(field) {
                var val = $scope.config[field.key];
                if (typeof val === 'string') {
                    try { $scope.config[field.key] = JSON.parse(val); } catch(e) { $scope.config[field.key] = []; }
                }
                if (!Array.isArray($scope.config[field.key])) {
                    $scope.config[field.key] = [];
                }
            });

            $scope.deviceListOptions = [];
            $scope.newDeviceEntry    = { idx: '', label: '', icon: '' };

            $http.get('json.htm?type=command&param=getdevices&order=Name&displayhidden=1&used=true')
                .then(function(resp) {
                    var all = (resp.data && resp.data.result) || [];
                    $scope.allDevicesForList = all;
                    $scope.deviceListOptions = all.slice().sort(function(a, b) {
                        return a.Name.localeCompare(b.Name);
                    }).map(function(d) {
                        var typeStr = (d.SubType && d.SubType !== d.Type)
                            ? d.Type + '/' + d.SubType
                            : d.Type;
                        return { value: String(d.idx), label: d.Name + (typeStr ? ' (' + typeStr + ')' : '') };
                    });
                });

            $scope.deviceListAddItem = function(fieldKey) {
                var e = $scope.newDeviceEntry;
                if (!e.idx) { return; }
                var d     = ($scope.allDevicesForList || []).find(function(x) { return String(x.idx) === String(e.idx); });
                var label = (e.label || '').trim() || (d ? d.Name : String(e.idx));
                var icon  = (e.icon  || '').trim();
                $scope.config[fieldKey].push({ idx: String(e.idx), label: label, icon: icon });
                $scope.newDeviceEntry = { idx: '', label: '', icon: '' };
            };

            $scope.deviceListRenameItem = function(fieldKey, index) {
                var entry = $scope.config[fieldKey][index];
                var dlg = window.bootbox.prompt('Rename device label', function(result) {
                    if (result === null) { return; }
                    $scope.$apply(function() {
                        entry.label = result.trim() || entry.label;
                    });
                });
                dlg.find('input').attr('type', 'text').val(entry.label || '');
            };

            $scope.deviceListRemoveItem = function(fieldKey, index) {
                $scope.config[fieldKey].splice(index, 1);
            };
        }

        // Helper: returns true when the currently selected device (deviceIdx) is a switch
        $scope.isSelectedDeviceSwitch = function() {
            var idx = $scope.config.deviceIdx;
            if (!idx) { return false; }
            var d = ($scope.deviceList || []).find(function(x) { return String(x.idx) === String(idx); });
            return !!(d && d.SwitchType && d.SwitchType !== 'Selector');
        };

        // For range-list fields: initialize arrays and add/remove helpers
        var rangeListFields = (descriptor.configSchema || []).filter(function(f) {
            return f.type === 'range-list';
        });
        if (rangeListFields.length) {
            rangeListFields.forEach(function(field) {
                var val = $scope.config[field.key];
                if (typeof val === 'string') {
                    try { $scope.config[field.key] = JSON.parse(val); } catch(e) { $scope.config[field.key] = []; }
                }
                if (!Array.isArray($scope.config[field.key])) {
                    $scope.config[field.key] = [];
                }
            });

            $scope.newRange = { from: '', to: '', color: '#66bb6a' };
            $scope.rangeDirectionError = {};

            function parseDecimal(v) {
                return parseFloat(String(v).replace(',', '.'));
            }

            $scope.rangeAddItem = function(fieldKey) {
                var r    = $scope.newRange;
                var from = parseDecimal(r.from);
                var to   = parseDecimal(r.to);
                if (isNaN(from) || isNaN(to) || from === to) { return; }
                $scope.config[fieldKey].push({ from: from, to: to, color: r.color || '#66bb6a' });
                var isRTL = $scope.config[fieldKey][0] && $scope.config[fieldKey][0].from > $scope.config[fieldKey][0].to;
                $scope.config[fieldKey].sort(function(a, b) {
                    return isRTL ? b.from - a.from : a.from - b.from;
                });
                $scope.rangeDirectionError[fieldKey] = false;
                $scope.newRange = { from: '', to: '', color: '#66bb6a' };
            };

            $scope.rangeRemoveItem = function(fieldKey, index) {
                $scope.config[fieldKey].splice(index, 1);
                $scope.rangeDirectionError[fieldKey] = false;
            };

            $scope.rangeSeedDefaults = function(fieldKey, defaults) {
                $scope.config[fieldKey] = defaults.map(function(r) {
                    return { from: r.from, to: r.to, color: r.color };
                });
            };
        }

        $scope.save = function() {
            if ($scope.settingsForm.$invalid) {
                $scope.settingsForm.$setSubmitted();
                return;
            }
            var hasDirectionError = false;
            rangeListFields.forEach(function(field) {
                var ranges = $scope.config[field.key] || [];
                var hasAsc = false, hasDesc = false;
                ranges.forEach(function(r) {
                    if (r.from < r.to) { hasAsc  = true; }
                    if (r.from > r.to) { hasDesc = true; }
                });
                if (hasAsc && hasDesc) {
                    $scope.rangeDirectionError[field.key] = true;
                    hasDirectionError = true;
                }
            });
            if (hasDirectionError) { return; }
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
