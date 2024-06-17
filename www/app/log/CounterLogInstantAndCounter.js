define(['app', 'log/Chart', 'log/CounterLogParams', 'log/CounterLogEnergySeriesSuppliers'], function (app) {

    app.directive('registerInstantAndCounter', function (chart, counterLogSubtypeRegistry, counterLogParams, counterLogEnergySeriesSuppliers, counterLogSeriesSupplier) {
        counterLogSubtypeRegistry.register('instantAndCounter', {
            chartParamsDayTemplate: {
                highchartTemplate: {
                    chart: {
                        alignTicks: false
                    },
                },
                synchronizeYaxes: true
            },
            chartParamsMonthYearTemplate: {

            },
            chartParamsCompareTemplate: function (ctrl) {
                return counterLogParams.chartParamsCompareTemplate(ctrl, chart.valueUnits.energy(chart.valueMultipliers.m1000));
            },
            extendDataRequestDay: function (dataRequest) {
                dataRequest.method = 1;
                return dataRequest;
            },
            yAxesDay: function (deviceType) {
                return [
                    {
                        title: {
                            text: $.t('Energy') + ' (' + chart.valueUnits.energy(chart.valueMultipliers.m1) + ')'
                        }
                    },
                    {
                        title: {
                            text: $.t('Power') + ' (' + chart.valueUnits.power(chart.valueMultipliers.m1) + ')'
                        },
                        opposite: true
                    }
                ];
            },
            yAxesMonthYear: function (deviceType) {
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
            daySeriesSuppliers: function (deviceType) {
                return []
                    .concat(counterLogEnergySeriesSuppliers.instantAndCounterDaySeriesSuppliers(deviceType))
                    .concat(counterLogEnergySeriesSuppliers.counterDaySeriesSuppliers(deviceType));
            },
            monthYearSeriesSuppliers: function (deviceType) {
                return []
                    .concat(counterLogEnergySeriesSuppliers.counterMonthYearSeriesSuppliers(deviceType))
                    .concat(counterLogEnergySeriesSuppliers.trendlineMonthYearSeriesSuppliers(deviceType))
					.concat(counterLogEnergySeriesSuppliers.pastMonthYearSeriesSuppliers(deviceType))
					.concat(counterLogEnergySeriesSuppliers.priceMonthYearSeriesSuppliers(deviceType));
                    
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
    });

});
