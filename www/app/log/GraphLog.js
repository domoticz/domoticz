define(['app', 'RefreshingChart'],
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

        app.component('deviceShortChart', {
            require: {
                logCtrl: '^deviceGraphLog'
            },
            bindings: {
                device: '<'
            },
            templateUrl: 'app/log/chart-day.html',
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi) {
                const self = this;
                self.range = 'day';

                self.$onInit = function () {
                    new RefreshingChart(
                        baseParams($),
                        angularParams($location, $route, $scope, $timeout, $element),
                        domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        chartParams(
                            domoticzGlobals,
                            self,
                            true, function (dataItem, yearOffset = 0) {
                                return GetLocalDateTimeFromString(dataItem.d, yearOffset);
                            },
                            [
                                {
                                    id: 'power',
                                    valueKeySuffix: '',
                                    colorIndex: 0,
                                    template: {
                                        name: domoticzGlobals.sensorNameForDevice(self.device),
                                        showInLegend: false
                                    }
                                }
                            ]
                        )
                    );
                }
            }
        });

        app.component('deviceLongChart', {
            require: {
                logCtrl: '^deviceGraphLog'
            },
            bindings: {
                device: '<',
                range: '@'
            },
            templateUrl: function($element, $attrs) { return 'app/log/chart-' + $attrs.range + '.html'; },
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi) {
                const self = this;

                self.$onInit = function () {
                    new RefreshingChart(
                        baseParams($),
                        angularParams($location, $route, $scope, $timeout, $element),
                        domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        chartParams(
                            domoticzGlobals,
                            self,
                            false,
                            function (dataItem, yearOffset = 0) {
                                return GetLocalDateFromString(dataItem.d, yearOffset);
                            },
                            [
                                {
                                    id: 'min',
                                    valueKeySuffix: '_min',
                                    colorIndex: 3,
                                    template: {
                                        name: $.t('Minimum')
                                    }
                                },
                                {
                                    id: 'max',
                                    valueKeySuffix: '_max',
                                    colorIndex: 2,
                                    template: {
                                        name: $.t('Maximum')
                                    }
                                },
                                {
                                    id: 'avg',
                                    valueKeySuffix: '_avg',
                                    colorIndex: 0,
                                    template: {
                                        name: $.t('Average')
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
        function chartParams(domoticzGlobals, ctrl, isShortLogChart, timestampFromDataItem, seriesSuppliers) {
            return {
                range: ctrl.range,
                device: ctrl.device,
				sensorType: domoticzGlobals.sensorTypeForDevice(ctrl.device),
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
                                return parseFloat(dataItem[valueKey + seriesSupplier.valueKeySuffix]);
                            }
                        ];
                        return seriesSupplier;
                    })
                }
            };
        }
    }
);
