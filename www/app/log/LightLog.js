define(['app', 'log/components'], function (app) {
    app.constant('deviceLightLogsHighchartSettings', {
        chart: {
            type: 'line',
            zoomType: 'x',
            resetZoomButton: {
                position: {
                    x: -30,
                    y: 10
                }
            },
            marginRight: 10
        },
        credits: {
            enabled: true,
            href: "http://www.domoticz.com",
            text: "Domoticz.com"
        },
        title: {
            text: null
        },
        xAxis: {
            type: 'datetime'
        },
        yAxis: {
            max: 100,
            title: {
                text: $.t('Percentage') + ' (%)'
            }
        },
        tooltip: {
            formatter: function () {
                return '' +
                    $.t(Highcharts.dateFormat('%A', this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M:%S', this.x) + ', ' + this.y + ' %';
            }
        },
        plotOptions: {
            line: {
                lineWidth: 3,
                states: {
                    hover: {
                        lineWidth: 3
                    }
                },
                marker: {
                    enabled: false,
                    states: {
                        hover: {
                            enabled: true,
                            symbol: 'circle',
                            radius: 5,
                            lineWidth: 1
                        }
                    }
                }
            }
        },
        series: [{
            id: 'percent',
            showInLegend: false,
            name: 'percent',
            step: 'left'
        }]
        ,
        navigation: {
            menuItemStyle: {
                fontSize: '10px'
            }
        }
    });

    app.controller('DeviceLightLogController', function ($routeParams, deviceLightLogsHighchartSettings, dataTableDefaultSettings, domoticzApi, deviceApi, permissions) {
        var vm = this;
        var $element = $('.js-device-logs:last');
        var logsChart;

        vm.clearLog = clearLog;

        init();

        function init() {
            vm.deviceIdx = $routeParams.id;

            deviceApi.getDeviceInfo(vm.deviceIdx).then(function (device) {
                vm.deviceName = device.Name;
            });

            logsChart = $element.find('#lightgraph').highcharts(deviceLightLogsHighchartSettings);
            refreshLog();
        }

        function refreshLog() {
            logsChart.highcharts().series[0].setData([]);

            domoticzApi
                .sendRequest({
                    type: 'lightlog',
                    idx: vm.deviceIdx
                })
                .then(function (data) {
                    if (typeof data.result === 'undefined') {
                        return;
                    }

                    vm.log = data.result || [];
                    var chartData = [];

                    data.result.forEach(function (item) {
                        var level = -1;

                        if (['Off', 'Disarm', 'No Motion', 'Normal'].includes(item.Status)) {
                            level = 0;
                        } else if (data.HaveSelector === true) {
                            level = parseInt(item.Level);
                        } else if (item.Status.indexOf('Set Level:') === 0) {
                            var lstr = item.Status.substr(11);
                            var idx = lstr.indexOf('%');

                            if (idx !== -1) {
                                lstr = lstr.substr(0, idx - 1);
                                level = parseInt(lstr);
                            }
                        } else {
                            var idx = item.Status.indexOf('Level: ');

                            if (idx !== -1) {
                                var lstr = item.Status.substr(idx + 7);
                                var idx = lstr.indexOf('%');
                                if (idx !== -1) {
                                    lstr = lstr.substr(0, idx - 1);
                                    level = parseInt(lstr);
                                    if (level > 100) {
                                        level = 100;
                                    }
                                }
                            } else if (item.Level > 0 && item.Level <= 100) {
                                level = item.Level;
                            } else {
                                level = 100;
                            }
                        }

                        if (level !== -1) {
                            chartData.unshift([GetUTCFromStringSec(item.Date), level]);
                        }
                    });

                    logsChart.highcharts().series[0].setData(chartData);
                });
        }

        function clearLog() {
            if (!permissions.hasPermission('Admin')) {
                var message = $.t('You do not have permission to do that!');
                HideNotify();
                ShowNotify(message, 2500, true);
                return;
            }

            bootbox.confirm($.t("Are you sure to delete the Log?\n\nThis action can not be undone!"), function (result) {
                if (result !== true) {
                    return;
                }

                domoticzApi
                    .sendCommand('clearlightlog', {
                        idx: vm.deviceIdx
                    })
                    .then(refreshLog)
                    .catch(function () {
                        HideNotify();
                        ShowNotify($.t('Problem clearing the Log!'), 2500, true);
                    });
            });
        }
    });
});