define([
    'app',
    'dashboardDynamic/dashboardDynamic.module',
    'dashboardDynamic/dashboardDynamicService',
    'dashboardDynamic/widgetRegistry.service',
    'dashboardDynamic/ddToast.service',
    'dashboardDynamic/ddSparkline.service',
    'dashboardDynamic/ddVisibility.service',
    'dashboardDynamic/ddWidgetContent.directive',
    'dashboardDynamic/ddWidgetWrapper',
    'dashboardDynamic/ddGrid',
    'dashboardDynamic/ddWidgetSettings.controller',
    'dashboardDynamic/ddWcpColor.directive',
    'dashboardDynamic/ddStatBar.directive',
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
    'dashboardDynamic/widgets/ddBaro.widget',
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
    'dashboardDynamic/widgets/ddEnergyWeather.widget',
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
    'dashboardDynamic/widgets/ddDial.widget',
    'dashboardDynamic/widgets/ddGauge.widget',
    'dashboardDynamic/widgets/ddBatteryMonitor.widget',
    'dashboardDynamic/widgets/ddTimeoutMonitor.widget',
    'dashboardDynamic/widgets/ddKwhTopConsumers.widget',
    'dashboardDynamic/widgets/ddWaterSummary.widget',
    'dashboardDynamic/widgets/ddQuickStat.widget'
], function(app) {
    'use strict';

    app.controller('DashboardDynamicController', [
        '$scope', '$timeout', '$interval', '$document', '$location', '$route', '$uibModal', '$q', '$http',
        'dashboardDynamicService', 'widgetRegistry', 'ddToast', 'bootbox', 'livesocket',
        function($scope, $timeout, $interval, $document, $location, $route, $uibModal, $q, $http,
                 dashboardDynamicService, widgetRegistry, ddToast, bootbox, livesocket) {

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
        $scope.roomPlans        = [];    // list of Domoticz room plans
        $scope.activeLayout      = null; // { id, name, isDefault }
        $scope.activeData        = null; // { version, columns, rowHeight, widgets: [] }
        $scope.error            = null;
        $scope.ddToast         = ddToast;
        $scope.gridReady        = false; // toggled false→true to destroy/recreate the grid on layout change
        var _savedDataSnapshot  = null;  // deep copy taken when entering edit mode
        var _body               = document.body;

        // ── Kiosk state ────────────────────────────────────────
        var LS_KIOSK     = 'dd_kiosk';
        var LS_FULLPAGE  = 'dd_fullpage';
        var _kioskDefaults = { enabled: false, layoutIds: [], interval: 30, loop: true };
        try {
            $scope.kiosk = angular.extend({}, _kioskDefaults, JSON.parse(localStorage.getItem(LS_KIOSK) || '{}'));
        } catch(e) {
            $scope.kiosk = angular.copy(_kioskDefaults);
        }
        $scope.uppercaseLabels = localStorage.getItem('dd_uppercase_labels') === '1';
        $scope.kioskActive   = false;
        $scope.kioskProgress = 0;
        var _kioskTickTimer  = null;
        var _kioskLayouts    = [];
        var _kioskIndex      = 0;
        var _kioskElapsed    = 0;
        var _kioskWasActive  = false;

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

        // ── Swipe navigation state ─────────────────────────────
        // Lets the user swipe left/right (e.g. on a tablet) to move to the
        // next/previous dashboard, mirroring the floorplan swipe gesture.
        var LS_SWIPE = 'dd_swipe';
        var _swipeDefaults = { enabled: false };
        try {
            $scope.swipe = angular.extend({}, _swipeDefaults, JSON.parse(localStorage.getItem(LS_SWIPE) || '{}'));
        } catch(e) {
            $scope.swipe = angular.copy(_swipeDefaults);
        }
        var _swipeStartX = 0;
        var _swipeStartY = 0;
        var _swipeMoved  = false;

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

        $scope.toggleUppercaseLabels = function() {
            $scope.uppercaseLabels = !$scope.uppercaseLabels;
            if ($scope.uppercaseLabels) {
                localStorage.setItem('dd_uppercase_labels', '1');
            } else {
                localStorage.removeItem('dd_uppercase_labels');
            }
        };

        // ── Private helpers ────────────────────────────────────
        // Another (saved) dashboard became active while in edit mode: Cancel must restore
        // that dashboard, not the snapshot of the one that was active when edit mode started.
        function resetEditSnapshot() {
            if (!$scope.editMode) { return; }
            _savedDataSnapshot = angular.copy($scope.activeData);
            $scope.isDirty = false;
            $scope.designPanel.open = false;
        }

        function loadLayout(id) {
            $scope.loading = true;
            $scope.error   = null;
            return dashboardDynamicService.loadLayout(id).then(function(full) {
                $scope.activeLayout = {
                    id:        full.id,
                    name:      full.name,
                    isDefault: full.isDefault
                };
                $scope.activeData = full.layout || dashboardDynamicService.migrateLayout({ version: 1, widgets: [] });
                resetEditSnapshot();
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
                version:   dashboardDynamicService.LAYOUT_VERSION,
                columns:   dashboardDynamicService.GRID_COLUMNS,
                rowHeight: 60,
                margin:    8,
                animate:   true,
                widgets:   []
            };
            return $http.get('json.htm?type=command&param=getdevices&filter=all&used=true&favorite=1&order=%5BOrder%5D')
                .then(function(resp) {
                    data.widgets = dashboardDynamicService.buildFavoritesWidgets(resp.data.result || []);
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
            $http.get('json.htm', { params: { type: 'command', param: 'getplans', order: 'name' } })
                .then(function(resp) {
                    $scope.roomPlans = (resp.data && resp.data.result) || [];
                });
            dashboardDynamicService.listLayouts().then(function(layouts) {
                $scope.layouts = layouts;
                if (layouts.length === 0) {
                    return createStarterLayout();
                }
                // Selection priority:
                //   1. ?id=<uuid> in the URL (deep-link from "Copy link")
                //   2. ?name=<name> in the URL (case-insensitive match on layout name)
                //   3. layout the user was last viewing (localStorage)
                //   4. layout marked isDefault
                //   5. first layout
                var urlId   = $location.search().id;
                var urlName = $location.search().name;
                var lastId  = null;
                try { lastId = localStorage.getItem(LS_KEY); } catch(e) {}
                var startLayout =
                      (urlId   && layouts.find(function(l) { return l.id === urlId; })) ||
                      (urlName && layouts.find(function(l) { return l.name && l.name.toLowerCase() === String(urlName).toLowerCase(); })) ||
                      (lastId  && layouts.find(function(l) { return l.id === lastId; })) ||
                      layouts.find(function(l) { return l.isDefault; }) ||
                      layouts[0];
                return loadLayout(startLayout.id).then(function() {
                    // Restore full-page state
                    var fp = null;
                    try { fp = localStorage.getItem(LS_FULLPAGE); } catch(e) {}
                    if (fp === '1') {
                        $scope.isFullPage = true;
                        _body.classList.add('dd-navbar-hidden');
                    }
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
        function confirmIfDirty() {
            if (!$scope.editMode || !$scope.isDirty) { return $q.when(); }
            return bootbox.confirm('Discard unsaved changes?');
        }

        $scope.switchLayout = function(id) {
            confirmIfDirty().then(function() {
                $scope.isDirty = false;
                try { localStorage.setItem(LS_KEY, id); } catch(e) {}
                loadLayout(id);
                resetStandbyTimer();
            }).catch(angular.noop);
        };

        // Build a deep-link URL for a specific dashboard and copy it to the clipboard.
        // The link uses `?id=<uuid>` so the destination machine doesn't depend on
        // dashboard ordering or the visitor's localStorage.
        $scope.copyLayoutLink = function(layout, $event) {
            if ($event) { $event.preventDefault(); $event.stopPropagation(); }
            if (!layout || !layout.id) { return; }
            var base = window.location.origin + window.location.pathname;
            var url  = base + '#/Dashboard?id=' + encodeURIComponent(layout.id);
            var done = function(ok) {
                if (ok) { ddToast.success('Link copied: ' + layout.name); }
                else    { ddToast.error('Failed to copy link'); }
            };
            if (navigator.clipboard && navigator.clipboard.writeText) {
                navigator.clipboard.writeText(url).then(function() { done(true); }, function() { done(false); });
            } else {
                // Fallback for non-secure contexts (HTTP without TLS)
                try {
                    var ta = document.createElement('textarea');
                    ta.value = url;
                    ta.style.position = 'fixed'; ta.style.opacity = '0';
                    document.body.appendChild(ta);
                    ta.select();
                    document.execCommand('copy');
                    document.body.removeChild(ta);
                    done(true);
                } catch(e) { done(false); }
            }
        };

        $scope.openRoomPlan = function(planIdx) {
            if (window.myglobals) { window.myglobals.LastPlanSelected = planIdx; }
            window._forceClassicDashboard = true;
            $route.reload();
        };

        $scope.toggleEditMode = function() {
            if (!$scope.editMode) {
                // Entering edit mode — pause kiosk and take a snapshot for cancel
                _kioskWasActive = $scope.kioskActive;
                if ($scope.kioskActive) { $scope.stopKiosk(); }
                _savedDataSnapshot = angular.copy($scope.activeData);
                $scope.editMode = true;
            } else {
                // Exiting edit mode — save if there are unsaved changes
                $scope.showLibrary = false;
                $scope.designPanel.open = false;
                if ($scope.isDirty) {
                    $scope.saveCurrentLayout().then(function() {
                        _savedDataSnapshot = null;
                        $scope.editMode = false;
                        if (_kioskWasActive) { _kioskWasActive = false; $scope.startKiosk(); }
                    });
                } else {
                    _savedDataSnapshot = null;
                    $scope.editMode = false;
                    if (_kioskWasActive) { _kioskWasActive = false; $scope.startKiosk(); }
                }
            }
        };

        $scope.cancelEdit = function() {
            function doCancel() {
                if (_savedDataSnapshot) {
                    $scope.activeData = angular.copy(_savedDataSnapshot);
                    _savedDataSnapshot = null;
                }
                $scope.editMode    = false;
                $scope.showLibrary = false;
                $scope.isDirty     = false;
                $scope.designPanel.open = false;
                refreshGrid();
                if (_kioskWasActive) { _kioskWasActive = false; $scope.startKiosk(); }
            }

            if (!$scope.isDirty) {
                $scope.$evalAsync(doCancel);
                return;
            }
            bootbox.confirm('Discard unsaved changes?').then(function() {
                $scope.$evalAsync(doCancel);
            }).catch(angular.noop);
        };

        $scope.toggleLibrary = function() {
            $scope.showLibrary = !$scope.showLibrary;
            if ($scope.showLibrary) {
                $timeout(function() {
                    var input = document.querySelector('.dd-library-search input');
                    if (input) { input.focus(); }
                }, 30);
            }
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

        $scope.libraryItemFilter = function(item) {
            var q = ($scope.librarySearch || '').trim().toLowerCase();
            if (!q) { return true; }
            return (item.label       || '').toLowerCase().indexOf(q) !== -1 ||
                   (item.description || '').toLowerCase().indexOf(q) !== -1 ||
                   (item.category    || '').toLowerCase().indexOf(q) !== -1;
        };

        $scope.saveCurrentLayout = function() {
            if (!$scope.activeLayout || !$scope.activeData) { return $q.when(); }
            // The first save gives a layout its design width: the width it was arranged at.
            // A window narrower than DESIGN_MIN (a phone) leaves it unset.
            var newDesignWidth = false;
            if ($scope.designState() === 'unset' && validDesignWidth($scope.frameWidth)) {
                $scope.activeData.designWidth = $scope.frameWidth;
                newDesignWidth = true;
            }
            return dashboardDynamicService.saveLayout($scope.activeLayout, $scope.activeData)
                .then(function() {
                    $scope.isDirty = false;
                    _savedDataSnapshot = angular.copy($scope.activeData);
                    ddToast.success(newDesignWidth
                        ? 'Dashboard saved, designed for ' + $scope.activeData.designWidth + ' px wide'
                        : 'Dashboard saved');
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
                var entry = $scope.layouts.find(function(l) { return l.id === $scope.activeLayout.id; });
                if (entry) { entry.name = name; }
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
                size:        'md',
                resolve: {
                    layouts:         function() { return $scope.layouts; },
                    kioskSettings:   function() { return $scope.kiosk; },
                    onKioskChange:   function() {
                        return function(settings) { $scope.saveKioskSettings(settings); };
                    },
                    standbySettings: function() { return $scope.standby; },
                    onStandbyChange: function() {
                        return function(settings) { $scope.saveStandbySettings(settings); };
                    },
                    swipeSettings:   function() { return $scope.swipe; },
                    onSwipeChange:   function() {
                        return function(settings) { $scope.saveSwipeSettings(settings); };
                    }
                }
            }).result.catch(angular.noop);
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
                                    resetEditSnapshot();
                                    refreshGrid();
                                    ddToast.success('Dashboard replaced');
                                });
                            } else {
                                return dashboardDynamicService.saveLayout(result.meta, result.data).then(function() {
                                    $scope.layouts.push(result.meta);
                                    $scope.activeLayout = result.meta;
                                    $scope.activeData   = result.data;
                                    resetEditSnapshot();
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
            try { localStorage.setItem(LS_FULLPAGE, $scope.isFullPage ? '1' : '0'); } catch(e) {}
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
            if (e.keyCode === 27 && $scope.editMode && $scope.designPanel.open) {
                $scope.$apply(function() { $scope.designPanel.open = false; });
                return;
            }
            if (e.keyCode === 27 && $scope.editMode) {
                $scope.cancelEdit();
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
            // Ctrl+L: toggle widget library (edit mode only)
            if (e.ctrlKey && e.keyCode === 76 && $scope.editMode) {
                e.preventDefault();
                $scope.$apply(function() { $scope.toggleLibrary(); });
            }
        }
        document.addEventListener('keydown', onKeyDown);

        // ── Navbar height tracking ────────────────────────────
        // The toolbar uses position:fixed with top equal to the navbar height.
        // On tablets in landscape mode the navbar can wrap to two lines, so we
        // measure the real height and expose it as a CSS variable instead of
        // using a hardcoded 50 px value.
        function syncNavbarHeight() {
            var nav = document.querySelector('.navbar.navbar-fixed-top');
            var h = nav ? nav.offsetHeight : 50;
            _body.style.setProperty('--dd-navbar-h', h + 'px');
        }
        syncNavbarHeight();
        window.addEventListener('resize', syncNavbarHeight);

        // ── Design width ──────────────────────────────────────
        // A layout stores the width it was arranged at (designWidth, in px). On a narrower
        // screen view mode shows the whole grid at that width, zoomed out (cards, text and
        // icons together, not lower than MIN_SCALE). On a wider screen the grid fills the
        // window as before. Edit mode shows the grid at exactly the design width, unzoomed,
        // because GridStack drag and resize do not work on a zoomed grid.
        // designWidth undefined = not set yet (taken from the window on the next save), 0 = off.
        // In kiosk mode (a wall display nobody scrolls) the floor does not apply: the whole
        // dashboard always fits. Browsers without CSS zoom (Firefox before 126) keep today's behaviour.
        var MIN_SCALE  = 0.6;
        var DESIGN_MIN = 600;    // the grid's own CSS min-width
        var DESIGN_MAX = 7680;
        var ZOOM_OK    = typeof CSS !== 'undefined' && CSS.supports && CSS.supports('zoom', '0.5');
        $scope.frameWidth  = 0;
        $scope.designPanel = { open: false, choice: 'window', other: 1280 };

        function validDesignWidth(v) {
            return v >= DESIGN_MIN && v <= DESIGN_MAX;
        }

        // Out of range values (a hand-edited or imported layout) count as off
        function getDesignWidth() {
            var dw = $scope.activeData ? parseInt($scope.activeData.designWidth, 10) : 0;
            return validDesignWidth(dw) ? dw : 0;
        }
        $scope.designWidthValue = getDesignWidth;

        $scope.designState = function() {
            if (!$scope.activeData || !$scope.frameWidth) { return ''; }
            var v = $scope.activeData.designWidth;
            if (v === undefined || v === null || v === '') { return 'unset'; }
            var dw = getDesignWidth();
            if (!dw) { return 'off'; }
            return Math.abs($scope.frameWidth - dw) > 16 ? 'differs' : 'match';
        };

        var _lastAvail = 0, _resizeEventTimer = null;
        function applyDesignWidth() {
            var frame = document.querySelector('.dd-page .dd-grid-frame');
            if (!frame) { return; }
            var root  = document.documentElement.classList;
            var avail = Math.round(frame.clientWidth);
            var dw = getDesignWidth(), width = '', zoom = '';
            var mode = dw + ($scope.kioskActive ? 'k' : '');
            var willScale = !!(avail && dw && ZOOM_OK && !$scope.editMode && avail < dw);
            // While scaled, keep room for the page scrollbar so it appearing or disappearing
            // (the zoom changes the page height) does not change the available width.
            // Switch it first and measure again, so the zoom is computed for the final width.
            if (willScale !== root.contains('dd-stable-gutter')) {
                root.toggle('dd-stable-gutter', willScale);
                avail = Math.round(frame.clientWidth);
            }
            if (avail && avail !== $scope.frameWidth) {
                $scope.$evalAsync(function() { $scope.frameWidth = avail; });
            }
            var grid = frame.querySelector('.dd-grid');
            if (!grid || !avail) { return; }
            if (dw && $scope.editMode) {
                width = dw + 'px';
            } else if (willScale) {
                var scaled = grid.style.zoom && grid.getAttribute('data-dd-design') === mode;
                // A zoom change can add or remove the page scrollbar, which changes the width again;
                // ignore such small steps so the zoom does not flip back and forth.
                if (scaled && Math.abs(avail - _lastAvail) < 20) { return; }
                var minScale = $scope.kioskActive ? 0 : MIN_SCALE;
                width = dw + 'px';
                zoom  = String(Math.round(Math.max(minScale, avail / dw) * 1000) / 1000);
            }
            var key = width + '|' + zoom;
            if (grid.getAttribute('data-dd-scale') === key) { return; }
            _lastAvail = avail;
            grid.setAttribute('data-dd-scale', key);
            grid.setAttribute('data-dd-design', mode);
            grid.style.width    = width;
            grid.style.minWidth = width ? '0' : '';
            grid.style.zoom     = zoom;
            // Widgets that size themselves on a window resize (charts) follow the new grid width
            if (_resizeEventTimer) { $timeout.cancel(_resizeEventTimer); }
            _resizeEventTimer = $timeout(function() {
                _resizeEventTimer = null;
                window.dispatchEvent(new Event('resize'));
            }, 50, false);
        }

        var _designTimer = null;
        function scheduleDesignWidth() {
            if (_designTimer) { $timeout.cancel(_designTimer); }
            _designTimer = $timeout(function() { _designTimer = null; applyDesignWidth(); }, 0, false);
        }
        $scope.$watchGroup(['editMode', 'gridReady', 'loading', 'activeData', 'activeData.designWidth', 'kioskActive'], scheduleDesignWidth);
        window.addEventListener('resize', scheduleDesignWidth);
        var _frameObserver = null;
        if (typeof ResizeObserver !== 'undefined') {
            _frameObserver = new ResizeObserver(scheduleDesignWidth);
            $timeout(function() {
                var frame = document.querySelector('.dd-page .dd-grid-frame');
                if (frame && _frameObserver) { _frameObserver.observe(frame); }
            }, 0, false);
        }

        $scope.useWindowWidth = function() {
            if (!$scope.activeData || !validDesignWidth($scope.frameWidth)) {
                ddToast.error('The window is too narrow, the design width must be at least ' + DESIGN_MIN + ' px');
                return;
            }
            $scope.activeData.designWidth = $scope.frameWidth;
            $scope.designPanel.open = false;
            $scope.isDirty = true;
        };

        $scope.openDesignPanel = function() {
            var v  = $scope.activeData && $scope.activeData.designWidth;
            var dw = getDesignWidth();
            $scope.designPanel.choice = dw ? 'current' : (v === 0 ? 'off' : 'window');
            $scope.designPanel.other  = dw || $scope.frameWidth || 1280;
            $scope.designPanel.open   = true;
        };

        $scope.closeDesignPanel = function() {
            $scope.designPanel.open = false;
        };

        $scope.applyDesignPanel = function() {
            if (!$scope.activeData) { return; }
            var p = $scope.designPanel, v;
            if (p.choice === 'current') { p.open = false; return; }
            if (p.choice === 'off') {
                v = 0;
            } else {
                v = Math.round(p.choice === 'window' ? $scope.frameWidth : +p.other);
                if (!validDesignWidth(v)) {
                    ddToast.error('The design width must be between ' + DESIGN_MIN + ' and ' + DESIGN_MAX + ' px');
                    return;
                }
            }
            $scope.activeData.designWidth = v;
            p.open = false;
            $scope.isDirty = true;
        };

        // ── Standby activity listeners ────────────────────────
        var _standbyActivityEvents = ['mousemove', 'touchstart', 'keydown', 'click'];
        _standbyActivityEvents.forEach(function(ev) {
            document.addEventListener(ev, resetStandbyTimer, { passive: true });
        });

        // ── Swipe navigation ──────────────────────────────────
        // Navigate to the previous/next dashboard relative to the one
        // currently shown, using the order of $scope.layouts. Does not wrap.
        function swipeToLayout(direction) {
            if (!$scope.layouts || $scope.layouts.length < 2 || !$scope.activeLayout) { return; }
            var idx = $scope.layouts.findIndex(function(l) {
                return l.id === $scope.activeLayout.id;
            });
            if (idx === -1) { return; }
            var nextIdx = idx + direction;
            if (nextIdx < 0 || nextIdx >= $scope.layouts.length) { return; }
            // Fired from a passive DOM touch handler (outside the digest);
            // $timeout safely enters a digest even if one is in progress.
            $timeout(function() {
                $scope.switchLayout($scope.layouts[nextIdx].id);
            });
        }

        function onSwipeStart(e) {
            _swipeMoved = false;
            var touch = e.changedTouches ? e.changedTouches[0] : null;
            _swipeStartX = touch ? touch.pageX : 0;
            _swipeStartY = touch ? touch.pageY : 0;
        }
        function onSwipeMove() { _swipeMoved = true; }
        function onSwipeEnd(e) {
            if (!$scope.swipe.enabled || !_swipeMoved) { return; }
            // Don't hijack gestures while editing or when a modal is open.
            if ($scope.editMode || $('.modal.in').length) { return; }
            var touch = e.changedTouches ? e.changedTouches[0] : null;
            if (!touch) { return; }
            var deltaX = touch.pageX - _swipeStartX;
            var deltaY = touch.pageY - _swipeStartY;
            // Higher than the floorplan's 50px: the dashboard scrolls
            // vertically and has interactive widgets, so require a firmer
            // horizontal gesture to avoid accidental dashboard switches.
            var SWIPE_THRESHOLD = 70;
            if (Math.abs(deltaX) > SWIPE_THRESHOLD && Math.abs(deltaX) > Math.abs(deltaY) * 2) {
                swipeToLayout(deltaX < 0 ? 1 : -1);
            }
        }
        document.addEventListener('touchstart', onSwipeStart, { passive: true });
        document.addEventListener('touchmove', onSwipeMove, { passive: true });
        document.addEventListener('touchend', onSwipeEnd, { passive: true });

        $scope.saveSwipeSettings = function(patch) {
            angular.extend($scope.swipe, patch);
            try { localStorage.setItem(LS_SWIPE, JSON.stringify($scope.swipe)); } catch(e) {}
        };

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
        $scope.resetToFavorites = function() {
            bootbox.confirm('Replace all widgets with the favorites layout?').then(function() {
                $http.get('json.htm?type=command&param=getdevices&filter=all&used=true&favorite=1&order=%5BOrder%5D')
                    .then(function(resp) {
                        $scope.activeData.widgets = dashboardDynamicService.buildFavoritesWidgets(resp.data.result || []);
                        $scope.isDirty = true;
                        refreshGrid();
                    })
                    .catch(function() {
                        bootbox.alert('Failed to load favorites. Please try again.');
                    });
            }).catch(angular.noop);
        };

        $scope.newDashboard = function() {
            confirmIfDirty().then(function() {
                $scope.isDirty = false;
                var name     = 'New Dashboard';
                var existing = $scope.layouts.filter(function(l) { return l.name.indexOf(name) === 0; });
                if (existing.length) { name = name + ' ' + (existing.length + 1); }
                var id        = dashboardDynamicService.generateId();
                var isFirst   = $scope.layouts.length === 0;
                var emptyData = { version: dashboardDynamicService.LAYOUT_VERSION, columns: dashboardDynamicService.GRID_COLUMNS, rowHeight: 60, margin: 8, animate: true, widgets: [] };
                dashboardDynamicService.saveLayout({ id: id, name: name, isDefault: isFirst }, emptyData)
                    .then(function() {
                        $scope.layouts.push({ id: id, name: name, isDefault: isFirst, updated: new Date().toISOString() });
                        $scope.switchLayout(id);
                        ddToast.success('Dashboard "' + name + '" created');
                    })
                    .catch(function() {
                        ddToast.error('Failed to create dashboard');
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
                resetEditSnapshot();
                try { localStorage.setItem(LS_KEY, newMeta.id); } catch(e) {}
                refreshGrid();
                ddToast.success('Dashboard duplicated');
            }).catch(function() {
                ddToast.error('Duplicate failed');
            });
        };

        $scope.setCurrentAsDefault = function() {
            if (!$scope.activeLayout || $scope.activeLayout.isDefault) { return; }
            $scope.layouts.forEach(function(l) { l.isDefault = false; });
            $scope.activeLayout.isDefault = true;
            dashboardDynamicService.saveLayout(
                { id: $scope.activeLayout.id, name: $scope.activeLayout.name, isDefault: true },
                null
            ).then(function() {
                ddToast.success('Set as default dashboard');
            }).catch(function() {
                ddToast.error('Failed to set as default');
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
            document.removeEventListener('touchstart', onSwipeStart);
            document.removeEventListener('touchmove', onSwipeMove);
            document.removeEventListener('touchend', onSwipeEnd);
            _standbyActivityEvents.forEach(function(ev) {
                document.removeEventListener(ev, resetStandbyTimer);
            });
            window.removeEventListener('resize', syncNavbarHeight);
            window.removeEventListener('resize', scheduleDesignWidth);
            if (_frameObserver) { _frameObserver.disconnect(); _frameObserver = null; }
            if (_designTimer) { $timeout.cancel(_designTimer); }
            if (_resizeEventTimer) { $timeout.cancel(_resizeEventTimer); }
            document.documentElement.classList.remove('dd-stable-gutter');
            _body.style.removeProperty('--dd-navbar-h');
            if (_standbyTimer) { $timeout.cancel(_standbyTimer); }
            _body.classList.remove('dd-navbar-hidden');
            _body.classList.remove('dd-standby');
            $scope.stopKiosk();
        });

        // ── Init ───────────────────────────────────────────────
        init();
    }]);
});
