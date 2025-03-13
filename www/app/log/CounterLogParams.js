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
            chartParamsHour: chartParamsHour,
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
                            },
							events: {
								afterSetExtremes: function (event) {
									var xMin = event.min;
									var xMax = event.max;
/*	
									var chart = Highcharts.charts[4]; //need_some_help: this is not always the hour chart!?
									var ex = chart.xAxis[0].getExtremes();
									if (ex.min != xMin || ex.max != xMax) {
										chart.xAxis[0].setExtremes(xMin, xMax, true, false);
									}
*/
								}
							}
                        },
                        plotOptions: {
							column: {
								pointPlacement: 'between'
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

        function chartParamsHour(domoticzGlobals, ctrl, chartParamsTemplate, dataSupplierTemplate, seriesSuppliers) {
            return _.merge(
                {
                    highchartTemplate: {
                        xAxis: {
                            dateTimeLabelFormats: {
                                hour: '%H:00',
                                day: '%H:00'
                            },
							events: {
								afterSetExtremes: function (event) {
									var xMin = event.min;
									var xMax = event.max;
/*									
									var chart = Highcharts.charts[0]; //need_some_help: this is not always the day chart!?
									var ex = chart.xAxis[0].getExtremes();
									if (ex.min != xMin || ex.max != xMax) {
										chart.xAxis[0].setExtremes(xMin, xMax, true, false);
									}
*/
								}
							},
                            tickInterval: 1 * 3600 * 1000
                        },
                        tooltip: {
                            crosshairs: false
                        },
						plotOptions: {
							column: {
								grouping: false,
								shadow: false,
								borderWidth: 0
							}
						}
                    },
                    ctrl: ctrl,
                    range: ctrl.range,
                    device: ctrl.device,
                    sensorType: 'counter',
                    chartName: $.t('Usage') + ' / ' + $.t('Hour'),
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
                chartName: ($.t('Comparing') + ' ' + $.t(deviceType === chart.deviceTypes.EnergyUsed ? 'Usage' : deviceType === chart.deviceTypes.EnergyGenerated ? 'Generated' : '')).trim(),
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
                                + '<tr><td colspan="2"><b>' + categoryKeyToStringEx(this.x) + '</b></td></tr>'
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
				if (ctrl.groupingBy === 'month') {
					return monthToString(parseInt(categoryKey));
				}
				return categoryKey;
			}

			function categoryKeyToStringEx(categoryKey) {
				if (ctrl.groupingBy === 'month') {
					return monthToString(parseInt(categoryKey)+1);
				} else if (ctrl.groupingBy === 'quarter') {
					return 'Q' + (parseInt(categoryKey)+1).toString();
				} else if (ctrl.groupingBy === 'year') {
					return parseInt(ctrl.chart.seriesSuppliers[0].id) + parseInt(categoryKey);
				} else {
					return categoryKey;
				}
			}

			function monthToString(month) {
				const months = {
					1: $.t('Jan'),
					2: $.t('Feb'),
					3: $.t('Mar'),
					4: $.t('Apr'),
					5: $.t('May'),
					6: $.t('Jun'),
					7: $.t('Jul'),
					8: $.t('Aug'),
					9: $.t('Sep'),
					10: $.t('Oct'),
					11: $.t('Nov'),
					12: $.t('Dec'),
				};
				return months[month];
			}
        }
    });
});
