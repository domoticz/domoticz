define(['app', 'log/Chart', 'log/CounterLogParams', 'log/CounterLogEnergySeriesSuppliers'], function (app) {

    app.directive('registerP1Energy', function (chart, counterLogSubtypeRegistry, counterLogParams, counterLogEnergySeriesSuppliers, counterLogSeriesSupplier) {
        counterLogSubtypeRegistry.register('p1Energy', {
            chartParamsDayTemplate: {
                highchartTemplate: {
                    chart: {
                        alignTicks: true
                    },
                    xAxis: {
                        dateTimeLabelFormats: {
                            day: '%a'
                        }
                    }
                }
            },
            chartParamsWeekTemplate: {
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
            },
            chartParamsMonthYearTemplate: {
                highchartTemplate: {
                    tooltip: {
                        shared: false
                    }
                }
            },
            chartParamsCompareTemplate: function (ctrl) {
                return counterLogParams.chartParamsCompareTemplate(ctrl, chart.valueUnits.energy(chart.valueMultipliers.m1000));
            },
            yAxesDay: function (deviceType) {
                return [
                    {
                        title: {
                            text: $.t('Energy') + ' (' + chart.valueUnits.energy(chart.valueMultipliers.m1) + ')'
                        },
                        min: 0
                    },
                    {
                        title: {
                            text: $.t('Power') + ' (' + chart.valueUnits.power(chart.valueMultipliers.m1) + ')'
                        },
                        min: 0,
                        opposite: true
                    }
                ];
            },
            yAxesWeek: function (deviceType) {
                return [
                    {
                        maxPadding: 0.2,
                        title: {
                            text: $.t('Energy') + ' (' + chart.valueUnits.energy(chart.valueMultipliers.m1000) + ')'
                        },
                        min: 0
                    }
                ];
            },
            yAxesMonthYear: function (deviceType) {
                return [
                    {
                        title: {
                            text: $.t('Energy') + ' (' + chart.valueUnits.energy(chart.valueMultipliers.m1000) + ')'
                        },
                        min: 0
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
            daySeriesSuppliers: function (deviceType) {
                return []
                    .concat(counterLogEnergySeriesSuppliers.p1DaySeriesSuppliers(deviceType))
                    .concat(counterLogEnergySeriesSuppliers.powerReturnedDaySeriesSuppliers(deviceType));
            },
            weekSeriesSuppliers: function (deviceType) {
                return []
                    .concat(counterLogEnergySeriesSuppliers.p1WeekSeriesSuppliers(deviceType))
                    .concat(counterLogEnergySeriesSuppliers.powerReturnedWeekSeriesSuppliers(deviceType));
            },
            monthYearSeriesSuppliers: function (deviceType) {
                return []
                    .concat(counterLogEnergySeriesSuppliers.counterMonthYearSeriesSuppliers(deviceType))
                    .concat(counterLogEnergySeriesSuppliers.powerReturnedMonthYearSeriesSuppliers(deviceType));
            },
            compareSeriesSuppliers: function (ctrl) {
                return counterLogSeriesSupplier.counterCompareSeriesSuppliers(ctrl);
            }
        });
        return {
            template: ''
        };
    });

});
