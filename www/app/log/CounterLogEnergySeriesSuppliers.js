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
            instantAndCounterDaySeriesSuppliers: instantAndCounterDaySeriesSuppliers,
            powerReturnedDaySeriesSuppliers: powerReturnedDaySeriesSuppliers,
            powerReturnedHourSeriesSuppliers: powerReturnedHourSeriesSuppliers,

            counterMonthYearSeriesSuppliers: counterMonthYearSeriesSuppliers,
            priceMonthYearSeriesSuppliers: priceMonthYearSeriesSuppliers,
			trendlineMonthYearSeriesSuppliers: trendlineMonthYearSeriesSuppliers,
			pastMonthYearSeriesSuppliers: pastMonthYearSeriesSuppliers,

            powerReturnedMonthYearSeriesSuppliers: powerReturnedMonthYearSeriesSuppliers,
			powerTrendlineReturnedMonthYearSeriesSuppliers: powerTrendlineReturnedMonthYearSeriesSuppliers,
			powerPastReturnedMonthYearSeriesSuppliers: powerPastReturnedMonthYearSeriesSuppliers,

            p1DaySeriesSuppliers: p1DaySeriesSuppliers,
            p1HourSeriesSuppliers: p1HourSeriesSuppliers,
            p1MonthYearSeriesSuppliers: p1MonthYearSeriesSuppliers,
			p1PastMonthYearSeriesSuppliers: p1PastMonthYearSeriesSuppliers,
            p1PriceHourSeriesSuppliers: p1PriceHourSeriesSuppliers,
			p1TrendlineMonthYearSeriesSuppliers: p1TrendlineMonthYearSeriesSuppliers,
        };

        function counterDaySeriesSuppliers(deviceType) {
            return [
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('v', new DoesNotContain('eu'), {
                    id: 'CDSS',
                    convertZeroToNull: true,
                    showWithoutDatapoints: false,
                    label: 'A',
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

        function instantAndCounterDaySeriesSuppliers(deviceType) {
            return [
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('eu', new DoesContain('eu'), {
                    id: 'IACDSS',
                    convertZeroToNull: true,
                    showWithoutDatapoints: false,
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
                    id: 'CLSS',
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

        function p1DaySeriesSuppliers(deviceType) {
            return [
                {
                    id: 'p1DSSEU',
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
                    id: 'p1DSSEG',
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
                    id: 'p1DSSU',
                    dataItemKeys: ['v'],
                    showWithoutDatapoints: false,
                    label: 'L0',
                    template: {
                        name: $.t('Usage'),
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.power(chart.valueMultipliers.m1)
                        },
                        color: 'rgba(3,190,252,0.8)',
                        stack: 'susage',
                        yAxis: 1
                    }
                },
                {
                    id: 'p1DSSU1',
                    dataItemKeys: ['v1'],
                    showWithoutDatapoints: false,
                    label: '<=',
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
                    id: 'p1DSSU2',
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
		//Month/Year
        function p1MonthYearSeriesSuppliers(deviceType) {
            return [
                counterLogSeriesSupplier.summingSeriesSupplier({
                    id: 'P1MYSS',
                    dataItemKeys: ['v1', 'v2'],
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
                })
            ];
        }
        function p1PastMonthYearSeriesSuppliers(deviceType) {
            return [
                counterLogSeriesSupplier.summingSeriesSupplier({
                    id: 'P1PMYSS',
                    dataItemKeys: ['v1', 'v2'],
                    useDataItemsFromPrevious: true,
                    convertZeroToNull: true,
                    label: 'D',
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

        function p1HourSeriesSuppliers(deviceType) {
            return [
                counterLogSeriesSupplier.summingSeriesSupplier({
                    id: 'P1HSS',
                    dataItemKeys: ['v'],
                    label: 'C',
                    series: {
                        type: 'column',
                        name: $.t('Usage'),
                        zIndex: 2,
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1),
                            valueDecimals: 0
                        },
						//pointPadding: 0.0,
						pointPlacement: 0,
						//pointWidth: 10,
                        color: 'rgba(3,190,252,0.8)',
                        yAxis: 0
                    }
                })
            ];
        }

        function p1TrendlineMonthYearSeriesSuppliers(deviceType) {
            return [
                counterLogSeriesSupplier.summingSeriesSupplier({
                    id: 'P1TMYSS',
                    dataItemKeys: ['v1', 'v2'],
                    postprocessDatapoints: chart.aggregateTrendline,
                    label: 'E',
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
                })
            ];
        }

        function powerReturnedHourSeriesSuppliers(deviceType) {
			return [
				counterLogSeriesSupplier.summingSeriesSupplier({
					id: 'pRHSS',
					dataItemKeys: ['r'],
                    dataIsValid: function (data) {
                        return data.delivered === true;
                    },
					label: 'C',
					series: {
						type: 'column',
						name: $.t('Return'),
						zIndex: 2,
						tooltip: {
							valueSuffix: ' ' + chart.valueUnits.energy(chart.valueMultipliers.m1),
							valueDecimals: 0
						},
						pointPadding: 0.2,
						pointPlacement: 0,
						//pointWidth: 10,
						color: 'rgba(3,252,190,0.8)',
						yAxis: 0
					}
				})
            ];
        }
        function p1PriceHourSeriesSuppliers(deviceType) {
            return [
                {
                    id: 'P1PHSS',
                    dataItemKeys: ['p'],
                    convertZeroToNull: true,
                    label: '2',
                    template: function (seriesSupplier) {
                        return {
							type: 'spline',
                            name: $.t('Costs'),
                            zIndex: 3,
							tooltip: {
								valueSuffix: ' ' + $.myglobals.currencysign
							},
							color: 'rgba(190,252,60,0.8)',
							showInLegend: false,
                            yAxis: 0
                        };
                    }
                }
            ];
        }

        function powerReturnedDaySeriesSuppliers(deviceType) {
            return [
                {
                    id: 'PRDSSR',
                    dataItemKeys: ['r'],
                    dataIsValid: function (data) {
                        return data.delivered === true;
                    },
                    showWithoutDatapoints: false,
                    label: 'R',
                    template: {
                        name: $.t('Return'),
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.power(chart.valueMultipliers.m1)
                        },
                        color: 'rgba(3,252,190,0.8)',
                        stack: 'sreturn',
                        yAxis: 1
                    }
                },
                {
                    id: 'PRDSS1',
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
                    id: 'PRDSS2',
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

		//Month/Year
        function counterMonthYearSeriesSuppliers(deviceType) {
            return [
                counterLogSeriesSupplier.summingSeriesSupplier({
                    id: 'CMYSS',
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
                })
            ];
        }

        function trendlineMonthYearSeriesSuppliers(deviceType) {
            return [
                counterLogSeriesSupplier.summingSeriesSupplier({
                    id: 'TMYSS',
                    dataItemKeys: ['v'],
                    postprocessDatapoints: chart.aggregateTrendline,
                    label: 'E',
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
                })
            ];
        }

        function pastMonthYearSeriesSuppliers(deviceType) {
            return [
                counterLogSeriesSupplier.summingSeriesSupplier({
                    id: 'PMYSS',
                    dataItemKeys: ['v'],
                    useDataItemsFromPrevious: true,
                    convertZeroToNull: true,
                    label: 'D',
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
                    id: 'PRMSS',
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
                })
            ];
        }

        function powerTrendlineReturnedMonthYearSeriesSuppliers(deviceType) {
            return [
                counterLogSeriesSupplier.summingSeriesSupplier({
                    id: 'PTRMYSS',
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
                })
            ];
        }
        function powerPastReturnedMonthYearSeriesSuppliers(deviceType) {
            return [
                counterLogSeriesSupplier.summingSeriesSupplier({
                    id: 'PPRMYSS',
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

        function priceMonthYearSeriesSuppliers(deviceType) {
            return [
                counterLogSeriesSupplier.dataItemsKeysPredicatedSeriesSupplier('p', new DoesNotContain('eu'), {
                    id: 'PRMYSS',
                    convertZeroToNull: true,
                    valueDecimals: 4,
                    label: 'B',
                    showWithoutDatapoints: false,
                    series: {
                        type: 'spline',
                        name: (deviceType === chart.deviceTypes.EnergyUsed ? $.t('Costs') : deviceType === chart.deviceTypes.EnergyGenerated ? $.t('Earned') : $.t('Earned')),
                        zIndex: 4,
                        tooltip: {
                            valueSuffix: ' ' + $.myglobals.currencysign
                        },
                        color: 'rgba(190,252,60,0.8)',
                        yAxis: 0,
                        visible: true
                    }
                })
            ];
        }
		
    });

});