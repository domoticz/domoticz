define(['lodash', 'Base', 'DomoticzBase', 'DataLoader', 'ChartLoader', 'ChartZoomer'], function (_, Base, DomoticzBase, DataLoader, ChartLoader, ChartZoomer) {

    function RefreshingChart(
            baseParams, angularParams, domoticzParams, params,
            dataLoader = new DataLoader(),
            chartLoader = new ChartLoader(angularParams.location),
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
        self.ctrl = params.ctrl;
        self.range = params.range;
        self.chartName = params.chartName;
        self.chartNameIsToggling = params.chartNameIsToggling;
        self.device = params.device;
        self.dataSupplier = params.dataSupplier;
        self.extendDataRequest = params.dataSupplier.extendDataRequest || function (dataRequest) { return dataRequest; };
        self.synchronizeYaxes = params.synchronizeYaxes;
        const chartDefinition = createChartDefinition(params.highchartTemplate);
        chartDefinition.range = params.range;
        self.chart = self.$element.find('.chartcontainer').highcharts(chartDefinition).highcharts();
        // Disable the Highcharts Reset Zoom button
        self.chart.showResetZoom = function () {};
        self.autoRefreshIsEnabled = params.autoRefreshIsEnabled;
        self.refreshTimestamp = 0;

        self.isZoomLeftSticky = false;
        self.isZoomRightSticky = false;
        self.isZoomed = false;

        refreshChartData(function () {
            self.$scope.chartTitle = chartTitle();
            self.$scope.chartTitleToggling = chartTitleIsToggling();
            initialZoom();
        });
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
                        panning: true,
                        panKey: 'shift'
                    },
                    xAxis: {
                        type: 'datetime',
                        gridLineColor: '#aaaaaa',
                        gridLineDashStyle: 'dot',
                        gridLineWidth: .5,
                        events: {
                            setExtremes: function (e) {
                                const xAxis = self.chart.xAxis[0];
                                self.consoledebug(function () {
                                    return 'xAxis.setExtremes():\n'
                                        + '    dataMin:' + Base.dateToString(xAxis.dataMin) + ', e.min:' + Base.dateToString(e.min) + '\n'
                                        + '    dataMax:' + Base.dateToString(xAxis.dataMax) + ', e.max:' + Base.dateToString(e.max);
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
                                self.seriesSuppliers.forEach(function (seriesSupplier) {
                                    if (seriesSupplier.chartZoomLevelChanged !== undefined) {
                                        seriesSupplier.chartZoomLevelChanged(self.chart,
                                            e.min !== null ? e.min : xAxis.dataMin, e.max !== null ? e.max : xAxis.dataMax);
                                    }
                                });

                                if (self.isMouseDown) {
                                    self.isSynchronizeYaxesRequired = true;
                                } else {
                                    synchronizeYaxes(true);
                                }
                                self.$timeout(function () {
                                    self.$scope.zoomed = self.isZoomed;
                                }, 0, true);
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
						//useHTML: true,
                        valueSuffix: self.dataSupplier.valueSuffix
                    },
                    plotOptions: {
                        series: {
                            point: {
                                events: {
                                    click: function (event) {
                                        self.consoledebug('Chart click: ' + event.point);
                                        self.chartPoint = event.point;
                                        self.chartPointPosition = self.touchStartPosition || self.mouseDownPosition;
                                        if (event.shiftKey === true) {
                                            if (deletePointIsSupported()) {
                                                self.domoticzDatapointApi.deletePoint(
                                                    self.device.idx,
                                                    event.point,
                                                    self.dataSupplier.isShortLogChart,
                                                    Intl.DateTimeFormat().resolvedOptions().timeZone
                                                ).then(function () {
                                                    self.$route.reload();
                                                });
                                            }
                                        }
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
                            minPointLength: 2,
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

        function refreshChartData(afterRefreshChartData) {
            const dataRequest = createDataRequest();
            const stopwatchDataRequest = stopwatch(function() { return 'sendRequest(' + JSON.stringify(dataRequest) + ')'; });
            self.domoticzApi
                .sendRequest(dataRequest)
                .then(function (data) {
                    self.consoledebug(function () { return stopwatchDataRequest.log(); });
                    self.consoledebug(function () { return '[' + Base.dateToString(new Date()) + '] refreshing ' + self; });

                    const stopwatchCycle = stopwatch('cycle');
                    loadDataInChart(data);
                    synchronizeYaxes();
                    redrawChart();
                    self.consoledebug(function () { return stopwatchCycle.log(); });
                    if (afterRefreshChartData !== undefined) {
                        afterRefreshChartData();
                    }

                    function loadDataInChart(data) {
                        const chartAnalysis = chartZoomer ? chartZoomer.analyseChart(self) : null;
                        dataLoader.prepareData(data, self);
                        dataLoader.loadData(data, self);
                        chartLoader.loadChart(self);
                        if (chartZoomer) {
                            chartZoomer.zoomChart(self, chartAnalysis);
                        }
                    }

                    function redrawChart() {
                        // Forces the legend to be redrawn as well
                        // self.chart.isDirtyLegend = true;
                        self.chart.redraw();
                    }
                });

            function createDataRequest() {
                return self.extendDataRequest({
                    type: 'command',
                    param: 'graph',
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
            const touchStartPanningDelay = intParam('startPanningDelay', 400);
            const touchDeletePointDelay = intParam('deletePointDelay', 800);

            self.isMouseDown = false;

            self.chart.container.addEventListener('touchstart', function (e) {
                self.consoledebug('Touchstart at ' + e.touches[0].clientX + ' on ' + Math.round(e.timeStamp)
                    + (self.touchEndTimestamp === undefined ? '' : ', te:' + self.touchEndTimestamp));
                if (self.touchEndTimestamp === undefined || self.touchEndTimestamp + interTouchEndTouchStartPeriod < e.timeStamp) {
                    self.touchStartTimestamp = e.timeStamp;
                    self.touchStartPosition = e.touches[0].clientX;
                    self.touchEndTimestamp = undefined;
                    self.touchMoveTimestamp = undefined;
                }
            });
            self.chart.container.addEventListener('touchmove', function (e) {
                self.consoledebug('Touchmove at ' + e.touches[0].clientX + ' on ' + Math.round(e.timeStamp)
                    + (self.touchStartTimestamp === undefined ? '' : ', ts:' + self.touchStartTimestamp));
                if (self.touchMoveTimestamp === undefined) {
                    self.touchMoveTimestamp = e.timeStamp;
                    if (self.touchStartTimestamp + touchStartPanningDelay < e.timeStamp) {
                        self.chart.pointer.followTouchMove = false;
                        self.consoledebug('Start panning');
                    } else {
                        self.consoledebug('Start following tooltip');
                    }
                }
            });
            self.chart.container.addEventListener('touchend', function (e) {
                self.consoledebug('Touchend at ' + e.changedTouches[0].clientX + ' on ' + Math.round(e.timeStamp));
                if (self.touchMoveTimestamp !== undefined) {
                    if (!self.chart.pointer.followTouchMove) {
                        self.consoledebug('End panning');
                    } else {
                        self.consoledebug('End following tooltip');
                    }
                } else {
                    if (self.touchStartTimestamp + touchDeletePointDelay < e.timeStamp) {
                        if (self.chartPoint !== undefined && self.chartPointPosition === self.touchStartPosition) {
                            if (deletePointIsSupported()) {
                                self.consoledebug('Delete point ' + self.chartPoint + ' at ' + self.chartPointPosition);
                                self.domoticzDatapointApi
                                    .deletePoint(self.device.idx, self.chartPoint, self.dataSupplier.isShortLogChart, Intl.DateTimeFormat().resolvedOptions().timeZone)
                                    .then(function () {
                                        self.$route.reload();
                                    });
                            }
                        }
                    }
                }
                self.touchEndTimestamp = e.timeStamp;
                self.chart.pointer.followTouchMove = true;
            });
            self.chart.container.addEventListener('mousedown', function (e) {
                debugMouseAction('Mousedown', e);
                self.mouseDownTimestamp = e.timeStamp;
                self.wasCtrlMouseDown = e.ctrlKey;
                self.mouseDownPosition = e.clientX;
                self.isMouseDown = true;
            });
            self.chart.container.addEventListener('mouseup', function (e) {
                debugMouseAction('Mouseup', e);
                self.mouseUpTimestamp = e.timeStamp;
                self.wasCtrlMouseUp = e.ctrlKey;
                self.mouseUpPosition = e.clientX;
                self.isMouseDown = false;
                if (self.isSynchronizeYaxesRequired) {
                    synchronizeYaxes(true);
                    self.isSynchronizeYaxesRequired = false;
                }
            });
            self.chart.container.addEventListener('click', function (e) {
                const event = self.chart.pointer.normalize(e);
                const plotX = event.chartX - self.chart.plotLeft;
                const plotY = event.chartY - self.chart.plotTop;
                self.consoledebug('Click at '
                    + ' (' + e.clientX + ',' + e.clientY + ') -> (' + plotX + ',' + plotY + ')'
                    + ' ctrl:' + e.ctrlKey + ' shift:' + e.shiftKey + ' alt:' + e.altKey
                    + ' type:' + self.device.Type + ', subtype:' + self.device.SubType);
                self.chart.yAxis.forEach(function (x) {
                    self.consoledebug('yAxis => index:' + x.userOptions.index + ', title:' + x.userOptions.title.text + ', opposite:' + x.userOptions.opposite + ', left:' + x.left + ', right:' + x.right + ', offset:' + x.offset + ', labelOffset:' + x.labelOffset + ', dataMin:' + x.dataMin + ', dataMax:' + x.dataMax + ', dzZoom:' + x.dzZoom);
                });
                if (plotX < 0 || self.chart.plotWidth < plotX && self.chart.yAxis.some(function (axis) { return axis.userOptions.opposite; })) {
                    const axes = axesOrdered(self.chart.yAxis, plotX, self.chart.plotWidth);
                    const nextZoomValue = nextZoom(axes[0].dzZoom, e);
                    axes.splice(0, e.altKey ? axes.length : 1).forEach(function (axis) {
                        yAxisZoom(axis, nextZoomValue);
                    });
                    synchronizeYaxes(true);
                    // self.chart.redraw();
                }

                function axesOrdered(axes, plotX, plotWidth) {
                    const x = plotX < 0 ? -plotX : plotX - plotWidth;
                    const xSide = plotX < 0 ? -1 : 1;
                    return axes
                        .map(function (axis) {
                            const axisOffset = Math.abs(axis.offset);
                            const axisSide = axis.userOptions.opposite ? 1 : -1;
                            return {axis: axis, ordinal: (axisOffset < x ? axisOffset - x : 1000) + (axisSide === xSide ? 0 : 1000)};
                        }).sort(function (l, r) {
                            return l.ordinal - r.ordinal;
                        }).map(function (axisHolder) {
                            return axisHolder.axis;
                        });
                }

                function nextZoom(zoom, e) {
                    const c = e.ctrlKey;
                    const s = e.shiftKey;
                    const def = !c && !s;
                    const full = c && s;
                    if (zoom === undefined && (def || full)) {
                        return 'full';
                    } else if (zoom === 'full' && (def || full)) {
                        return 'default';
                    }
                    return 'default';
                }

                function yAxisZoom(yAxis, zoom) {
                    let axisMin, axisMax;
                    const t = self.device.Type;
                    const s = self.device.SubType;
                    if (['Percentage'].includes(s) || ['Temp', 'Setpoint', 'Humidity', 'Heating'].includes(t)) {
                        axisMin = 0;
                        axisMax = 100;
                    } else if (['Visibility'].includes(s)) {
                        axisMin = 0;
                        axisMax = 10;
                    } else if (['Lux'].includes(s)) {
                        axisMin = 0;
                        axisMax = 10000;
                    } else if (['Usage', 'Temp + Humidity + Baro'].includes(t)) {
                        axisMin = 0;
                        axisMax = yAxis.dataMax;
                    }
                    if (zoom === 'low') {
                        if (axisMin !== undefined) {
                            self.consoledebug('yAxis.setExtremes(axisMin [' + axisMin + '],null)');
                            yAxis.setExtremes(axisMin, null);
                        }
                        axisSetTitle(yAxis, bottom);
                        yAxis.dzZoom = 'low';
                    } else if (zoom === 'high') {
                        if (axisMax !== undefined) {
                            self.consoledebug('yAxis.setExtremes(null,axisMax [' + axisMax + '])');
                            yAxis.setExtremes(null, axisMax);
                        }
                        axisSetTitle(yAxis, top);
                        yAxis.dzZoom = 'high';
                    } else if (zoom === 'full') {
                        if (axisMin !== undefined && axisMax !== undefined) {
                            self.consoledebug('yAxis.setExtremes(axisMin [' + axisMin + '],axisMax [' + axisMax + '])');
                            yAxis.setExtremes(axisMin, axisMax);
                        }
                        axisSetTitle(yAxis, topbottom);
                        yAxis.dzZoom = 'full';
                    } else if (zoom === 'default') {
                        self.consoledebug('yAxis.setExtremes(null,null)');
                        yAxis.setExtremes(null, null);
                        axisSetTitle(yAxis, none);
                        yAxis.dzZoom = undefined;
                    }

                    function axisSetTitle(axis, newTitle) {
                        const title = axis.userOptions.title.text.replace(/^\[\u2004/, '').replace(/\u2004\]$/, '');
                        axis.setTitle({text: newTitle(axis, title)}, true);
                    }

                    function none(axis, title) {
                        return title;
                    }

                    function top(axis, title) {
                        return axis.userOptions.opposite ? '[\u2004' + title : title + '\u2004]';
                    }

                    function topbottom(axis, title) {
                        return '[\u2004' + title + '\u2004]';
                    }

                    function bottom(axis, title) {
                        return axis.userOptions.opposite ? title + '\u2004]' : '[\u2004' + title;
                    }
                }
            });

            function debugMouseAction(action, e) {
                self.consoledebug(action + (e.ctrlKey ? ' +ctrl' : '') + (e.shiftKey ? ' +shift' : '') + (e.altKey ? ' +alt' : '') + ' at (' + e.clientX + ',' + e.clientY + ') on ' + Math.round(e.timeStamp));
            }

            self.$scope.zoomed = false;

            self.$scope.shortLogHistoryMaxDays = self.$scope.$root.config.FiveMinuteHistoryDays;

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
                const right = Math.min(xAxis.max, xAxis.dataMax);
                zoom(right - hours * 3600 * 1000, right);
            }

            self.$scope.zoomDays = function (days) {
                const xAxis = self.chart.xAxis[0];
                const right = Math.min(xAxis.max, xAxis.dataMax);
                zoom(addDays(right, -days), right);

                function addDays(right, days) {
                    const leftDate = new Date(right);
                    leftDate.setDate(leftDate.getDate() + days);
                    return leftDate.getTime();
                }
            }

            self.$scope.zoomMonths = function (months) {
                const xAxis = self.chart.xAxis[0];
                const right = Math.min(xAxis.max, xAxis.dataMax);
                zoom(addMonths(right, -months), right);

                function addMonths(right, months) {
                    const leftDate = new Date(right);
                    leftDate.setDate(leftDate.getDate() + 1);
                    leftDate.setMonth(leftDate.getMonth() + months);
                    leftDate.setDate(leftDate.getDate() - 1);
                    return leftDate.getTime();
                }
            }

            self.$scope.zoomreset = function () {
                const xAxis = self.chart.xAxis[0];
                zoom(xAxis.dataMin, xAxis.dataMax);
            }

            self.$scope.groupByLabel = function (label) {
                const matcher = label.match(/^(?<letter>[yq])$/);
                if (matcher !== null) {
                    const letter = matcher.groups.letter;
                    const groupingBy =
                        letter === 'y' ? 'Year' :
                            letter === 'q' ? 'Quarter' :
                                letter === 'm' ? 'Month' : '';
                    return $.t(groupingBy).substring(0, 1).toLowerCase();
                }
                return $.t(label).toLowerCase();
            }

            self.$scope.groupBy = function (groupingBy) {
                self.ctrl.groupingBy = groupingBy;
                self.chart.update(
                    {
                        plotOptions: {
                            column: {
                                stacking: self.ctrl.groupingBy === 'year' ? 'normal' : undefined
                            }
                        }
                    }, false);
                refreshChartData();
            };

            self.$element.find('.chart-title-container').on('click', function (e) {
                debugMouseAction('Click on title', e);
                if (self.ctrl !== undefined && self.ctrl.toggleTitleState !== undefined) {
                    if (self.ctrl.toggleTitleState()) {
                        refreshChartData(function () {
                            self.$scope.chartTitle = chartTitle();
                            self.$scope.chartTitleToggling = chartTitleIsToggling();
                        });
                    }
                }
            });
            self.$element.find('.chartcontainer').on('refreshChartData', function (e) {
                refreshChartData();
            });
            self.$element.find('.chartcontainer').on('dblclick', function (e) {
                const event = self.chart.pointer.normalize(e);
                const plotX = event.chartX - self.chart.plotLeft;
                const plotWidth = self.chart.plotSizeX;
                const xAxis = self.chart.xAxis[0];
                const plotRange = xAxis.max - xAxis.min;
                const plotRangeNew = plotRange * .5;
                const plotRangeCut = plotRange - plotRangeNew;
                self.consoledebug('plotX:' + plotX + ', plotWidth:' + plotWidth);
                if (plotX <= 10) {
                    // keep left edge on same position
                    zoom(xAxis.min, -plotRangeCut + xAxis.max);
                } else if (0+10 < plotX && plotX < -10+plotWidth) {
                    // keep touchpoint on same position
                    const plotRangeCutLeft = plotRangeCut * (plotX/plotWidth);
                    const plotRangeCutRight = plotRangeCut * ((plotWidth-plotX)/plotWidth);
                    zoom(xAxis.min + plotRangeCutLeft, -plotRangeCutRight + xAxis.max);
                    // keep center on same position
                    // zoom(xAxis.min + plotRangeCut/2, -plotRangeCut/2 + xAxis.max);
                } else {
                    // keep right edge on same position
                    zoom(xAxis.min + plotRangeCut, xAxis.max);
                }
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
            self.chart.tooltip.hide();
        }

        function chartTitle() {
            if (self.chartName !== undefined) {
                let chartName;
                chartName = fromInstanceOrFunction()(self.chartName);
                const periodInTitle = chartTitlePeriod();
                return chartName + (periodInTitle ? ' ' + periodInTitle : '');
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

        function chartTitleIsToggling() {
            return typeof self.chartNameIsToggling === 'function' && self.chartNameIsToggling();
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

        function deletePointIsSupported() {
            return ['day', 'month', 'year'].includes(self.range);
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
                    + fromInstanceOrFunction()(this.label)
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
