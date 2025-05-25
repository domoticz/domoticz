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
            chartParamsHourTemplate: {
                highchartTemplate: {
                    plotOptions: {
                    },
                    tooltip: {
						headerFormat: '{point.x:%A, %B %d, %Y %H:00}<br/>',
                        outside: true,
						crosshairs: true,
						shared: true,
						//useHTML: true
                    }
                }
            },
            chartParamsMonthYearTemplate: {
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
						headerFormat: '{point.x:%A, %B %d}<br/>',
						//pointFormat: '<span style="color: {point.color}">●</span> {series.name}: <b>{abs3 point.y} {point.series.tooltipOptions.valueSuffix}</b><br>',
                        outside: true,
						crosshairs: true,
                        shared: true,
						//useHTML: true
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
                    },
                    {
                        title: {
                            text: $.t('Power') + ' (' + chart.valueUnits.power(chart.valueMultipliers.m1) + ')'
                        },
                        opposite: true
                    }
                ];
            },
            yAxesHour: function (deviceType) {
                return [
                    {
                        title: {
                            text: $.t('Energy') + ' (' + chart.valueUnits.energy(chart.valueMultipliers.m1) + ')'
                        },
						min: 0
                    },
                    {
                        title: {
                            text: $.t('Price') + ' (' + $.myglobals.currencysign + ')'
                        },
						visible: true,
						showEmpty: false,
						opposite: true
                    }
                ];
            },
            yAxesMonthYear: function (deviceType) {
                return [
                    {
						labels: {
							formatter: function () {
								return Math.abs(Highcharts.numberFormat(this.value, 0));
							}
						},
                        title: {
                            text: $.t('Energy') + ' (' + chart.valueUnits.energy(chart.valueMultipliers.m1000) + ')'
                        },
                    },
                    {
                        title: {
                            text: $.t('Price') + ' (' + $.myglobals.currencysign + ')'
                        },
						visible: true,
						showEmpty: false,
						opposite: true
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
            daySeriesSuppliers: function (deviceType, divider) {
                return []
                    .concat(counterLogEnergySeriesSuppliers.p1DaySeriesSuppliers(deviceType))
                    .concat(counterLogEnergySeriesSuppliers.powerReturnedDaySeriesSuppliers(deviceType));
            },
            hourSeriesSuppliers: function (deviceType) {
                return []
                    .concat(counterLogEnergySeriesSuppliers.p1HourSeriesSuppliers(deviceType))
                    .concat(counterLogEnergySeriesSuppliers.powerReturnedHourSeriesSuppliers(deviceType))
                    .concat(counterLogEnergySeriesSuppliers.p1PriceSeriesSuppliers(deviceType));
            },
            monthYearSeriesSuppliers: function (deviceType) {
                return []
                    .concat(counterLogEnergySeriesSuppliers.p1MonthYearSeriesSuppliers(deviceType))
                    .concat(counterLogEnergySeriesSuppliers.p1TrendlineMonthYearSeriesSuppliers(deviceType))
					.concat(counterLogEnergySeriesSuppliers.powerReturnedMonthYearSeriesSuppliers(deviceType))
					.concat(counterLogEnergySeriesSuppliers.powerTrendlineReturnedMonthYearSeriesSuppliers(deviceType))
					.concat(counterLogEnergySeriesSuppliers.p1PastMonthYearSeriesSuppliers(deviceType))
					.concat(counterLogEnergySeriesSuppliers.powerPastReturnedMonthYearSeriesSuppliers(deviceType))
                    .concat(counterLogEnergySeriesSuppliers.p1PriceSeriesSuppliers(deviceType));
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
