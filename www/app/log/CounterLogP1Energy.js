define(['app', 'log/CounterLogSeriesSuppliers'], function (app) {

    app.directive('registerP1Energy', function (counterLogSubtypeRegistry, counterLogSeriesSuppliers) {
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
            daySeriesSuppliers: function (deviceType) {
                return []
                    .concat(counterLogSeriesSuppliers.p1DaySeriesSuppliers(deviceType))
                    .concat(counterLogSeriesSuppliers.powerReturnedDaySeriesSuppliers(deviceType));
            },
            weekSeriesSuppliers: function (deviceType) {
                return []
                    .concat(counterLogSeriesSuppliers.p1WeekSeriesSuppliers(deviceType))
                    .concat(counterLogSeriesSuppliers.powerReturnedWeekSeriesSuppliers(deviceType));
            },
            monthYearSeriesSuppliers: function (deviceType) {
                return []
                    .concat(counterLogSeriesSuppliers.counterMonthYearSeriesSuppliers(deviceType))
                    .concat(counterLogSeriesSuppliers.powerReturnedMonthYearSeriesSuppliers(deviceType));
            }
        });
        return {
            template: ''
        };
    });

});
