/**
 * Unit tests for deviceDetection service
 */
define([
    'angular',
    'angular-mocks',
    'services/deviceDetection'
], function() {
    'use strict';

    describe('deviceDetection', function() {
        var deviceDetection;
        var originalNavigator;
        var originalLocalStorage;

        beforeEach(function() {
            module('domoticz');

            inject(function(_deviceDetection_) {
                deviceDetection = _deviceDetection_;
            });

            // Save original navigator and localStorage
            originalNavigator = navigator.userAgent;
            originalLocalStorage = window.localStorage;

            // Clear localStorage
            localStorage.clear();

            // Clear window.myglobals
            delete window.myglobals;
        });

        afterEach(function() {
            // Restore localStorage
            window.localStorage = originalLocalStorage;

            // Clear myglobals
            delete window.myglobals;
        });

        describe('isMobile', function() {
            it('should detect iPhone as mobile', function() {
                // Mock navigator.userAgent
                Object.defineProperty(navigator, 'userAgent', {
                    value: 'Mozilla/5.0 (iPhone; CPU iPhone OS 14_0 like Mac OS X) AppleWebKit/605.1.15',
                    writable: true,
                    configurable: true
                });

                expect(deviceDetection.isMobile()).toBe(true);

                // Restore
                Object.defineProperty(navigator, 'userAgent', {
                    value: originalNavigator,
                    writable: true,
                    configurable: true
                });
            });

            it('should detect Android mobile as mobile', function() {
                Object.defineProperty(navigator, 'userAgent', {
                    value: 'Mozilla/5.0 (Linux; Android 10; SM-G973F) AppleWebKit/537.36 Mobile Safari/537.36',
                    writable: true,
                    configurable: true
                });

                expect(deviceDetection.isMobile()).toBe(true);

                Object.defineProperty(navigator, 'userAgent', {
                    value: originalNavigator,
                    writable: true,
                    configurable: true
                });
            });

            it('should not detect desktop as mobile', function() {
                Object.defineProperty(navigator, 'userAgent', {
                    value: 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 Chrome/91.0.4472.124',
                    writable: true,
                    configurable: true
                });

                expect(deviceDetection.isMobile()).toBe(false);

                Object.defineProperty(navigator, 'userAgent', {
                    value: originalNavigator,
                    writable: true,
                    configurable: true
                });
            });

            it('should not detect iPad as mobile (it is tablet)', function() {
                Object.defineProperty(navigator, 'userAgent', {
                    value: 'Mozilla/5.0 (iPad; CPU OS 14_0 like Mac OS X) AppleWebKit/605.1.15',
                    writable: true,
                    configurable: true
                });

                expect(deviceDetection.isMobile()).toBe(false);

                Object.defineProperty(navigator, 'userAgent', {
                    value: originalNavigator,
                    writable: true,
                    configurable: true
                });
            });
        });

        describe('isTablet', function() {
            it('should detect iPad as tablet', function() {
                Object.defineProperty(navigator, 'userAgent', {
                    value: 'Mozilla/5.0 (iPad; CPU OS 14_0 like Mac OS X) AppleWebKit/605.1.15',
                    writable: true,
                    configurable: true
                });

                expect(deviceDetection.isTablet()).toBe(true);

                Object.defineProperty(navigator, 'userAgent', {
                    value: originalNavigator,
                    writable: true,
                    configurable: true
                });
            });

            it('should detect Android tablet as tablet', function() {
                Object.defineProperty(navigator, 'userAgent', {
                    value: 'Mozilla/5.0 (Linux; Android 9; SM-T830) AppleWebKit/537.36 Safari/537.36',
                    writable: true,
                    configurable: true
                });

                expect(deviceDetection.isTablet()).toBe(true);

                Object.defineProperty(navigator, 'userAgent', {
                    value: originalNavigator,
                    writable: true,
                    configurable: true
                });
            });

            it('should not detect mobile phone as tablet', function() {
                Object.defineProperty(navigator, 'userAgent', {
                    value: 'Mozilla/5.0 (iPhone; CPU iPhone OS 14_0 like Mac OS X)',
                    writable: true,
                    configurable: true
                });

                expect(deviceDetection.isTablet()).toBe(false);

                Object.defineProperty(navigator, 'userAgent', {
                    value: originalNavigator,
                    writable: true,
                    configurable: true
                });
            });

            it('should not detect desktop as tablet', function() {
                Object.defineProperty(navigator, 'userAgent', {
                    value: 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/91.0',
                    writable: true,
                    configurable: true
                });

                expect(deviceDetection.isTablet()).toBe(false);

                Object.defineProperty(navigator, 'userAgent', {
                    value: originalNavigator,
                    writable: true,
                    configurable: true
                });
            });
        });

        describe('getDeviceType', function() {
            it('should return "mobile" for mobile devices', function() {
                Object.defineProperty(navigator, 'userAgent', {
                    value: 'Mozilla/5.0 (iPhone; CPU iPhone OS 14_0 like Mac OS X)',
                    writable: true,
                    configurable: true
                });

                expect(deviceDetection.getDeviceType()).toBe('mobile');

                Object.defineProperty(navigator, 'userAgent', {
                    value: originalNavigator,
                    writable: true,
                    configurable: true
                });
            });

            it('should return "tablet" for tablet devices', function() {
                Object.defineProperty(navigator, 'userAgent', {
                    value: 'Mozilla/5.0 (iPad; CPU OS 14_0 like Mac OS X)',
                    writable: true,
                    configurable: true
                });

                expect(deviceDetection.getDeviceType()).toBe('tablet');

                Object.defineProperty(navigator, 'userAgent', {
                    value: originalNavigator,
                    writable: true,
                    configurable: true
                });
            });

            it('should return "desktop" for desktop devices', function() {
                Object.defineProperty(navigator, 'userAgent', {
                    value: 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/91.0',
                    writable: true,
                    configurable: true
                });

                expect(deviceDetection.getDeviceType()).toBe('desktop');

                Object.defineProperty(navigator, 'userAgent', {
                    value: originalNavigator,
                    writable: true,
                    configurable: true
                });
            });
        });

        describe('getEffectiveType', function() {
            beforeEach(function() {
                // Set desktop user agent by default
                Object.defineProperty(navigator, 'userAgent', {
                    value: 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/91.0',
                    writable: true,
                    configurable: true
                });
            });

            afterEach(function() {
                Object.defineProperty(navigator, 'userAgent', {
                    value: originalNavigator,
                    writable: true,
                    configurable: true
                });
            });

            it('should prioritize localStorage override over everything else', function() {
                localStorage.setItem('dashboardViewOverride', 'mobile');
                window.myglobals = { DashboardType: 0, ismobile: false };

                expect(deviceDetection.getEffectiveType()).toBe('mobile');
            });

            it('should respect desktop override in localStorage', function() {
                localStorage.setItem('dashboardViewOverride', 'desktop');

                // Even with mobile user agent
                Object.defineProperty(navigator, 'userAgent', {
                    value: 'Mozilla/5.0 (iPhone; CPU iPhone OS 14_0 like Mac OS X)',
                    writable: true,
                    configurable: true
                });

                expect(deviceDetection.getEffectiveType()).toBe('desktop');

                Object.defineProperty(navigator, 'userAgent', {
                    value: originalNavigator,
                    writable: true,
                    configurable: true
                });
            });

            it('should use server DashboardType when no override exists', function() {
                window.myglobals = { DashboardType: 2 };

                expect(deviceDetection.getEffectiveType()).toBe('mobile');
            });

            it('should return desktop for DashboardType 0', function() {
                window.myglobals = { DashboardType: 0 };

                expect(deviceDetection.getEffectiveType()).toBe('desktop');
            });

            it('should return desktop for DashboardType 1', function() {
                window.myglobals = { DashboardType: 1 };

                expect(deviceDetection.getEffectiveType()).toBe('desktop');
            });

            it('should return desktop for DashboardType 3 (floorplan)', function() {
                window.myglobals = { DashboardType: 3 };

                expect(deviceDetection.getEffectiveType()).toBe('desktop');
            });

            it('should check legacy ismobile flag', function() {
                window.myglobals = { ismobile: true };

                expect(deviceDetection.getEffectiveType()).toBe('mobile');
            });

            it('should fall back to user-agent detection', function() {
                // No override, no server config
                Object.defineProperty(navigator, 'userAgent', {
                    value: 'Mozilla/5.0 (iPhone; CPU iPhone OS 14_0 like Mac OS X)',
                    writable: true,
                    configurable: true
                });

                expect(deviceDetection.getEffectiveType()).toBe('mobile');

                Object.defineProperty(navigator, 'userAgent', {
                    value: originalNavigator,
                    writable: true,
                    configurable: true
                });
            });

            it('should return desktop for tablet devices in user-agent fallback', function() {
                Object.defineProperty(navigator, 'userAgent', {
                    value: 'Mozilla/5.0 (iPad; CPU OS 14_0 like Mac OS X)',
                    writable: true,
                    configurable: true
                });

                expect(deviceDetection.getEffectiveType()).toBe('desktop');

                Object.defineProperty(navigator, 'userAgent', {
                    value: originalNavigator,
                    writable: true,
                    configurable: true
                });
            });
        });

        describe('setOverride', function() {
            it('should save mobile override to localStorage', function() {
                deviceDetection.setOverride('mobile');

                expect(localStorage.getItem('dashboardViewOverride')).toBe('mobile');
            });

            it('should save desktop override to localStorage', function() {
                deviceDetection.setOverride('desktop');

                expect(localStorage.getItem('dashboardViewOverride')).toBe('desktop');
            });

            it('should not save invalid override values', function() {
                deviceDetection.setOverride('tablet');

                expect(localStorage.getItem('dashboardViewOverride')).toBeNull();
            });

            it('should not save invalid override values like "auto"', function() {
                deviceDetection.setOverride('auto');

                expect(localStorage.getItem('dashboardViewOverride')).toBeNull();
            });
        });

        describe('clearOverride', function() {
            it('should remove override from localStorage', function() {
                localStorage.setItem('dashboardViewOverride', 'mobile');

                deviceDetection.clearOverride();

                expect(localStorage.getItem('dashboardViewOverride')).toBeNull();
            });

            it('should not throw error if no override exists', function() {
                expect(function() {
                    deviceDetection.clearOverride();
                }).not.toThrow();
            });
        });

        describe('getOverride', function() {
            it('should return current override value', function() {
                localStorage.setItem('dashboardViewOverride', 'mobile');

                expect(deviceDetection.getOverride()).toBe('mobile');
            });

            it('should return null if no override is set', function() {
                expect(deviceDetection.getOverride()).toBeNull();
            });
        });
    });
});
