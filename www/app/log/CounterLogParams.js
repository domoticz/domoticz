define(['app', 'log/Chart'], function (app) {

    app.factory('counterLogSubtypeRegistry', function () {
        return {
            services: {},
            register: function (name, counterLogService) {
                this.services[name] = counterLogService;
            },
            get: function (name) {
                return this.services[name];
            }
        };
    });

    app.factory('counterLogParams', function (chart) {
        return {
            chartParamsDay: chartParamsDay,
            chartParamsWeek: chartParamsWeek,
            chartParamsMonthYear: chartParamsMonthYear,
            chartParamsCompare: chartParamsCompare,
            chartParamsCompareTemplate: chartParamsCompareTemplate
        }

        function chartParamsDay(domoticzGlobals, ctrl, chartParamsTemplate, dataSupplierTemplate, seriesSuppliers) {
            return _.merge(
                {
                    highchartTemplate: {
                        xAxis: {
                            dateTimeLabelFormats: {
                                day: '%a'
                            }
                        },
                        plotOptions: {
							column: {
								pointPlacement: 0
							},
                            series: {
                                matchExtremes: true
                            }
                        }
                    },
                    ctrl: ctrl,
                    range: ctrl.range,
                    device: ctrl.device,
                    sensorType: 'counter',
                    chartName: ctrl.device.SwitchTypeVal === chart.deviceTypes.EnergyGenerated ? $.t('Generated') : $.t('Usage'),
                    autoRefreshIsEnabled: function () {
                        return ctrl.logCtrl.autoRefresh;
                    },
                    dataSupplier:
                        _.merge(
                            {
                                seriesSuppliers: seriesSuppliers
                            },
                            dataSupplierTemplate
                        )
                },
                chartParamsTemplate
            );
        }

        function chartParamsWeek(domoticzGlobals, ctrl, chartParamsTemplate, dataSupplierTemplate, seriesSuppliers) {
            return _.merge(
                {
                    highchartTemplate: {
                        chart: {
                            type: 'column',
                            zoomType: false,
                            marginRight: 10
                        },
                        xAxis: {
                            dateTimeLabelFormats: {
                                day: '%a'
                            },
                            tickInterval: 24 * 3600 * 1000
                        },
                        plotOptions: {
                            column: {
                                pointPlacement: 0,
                                stacking: undefined
                            },
                            series: {
                                // colorByPoint: true
                                stacking: undefined
                            }
                        },
                        tooltip: {
                            shared: false,
                            crosshairs: false
                        }
                    },
                    ctrl: ctrl,
                    range: ctrl.range,
                    device: ctrl.device,
                    sensorType: 'counter',
                    chartName: ctrl.device.SwitchTypeVal === chart.deviceTypes.EnergyGenerated ? $.t('Generated') : $.t('Usage'),
                    autoRefreshIsEnabled: function () {
                        return ctrl.logCtrl.autoRefresh;
                    },
                    dataSupplier:
                        _.merge(
                            {
                                seriesSuppliers: seriesSuppliers
                            },
                            dataSupplierTemplate
                        )
                },
                chartParamsTemplate
            );
        }

        function chartParamsMonthYear(domoticzGlobals, ctrl, chartParamsTemplate, dataSupplierTemplate, seriesSuppliers) {
            return _.merge(
                {
                    highchartTemplate: {
                        chart: {
                            marginRight: 10
                        },
                        tooltip: {
                            crosshairs: false
                        },
						plotOptions: {
							column: {
								pointPlacement: 0
							}
						}
                    },
                    ctrl: ctrl,
                    range: ctrl.range,
                    device: ctrl.device,
                    sensorType: 'counter',
                    chartName: ctrl.device.SwitchTypeVal === chart.deviceTypes.EnergyGenerated ? $.t('Generated') : $.t('Usage'),
                    autoRefreshIsEnabled: function () {
                        return ctrl.logCtrl.autoRefresh;
                    },
                    dataSupplier:
                        _.merge(
                            {
                                seriesSuppliers: seriesSuppliers
                            },
                            dataSupplierTemplate
                        )
                },
                chartParamsTemplate
            );
        }

        function chartParamsCompare(domoticzGlobals, ctrl, chartParamsTemplate, dataSupplierTemplate, seriesSuppliers) {
            return _.merge(
                {
                    highchartTemplate: {
                        xAxis: {
                            type: 'category'
                        },
                        plotOptions: {
                            column: {
                                pointPlacement: 0,
                                stacking: undefined
                            },
                            series: {
                                // colorByPoint: true
                                stacking: undefined
                            }
                        },
                        chart: {
                            marginRight: 10
                        },
                        tooltip: {
                            crosshairs: false
                        }
                    },
                    ctrl: ctrl,
                    range: ctrl.range,
                    device: ctrl.device,
                    sensorType: 'counter',
                    chartName: ctrl.device.SwitchTypeVal === chart.deviceTypes.EnergyGenerated ? $.t('Generated') : $.t('Usage'),
                    autoRefreshIsEnabled: function () {
                        return ctrl.logCtrl.autoRefresh;
                    },
                    dataSupplier:
                        _.merge(
                            {
                                seriesSuppliers: seriesSuppliers
                            },
                            dataSupplierTemplate
                        )
                },
                chartParamsTemplate
            );
        }

        function chartParamsCompareTemplate(ctrl, deviceUnit) {
            const deviceType = ctrl.device.SwitchTypeVal;
            const template = {
                chartName: $.t('Comparing') + ' ' + $.t(deviceType === chart.deviceTypes.EnergyUsed ? 'Usage' : deviceType === chart.deviceTypes.EnergyGenerated ? 'Generated' : chart.deviceTypes.fromIndex(deviceType)),
                trendValuationIsReversed: function () {
                    return deviceType === chart.deviceTypes.EnergyGenerated;
                },
                highchartTemplate: {
                    chart: {
                        type: 'column'
                    },
                    plotOptions: {
                        column: {
                            stacking: ctrl.groupingBy === 'year' ? 'normal' : undefined
                        }
                    },
                    xAxis: {
                        labels: {
                            formatter: function () {
                                return categoryKeyToString(this.value);
                            }
                        }
                    },
                    tooltip: {
                        useHTML: true,
                        formatter: function () {
                            return ''
                                + '<table>'
                                + '<tr><td colspan="2"><b>' + categoryKeyToString(this.x) + '</b></td></tr>'
                                + this.points.reduce(
                                    function (rowsHtml, point) {
                                        return rowsHtml
                                            + '<tr>'
                                            + '<td><span style="color:' + point.color + '">●</span> ' + point.series.name + ': </td>'
                                            + '<td><b>' + Highcharts.numberFormat(point.y) + '</b></td>'
                                            + '<td><b>' + (deviceUnit ? '&nbsp;' + deviceUnit : '') + '</b></td>'
                                            + '<td style="text-align: center; padding-left: 3px;">' + fontAwesomeIcon(
                                                    trendToFontAwesomeIconNameAndSize(point.point.options.trend),
                                                    trendToColor(point.point.options.trend)) + '</td>'
                                            + '</tr>';
                                    }, '')
                                + '</table>';

                            function trendToFontAwesomeIconNameAndSize(trend) {
                                if (trend === 'up' || trend === 'down') {
                                    return {
                                        name: 'caret-' + trend,
                                        size: '1.3em'
                                    };
                                }
                                if (trend === 'equal') {
                                    return {
                                        name: 'equals',
                                        size: '0.9em'
                                    };
                                }
                            }

                            function trendToColor(trend) {
                                const valuation = trendValuation(trend);
                                if (valuation === 'better') {
                                    return 'rgb(125,220,78)';
                                }
                                if (valuation === 'worse') {
                                    return 'rgb(255,107,107)';
                                }
                                if (valuation === 'same') {
                                    return 'rgb(192,192,192)';
                                }
                            }

                            function trendValuation(trend) {
                                if (template.trendValuationIsReversed()) {
                                    if (trend === 'up') {
                                        return 'better';
                                    }
                                    if (trend === 'down') {
                                        return 'worse';
                                    }
                                } else {
                                    if (trend === 'up') {
                                        return 'worse';
                                    }
                                    if (trend === 'down') {
                                        return 'better';
                                    }
                                }
                                if (trend === 'equal') {
                                    return 'same';
                                }
                            }

                            function fontAwesomeIcon(nameAndSize, color) {
                                if (nameAndSize === undefined) {
                                    return '';
                                }
                                return '<i class="fas fa-' + nameAndSize.name + '" style="font-size: ' + nameAndSize.size + '; color: ' + color + ';"></i>';
                            }
                        }
                    }
                }
            };
            return template;

            function categoryKeyToString(categoryKey) {
                return ctrl.groupingBy === 'month' ? monthToString(categoryKey) : categoryKey;
            }

            function monthToString(month) {
                const months = {
                    '01': $.t('Jan'),
                    '02': $.t('Feb'),
                    '03': $.t('Mar'),
                    '04': $.t('Apr'),
                    '05': $.t('May'),
                    '06': $.t('Jun'),
                    '07': $.t('Jul'),
                    '08': $.t('Aug'),
                    '09': $.t('Sep'),
                    '10': $.t('Oct'),
                    '11': $.t('Nov'),
                    '12': $.t('Dec'),
                };
                return months[month];
            }
        }
    });
});