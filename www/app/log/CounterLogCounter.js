define(['app', 'log/Chart', 'log/CounterLogParams', 'log/CounterLogCounterSeriesSuppliers'], function (app) {

    app.directive('registerCounter', function (chart, counterLogSubtypeRegistry, counterLogCounterSeriesSuppliers) {
        counterLogSubtypeRegistry.register('counter', {
            chartParamsDayTemplate: {

            },
            chartParamsWeekTemplate: {

            },
            chartParamsMonthYearTemplate: {

            },
            yAxesDay: [
                {
                    title: {
                        text: $.t('Gas') + ' (' + chart.valueUnits.gas(chart.valueMultipliers.m1) + ')'
                    }
                }
            ],
            yAxesWeek: [
                {
                    title: {
                        text: $.t('Gas') + ' (' + chart.valueUnits.gas(chart.valueMultipliers.m1) + ')'
                    }
                }
            ],
            yAxesMonthYear: [
                {
                    title: {
                        text: $.t('Gas') + ' (' + chart.valueUnits.gas(chart.valueMultipliers.m1) + ')'
                    }
                }
            ],
            daySeriesSuppliers: function (deviceType) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesSuppliers(deviceType));
            },
            weekSeriesSuppliers: function (deviceType) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesSuppliers(deviceType));
            },
            monthYearSeriesSuppliers: function (deviceType) {
                return []
                    .concat(counterLogCounterSeriesSuppliers.counterSeriesSuppliers(deviceType))
                    .concat(counterLogCounterSeriesSuppliers.counterTrendlineSeriesSuppliers(deviceType))
                    .concat(counterLogCounterSeriesSuppliers.counterPreviousSeriesSupplier(deviceType));
            }
        });
        return {
            template: ''
        };
    });

});
