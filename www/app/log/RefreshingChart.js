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
        self.chartName = params.chartName;
        self.device = params.device;
        self.dataSupplier = params.dataSupplier;
        self.chartType = params.chartType;
        self.chart = self.$element.find('.chartcontainer').highcharts(self.createChartDefinition()).highcharts();

        self.isZoomLeftSticky = false;
        self.isZoomRightSticky = false;
        self.isZoomed = false;

        self.$scope.chartTitle = chartTitle();

        self.chartOnMouseDown = self.chart.container.onmousedown;
        self.chart.container.onmousedown = function (e) {
            self.consoledebug('Mousedown ' + self + ' clientX:' + e.clientX + ' ctrl:' + e.ctrlKey);
            self.wasCtrlMouseDown = e.ctrlKey;
            self.mouseDownPosition = e.clientX;
            self.chartOnMouseDown(e);
        };
        self.chart.container.onmouseup = function (e) {
            self.consoledebug('Mouseup ' + self + ' clientX:' + e.clientX + ' ctrl:' + e.ctrlKey);
            self.wasCtrlMouseUp = e.ctrlKey;
            self.mouseUpPosition = e.clientX;
        };

        self.refreshChartData();

        self.refreshTimestamp = 0;

        self.$scope.zoomed = false;

        self.$scope.zoomLabel = function (label) {
            const matcher = label.match(/^(?<count>[0-9]+)(?<letter>[HdwM])$/);
            if (matcher !== null) {
                const letter = matcher.groups.letter;
                const duration =
                    letter === 'H' ? 'Hour' :
                        letter === 'd' ? 'Day' :
                            letter === 'w' ? 'Week' :
                                letter === 'M' ? 'Month' : '';
                return matcher.groups.count + $.t(duration).substring(0, 1).toLowerCase();
            }
            return $.t(label).toLowerCase();
        }

        self.$scope.zoomHours = function (hours) {
            const xAxis = self.chart.xAxis[0];
            const reference = xAxis.max < xAxis.dataMax ? xAxis.max : xAxis.dataMax;
            zoom(reference - hours * 3600 * 1000, reference);
        }

        self.$scope.zoomDays = function (days) {
            const xAxis = self.chart.xAxis[0];
            const reference = xAxis.max < xAxis.dataMax ? xAxis.max : xAxis.dataMax;
            zoom(reference - days * 24 * 3600 * 1000, reference);
        }

        self.$scope.zoomreset = function () {
            const xAxis = self.chart.xAxis[0];
            zoom(xAxis.dataMin, xAxis.dataMax);
        }

        self.$element.find('.chartcontainer').on('dblclick', function (e) {
            const event = self.chart.pointer.normalize(e);
            const plotX = event.chartX - self.chart.plotLeft;
            const plotWidth = self.chart.plotSizeX;
            const xAxis = self.chart.xAxis[0];
            const plotRange = xAxis.max - xAxis.min;
            const plotRangeNew = plotRange * .5;
            const plotRangeCut = plotRange - plotRangeNew;
            const plotRangeCutLeft = plotRangeCut * (plotX / plotWidth);
            const plotRangeCutRight = plotRangeCut * ((plotWidth - plotX) / plotWidth);
            zoom(xAxis.min + plotRangeCutLeft, -plotRangeCutRight + xAxis.max);
        });

        function zoom(min, max) {
            const xAxis = self.chart.xAxis[0];
            xAxis.zoom(min, max);
            self.chart.redraw();
        }

        self.$scope.$on('time_update', function (event, update) {
            if (params.autoRefreshIsEnabled()) {
                const serverTime = GetLocalTimestampFromString(update.serverTime);
                const secondsIntoCurrentSlot = Math.floor(serverTime % (300 * 1000) / 1000);
                if (5 < secondsIntoCurrentSlot) {
                    const currentSlot = Math.floor(serverTime / (300 * 1000));
                    const refreshSlot = Math.floor(self.refreshTimestamp / (300 * 1000));
                    if (refreshSlot < currentSlot) {
                        self.refreshChartData();
                        self.refreshTimestamp = serverTime;
                    }
                }
            } else {
                self.refreshTimestamp = 0;
            }
        });
        // self.$scope.$on('device_update', function (event, device) {
        //     if (params.autoRefreshIsEnabled() && device.idx === self.device.idx) {
        //         self.refreshChartData();
        //     }
        // });

        function chartTitle() {
            if (self.chartName !== undefined) {
                return self.chartName + ' ' + uncapitalize(chartTitlePeriod());
            } else {
                return chartTitlePeriod();
            }

            function chartTitlePeriod() {
                return self.range === 'day' ? self.domoticzGlobals.Get5MinuteHistoryDaysGraphTitle() :
                    self.range === 'month' ? $.t('Last Month') :
                        self.range === 'year' ? $.t('Last Year') : '';
            }

            function uncapitalize(text) {
                return text.substring(0, 1).toLowerCase() + text.substring(1);
            }
        }
    }

    RefreshingChart.prototype = Object.create(DomoticzBase.prototype);
    RefreshingChart.prototype.constructor = RefreshingChart;

    RefreshingChart.prototype.createChartDefinition = function () {
        const self = this;
        return {
            chart: {
                type: self.chartType !== undefined ? self.chartType : 'spline',
                zoomType: 'x',
                marginTop: 45,
                resetZoomButton: {
                    theme: {
                        display: 'none'
                    },
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
                        const xAxis = self.chart.xAxis[0];
                        self.consoledebug(function () {
                            return 'setExtremes():\n'
                            + 'dataMin:' + new Date(xAxis.dataMin).toString() + ', e.min:' + (e.min === null ? '' : new Date(e.min).toString()) + '\n'
                            + 'dataMax:' + new Date(xAxis.dataMax).toString() + ', e.max:' + (e.max === null ? '' : new Date(e.max).toString());
                        });
                        if (e.min === null && e.max === null || e.min <= xAxis.dataMin && xAxis.dataMax <= e.max) {
                            self.isZoomLeftSticky = false;
                            self.isZoomRightSticky = false;
                            self.consoledebug('Reset zoom ' + self + ': left-sticky:' + self.isZoomLeftSticky + ', right-sticky:' + self.isZoomRightSticky);
                            self.$scope.zoomed = self.isZoomed = false;
                        } else {
                            const wasMouseDrag = self.mouseDownPosition !== self.mouseUpPosition;
                            if (wasMouseDrag) {
                                const wasMouseUpRightOfMouseDown = self.mouseDownPosition < self.mouseUpPosition;
                                self.isZoomLeftSticky = wasMouseUpRightOfMouseDown ? self.wasCtrlMouseDown : self.wasCtrlMouseUp;
                                self.isZoomRightSticky = wasMouseUpRightOfMouseDown ? self.wasCtrlMouseUp : self.wasCtrlMouseDown;
                            }
                            self.consoledebug('Set zoom ' + self + ': left-sticky:' + self.isZoomLeftSticky + ', right-sticky:' + self.isZoomRightSticky);
                            self.$scope.zoomed = self.isZoomed = true;
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
            title: false,
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
                    self.dataSupplier.seriesSuppliers.forEach(function (seriesSupplier) {
                        const chartSeries = self.chart.get(seriesSupplier.id);
                        self.consoledebug('series: \'' + seriesSupplier.id + '\'' + (chartSeries === undefined ? ' (new)' : ''));
                        const datapoints = [];
                        if (seriesSupplier.useDataItemFromPrevious === undefined || !seriesSupplier.useDataItemFromPrevious) {
                            processDataItems(data.result, datapoints, seriesSupplier, function (item) {
                                return self.dataSupplier.timestampFromDataItem(item);
                            });
                        } else {
                            if (data.resultprev !== undefined) {
                                processDataItems(data.resultprev, datapoints, seriesSupplier, function (item) {
                                    return self.dataSupplier.timestampFromDataItem(item, 1);
                                });
                            }
                        }
                        if (datapoints.length !== 0) {
                            if (chartSeries === undefined) {
                                const series = seriesSupplier.template;
                                series.id = seriesSupplier.id;
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
                function processDataItems(dataItems, datapoints, seriesSupplier, timestampFromDataItem) {
                    if (seriesSupplier.valuesFromDataItem !== undefined) {
                        dataItems.forEach(function (item) {
                            if (seriesSupplier.dataItemIsValid === undefined || seriesSupplier.dataItemIsValid(item)) {
                                const datapoint = [timestampFromDataItem(item)];
                                seriesSupplier.valuesFromDataItem.forEach(function (valueFromDataItem) {
                                    datapoint.push(parseFloat(valueFromDataItem(item)));
                                });
                                datapoints.push(datapoint);
                            }
                        });
                    }
                    if (seriesSupplier.aggregateDatapoints !== undefined) {
                        seriesSupplier.aggregateDatapoints(datapoints);
                    }
                }

                function setNewZoomEdgesAndRedraw() {
                    if (self.isZoomed) {
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
                            self.chart.xAxis[0].setExtremes(zoomEdgeLeft1, zoomEdgeRight1, false);
                        }
                    }
                    self.chart.redraw();
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
