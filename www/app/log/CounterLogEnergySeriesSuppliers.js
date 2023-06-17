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
                    convertZeroToNull: true,
                    label: 'A',
                    showWithoutDatapoints: false,
                    series: {
                        type: 'column',
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
                    showWithoutDatapoints: false,
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

        function instantAndCounterDaySeriesSuppliers(deviceType) {
            return [
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('eu', new DoesContain('eu'), {
                    id: 'instantAndCounterEnergyUsedOrGenerated',
                    convertZeroToNull: true,
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
                    showWithoutDatapoints: false,
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
                    convertZeroToNull: true,
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
                    showWithoutDatapoints: false,
                    series: {
                        type: 'spline',
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
                    showWithoutDatapoints: false,
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
                    showWithoutDatapoints: false,
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
                    id: 'p1EnergyGeneratedArea',
                    dataItemKeys: ['eg'],
                    valueDecimals: 3,
                    showWithoutDatapoints: false,
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
                    showWithoutDatapoints: false,
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
                    showWithoutDatapoints: false,
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
                    showWithoutDatapoints: false,
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
						//make all values negative for the graph
						for (var i = 0; i < data.result.length; i++) {
							data.result[i]['r1']= -data.result[i]['r1'];
						}
                        return data.delivered === true;
                    },
                    valueDecimals: 3,
                    convertZeroToNull: true,
                    showWithoutDatapoints: false,
                    label: 'T',
                    template: {
                        name: $.t('Return') + ' 1',
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1000)
                        },
                        color: 'rgba(30,242,110,0.8)',
                        stack: 'susage',
                        yAxis: 0,
                    }
                },
                {
                    id: 'powerReturned2',
                    dataItemKeys: ['r2'],
                    dataIsValid: function (data) {
						//make all values negative for the graph
						for (var i = 0; i < data.result.length; i++) {
							data.result[i]['r2']= -data.result[i]['r2'];
						}
                        return data.delivered === true;
                    },
                    valueDecimals: 3,
                    convertZeroToNull: true,
                    showWithoutDatapoints: false,
                    label: 'U',
                    template: {
                        name: $.t('Return') + ' 2',
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1000)
                        },
                        color: 'rgba(3,252,190,0.8)',
                        stack: 'susage',
                        yAxis: 0
                    }
                }
            ];
        }

		//Month/Year
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
                    postprocessDatapoints: chart.aggregateTrendline,
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
                    convertZeroToNull: true,
                    label: 'E',
                    series: {
                        type: 'spline',
                        name: $.t('Past') + ' ' + (deviceType === chart.deviceTypes.EnergyUsed ? $.t('Usage') : deviceType === chart.deviceTypes.EnergyGenerated ? $.t('Generated') : $.t('Return')),
						zIndex: 2,
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1000),
                            valueDecimals: 3
                        },
                        color: 'rgba(190,60,252,0.8)',
                        yAxis: 0,
                        visible: false
                    }
                })
            ];
        }

        function powerReturnedMonthYearSeriesSuppliers(deviceType) {
            return [
                counterLogSeriesSupplier.summingSeriesSupplier({
                    id: 'powerReturnedTotal',
                    dataIsValid: function (data) {
						//make all values negative for the graph
						for (var i = 0; i < data.result.length; i++) {
							data.result[i]['r1']= -data.result[i]['r1'];
							data.result[i]['r2']= -data.result[i]['r2'];
						}
                        return data.delivered === true;
                    },
                    dataItemKeys: ['r1','r2'],
                    convertZeroToNull: true,
                    showWithoutDatapoints: false,
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
                    postprocessDatapoints: chart.aggregateTrendline,
                    showWithoutDatapoints: false,
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
						//make all values negative for the graph
						if (typeof data.resultprev != 'undefined') {
							for (var i = 0; i < data.resultprev.length; i++) {
								data.resultprev[i]['r1']= -data.resultprev[i]['r1'];
								data.resultprev[i]['r2']= -data.resultprev[i]['r2'];
							}
						}
                        return data.delivered === true;
                    },
                    dataItemKeys: ['r1', 'r2'],
                    useDataItemsFromPrevious: true,
                    convertZeroToNull: true,
                    showWithoutDatapoints: false,
                    label: 'X',
                    series: {
                        type: 'spline',
                        name: $.t('Past') + ' ' + $.t('Return'),
						zIndex: 2, //line in front of bars?
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