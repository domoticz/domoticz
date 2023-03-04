define(['app', 'lodash', 'log/Chart', 'log/CounterLogParams', 'log/CounterLogCounterSeriesSuppliers', 'log/CounterLogEnergySeriesSuppliers', 'log/CounterLogSeriesSupplier'], function (app, _) {

    app.directive('registerCounter', function (chart, counterLogSubtypeRegistry, counterLogParams, counterLogCounterSeriesSuppliers, counterLogSeriesSupplier) {
        counterLogSubtypeRegistry.register('energy-used', {
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
                return counterLogParams.chartParamsCompareTemplate(ctrl, chart.valueUnits.energy(chart.valueMultipliers.m1000));
            },
            yAxesDay: function (deviceTypeIndex) {
                return [
                    {
                        title: {
                            text: $.t('Energy') + ' (' + chart.valueUnits.energy(chart.valueMultipliers.m1) + ')'
                        }
                    }
                ];
            },
            yAxesWeek: function (deviceTypeIndex) {
                return [
                    {
                        title: {
                            text: $.t('Energy') + ' (' + chart.valueUnits.energy(chart.valueMultipliers.m1000) + ')'
                        }
                    }
                ];
            },
            yAxesMonthYear: function (deviceTypeIndex) {
                return [
                    {
                        title: {
                            text: $.t('Energy') + ' (' + chart.valueUnits.energy(chart.valueMultipliers.m1000) + ')'
                        }
                    }
                ];
            },
            yAxesCompare: function (deviceTypeIndex) {
                return [
                    {
                        title: {
                            text: $.t('Energy') + ' (' + chart.valueUnits.energy(chart.valueMultipliers.m1000) + ')'
                        }
                    }
                ];
            },
            daySeriesSuppliers: function (deviceTypeIndex) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesDaySuppliers(deviceTypeIndex, chart.valueMultipliers.m1, undefined));
            },
            weekSeriesSuppliers: function (deviceTypeIndex) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesSuppliers(deviceTypeIndex, chart.valueMultipliers.m1000));
            },
            monthYearSeriesSuppliers: function (deviceTypeIndex) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesSuppliers(deviceTypeIndex, chart.valueMultipliers.m1000))
                    .concat(counterLogCounterSeriesSuppliers.counterTrendlineSeriesSuppliers(deviceTypeIndex, chart.valueMultipliers.m1000))
                    .concat(counterLogCounterSeriesSuppliers.counterPreviousSeriesSupplier(deviceTypeIndex, chart.valueMultipliers.m1000));
            },
            extendDataRequestCompare: function (dataRequest) {
                return dataRequest;
            },
            compareSeriesSuppliers: function (ctrl) {
                return counterLogSeriesSupplier.counterCompareSeriesSuppliers(ctrl);
            }
        });

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
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesDaySuppliers(deviceTypeIndex, chart.valueMultipliers.m1));
            },
            weekSeriesSuppliers: function (deviceTypeIndex) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesSuppliers(deviceTypeIndex, chart.valueMultipliers.m1));
            },
            monthYearSeriesSuppliers: function (deviceTypeIndex) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesSuppliers(deviceTypeIndex, chart.valueMultipliers.m1))
                    .concat(counterLogCounterSeriesSuppliers.counterTrendlineSeriesSuppliers(deviceTypeIndex, chart.valueMultipliers.m1))
                    .concat(counterLogCounterSeriesSuppliers.counterPreviousSeriesSupplier(deviceTypeIndex, chart.valueMultipliers.m1));
            },
            extendDataRequestCompare: function (dataRequest) {
                return dataRequest;
            },
            compareSeriesSuppliers: function (ctrl) {
                return counterLogSeriesSupplier.counterCompareSeriesSuppliers(ctrl);
            }
        });

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
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesDaySuppliers(deviceTypeIndex, chart.valueMultipliers.m1, times1000, 0));
            },
            weekSeriesSuppliers: function (deviceTypeIndex) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesSuppliers(deviceTypeIndex, chart.valueMultipliers.m1, times1000, 0));
            },
            monthYearSeriesSuppliers: function (deviceTypeIndex) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesSuppliers(deviceTypeIndex, chart.valueMultipliers.m1, times1000, 0))
                    .concat(counterLogCounterSeriesSuppliers.counterTrendlineSeriesSuppliers(deviceTypeIndex, chart.valueMultipliers.m1, times1000, 0))
                    .concat(counterLogCounterSeriesSuppliers.counterPreviousSeriesSupplier(deviceTypeIndex, chart.valueMultipliers.m1, times1000, 0));
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
            daySeriesSuppliers: function (deviceTypeIndex, divider) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesDaySuppliers(deviceTypeIndex, chart.valueMultipliers.m1, undefined, decimalPlacesDiv1(divider)));
            },
            weekSeriesSuppliers: function (deviceTypeIndex, divider) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesSuppliers(deviceTypeIndex, chart.valueMultipliers.m1, undefined, decimalPlacesDiv1(divider)));
            },
            monthYearSeriesSuppliers: function (deviceTypeIndex, divider) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesSuppliers(deviceTypeIndex, chart.valueMultipliers.m1, undefined, decimalPlacesDiv1(divider)))
                    .concat(counterLogCounterSeriesSuppliers.counterTrendlineSeriesSuppliers(deviceTypeIndex, chart.valueMultipliers.m1, undefined, decimalPlacesDiv1(divider)))
                    .concat(counterLogCounterSeriesSuppliers.counterPreviousSeriesSupplier(deviceTypeIndex, chart.valueMultipliers.m1, undefined, decimalPlacesDiv1(divider)));
            },
            extendDataRequestCompare: function (dataRequest) {
                return dataRequest;
            },
            compareSeriesSuppliers: function (ctrl) {
                return counterLogSeriesSupplier.counterCompareSeriesSuppliers(ctrl);
            }
        });

        counterLogSubtypeRegistry.register('energy-generated', {
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
                return counterLogParams.chartParamsCompareTemplate(ctrl, chart.valueUnits.energy(chart.valueMultipliers.m1000));
            },
            yAxesDay: function (deviceTypeIndex) {
                return [
                    {
                        title: {
                            text: $.t('Energy') + ' (' + chart.valueUnits.energy(chart.valueMultipliers.m1) + ')'
                        }
                    }
                ];
            },
            yAxesWeek: function (deviceTypeIndex) {
                return [
                    {
                        title: {
                            text: $.t('Energy') + ' (' + chart.valueUnits.energy(chart.valueMultipliers.m1000) + ')'
                        }
                    }
                ];
            },
            yAxesMonthYear: function (deviceTypeIndex) {
                return [
                    {
                        title: {
                            text: $.t('Energy') + ' (' + chart.valueUnits.energy(chart.valueMultipliers.m1000) + ')'
                        }
                    }
                ];
            },
            yAxesCompare: function (deviceTypeIndex) {
                return [
                    {
                        title: {
                            text: $.t('Energy') + ' (' + chart.valueUnits.energy(chart.valueMultipliers.m1000) + ')'
                        }
                    }
                ];
            },
            daySeriesSuppliers: function (deviceTypeIndex) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesDaySuppliers(deviceTypeIndex, chart.valueMultipliers.m1, undefined, chart.valueMultipliers.m1000));
            },
            weekSeriesSuppliers: function (deviceTypeIndex) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesSuppliers(deviceTypeIndex, chart.valueMultipliers.m1000));
            },
            monthYearSeriesSuppliers: function (deviceTypeIndex) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesSuppliers(deviceTypeIndex, chart.valueMultipliers.m1000))
                    .concat(counterLogCounterSeriesSuppliers.counterTrendlineSeriesSuppliers(deviceTypeIndex, chart.valueMultipliers.m1000))
                    .concat(counterLogCounterSeriesSuppliers.counterPreviousSeriesSupplier(deviceTypeIndex, chart.valueMultipliers.m1000));
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
            this.Divider = data.Divider ? data.Divider : 1;
            this.deviceCounterName = data.ValueQuantity ? data.ValueQuantity : undefined;
            this.deviceValueUnit = data.ValueUnits ? data.ValueUnits : undefined;
        }

        function times1000(value) {
            return value * 1000;
        }
    });

});
