define(['app', 'RefreshingChart'],
    function (app, RefreshingChart) {

        const deviceTypes = {
            EnergyUsed: 0,
            Gas: 1,
            Water: 2,
            Counter: 3,
            EnergyGenerated: 4,
            Time: 5,
        };

        app.component('deviceCounterLog', {
            bindings: {
                device: '<'
            },
            templateUrl: 'app/log/CounterLog.html',
            controllerAs: '$ctrl',
            controller: function () {
                const $ctrl = this;
                $ctrl.autoRefresh = true;
            }
        });

        app.component('counterTimeChart', {
            require: {
                logCtrl: '^deviceCounterLog'
            },
            bindings: {
                device: '<'
            },
            templateUrl: 'app/log/chart-day.html',
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi) {
                const self = this;
                self.range = 'day';

                const seriesTypes = {
                    Counter: 'Counter',
                    InstantAndCounter: 'InstantAndCounter',
                    P1: 'P1'
                };

                self.$onInit = function () {
                    const chart = new RefreshingChart(
                        baseParams($),
                        angularParams($location, $route, $scope, $element),
                        domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        chartParams(
                            domoticzGlobals,
                            self,
                            {
                                isShortLogChart: true,
                                timestampFromDataItem: function (dataItem, yearOffset = 0) {
                                    return GetLocalDateTimeFromString(dataItem.d, yearOffset);
                                }
                            },
                            [
                                function(dataSupplier) {
                                    const seriesSupplier = {
                                        id: 'eUsed',
                                        seriesType: seriesTypes.InstantAndCounter,
                                        valueDecimals: 0,
                                        analyseDataItem: function (dataItem) {
                                            const self = this;
                                            if (dataItem.v2 !== undefined) {
                                                self.seriesType = seriesTypes.P1;
                                            }
                                            if (self.valueDecimals !== 1 && dataItem.v % 1 !== 0) {
                                                self.valueDecimals = 1;
                                            }
                                        },
                                        dataItemIsValid: function (dataItem) {
                                            return dataItem.eu !== undefined;
                                        },
                                        valuesFromDataItem: [
                                            function (dataItem) {
                                                return dataItem.eu;
                                            }
                                        ],
                                        template: {
                                            name: self.device.SwitchTypeVal === deviceTypes.EnergyUsed ? $.t('Energy Usage') : $.t('Energy Generated'),
                                            tooltip: {
                                                valueSuffix: ' Wh',
                                                valueDecimals: this.valueDecimals
                                            },
                                            yAxis: 0
                                        }
                                    };
                                    if (seriesSupplier.seriesType === seriesTypes.InstantAndCounter) {
                                        seriesSupplier.template.type = 'column';
                                        seriesSupplier.template.pointRange = 3600 * 1000;
                                        seriesSupplier.template.zIndex = 5;
                                        seriesSupplier.template.animation = false;
                                        seriesSupplier.template.color = 'rgba(3,190,252,0.8)';
                                    }
                                    if (seriesSupplier.seriesType === seriesTypes.P1) {
                                        seriesSupplier.template.type = 'area';
                                        seriesSupplier.template.color = 'rgba(120,150,220,0.9)';
                                        seriesSupplier.template.fillOpacity = 0.2;
                                    }
                                    return seriesSupplier;
                                },
                                {
                                    id: 'eGen',
                                    valueDecimals: 0,
                                    analyseDataItem: function (dataItem) {
                                        const self = this;
                                        if (self.valueDecimals !== 1 && dataItem.eg % 1 !== 0) {
                                            self.valueDecimals = 1;
                                        }
                                    },
                                    dataItemIsValid: function (dataItem) {
                                        return dataItem.eg !== undefined;
                                    },
                                    valuesFromDataItem: [
                                        function (dataItem) {
                                            return dataItem.eg;
                                        }
                                    ],
                                    template : {
                                        type: 'area',
                                        name: $.t('Energy Returned'),
                                        tooltip: {
                                            valueSuffix: ' Wh',
                                            valueDecimals: this.valueDecimals
                                        },
                                        color: 'rgba(120,220,150,0.9)',
                                        fillOpacity: 0.2,
                                        yAxis: 0,
                                        //visible: false
                                    }
                                },
                                {
                                    id: 'usage1',
                                    seriesType: seriesTypes.Counter,
                                    valueDecimals: 0,
                                    analyseDataItem: function (dataItem) {
                                        const self = this;
                                        if (self.seriesType !== seriesTypes.P1 && dataItem.eu !== undefined) {
                                            self.seriesType = seriesTypes.InstantAndCounter;
                                        }
                                        if (dataItem.v2 !== undefined) {
                                            self.seriesType = seriesTypes.P1;
                                        }
                                        if (self.valueDecimals !== 1 && dataItem.v % 1 !== 0) {
                                            self.valueDecimals = 1;
                                        }
                                    },
                                    dataItemIsValid: function (dataItem) {
                                        return dataItem.v !== undefined;
                                    },
                                    valuesFromDataItem: [
                                        function (dataItem) {
                                            return dataItem.v;
                                        }
                                    ],
                                    template: function () {
                                        const selfSeriesSupplier = this;
                                        const deviceType = self.device.SwitchTypeVal;
                                        if (selfSeriesSupplier.seriesType === seriesTypes.Counter) {
                                            return {
                                                name: deviceType === deviceTypes.EnergyUsed ? $.t('Energy Usage') : $.t('Energy Generated'),
                                                color: 'rgba(3,190,252,0.8)',
                                                tooltip: {
                                                    valueSuffix: ' Wh',
                                                    valueDecimals: selfSeriesSupplier.valueDecimals
                                                },
                                                stack: 'susage',
                                                yAxis: 0
                                            };
                                        }
                                        if (selfSeriesSupplier.seriesType === seriesTypes.InstantAndCounter) {
                                            return {
                                                type: 'spline',
                                                name: deviceType === deviceTypes.EnergyUsed ? $.t('Power Usage') : $.t('Power Generated'),
                                                color: 'rgba(255,255,0,0.8)',
                                                tooltip: {
                                                    valueSuffix: ' Watt',
                                                    valueDecimals: selfSeriesSupplier.valueDecimals
                                                },
                                                stack: 'susage',
                                                zIndex: 10,
                                                yAxis: 1
                                            };
                                        }
                                        if (selfSeriesSupplier.seriesType === seriesTypes.P1) {
                                            return {
                                                name: $.t('Usage') + ' 1',
                                                color: 'rgba(60,130,252,0.8)',
                                                tooltip: {
                                                    valueSuffix: ' Watt',
                                                    valueDecimals: this.valueDecimals
                                                },
                                                stack: 'susage',
                                                yAxis: 1
                                            };
                                        }
                                        return {};
                                    }
                                }
                            ]
                        )
                    );
                    chart.createDataRequest = function () {
                        const request = RefreshingChart.prototype.createDataRequest(this);
                        request.method = 1;
                        return request;
                    }

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
                            {
                                isShortLogChart: false,
                                timestampFromDataItem: function (dataItem, yearOffset = 0) {
                                    return GetLocalDateFromString(dataItem.d, yearOffset);
                                }
                            },
                            [
                                {
                                    id: 'min',
                                    valueKeySuffix: '_min',
                                    template: {
                                        name: $.t('Minimum'),
                                        colorIndex: 3
                                    }
                                },
                                {
                                    id: 'max',
                                    valueKeySuffix: '_max',
                                    template: {
                                        name: $.t('Maximum'),
                                        colorIndex: 2
                                    }
                                },
                                {
                                    id: 'avg',
                                    valueKeySuffix: '_avg',
                                    template: {
                                        name: $.t('Average'),
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
        function chartParams(domoticzGlobals, ctrl, dataSupplierTemplate, seriesSuppliers) {
            const dataSupplier = dataSupplierTemplate;
            dataSupplier.yAxes = [
                {
                    title: {
                        text: $.t('Energy') + ' (Wh)'
                    }
                },
                {
                    title: {
                        text: $.t('Power') + ' (Watt)'
                    },
                    opposite: true
                }
            ];
            dataSupplier.seriesSuppliers = seriesSuppliers;
            return {
                highchartParams: {
                    alignTicks: false
                },
                synchronizeYaxes: true,
                range: ctrl.range,
                device: ctrl.device,
				sensorType: 'counter',
                chartName: ctrl.device.SwitchTypeVal === deviceTypes.EnergyGenerated ? $.t('Generated') : $.t('Usage'),
                autoRefreshIsEnabled: function() { return ctrl.logCtrl.autoRefresh; },
                dataSupplier: dataSupplier
            };
        }
    }
);
