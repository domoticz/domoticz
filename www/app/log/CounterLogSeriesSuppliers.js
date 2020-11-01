define(['app', 'log/Chart', 'log/CounterLogSeriesSupplier'], function (app) {

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

    function ContainsEnergyUsedAndPowerNormalAndLow() {

    }

    ContainsEnergyUsedAndPowerNormalAndLow.prototype.test = function (dataItemsKeys) {
        return dataItemsKeys.includes('v') && dataItemsKeys.includes('eu') && dataItemsKeys.includes('v2');
    }

    function ContainsPowerNormalOnly() {

    }

    ContainsPowerNormalOnly.prototype.test = function (dataItemsKeys) {
        return dataItemsKeys.includes('v') && !dataItemsKeys.includes('eu') && !dataItemsKeys.includes('v2');
    }

    function ContainsEnergyUsedAndPowerNormalOnly() {

    }

    ContainsEnergyUsedAndPowerNormalOnly.prototype.test = function (dataItemsKeys) {
        return dataItemsKeys.includes('v') && dataItemsKeys.includes('eu') && !dataItemsKeys.includes('v2');
    }

    function ContainsEnergyUsed() {

    }

    ContainsEnergyUsed.prototype.test = function (dataItemsKeys) {
        return dataItemsKeys.includes('eu');
    }

    function ContainsEnergyGenerated() {

    }

    ContainsEnergyGenerated.prototype.test = function (dataItemsKeys) {
        return dataItemsKeys.includes('eg');
    }

    function ContainsPowerLow() {

    }

    ContainsPowerLow.prototype.test = function (dataItemsKeys) {
        return dataItemsKeys.includes('v2');
    }

    function ContainsPowerNormalAndReturnNormal() {

    }

    ContainsPowerNormalAndReturnNormal.prototype.test = function (dataItemsKeys) {
        return dataItemsKeys.includes('v') && dataItemsKeys.includes('r1');
    }

    function ContainsReturnNormal() {

    }

    ContainsReturnNormal.prototype.test = function (dataItemsKeys) {
        return dataItemsKeys.includes('r1');
    }

    function ContainsReturnLow() {

    }

    ContainsReturnLow.prototype.test = function (dataItemsKeys) {
        return dataItemsKeys.includes('r2');
    }

    app.factory('counterLogSubtypeRegistry', function () {
        return {
            services: {},
            register: function (name, counterLogService) {
                this.services[name] = counterLogService;
            },
            get: function (name) {
                return this.services[name];
            }
        };
    });

    app.factory('counterLogSeriesSuppliers', function (chart, counterLogSeriesSupplier) {
        return {
            counterDaySeriesSuppliers: counterDaySeriesSuppliers,
            counterWeekSeriesSuppliers: counterWeekSeriesSuppliers,
            counterMonthYearSeriesSuppliers: counterMonthYearSeriesSuppliers,
            instantAndCounterDaySeriesSuppliers: instantAndCounterDaySeriesSuppliers,
            instantAndCounterWeekSeriesSuppliers: instantAndCounterWeekSeriesSuppliers,
            p1DaySeriesSuppliers: p1DaySeriesSuppliers,
            p1WeekSeriesSuppliers: p1WeekSeriesSuppliers,
            powerReturnedDaySeriesSuppliers: powerReturnedDaySeriesSuppliers,
            powerReturnedWeekSeriesSuppliers: powerReturnedWeekSeriesSuppliers,
            powerReturnedMonthYearSeriesSuppliers: powerReturnedMonthYearSeriesSuppliers
        };

        function counterDaySeriesSuppliers(deviceType) {
            return [
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('v', new ContainsPowerNormalOnly(), {
                    id: 'counterEnergyUsedOrGenerated',
                    label: 'A',
                    series: {
                        type: 'spline',
                        name: deviceType === chart.deviceTypes.EnergyUsed ? $.t('Energy Usage') : $.t('Energy Generated'),
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
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('v', new ContainsPowerNormalOnly(), {
                    id: 'counterEnergyUsedOrGenerated',
                    valueDecimals: 3,
                    label: 'B',
                    series: {
                        type: 'column',
                        name: deviceType === chart.deviceTypes.EnergyUsed ? $.t('Energy Usage') : $.t('Energy Generated'),
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
                counterLogSeriesSupplier.summingSeriesSupplier({
                    id: 'counterEnergyUsedOrGeneratedTotal',
                    dataItemKeys: ['v', 'v2'],
                    convertZeroToNull: true,
                    label: 'C',
                    series: {
                        type: 'column',
                        name: deviceType === chart.deviceTypes.EnergyUsed ? $.t('Total Usage') : deviceType === chart.deviceTypes.EnergyGenerated ? $.t('Total Generated') : $.t('Total Return'),
                        zIndex: 2,
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1000),
                            valueDecimals: 3
                        },
                        color: 'rgba(3,190,252,0.8)',
                        yAxis: 0
                    }
                }),
                counterLogSeriesSupplier.summingSeriesSupplier({
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
                        name: $.t('Trendline') + ' ' + (deviceType === chart.deviceTypes.EnergyUsed ? $.t('Usage') : deviceType === chart.deviceTypes.EnergyGenerated ? $.t('Generated') : $.t('Return')),
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
                counterLogSeriesSupplier.summingSeriesSupplier({
                    id: 'counterEnergyUsedOrGeneratedPrevious',
                    dataItemKeys: ['v', 'v2'],
                    useDataItemsFromPrevious: true,
                    label: 'E',
                    series: {
                        name: $.t('Past') + ' ' + (deviceType === chart.deviceTypes.EnergyUsed ? $.t('Usage') : deviceType === chart.deviceTypes.EnergyGenerated ? $.t('Generated') : $.t('Return')),
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
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('eu', new ContainsEnergyUsedAndPowerNormalOnly(), {
                    id: 'instantAndCounterEnergyUsedOrGenerated',
                    label: 'F',
                    series: {
                        type: 'column',
                        pointRange: 3600 * 1000,
                        zIndex: 5,
                        animation: false,
                        name: deviceType === chart.deviceTypes.EnergyUsed ? $.t('Energy Usage') : $.t('Energy Generated'),
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1)
                        },
                        color: 'rgba(3,190,252,0.8)',
                        yAxis: 0
                    }
                }),
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('v', new ContainsEnergyUsedAndPowerNormalOnly(), {
                    id: 'instantAndCounterPowerUsedOrGenerated',
                    label: 'G',
                    series: {
                        name: deviceType === chart.deviceTypes.EnergyUsed ? $.t('Power Usage') : $.t('Power Generated'),
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
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('eu', new ContainsEnergyUsedAndPowerNormalOnly(), {
                    id: 'instantAndCounterEnergyUsedOrGenerated',
                    valueDecimals: 3,
                    label: 'H',
                    series: {
                        type: 'column',
                        pointRange: 3600 * 1000,
                        zIndex: 5,
                        animation: false,
                        name: deviceType === chart.deviceTypes.EnergyUsed ? $.t('Energy Usage') : $.t('Energy Generated'),
                        tooltip: {
                            valueSuffix: ' ' + valueUnits.energy(valueMultipliers.m1000)
                        },
                        color: 'rgba(3,190,252,0.8)',
                        yAxis: 0
                    }
                }),
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('v', new ContainsEnergyUsedAndPowerNormalOnly(), {
                    id: 'instantAndCounterPowerUsedOrGenerated',
                    valueDecimals: 3,
                    label: 'I',
                    series: {
                        name: deviceType === chart.deviceTypes.EnergyUsed ? $.t('Power Usage') : $.t('Power Generated'),
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
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('eu', new ContainsEnergyUsedAndPowerNormalAndLow(), {
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
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('eg', new ContainsEnergyUsedAndPowerNormalAndLow(), {
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
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('v', new ContainsEnergyUsedAndPowerNormalAndLow(), {
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
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('v2', new ContainsEnergyUsedAndPowerNormalAndLow(), {
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
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('eu', new ContainsEnergyUsed(), {
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
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('eg', new ContainsEnergyGenerated(), {
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
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('v', new ContainsPowerNormalAndReturnNormal(), {
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
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('v2', new ContainsPowerLow(), {
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
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('r1', new ContainsReturnNormal(), {
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
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('r2', new ContainsReturnLow(), {
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
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('r1', new ContainsReturnNormal(), {
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
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('r2', new ContainsReturnLow(), {
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
                counterLogSeriesSupplier.summingSeriesSupplier({
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
                counterLogSeriesSupplier.summingSeriesSupplier({
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
                counterLogSeriesSupplier.summingSeriesSupplier({
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
    });

});