define(['app', 'lodash', 'log/Chart', 'log/CounterLogParams', 'log/CounterLogCounterSeriesSuppliers', 'log/CounterLogEnergySeriesSuppliers', 'log/CounterLogSeriesSupplier'], function (app, _) {

    app.directive('registerCounter', function (chart, counterLogSubtypeRegistry, counterLogCounterSeriesSuppliers, counterLogSeriesSupplier) {
        counterLogSubtypeRegistry.register('gas', {
            chartParamsDayTemplate: {

            },
            chartParamsWeekTemplate: {
                highchartTemplate: {
                    plotOptions: {
                        column: {
                            dataLabels: {
                                enabled: true
                            }
                        }
                    }
                }
            },
            chartParamsMonthYearTemplate: {

            },
            chartParamsCompareTemplate: {
                highchartTemplate: {
                    chart: {
                        type: 'column'
                    },
                    plotOptions: {
                        column: {
                            stacking: 'normal'
                        }
                    }
                }
            },
            yAxesDay: function (deviceTypeIndex) {
                return [
                    {
                        title: {
                            text: $.t('Gas') + ' (' + chart.valueUnits.gas(chart.valueMultipliers.m1) + ')'
                        }
                    }
                ];
            },
            yAxesWeek: function (deviceTypeIndex) {
                return [
                    {
                        title: {
                            text: $.t('Gas') + ' (' + chart.valueUnits.gas(chart.valueMultipliers.m1) + ')'
                        }
                    }
                ];
            },
            yAxesMonthYear: function (deviceTypeIndex) {
                return [
                    {
                        title: {
                            text: $.t('Gas') + ' (' + chart.valueUnits.gas(chart.valueMultipliers.m1) + ')'
                        }
                    }
                ];
            },
            yAxesCompare: function (deviceTypeIndex) {
                return [
                    {
                        title: {
                            text: $.t('Gas') + ' (' + chart.valueUnits.gas(chart.valueMultipliers.m1) + ')'
                        }
                    }
                ];
            },
            daySeriesSuppliers: function (deviceTypeIndex) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesSuppliers(deviceTypeIndex));
            },
            weekSeriesSuppliers: function (deviceTypeIndex) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesSuppliers(deviceTypeIndex));
            },
            monthYearSeriesSuppliers: function (deviceTypeIndex) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesSuppliers(deviceTypeIndex))
                    .concat(counterLogCounterSeriesSuppliers.counterTrendlineSeriesSuppliers(deviceTypeIndex))
                    .concat(counterLogCounterSeriesSuppliers.counterPreviousSeriesSupplier(deviceTypeIndex));
            },
            compareSeriesSuppliers: function (deviceTypeIndex) {
                return counterLogSeriesSupplier.counterCompareYearSeriesSuppliers(deviceTypeIndex);
            }
        });

        function times1000(value) {
            return value * 1000;
        }

        counterLogSubtypeRegistry.register('water', {
            chartParamsDayTemplate: {

            },
            chartParamsWeekTemplate: {
                highchartTemplate: {
                    plotOptions: {
                        column: {
                            dataLabels: {
                                enabled: true
                            }
                        }
                    }
                }
            },
            chartParamsMonthYearTemplate: {

            },
            chartParamsCompareTemplate: {
                highchartTemplate: {
                    chart: {
                        type: 'column'
                    },
                    plotOptions: {
                        column: {
                            stacking: 'normal'
                        }
                    }
                }
            },
            yAxesDay: function (deviceTypeIndex) {
                return [
                    {
                        title: {
                            text: $.t('Water') + ' (' + chart.valueUnits.water(chart.valueMultipliers.m1) + ')'
                        }
                    }
                ];
            },
            yAxesWeek: function (deviceTypeIndex) {
                return [
                    {
                        title: {
                            text: $.t('Water') + ' (' + chart.valueUnits.water(chart.valueMultipliers.m1) + ')'
                        }
                    }
                ];
            },
            yAxesMonthYear: function (deviceTypeIndex) {
                return [
                    {
                        title: {
                            text: $.t('Water') + ' (' + chart.valueUnits.water(chart.valueMultipliers.m1) + ')'
                        }
                    }
                ];
            },
            yAxesCompare: function (deviceTypeIndex) {
                return [
                    {
                        title: {
                            text: $.t('Water') + ' (' + chart.valueUnits.water(chart.valueMultipliers.m1) + ')'
                        }
                    }
                ];
            },
            daySeriesSuppliers: function (deviceTypeIndex) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesSuppliers(deviceTypeIndex, times1000, 0));
            },
            weekSeriesSuppliers: function (deviceTypeIndex) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesSuppliers(deviceTypeIndex, times1000, 0));
            },
            monthYearSeriesSuppliers: function (deviceTypeIndex) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesSuppliers(deviceTypeIndex, times1000, 0))
                    .concat(counterLogCounterSeriesSuppliers.counterTrendlineSeriesSuppliers(deviceTypeIndex, times1000, 0))
                    .concat(counterLogCounterSeriesSuppliers.counterPreviousSeriesSupplier(deviceTypeIndex, times1000, 0));
            },
            compareSeriesSuppliers: function (deviceTypeIndex) {
                return counterLogSeriesSupplier.counterCompareYearSeriesSuppliers(deviceTypeIndex);
            }
        });

        counterLogSubtypeRegistry.register('counter', {
            preprocessData: function (data) {
                this.deviceCounterName = data.ValueQuantity ? data.ValueQuantity : undefined;
                this.deviceValueUnit = data.ValueUnits ? data.ValueUnits : undefined;
            },
            chartParamsDayTemplate: {

            },
            chartParamsWeekTemplate: {
                highchartTemplate: {
                    plotOptions: {
                        column: {
                            dataLabels: {
                                enabled: true
                            }
                        }
                    }
                }
            },
            chartParamsMonthYearTemplate: {

            },
            chartParamsCompareTemplate: {
                highchartTemplate: {
                    chart: {
                        type: 'column'
                    },
                    plotOptions: {
                        column: {
                            stacking: 'normal'
                        }
                    }
                }
            },
            yAxesDay: function (deviceTypeIndex) {
                return [
                    {
                        title: {
                            text: $.t('Count')
                        }
                    }
                ];
            },
            yAxesWeek: function (deviceTypeIndex) {
                return [
                    {
                        title: {
                            text: $.t('Count')
                        }
                    }
                ];
            },
            yAxesMonthYear: function (deviceTypeIndex) {
                return [
                    {
                        title: {
                            text: $.t('Count')
                        }
                    }
                ];
            },
            yAxesCompare: function (deviceTypeIndex) {
                return [
                    {
                        title: {
                            text: $.t('Count')
                        }
                    }
                ];
            },
            daySeriesSuppliers: function (deviceTypeIndex) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesSuppliers(deviceTypeIndex, undefined, undefined));
            },
            weekSeriesSuppliers: function (deviceTypeIndex) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesSuppliers(deviceTypeIndex, undefined, undefined));
            },
            monthYearSeriesSuppliers: function (deviceTypeIndex) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesSuppliers(deviceTypeIndex, undefined, undefined))
                    .concat(counterLogCounterSeriesSuppliers.counterTrendlineSeriesSuppliers(deviceTypeIndex, undefined, undefined))
                    .concat(counterLogCounterSeriesSuppliers.counterPreviousSeriesSupplier(deviceTypeIndex, undefined, undefined));
            },
            compareSeriesSuppliers: function (deviceTypeIndex) {
                return counterLogSeriesSupplier.counterCompareYearSeriesSuppliers(deviceTypeIndex);
            }
        });
        return {
            template: ''
        };
    });

});
