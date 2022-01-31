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
                        pointFormat: '{series.name}: <b>{point.y:,.f}</b> ( {point.percentage:.0f}% )<br>',
                        footerFormat: 'Total: {point.total:,.f} {series.tooltipOptions.valueSuffix}'
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
                ctrl.toggleTitleState = function () {
                    const sensorareaPrevious = this.sensorarea;
                    if (this.sensorarea === 'usage' && this.dataContainsDelivery) {
                        this.sensorarea = 'delivery';
                    } else /* if (this.sensorarea === 'delivery') */ {
                        this.sensorarea = 'usage';
                    }
                    return this.sensorarea !== sensorareaPrevious;
                };
                return _.merge(
                    counterLogParams.chartParamsCompareTemplate(ctrl, chart.valueUnits.energy(chart.valueMultipliers.m1000)),
                    {
                        chartName: function () {
                            return $.t('Comparing')
                                + ' <span class="' + ((ctrl.sensorarea || 'usage') === 'usage' ? 'chart-title-active' : 'chart-title-inactive') + '">' + $.t('Usage') + '</span>'
                                + (ctrl.dataContainsDelivery ? ' / <span class="' + (ctrl.sensorarea === 'delivery' ? 'chart-title-active' : 'chart-title-inactive') + '">' + $.t('Return') + '</span>' : '');
                        },
                        chartNameIsToggling: function () {
                            return ctrl.dataContainsDelivery;
                        },
                        trendValuationIsReversed: function () {
                            return ctrl.dataContainsDelivery && ctrl.sensorarea === 'delivery';
                        }
                    }
                );
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
            preprocessCompareData: function (data) {
                this.dataContainsDelivery = data.delivered ? true : false;
                this.sensorarea = this.sensorarea || 'usage';
            },
            extendDataRequestCompare: function (dataRequest) {
                dataRequest['sensorarea'] = this.sensorarea || 'usage';
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
