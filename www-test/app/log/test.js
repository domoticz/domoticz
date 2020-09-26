const assert = require('assert');
const requirejs = require('requirejs');
const simple = require('simple-mock');

require('jsdom-global')();

// before(function (done) {
// });
describe('RefreshingDayChart', function () {
    describe('constructor', function () {
        it('should be instantiable', function() {
            function baseParams(jquery) {
                return {
                    jquery: jquery
                };
            }
            function angularParams(location, route, scope, element) {
                return {
                    location: location,
                    route: route,
                    scope: scope,
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
            function chartParams(range, device, chartTitle, autoRefreshIsEnabled) {
                return {
                    range: range,
                    device: device,
                    chartTitle: chartTitle,
                    autoRefreshIsEnabled: autoRefreshIsEnabled
                };
            }
            requirejs(['main.js'], function () {
                requirejs(['jquery'], function($) {
                    const $element = function () {
                    };
                    const $scope = function () {
                    };
                    const $location = {
                        search: function() {
                            return {
                                consoledebug: 'true'
                            };
                        }
                    };
                    const device = {
                        idx: 1234,
                        getUnit: function() {
                            return 'kWh;'
                        }
                    };
                    const domoticzApi = function () {
                    };
                    const domoticzApiResponse = {
                        then: function(handler) {

                        }
                    };
                    const domoticzGlobals = {
                        Get5MinuteHistoryDaysGraphTitle: function() {
                            return 'Get5MinuteHistoryDaysGraphTitle';
                        }
                    };
                    simple.mock($element, 'highcharts').callFn(function (x) {
                        return {
                            highcharts: function() {
                                return {
                                    container: {
                                        onmousedown: function() {

                                        }
                                    }
                                };
                            }
                        };
                    });
                    simple.mock(domoticzApi, 'sendRequest').returnWith(domoticzApiResponse);
                    simple.mock($scope, '$on').callFn(function (eventType, handler) { console.log('installed:'+handler+' for '+eventType); });
                    simple.mock($, 't').callFn(function (text) { return text; });
                    requirejs(['RefreshingChart', 'RefreshingSingleChart'], function (RefreshingChart, RefreshingSingleChart) {
                        const sut = new RefreshingSingleChart(
                            baseParams($),
                            angularParams($location, null, $scope, $element),
                            domoticzParams(domoticzGlobals, domoticzApi, null),
                            chartParams('day', device, 'Chart Title', function() { return true; })
                        );
                    });
                    requirejs(['RefreshingMinMaxAvgChart'], function (RefreshingMinMaxAvgChart) {
                        const sut = new RefreshingMinMaxAvgChart(
                            baseParams($),
                            angularParams($location, null, $scope, $element),
                            domoticzParams(domoticzGlobals, domoticzApi,null),
                            chartParams('day', device, 'Chart Title', function() { return true; })
                        );
                    });
                });
            });
        })
    });
});