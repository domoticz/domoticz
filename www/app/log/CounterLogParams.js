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
            chartParamsMonthYear: chartParamsMonthYear
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
    });
});