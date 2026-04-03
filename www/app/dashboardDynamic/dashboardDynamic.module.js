define(['app'], function(app) {
    'use strict';

    /**
     * Dashboard 2.0 AngularJS module.
     * All dashboardDynamic services, directives, and controllers are registered here.
     */

    // Module-level constant: supported grid columns
    app.constant('DD_GRID_COLUMNS', 12);

    // Module-level constant: list of all widget types (used by Feature 05)
    app.constant('DD_WIDGET_CATALOG', []);  // populated by Feature 05

    /**
     * ddDeviceClassifier
     *
     * Determines which existing AngularJS widget directive should render a
     * given device object returned by the /json.htm API.
     *
     * Returns one of:
     *   'dz-light-widget'   — switches, lights, blinds, color devices, fans, …
     *   'dz-scene-widget'   — scenes and groups
     *   'dz-utility-widget' — temperature, humidity, weather, meters, counters, …
     */
    app.factory('ddDeviceClassifier', [function() {

        function getDirective(device) {
            if (!device) { return null; }

            var type = device.Type || '';

            // Scenes and Groups
            if (type.indexOf('Scene') === 0 || type.indexOf('Group') === 0) {
                return 'dz-scene-widget';
            }

            // Light / Switch / Blind / Color / RFY / Fan / Chime / Security / Thermostat
            if (
                type.indexOf('Light')       === 0 ||
                type.indexOf('Blind')       === 0 ||
                type.indexOf('Curtain')     === 0 ||
                type.indexOf('Color Switch') === 0 ||
                type.indexOf('Chime')       === 0 ||
                type.indexOf('Thermostat 2') === 0 ||
                type.indexOf('Thermostat 3') === 0 ||
                type.indexOf('RFY')         === 0 ||
                type.indexOf('ASA')         === 0 ||
                type.indexOf('Fan')         === 0 ||
                type === 'Security'
            ) {
                return 'dz-light-widget';
            }

            // Everything else (Temp, Humidity, Weather, Utility, Energy, …)
            return 'dz-utility-widget';
        }

        return { getDirective: getDirective };
    }]);

    // Small helper directive: auto-focuses an input when it appears in the DOM
    app.directive('ddAutofocus', ['$timeout', function($timeout) {
        return {
            restrict: 'A',
            link: function(scope, element) {
                $timeout(function() { element[0].focus(); element[0].select(); }, 30);
            }
        };
    }]);

    return app;
});
