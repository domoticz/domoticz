define(['app', 'log/Chart', 'log/CounterLogSeriesSupplier'], function (app) {

    function DoesContain(key) {
        this.key = key;
    }
    DoesContain.prototype.test = function (dataSeriesItemsKeys) {
        return dataSeriesItemsKeys.includes(this.key);
    }

    function DoesNotContain(key) {
        this.key = key;
    }
    DoesNotContain.prototype.test = function (dataSeriesItemsKeys) {
        return !dataSeriesItemsKeys.includes(this.key);
    }

    app.factory('counterLogEnergySeriesSuppliers', function (chart, counterLogSeriesSupplier) {
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
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('v', new DoesNotContain('eu'), {
                    id: 'counterEnergyUsedOrGenerated',
                    label: 'A',
                    series: {
                        type: 'spline',
                        name: deviceType === chart.deviceTypes.EnergyUsed ? $.t('Energy Usage') : $.t('Energy Generated'),
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1)
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
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('v', new DoesNotContain('eu'), {
                    id: 'counterEnergyUsedOrGenerated',
                    valueDecimals: 3,
                    label: 'B',
                    series: {
                        type: 'column',
                        name: deviceType === chart.deviceTypes.EnergyUsed ? $.t('Energy Usage') : $.t('Energy Generated'),
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1000)
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
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1000),
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
                        zIndex: 3,
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1000),
                            valueDecimals: 3
                        },
                        color: 'rgba(252,3,3,0.8)',
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
                        type: 'column',
                        name: $.t('Past') + ' ' + (deviceType === chart.deviceTypes.EnergyUsed ? $.t('Usage') : deviceType === chart.deviceTypes.EnergyGenerated ? $.t('Generated') : $.t('Return')),
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1000),
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
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('eu', new DoesContain('eu'), {
                    id: 'instantAndCounterEnergyUsedOrGenerated',
                    label: 'F',
                    series: {
                        type: 'column',
                        pointRange: 3600 * 1000,
                        zIndex: 5,
                        animation: false,
                        name: deviceType === chart.deviceTypes.EnergyUsed ? $.t('Energy Usage') : $.t('Energy Generated'),
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1)
                        },
                        color: 'rgba(3,190,252,0.8)',
                        yAxis: 0
                    }
                }),
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('v', new DoesContain('eu'), {
                    id: 'instantAndCounterPowerUsedOrGenerated',
                    label: 'G',
                    series: {
                        type: 'spline',
                        name: deviceType === chart.deviceTypes.EnergyUsed ? $.t('Power Usage') : $.t('Power Generated'),
                        zIndex: 10,
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.power(chart.valueMultipliers.m1)
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
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('eu', new DoesContain('eu'), {
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
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1000)
                        },
                        color: 'rgba(3,190,252,0.8)',
                        yAxis: 0
                    }
                }),
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('v', new DoesContain('eu'), {
                    id: 'instantAndCounterPowerUsedOrGenerated',
                    valueDecimals: 3,
                    label: 'I',
                    series: {
                        type: 'column',
                        name: deviceType === chart.deviceTypes.EnergyUsed ? $.t('Power Usage') : $.t('Power Generated'),
                        zIndex: 10,
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1000)
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
                {
                    id: 'p1EnergyUsedArea',
                    dataItemKeys: ['eu'],
                    label: 'J',
                    template: {
                        type: 'area',
                        name: $.t('Energy Usage'),
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1)
                        },
                        color: 'rgba(120,150,220,0.9)',
                        fillOpacity: 0.2,
                        yAxis: 0,
                        visible: false
                    }
                },
                {
                    id: 'p1EnergyGeneratedArea',
                    dataItemKeys: ['eg'],
                    label: 'K',
                    template: {
                        type: 'area',
                        name: $.t('Energy Returned'),
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1)
                        },
                        color: 'rgba(120,220,150,0.9)',
                        fillOpacity: 0.2,
                        yAxis: 0,
                        visible: false
                    }
                },
                {
                    id: 'p1PowerUsed',
                    dataItemKeys: ['v'],
                    label: 'L',
                    template: {
                        name: $.t('Usage') + ' 1',
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.power(chart.valueMultipliers.m1)
                        },
                        color: 'rgba(60,130,252,0.8)',
                        stack: 'susage',
                        yAxis: 1
                    }
                },
                {
                    id: 'p1PowerGenerated',
                    dataItemKeys: ['v2'],
                    label: 'M',
                    template: {
                        name: $.t('Usage') + ' 2',
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.power(chart.valueMultipliers.m1)
                        },
                        color: 'rgba(3,190,252,0.8)',
                        stack: 'susage',
                        yAxis: 1
                    }
                }
            ];
        }

        function p1WeekSeriesSuppliers(deviceType) {
            return [
                {
                    id: 'p1EnergyUsedArea',
                    dataItemKeys: ['eu'],
                    valueDecimals: 3,
                    label: 'N',
                    template: {
                        type: 'area',
                        name: $.t('Energy Usage'),
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1000)
                        },
                        color: 'rgba(120,150,220,0.9)',
                        fillOpacity: 0.2,
                        yAxis: 0,
                        visible: false
                    }
                },
                {
                    id: 'p1EnergyGeneratedArea',
                    dataItemKeys: ['eg'],
                    valueDecimals: 3,
                    label: 'O',
                    template: {
                        type: 'area',
                        name: $.t('Energy Returned'),
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1000)
                        },
                        color: 'rgba(120,220,150,0.9)',
                        fillOpacity: 0.2,
                        yAxis: 0,
                        visible: false
                    }
                },
                {
                    id: 'p1EnergyUsed',
                    dataItemKeys: ['v'],
                    valueDecimals: 3,
                    convertZeroToNull: true,
                    label: 'P',
                    template: {
                        name: $.t('Usage') + ' 1',
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1000)
                        },
                        color: 'rgba(60,130,252,0.8)',
                        stack: 'susage',
                        yAxis: 0
                    }
                },
                {
                    id: 'p1EnergyGenerated',
                    dataItemKeys: ['v2'],
                    valueDecimals: 3,
                    convertZeroToNull: true,
                    label: 'Q',
                    template: {
                        name: $.t('Usage') + ' 2',
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1000)
                        },
                        color: 'rgba(3,190,252,0.8)',
                        stack: 'susage',
                        yAxis: 0
                    }
                }
            ];
        }

        function powerReturnedDaySeriesSuppliers(deviceType) {
            return [
                {
                    id: 'powerReturned1',
                    dataItemKeys: ['r1'],
                    dataIsValid: function (data) {
                        return data.delivered === true;
                    },
                    label: 'R',
                    template: {
                        name: $.t('Return') + ' 1',
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.power(chart.valueMultipliers.m1)
                        },
                        color: 'rgba(30,242,110,0.8)',
                        stack: 'sreturn',
                        yAxis: 1
                    }
                },
                {
                    id: 'powerReturned2',
                    dataItemKeys: ['r2'],
                    dataIsValid: function (data) {
                        return data.delivered === true;
                    },
                    label: 'S',
                    template: {
                        name: $.t('Return') + ' 2',
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.power(chart.valueMultipliers.m1)
                        },
                        color: 'rgba(3,252,190,0.8)',
                        stack: 'sreturn',
                        yAxis: 1
                    }
                }
            ];
        }

        function powerReturnedWeekSeriesSuppliers(deviceType) {
            return [
                {
                    id: 'powerReturned1',
                    dataItemKeys: ['r1'],
                    dataIsValid: function (data) {
                        return data.delivered === true;
                    },
                    valueDecimals: 3,
                    convertZeroToNull: true,
                    label: 'T',
                    template: {
                        name: $.t('Return') + ' 1',
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1000)
                        },
                        color: 'rgba(30,242,110,0.8)',
                        stack: 'sreturn',
                        yAxis: 0
                    }
                },
                {
                    id: 'powerReturned2',
                    dataItemKeys: ['r2'],
                    dataIsValid: function (data) {
                        return data.delivered === true;
                    },
                    valueDecimals: 3,
                    convertZeroToNull: true,
                    label: 'U',
                    template: {
                        name: $.t('Return') + ' 2',
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1000)
                        },
                        color: 'rgba(3,252,190,0.8)',
                        stack: 'sreturn',
                        yAxis: 0
                    }
                }
            ];
        }

        function powerReturnedMonthYearSeriesSuppliers(deviceType) {
            return [
                counterLogSeriesSupplier.summingSeriesSupplier({
                    id: 'powerReturnedTotal',
                    dataIsValid: function (data) {
                        return data.delivered === true;
                    },
                    dataItemKeys: ['r1', 'r2'],
                    convertZeroToNull: true,
                    label: 'V',
                    series: {
                        type: 'column',
                        name: $.t('Total Return'),
                        zIndex: 1,
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1000),
                            valueDecimals: 3
                        },
                        color: 'rgba(3,252,190,0.8)',
                        yAxis: 0
                    }
                }),
                counterLogSeriesSupplier.summingSeriesSupplier({
                    id: 'powerReturnedTotalTrendline',
                    dataIsValid: function (data) {
                        return data.delivered === true;
                    },
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
                        zIndex: 3,
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1000),
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
                    dataIsValid: function (data) {
                        return data.delivered === true;
                    },
                    dataItemKeys: ['r1', 'r2'],
                    useDataItemsFromPrevious: true,
                    label: 'X',
                    series: {
                        type: 'column',
                        name: $.t('Past') + ' ' + $.t('Return'),
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1000),
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