define(['app'], function (app) {

    const deviceTypes = {
        EnergyUsed: 0,
        Gas: 1,
        Water: 2,
        Counter: 3,
        EnergyGenerated: 4,
        Time: 5,
    };

    app.constant('chart', {
        baseParams: baseParams,
        angularParams: angularParams,
        domoticzParams: domoticzParams,
        deviceTypes: deviceTypes
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