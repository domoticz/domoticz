define([
    'app',
    'dashboard2/dashboard2.module',
    'dashboard2/dashboard2Service',
    'dashboard2/widgetRegistry.service',
    'dashboard2/db2Toast.service',
    'dashboard2/db2WidgetContent.directive',
    'dashboard2/db2WidgetWrapper',
    'dashboard2/db2Grid',
    'dashboard2/db2WidgetSettings.controller',
    'dashboard2/db2DashboardManager.controller',
    'dashboard2/db2ExportImport.controller',
    'widgets/dzLightWidget',
    'widgets/dzUtilityWidget',
    'widgets/dzSceneWidget',
    'dashboard2/widgets/db2Clock.widget',
    'dashboard2/widgets/db2SunInfo.widget',
    'dashboard2/widgets/db2ActivityLog.widget',
    'dashboard2/widgets/db2SystemStatus.widget',
    'dashboard2/widgets/db2TextNote.widget',
    'dashboard2/widgets/db2QuickActions.widget',
    'dashboard2/widgets/db2WeatherWidget.widget',
    'dashboard2/widgets/db2StatCounter.widget',
    'dashboard2/widgets/db2TemperatureGraph.widget',
    'dashboard2/widgets/db2EnergyChart.widget',
    'dashboard2/widgets/db2IframeEmbed.widget',
    'dashboard2/widgets/db2ImageWidget.widget',
    'dashboard2/widgets/db2CameraFeed.widget',
    'dashboard2/widgets/db2DzDevice.widget',
    'dashboard2/widgets/db2DzScene.widget',
    'dashboard2/widgets/db2DzFavorites.widget',
    'dashboard2/widgets/db2DzRoom.widget',
    'dashboard2/widgets/db2HtmlWidget.widget'
], function(app) {
    'use strict';

    app.controller('Dashboard2Controller', [
        '$scope', '$timeout', '$location', '$uibModal', '$q',
        'dashboard2Service', 'widgetRegistry', 'db2Toast',
        function($scope, $timeout, $location, $uibModal, $q,
                 dashboard2Service, widgetRegistry, db2Toast) {

        // ── State ──────────────────────────────────────────────
        $scope.loading          = true;
        $scope.editMode         = false;
        $scope.showLibrary      = false;
        $scope.isDirty          = false;
        $scope.layouts          = [];    // list of layout metadata objects
        $scope.activeLayout      = null; // { id, name, isDefault }
        $scope.activeData        = null; // { version, columns, rowHeight, widgets: [] }
        $scope.error            = null;
        $scope.db2Toast         = db2Toast;
        $scope.gridVersion      = 0;     // incrementing this forces ng-if to destroy/recreate the grid
        var _savedDataSnapshot  = null;  // deep copy taken when entering edit mode

        // ── Private helpers ────────────────────────────────────
        function loadLayout(id) {
            $scope.loading = true;
            $scope.error   = null;
            return dashboard2Service.loadLayout(id).then(function(full) {
                $scope.activeLayout = {
                    id:        full.id,
                    name:      full.name,
                    isDefault: full.isDefault
                };
                $scope.activeData = full.layout || { version: 1, widgets: [] };
                $scope.gridVersion++;
                $scope.loading = false;
            }).catch(function(err) {
                $scope.error   = err;
                $scope.loading = false;
            });
        }

        // ── Starter layout ─────────────────────────────────────
        function createStarterLayout() {
            var meta = { id: dashboard2Service.generateId(), name: 'Dashboard', isDefault: true };
            var data = {
                version:   1,
                columns:   12,
                rowHeight: 60,
                margin:    8,
                animate:   true,
                widgets: [
                    { id: dashboard2Service.generateId(), type: 'clock',        x: 0, y: 0, w: 2, h: 2, config: {} },
                    { id: dashboard2Service.generateId(), type: 'sun-info',     x: 2, y: 0, w: 2, h: 2, config: {} },
                    { id: dashboard2Service.generateId(), type: 'activity-log', x: 0, y: 2, w: 6, h: 4, config: {} }
                ]
            };
            return dashboard2Service.saveLayout(meta, data).then(function() {
                $scope.layouts = [meta];
                $scope.activeLayout = meta;
                $scope.activeData   = data;
                $scope.loading = false;
            });
        }

        // ── Lifecycle ──────────────────────────────────────────
        function init() {
            $scope.loading = true;
            dashboard2Service.listLayouts().then(function(layouts) {
                $scope.layouts = layouts;
                if (layouts.length === 0) {
                    return createStarterLayout();
                }
                var defaultLayout = layouts.find(function(l) { return l.isDefault; }) || layouts[0];
                return loadLayout(defaultLayout.id);
            }).catch(function(err) {
                $scope.error   = 'Failed to load dashboard';
                $scope.loading = false;
            });
        }

        // ── Public actions ─────────────────────────────────────
        $scope.switchLayout = function(id) {
            loadLayout(id);
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
            $scope.gridVersion++;
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
                templateUrl: 'views/dashboard2/widget-settings-modal.html',
                controller:  'Db2WidgetSettingsCtrl',
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

        $scope.savedFlash = false;

        $scope.saveCurrentLayout = function() {
            if (!$scope.activeLayout || !$scope.activeData) { return $q.when(); }
            return dashboard2Service.saveLayout($scope.activeLayout, $scope.activeData)
                .then(function() {
                    $scope.isDirty = false;
                    $scope.savedFlash = true;
                    $timeout(function() { $scope.savedFlash = false; }, 2000);
                    db2Toast.success('Dashboard saved');
                })
                .catch(function(err) {
                    db2Toast.error('Save failed: ' + (err || 'Unknown error'));
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
                dashboard2Service.saveLayout($scope.activeLayout, null)
                    .catch(function() { db2Toast.error('Rename failed'); });
            }
            $scope.editingTitle = false;
        };

        $scope.cancelTitleEdit = function() {
            $scope.editingTitle = false;
        };

        $scope.openDashboardManager = function() {
            $uibModal.open({
                templateUrl: 'views/dashboard2/dashboard-manager-modal.html',
                controller:  'Db2DashboardManagerCtrl',
                size:        'lg',
                resolve: {
                    layouts:   function() { return $scope.layouts; },
                    currentId: function() { return $scope.activeLayout && $scope.activeLayout.id; },
                    onSwitch:  function() { return function(id) { loadLayout(id); }; }
                }
            }).result.then(function() {
                // Refresh layout list after the manager closes
                return dashboard2Service.listLayouts().then(function(l) {
                    $scope.layouts = l;
                });
            }).catch(angular.noop); // dismiss is not an error
        };

        // ── Export / Import ───────────────────────────────────
        $scope.openExportImport = function(startTab) {
            $uibModal.open({
                templateUrl: 'views/dashboard2/export-import-modal.html',
                controller:  'Db2ExportImportCtrl',
                size:        'md',
                resolve: {
                    activeLayout: function() { return $scope.activeLayout; },
                    activeData:   function() { return $scope.activeData; },
                    layouts:      function() { return $scope.layouts; },
                    onImported:   function() {
                        return function(result) {
                            if (result.replace) {
                                return dashboard2Service.saveLayout(result.meta, result.data).then(function() {
                                    $scope.activeData = result.data;
                                    $scope.gridVersion++;
                                    db2Toast.success('Dashboard replaced');
                                });
                            } else {
                                return dashboard2Service.saveLayout(result.meta, result.data).then(function() {
                                    $scope.layouts.push(result.meta);
                                    $scope.activeLayout = result.meta;
                                    $scope.activeData   = result.data;
                                    $scope.gridVersion++;
                                    db2Toast.success('Imported: ' + result.meta.name);
                                });
                            }
                        };
                    }
                }
            }).catch(angular.noop);
        };

        // ── Keyboard shortcuts ────────────────────────────────
        function onKeyDown(e) {
            // Escape: exit edit mode
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

        // ── Cleanup ────────────────────────────────────────────
        $scope.$on('$destroy', function() {
            document.removeEventListener('keydown', onKeyDown);
        });

        // ── Init ───────────────────────────────────────────────
        init();
    }]);
});
