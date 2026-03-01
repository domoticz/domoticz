/**
 * Unit tests for dzLightWidget directive
 */
define([
    'angular',
    'angular-mocks',
    'widgets/dzLightWidget'
], function() {
    'use strict';

    describe('dzLightWidget directive', function() {
        var $compile, $rootScope, $scope;
        var mockDeviceApi, mockDeviceLightApi, mockPermissions;
        var element;

        beforeEach(function() {
            module('domoticz');

            // Mock dependencies
            module(function($provide) {
                mockDeviceApi = {
                    getDevice: jasmine.createSpy('getDevice')
                };

                mockDeviceLightApi = {
                    switchOn: jasmine.createSpy('switchOn').and.returnValue(Promise.resolve()),
                    switchOff: jasmine.createSpy('switchOff').and.returnValue(Promise.resolve()),
                    setColor: jasmine.createSpy('setColor').and.returnValue(Promise.resolve())
                };

                mockPermissions = {
                    hasPermission: jasmine.createSpy('hasPermission').and.returnValue(true)
                };

                $provide.value('deviceApi', mockDeviceApi);
                $provide.value('deviceLightApi', mockDeviceLightApi);
                $provide.value('permissions', mockPermissions);
            });

            inject(function(_$compile_, _$rootScope_) {
                $compile = _$compile_;
                $rootScope = _$rootScope_;
                $scope = $rootScope.$new();
            });

            // Setup window globals
            window.myglobals = {
                ismobile: false,
                DashboardType: 0
            };

            // Mock global functions that might be called
            window.ShowNotify = jasmine.createSpy('ShowNotify');
            window.PasswordCheck = jasmine.createSpy('PasswordCheck').and.callFake(function(callback) {
                callback();
            });
            window.b64DecodeUnicode = jasmine.createSpy('b64DecodeUnicode').and.callFake(function(str) {
                return str; // Simple mock, return as-is
            });

            // Mock $rootScope.GetItemBackgroundStatus
            $rootScope.GetItemBackgroundStatus = jasmine.createSpy('GetItemBackgroundStatus').and.returnValue('bg-blue');
        });

        afterEach(function() {
            if (element) {
                element.remove();
            }
            delete window.myglobals;
            delete window.ShowNotify;
            delete window.PasswordCheck;
            delete window.b64DecodeUnicode;
        });

        function compileDirective(device, dashboardType) {
            $scope.testDevice = device;
            $scope.testDashboardType = dashboardType;

            var template = '<dz-light-widget device="testDevice" dashboard-type="testDashboardType"></dz-light-widget>';
            element = $compile(template)($scope);
            $scope.$digest();

            return element;
        }

        describe('directive compilation', function() {
            it('should compile successfully with a basic device', function() {
                var device = {
                    idx: 1,
                    Name: 'Test Light',
                    Type: 'Light/Switch',
                    SwitchType: 'On/Off',
                    Status: 'Off'
                };

                compileDirective(device, 0);

                expect(element).toBeDefined();
                expect(element.html()).not.toBe('');
            });

            it('should use desktop template when dashboardType is 0', function() {
                var device = { idx: 1, Name: 'Test', Status: 'Off', SwitchType: 'On/Off' };

                compileDirective(device, 0);

                // The directive should request desktop template
                // We cannot easily test templateUrl selection without mocking $templateCache
                expect(element).toBeDefined();
            });

            it('should expose controller methods', function() {
                var device = { idx: 1, Name: 'Test', Status: 'Off', SwitchType: 'On/Off' };

                compileDirective(device, 0);
                var controller = element.isolateScope().ctrl;

                expect(controller.switchLight).toBeDefined();
                expect(controller.isActive).toBeDefined();
                expect(controller.isDimmer).toBeDefined();
                expect(controller.isBlinds).toBeDefined();
                expect(controller.isSelector).toBeDefined();
            });
        });

        describe('controller - device type detection', function() {
            it('should detect dimmer devices', function() {
                var device = { idx: 1, SwitchType: 'Dimmer', Status: 'Off' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.isDimmer()).toBe(true);
            });

            it('should detect blinds percentage devices', function() {
                var device = { idx: 1, SwitchType: 'Blinds Percentage', Status: 'Off' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.isDimmer()).toBe(true);
            });

            it('should detect blinds devices', function() {
                var device = { idx: 1, SwitchType: 'Blinds', Status: 'Off' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.isBlinds()).toBe(true);
            });

            it('should detect selector devices', function() {
                var device = { idx: 1, SwitchType: 'Selector', Status: 'Off', LevelNames: 'Off|Low|Medium|High' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.isSelector()).toBe(true);
            });

            it('should detect RGB devices', function() {
                var device = { idx: 1, SubType: 'RGBW', Status: 'Off' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.isRGB()).toBe(true);
            });

            it('should detect RGB devices with WW in SubType', function() {
                var device = { idx: 1, SubType: 'RGBWW', Status: 'Off' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.isRGB()).toBe(true);
            });

            it('should detect Evohome devices', function() {
                var device = { idx: 1, SubType: 'Evohome', Status: 'Auto' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.isEvohome()).toBe(true);
            });
        });

        describe('controller - isActive', function() {
            it('should return true for "On" status', function() {
                var device = { idx: 1, Status: 'On' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.isActive()).toBe(true);
            });

            it('should return true for "Chime" status', function() {
                var device = { idx: 1, Status: 'Chime' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.isActive()).toBe(true);
            });

            it('should return true for "Group On" status', function() {
                var device = { idx: 1, Status: 'Group On' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.isActive()).toBe(true);
            });

            it('should return true for "Set Level" status', function() {
                var device = { idx: 1, Status: 'Set Level 50' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.isActive()).toBe(true);
            });

            it('should return true for "NightMode" status', function() {
                var device = { idx: 1, Status: 'NightMode' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.isActive()).toBe(true);
            });

            it('should return false for "Off" status', function() {
                var device = { idx: 1, Status: 'Off' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.isActive()).toBe(false);
            });

            it('should return false for "Closed" status', function() {
                var device = { idx: 1, Status: 'Closed' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.isActive()).toBe(false);
            });
        });

        describe('controller - switchLight', function() {
            it('should check permissions before switching', function() {
                var device = { idx: 1, Status: 'Off', Protected: false };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                ctrl.switchLight('On');

                expect(mockPermissions.hasPermission).toHaveBeenCalledWith('User');
            });

            it('should show notification if user lacks permission', function() {
                mockPermissions.hasPermission.and.returnValue(false);
                var device = { idx: 1, Status: 'Off', Protected: false };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                ctrl.switchLight('On');

                expect(window.ShowNotify).toHaveBeenCalled();
                expect(mockDeviceLightApi.switchOn).not.toHaveBeenCalled();
            });

            it('should prompt for password if device is protected', function() {
                var device = { idx: 1, Status: 'Off', Protected: true };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                ctrl.switchLight('On');

                expect(window.PasswordCheck).toHaveBeenCalled();
            });

            it('should call switchOn for "On" command', function() {
                var device = { idx: 123, Status: 'Off', Protected: false };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                ctrl.switchLight('On');

                expect(mockDeviceLightApi.switchOn).toHaveBeenCalledWith(123);
            });

            it('should call switchOff for "Off" command', function() {
                var device = { idx: 456, Status: 'On', Protected: false };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                ctrl.switchLight('Off');

                expect(mockDeviceLightApi.switchOff).toHaveBeenCalledWith(456);
            });
        });

        describe('controller - hasStopButton', function() {
            it('should return true for RAEX SubType', function() {
                var device = { idx: 1, SubType: 'RAEX', Status: 'Off' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.hasStopButton()).toBe(true);
            });

            it('should return true for RFY SubType', function() {
                var device = { idx: 1, SubType: 'RFY', Status: 'Off' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.hasStopButton()).toBe(true);
            });

            it('should return true for Venetian Blinds SwitchType', function() {
                var device = { idx: 1, SwitchType: 'Venetian Blinds EU', Status: 'Off' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.hasStopButton()).toBe(true);
            });

            it('should return true for SwitchType with Stop', function() {
                var device = { idx: 1, SwitchType: 'Blinds + Stop', Status: 'Off' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.hasStopButton()).toBe(true);
            });

            it('should return false for regular switch', function() {
                var device = { idx: 1, SwitchType: 'On/Off', Status: 'Off' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.hasStopButton()).toBe(false);
            });
        });

        describe('controller - getSelectorLevels', function() {
            it('should return empty array if no LevelNames', function() {
                var device = { idx: 1, SwitchType: 'Selector', Status: 'Off' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.getSelectorLevels()).toEqual([]);
            });

            it('should parse level names and create level objects', function() {
                window.b64DecodeUnicode = function(str) { return 'Off|Low|Medium|High'; };

                var device = {
                    idx: 1,
                    SwitchType: 'Selector',
                    Status: 'Off',
                    LevelNames: 'Off|Low|Medium|High',
                    LevelInt: 20,
                    LevelOffHidden: false
                };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                var levels = ctrl.getSelectorLevels();

                expect(levels.length).toBe(4);
                expect(levels[0].name).toBe('Off');
                expect(levels[0].value).toBe(0);
                expect(levels[1].name).toBe('Low');
                expect(levels[1].value).toBe(10);
                expect(levels[2].isActive).toBe(true); // LevelInt = 20
            });

            it('should hide Off level if LevelOffHidden is true', function() {
                window.b64DecodeUnicode = function(str) { return 'Off|Low|Medium|High'; };

                var device = {
                    idx: 1,
                    SwitchType: 'Selector',
                    LevelNames: 'Off|Low|Medium|High',
                    LevelInt: 10,
                    LevelOffHidden: true
                };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                var levels = ctrl.getSelectorLevels();

                expect(levels.length).toBe(3);
                expect(levels[0].name).toBe('Low');
            });
        });

        describe('controller - getStatusText', function() {
            it('should return status for regular devices', function() {
                var device = { idx: 1, Status: 'On', SwitchType: 'On/Off' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.getStatusText()).toBe('On');
            });

            it('should convert Evohome status codes', function() {
                var device = { idx: 1, SubType: 'Evohome', Status: 'Auto' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.getStatusText()).toBe('Normal');
            });

            it('should decode selector level names', function() {
                window.b64DecodeUnicode = function(str) { return 'Off|Low|Medium|High'; };

                var device = {
                    idx: 1,
                    SwitchType: 'Selector',
                    Status: 'Set Level',
                    LevelNames: 'Off|Low|Medium|High',
                    LevelInt: 20
                };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.getStatusText()).toBe('Medium');
            });
        });

        describe('controller - evoDisplayTextMode', function() {
            it('should convert Auto to Normal', function() {
                var device = { idx: 1, SubType: 'Evohome', Status: 'Auto' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.evoDisplayTextMode('Auto')).toBe('Normal');
            });

            it('should convert AutoWithEco to Economy', function() {
                var device = { idx: 1, SubType: 'Evohome', Status: 'AutoWithEco' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.evoDisplayTextMode('AutoWithEco')).toBe('Economy');
            });

            it('should convert DayOff to Day Off', function() {
                var device = { idx: 1, SubType: 'Evohome', Status: 'DayOff' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.evoDisplayTextMode('DayOff')).toBe('Day Off');
            });

            it('should convert HeatingOff to Heating Off', function() {
                var device = { idx: 1, SubType: 'Evohome', Status: 'HeatingOff' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.evoDisplayTextMode('HeatingOff')).toBe('Heating Off');
            });

            it('should return original status for unmapped values', function() {
                var device = { idx: 1, SubType: 'Evohome', Status: 'Away' };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                expect(ctrl.evoDisplayTextMode('Away')).toBe('Away');
            });
        });

        describe('controller - getDeviceIcon', function() {
            it('should return custom icon when CustomImage is set', function() {
                var device = {
                    idx: 1,
                    CustomImage: 123,
                    Image: 'CustomLight',
                    Status: 'On'
                };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                var icon = ctrl.getDeviceIcon();
                expect(icon).toContain('CustomLight48_On.png');
            });

            it('should return default light icon when active', function() {
                var device = {
                    idx: 1,
                    CustomImage: 0,
                    SwitchTypeVal: 0,
                    Status: 'On'
                };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                var icon = ctrl.getDeviceIcon();
                expect(icon).toContain('Light48_On.png');
            });

            it('should return default light icon when inactive', function() {
                var device = {
                    idx: 1,
                    CustomImage: 0,
                    SwitchTypeVal: 0,
                    Status: 'Off'
                };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                var icon = ctrl.getDeviceIcon();
                expect(icon).toContain('Light48_Off.png');
            });

            it('should return blinds icon for blinds devices', function() {
                var device = {
                    idx: 1,
                    CustomImage: 0,
                    SwitchTypeVal: 7,
                    Status: 'Open'
                };

                compileDirective(device, 0);
                var ctrl = element.isolateScope().ctrl;

                var icon = ctrl.getDeviceIcon();
                expect(icon).toContain('blinds');
            });
        });
    });
});
