define(['app'], function (app) {

    const deviceTypes = {
        EnergyUsed: 0,
        Gas: 1,
        Water: 2,
        Counter: 3,
        EnergyGenerated: 4,
        Time: 5,
    };

    const valueMultipliers = {
        m1: 'm1',
        m1000: 'm1000',
        forRange: function (range) {
            return range === 'day' ? this.m1
                : range === 'week' ? this.m1000
                    : null;
        }
    }

    const valueUnits = {
        Wh: 'Wh',
        kWh: 'kWh',
        W: 'Watt',
        kW: 'kW',
        m3: 'm³',
        energy: function (multiplier) {
            if (multiplier === valueMultipliers.m1) {
                return this.Wh;
            }
            if (multiplier === valueMultipliers.m1000) {
                return this.kWh;
            }
            return '';
        },
        power: function (multiplier) {
            if (multiplier === valueMultipliers.m1) {
                return this.W;
            }
            if (multiplier === valueMultipliers.m1000) {
                return this.kW;
            }
            return '';
        },
        gas: function (multiplier) {
            if (multiplier === valueMultipliers.m1) {
                return this.m3;
            }
            return '';
        },
        water: function (multiplier) {
            if (multiplier === valueMultipliers.m1) {
                return this.m3;
            }
            return '';
        }
    }

    app.constant('chart', {
        baseParams: baseParams,
        angularParams: angularParams,
        domoticzParams: domoticzParams,
        deviceTypes: deviceTypes,
        valueMultipliers: valueMultipliers,
        valueUnits: valueUnits
    });

    function baseParams(jquery) {
        return {
            jquery: jquery
        };
    }

    function angularParams(location, route, scope, timeout, element) {
        return {
            location: location,
            route: route,
            scope: scope,
            timeout: timeout,
            element: element
        };
    }

    function domoticzParams(globals, api, datapointApi) {
        return {
            globals: globals,
            api: api,
            datapointApi: datapointApi
        };
    }
});