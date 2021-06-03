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
                            series: {
                                matchExtremes: true
                            }
                        }
                    },
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
                        tooltip: {
                            shared: false,
                            crosshairs: false
                        }
                    },
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
                        }
                    },
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
            return {
                chartName: $.t('Comparing') + ' ' + $.t(deviceType === chart.deviceTypes.EnergyUsed ? 'Usage' : deviceType === chart.deviceTypes.EnergyGenerated ? 'Generated' : chart.deviceTypes.fromIndex(deviceType)),
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
                                    function (s, point) {
                                        return s
                                            + '<tr>'
                                            + '<td><span style="color:' + point.color + '">●</span> ' + point.series.name + ': </td>'
                                            + '<td><b>' + Highcharts.numberFormat(point.y) + '</b></td>'
                                            + '<td><b>' + (deviceUnit ? '&nbsp;' + deviceUnit : '') + '</b></td>'
                                            + '<td>' + (point.point.options.trend ?
                                                '<img src="/images/' + point.point.options.trend + '.png" alt="' + point.point.options.trend + '">' : '') + '</td>'
                                            + '</tr>';
                                    }, '')
                                + '</table>';
                        }
                    }
                }
            };

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