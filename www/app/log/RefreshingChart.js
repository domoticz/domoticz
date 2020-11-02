define(['lodash', 'DomoticzBase', 'DataLoader', 'ChartLoader', 'ChartZoomer'],
    function (_, DomoticzBase, DataLoader, ChartLoader, ChartZoomer) {

    function RefreshingChart(
            baseParams, angularParams, domoticzParams, params,
            dataLoader = new DataLoader(),
            chartLoader = new ChartLoader(),
            chartZoomer = new ChartZoomer()) {
        DomoticzBase.call(this, baseParams, angularParams, domoticzParams);
        const self = this;
        self.consoledebug('device -> '
            + 'idx:' + params.device.idx
            + ', name:' + params.device.Name
            + ', type:' + params.device.Type
            + ', subtype:' + params.device.SubType
            + ', sensorType:' + params.sensorType);
        self.sensorType = params.sensorType;
        self.range = params.range;
        self.chartName = params.chartName;
        self.device = params.device;
        self.dataSupplier = params.dataSupplier;
        self.extendDataRequest = params.dataSupplier.extendDataRequest || function (dataRequest) { return dataRequest; };
        self.seriesSuppliers = completeSeriesSuppliers(params.dataSupplier);
        self.synchronizeYaxes = params.synchronizeYaxes;
        self.chart = self.$element.find('.chartcontainer').highcharts(createChartDefinition(params.highchartTemplate)).highcharts();
        self.autoRefreshIsEnabled = params.autoRefreshIsEnabled;
        self.refreshTimestamp = 0;

        self.isZoomLeftSticky = false;
        self.isZoomRightSticky = false;
        self.isZoomed = false;

        self.$scope.chartTitle = chartTitle();

        refreshChartData(initialZoom);
        self.refreshTimestamp = new Date().getTime();
        configureZoomingAndPanning();

        self.$scope.$on('$routeChangeStart', function($event, next, current) {
            self.chart.tooltip.hide();
        });

        if (true) {
            refreshChartDataEveryTimeUpdate();
        } else {
            refreshChartDataEveryDeviceUpdate();
        }

        function refreshChartDataEveryTimeUpdate() {
            self.$scope.$on('time_update', function (event, update) {
                if (self.autoRefreshIsEnabled()) {
                    const serverTime = GetLocalTimestampFromString(update.serverTime);
                    const secondsIntoCurrentSlot = Math.floor(serverTime % (300 * 1000) / 1000);
                    if (3 < secondsIntoCurrentSlot) {
                        const currentSlot = Math.floor(serverTime / (300 * 1000));
                        const refreshSlot = Math.floor(self.refreshTimestamp / (300 * 1000));
                        if (refreshSlot < currentSlot) {
                            refreshChartData();
                            self.refreshTimestamp = serverTime;
                        }
                    }
                } else {
                    self.refreshTimestamp = 0;
                }
            });
        }

        function refreshChartDataEveryDeviceUpdate() {
            self.$scope.$on('device_update', function (event, device) {
                if (params.autoRefreshIsEnabled() && device.idx === self.device.idx) {
                    refreshChartData();
                }
            });
        }

        function createChartDefinition(template) {
            return _.merge({
                        chart: {
                            type: 'spline',
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
                                        return 'xAxis.setExtremes():\n'
                                            + '    dataMin:' + dateToString(xAxis.dataMin) + ', e.min:' + dateToString(e.min) + '\n'
                                            + '    dataMax:' + dateToString(xAxis.dataMax) + ', e.max:' + dateToString(e.max);
                                    });
                                    if (e.min === null && e.max === null || e.min <= xAxis.dataMin && xAxis.dataMax <= e.max) {
                                        self.isZoomLeftSticky = false;
                                        self.isZoomRightSticky = false;
                                        self.isZoomed = false;
                                        self.consoledebug('Reset zoom ' + self + ': left-sticky:' + self.isZoomLeftSticky + ', right-sticky:' + self.isZoomRightSticky);
                                    } else {
                                        const wasMouseDrag = self.mouseDownPosition !== self.mouseUpPosition;
                                        if (wasMouseDrag) {
                                            const wasMouseUpRightOfMouseDown = self.mouseDownPosition < self.mouseUpPosition;
                                            self.isZoomLeftSticky = wasMouseUpRightOfMouseDown ? self.wasCtrlMouseDown : self.wasCtrlMouseUp;
                                            self.isZoomRightSticky = wasMouseUpRightOfMouseDown ? self.wasCtrlMouseUp : self.wasCtrlMouseDown;
                                        }
                                        self.isZoomed = true;
                                        self.consoledebug('Set zoom ' + self + ': left-sticky:' + self.isZoomLeftSticky + ', right-sticky:' + self.isZoomRightSticky);
                                    }
                                    if (self.isMouseDown) {
                                        self.isSynchronizeYaxesRequired = true;
                                    } else {
                                        synchronizeYaxes(true);
                                    }
                                    self.$timeout(function () { self.$scope.zoomed = self.isZoomed; }, 0, true);
                                }
                            }
                        },
                        yAxis: self.dataSupplier.yAxes.map(function (yAxis) {
                            return _.merge(
                                {
                                    events: {
                                        setExtremes: function (e) {
                                            self.consoledebug(function () {
                                                return 'yAxis(' + yAxisToString(yAxis) + ').setExtremes():\n'
                                                    + '    dataMin:' + yAxis.dataMin + ', e.min:' + e.min + '\n'
                                                    + '    dataMax:' + yAxis.dataMax + ', e.max:' + e.max;
                                            });
                                        }
                                    }
                                },
                                yAxis
                            );
                        }),
                        tooltip: {
                            className: 'chart-tooltip-container',
                            followTouchMove: self.$location.search().followTouchMove !== 'false',
                            outside: true,
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
                                                .deletePoint(self.device.idx, event.point, self.dataSupplier.isShortLogChart, Intl.DateTimeFormat().resolvedOptions().timeZone)
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
                            },
                            areaspline: {
                                lineWidth: 3,
                                marker: {
                                    enabled: false
                                },
                                states: {
                                    hover: {
                                        lineWidth: 3
                                    }
                                }
                            },
                            column: {
                                pointPlacement: 'between',
                                borderWidth: 0,
                                minPointLength: 4,
                                pointPadding: 0.1,
                                groupPadding: 0,
                                dataLabels: {
                                    enabled: false,
                                    color: 'white'
                                }
                            }
                        },
                        title: false,
                        series: [],
                        legend: {
                            enabled: true
                        },
                        time: {
                            timezone: Intl.DateTimeFormat().resolvedOptions().timeZone
                        }
                    },
                    template
                );
        }

        function refreshChartData(afterChartRefreshed) {
            const dataRequest = createDataRequest();
            const stopwatchDataRequest = stopwatch(function() { return 'sendRequest(' + JSON.stringify(dataRequest) + ')'; });
            self.domoticzApi
                .sendRequest(dataRequest)
                .then(function (data) {
                    self.consoledebug(function () { return stopwatchDataRequest.log(); });
                    self.consoledebug(function () { return '[' + new Date().toString() + '] refreshing ' + self; });

                    if (typeof data.result === 'undefined') {
                        return;
                    }

                    const stopwatchCycle = stopwatch('cycle');
                    loadDataInChart(data);
                    synchronizeYaxes();
                    redrawChart();
                    self.consoledebug(function () { return stopwatchCycle.log(); });
                    if (afterChartRefreshed !== undefined) {
                        afterChartRefreshed();
                    }

                    function loadDataInChart(data) {
                        const chartAnalysis = chartZoomer.analyseChart(self);
                        dataLoader.loadData(data, self);
                        chartLoader.loadChart(self);
                        chartZoomer.zoomChart(self, chartAnalysis);
                    }

                    function redrawChart() {
                        self.chart.redraw();
                    }
                });

            function createDataRequest() {
                return self.extendDataRequest({
                    type: 'graph',
                    sensor: self.sensorType,
                    range: self.range,
                    idx: self.device.idx
                });
            }
        }

        function synchronizeYaxes(redraw=false) {
            if (self.synchronizeYaxes) {
                const yAxes = self.chart.series.map(getYaxisForSeries).reduce(collectToSet, []);
                yAxes.forEach(function (yAxis) {
                    yAxis.setExtremes(null, null, false);
                });
                self.chart.redraw();

                const iMin = yAxes.map(function (yAxis) { return yAxis.min; }).reduce(min, Infinity);
                const iMax = yAxes.map(function (yAxis) { return yAxis.max; }).reduce(max, -Infinity);

                const yAxisMinSynchronized = iMin === Infinity ? null : iMin;
                const yAxisMaxSynchronized = iMax === -Infinity ? null : iMax;
                self.consoledebug('Synchronizing yAxes to extremes (' + yAxisMinSynchronized + ', ' + yAxisMaxSynchronized + '):');
                yAxes.forEach(function (yAxis) {
                    self.consoledebug('    yAxis:' + yAxisToString(yAxis));
                    yAxis.setExtremes(yAxisMinSynchronized, yAxisMaxSynchronized, redraw);
                });
            }

            function min(valueMin, value) {
                return Math.min(value, valueMin);
            }

            function max(valueMax, value) {
                return Math.max(value, valueMax);
            }

            function getYaxisForSeries(series) {
                return self.chart.yAxis[series.options.yAxis];
            }

            function collectToSet(set, item) {
                if (!set.includes(item)) {
                    set.push(item);
                }
                return set;
            }
        }

        function configureZoomingAndPanning() {
            const interTouchEndTouchStartPeriod = intParam('endDelay', 500);
            const touchStartDelay = intParam('startDelay', 400);

            self.isMouseDown = false;

            self.chart.container.addEventListener('touchstart', function (e) {
                if (self.touchEndTimestamp === undefined || self.touchEndTimestamp + interTouchEndTouchStartPeriod < e.timeStamp) {
                    self.touchStartTimestamp = e.timeStamp;
                    self.touchEndTimestamp = undefined;
                    self.consoledebug('Touch start at ' + dateToString(self.touchStartTimestamp));
                }
            });
            self.chart.container.addEventListener('touchmove', function (e) {
                if (self.touchStartTimestamp !== undefined) {
                    if (self.touchStartTimestamp + touchStartDelay < e.timeStamp) {
                        self.chart.pointer.followTouchMove = false;
                        self.consoledebug('Touch start panning');
                    } else {
                        self.consoledebug('Touch start tooltipping');
                    }
                    self.touchStartTimestamp = undefined;
                }
            });
            self.chart.container.addEventListener('touchend', function (e) {
                if (!self.chart.pointer.followTouchMove) {
                    self.consoledebug('Touch end panning');
                } else {
                    self.consoledebug('Touch end tooltipping');
                }
                self.touchEndTimestamp = e.timeStamp;
                self.chart.pointer.followTouchMove = true;
                self.touchStartTimestamp = undefined;
            });
            self.chart.container.addEventListener('mousedown', function (e) {
                self.consoledebug('Mousedown ' + self + ' clientX:' + e.clientX + ' ctrl:' + e.ctrlKey);
                self.wasCtrlMouseDown = e.ctrlKey;
                self.mouseDownPosition = e.clientX;
                self.isMouseDown = true;
            });
            self.chart.container.addEventListener('mouseup', function (e) {
                self.consoledebug('Mouseup ' + self + ' clientX:' + e.clientX + ' ctrl:' + e.ctrlKey);
                self.wasCtrlMouseUp = e.ctrlKey;
                self.mouseUpPosition = e.clientX;
                self.isMouseDown = false;
                if (self.isSynchronizeYaxesRequired) {
                    synchronizeYaxes(true);
                    self.isSynchronizeYaxesRequired = false;
                }
            });

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

            function intParam(parameterName, defaultValue) {
                const parameterValue = self.$location.search()[parameterName];
                if (parameterValue === undefined || !parameterValue.match(/^[0-9]+$/)) {
                    return defaultValue;
                }
                return parseInt(parameterValue);
            }
        }

        function initialZoom() {
            if (zoomValue('zoom') !== undefined) {
                zoomPreset(zoomValue('zoom'), /^(?<value>[0-9]+)d$/, self.$scope.zoomDays);
                zoomPreset(zoomValue('zoom'), /^(?<value>[0-9]+)h$/, self.$scope.zoomHours);
            }
            if (zoomValue('zoomleft') !== undefined || zoomValue('zoomright') !== undefined) {
                zoom(
                    zoomDateTimeEdge(zoomValue('zoomleft'), self.chart.xAxis[0].min),
                    zoomDateTimeEdge(zoomValue('zoomright'), self.chart.xAxis[0].max));
            }
            if (zoomValue('left-sticky') !== undefined) {
                self.isZoomLeftSticky = true;
            }
            if (zoomValue('right-sticky') !== undefined) {
                self.isZoomRightSticky = true;
            }

            function zoomDateTimeEdge(parameterValue, defaultValue=null) {
                if (parameterValue === undefined) {
                    return defaultValue;
                }
                return GetLocalDateTimeFromString(parameterValue);
            }

            function zoomPreset(zoomPresetValue, regex, zoomFunction) {
                const match = zoomPresetValue.match(regex);
                if (match) {
                    zoomFunction(parseInt(match.groups.value));
                }
            }

            function zoomValue(zoomParameter) {
                return self.$location.search()[self.range + '.' + zoomParameter];
            }
        }

        function zoom(min, max) {
            self.chart.xAxis[0].zoom(min, max);
            synchronizeYaxes();
            self.chart.redraw();
        }

        function chartTitle() {
            if (self.chartName !== undefined) {
                return self.chartName + ' ' + chartTitlePeriod();
            } else {
                return chartTitlePeriod();
            }

            function chartTitlePeriod() {
                return self.range === 'day' ? self.domoticzGlobals.Get5MinuteHistoryDaysGraphTitle()
                    : self.range === 'week' ? $.t('Last Week')
                        : self.range === 'month' ? $.t('Last Month')
                            : self.range === 'year' ? $.t('Last Year')
                                : '';
            }
        }

        function completeSeriesSuppliers(dataSupplier) {
            return dataSupplier.seriesSuppliers
                .map(function (seriesSupplierOrFunction) {
                    if (typeof seriesSupplierOrFunction === 'function') {
                        return seriesSupplierOrFunction(dataSupplier);
                    } else {
                        return seriesSupplierOrFunction;
                    }
                })
                .map(function (seriesSupplier) {
                    return _.merge({
                        useDataItemsFromPrevious: false,
                        extendSeriesNameWithLabel: self.$location.search().serieslabels === 'true',
                        dataSupplier: dataSupplier,
                        dataItemKeys: [],
                        dataItemIsValid:
                            function (dataItem) {
                                return this.dataItemKeys.every(function (dataItemKey) {
                                    return dataItem[dataItemKey] !== undefined;
                                });
                            },
                        valuesFromDataItem:
                            function (dataItem) {
                                return this.dataItemKeys.map(function (dataItemKey) {
                                    return parseFloat(dataItem[dataItemKey]);
                                });
                            },
                        timestampFromDataItem: function (dataItem) {
                            if (!this.useDataItemsFromPrevious) {
                                return dataSupplier.timestampFromDataItem(dataItem);
                            } else {
                                return dataSupplier.timestampFromDataItem(dataItem, 1);
                            }
                        }
                    }, seriesSupplier);
                });
        }

        function dateToString(date) {
            if (date === undefined) {
                return 'undefined';
            }
            if (date === null) {
                return 'null';
            }
            if (date === Infinity) {
                return 'Infinity';
            }
            if (date === -Infinity) {
                return '-Infinity';
            }
            return new Date(date).toString();
        }

        function yAxisToString(yAxis) {
            if (yAxis === undefined) {
                return 'undefined';
            }
            if (yAxis.title === undefined) {
                return 'yAxis.undefined';
            }
            if (yAxis.title.text === undefined) {
                return 'yAxis.title.undefined';
            }
            return yAxis.title.text;
        }
    }

    RefreshingChart.prototype = Object.create(DomoticzBase.prototype);
    RefreshingChart.prototype.constructor = RefreshingChart;

    RefreshingChart.prototype.toString = function () {
        return this.chartName + ' ' + this.range;
    };

    function stopwatch(label, f, indent='') {
        const timer = {
            label : label,
            indent: indent,
            startTime: Date.now(),
            stop: function() {
                this.endTime = Date.now();
            },
            lapsedMsecs: function () {
                if (this.endTime === undefined) {
                    return Date.now() - this.startTime;
                }
                return this.endTime - this.startTime;
            },
            log: function () {
                return this.indent
                    + (typeof this.label === 'function' ? this.label() : this.label)
                    + ': ' + this.lapsedMsecs() + 'msecs';
            },
            split: function (label) {
                return stopwatch(label, indent='    ');
            }
        };
        if (f !== undefined) {
            f();
            timer.stop();
        }
        return timer;
    }

    return RefreshingChart;
});
