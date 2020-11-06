define(['app', 'log/CounterLogParams', 'log/CounterLogEnergySeriesSuppliers'], function (app) {

    app.directive('registerInstantAndCounter', function (counterLogSubtypeRegistry, counterLogEnergySeriesSuppliers) {
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
            extendDataRequestDay: function (dataRequest) {
                dataRequest.method = 1;
                return dataRequest;
            },
            yAxesDay: function (deviceType) {
                return [
                    {
                        title: {
                            text: $.t('Energy') + ' (Wh)'
                        }
                    },
                    {
                        title: {
                            text: $.t('Power') + ' (Watt)'
                        },
                        opposite: true
                    }
                ];
            },
            yAxesWeek: function (deviceType) {
                return [
                    {
                        maxPadding: 0.2,
                        title: {
                            text: $.t('Energy') + ' (kWh)'
                        }
                    }
                ];
            },
            yAxesMonthYear: function (deviceType) {
                return [
                    {
                        title: {
                            text: $.t('Energy') + ' (kWh)'
                        }
                    }
                ];
            },
            daySeriesSuppliers: function (deviceType) {
                return []
                    .concat(counterLogEnergySeriesSuppliers.instantAndCounterDaySeriesSuppliers(deviceType))
                    .concat(counterLogEnergySeriesSuppliers.counterDaySeriesSuppliers(deviceType));
            },
            weekSeriesSuppliers: function (deviceType) {
                return []
                    .concat(counterLogEnergySeriesSuppliers.instantAndCounterWeekSeriesSuppliers(deviceType))
                    .concat(counterLogEnergySeriesSuppliers.counterWeekSeriesSuppliers(deviceType));
            },
            monthYearSeriesSuppliers: function (deviceType) {
                return []
                    .concat(counterLogEnergySeriesSuppliers.counterMonthYearSeriesSuppliers(deviceType));
            }
        });
        return {
            template: ''
        };
    });

});
