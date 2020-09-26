define(['DomoticzBase'], function (DomoticzBase) {

    RefreshingChart.prototype.getChartUnit = null;
    RefreshingChart.prototype.dataPointIsShort = null;
    RefreshingChart.prototype.loadData = null;
    RefreshingChart.prototype.getDataEdgeLeft = null;
    RefreshingChart.prototype.getDataEdgeRight = null;

    function RefreshingChart(baseParams, angularParams, domoticzParams, params) {
        DomoticzBase.call(this, baseParams, angularParams, domoticzParams);
        const self = this;
        self.consoledebug('device:' + params.device.idx);
        self.range = params.range;
        self.chartTitle = params.chartTitle;
        self.device = params.device;
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

        self.$scope.$on('device_update', function (event, device) {
            if (params.autoRefreshIsEnabled() && device.idx === self.device.idx) {
                self.refreshChartData();
            }
        });
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
            yAxis: {
                title: {
                    text: self.getAxisTitle()
                },
                labels: {
                    formatter: function () {
                        const value = self.getChartUnit() === 'vM' ? Highcharts.numberFormat(this.value, 0) : this.value;
                        return value + ' ' + self.getChartUnit();
                    }
                }
            },
            tooltip: {
                crosshairs: true,
                shared: true,
                valueSuffix: ' ' + self.getChartUnit()
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
                                    .deletePoint(self.device.idx, event.point, self.dataPointIsShort())
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
            sensor: this.getChartType(),
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

                let dataEdgeLeft = -1;
                let dataEdgeRight = -1
                let chartEdgeLeft = -1;
                let chartEdgeRight = -1;
                let zoomLeft = -1;
                let zoomRight = -1;
                let zoomLeftRelativeToEdge = null;
                let zoomRightRelativeToEdge = null;

                determineDataEdges();
                determineCurrentZoomEdges();
                self.loadData(data);
                setNewZoomEdgesAndRedraw();

                function determineDataEdges() {
                    dataEdgeLeft = self.getDataEdgeLeft(data);
                    dataEdgeRight = self.getDataEdgeRight(data);
                    self.consoledebug('dataEdgeLeft:' + new Date(dataEdgeLeft).toString() + ', dataEdgeRight:' + new Date(dataEdgeRight).toString());
                }

                function determineCurrentZoomEdges() {
                    if (self.chart.series.length !== 0) {
                        chartEdgeLeft = self.chart.series[0].xData[0];
                        chartEdgeRight = self.chart.series[0].xData[self.chart.series[0].xData.length - 1];
                        self.consoledebug(
                            'chartEdgeLeft:' + new Date(chartEdgeLeft).toString() + (chartEdgeLeft !== dataEdgeLeft ? '!' : '=')
                            + ', chartEdgeRight:' + new Date(chartEdgeRight).toString() + (chartEdgeRight !== dataEdgeRight ? '!' : '='));

                        zoomLeft = self.chart.xAxis[0].min;
                        zoomRight = self.chart.xAxis[0].max;
                        self.consoledebug(
                            'zoomLeft:' + new Date(zoomLeft).toString()
                            + ', zoomRight:' + new Date(zoomRight).toString());
                        zoomLeftRelativeToEdge = zoomLeft - chartEdgeLeft;
                        zoomRightRelativeToEdge = zoomRight - chartEdgeRight;
                        self.consoledebug(
                            'zoomLeftRelativeToEdge:' + zoomLeftRelativeToEdge
                            + ', zoomRightRelativeToEdge:' + zoomRightRelativeToEdge);
                    }
                }

                function setNewZoomEdgesAndRedraw() {
                    const zoomRight1 = self.isZoomRightSticky || zoomRightRelativeToEdge === null
                        ? zoomRight
                        : dataEdgeRight + zoomRightRelativeToEdge;
                    const zoomLeft1 = self.isZoomLeftSticky || zoomLeftRelativeToEdge === null
                        ? zoomLeft
                        : dataEdgeLeft + zoomLeftRelativeToEdge;
                    if (zoomLeft1 !== zoomLeft || zoomRight1 !== zoomRight) {
                        self.consoledebug(
                            'zoomLeft1:' + zoomLeft1 + ' (' + new Date(zoomLeft1).toString() + ')'
                            + ', zoomRight1:' + zoomRight1 + ' (' + new Date(zoomRight1).toString() + ')');
                        self.chart.xAxis[0].setExtremes(zoomLeft1, zoomRight1, true);
                    } else {
                        self.chart.redraw();
                    }
                }
            });
    }
    RefreshingChart.prototype.getPointValueKey = function () {
        if (this.device.SubType === 'Lux') {
            return 'lux'
        } else if (this.device.Type === 'Usage') {
            return 'u'
        } else {
            return 'v';
        }
    }
    RefreshingChart.prototype.getChartType = function () {
        if (['Custom Sensor', 'Waterflow', 'Percentage'].includes(this.device.SubType)) {
            return 'Percentage';
        } else {
            return 'counter';
        }
    }
    RefreshingChart.prototype.getAxisTitle = function () {
        const sensor = this.getSensorName();
        const unit = this.getChartUnit();
        return this.device.SubType === 'Custom Sensor' ? unit : sensor + ' (' + unit + ')';
    }
    RefreshingChart.prototype.getSensorName = function () {
        if (this.device.Type === 'Usage') {
            return this.$.t('Usage');
        } else {
            return this.$.t(this.device.SubType)
        }
    }
    RefreshingChart.prototype.getChartUnit = function () {
        return this.device.getUnit();
    }

    return RefreshingChart;
});