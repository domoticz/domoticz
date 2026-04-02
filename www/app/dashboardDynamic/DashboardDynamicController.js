define([
    'app',
    'dashboardDynamic/dashboardDynamic.module',
    'dashboardDynamic/dashboardDynamicService',
    'dashboardDynamic/widgetRegistry.service',
    'dashboardDynamic/ddToast.service',
    'dashboardDynamic/ddWidgetContent.directive',
    'dashboardDynamic/ddWidgetWrapper',
    'dashboardDynamic/ddGrid',
    'dashboardDynamic/ddWidgetSettings.controller',
    'dashboardDynamic/ddDashboardManager.controller',
    'dashboardDynamic/ddExportImport.controller',
    'widgets/dzLightWidget',
    'widgets/dzUtilityWidget',
    'widgets/dzSceneWidget',
    'dashboardDynamic/widgets/ddClock.widget',
    'dashboardDynamic/widgets/ddSunInfo.widget',
    'dashboardDynamic/widgets/ddActivityLog.widget',
    'dashboardDynamic/widgets/ddSystemStatus.widget',
    'dashboardDynamic/widgets/ddTextNote.widget',
    'dashboardDynamic/widgets/ddQuickActions.widget',
    'dashboardDynamic/widgets/ddWeatherWidget.widget',
    'dashboardDynamic/widgets/ddStatCounter.widget',
    'dashboardDynamic/widgets/ddTemperatureGraph.widget',
    'dashboardDynamic/widgets/ddEnergyChart.widget',
    'dashboardDynamic/widgets/ddWindChart.widget',
    'dashboardDynamic/widgets/ddRainChart.widget',
    'dashboardDynamic/widgets/ddIframeEmbed.widget',
    'dashboardDynamic/widgets/ddImageWidget.widget',
    'dashboardDynamic/widgets/ddCameraFeed.widget',
    'dashboardDynamic/widgets/ddDzDevice.widget',
    'dashboardDynamic/widgets/ddDzScene.widget',
    'dashboardDynamic/widgets/ddDzFavorites.widget',
    'dashboardDynamic/widgets/ddDzRoom.widget',
    'dashboardDynamic/widgets/ddHtmlWidget.widget',
    'dashboardDynamic/widgets/ddKwhSummary.widget',
    'dashboardDynamic/widgets/ddGasSummary.widget',
    'dashboardDynamic/widgets/ddP1Electricity.widget',
    'dashboardDynamic/widgets/ddTextSensor.widget',
    'dashboardDynamic/widgets/ddBatteryStatus.widget',
    'dashboardDynamic/widgets/ddSetpoint.widget',
    'dashboardDynamic/widgets/ddComboThermostat.widget',
    'dashboardDynamic/widgets/ddThermostat6.widget',
    'dashboardDynamic/widgets/ddRssFeed.widget',
    'dashboardDynamic/widgets/ddWeatherForecast.widget',
    'dashboardDynamic/widgets/ddDomoticzLog.widget',
    'dashboardDynamic/widgets/ddMoonPhase.widget',
    'dashboardDynamic/widgets/ddGoogleCalendar.widget',
    'dashboardDynamic/widgets/ddEnergyDashboard.widget',
    'dashboardDynamic/widgets/ddSelfSufficiency.widget',
    'dashboardDynamic/widgets/ddCustomChart.widget',
    'dashboardDynamic/widgets/ddGauge.widget',
    'dashboardDynamic/widgets/ddBatteryMonitor.widget'
], function(app) {
    'use strict';

    app.controller('DashboardDynamicController', [
        '$scope', '$timeout', '$interval', '$document', '$location', '$uibModal', '$q', '$http',
        'dashboardDynamicService', 'widgetRegistry', 'ddToast', 'bootbox',
        function($scope, $timeout, $interval, $document, $location, $uibModal, $q, $http,
                 dashboardDynamicService, widgetRegistry, ddToast, bootbox) {

        // One-time migration of legacy localStorage keys
        try {
            ['db2_kiosk', 'db2_standby', 'db2_last_layout'].forEach(function(oldKey) {
                var newKey = oldKey.replace('db2_', 'dd_');
                var val = localStorage.getItem(oldKey);
                if (val !== null) {
                    localStorage.setItem(newKey, val);
                    localStorage.removeItem(oldKey);
                }
            });
        } catch(e) {}

        // ── State ──────────────────────────────────────────────
        $scope.loading          = true;
        $scope.editMode         = false;
        $scope.showLibrary      = false;
        $scope.isDirty          = false;
        $scope.isFullPage       = false;
        $scope.layouts          = [];    // list of layout metadata objects
        $scope.activeLayout      = null; // { id, name, isDefault }
        $scope.activeData        = null; // { version, columns, rowHeight, widgets: [] }
        $scope.error            = null;
        $scope.ddToast         = ddToast;
        $scope.gridReady        = false; // toggled false→true to destroy/recreate the grid on layout change
        var _savedDataSnapshot  = null;  // deep copy taken when entering edit mode
        var _body               = document.body;

        // ── Kiosk state ────────────────────────────────────────
        var LS_KIOSK = 'dd_kiosk';
        var _kioskDefaults = { enabled: false, layoutIds: [], interval: 30, loop: true };
        try {
            $scope.kiosk = angular.extend({}, _kioskDefaults, JSON.parse(localStorage.getItem(LS_KIOSK) || '{}'));
        } catch(e) {
            $scope.kiosk = angular.copy(_kioskDefaults);
        }
        $scope.kioskActive   = false;
        $scope.kioskProgress = 0;
        var _kioskTickTimer  = null;
        var _kioskLayouts    = [];
        var _kioskIndex      = 0;
        var _kioskElapsed    = 0;

        // ── Standby state ───────────────────────────────────────
        var LS_STANDBY = 'dd_standby';
        var _standbyDefaults = { enabled: false, timeout: 5, opacity: 5, blackout: false };
        try {
            $scope.standby = angular.extend({}, _standbyDefaults, JSON.parse(localStorage.getItem(LS_STANDBY) || '{}'));
        } catch(e) {
            $scope.standby = angular.copy(_standbyDefaults);
        }
        $scope.standbyActive = false;
        var _standbyTimer    = null;

        // Cycle gridReady false→true so ng-if fully destroys and recreates the grid,
        // ensuring compiled widget cells always reference the current activeData.
        function refreshGrid() {
            $scope.gridReady = false;
            $timeout(function() { $scope.gridReady = true; });
        }

        // ── Standby functions ──────────────────────────────────
        function enterStandby() {
            $scope.$apply(function() { $scope.standbyActive = true; });
            var opacity = $scope.standby.blackout ? 0 : ($scope.standby.opacity / 100);
            _body.style.setProperty('--dd-standby-opacity', opacity);
            _body.classList.add('dd-standby');
        }

        function exitStandby() {
            _body.classList.remove('dd-standby');
            $scope.$apply(function() { $scope.standbyActive = false; });
            resetStandbyTimer();
        }

        function resetStandbyTimer() {
            if (_standbyTimer) {
                $timeout.cancel(_standbyTimer);
                _standbyTimer = null;
            }
            if (!$scope.standby.enabled) { return; }
            if ($scope.standbyActive) {
                exitStandby();
                return;
            }
            var ms = Math.max(1, parseInt($scope.standby.timeout, 10) || 5) * 60000;
            _standbyTimer = $timeout(enterStandby, ms);
        }

        $scope.saveStandbySettings = function(patch) {
            angular.extend($scope.standby, patch);
            try { localStorage.setItem(LS_STANDBY, JSON.stringify($scope.standby)); } catch(e) {}
            if (_standbyTimer) {
                $timeout.cancel(_standbyTimer);
                _standbyTimer = null;
            }
            if ($scope.standbyActive) {
                _body.classList.remove('dd-standby');
                $scope.standbyActive = false;
            }
            if ($scope.standby.enabled) {
                var ms = Math.max(1, parseInt($scope.standby.timeout, 10) || 5) * 60000;
                _standbyTimer = $timeout(enterStandby, ms);
            }
        };

        // ── Private helpers ────────────────────────────────────
        function loadLayout(id) {
            $scope.loading = true;
            $scope.error   = null;
            return dashboardDynamicService.loadLayout(id).then(function(full) {
                $scope.activeLayout = {
                    id:        full.id,
                    name:      full.name,
                    isDefault: full.isDefault
                };
                $scope.activeData = full.layout || { version: 1, widgets: [] };
                refreshGrid();
                $scope.loading = false;
            }).catch(function(err) {
                $scope.error   = err;
                $scope.loading = false;
            });
        }

        // ── Starter layout ─────────────────────────────────────
        function createStarterLayout() {
            var meta = { id: dashboardDynamicService.generateId(), name: 'Dashboard', isDefault: true };
            var data = {
                version:   1,
                columns:   12,
                rowHeight: 60,
                margin:    8,
                animate:   true,
                widgets:   []
            };
            return $http.get('json.htm?type=command&param=getdevices&filter=all&used=true&favorite=1&order=%5BOrder%5D')
                .then(function(resp) {
                    data.widgets = buildFavoritesWidgets(resp.data.result || []);
                })
                .catch(function() {
                    data.widgets = [];
                })
                .then(function() {
                    return dashboardDynamicService.saveLayout(meta, data).then(function() {
                        $scope.layouts      = [meta];
                        $scope.activeLayout = meta;
                        $scope.activeData   = data;
                        refreshGrid();
                        $scope.loading      = false;
                    });
                });
        }

        // ── Lifecycle ──────────────────────────────────────────
        var LS_KEY = 'dd_last_layout';

        function init() {
            $scope.loading = true;
            dashboardDynamicService.listLayouts().then(function(layouts) {
                $scope.layouts = layouts;
                if (layouts.length === 0) {
                    return createStarterLayout();
                }
                // Prefer the layout the user was last viewing (stored in localStorage)
                var lastId = null;
                try { lastId = localStorage.getItem(LS_KEY); } catch(e) {}
                var startLayout = (lastId && layouts.find(function(l) { return l.id === lastId; })) ||
                                  layouts.find(function(l) { return l.isDefault; }) ||
                                  layouts[0];
                return loadLayout(startLayout.id).then(function() {
                    if ($scope.kiosk.enabled && $scope.layouts.length >= 2) {
                        $timeout(function() { $scope.startKiosk(); }, 0);
                    }
                    if ($scope.standby.enabled) {
                        resetStandbyTimer();
                    }
                });
            }).catch(function(err) {
                $scope.error   = 'Failed to load dashboard';
                $scope.loading = false;
            });
        }

        // ── Public actions ─────────────────────────────────────
        $scope.switchLayout = function(id) {
            try { localStorage.setItem(LS_KEY, id); } catch(e) {}
            loadLayout(id);
            resetStandbyTimer();
        };

        $scope.toggleEditMode = function() {
            if (!$scope.editMode) {
                // Entering edit mode — take a snapshot for cancel
                _savedDataSnapshot = angular.copy($scope.activeData);
                $scope.editMode = true;
            } else {
                // Exiting edit mode — save if there are unsaved changes
                $scope.showLibrary = false;
                if ($scope.isDirty) {
                    $scope.saveCurrentLayout().then(function() {
                        _savedDataSnapshot = null;
                        $scope.editMode = false;
                    });
                } else {
                    _savedDataSnapshot = null;
                    $scope.editMode = false;
                }
            }
        };

        $scope.cancelEdit = function() {
            if (_savedDataSnapshot) {
                $scope.activeData = angular.copy(_savedDataSnapshot);
                _savedDataSnapshot = null;
            }
            $scope.editMode    = false;
            $scope.showLibrary = false;
            $scope.isDirty     = false;
            // Force the grid to re-render with the restored data
            refreshGrid();
        };

        $scope.toggleLibrary = function() {
            $scope.showLibrary = !$scope.showLibrary;
        };

        var autoSaveTimeout = null;

        $scope.onGridChange = function() {
            $scope.isDirty = true;
        };

        $scope.configureWidget = function(id) {
            var widget     = findWidget(id);
            if (!widget) { return; }
            var descriptor = widgetRegistry.get(widget.type);
            if (!descriptor || !descriptor.configSchema || !descriptor.configSchema.length) { return; }

            $uibModal.open({
                templateUrl: 'views/dashboardDynamic/widget-settings-modal.html',
                controller:  'DdWidgetSettingsCtrl',
                resolve: {
                    widget:     function() { return widget; },
                    descriptor: function() { return descriptor; }
                }
            }).result.then(function(newConfig) {
                // Mutate config in-place so directives watching config properties detect the change
                angular.copy(newConfig, widget.config);
                $scope.isDirty = true;
            });
        };

        function findWidget(id) {
            return $scope.activeData && $scope.activeData.widgets &&
                   $scope.activeData.widgets.find(function(w) { return w.id === id; });
        }

        // All widgets are pre-loaded via RequireJS deps — snapshot once at init.
        // Using a live getter would return a new object every digest → infinite loop.
        $scope.widgetCatalogGrouped = widgetRegistry.getGrouped();

        $scope.saveCurrentLayout = function() {
            if (!$scope.activeLayout || !$scope.activeData) { return $q.when(); }
            return dashboardDynamicService.saveLayout($scope.activeLayout, $scope.activeData)
                .then(function() {
                    $scope.isDirty = false;
                    ddToast.success('Dashboard saved');
                })
                .catch(function(err) {
                    ddToast.error('Save failed: ' + (err || 'Unknown error'));
                });
        };

        $scope.isEmptyObject = function(obj) {
            return !obj || Object.keys(obj).length === 0;
        };

        // ── Inline title rename ────────────────────────────────
        $scope.editingTitle    = false;
        $scope.titleEditValue  = '';

        $scope.startTitleEdit = function() {
            if (!$scope.activeLayout) { return; }
            $scope.titleEditValue = $scope.activeLayout.name;
            $scope.editingTitle   = true;
        };

        $scope.finishTitleEdit = function() {
            var name = ($scope.titleEditValue || '').trim();
            if (name && $scope.activeLayout) {
                $scope.activeLayout.name = name;
                dashboardDynamicService.saveLayout($scope.activeLayout, null)
                    .catch(function() { ddToast.error('Rename failed'); });
            }
            $scope.editingTitle = false;
        };

        $scope.cancelTitleEdit = function() {
            $scope.editingTitle = false;
        };

        $scope.openDashboardManager = function() {
            $uibModal.open({
                templateUrl: 'views/dashboardDynamic/dashboard-manager-modal.html',
                controller:  'DdDashboardManagerCtrl',
                size:        'lg',
                resolve: {
                    layouts:         function() { return $scope.layouts; },
                    currentId:       function() { return $scope.activeLayout && $scope.activeLayout.id; },
                    onSwitch:        function() { return function(id) { $scope.switchLayout(id); }; },
                    kioskSettings:   function() { return $scope.kiosk; },
                    onKioskChange:   function() {
                        return function(settings) { $scope.saveKioskSettings(settings); };
                    },
                    standbySettings: function() { return $scope.standby; },
                    onStandbyChange: function() {
                        return function(settings) { $scope.saveStandbySettings(settings); };
                    }
                }
            }).result.then(function() {
                // Refresh layout list after the manager closes
                return dashboardDynamicService.listLayouts().then(function(l) {
                    $scope.layouts = l;
                });
            }).catch(angular.noop); // dismiss is not an error
        };

        // ── Export / Import ───────────────────────────────────
        $scope.openExportImport = function(startTab) {
            $uibModal.open({
                templateUrl: 'views/dashboardDynamic/export-import-modal.html',
                controller:  'DdExportImportCtrl',
                size:        'md',
                resolve: {
                    activeLayout: function() { return $scope.activeLayout; },
                    activeData:   function() { return $scope.activeData; },
                    layouts:      function() { return $scope.layouts; },
                    onImported:   function() {
                        return function(result) {
                            if (result.replace) {
                                return dashboardDynamicService.saveLayout(result.meta, result.data).then(function() {
                                    $scope.activeData = result.data;
                                    refreshGrid();
                                    ddToast.success('Dashboard replaced');
                                });
                            } else {
                                return dashboardDynamicService.saveLayout(result.meta, result.data).then(function() {
                                    $scope.layouts.push(result.meta);
                                    $scope.activeLayout = result.meta;
                                    $scope.activeData   = result.data;
                                    refreshGrid();
                                    ddToast.success('Imported: ' + result.meta.name);
                                });
                            }
                        };
                    }
                }
            }).result.catch(angular.noop);
        };

        // ── Full-page mode (manual navbar toggle) ────────────
        $scope.toggleFullPage = function() {
            $scope.isFullPage = !$scope.isFullPage;
            if ($scope.isFullPage) {
                _body.classList.add('dd-navbar-hidden');
            } else {
                _body.classList.remove('dd-navbar-hidden');
            }
        };

        // ── Keyboard shortcuts ────────────────────────────────
        function onKeyDown(e) {
            // Escape: stop kiosk if active, otherwise exit edit mode
            // Guard: if a modal is open let it handle its own Escape key
            if (e.keyCode === 27 && $('.modal.in').length) { return; }
            if (e.keyCode === 27 && $scope.kioskActive) {
                $scope.$apply(function() { $scope.stopKiosk(); });
                return;
            }
            if (e.keyCode === 27 && $scope.editMode) {
                $scope.$apply(function() { $scope.toggleEditMode(); });
            }
            // Ctrl+S: save layout
            if (e.ctrlKey && e.keyCode === 83 && $scope.editMode) {
                e.preventDefault();
                $scope.$apply(function() { $scope.saveCurrentLayout(); });
            }
            // Ctrl+E: toggle edit mode
            if (e.ctrlKey && e.keyCode === 69) {
                e.preventDefault();
                $scope.$apply(function() { $scope.toggleEditMode(); });
            }
        }
        document.addEventListener('keydown', onKeyDown);

        // ── Standby activity listeners ────────────────────────
        var _standbyActivityEvents = ['mousemove', 'touchstart', 'keydown', 'click'];
        _standbyActivityEvents.forEach(function(ev) {
            document.addEventListener(ev, resetStandbyTimer, { passive: true });
        });

        // ── Clear all widgets ──────────────────────────────────
        $scope.clearAllWidgets = function() {
            if (!$scope.activeData || !$scope.activeData.widgets.length) { return; }
            bootbox.confirm('Remove all widgets from this dashboard?').then(function() {
                $scope.activeData.widgets = [];
                $scope.isDirty = true;
                refreshGrid();
            }).catch(angular.noop);
        };

        // ── Reset to favorites layout ─────────────────────────
        function categorizeFavorites(devices) {
            var result = { lights: [], temperature: [], weather: [], utility: [] };
            devices.forEach(function(d) {
                // Skip scenes/groups — handled separately via getscenes
                if (d.Type.indexOf('Scene') === 0 || d.Type.indexOf('Group') === 0) { return; }

                if (d.Type.indexOf('Light')        === 0 || d.Type.indexOf('Security') === 0 ||
                    d.Type.indexOf('Blind')        === 0 || d.Type.indexOf('Curtain')  === 0 ||
                    d.Type.indexOf('Color Switch') === 0 || d.Type.indexOf('Chime')    === 0 ||
                    d.Type.indexOf('Thermostat')   === 0 || d.Type.indexOf('Heating')  === 0 ||
                    d.Type.indexOf('ASA')          === 0 || d.Type.indexOf('Fan')      === 0 ||
                    d.SubType === 'Smartwares Mode' || d.SubType === 'Relay' ||
                    (d.SubType && (d.SubType.indexOf('Itho') === 0 || d.SubType.indexOf('Lucci') === 0 ||
                                   d.SubType.indexOf('Westinghouse') === 0 || d.SubType.indexOf('Falmec') === 0)) ||
                    (d.Type.indexOf('Value') === 0 && typeof d.SwitchType !== 'undefined')
                ) {
                    result.lights.push(d);
                } else if (typeof d.Temp !== 'undefined' || typeof d.Humidity !== 'undefined' || typeof d.Chill !== 'undefined') {
                    result.temperature.push(d);
                    if (typeof d.Rain !== 'undefined' || typeof d.Visibility !== 'undefined' ||
                        typeof d.UVI  !== 'undefined' || typeof d.Radiation  !== 'undefined' ||
                        typeof d.Direction !== 'undefined' || typeof d.Barometer !== 'undefined') {
                        result.weather.push(d);
                    }
                } else if (typeof d.Rain !== 'undefined' || typeof d.Visibility !== 'undefined' ||
                           typeof d.UVI  !== 'undefined' || typeof d.Radiation  !== 'undefined' ||
                           typeof d.Direction !== 'undefined' || typeof d.Barometer !== 'undefined') {
                    result.weather.push(d);
                } else if (
                    d.Type === 'Lux'          || d.Type === 'Air Quality'  || d.Type === 'Counter'  ||
                    d.Type === 'Current'      || d.Type === 'Energy'       || d.Type === 'Current/Energy' ||
                    d.Type === 'Power'        || d.Type === 'Gas'          || d.Type === 'Water'    ||
                    d.Type === 'Weight'       || d.Type === 'Usage'        || d.Type === 'Radiator 1' ||
                    d.SubType === 'kWh'       || d.SubType === 'Percentage' || d.SubType === 'Voltage' ||
                    d.SubType === 'Distance'  || d.SubType === 'Current'   || d.SubType === 'Text'  ||
                    d.SubType === 'Alert'     || d.SubType === 'Pressure'  || d.SubType === 'A/D'   ||
                    d.SubType === 'Thermostat Mode' || d.SubType === 'Thermostat Fan Mode' ||
                    d.SubType === 'Fan'       || d.SubType === 'Smartwares' || d.SubType === 'Waterflow' ||
                    d.SubType === 'Sound Level' || d.SubType === 'Custom Sensor' ||
                    d.SubType === 'Thermostat Clock' || d.SubType === 'Soil Moisture' ||
                    d.SubType === 'Leaf Wetness' ||
                    (d.Type === 'Setpoint' && d.SubType === 'SetPoint')
                ) {
                    result.utility.push(d);
                }
            });
            return result;
        }

        function buildFavoritesWidgets(all) {
            var cats    = categorizeFavorites(all);
            var scenes  = all.filter(function(d) {
                return d.Type.indexOf('Scene') === 0 || d.Type.indexOf('Group') === 0;
            });
            var widgets = [];
            var y       = 0;
            var COLS    = 4, W = 3, H = 2, HEADER_H = 1;

            function addSection(label, items, widgetType, configKey) {
                widgetType = widgetType || 'dz-device';
                configKey  = configKey  || 'deviceIdx';
                if (!items.length) { return; }
                widgets.push({
                    id: dashboardDynamicService.generateId(), type: 'text-note',
                    x: 0, y: y, w: 12, h: HEADER_H, minH: 1,
                    config: { content: label, fontSize: 16, textAlign: 'left' }
                });
                y += HEADER_H;
                items.forEach(function(item, i) {
                    var cfg = {};
                    cfg[configKey] = String(item.idx);
                    widgets.push({
                        id: dashboardDynamicService.generateId(), type: widgetType,
                        x: (i % COLS) * W, y: y + Math.floor(i / COLS) * H,
                        w: W, h: H, config: cfg
                    });
                });
                y += Math.ceil(items.length / COLS) * H;
            }

            addSection('Scenes:',          scenes,  'dz-scene', 'sceneIdx');
            addSection('Lights/Switches:', cats.lights);
            addSection('Temperature:',     cats.temperature);
            addSection('Weather:',         cats.weather);
            addSection('Utility:',         cats.utility);

            return widgets;
        }

        $scope.resetToFavorites = function() {
            bootbox.confirm('Replace all widgets with the favorites layout?').then(function() {
                $http.get('json.htm?type=command&param=getdevices&filter=all&used=true&favorite=1&order=%5BOrder%5D')
                    .then(function(resp) {
                        $scope.activeData.widgets = buildFavoritesWidgets(resp.data.result || []);
                        $scope.isDirty = true;
                        refreshGrid();
                    })
                    .catch(function() {
                        bootbox.alert('Failed to load favorites. Please try again.');
                    });
            }).catch(angular.noop);
        };

        $scope.duplicateLayout = function() {
            if (!$scope.activeLayout || !$scope.activeData) { return; }
            var newMeta = {
                id:        dashboardDynamicService.generateId(),
                name:      $scope.activeLayout.name + ' (copy)',
                isDefault: false
            };
            var newData = angular.copy($scope.activeData);
            // Assign fresh widget IDs to avoid ID collisions
            newData.widgets.forEach(function(w) {
                w.id = dashboardDynamicService.generateId();
            });
            dashboardDynamicService.saveLayout(newMeta, newData).then(function() {
                $scope.layouts.push(newMeta);
                $scope.activeLayout = newMeta;
                $scope.activeData   = newData;
                $scope.isDirty      = false;
                try { localStorage.setItem(LS_KEY, newMeta.id); } catch(e) {}
                refreshGrid();
                ddToast.success('Dashboard duplicated');
            }).catch(function() {
                ddToast.error('Duplicate failed');
            });
        };

        $scope.deleteCurrentLayout = function() {
            if (!$scope.activeLayout) { return; }
            if ($scope.layouts.length <= 1) {
                bootbox.alert('Cannot delete the only dashboard.');
                return;
            }
            bootbox.confirm('Delete "' + $scope.activeLayout.name + '"? This cannot be undone.').then(function() {
                var idToDelete = $scope.activeLayout.id;
                dashboardDynamicService.deleteLayout(idToDelete).then(function() {
                    var idx = $scope.layouts.findIndex(function(l) { return l.id === idToDelete; });
                    $scope.layouts.splice(idx, 1);
                    var next = $scope.layouts[Math.max(0, idx - 1)];
                    $scope.switchLayout(next.id);
                    ddToast.success('Dashboard deleted');
                }).catch(function() {
                    ddToast.error('Delete failed');
                });
            }).catch(angular.noop);
        };

        // ── Kiosk functions ────────────────────────────────────
        $scope.saveKioskSettings = function(settings) {
            angular.extend($scope.kiosk, settings);
            try { localStorage.setItem(LS_KIOSK, JSON.stringify($scope.kiosk)); } catch(e) {}
        };

        $scope.startKiosk = function() {
            if ($scope.kioskActive) { return; }

            var ids = $scope.kiosk.layoutIds && $scope.kiosk.layoutIds.length
                ? $scope.layouts.filter(function(l) { return $scope.kiosk.layoutIds.indexOf(l.id) !== -1; })
                : $scope.layouts.slice();

            if (ids.length < 2) { return; }
            _kioskLayouts = ids;

            // Position index to the current layout
            var curIdx = _kioskLayouts.findIndex(function(l) {
                return $scope.activeLayout && l.id === $scope.activeLayout.id;
            });
            _kioskIndex   = curIdx !== -1 ? curIdx : 0;
            _kioskElapsed = 0;
            $scope.kioskProgress = 0;
            $scope.kioskActive   = true;

            var intervalSec = Math.max(5, parseInt($scope.kiosk.interval, 10) || 30);

            // Tick every second to update progress
            _kioskTickTimer = $interval(function() {
                _kioskElapsed++;
                $scope.kioskProgress = Math.min(100, Math.round((_kioskElapsed / intervalSec) * 100));

                if (_kioskElapsed >= intervalSec) {
                    _kioskElapsed = 0;
                    $scope.kioskProgress = 0;
                    _kioskIndex++;
                    if (_kioskIndex >= _kioskLayouts.length) {
                        if ($scope.kiosk.loop) {
                            _kioskIndex = 0;
                        } else {
                            $scope.stopKiosk();
                            return;
                        }
                    }
                    $scope.switchLayout(_kioskLayouts[_kioskIndex].id);
                }
            }, 1000);
        };

        $scope.stopKiosk = function() {
            if (_kioskTickTimer) {
                $interval.cancel(_kioskTickTimer);
                _kioskTickTimer = null;
            }
            $scope.kioskActive   = false;
            $scope.kioskProgress = 0;
            _kioskElapsed        = 0;
        };

        $scope.toggleKiosk = function() {
            if ($scope.kioskActive) {
                $scope.stopKiosk();
            } else {
                $scope.startKiosk();
            }
        };

        // ── Cleanup ────────────────────────────────────────────
        $scope.$on('$destroy', function() {
            document.removeEventListener('keydown', onKeyDown);
            _standbyActivityEvents.forEach(function(ev) {
                document.removeEventListener(ev, resetStandbyTimer);
            });
            if (_standbyTimer) { $timeout.cancel(_standbyTimer); }
            _body.classList.remove('dd-navbar-hidden');
            _body.classList.remove('dd-standby');
            $scope.stopKiosk();
        });

        // ── Init ───────────────────────────────────────────────
        init();
    }]);
});
