define(['app', 'log/factories'], function (app) {
    app.component('deviceGraphLog', {
        bindings: {
            device: '<'
        },
        templateUrl: 'app/log/GraphLog.html',
        controllerAs: '$ctrl',
        controller: function () {
            const $ctrl = this;
            $ctrl.autoRefresh = true;
        }
    });

    app.component('deviceLogChart', {
        require: {
            logCtrl: '^deviceGraphLog'
        },
        bindings: {
            device: '<',
            deviceType: '<',
            degreeType: '<',
            range: '@'
        },
        template: '<div></div>',
        controllerAs: 'vm',
        controller: function ($scope, $element, $route, $location, domoticzApi, domoticzDataPointApi, domoticzGlobals) {
            const debug = 'true' === $location.search().consoledebug;

            function consoledebug(s) {
                if (debug && window.console) {
                    console.log(s);
                }
            }

            const vm = this;
            let chart;

            vm.$onInit = init;
            vm.$onChanges = onChanges;

            vm.wasCtrlMouseDown = false;
            vm.wasCtrlMouseUp = false;
            vm.mouseDownPosition;
            vm.mouseUpPosition;
            vm.isZoomLeftSticky = false;
            vm.isZoomRightSticky = false;

            function init() {
                const sensor = getSensorName();
                const unit = getChartUnit();
                const axisTitle = vm.device.SubType === 'Custom Sensor' ? unit : sensor + ' (' + unit + ')';

                chart = $element.highcharts({
                    chart: {
                        type: 'spline',
                        zoomType: 'x',
                        resetZoomButton: {
                            position: {
                                x: -30,
                                y: -36
                            }
                        },
                        panning: true,
                        panKey: 'shift'
                    },
                    xAxis: {
                        type: 'datetime',
                        events: {
                            setExtremes: function(e) {
                                if (e.min === null && e.max === null) {
                                    vm.isZoomLeftSticky = false;
                                    vm.isZoomRightSticky = false;
                                    consoledebug('Reset zoom: left-sticky:' + vm.isZoomLeftSticky + ', right-sticky:' + vm.isZoomRightSticky);
                                } else {
                                    const wasMouseUpRightOfMouseDown = vm.mouseDownPosition < vm.mouseUpPosition;
                                    vm.isZoomLeftSticky = wasMouseUpRightOfMouseDown ? vm.wasCtrlMouseDown : vm.wasCtrlMouseUp;
                                    vm.isZoomRightSticky = wasMouseUpRightOfMouseDown ? vm.wasCtrlMouseUp : vm.wasCtrlMouseDown;
                                    consoledebug('Set zoom: left-sticky:' + vm.isZoomLeftSticky + ', right-sticky:' + vm.isZoomRightSticky);
                                }
                            }
                        }
                    },
                    yAxis: {
                        title: {
                            text: axisTitle
                        },
                        labels: {
                            formatter: function () {
                                const value = unit === 'vM' ? Highcharts.numberFormat(this.value, 0) : this.value;
                                return value + ' ' + unit;
                            }
                        }
                    },
                    tooltip: {
                        crosshairs: true,
                        shared: true,
                        valueSuffix: ' ' + unit
                    },
                    plotOptions: {
                        series: {
                            point: {
                                events: {
                                    click: function (event) {
                                        if (event.shiftKey !== true) {
                                            return;
                                        }

                                        domoticzDataPointApi
                                            .deletePoint(vm.device.idx, event.point, vm.range === 'day')
                                            .then(function () {
                                                $route.reload();
                                            });
                                    }
                                }
                            }
                        },
                        spline: {
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
                    title: {
                        text: getChartTitle()
                    },
                    series: [],
                    legend: {
                        enabled: true
                    },
                    time: {
                        timezoneOffset: new Date().getTimezoneOffset()
                    }
                }).highcharts();
                
                vm.chartOnMouseDown = chart.container.onmousedown;
                chart.container.onmousedown = function (e) {
                    consoledebug('Mousedown ' + vm.range + ' ctrl:' + e.ctrlKey);
                    vm.wasCtrlMouseDown = e.ctrlKey;
                    vm.mouseDownPosition = e.clientX;
                    vm.chartOnMouseDown(e);
                };
                chart.container.onmouseup = function (e) {
                    consoledebug('Mouseup ' + vm.range + ' ctrl:' + e.ctrlKey);
                    vm.wasCtrlMouseUp = e.ctrlKey;
                    vm.mouseUpPosition = e.clientX;
                };

                refreshChartData();

                $scope.$on('device_update', function(event, device) {
                    if (vm.logCtrl.autoRefresh && device.idx === vm.device.idx) {
                        refreshChartData();
                    }
                });

            }

            function onChanges(changesObj) {
                if (!chart) {
                    return;
                }

                if (changesObj.deviceIdx || changesObj.range) {
                    refreshChartData();
                }

                if (changesObj.deviceType) {
                    chart.setTitle({text: getChartTitle()});
                }
            }

            function refreshChartData() {
                domoticzApi
                    .sendRequest({
                        type: 'graph',
                        sensor: getChartType(),
                        range: vm.range,
                        idx: vm.device.idx
                    })
                    .then(function (data) {
                        consoledebug('['+new Date().toString()+'] refreshing '+vm.range);

                        if (typeof data.result === 'undefined') {
                            return;
                        }

                        const valueKey = getPointValueKey();

                        let dataEdgeLeft = -1;
                        let dataEdgeRight = -1
                        let chartEdgeLeft = -1;
                        let chartEdgeRight = -1;
                        let zoomLeft = -1;
                        let zoomRight = -1;
                        let zoomLeftRelativeToEdge = null;
                        let zoomRightRelativeToEdge = null;
                        let zoomSize = null;

                        determineDataEdges();
                        determineCurrentZoomEdges();
                        loadDayData();
                        loadMonthAndYearData();
                        setNewZoomEdgesAndRedraw();

                        function determineDataEdges() {
                            dataEdgeLeft = vm.range === 'day'
                                ? GetLocalDateTimeFromString(data.result[0].d)
                                : GetLocalDateFromString(data.result[0].d);
                            dataEdgeRight = vm.range === 'day'
                                ? GetLocalDateTimeFromString(data.result[data.result.length - 1].d)
                                : GetLocalDateFromString(data.result[data.result.length - 1].d);
                            consoledebug('dataEdgeLeft:' + new Date(dataEdgeLeft).toString() + ', dataEdgeRight:' + new Date(dataEdgeRight).toString());
                        }

                        function determineCurrentZoomEdges() {
                            if (chart.series.length !== 0) {
                                chartEdgeLeft = chart.series[0].xData[0];
                                chartEdgeRight = chart.series[0].xData[chart.series[0].xData.length - 1];
                                consoledebug(
                                    'chartEdgeLeft:' + new Date(chartEdgeLeft).toString() + (chartEdgeLeft !== dataEdgeLeft ? '!' : '=')
                                    + ', chartEdgeRight:' + new Date(chartEdgeRight).toString() + (chartEdgeRight !== dataEdgeRight ? '!' : '='));

                                zoomLeft = chart.xAxis[0].min;
                                zoomRight = chart.xAxis[0].max;
                                consoledebug(
                                    'zoomLeft:' + new Date(zoomLeft).toString()
                                    + ', zoomRight:' + new Date(zoomRight).toString());
                                zoomLeftRelativeToEdge = zoomLeft - chartEdgeLeft;
                                zoomRightRelativeToEdge = zoomRight - chartEdgeRight;
                                zoomSize = zoomRight - zoomLeft;
                                consoledebug(
                                    'zoomLeftRelativeToEdge:' + zoomLeftRelativeToEdge
                                    + ', zoomRightRelativeToEdge:' + zoomRightRelativeToEdge
                                    + ', zoomSize:' + zoomSize);
                            }
                        }

                        function loadDayData() {
                            if (vm.range === 'day') {
                                const series = data.result.map(function (item) {
                                    return [GetLocalDateTimeFromString(item.d), parseFloat(item[valueKey])]
                                });
                                if (chart.series.length === 0) {
                                    chart.addSeries(
                                        {
                                            showInLegend: false,
                                            name: getSensorName(),
                                            color: Highcharts.getOptions().colors[0],
                                            data: series
                                        },
                                        false);
                                } else {
                                    chart.series[0].setData(series, false);
                                }
                            }
                        }

                        function loadMonthAndYearData() {
                            if (vm.range === 'month' || vm.range === 'year') {
                                const series = {min: [], max: [], avg: []};
                                data.result.forEach(function (item) {
                                    series.min.push([GetLocalDateFromString(item.d), parseFloat(item[valueKey + '_min'])]);
                                    series.max.push([GetLocalDateFromString(item.d), parseFloat(item[valueKey + '_max'])]);
                                    if (item[valueKey + '_avg'] !== undefined) {
                                        series.avg.push([GetLocalDateFromString(item.d), parseFloat(item[valueKey + '_avg'])]);
                                    }
                                });
                                if (chart.series.length === 0) {
                                    chart.addSeries(
                                        {
                                            name: 'min',
                                            color: Highcharts.getOptions().colors[3],
                                            data: series.min
                                        }
                                    );
                                    chart.addSeries(
                                        {
                                            name: 'max',
                                            color: Highcharts.getOptions().colors[2],
                                            data: series.max
                                        }
                                    );
                                    if (series.avg.length !== 0) {
                                        chart.addSeries(
                                            {
                                                name: 'avg',
                                                color: Highcharts.getOptions().colors[0],
                                                data: series.avg
                                            }
                                        );
                                    }
                                } else {
                                    chart.series[0].setData(series.min, false);
                                    chart.series[1].setData(series.max, false);
                                    if (series.avg.length !== 0) {
                                        if (chart.series.length !== 3) {
                                            chart.addSeries(
                                                {
                                                    name: 'avg',
                                                    color: Highcharts.getOptions().colors[0],
                                                    data: series.avg
                                                }
                                            );
                                        } else {
                                            chart.series[2].setData(series.avg, false);
                                        }
                                    } else {
                                        if (chart.series.length === 3) {
                                            chart.series[2].remove();
                                        }
                                    }
                                }
                            }
                        }

                        function setNewZoomEdgesAndRedraw() {
                            const zoomRight1 = vm.isZoomRightSticky || zoomRightRelativeToEdge === null
                                ? zoomRight
                                : dataEdgeRight + zoomRightRelativeToEdge;
                            const zoomLeft1 = vm.isZoomLeftSticky || zoomLeftRelativeToEdge === null
                                ? zoomLeft
                                : zoomLeftRelativeToEdge <= 0
                                    ? dataEdgeLeft + zoomLeftRelativeToEdge
                                    : zoomRight1 - zoomSize;
                            if (zoomLeft1 !== zoomLeft || zoomRight1 !== zoomRight) {
                                consoledebug(
                                    'zoomLeft1:' + zoomLeft1 + ' (' + new Date(zoomLeft1).toString() + ')'
                                    + ', zoomRight1:' + zoomRight1 + ' (' + new Date(zoomRight1).toString() + ')');
                                chart.xAxis[0].setExtremes(zoomLeft1, zoomRight1, true);
                            } else {
                                chart.redraw();
                            }
                        }
                    });
            }

            function getPointValueKey() {
                if (vm.device.SubType === 'Lux') {
                    return 'lux'
                } else if (vm.device.Type === 'Usage') {
                    return 'u'
                } else {
                    return 'v';
                }
            }

            function getChartTitle() {
                if (vm.range === 'day') {
                    return domoticzGlobals.Get5MinuteHistoryDaysGraphTitle();
                } else if (vm.range === 'month') {
                    return $.t('Last Month');
                } else if (vm.range === 'year') {
                    return $.t('Last Year');
                }
            }

            function getChartUnit() {
                return vm.device.getUnit();
            }

            function getChartType() {
                if (['Custom Sensor', 'Waterflow', 'Percentage'].includes(vm.device.SubType)) {
                    return 'Percentage';
                } else {
                    return 'counter';
                }
            }

            function getSensorName() {
                if (vm.device.Type === 'Usage') {
                    return $.t('Usage');
                } else {
                    return $.t(vm.device.SubType)
                }
            }
        }
    });
});