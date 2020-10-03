define(['DomoticzBase'], function (DomoticzBase) {

    function RefreshingChart(baseParams, angularParams, domoticzParams, params) {
        DomoticzBase.call(this, baseParams, angularParams, domoticzParams);
        const self = this;
        self.consoledebug('device -> '
            + 'idx:' + params.device.idx
            + ', type:' + params.device.Type
            + ', subtype:' + params.device.SubType
            + ', sensorType:' + params.sensorType);
        self.sensorType = params.sensorType;
        self.range = params.range;
        self.chartTitle = params.chartTitle;
        self.device = params.device;
        self.dataSupplier = params.dataSupplier;
        self.chart = self.$element.highcharts(self.createChartDefinition()).highcharts();

        self.chartOnMouseDown = self.chart.container.onmousedown;
        self.chart.container.onmousedown = function (e) {
            self.consoledebug('Mousedown ' + self + ' ctrl:' + e.ctrlKey);
            self.wasCtrlMouseDown = e.ctrlKey;
            self.mouseDownPosition = e.clientX;
            self.chartOnMouseDown(e);
        };
        self.chart.container.onmouseup = function (e) {
            self.consoledebug('Mouseup ' + self + ' ctrl:' + e.ctrlKey);
            self.wasCtrlMouseUp = e.ctrlKey;
            self.mouseUpPosition = e.clientX;
        };

        self.refreshChartData();

        self.refreshTimestamp = 0;
        self.$scope.$on('time_update', function (event, update) {
            if (params.autoRefreshIsEnabled()) {
                const serverTime = GetLocalTimestampFromString(update.serverTime);
                const secondsIntoCurrentSlot = Math.floor(serverTime % (300*1000) / 1000);
                if (5 < secondsIntoCurrentSlot) {
                    const currentSlot = Math.floor(serverTime / (300 * 1000));
                    const refreshSlot = Math.floor(self.refreshTimestamp / (300 * 1000));
                    if (refreshSlot < currentSlot) {
                        self.refreshChartData();
                        self.refreshTimestamp = serverTime;
                    }
                }
            }
        });
        // self.$scope.$on('device_update', function (event, device) {
        //     if (params.autoRefreshIsEnabled() && device.idx === self.device.idx) {
        //         self.refreshChartData();
        //     }
        // });
    }

    RefreshingChart.prototype = Object.create(DomoticzBase.prototype);
    RefreshingChart.prototype.constructor = RefreshingChart;

    RefreshingChart.prototype.createChartDefinition = function () {
        const self = this;
        return {
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
                    setExtremes: function (e) {
                        if (e.min === null && e.max === null) {
                            self.isZoomLeftSticky = false;
                            self.isZoomRightSticky = false;
                            self.consoledebug('Reset zoom ' + self + ': left-sticky:' + self.isZoomLeftSticky + ', right-sticky:' + self.isZoomRightSticky);
                        } else {
                            const wasMouseUpRightOfMouseDown = self.mouseDownPosition < self.mouseUpPosition;
                            self.isZoomLeftSticky = wasMouseUpRightOfMouseDown ? self.wasCtrlMouseDown : self.wasCtrlMouseUp;
                            self.isZoomRightSticky = wasMouseUpRightOfMouseDown ? self.wasCtrlMouseUp : self.wasCtrlMouseDown;
                            self.consoledebug('Set zoom ' + self + ': left-sticky:' + self.isZoomLeftSticky + ', right-sticky:' + self.isZoomRightSticky);
                        }
                    }
                }
            },
            yAxis: self.dataSupplier.yAxes,
            tooltip: {
                crosshairs: true,
                shared: true,
                valueSuffix: self.dataSupplier.valueSuffix
            },
            plotOptions: {
                series: {
                    point: {
                        events: {
                            click: function (event) {
                                if (event.shiftKey !== true) {
                                    return;
                                }
                                self.domoticzDatapointApi
                                    .deletePoint(
                                        self.device.idx,
                                        event.point,
                                        self.dataSupplier.isShortLogChart,
                                        new Date().getTimezoneOffset())
                                    .then(function () {
                                        self.$route.reload();
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
                },
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
                },
                areasplinerange: {
                    marker: {
                        enabled: false
                    }
                }
            },
            title: {
                text: self.chartTitle
            },
            series: [],
            legend: {
                enabled: true
            },
            time: {
                timezoneOffset: new Date().getTimezoneOffset()
            }
        };
    }

    RefreshingChart.prototype.createDataRequest = function () {
        return {
            type: 'graph',
            sensor: this.sensorType,
            range: this.range,
            idx: this.device.idx
        };
    }

    RefreshingChart.prototype.refreshChartData = function () {
        const self = this;
        self.domoticzApi
            .sendRequest(self.createDataRequest())
            .then(function (data) {
                self.consoledebug('[' + new Date().toString() + '] refreshing ' + self);

                if (typeof data.result === 'undefined') {
                    return;
                }

                const dataEdgeLeft = getDataEdgeLeft(data);
                const dataEdgeRight = getDataEdgeRight(data);
                const chartEdgeLeft = getChartEdgeLeft();
                const chartEdgeRight = getChartEdgeRight();
                const zoomEdgeLeft = getZoomEdgeLeft();
                const zoomEdgeRight = getZoomEdgeRight();

                self.consoledebug(
                    'dataEdgeLeft:' + new Date(dataEdgeLeft).toString()
                    + ', dataEdgeRight:' + new Date(dataEdgeRight).toString());
                if (self.chart.series.length !== 0) {
                    self.consoledebug(
                        'chartEdgeLeft:' + new Date(chartEdgeLeft).toString() + (chartEdgeLeft !== dataEdgeLeft ? '!' : '=')
                        + ', chartEdgeRight:' + new Date(chartEdgeRight).toString() + (chartEdgeRight !== dataEdgeRight ? '!' : '='));

                    self.consoledebug(
                        'zoomEdgeLeft:' + new Date(zoomEdgeLeft).toString()
                        + ', zoomEdgeRight:' + new Date(zoomEdgeRight).toString());
                }

                loadDataInChart(data);
                setNewZoomEdgesAndRedraw();

                function loadDataInChart(data) {
                    self.dataSupplier.seriesSuppliers.forEach(function(seriesSupplier) {
                        const chartSeries = self.chart.get(seriesSupplier.id);
                        self.consoledebug('series: \'' + seriesSupplier.id + '\'' + (chartSeries === undefined ? ' (new)' : ''));
                        const datapoints = [];
                        if (seriesSupplier.previous === undefined || !seriesSupplier.previous) {
                            data.result.forEach(function (item) {
                                if (seriesSupplier.dataItemIsValid(item)) {
                                    const datapoint = [self.dataSupplier.timestampFromDataItem(item)];
                                    seriesSupplier.valuesFromDataItem.forEach(function(valueFromDataItem) {
                                        datapoint.push(parseFloat(valueFromDataItem(item)));
                                    });
                                    datapoints.push(datapoint);
                                }
                            });
                        } else {
                            if (data.resultprev !== undefined) {
                                data.resultprev.forEach(function (item) {
                                    if (seriesSupplier.dataItemIsValid(item)) {
                                        const datapoint = [self.dataSupplier.timestampFromDataItem(item, 1)];
                                        seriesSupplier.valuesFromDataItem.forEach(function (valueFromDataItem) {
                                            datapoint.push(parseFloat(valueFromDataItem(item)));
                                        });
                                        datapoints.push(datapoint);
                                    }
                                });
                            }
                        }
                        if (datapoints.length !== 0) {
                            if (chartSeries === undefined) {
                                const series = seriesSupplier.template;
                                series.id = seriesSupplier.id;
                                if (seriesSupplier.name !== undefined) {
                                    series.name = $.t(seriesSupplier.name);
                                }
                                if (series.colorIndex !== undefined) {
                                    series.color = Highcharts.getOptions().colors[series.colorIndex];
                                }
                                series.data = datapoints;
                                self.chart.addSeries(series, false);
                            } else {
                                chartSeries.setData(datapoints, false);
                            }
                        } else {
                            if (chartSeries !== undefined) {
                                chartSeries.setData(datapoints, false);
                            }
                        }
                    });
                }

                function setNewZoomEdgesAndRedraw() {
                    const zoomEdgeRight1 = self.isZoomRightSticky || zoomEdgeRight === -1 || chartEdgeRight === -1
                        ? zoomEdgeRight
                        : dataEdgeRight + (zoomEdgeRight - chartEdgeRight);
                    const zoomEdgeLeft1 = self.isZoomLeftSticky || zoomEdgeLeft === -1 || chartEdgeLeft === -1
                        ? zoomEdgeLeft
                        : dataEdgeLeft + (zoomEdgeLeft - chartEdgeLeft);
                    if (zoomEdgeLeft1 !== zoomEdgeLeft || zoomEdgeRight1 !== zoomEdgeRight) {
                        self.consoledebug(
                            'zoomEdgeLeft1:' + zoomEdgeLeft1 + ' (' + new Date(zoomEdgeLeft1).toString() + ')'
                            + ', zoomEdgeRight1:' + zoomEdgeRight1 + ' (' + new Date(zoomEdgeRight1).toString() + ')');
                        self.chart.xAxis[0].setExtremes(zoomEdgeLeft1, zoomEdgeRight1, true);
                    } else {
                        self.chart.redraw();
                    }
                }

                function getDataEdgeLeft(data) {
                    return self.dataSupplier.timestampFromDataItem(data.result[0]);
                }

                function getDataEdgeRight(data) {
                    return self.dataSupplier.timestampFromDataItem(data.result[data.result.length - 1]);
                }

                function getChartEdgeLeft() {
                    if (self.chart.series.length === 0) {
                        return -1;
                    }
                    return self.chart.series[0].xData[0];
                }

                function getChartEdgeRight() {
                    if (self.chart.series.length === 0) {
                        return -1;
                    }
                    return self.chart.series[0].xData[self.chart.series[0].xData.length - 1];
                }

                function getZoomEdgeLeft() {
                    if (self.chart.series.length === 0) {
                        return -1;
                    }
                    return self.chart.xAxis[0].min;
                }

                function getZoomEdgeRight() {
                    if (self.chart.series.length === 0) {
                        return -1;
                    }
                    return self.chart.xAxis[0].max;
                }
            });
    }

    return RefreshingChart;
});
