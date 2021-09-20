define(['app', 'lodash', 'log/Chart', 'log/CounterLogParams', 'log/CounterLogCounterSeriesSuppliers', 'log/CounterLogEnergySeriesSuppliers', 'log/CounterLogSeriesSupplier'], function (app, _) {

    app.directive('registerCounter', function (chart, counterLogSubtypeRegistry, counterLogParams, counterLogCounterSeriesSuppliers, counterLogSeriesSupplier) {
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
            chartParamsCompareTemplate: function (ctrl) {
                return counterLogParams.chartParamsCompareTemplate(ctrl, chart.valueUnits.gas(chart.valueMultipliers.m1));
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
            extendDataRequestCompare: function (dataRequest) {
                return dataRequest;
            },
            compareSeriesSuppliers: function (ctrl) {
                return counterLogSeriesSupplier.counterCompareSeriesSuppliers(ctrl);
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
            chartParamsCompareTemplate: function (ctrl) {
                return counterLogParams.chartParamsCompareTemplate(ctrl, chart.valueUnits.water(chart.valueMultipliers.m1000));
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
                            text: $.t('Water') + ' (' + chart.valueUnits.water(chart.valueMultipliers.m1000) + ')'
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
            extendDataRequestCompare: function (dataRequest) {
                return dataRequest;
            },
            compareSeriesSuppliers: function (ctrl) {
                return counterLogSeriesSupplier.counterCompareSeriesSuppliers(ctrl);
            }
        });

        counterLogSubtypeRegistry.register('counter', {
            preprocessDayData: function (data) {
                preprocessData.call(this, data);
            },
            preprocessWeekData: function (data) {
                preprocessData.call(this, data);
            },
            preprocessMonthYearData: function (data) {
                preprocessData.call(this, data);
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
            chartParamsCompareTemplate: function (ctrl) {
                return counterLogParams.chartParamsCompareTemplate(ctrl);
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
            extendDataRequestCompare: function (dataRequest) {
                return dataRequest;
            },
            compareSeriesSuppliers: function (ctrl) {
                return counterLogSeriesSupplier.counterCompareSeriesSuppliers(ctrl);
            }
        });
        return {
            template: ''
        };

        function preprocessData(data) {
            this.deviceCounterName = data.ValueQuantity ? data.ValueQuantity : undefined;
            this.deviceValueUnit = data.ValueUnits ? data.ValueUnits : undefined;
        }
    });

});
