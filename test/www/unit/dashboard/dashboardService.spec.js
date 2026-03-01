/**
 * Unit tests for dashboardService
 */
define([
    'angular',
    'angular-mocks',
    'dashboard/dashboardService'
], function() {
    'use strict';

    describe('dashboardService', function() {
        var dashboardService, $rootScope, $q;
        var mockDomoticzApi, mockDeviceApi, mockSceneApi, mockLivesocket;

        beforeEach(function() {
            // Load the domoticz module
            module('domoticz');

            // Mock dependencies
            module(function($provide) {
                mockDomoticzApi = {
                    sendRequest: jasmine.createSpy('sendRequest').and.callFake(function() {
                        return $q.resolve({ result: [], ActTime: 1234567890 });
                    }),
                    sendCommand: jasmine.createSpy('sendCommand').and.callFake(function() {
                        return $q.resolve({ status: 'OK' });
                    })
                };

                mockDeviceApi = {
                    getDevice: jasmine.createSpy('getDevice').and.callFake(function(idx) {
                        return $q.resolve({ idx: idx, Name: 'Test Device' });
                    })
                };

                mockSceneApi = {
                    getScene: jasmine.createSpy('getScene').and.callFake(function(idx) {
                        return $q.resolve({ idx: idx, Name: 'Test Scene' });
                    })
                };

                mockLivesocket = {
                    subscribe: jasmine.createSpy('subscribe')
                };

                $provide.value('domoticzApi', mockDomoticzApi);
                $provide.value('deviceApi', mockDeviceApi);
                $provide.value('sceneApi', mockSceneApi);
                $provide.value('livesocket', mockLivesocket);
            });

            inject(function(_dashboardService_, _$rootScope_, _$q_) {
                dashboardService = _dashboardService_;
                $rootScope = _$rootScope_;
                $q = _$q_;
            });
        });

        describe('loadFavorites', function() {
            beforeEach(function() {
                // Setup global myglobals
                window.myglobals = {
                    LastPlanSelected: 0
                };
            });

            afterEach(function() {
                delete window.myglobals;
            });

            it('should call domoticzApi.sendRequest with correct parameters for favorites', function() {
                dashboardService.loadFavorites();

                expect(mockDomoticzApi.sendRequest).toHaveBeenCalledWith({
                    type: 'command',
                    param: 'getdevices',
                    filter: 'all',
                    used: true,
                    favorite: 1,
                    order: '[Order]',
                    plan: 0
                });
            });

            it('should call domoticzApi.sendRequest with favorite=0 when plan is selected', function() {
                dashboardService.loadFavorites(5);

                expect(mockDomoticzApi.sendRequest).toHaveBeenCalledWith({
                    type: 'command',
                    param: 'getdevices',
                    filter: 'all',
                    used: true,
                    favorite: 0,
                    order: '[Order]',
                    plan: 5
                });
            });

            it('should return devices and lastUpdateTime from response', function(done) {
                var mockDevices = [
                    { idx: 1, Name: 'Device 1' },
                    { idx: 2, Name: 'Device 2' }
                ];

                mockDomoticzApi.sendRequest.and.returnValue($q.resolve({
                    result: mockDevices,
                    ActTime: 9876543210
                }));

                dashboardService.loadFavorites().then(function(result) {
                    expect(result.devices).toEqual(mockDevices);
                    expect(result.lastUpdateTime).toBe(9876543210);
                    done();
                });

                $rootScope.$digest();
            });

            it('should handle missing result array', function(done) {
                mockDomoticzApi.sendRequest.and.returnValue($q.resolve({ ActTime: 123 }));

                dashboardService.loadFavorites().then(function(result) {
                    expect(result.devices).toEqual([]);
                    expect(result.lastUpdateTime).toBe(123);
                    done();
                });

                $rootScope.$digest();
            });

            it('should handle API errors', function(done) {
                mockDomoticzApi.sendRequest.and.returnValue($q.reject({ error: 'API Error' }));

                dashboardService.loadFavorites().catch(function(error) {
                    expect(error.error).toBe('API Error');
                    done();
                });

                $rootScope.$digest();
            });
        });

        describe('categorizeDevices', function() {
            it('should return empty categories for null or undefined devices', function() {
                var result = dashboardService.categorizeDevices(null);
                expect(result.scenes).toEqual([]);
                expect(result.lights).toEqual([]);
                expect(result.temperature).toEqual([]);
                expect(result.weather).toEqual([]);
                expect(result.utility).toEqual([]);
            });

            it('should categorize scene devices', function() {
                var devices = [
                    { idx: 1, Type: 'Scene', Name: 'Living Room Scene' },
                    { idx: 2, Type: 'Group', Name: 'All Lights' }
                ];

                var result = dashboardService.categorizeDevices(devices);
                expect(result.scenes.length).toBe(2);
                expect(result.scenes[0].idx).toBe(1);
                expect(result.scenes[1].idx).toBe(2);
            });

            it('should categorize light devices', function() {
                var devices = [
                    { idx: 1, Type: 'Light/Switch', SubType: 'Switch' },
                    { idx: 2, Type: 'Blinds', SubType: 'Blinds' },
                    { idx: 3, Type: 'Color Switch', SubType: 'RGBW' },
                    { idx: 4, Type: 'Thermostat 2', SubType: 'Setpoint' },
                    { idx: 5, SubType: 'Relay' }
                ];

                var result = dashboardService.categorizeDevices(devices);
                expect(result.lights.length).toBe(5);
            });

            it('should categorize temperature devices', function() {
                var devices = [
                    { idx: 1, Type: 'Temp', Temp: 22.5 },
                    { idx: 2, Type: 'Humidity', Humidity: 60 },
                    { idx: 3, Type: 'Temp+Humidity', Temp: 20, Humidity: 55, Chill: 18 }
                ];

                var result = dashboardService.categorizeDevices(devices);
                expect(result.temperature.length).toBe(3);
            });

            it('should categorize weather devices', function() {
                var devices = [
                    { idx: 1, Type: 'Rain', Rain: 5.2 },
                    { idx: 2, Type: 'Wind', Direction: 180, Speed: 10 },
                    { idx: 3, Type: 'UV', UVI: 6 },
                    { idx: 4, Type: 'Barometer', Barometer: 1013 }
                ];

                var result = dashboardService.categorizeDevices(devices);
                expect(result.weather.length).toBe(4);
            });

            it('should categorize utility devices', function() {
                var devices = [
                    { idx: 1, Type: 'Energy', SubType: 'kWh' },
                    { idx: 2, Type: 'Power', Power: 100 },
                    { idx: 3, Type: 'Current', Counter: 1000 },
                    { idx: 4, Type: 'Air Quality', SubType: 'Air Quality' },
                    { idx: 5, Type: 'Lux', SubType: 'Lux' },
                    { idx: 6, SubType: 'Percentage' },
                    { idx: 7, SubType: 'Text' },
                    { idx: 8, Type: 'Setpoint', SubType: 'SetPoint' }
                ];

                var result = dashboardService.categorizeDevices(devices);
                expect(result.utility.length).toBe(8);
            });

            it('should handle mixed device types', function() {
                var devices = [
                    { idx: 1, Type: 'Scene', Name: 'Scene 1' },
                    { idx: 2, Type: 'Light/Switch', SubType: 'Switch' },
                    { idx: 3, Type: 'Temp', Temp: 21 },
                    { idx: 4, Type: 'Rain', Rain: 2.5 },
                    { idx: 5, Type: 'Energy', SubType: 'kWh' }
                ];

                var result = dashboardService.categorizeDevices(devices);
                expect(result.scenes.length).toBe(1);
                expect(result.lights.length).toBe(1);
                expect(result.temperature.length).toBe(1);
                expect(result.weather.length).toBe(1);
                expect(result.utility.length).toBe(1);
            });
        });

        describe('switchDevice', function() {
            it('should reject with needsPassword when device is protected', function(done) {
                dashboardService.switchDevice(123, 'On', true).catch(function(error) {
                    expect(error.needsPassword).toBe(true);
                    expect(mockDomoticzApi.sendCommand).not.toHaveBeenCalled();
                    done();
                });

                $rootScope.$digest();
            });

            it('should reject with needsPassword when isProtected is 1', function(done) {
                dashboardService.switchDevice(123, 'On', 1).catch(function(error) {
                    expect(error.needsPassword).toBe(true);
                    done();
                });

                $rootScope.$digest();
            });

            it('should call domoticzApi.sendCommand when device is not protected', function() {
                dashboardService.switchDevice(456, 'Off', false);

                expect(mockDomoticzApi.sendCommand).toHaveBeenCalledWith('switchlight', {
                    idx: 456,
                    switchcmd: 'Off'
                });
            });
        });

        describe('switchScene', function() {
            it('should reject with needsPassword when scene is protected', function(done) {
                dashboardService.switchScene(789, 'On', true).catch(function(error) {
                    expect(error.needsPassword).toBe(true);
                    expect(mockDomoticzApi.sendCommand).not.toHaveBeenCalled();
                    done();
                });

                $rootScope.$digest();
            });

            it('should call domoticzApi.sendCommand when scene is not protected', function() {
                dashboardService.switchScene(789, 'Toggle', false);

                expect(mockDomoticzApi.sendCommand).toHaveBeenCalledWith('switchscene', {
                    idx: 789,
                    switchcmd: 'Toggle'
                });
            });
        });

        describe('switchModal', function() {
            it('should call domoticzApi.sendCommand with correct Evohome parameters', function() {
                dashboardService.switchModal(101, 'Auto', 1);

                expect(mockDomoticzApi.sendCommand).toHaveBeenCalledWith('switchmodal', {
                    idx: 101,
                    status: 'Auto',
                    action: 1
                });
            });

            it('should default action to 1 if not provided', function() {
                dashboardService.switchModal(102, 'Away');

                expect(mockDomoticzApi.sendCommand).toHaveBeenCalledWith('switchmodal', {
                    idx: 102,
                    status: 'Away',
                    action: 1
                });
            });
        });

        describe('getEvohomeDisplayText', function() {
            it('should convert Auto to Normal', function() {
                expect(dashboardService.getEvohomeDisplayText('Auto')).toBe('Normal');
            });

            it('should convert AutoWithEco to Economy', function() {
                expect(dashboardService.getEvohomeDisplayText('AutoWithEco')).toBe('Economy');
            });

            it('should convert DayOff to Day Off', function() {
                expect(dashboardService.getEvohomeDisplayText('DayOff')).toBe('Day Off');
            });

            it('should convert HeatingOff to Heating Off', function() {
                expect(dashboardService.getEvohomeDisplayText('HeatingOff')).toBe('Heating Off');
            });

            it('should return original status for unmapped values', function() {
                expect(dashboardService.getEvohomeDisplayText('Custom')).toBe('Custom');
                expect(dashboardService.getEvohomeDisplayText('Away')).toBe('Away');
            });
        });

        describe('reorderFavorites', function() {
            it('should call domoticzApi.sendCommand with orderfavorite', function() {
                var deviceOrder = '1,5,3,7,2';
                dashboardService.reorderFavorites(deviceOrder);

                expect(mockDomoticzApi.sendCommand).toHaveBeenCalledWith('orderfavorite', {
                    order: deviceOrder
                });
            });
        });

        describe('subscribeToUpdates', function() {
            var mockScope, cleanupFn;

            beforeEach(function() {
                mockScope = $rootScope.$new();
                spyOn(mockScope, '$on').and.callThrough();
            });

            afterEach(function() {
                if (cleanupFn) {
                    cleanupFn();
                }
                mockScope.$destroy();
            });

            it('should register device_update event handler', function() {
                var onDeviceUpdate = jasmine.createSpy('onDeviceUpdate');
                cleanupFn = dashboardService.subscribeToUpdates(mockScope, {
                    onDeviceUpdate: onDeviceUpdate
                });

                expect(mockScope.$on).toHaveBeenCalledWith('device_update', jasmine.any(Function));
            });

            it('should register scene_update event handler', function() {
                var onSceneUpdate = jasmine.createSpy('onSceneUpdate');
                cleanupFn = dashboardService.subscribeToUpdates(mockScope, {
                    onSceneUpdate: onSceneUpdate
                });

                expect(mockScope.$on).toHaveBeenCalledWith('scene_update', jasmine.any(Function));
            });

            it('should call onDeviceUpdate callback when device_update event fires', function() {
                var onDeviceUpdate = jasmine.createSpy('onDeviceUpdate');
                var testDeviceData = { idx: 1, Name: 'Test Device' };

                cleanupFn = dashboardService.subscribeToUpdates(mockScope, {
                    onDeviceUpdate: onDeviceUpdate
                });

                mockScope.$emit('device_update', testDeviceData);
                $rootScope.$digest();

                expect(onDeviceUpdate).toHaveBeenCalledWith(testDeviceData);
            });

            it('should call onSceneUpdate callback when scene_update event fires', function() {
                var onSceneUpdate = jasmine.createSpy('onSceneUpdate');
                var testSceneData = { idx: 5, Name: 'Test Scene' };

                cleanupFn = dashboardService.subscribeToUpdates(mockScope, {
                    onSceneUpdate: onSceneUpdate
                });

                mockScope.$emit('scene_update', testSceneData);
                $rootScope.$digest();

                expect(onSceneUpdate).toHaveBeenCalledWith(testSceneData);
            });

            it('should return cleanup function that unsubscribes handlers', function() {
                var onDeviceUpdate = jasmine.createSpy('onDeviceUpdate');
                cleanupFn = dashboardService.subscribeToUpdates(mockScope, {
                    onDeviceUpdate: onDeviceUpdate
                });

                // Call cleanup
                cleanupFn();

                // Emit event after cleanup - callback should not be called
                mockScope.$emit('device_update', {});
                $rootScope.$digest();

                expect(onDeviceUpdate).not.toHaveBeenCalled();
            });
        });

        describe('refreshDevice', function() {
            it('should call deviceApi.getDevice with correct idx', function() {
                dashboardService.refreshDevice(555);

                expect(mockDeviceApi.getDevice).toHaveBeenCalledWith(555);
            });

            it('should return promise that resolves to device data', function(done) {
                var mockDevice = { idx: 555, Name: 'Refreshed Device' };
                mockDeviceApi.getDevice.and.returnValue($q.resolve(mockDevice));

                dashboardService.refreshDevice(555).then(function(device) {
                    expect(device).toEqual(mockDevice);
                    done();
                });

                $rootScope.$digest();
            });
        });

        describe('refreshScene', function() {
            it('should call sceneApi.getScene with correct idx', function() {
                dashboardService.refreshScene(777);

                expect(mockSceneApi.getScene).toHaveBeenCalledWith(777);
            });

            it('should return promise that resolves to scene data', function(done) {
                var mockScene = { idx: 777, Name: 'Refreshed Scene' };
                mockSceneApi.getScene.and.returnValue($q.resolve(mockScene));

                dashboardService.refreshScene(777).then(function(scene) {
                    expect(scene).toEqual(mockScene);
                    done();
                });

                $rootScope.$digest();
            });
        });
    });
});
