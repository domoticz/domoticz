define(['app', 'lodash', 'RefreshingChart'],
    function (app, _, RefreshingChart) {

        app.component('deviceCounterLog', {
            bindings: {
                device: '<',
                subtype: '<'
            },
            templateUrl: 'app/log/CounterLog.html',
            controllerAs: '$ctrl',
            controller: function () {
                const $ctrl = this;
                $ctrl.autoRefresh = true;
            }
        });

        app.component('counterDayChart', {
            require: {
                logCtrl: '^deviceCounterLog'
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
                    self.chart = new RefreshingChart(
                        baseParams($),
                        angularParams($location, $route, $scope, $timeout, $element),
                        domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        chartParamsDay(domoticzGlobals, self,
                            {
                                isShortLogChart: true,
                                timestampFromDataItem: function (dataItem, yearOffset = 0) {
                                    return GetLocalDateTimeFromString(dataItem.d, yearOffset);
                                },
                                extendDataRequest: function (dataRequest) {
                                    dataRequest.method = 1;
                                    return dataRequest;
                                }
                            },
                            []
                                .concat(instantAndCounterDaySeriesSuppliers(self.device.SwitchTypeVal))
                                .concat(counterDaySeriesSuppliers(self.device.SwitchTypeVal))
                                .concat(p1DaySeriesSuppliers(self.device.SwitchTypeVal))
                                .concat(powerReturnedDaySeriesSuppliers(self.device.SwitchTypeVal))
                        )
                    );
                }
            }
        });

        app.component('counterWeekChart', {
            require: {
                logCtrl: '^deviceCounterLog'
            },
            bindings: {
                device: '<'
            },
            templateUrl: 'app/log/chart-week.html',
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi) {
                const self = this;
                self.range = 'week';

                self.$onInit = function () {
                    self.chart = new RefreshingChart(
                        baseParams($),
                        angularParams($location, $route, $scope, $timeout, $element),
                        domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        chartParamsWeek(domoticzGlobals, self,
                            {
                                isShortLogChart: false,
                                timestampFromDataItem: function (dataItem, yearOffset = 0) {
                                    return GetLocalDateFromString(dataItem.d, yearOffset);
                                }
                            },
                            []
                                .concat(instantAndCounterWeekSeriesSuppliers(self.device.SwitchTypeVal))
                                .concat(counterWeekSeriesSuppliers(self.device.SwitchTypeVal))
                                .concat(p1WeekSeriesSuppliers(self.device.SwitchTypeVal))
                                .concat(powerReturnedWeekSeriesSuppliers(self.device.SwitchTypeVal))
                        )
                    );
                }
            }
        });

        app.component('counterMonthChart', {
            require: {
                logCtrl: '^deviceCounterLog'
            },
            bindings: {
                device: '<'
            },
            templateUrl: 'app/log/chart-month.html',
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi) {
                const self = this;
                self.range = 'month';

                self.$onInit = function () {
                    self.chart = new RefreshingChart(
                        baseParams($),
                        angularParams($location, $route, $scope, $timeout, $element),
                        domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        chartParamsMonthYear(domoticzGlobals, self,
                            {
                                isShortLogChart: false,
                                timestampFromDataItem: function (dataItem, yearOffset = 0) {
                                    return GetLocalDateFromString(dataItem.d, yearOffset);
                                }
                            },
                            []
                                .concat(counterMonthYearSeriesSuppliers(self.device.SwitchTypeVal))
                                .concat(powerReturnedMonthYearSeriesSuppliers(self.device.SwitchTypeVal))
                        )
                    );
                }
            }
        });

        app.component('counterYearChart', {
            require: {
                logCtrl: '^deviceCounterLog'
            },
            bindings: {
                device: '<'
            },
            templateUrl: 'app/log/chart-year.html',
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi) {
                const self = this;
                self.range = 'year';

                self.$onInit = function () {
                    self.chart = new RefreshingChart(
                        baseParams($),
                        angularParams($location, $route, $scope, $timeout, $element),
                        domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        chartParamsMonthYear(domoticzGlobals, self,
                            {
                                isShortLogChart: false,
                                timestampFromDataItem: function (dataItem, yearOffset = 0) {
                                    return GetLocalDateFromString(dataItem.d, yearOffset);
                                }
                            },
                            []
                                .concat(counterMonthYearSeriesSuppliers(self.device.SwitchTypeVal))
                                .concat(powerReturnedMonthYearSeriesSuppliers(self.device.SwitchTypeVal))
                        )
                    );
                }
            }
        });

        function ContainsEnergyAndPowerNormalAndLow() {

        }

        ContainsEnergyAndPowerNormalAndLow.prototype.test = function (dataItemsKeys) {
            return dataItemsKeys.includes('v') && dataItemsKeys.includes('eu') && dataItemsKeys.includes('v2');
        }

        function ContainsPowerNormalOnly() {

        }

        ContainsPowerNormalOnly.prototype.test = function (dataItemsKeys) {
            return dataItemsKeys.includes('v') && !dataItemsKeys.includes('eu') && !dataItemsKeys.includes('v2');
        }

        function ContainsEnergyAndPowerNormalOnly() {

        }

        ContainsEnergyAndPowerNormalOnly.prototype.test = function (dataItemsKeys) {
            return dataItemsKeys.includes('v') && dataItemsKeys.includes('eu') && !dataItemsKeys.includes('v2');
        }

        function ContainsPowerNormalAndReturnNormal() {

        }

        ContainsPowerNormalAndReturnNormal.prototype.test = function (dataItemsKeys) {
            return dataItemsKeys.includes('v') && dataItemsKeys.includes('r1');
        }

        function ContainsDataItemValueKey(dataItemValueKey) {
            this.dataItemValueKey = dataItemValueKey;
        }

        ContainsDataItemValueKey.prototype.test = function (dataItemsKeys) {
            return dataItemsKeys.includes(this.dataItemValueKey);
        }

        function counterDaySeriesSuppliers(deviceType) {
            return [
                completeShortOrLongSeriesSupplier('v', new ContainsPowerNormalOnly(), {
                    id: 'counterEnergyUsedOrGenerated',
                    label: 'A',
                    series: {
                        type: 'spline',
                        name: deviceType === deviceTypes.EnergyUsed ? $.t('Energy Usage') : $.t('Energy Generated'),
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1)
                        },
                        color: 'rgba(3,190,252,0.8)',
                        stack: 'susage',
                        yAxis: 0
                    }
                })
            ];
        }

        function counterWeekSeriesSuppliers(deviceType) {
            return [
                completeShortOrLongSeriesSupplier('v', new ContainsPowerNormalOnly(), {
                    id: 'counterEnergyUsedOrGenerated',
                    valueDecimals: 3,
                    label: 'B',
                    series: {
                        type: 'column',
                        name: deviceType === deviceTypes.EnergyUsed ? $.t('Energy Usage') : $.t('Energy Generated'),
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1000)
                        },
                        color: 'rgba(3,190,252,0.8)',
                        stack: 'susage',
                        yAxis: 0
                    }
                })
            ];
        }

        function counterMonthYearSeriesSuppliers(deviceType) {
            return [
                completeMonthOrYearSeriesSupplier({
                    id: 'counterEnergyUsedOrGeneratedTotal',
                    dataItemKeys: ['v', 'v2'],
                    convertZeroToNull: true,
                    label: 'C',
                    series: {
                        type: 'column',
                        name: deviceType === deviceTypes.EnergyUsed ? $.t('Total Usage') : deviceType === deviceTypes.EnergyGenerated ? $.t('Total Generated') : $.t('Total Return'),
                        zIndex: 2,
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1000),
                            valueDecimals: 3
                        },
                        color: 'rgba(3,190,252,0.8)',
                        yAxis: 0
                    }
                }),
                completeMonthOrYearSeriesSupplier({
                    id: 'counterEnergyUsedOrGeneratedTotalTrendline',
                    dataItemKeys: ['v', 'v2'],
                    aggregateDatapoints: function (datapoints) {
                        const trendline = CalculateTrendLine(datapoints);
                        datapoints.length = 0;
                        if (trendline !== undefined) {
                            datapoints.push([trendline.x0, trendline.y0]);
                            datapoints.push([trendline.x1, trendline.y1]);
                        }
                    },
                    label: 'D',
                    series: {
                        name: $.t('Trendline') + ' ' + (deviceType === deviceTypes.EnergyUsed ? $.t('Usage') : deviceType === deviceTypes.EnergyGenerated ? $.t('Generated') : $.t('Return')),
                        zIndex: 1,
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1000),
                            valueDecimals: 3
                        },
                        color: 'rgba(255,3,3,0.8)',
                        dashStyle: 'LongDash',
                        yAxis: 0,
                        visible: false
                    }
                }),
                completeMonthOrYearSeriesSupplier({
                    id: 'counterEnergyUsedOrGeneratedPrevious',
                    dataItemKeys: ['v', 'v2'],
                    useDataItemsFromPrevious: true,
                    label: 'E',
                    series: {
                        name: $.t('Past') + ' ' + (deviceType === deviceTypes.EnergyUsed ? $.t('Usage') : deviceType === deviceTypes.EnergyGenerated ? $.t('Generated') : $.t('Return')),
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1000),
                            valueDecimals: 3
                        },
                        color: 'rgba(190,3,252,0.8)',
                        yAxis: 0,
                        visible: false
                    }
                })
            ];
        }

        function instantAndCounterDaySeriesSuppliers(deviceType) {
            return [
                completeShortOrLongSeriesSupplier('eu', new ContainsEnergyAndPowerNormalOnly(), {
                    id: 'instantAndCounterEnergyUsedOrGenerated',
                    label: 'F',
                    series: {
                        type: 'column',
                        pointRange: 3600 * 1000,
                        zIndex: 5,
                        animation: false,
                        name: deviceType === deviceTypes.EnergyUsed ? $.t('Energy Usage') : $.t('Energy Generated'),
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1)
                        },
                        color: 'rgba(3,190,252,0.8)',
                        yAxis: 0
                    }
                }),
                completeShortOrLongSeriesSupplier('v', new ContainsEnergyAndPowerNormalOnly(), {
                    id: 'instantAndCounterPowerUsedOrGenerated',
                    label: 'G',
                    series: {
                        name: deviceType === deviceTypes.EnergyUsed ? $.t('Power Usage') : $.t('Power Generated'),
                        zIndex: 10,
                        type: 'spline',
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.power(valueMultipliers.m1)
                        },
                        color: 'rgba(255,255,0,0.8)',
                        stack: 'susage',
                        yAxis: 1
                    }
                })
            ];
        }

        function instantAndCounterWeekSeriesSuppliers(deviceType) {
            return [
                completeShortOrLongSeriesSupplier('eu', new ContainsEnergyAndPowerNormalOnly(), {
                    id: 'instantAndCounterEnergyUsedOrGenerated',
                    valueDecimals: 3,
                    label: 'H',
                    series: {
                        type: 'column',
                        pointRange: 3600 * 1000,
                        zIndex: 5,
                        animation: false,
                        name: deviceType === deviceTypes.EnergyUsed ? $.t('Energy Usage') : $.t('Energy Generated'),
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1000)
                        },
                        color: 'rgba(3,190,252,0.8)',
                        yAxis: 0
                    }
                }),
                completeShortOrLongSeriesSupplier('v', new ContainsEnergyAndPowerNormalOnly(), {
                    id: 'instantAndCounterPowerUsedOrGenerated',
                    valueDecimals: 3,
                    label: 'I',
                    series: {
                        name: deviceType === deviceTypes.EnergyUsed ? $.t('Power Usage') : $.t('Power Generated'),
                        zIndex: 10,
                        type: 'column',
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1000)
                        },
                        color: 'rgba(3,190,252,0.8)',
                        stack: 'susage',
                        yAxis: 0
                    }
                })
            ];
        }

        function p1DaySeriesSuppliers(deviceType) {
            return [
                completeShortOrLongSeriesSupplier('eu', new ContainsEnergyAndPowerNormalAndLow(), {
                    id: 'p1EnergyUsedArea',
                    label: 'J',
                    series: {
                        type: 'area',
                        name: $.t('Energy Usage'),
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1)
                        },
                        color: 'rgba(120,150,220,0.9)',
                        fillOpacity: 0.2,
                        yAxis: 0,
                        visible: false
                    }
                }),
                completeShortOrLongSeriesSupplier('eg', new ContainsEnergyAndPowerNormalAndLow(), {
                    id: 'p1EnergyGeneratedArea',
                    label: 'K',
                    series: {
                        type: 'area',
                        name: $.t('Energy Returned'),
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1)
                        },
                        color: 'rgba(120,220,150,0.9)',
                        fillOpacity: 0.2,
                        yAxis: 0,
                        visible: false
                    }
                }),
                completeShortOrLongSeriesSupplier('v', new ContainsEnergyAndPowerNormalAndLow(), {
                    id: 'p1PowerUsed',
                    label: 'L',
                    series: {
                        name: $.t('Usage') + ' 1',
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.power(valueMultipliers.m1)
                        },
                        color: 'rgba(60,130,252,0.8)',
                        stack: 'susage',
                        yAxis: 1
                    }
                }),
                completeShortOrLongSeriesSupplier('v2', new ContainsEnergyAndPowerNormalAndLow(), {
                    id: 'p1PowerGenerated',
                    label: 'M',
                    series: {
                        name: $.t('Usage') + ' 2',
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.power(valueMultipliers.m1)
                        },
                        color: 'rgba(3,190,252,0.8)',
                        stack: 'susage',
                        yAxis: 1
                    }
                })
            ];
        }

        function p1WeekSeriesSuppliers(deviceType) {
            return [
                completeShortOrLongSeriesSupplier('eu', new ContainsDataItemValueKey('eu'), {
                    id: 'p1EnergyUsedArea',
                    valueDecimals: 3,
                    label: 'N',
                    series: {
                        type: 'area',
                        name: $.t('Energy Usage'),
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1000)
                        },
                        color: 'rgba(120,150,220,0.9)',
                        fillOpacity: 0.2,
                        yAxis: 0,
                        visible: false
                    }
                }),
                completeShortOrLongSeriesSupplier('eg', new ContainsDataItemValueKey('eg'), {
                    id: 'p1EnergyGeneratedArea',
                    valueDecimals: 3,
                    label: 'O',
                    series: {
                        type: 'area',
                        name: $.t('Energy Returned'),
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1000)
                        },
                        color: 'rgba(120,220,150,0.9)',
                        fillOpacity: 0.2,
                        yAxis: 0,
                        visible: false
                    }
                }),
                completeShortOrLongSeriesSupplier('v', new ContainsPowerNormalAndReturnNormal(), {
                    id: 'p1EnergyUsed',
                    valueDecimals: 3,
                    convertZeroToNull: true,
                    label: 'P',
                    series: {
                        name: $.t('Usage') + ' 1',
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1000)
                        },
                        color: 'rgba(60,130,252,0.8)',
                        stack: 'susage',
                        yAxis: 0
                    }
                }),
                completeShortOrLongSeriesSupplier('v2', new ContainsDataItemValueKey('v2'), {
                    id: 'p1EnergyGenerated',
                    valueDecimals: 3,
                    convertZeroToNull: true,
                    label: 'Q',
                    series: {
                        name: $.t('Usage') + ' 2',
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1000)
                        },
                        color: 'rgba(3,190,252,0.8)',
                        stack: 'susage',
                        yAxis: 0
                    }
                })
            ];
        }

        function powerReturnedDaySeriesSuppliers(deviceType) {
            return [
                completeShortOrLongSeriesSupplier('r1', new ContainsDataItemValueKey('r1'), {
                    id: 'powerReturned1',
                    label: 'R',
                    series: {
                        name: $.t('Return') + ' 1',
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.power(valueMultipliers.m1)
                        },
                        color: 'rgba(30,242,110,0.8)',
                        stack: 'sreturn',
                        yAxis: 1
                    }
                }),
                completeShortOrLongSeriesSupplier('r2', new ContainsDataItemValueKey('r2'), {
                    id: 'powerReturned2',
                    label: 'S',
                    series: {
                        name: $.t('Return') + ' 2',
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.power(valueMultipliers.m1)
                        },
                        color: 'rgba(3,252,190,0.8)',
                        stack: 'sreturn',
                        yAxis: 1
                    }
                })
            ];
        }

        function powerReturnedWeekSeriesSuppliers(deviceType) {
            return [
                completeShortOrLongSeriesSupplier('r1', new ContainsDataItemValueKey('r1'), {
                    id: 'powerReturned1',
                    valueDecimals: 3,
                    convertZeroToNull: true,
                    label: 'T',
                    series: {
                        name: $.t('Return') + ' 1',
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1000)
                        },
                        color: 'rgba(30,242,110,0.8)',
                        stack: 'sreturn',
                        yAxis: 0
                    }
                }),
                completeShortOrLongSeriesSupplier('r2', new ContainsDataItemValueKey('r2'), {
                    id: 'powerReturned2',
                    valueDecimals: 3,
                    convertZeroToNull: true,
                    label: 'U',
                    series: {
                        name: $.t('Return') + ' 2',
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1000)
                        },
                        color: 'rgba(3,252,190,0.8)',
                        stack: 'sreturn',
                        yAxis: 0
                    }
                })
            ];
        }

        function powerReturnedMonthYearSeriesSuppliers(deviceType) {
            return [
                completeMonthOrYearSeriesSupplier({
                    id: 'powerReturnedTotal',
                    dataItemKeys: ['r1', 'r2'],
                    convertZeroToNull: true,
                    label: 'V',
                    series: {
                        type: 'column',
                        name: $.t('Total Return'),
                        zIndex: 1,
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1000),
                            valueDecimals: 3
                        },
                        color: 'rgba(3,252,190,0.8)',
                        yAxis: 0
                    }
                }),
                completeMonthOrYearSeriesSupplier({
                    id: 'powerReturnedTotalTrendline',
                    dataItemKeys: ['r1', 'r2'],
                    aggregateDatapoints: function (datapoints) {
                        const trendline = CalculateTrendLine(datapoints);
                        datapoints.length = 0;
                        if (trendline !== undefined) {
                            datapoints.push([trendline.x0, trendline.y0]);
                            datapoints.push([trendline.x1, trendline.y1]);
                        }
                    },
                    label: 'W',
                    series: {
                        name: $.t('Trendline') + ' ' + $.t('Return'),
                        zIndex: 1,
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1000),
                            valueDecimals: 3
                        },
                        color: 'rgba(255,127,39,0.8)',
                        dashStyle: 'LongDash',
                        yAxis: 0,
                        visible: false
                    }
                }),
                completeMonthOrYearSeriesSupplier({
                    id: 'powerReturnedTotalPrevious',
                    dataItemKeys: ['r1', 'r2'],
                    useDataItemsFromPrevious: true,
                    label: 'X',
                    series: {
                        name: $.t('Past') + ' ' + $.t('Return'),
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1000),
                            valueDecimals: 3
                        },
                        color: 'rgba(252,190,3,0.8)',
                        yAxis: 0,
                        visible: false
                    }
                })
            ];
        }

        function completeShortOrLongSeriesSupplier(dataItemValueKey, dataSeriesItemsKeysPredicate, seriesSupplier) {
            return _.merge(
                {
                    valueDecimals: 0,
                    dataSeriesItemsKeys: [],
                    analyseDataItem: function (dataItem) {
                        const self = this;
                        dataItemKeysCollect(dataItem);
                        valueDecimalsCalculate(dataItem);

                        function dataItemKeysCollect(dataItem) {
                            Object.keys(dataItem)
                                .filter(function (key) {
                                    return !self.dataSeriesItemsKeys.includes(key);
                                })
                                .forEach(function (key) {
                                    self.dataSeriesItemsKeys.push(key);
                                });
                        }

                        function valueDecimalsCalculate(dataItem) {
                            if (self.valueDecimals === 0 && dataItem[dataItemValueKey] % 1 !== 0) {
                                self.valueDecimals = 1;
                            }
                        }
                    },
                    dataItemIsValid: function (dataItem) {
                        return dataSeriesItemsKeysPredicate.test(this.dataSeriesItemsKeys) && dataItem[dataItemValueKey] !== undefined;
                    },
                    valuesFromDataItem: function (dataItem) {
                        return [parseFloat(dataItem[dataItemValueKey])];
                    },
                    template: function () {
                        return _.merge(
                            {
                                tooltip: {
                                    valueDecimals: seriesSupplier.valueDecimals
                                }
                            },
                            seriesSupplier.series
                        );
                    }
                },
                seriesSupplier
            );
        }

        function completeMonthOrYearSeriesSupplier(seriesSupplier) {
            return _.merge(
                {
                    dataItemKeys: [],
                    dataItemIsValid:
                        function (dataItem) {
                            return this.dataItemKeys.length !== 0 && dataItem[this.dataItemKeys[0]] !== undefined;
                        },
                    valuesFromDataItem: function (dataItem) {
                        return [this.dataItemKeys.reduce(sumDataItemValue, 0.0)];

                        function sumDataItemValue(totalValue, key) {
                            const value = dataItem[key];
                            if (value === undefined) {
                                return totalValue;
                            }
                            return totalValue + parseFloat(value);
                        }
                    },
                    template: function () {
                        return _.merge(
                            {
                                tooltip: {
                                    valueDecimals: seriesSupplier.valueDecimals
                                }
                            },
                            seriesSupplier.series
                        );
                    }
                },
                seriesSupplier
            );
        }

        function chartParamsDay(domoticzGlobals, ctrl, dataSupplierTemplate, seriesSuppliers) {
            return _.merge(
                {
                    highchartTemplate: {
                        plotOptions: {
                            series: {
                                matchExtremes: true
                            }
                        }
                    },
                    range: ctrl.range,
                    device: ctrl.device,
                    sensorType: 'counter',
                    chartName: ctrl.device.SwitchTypeVal === deviceTypes.EnergyGenerated ? $.t('Generated') : $.t('Usage'),
                    autoRefreshIsEnabled: function () {
                        return ctrl.logCtrl.autoRefresh;
                    },
                    dataSupplier:
                        _.merge(
                            {
                                yAxes:
                                    [
                                        _.merge(
                                            {
                                                title: {
                                                    text: $.t('Energy') + ' (Wh)'
                                                }
                                            },
                                            seriesYaxisSubtype()
                                        ),
                                        _.merge(
                                            {
                                                title: {
                                                    text: $.t('Power') + ' (Watt)'
                                                },
                                                opposite: true
                                            },
                                            seriesYaxisSubtype()
                                        )
                                    ],
                                seriesSuppliers: seriesSuppliers
                            },
                            dataSupplierTemplate
                        )
                },
                chartParamsDaySubtype(ctrl.logCtrl.subtype)
            );

            function chartParamsDaySubtype(subtype) {
                if (subtype === 'p1') {
                    return {
                        highchartTemplate: {
                            chart: {
                                alignTicks: true
                            }
                        }
                    };
                } else {
                    return {
                        highchartTemplate: {
                            chart: {
                                alignTicks: false
                            },
                        },
                        synchronizeYaxes: true
                    };
                }
            }
        }

        function chartParamsWeek(domoticzGlobals, ctrl, dataSupplierTemplate, seriesSuppliers) {
            return _.merge(
                {
                    highchartTemplate: {
                        chart: {
                            type: 'column',
                            marginRight: 10
                        },
                        xAxis: {
                            dateTimeLabelFormats: {
                                day: '%a'
                            },
                            tickInterval: 24 * 3600 * 1000
                        },
                        tooltip: {
                            shared: false,
                            crosshairs: false
                        }
                    },
                    range: ctrl.range,
                    device: ctrl.device,
                    sensorType: 'counter',
                    chartName: ctrl.device.SwitchTypeVal === deviceTypes.EnergyGenerated ? $.t('Generated') : $.t('Usage'),
                    autoRefreshIsEnabled: function () {
                        return ctrl.logCtrl.autoRefresh;
                    },
                    dataSupplier:
                        _.merge(
                            {
                                yAxes:
                                    [
                                        _.merge(
                                            {
                                                maxPadding: 0.2,
                                                title: {
                                                    text: $.t('Energy') + ' (kWh)'
                                                }
                                            },
                                            seriesYaxisSubtype()
                                        )
                                    ],
                                seriesSuppliers: seriesSuppliers
                            },
                            dataSupplierTemplate
                        )
                },
                chartParamsWeekSubtype(ctrl.logCtrl.subtype)
            );

            function chartParamsWeekSubtype(subtype) {
                if (subtype === 'p1') {
                    return {
                        highchartTemplate: {
                            plotOptions: {
                                column: {
                                    stacking: 'normal',
                                    dataLabels: {
                                        enabled: false
                                    }
                                }
                            },
                            tooltip: {
                                pointFormat: '{series.name}: <b>{point.y}</b> ( {point.percentage:.0f}% )<br>',
                                footerFormat: 'Total: {point.total} {series.tooltipOptions.valueSuffix}'
                            }
                        }
                    };
                } else {
                    return {
                        highchartTemplate: {
                            plotOptions: {
                                column: {
                                    dataLabels: {
                                        enabled: true
                                    }
                                }
                            }
                        }
                    };
                }
            }
        }

        function chartParamsMonthYear(domoticzGlobals, ctrl, dataSupplierTemplate, seriesSuppliers) {
            return _.merge(
                {
                    highchartTemplate: {
                        chart: {
                            marginRight: 10
                        },
                        tooltip: {
                            crosshairs: false
                        }
                    },
                    range: ctrl.range,
                    device: ctrl.device,
                    sensorType: 'counter',
                    chartName: ctrl.device.SwitchTypeVal === deviceTypes.EnergyGenerated ? $.t('Generated') : $.t('Usage'),
                    autoRefreshIsEnabled: function () {
                        return ctrl.logCtrl.autoRefresh;
                    },
                    dataSupplier:
                        _.merge(
                            {
                                yAxes:
                                    [
                                        _.merge(
                                            {
                                                title: {
                                                    text: $.t('Energy') + ' (kWh)'
                                                }
                                            },
                                            seriesYaxisSubtype()
                                        )
                                    ],
                                seriesSuppliers: seriesSuppliers
                            },
                            dataSupplierTemplate
                        )
                },
                chartParamsMonthYearSubtype(ctrl.logCtrl.subtype)
            );

            function chartParamsMonthYearSubtype(subtype) {
                if (subtype === 'p1') {
                    return {
                        highchartTemplate: {
                            tooltip: {
                                shared: false
                            }
                        }
                    };
                } else {
                    return {

                    };
                }
            }
        }

        function seriesYaxisSubtype(subtype) {
            if (subtype === 'p1') {
                return {
                    min: 0
                };
            } else {
                return {

                };
            }
        }

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
            }
        }
    }
);
