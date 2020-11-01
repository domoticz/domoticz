define(['app', 'log/CounterLogSeriesSuppliers'], function (app) {

    app.directive('registerInstantAndCounter', function (counterLogSubtypeRegistry, counterLogSeriesSuppliers) {
        counterLogSubtypeRegistry.register('instantAndCounter', {
            chartParamsDayTemplate: {
                highchartTemplate: {
                    chart: {
                        alignTicks: false
                    },
                },
                synchronizeYaxes: true
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
            daySeriesSuppliers: function (deviceType) {
                return []
                    .concat(counterLogSeriesSuppliers.instantAndCounterDaySeriesSuppliers(deviceType))
                    .concat(counterLogSeriesSuppliers.counterDaySeriesSuppliers(deviceType));
            },
            weekSeriesSuppliers: function (deviceType) {
                return []
                    .concat(counterLogSeriesSuppliers.instantAndCounterWeekSeriesSuppliers(deviceType))
                    .concat(counterLogSeriesSuppliers.counterWeekSeriesSuppliers(deviceType));
            },
            monthYearSeriesSuppliers: function (deviceType) {
                return []
                    .concat(counterLogSeriesSuppliers.counterMonthYearSeriesSuppliers(deviceType));
            }
        });
        return {
            template: ''
        };
    });

});
