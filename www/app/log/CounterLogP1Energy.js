define(['app', 'log/CounterLogParams', 'log/CounterLogEnergySeriesSuppliers'], function (app) {

    app.directive('registerP1Energy', function (counterLogSubtypeRegistry, counterLogEnergySeriesSuppliers) {
        counterLogSubtypeRegistry.register('p1Energy', {
            chartParamsDayTemplate: {
                highchartTemplate: {
                    chart: {
                        alignTicks: true
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
            yAxesDay: [
                {
                    title: {
                        text: $.t('Energy') + ' (Wh)'
                    },
                    min: 0
                },
                {
                    title: {
                        text: $.t('Power') + ' (Watt)'
                    },
                    min: 0,
                    opposite: true
                }
            ],
            yAxesWeek: [
                {
                    maxPadding: 0.2,
                    title: {
                        text: $.t('Energy') + ' (kWh)'
                    },
                    min: 0
                }
            ],
            yAxesMonthYear: [
                {
                    title: {
                        text: $.t('Energy') + ' (kWh)'
                    },
                    min: 0
                }
            ],
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
            }
        });
        return {
            template: ''
        };
    });

});
