/**
 * DashboardMobileController
 *
 * Dedicated mobile/touch-optimized dashboard controller for Domoticz.
 * Displays favorite devices in a simplified, touch-friendly interface with
 * real-time updates via WebSocket. Optimized for speed and simplicity.
 *
 * Features:
 * - Room plan support for filtering devices
 * - Touch-optimized layout with large tap targets (44px minimum)
 * - Real-time device updates via livesocket
 * - Simplified UI - no drag-and-drop, no search
 * - Categorized device display (Scenes, Lights, Temperature, Weather, Utility)
 * - Pull-to-refresh capability
 *
 * @module DashboardMobileController
 * @requires dashboardService
 * @requires livesocket
 */
define([
    'app',
    'dashboard/dashboardService',
    'widgets/dzLightWidget',
    'widgets/dzSceneWidget',
    'widgets/dzUtilityWidget',
    'livesocket'
], function (app) {
    app.controller('DashboardMobileController', [
        '$scope', '$rootScope', '$location', '$route', '$routeParams', '$q', '$timeout',
        'permissions', 'livesocket', 'dashboardService', 'domoticzApi',
        function($scope, $rootScope, $location, $route, $routeParams, $q, $timeout,
                 permissions, livesocket, dashboardService, domoticzApi) {

            // Initialize scope variables
            $scope.isMobile = true;
            $scope.loading = true;
            $scope.hasDevices = false;
            $scope.dashboardType = 2; // Mobile layout type

            // Device categories
            $scope.scenes = [];
            $scope.lights = [];
            $scope.temperature = [];
            $scope.weather = [];
            $scope.utility = [];

            // Room plan support
            $scope.RoomPlans = [];
            $scope.roomSelected = '';
            $scope.selectedPlanId = 0;

            // Livesocket subscription cleanup function
            var unsubscribe = null;

            /**
             * Initialize the mobile dashboard
             */
            function init() {
                // Set body classes for mobile styling
                $('body').addClass('dashboard').addClass('dashMobile');

                // Get selected plan from route params or global state
                var planParam = $routeParams.plan;
                if (planParam !== undefined) {
                    $scope.selectedPlanId = parseInt(planParam);
                    $scope.roomSelected = $scope.selectedPlanId;
                } else if (window.myglobals && window.myglobals.LastPlanSelected !== undefined) {
                    $scope.selectedPlanId = window.myglobals.LastPlanSelected;
                    $scope.roomSelected = $scope.selectedPlanId;
                }

                // Load room plans
                loadRoomPlans();

                // Load favorite devices
                loadDevices();

                // Subscribe to real-time updates
                subscribeToUpdates();
            }

            /**
             * Load available room plans from server
             */
            function loadRoomPlans() {
                domoticzApi.sendRequest({
                    type: 'command',
                    param: 'getplans',
                    order: 'name'
                }).then(function(data) {
                    if (data.result && data.result.length > 0) {
                        $scope.RoomPlans = data.result;
                    }
                }).catch(function(error) {
                    // Failed to load room plans - continue without them
                });
            }

            /**
             * Load devices and categorize them
             */
            function loadDevices() {
                $scope.loading = true;

                dashboardService.loadFavorites($scope.selectedPlanId).then(function(result) {
                    var devices = result.devices || [];

                    // Categorize devices
                    var categorized = dashboardService.categorizeDevices(devices);

                    // Update scope
                    $scope.scenes = categorized.scenes;
                    $scope.lights = categorized.lights;
                    $scope.temperature = categorized.temperature;
                    $scope.weather = categorized.weather;
                    $scope.utility = categorized.utility;

                    // Check if we have any devices
                    $scope.hasDevices = devices.length > 0;

                    $scope.loading = false;
                }).catch(function(error) {
                    $scope.loading = false;
                    $scope.hasDevices = false;
                });
            }

            /**
             * Handle room plan selection change
             */
            $scope.changeRoom = function() {
                $scope.selectedPlanId = $scope.roomSelected || 0;

                // Update global state
                if (window.myglobals) {
                    window.myglobals.LastPlanSelected = $scope.selectedPlanId;
                }

                // Update URL
                if ($scope.selectedPlanId > 0) {
                    $location.search('plan', $scope.selectedPlanId).replace();
                } else {
                    $location.search('plan', null).replace();
                }

                // Reload devices
                loadDevices();
            };

            /**
             * Subscribe to real-time device and scene updates
             */
            function subscribeToUpdates() {
                unsubscribe = dashboardService.subscribeToUpdates($scope, {
                    onDeviceUpdate: function(deviceData) {
                        handleDeviceUpdate(deviceData);
                    },
                    onSceneUpdate: function(sceneData) {
                        handleSceneUpdate(sceneData);
                    }
                });
            }

            /**
             * Handle real-time device update from WebSocket
             * @param {Object} deviceData - Updated device data
             */
            function handleDeviceUpdate(deviceData) {
                if (!deviceData || !deviceData.idx) {
                    return;
                }

                var idx = deviceData.idx;
                var updated = false;

                // Try to find and update device in each category
                var categories = ['lights', 'temperature', 'weather', 'utility'];

                categories.forEach(function(category) {
                    var devices = $scope[category];
                    for (var i = 0; i < devices.length; i++) {
                        if (devices[i].idx === idx) {
                            // Update device properties
                            angular.extend(devices[i], deviceData);
                            updated = true;
                            break;
                        }
                    }
                });

                // Device update applied or ignored if not in current view
            }

            /**
             * Handle real-time scene update from WebSocket
             * @param {Object} sceneData - Updated scene data
             */
            function handleSceneUpdate(sceneData) {
                if (!sceneData || !sceneData.idx) {
                    return;
                }

                var idx = sceneData.idx;

                // Find and update scene
                for (var i = 0; i < $scope.scenes.length; i++) {
                    if ($scope.scenes[i].idx === idx) {
                        // Update scene properties
                        angular.extend($scope.scenes[i], sceneData);
                        break;
                    }
                }
            }

            /**
             * Refresh all devices (pull-to-refresh)
             */
            $scope.refresh = function() {
                loadDevices();
            };

            /**
             * Cleanup on controller destroy
             */
            $scope.$on('$destroy', function() {
                // Remove body classes
                $('body').removeClass('dashboard').removeClass('dashMobile');

                // Unsubscribe from livesocket updates
                if (unsubscribe) {
                    unsubscribe();
                }
            });

            // Global backward-compatibility functions for legacy code
            // These maintain compatibility with existing onclick handlers and modals

            /**
             * Switch a scene on/off (global function for compatibility)
             * @param {number} idx - Scene index
             * @param {string} switchcmd - Command ('On', 'Off', 'Toggle')
             * @param {string} [passcode] - Optional password for protected scenes
             */
            window.SwitchScene = function(idx, switchcmd, passcode) {
                var isProtected = false;

                // Find the scene to check if it's protected
                for (var i = 0; i < $scope.scenes.length; i++) {
                    if ($scope.scenes[i].idx === idx) {
                        isProtected = $scope.scenes[i].Protected === true || $scope.scenes[i].Protected === 1;
                        break;
                    }
                }

                dashboardService.switchScene(idx, switchcmd, isProtected && !passcode).then(function() {
                    // Success - update will come via WebSocket
                }).catch(function(error) {
                    if (error.needsPassword) {
                        // Trigger password dialog
                        PasswordCheck(function() {
                            window.SwitchScene(idx, switchcmd, true);
                        });
                    } else {
                        ShowNotify($.t('Problem switching scene!'), 2500, true);
                    }
                });
            };

            /**
             * Switch a light/device on/off (global function for compatibility)
             * @param {number} idx - Device index
             * @param {string} switchcmd - Command ('On', 'Off', etc.)
             * @param {string} [passcode] - Optional password for protected devices
             */
            window.SwitchLight = function(idx, switchcmd, passcode) {
                var isProtected = false;

                // Find the device to check if it's protected
                var allDevices = [].concat($scope.lights, $scope.utility);
                for (var i = 0; i < allDevices.length; i++) {
                    if (allDevices[i].idx === idx) {
                        isProtected = allDevices[i].Protected === true || allDevices[i].Protected === 1;
                        break;
                    }
                }

                dashboardService.switchDevice(idx, switchcmd, isProtected && !passcode).then(function() {
                    // Success - update will come via WebSocket
                }).catch(function(error) {
                    if (error.needsPassword) {
                        // Trigger password dialog
                        PasswordCheck(function() {
                            window.SwitchLight(idx, switchcmd, true);
                        });
                    } else {
                        ShowNotify($.t('Problem switching device!'), 2500, true);
                    }
                });
            };

            /**
             * Switch Evohome modal status (global function for compatibility)
             * @param {number} idx - Device index
             * @param {string} name - Display name (unused)
             * @param {string} status - Status to set
             */
            window.SwitchModal = function(idx, name, status) {
                dashboardService.switchModal(idx, status).then(function() {
                    // Close any open modals
                    if (typeof bootbox !== 'undefined') {
                        bootbox.hideAll();
                    }
                }).catch(function(error) {
                    ShowNotify($.t('Problem switching mode!'), 2500, true);
                });
            };

            /**
             * Set color value (global function for compatibility)
             * @param {number} idx - Device index
             * @param {string} hue - Hue value
             * @param {string} brightness - Brightness value
             * @param {string} iswhite - White mode flag
             */
            window.SetColValue = function(idx, hue, brightness, iswhite) {
                // This is a simple implementation - full color picker logic is complex
                $.ajax({
                    url: 'json.htm?type=command&param=setcolbrightnessvalue&idx=' + idx +
                         '&hue=' + hue + '&brightness=' + brightness + '&iswhite=' + iswhite,
                    async: false,
                    dataType: 'json'
                });
            };

            /**
             * Navigate to different layout/view (global function for compatibility)
             * @param {string} layout - Layout name
             */
            window.SwitchLayout = function(layout) {
                $location.path('/' + layout);
                $scope.$apply();
            };

            // Initialize controller
            init();
        }
    ]);
});
