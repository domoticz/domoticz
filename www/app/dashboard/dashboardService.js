/**
 * Dashboard Service
 *
 * Shared service layer that encapsulates common dashboard logic for both
 * Desktop and Mobile dashboard controllers. Handles device loading,
 * categorization, switching, real-time updates, and Evohome operations.
 *
 * @module dashboardService
 */
define(['app', 'domoticz.api', 'livesocket'], function(app) {
    app.factory('dashboardService', ['$q', '$rootScope', 'domoticzApi', 'deviceApi', 'sceneApi', 'livesocket',
        function($q, $rootScope, domoticzApi, deviceApi, sceneApi, livesocket) {

            /**
             * Load all favorite devices for a given plan
             *
             * @param {number} [planId] - Optional plan ID. Defaults to window.myglobals.LastPlanSelected or 0
             * @returns {Promise<{devices: Array, lastUpdateTime: number}>} Promise resolving to device data
             */
            function loadFavorites(planId) {
                var selectedPlan = planId;

                if (typeof selectedPlan === 'undefined') {
                    selectedPlan = (typeof window.myglobals !== 'undefined' && typeof window.myglobals.LastPlanSelected !== 'undefined')
                        ? window.myglobals.LastPlanSelected
                        : 0;
                }

                var bFavorites = 1;
                if (selectedPlan > 0) {
                    bFavorites = 0;
                }

                var params = {
                    type: 'command',
                    param: 'getdevices',
                    filter: 'all',
                    used: selectedPlan > 0 ? 'all' : 'true',
                    favorite: bFavorites,
                    order: '[Order]',
                    plan: selectedPlan
                };

                return domoticzApi.sendRequest(params).then(function(data) {
                    var result = {
                        devices: data.result || [],
                        lastUpdateTime: data.ActTime ? parseInt(data.ActTime) : 0,
                        sunrise: data.Sunrise,
                        sunset: data.Sunset,
                        serverTime: data.ServerTime
                    };

                    return result;
                }).catch(function(error) {
                    return $q.reject(error);
                });
            }

            /**
             * Categorize a flat array of devices into type-specific categories
             *
             * @param {Array} devices - Array of device objects
             * @returns {Object} Object with categorized device arrays:
             *                   {lights: [], scenes: [], temperature: [], weather: [], utility: []}
             */
            function categorizeDevices(devices) {
                var categorized = {
                    scenes: [],
                    lights: [],
                    temperature: [],
                    weather: [],
                    utility: []
                };

                if (!devices || !Array.isArray(devices)) {
                    return categorized;
                }

                devices.forEach(function(item) {
                    // Scenes/Groups
                    if (item.Type.indexOf('Scene') === 0 || item.Type.indexOf('Group') === 0) {
                        categorized.scenes.push(item);
                    }
                    // Light devices
                    else if (
                        item.Type.indexOf('Light') === 0 ||
                        item.SubType === 'Smartwares Mode' ||
                        item.Type.indexOf('Blind') === 0 ||
                        item.Type.indexOf('Curtain') === 0 ||
                        item.Type.indexOf('Thermostat 2') === 0 ||
                        item.Type.indexOf('Thermostat 3') === 0 ||
                        item.Type.indexOf('Chime') === 0 ||
                        item.Type.indexOf('Color Switch') === 0 ||
                        item.Type.indexOf('RFY') === 0 ||
                        item.Type.indexOf('ASA') === 0 ||
                        item.SubType === 'Relay' ||
                        (typeof item.SubType !== 'undefined' && item.SubType.indexOf('Itho') === 0) ||
                        (typeof item.SubType !== 'undefined' && item.SubType.indexOf('Lucci') === 0) ||
                        (typeof item.SubType !== 'undefined' && item.SubType.indexOf('Westinghouse') === 0) ||
                        (typeof item.SubType !== 'undefined' && item.SubType.indexOf('Falmec') === 0) ||
                        (item.Type.indexOf('Value') === 0 && typeof item.SwitchType !== 'undefined')
                    ) {
                        categorized.lights.push(item);
                    }
                    // Temperature sensors
                    else if (
                        typeof item.Temp !== 'undefined' ||
                        typeof item.Humidity !== 'undefined' ||
                        typeof item.Chill !== 'undefined'
                    ) {
                        categorized.temperature.push(item);
                        // Some temperature devices also have weather properties (e.g. Temp+Hum+Baro,
                        // Temp+Baro, Wind with temp, UV with temp) - show them in both sections
                        if (
                            typeof item.Rain !== 'undefined' ||
                            typeof item.Visibility !== 'undefined' ||
                            typeof item.UVI !== 'undefined' ||
                            typeof item.Radiation !== 'undefined' ||
                            typeof item.Direction !== 'undefined' ||
                            typeof item.Barometer !== 'undefined'
                        ) {
                            categorized.weather.push(item);
                        }
                    }
                    // Weather sensors (without temperature properties)
                    else if (
                        typeof item.Rain !== 'undefined' ||
                        typeof item.Visibility !== 'undefined' ||
                        typeof item.UVI !== 'undefined' ||
                        typeof item.Radiation !== 'undefined' ||
                        typeof item.Direction !== 'undefined' ||
                        typeof item.Barometer !== 'undefined'
                    ) {
                        categorized.weather.push(item);
                    }
                    // Utility sensors (everything else that matches utility criteria)
                    else if (
                        typeof item.Counter !== 'undefined' ||
                        item.Type === 'Current' ||
                        item.Type === 'Energy' ||
                        item.SubType === 'kWh' ||
                        item.Type === 'Current/Energy' ||
                        item.Type === 'Power' ||
                        item.Type === 'Air Quality' ||
                        item.Type === 'Lux' ||
                        item.Type === 'Weight' ||
                        item.Type === 'Usage' ||
                        item.SubType === 'Percentage' ||
                        (item.Type === 'Setpoint' && item.SubType === 'SetPoint') ||
                        item.SubType === 'Soil Moisture' ||
                        item.SubType === 'Leaf Wetness' ||
                        item.SubType === 'Voltage' ||
                        item.SubType === 'Distance' ||
                        item.SubType === 'Current' ||
                        item.SubType === 'Text' ||
                        item.SubType === 'Alert' ||
                        item.SubType === 'Pressure' ||
                        item.SubType === 'A/D' ||
                        item.SubType === 'Thermostat Mode' ||
                        item.SubType === 'Thermostat Fan Mode' ||
                        item.SubType === 'Fan' ||
                        item.SubType === 'Smartwares' ||
                        item.SubType === 'Waterflow' ||
                        item.SubType === 'Sound Level' ||
                        item.SubType === 'Custom Sensor' ||
                        item.SubType === 'Thermostat Clock' ||
                        item.Type === 'Radiator 1'
                    ) {
                        categorized.utility.push(item);
                    }
                });

                return categorized;
            }

            /**
             * Switch a device on/off
             *
             * @param {number} idx - Device index
             * @param {string} command - Command to send ('On', 'Off', etc.)
             * @param {boolean} isProtected - Whether device is password protected
             * @returns {Promise} Promise that resolves on success, rejects if password needed
             */
            function switchDevice(idx, command, isProtected) {
                if (isProtected === true || isProtected === 1) {
                    return $q.reject({needsPassword: true});
                }

                return domoticzApi.sendCommand('switchlight', {
                    idx: idx,
                    switchcmd: command
                });
            }

            /**
             * Switch a scene on/off
             *
             * @param {number} idx - Scene index
             * @param {string} command - Command to send ('On', 'Off', 'Toggle')
             * @param {boolean} isProtected - Whether scene is password protected
             * @returns {Promise} Promise that resolves on success, rejects if password needed
             */
            function switchScene(idx, command, isProtected) {
                if (isProtected === true || isProtected === 1) {
                    return $q.reject({needsPassword: true});
                }

                return domoticzApi.sendCommand('switchscene', {
                    idx: idx,
                    switchcmd: command
                });
            }

            /**
             * Switch Evohome modal status
             *
             * @param {number} idx - Device index
             * @param {string} status - Status to set (Auto, AutoWithEco, Away, DayOff, Custom, HeatingOff)
             * @param {number} [action=1] - Action flag
             * @returns {Promise} Promise that resolves on success
             */
            function switchModal(idx, status, action) {
                var actionFlag = action || 1;

                return domoticzApi.sendCommand('switchmodal', {
                    idx: idx,
                    status: status,
                    action: actionFlag
                });
            }

            /**
             * Convert Evohome status codes to display text
             *
             * @param {string} status - Raw Evohome status code
             * @returns {string} Human-readable status text
             */
            function getEvohomeDisplayText(status) {
                var displayMap = {
                    'Auto': 'Normal',
                    'AutoWithEco': 'Economy',
                    'DayOff': 'Day Off',
                    'HeatingOff': 'Heating Off'
                };

                return displayMap[status] || status;
            }

            /**
             * Reorder favorites by saving new device order
             *
             * @param {string} deviceOrder - Comma-separated list of device indices in new order
             * @returns {Promise} Promise that resolves on success
             */
            function reorderFavorites(deviceOrder) {
                return domoticzApi.sendCommand('orderfavorite', {
                    order: deviceOrder
                });
            }

            /**
             * Subscribe to real-time device and scene updates via WebSocket
             *
             * @param {Object} scope - Angular scope for automatic cleanup
             * @param {Object} callbacks - Callback functions: {onDeviceUpdate: fn, onSceneUpdate: fn}
             * @returns {Function} Cleanup function to unsubscribe
             */
            function subscribeToUpdates(scope, callbacks) {
                var deviceUpdateHandler = null;
                var sceneUpdateHandler = null;

                if (callbacks.onDeviceUpdate) {
                    deviceUpdateHandler = scope.$on('device_update', function(event, deviceData) {
                        $rootScope.$applyAsync(function() {
                            callbacks.onDeviceUpdate(deviceData);
                        });
                    });
                }

                if (callbacks.onSceneUpdate) {
                    sceneUpdateHandler = scope.$on('scene_update', function(event, sceneData) {
                        $rootScope.$applyAsync(function() {
                            callbacks.onSceneUpdate(sceneData);
                        });
                    });
                }

                // Return cleanup function
                return function cleanup() {
                    if (deviceUpdateHandler) {
                        deviceUpdateHandler();
                    }
                    if (sceneUpdateHandler) {
                        sceneUpdateHandler();
                    }
                };
            }

            /**
             * Refresh a single device's data from server
             *
             * @param {number} idx - Device index
             * @returns {Promise<Object>} Promise resolving to updated device data
             */
            function refreshDevice(idx) {
                return deviceApi.getDevice(idx);
            }

            /**
             * Refresh a single scene's data from server
             *
             * @param {number} idx - Scene index
             * @returns {Promise<Object>} Promise resolving to updated scene data
             */
            function refreshScene(idx) {
                return sceneApi.getScene(idx);
            }

            // Public API
            return {
                loadFavorites: loadFavorites,
                categorizeDevices: categorizeDevices,
                switchDevice: switchDevice,
                switchScene: switchScene,
                switchModal: switchModal,
                getEvohomeDisplayText: getEvohomeDisplayText,
                reorderFavorites: reorderFavorites,
                subscribeToUpdates: subscribeToUpdates,
                refreshDevice: refreshDevice,
                refreshScene: refreshScene
            };
        }
    ]);
});
