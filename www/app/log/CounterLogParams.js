define(['app', 'log/Chart'], function (app) {

    app.factory('counterLogParams', function (chart) {
        return {
            chartParamsDay: chartParamsDay,
            chartParamsWeek: chartParamsWeek,
            chartParamsMonthYear: chartParamsMonthYear
        }

        function seriesYaxisSubtype(subtype) {
            if (subtype === 'p1') {
                return {
                    min: 0
                };
            } else {
                return {};
            }
        }

        function chartParamsDay(domoticzGlobals, ctrl, chartParamsTemplate, dataSupplierTemplate, seriesSuppliers) {
            return _.merge(
                {
                    highchartTemplate: {
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
                                yAxes:
                                    [
                                        _.merge(
                                            {
                                                title: {
                                                    text: $.t('Energy') + ' (Wh)'
                                                }
                                            },
                                            seriesYaxisSubtype()
                                        ),
                                        _.merge(
                                            {
                                                title: {
                                                    text: $.t('Power') + ' (Watt)'
                                                },
                                                opposite: true
                                            },
                                            seriesYaxisSubtype()
                                        )
                                    ],
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
                                yAxes:
                                    [
                                        _.merge(
                                            {
                                                maxPadding: 0.2,
                                                title: {
                                                    text: $.t('Energy') + ' (kWh)'
                                                }
                                            },
                                            seriesYaxisSubtype()
                                        )
                                    ],
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
                                yAxes:
                                    [
                                        _.merge(
                                            {
                                                title: {
                                                    text: $.t('Energy') + ' (kWh)'
                                                }
                                            },
                                            seriesYaxisSubtype()
                                        )
                                    ],
                                seriesSuppliers: seriesSuppliers
                            },
                            dataSupplierTemplate
                        )
                },
                chartParamsTemplate
            );
        }
    });
});