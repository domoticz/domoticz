define(['app.permissions', 'livesocket'], function(appPermissionsModule, websocketModule) {
    var module = angular.module('domoticz.api', [appPermissionsModule.name, websocketModule.name]);

    module.factory('domoticzApi', function($q, $http, $httpParamSerializer, livesocket) {
        return {
            sendRequest: sendRequest,
            sendCommand: sendCommand,
            errorHandler: errorHandler
        };

        function sendRequest(data, useWebSockets) {
            if (useWebSockets) {
                const query = $httpParamSerializer(data);
                return livesocket.sendRequest(query);
            } else {
                return $http.get('json.htm', {
                    params: data
                }).then(function(response) {
                    return response.data;
                });
            }
        }

        function sendCommand(command, data) {
            var commandParams = { type: 'command', param: command };
            return sendRequest(Object.assign({}, commandParams, data)).then(errorHandler);
        }

        function errorHandler(response) {
            return response && response.status !== 'OK' ? $q.reject(response) : response;
        }
    });

    module.factory('dzApiHelper', function($q, permissions) {
        return {
            checkViewerPermissions: checkPermissions.bind(this, 'Viewer'),
            checkUserPermissions: checkPermissions.bind(this, 'User'),
            checkAdminPermissions: checkPermissions.bind(this, 'Admin')
        };

        function checkPermissions(value) {
            if (!permissions.hasPermission(value)) {
                var message = $.t('You do not have permission to do that!');
                HideNotify();
                ShowNotify(message, 2500, true);
                return $q.reject(message);
            } else {
                return $q.resolve();
            }
        }
    });

    module.factory('sceneApi', function(bootbox, domoticzApi, dzApiHelper, dzNotification) {
        return {
            switchOn: createSwitchCommand('On'),
            switchOff: createSwitchCommand('Off'),
            renameScene: renameScene
        };

        function createSwitchCommand(command) {
            return function(deviceIdx) {
                var notification = dzNotification.show($.t('Switching') + ' ' + $.t(command));

                return dzApiHelper.checkUserPermissions().then(function() {
                    var promise = domoticzApi.sendCommand('switchscene', {
                        idx: deviceIdx,
                        switchcmd: command
                    });

                    promise.catch(function(result) {
                        bootbox.alert(result.message || 'Problem sending switch command');
                    }).finally(function() {
                        notification.hide();
                    });

                    return promise;
                });
            }
        }

        function renameScene(sceneIdx, name) {
            return domoticzApi.sendCommand('renamescene', {
                idx: sceneIdx,
                name: name
            });
        }


    });

    module.factory('deviceApi', function($q, domoticzApi, dzApiHelper, dzTimeAndSun, dzNotification, Device) {
        return {
            getLightsDevices: getLightsDevices,
            getDeviceInfo: getDeviceInfo,
            updateDeviceInfo: updateDeviceInfo,
            renameDevice: renameDevice,
            removeDevice: removeDevice,
            removeScene: removeScene,
            setDeviceUsed: setDeviceUsed,
            makeFavorite: makeFavorite,
        };

        function getLightsDevices() {
            return domoticzApi.sendCommand('getlightswitches')
                .then(function(response) {
                    return response.result || [];
                });
        }

        function getDeviceInfo(deviceIdx) {
            return domoticzApi.sendRequest({ type: 'devices', rid: deviceIdx })
                .then(function(data) {
                    dzTimeAndSun.updateData(data);

                    return data && data.result && data.result.length === 1
                        ? new Device(data.result[0])
                        : $q.reject(data);
                });
        }

        function updateDeviceInfo(deviceIdx, data) {
            return domoticzApi.sendRequest(Object.assign({}, data, {
                idx: deviceIdx
            }));
        }

        function renameDevice(deviceIdx, name) {
            return domoticzApi.sendCommand('renamedevice', {
                idx: deviceIdx,
                name: name
            });
        }

        function removeDevice(deviceIdx) {
            return domoticzApi.sendRequest({
                type: 'deletedevice',
                idx: Array.isArray(deviceIdx) ? deviceIdx.join(';') : deviceIdx
            }).then(domoticzApi.errorHandler);
        }

        function removeScene(deviceIdx) {
            return domoticzApi.sendRequest({
                type: 'deletescene',
                idx: Array.isArray(deviceIdx) ? deviceIdx.join(';') : deviceIdx
            }).then(domoticzApi.errorHandler);
        }

        function setDeviceUsed(deviceIdx, bUsed, name, mainDeviceIdx) {
            return domoticzApi.sendCommand('setdevused', {
                idx: deviceIdx,
                used: bUsed,
                name: name || '',
                maindeviceidx: mainDeviceIdx || ''
            });
        }

        function makeFavorite(deviceIdx, isFavorite) {
            return dzApiHelper.checkUserPermissions().then(function() {
                return domoticzApi.sendCommand('makefavorite', {
                    idx: deviceIdx,
                    isfavorite: isFavorite,
                });
            });
        }
    });

    module.factory('deviceLightApi', function(bootbox, domoticzApi, dzNotification, dzApiHelper) {
        return {
            switchOff: createSwitchCommand('Off'),
            switchOn: createSwitchCommand('On'),
            setColor: setColor,
            brightnessUp: createCommand('brightnessup'),
            brightnessDown: createCommand('brightnessdown'),
            nightLight: createCommand('nightlight'),
            fullLight: createCommand('fulllight'),
            whiteLight: createCommand('whitelight'),
            colorWarmer: createCommand('warmer'),
            colorColder: createCommand('cooler'),
            discoUp: createCommand('discoup'),
            discoDown: createCommand('discodown'),
            discoMode: createCommand('discomode'),
            speedUp: createCommand('speedup'),
            speedDown: createCommand('speeddown'),
            speedMin: createCommand('speedmin'),
            speedMax: createCommand('speedmax')
        };

        function createCommand(command) {
            return function(deviceIdx) {
                return dzApiHelper.checkUserPermissions().then(function() {
                    return domoticzApi.sendCommand(command, {
                        idx: deviceIdx
                    });
                });
            }
        }

        function createSwitchCommand(command) {
            return dzNotification.withNotificationDecorator(function(deviceIdx) {
                return dzApiHelper.checkUserPermissions().then(function() {
                    var promise = domoticzApi.sendCommand('switchlight', {
                        idx: deviceIdx,
                        switchcmd: command
                    });

                    promise.catch(function(result) {
                        bootbox.alert(result.message || 'Problem sending switch command');
                    });

                    return promise;
                });
            }, $.t('Switching') + ' ' + $.t(command))
        }

        function setColor(deviceIdx, color, brightness) {
            return dzApiHelper.checkUserPermissions().then(function() {
                return domoticzApi.sendCommand('setcolbrightnessvalue', {
                    idx: deviceIdx,
                    color: color,
                    brightness: brightness
                });
            });
        }
    });

    return module;
});
