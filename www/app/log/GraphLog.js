define(['app', 'RefreshingSingleChart', 'RefreshingMinMaxAvgChart'],
    function (app, RefreshingSingleChart, RefreshingMinMaxAvgChart) {

        app.component('deviceGraphLog', {
            bindings: {
                device: '<'
            },
            templateUrl: 'app/log/GraphLog.html',
            controllerAs: '$ctrl',
            controller: function () {
                const $ctrl = this;
                $ctrl.autoRefresh = true;
            }
        });

        app.component('refreshingSingleChart', {
            require: {
                logCtrl: '^deviceGraphLog'
            },
            bindings: {
                device: '<',
                deviceType: '<',
                degreeType: '<',
                range: '@'
            },
            template: '<div></div>',
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $element, domoticzDataPointApi, domoticzApi, domoticzGlobals) {

                const self = this;

                self.$onInit = function () {
                    new RefreshingSingleChart(
                        baseParams($),
                        angularParams($location, $route, $scope, $element),
                        domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        chartParams(
                            self.range,
                            self.device,
                            self.domoticzGlobals.Get5MinuteHistoryDaysGraphTitle(),
                            function() { return self.logCtrl.autoRefresh; })
                    );
                }
            }
        });

        app.component('refreshingMinMaxAvgChart', {
            require: {
                logCtrl: '^deviceGraphLog'
            },
            bindings: {
                device: '<',
                deviceType: '<',
                degreeType: '<',
                range: '@',
                chartTitle: '@'
            },
            template: '<div></div>',
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $element, domoticzDataPointApi, domoticzApi, domoticzGlobals) {

                const self = this;

                self.$onInit = function () {
                    new RefreshingMinMaxAvgChart(
                        baseParams($),
                        angularParams($location, $route, $scope, $element),
                        domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        chartParams(
                            self.range,
                            self.device,
                            $.t(self.chartTitle),
                            function() { return self.logCtrl.autoRefresh; })
                    );
                }
            }
        });

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
    });
});
