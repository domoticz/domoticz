define(['app', 'lodash', 'RefreshingChart'],
    function (app, _, RefreshingChart) {

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
                                .concat(instantAndCounterShortSeriesSuppliers(self.device.SwitchTypeVal))
                                .concat(counterShortSeriesSuppliers(self.device.SwitchTypeVal))
                                .concat(p1ShortSeriesSuppliers(self.device.SwitchTypeVal))
                                .concat(powerReturnedShortSeriesSuppliers(self.device.SwitchTypeVal))
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
                                .concat(instantAndCounterLongSeriesSuppliers(self.device.SwitchTypeVal))
                                .concat(counterLongSeriesSuppliers(self.device.SwitchTypeVal))
                                .concat(p1LongSeriesSuppliers(self.device.SwitchTypeVal))
                                .concat(powerReturnedLongSeriesSuppliers(self.device.SwitchTypeVal))
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

        function counterEnergy(dataItemValueKey, seriesSupplier) {
            return completeSeriesSupplier(seriesSupplier, [dataItemValueKey], function (dataItemKeys) {
                return dataItemKeys.includes('v') && !dataItemKeys.includes('eu') && !dataItemKeys.includes('v2');
            });
        }

        function counterEnergyTotal(dataItemValueKeys, seriesSupplier) {
            return completeSeriesSupplier(seriesSupplier, dataItemValueKeys, function (dataItemKeys) {
                return dataItemKeys.includes('v');
            });
        }

        function counterShortSeriesSuppliers(deviceType) {
            return [
                counterEnergy('v', {
                    id: 'counterEnergyUsedOrGenerated',
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

        function counterLongSeriesSuppliers(deviceType) {
            return [
                counterEnergy('v', {
                    id: 'counterEnergyUsedOrGenerated',
                    valueDecimals: 3,
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
                {
                    id: 'counterEnergyUsedOrGeneratedTotal',
                    dataItemIsValid: function (dataItem) {
                        return dataItem.v !== undefined;
                    },
                    valuesFromDataItem: [
                        function (dataItem) {
                            return parseFloat(dataItem.v) + (dataItem.v2 !== undefined ? parseFloat(dataItem.v2) : 0.0);
                        }
                    ],
                    template: {
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
                },
                {
                    id: 'counterEnergyUsedOrGeneratedTotalTrendline',
                    dataItemIsValid: function (dataItem) {
                        return dataItem.v !== undefined;
                    },
                    valuesFromDataItem: [
                        function (dataItem) {
                            return parseFloat(dataItem.v) + (dataItem.v2 !== undefined ? parseFloat(dataItem.v2) : 0.0);
                        }
                    ],
                    aggregateDatapoints: function (datapoints) {
                        const trendline = CalculateTrendLine(datapoints);
                        datapoints.length = 0;
                        if (trendline !== undefined) {
                            datapoints.push([trendline.x0, trendline.y0]);
                            datapoints.push([trendline.x1, trendline.y1]);
                        }
                    },
                    template: {
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
                },
                {
                    id: 'counterEnergyUsedOrGeneratedPrevious',
                    dataItemIsValid: function (dataItem) {
                        return dataItem.v !== undefined;
                    },
                    valuesFromDataItem: [
                        function (dataItem) {
                            return parseFloat(dataItem.v) + (dataItem.v2 !== undefined ? parseFloat(dataItem.v2) : 0.0);
                        }
                    ],
                    useDataItemsFromPrevious: true,
                    template: {
                        name: $.t('Past') + ' ' + (deviceType === deviceTypes.EnergyUsed ? $.t('Usage') : deviceType === deviceTypes.EnergyGenerated ? $.t('Generated') : $.t('Return')),
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1000),
                            valueDecimals: 3
                        },
                        color: 'rgba(190,3,252,0.8)',
                        yAxis: 0,
                        visible: false
                    }
                }
            ];
        }

        function instantAndCounterPowerAndEnergy(dataItemValueKey, seriesSupplier) {
            return completeSeriesSupplier(seriesSupplier, [dataItemValueKey], function (dataItemKeys) {
                return dataItemKeys.includes('v') && dataItemKeys.includes('eu') && !dataItemKeys.includes('v2');
            });
        }

        function instantAndCounterShortSeriesSuppliers(deviceType) {
            return [
                instantAndCounterPowerAndEnergy('eu', {
                    id: 'instantAndCounterEnergyUsedOrGenerated',
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
                instantAndCounterPowerAndEnergy('v', {
                    id: 'instantAndCounterPowerUsedOrGenerated',
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

        function instantAndCounterLongSeriesSuppliers(deviceType) {
            return [
                instantAndCounterPowerAndEnergy('eu', {
                    id: 'instantAndCounterEnergyUsedOrGenerated',
                    valueDecimals: 3,
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
                instantAndCounterPowerAndEnergy('v', {
                    id: 'instantAndCounterPowerUsedOrGenerated',
                    valueDecimals: 3,
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

        function p1PowerAndEnergyUsed(dataItemValueKey, seriesSupplier) {
            return completeSeriesSupplier(seriesSupplier, [dataItemValueKey], function (dataItemKeys) {
                return dataItemKeys.includes('v') && dataItemKeys.includes('eu') && dataItemKeys.includes('v2');
            });
        }

        function p1PowerAndEnergyGenerated(dataItemValueKey, seriesSupplier) {
            return completeSeriesSupplier(seriesSupplier, [dataItemValueKey], function (dataItemKeys) {
                return dataItemKeys.includes(dataItemValueKey);
            });
        }

        function p1ShortSeriesSuppliers(deviceType) {
            return [
                p1PowerAndEnergyUsed('eu', {
                    id: 'p1EnergyUsedArea',
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
                p1PowerAndEnergyGenerated('eg', {
                    id: 'p1EnergyGeneratedArea',
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
                p1PowerAndEnergyUsed('v', {
                    id: 'p1PowerUsed',
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
                p1PowerAndEnergyGenerated('v2', {
                    id: 'p1PowerGenerated',
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

        function p1LongSeriesSuppliers(deviceType) {
            return [
                p1PowerAndEnergyUsed('eu', {
                    id: 'p1EnergyUsedArea',
                    valueDecimals: 3,
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
                p1PowerAndEnergyGenerated('eg', {
                    id: 'p1EnergyGeneratedArea',
                    valueDecimals: 3,
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
                p1PowerAndEnergyUsed('v', {
                    id: 'p1EnergyUsed',
                    valueDecimals: 3,
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
                p1PowerAndEnergyGenerated('v2', {
                    id: 'p1EnergyGenerated',
                    valueDecimals: 3,
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

        function powerReturned(dataItemValueKey, seriesSupplier) {
            return completeSeriesSupplier(seriesSupplier, [dataItemValueKey], function (dataItemKeys) {
                return dataItemKeys.includes(dataItemValueKey);
            });
        }

        function powerReturnedShortSeriesSuppliers(deviceType) {
            return [
                powerReturned('r1', {
                    id: 'powerReturned1',
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
                powerReturned('r2', {
                    id: 'powerReturned2',
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

        function powerReturnedLongSeriesSuppliers(deviceType) {
            return [
                powerReturned('r1', {
                    id: 'powerReturned1',
                    valueDecimals: 3,
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
                powerReturned('r2', {
                    id: 'powerReturned2',
                    valueDecimals: 3,
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
                {
                    id: 'powerReturnedTotal',
                    dataItemIsValid: function (dataItem) {
                        return dataItem.r1 !== undefined;
                    },
                    valuesFromDataItem: [
                        function (dataItem) {
                            return parseFloat(dataItem.r1) + (dataItem.r2 !== undefined ? parseFloat(dataItem.r2) : 0.0);
                        }
                    ],
                    template: {
                        name: $.t('Total Return'),
                        zIndex: 1,
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1000),
                            valueDecimals: 3
                        },
                        color: 'rgba(3,252,190,0.8)',
                        yAxis: 0
                    }
                },
                {
                    id: 'powerReturnedTotalTrendline',
                    dataItemIsValid: function (dataItem) {
                        return dataItem.r1 !== undefined;
                    },
                    valuesFromDataItem: [
                        function (dataItem) {
                            return parseFloat(dataItem.r1) + (dataItem.r2 !== undefined ? parseFloat(dataItem.r2) : 0.0);
                        }
                    ],
                    aggregateDatapoints: function (datapoints) {
                        const trendline = CalculateTrendLine(datapoints);
                        datapoints.length = 0;
                        if (trendline !== undefined) {
                            datapoints.push([trendline.x0, trendline.y0]);
                            datapoints.push([trendline.x1, trendline.y1]);
                        }
                    },
                    template: {
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
                },
                {
                    id: 'powerReturnedTotalPrevious',
                    dataItemIsValid: function (dataItem) {
                        return dataItem.r1 !== undefined;
                    },
                    valuesFromDataItem: [
                        function (dataItem) {
                            return parseFloat(dataItem.r1) + (dataItem.r2 !== undefined ? parseFloat(dataItem.r2) : 0.0);
                        }
                    ],
                    useDataItemsFromPrevious: true,
                    template: {
                        name: $.t('Past') + ' ' + $.t('Return'),
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1000),
                            valueDecimals: 3
                        },
                        color: 'rgba(252,190,3,0.8)',
                        yAxis: 0,
                        visible: false
                    }
                }
            ];
        }

        function completeSeriesSupplier(seriesSupplier, dataItemValueKeys, dataItemKeysAreSupported) {
            return _.merge(
                {
                    valueDecimals: 0,
                    dataItemKeys: [],
                    analyseDataItem: function (dataItem) {
                        const self = this;
                        dataItemKeysCollect(dataItem);
                        valueDecimalsCalculate(dataItem);

                        function dataItemKeysCollect(dataItem) {
                            Object.keys(dataItem)
                                .filter(function (key) {
                                    return !self.dataItemKeys.includes(key);
                                })
                                .forEach(function (key) {
                                    self.dataItemKeys.push(key);
                                });
                        }

                        function valueDecimalsCalculate(dataItem) {
                            if (self.valueDecimals === 0 && dataItemValueKeys.reduce(function (valueDecimals, dataItemValueKey) { return valueDecimals + dataItem[dataItemValueKey] % 1; }, 0) !== 0) {
                                self.valueDecimals = 1;
                            }
                        }
                    },
                    dataItemIsValid: function (dataItem) {
                        return dataItemKeysAreSupported(this.dataItemKeys) && dataItemValueKeys.some(function (dataItemValueKey) { return dataItem[dataItemValueKey] !== undefined; });
                    },
                    valuesFromDataItem: [
                        function (dataItem) {
                            return dataItemValueKeys.reduce(function (totalValue, dataItemValueKey) {
                                const dataItemElement = dataItem[dataItemValueKey];
                                if (dataItemElement === undefined) {
                                    return totalValue;
                                }
                                return totalValue + parseFloat(dataItemElement);
                            }, 0.0);
                        }
                    ],
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
            return {
                highchartTemplate: {
                    chart: {
                        alignTicks: false
                    },
                    tooltip: {
                        shared: false
                    },
                    plotOptions: {
                        series: {
                            matchExtremes: true
                        }
                    }
                },
                synchronizeYaxes: true,
                range: ctrl.range,
                device: ctrl.device,
				sensorType: 'counter',
                chartName: ctrl.device.SwitchTypeVal === deviceTypes.EnergyGenerated ? $.t('Generated') : $.t('Usage'),
                autoRefreshIsEnabled: function() {
                    return ctrl.logCtrl.autoRefresh;
                },
                dataSupplier:
                    _.merge(
                        {
                            yAxes:
                                [
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
                                ],
                            seriesSuppliers: seriesSuppliers
                        },
                        dataSupplierTemplate
                    )
            };
        }

        function chartParamsWeek(domoticzGlobals, ctrl, dataSupplierTemplate, seriesSuppliers) {
            return {
                highchartTemplate: {
                    chart: {
                        type: 'column',
                        marginRight:10
                    },
                    xAxis: {
                        dateTimeLabelFormats: {
                            day: '%a'
                        },
                        tickInterval: 24 * 3600 * 1000
                    },
                    plotOptions: {
                        column: {
                            dataLabels: {
                                enabled: true
                            }
                        }
                    }
                },
                range: ctrl.range,
                device: ctrl.device,
				sensorType: 'counter',
                chartName: ctrl.device.SwitchTypeVal === deviceTypes.EnergyGenerated ? $.t('Generated') : $.t('Usage'),
                autoRefreshIsEnabled: function() { return ctrl.logCtrl.autoRefresh; },
                dataSupplier:
                    _.merge(
                        {
                            yAxes:
                                [
                                    {
                                        maxPadding: 0.2,
                                        title: {
                                            text: $.t('Energy') + ' (kWh)'
                                        }
                                    }
                                ],
                            seriesSuppliers: seriesSuppliers
                        },
                        dataSupplierTemplate
                    )
            };
        }

        function chartParamsMonthYear(domoticzGlobals, ctrl, dataSupplierTemplate, seriesSuppliers) {
            return {
                highchartTemplate: {
                    chart: {
                        marginRight:10
                    }
                },
                range: ctrl.range,
                device: ctrl.device,
				sensorType: 'counter',
                chartName: ctrl.device.SwitchTypeVal === deviceTypes.EnergyGenerated ? $.t('Generated') : $.t('Usage'),
                autoRefreshIsEnabled: function() { return ctrl.logCtrl.autoRefresh; },
                dataSupplier:
                    _.merge(
                        {
                            yAxes:
                                [
                                    {
                                        title: {
                                            text: $.t('Energy') + ' (kWh)'
                                        }
                                    }
                                ],
                            seriesSuppliers: seriesSuppliers
                        },
                        dataSupplierTemplate
                    )
            };
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
