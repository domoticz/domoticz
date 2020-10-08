define(['app', 'RefreshingChart', 'log/factories'],
    function (app, RefreshingChart) {

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

        app.component('shortChart', {
            require: {
                logCtrl: '^deviceGraphLog'
            },
            bindings: {
                device: '<'
            },
            template: '<div></div>',
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi) {
                const self = this;
                self.range = 'day';

                self.$onInit = function () {
                    new RefreshingChart(
                        baseParams($),
                        angularParams($location, $route, $scope, $element),
                        domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        chartParams(
                            domoticzGlobals,
                            self,
                            domoticzGlobals.Get5MinuteHistoryDaysGraphTitle(),
                            true,
                            function (dataItem, yearOffset=0) {
                                return GetLocalDateTimeFromString(dataItem.d, yearOffset);
                            },
                            [
                                {
                                    id: 'power',
                                    name: domoticzGlobals.sensorNameForDevice(self.device),
                                    valueKeySuffix: '',
                                    template: {
                                        showInLegend: false,
                                        colorIndex: 0
                                    }
                                }
                            ]
                        )
                    );
                }
            }
        });

        app.component('longChart', {
            require: {
                logCtrl: '^deviceGraphLog'
            },
            bindings: {
                device: '<',
                chartTitle: '@',
                range: '@'
            },
            template: '<div></div>',
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi) {
                const self = this;

                self.$onInit = function () {
                    new RefreshingChart(
                        baseParams($),
                        angularParams($location, $route, $scope, $element),
                        domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        chartParams(
                            domoticzGlobals,
                            self,
                            $.t(self.chartTitle),
                            false,
                            function (dataItem, yearOffset=0) {
                                return GetLocalDateFromString(dataItem.d, yearOffset);
                            },
                            [
                                {
                                    id: 'min',
                                    name: 'min',
                                    valueKeySuffix: '_min',
                                    template: {
                                        colorIndex: 3
                                    }
                                },
                                {
                                    id: 'max',
                                    name: 'max',
                                    valueKeySuffix: '_max',
                                    template: {
                                        colorIndex: 2
                                    }
                                },
                                {
                                    id: 'avg',
                                    name: 'avg',
                                    valueKeySuffix: '_avg',
                                    template: {
                                        colorIndex: 0
                                    }
                                }
                            ]
                        )
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
        function chartParams(domoticzGlobals, ctrl, chartTitle, isShortLogChart, timestampFromDataItem, seriesSuppliers) {
            return {
                range: ctrl.range,
                device: ctrl.device,
				sensorType: domoticzGlobals.sensorTypeForDevice(ctrl.device),
                chartTitle: chartTitle,
                autoRefreshIsEnabled: function() { return ctrl.logCtrl.autoRefresh; },
                dataSupplier: {
                    yAxes: [
                        {
                            title: {
                                text: domoticzGlobals.axisTitleForDevice(ctrl.device)
                            },
                            labels: {
                                formatter: function () {
                                    return ctrl.device.getUnit() === 'vM' ? Highcharts.numberFormat(this.value, 0) : this.value;
                                }
                            }
                        }
                    ],
                    valueSuffix: ' ' + ctrl.device.getUnit(),
                    timestampFromDataItem: timestampFromDataItem,
                    isShortLogChart: isShortLogChart,
                    seriesSuppliers: seriesSuppliers.map(function (seriesSupplier) {
                        const valueKey = domoticzGlobals.valueKeyForDevice(ctrl.device);
                        seriesSupplier.dataItemIsValid = function (dataItem) {
                            return dataItem[valueKey + seriesSupplier.valueKeySuffix] !== undefined;
                        }
                        seriesSupplier.valuesFromDataItem = [
                            function (dataItem) {
                                return dataItem[valueKey + seriesSupplier.valueKeySuffix];
                            }
                        ];
                        return seriesSupplier;
                    })
                }
            };
        }
    }
);
