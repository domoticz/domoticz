define(['app', 'RefreshingChart', 'DataLoader', 'ChartLoader', 'log/Chart', 'log/factories'], function (app, RefreshingChart, DataLoader, ChartLoader) {

    app.component('deviceTemperatureLog', {
        bindings: {
            device: '<',
        },
        templateUrl: 'app/log/TemperatureLog.html',
        controller: function() {
            const $ctrl = this;
            $ctrl.autoRefresh = true;
            $ctrl.showAdvancedCharts = false;
            $ctrl.toggleAdvancedCharts = function () {
                $ctrl.showAdvancedCharts = !$ctrl.showAdvancedCharts;
            };

            $ctrl.$onInit = function() {
                $ctrl.deviceIdx = $ctrl.device.idx;
                $ctrl.deviceType = $ctrl.device.Type;
                $ctrl.degreeType = $.myglobals.tempsign;
            }
        }
    });

    const degreeSuffix = '\u00B0' + $.myglobals.tempsign;

    app.directive('temperatureShortChart', function () {
        return {
            require: {
                logCtrl: '^deviceTemperatureLog'
            },
            scope: {
                device: '<',
                degreeType: '<'
            },
            templateUrl: 'app/log/chart-day.html',
            replace: true,
            transclude: true,
            bindToController: true,
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi) {
                const self = this;
                self.range = 'day';
                self.sensorType = 'temp';

                self.$onInit = function() {
                    var params = chartParams(
                        domoticzGlobals,
                        self,
                        true,
                        function (dataItem, yearOffset = 0) {
                            return GetLocalDateTimeFromString(dataItem.d, yearOffset);
                        },
                        [
                            humiditySeriesSupplier(),
                            chillSeriesSupplier(),
                            setpointSeriesSupplier(),
                            temperatureSeriesSupplier(self.device.Type)
                        ]
                    );
                    params.dataSupplier.preprocessData = function (data) {
                        self.logCtrl.dayGraphData = data.result;
                    };
                    self.chart = new RefreshingChart(
                        baseParams($),
                        angularParams($location, $route, $scope, $timeout, $element),
                        domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        params
                    );
                };
            }
        }
    });

    app.directive('temperatureLongChart', function () {
        return {
            require: {
                logCtrl: '^deviceTemperatureLog'
            },
            scope: {
                device: '<',
                degreeType: '<',
                range: '@'
            },
            templateUrl: function($element, $attrs) { return 'app/log/chart-' + $attrs.range + '.html'; },
            replace: true,
            bindToController: true,
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi) {
                const self = this;
                self.sensorType = 'temp';

                self.$onInit = function() {
                    self.chart = new RefreshingChart(
                        baseParams($),
                        angularParams($location, $route, $scope, $timeout, $element),
                        domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        chartParams(
                            domoticzGlobals,
                            self,
                            false,
                            function (dataItem, yearOffset = 0) {
                                return GetLocalDateFromString(dataItem.d, yearOffset);
                            },
                            [
                                humiditySeriesSupplier(),
                                chillSeriesSupplier(),
                                chillMinimumSeriesSupplier(),
                                setpointAverageSeriesSupplier(),
                                setpointRangeSeriesSupplier(),
                                setpointPreviousSeriesSupplier(),
                                temperatureAverageSeriesSupplier(self.device.Type),
                                temperatureRangeSeriesSupplier(self.device.Type),
                                temperaturePreviousSeriesSupplier(),
                                temperatureTrendlineSeriesSupplier(self.device.Type)
                            ]
                        )
                    );
                };
            }
        }
    });
	
	changeCompType = function() {
			alert("Change!!");
	}

    app.directive('temperatureCompareChart', function () {
        return {
            require: {
                logCtrl: '^deviceTemperatureLog'
            },
            scope: {
                device: '<',
                degreeType: '<',
                range: '@'
            },
            templateUrl: function($element, $attrs) { return 'app/log/chart-' + $attrs.range + '-temp.html'; },
            replace: true,
            bindToController: true,
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi, chart) {
                const self = this;
				self.groupingBy = 'month';
				//self.deviceType = this.device.Type;
				//console.log(self.deviceType);
                self.sensorType = 'temp';
				self.var_name = 'Temp_Avg';
				self.valueSuffix = degreeSuffix;

                self.$onInit = function() {
					let bIsHumidity = (this.device.Type === 'Humidity');
					if (bIsHumidity) {
						self.sensorType = 'hum';
						self.var_name = 'Humidity';
						self.valueSuffix = '%';
					}
					
                    self.chart = new RefreshingChart(
                        chart.baseParams($),
                        chart.angularParams($location, $route, $scope, $timeout, $element),
                        chart.domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
						chart.chartParamsCompare(
							domoticzGlobals,
							self,
							chart.chartParamsCompareTemplate(self, 'Temperature', self.valueSuffix),
                            {
                                isShortLogChart: false,
                                yAxes: [{
											title: {
												text: (!bIsHumidity) ? $.t('Degrees') : $.t('Humidity') + ' ' + self.valueSuffix
											}
										}],
                                extendDataRequest: function (dataRequest) {
                                    dataRequest['groupby'] = self.groupingBy;
									dataRequest['var_name'] = self.var_name;
                                    return dataRequest;
                                },
                                preprocessData: function (data) {
                                    this.firstYear = data.firstYear;
                                    this.categories = categoriesFromGroupingBy.call(this, self.groupingBy);
                                    if (self.chart.chart.xAxis[0].categories === true) {
                                        self.chart.chart.xAxis[0].categories = [];
                                    } else {
                                        self.chart.chart.xAxis[0].categories.length = 0;
                                    }
                                    this.categories.forEach(function (c) {
                                        self.chart.chart.xAxis[0].categories.push(c); });

                                    function categoriesFromGroupingBy(groupingBy) {
                                        if (groupingBy === 'year') {
                                            if (this.firstYear === undefined) {
                                                return [];
                                            }
                                            return _.range(this.firstYear, new Date().getFullYear() + 1).map(year => year.toString());
                                        } else if (groupingBy === 'quarter') {
                                            return ['Q1', 'Q2', 'Q3', 'Q4'];
                                        } else if (groupingBy === 'month') {
                                            return _.range(1, 13).map(month => pad2(month));
                                        }

                                        function pad2(i) {
                                            return (i < 10 ? '0' : '') + i.toString();
                                        }
                                    }
                                },
                            },
                            chart.compareSeriesSuppliers(self)
                        ),
                        new DataLoader(),
                        new ChartLoader($location),
                        null
                    );
                };
            }
        }
    });

    function humiditySeriesSupplier() {
        return {
            id: 'humidity',
            dataItemKeys: ['hu'],
            showWithoutDatapoints: false,
            label: 'Hu',
            template: {
                name: $.t('Humidity'),
                color: 'limegreen',
				type: 'spline',
                yAxis: 1,
                tooltip: {
                    valueSuffix: ' %',
                    valueDecimals: 0
                }
            }
        };
    }

    function chillSeriesSupplier() {
        return {
            id: 'chill',
            dataItemKeys: ['ch'],
            showWithoutDatapoints: false,
            label: 'Ch',
            template: {
                name: $.t('Chill'),
                color: 'red',
                yAxis: 0,
                zIndex: 1,
                tooltip: {
                    valueSuffix: ' ' + degreeSuffix,
                }
            }
        };
    }

    function setpointSeriesSupplier() {
        return {
            id: 'setpoint',
            dataItemKeys: ['se'],
            showWithoutDatapoints: false,
            label: 'Se',
            template: {
                name: $.t('Set Point'),
                color: 'blue',
                yAxis: 0,
                zIndex: 1,
                tooltip: {
                    valueSuffix: ' ' + degreeSuffix,
                    valueDecimals: 1
                }
            }
        };
    }

    function temperatureSeriesSupplier(deviceType) {
        return {
            id: 'temperature',
            dataItemKeys: ['te'],
			showWithoutDatapoints: (deviceType !== 'Humidity'),
            label: 'Te',
            template: {
                name: $.t('Temperature'),
                color: 'yellow',
                yAxis: 0,
                step: deviceType === 'Setpoint' ? 'left' : undefined,
                tooltip: {
                    valueSuffix: ' ' + degreeSuffix,
                }
            }
        };
    }

    function chillMinimumSeriesSupplier() {
        return {
            id: 'chillmin',
            dataItemKeys: ['cm'],
            dataItemIsComplete: function (dataItem) {
                return dataItem.ch !== undefined;
            },
            showWithoutDatapoints: false,
            label: 'Cm',
            template: {
                name: $.t('Chill') + ' ' + $.t('Minimum'),
                color: 'rgba(255,127,39,0.8)',
                linkedTo: ':previous',
                zIndex: 1,
                tooltip: {
                    valueSuffix: ' ' + degreeSuffix,
                },
                yAxis: 0
            }
        };
    }

    function setpointAverageSeriesSupplier() {
        return {
            id: 'setpointavg',
            dataItemKeys: ['se'],
            showWithoutDatapoints: false,
            label: 'Sa',
            template: {
                name: $.t('Set Point') + ' ' + $.t('Average'),
                color: 'blue',
                fillOpacity: 0.7,
                zIndex: 2,
                tooltip: {
                    valueSuffix: ' ' + degreeSuffix,
                    valueDecimals: 1
                },
                yAxis: 0
            }
        };
    }

    function setpointRangeSeriesSupplier() {
        return {
            id: 'setpointrange',
            dataItemKeys: ['sm', 'sx'],
            dataItemIsComplete: function (dataItem) {
                return dataItem.se !== undefined;
            },
            showWithoutDatapoints: false,
            label: 'Sr',
            template: {
                name: $.t('Set Point') + ' ' + $.t('Range'),
                color: 'rgba(164,75,148,1.0)',
                type: 'areasplinerange',
                linkedTo: ':previous',
                zIndex: 1,
                lineWidth: 0,
                fillOpacity: 0.5,
                yAxis: 0,
                tooltip: {
                    valueSuffix: ' ' + degreeSuffix,
                    valueDecimals: 1
                }
            }
        };
    }

    function setpointPreviousSeriesSupplier() {
        return {
            id: 'prev_setpoint',
            dataItemKeys: ['se'],
            useDataItemsFromPrevious: true,
            showWithoutDatapoints: false,
            label: 'Sp',
            template: {
                name: $.t('Past') + ' ' + $.t('Set Point'),
                color: 'rgba(223,212,246,0.8)',
                zIndex: 3,
                yAxis: 0,
                tooltip: {
                    valueSuffix: ' ' + degreeSuffix,
                    valueDecimals: 1
                },
                visible: false
            }
        };
    }

    function temperatureAverageSeriesSupplier(deviceType) {
        return {
            id: 'temperature_avg',
            dataItemKeys: ['ta'],
			showWithoutDatapoints: (deviceType !== 'Humidity'),
            dataItemIsComplete: function (dataItem) {
                return dataItem.te !== undefined && dataItem.ta !== undefined;
            },
            label: 'Ta',
            template: {
                name: $.t('Temperature') + ' ' + $.t('Average'),
                color: 'yellow',
                fillOpacity: 0.7,
                yAxis: 0,
                zIndex: 2,
                tooltip: {
                    valueSuffix: ' ' + degreeSuffix,
                    valueDecimals: 1
                }
            }
        };
    }

    function temperatureRangeSeriesSupplier(deviceType) {
        return {
            id: 'temperature',
            dataItemKeys: ['tm', 'te'],
			showWithoutDatapoints: (deviceType !== 'Humidity'),
            dataItemIsComplete: function (dataItem) {
                return dataItem.te !== undefined;
            },
            label: 'Tr',
            template: {
                name: $.t('Temperature') + ' ' + $.t('Range'),
                color: 'rgba(3,190,252,1.0)',
                type: 'areasplinerange',
                linkedTo: ':previous',
                zIndex: 0,
                lineWidth: 0,
                fillOpacity: 0.5,
                yAxis: 0,
                tooltip: {
                    valueSuffix: ' ' + degreeSuffix,
                }
            }
        };
    }

    function temperaturePreviousSeriesSupplier() {
        return {
            id: 'prev_temperature',
            dataItemKeys: ['ta'],
            useDataItemsFromPrevious: true,
            showWithoutDatapoints: false,
            label: 'Tp',
            template: {
                name: $.t('Past') + ' ' + $.t('Temperature'),
                color: 'rgba(224,224,230,0.8)',
                zIndex: 3,
                yAxis: 0,
                tooltip: {
                    valueSuffix: ' ' + degreeSuffix,
                    valueDecimals: 1
                },
                visible: false
            }
        };
    }

    function temperatureTrendlineSeriesSupplier(deviceType) {
        return {
            id: 'temp_trendline',
            dataItemKeys: ['ta'],
			showWithoutDatapoints: (deviceType !== 'Humidity'),
            dataItemIsComplete: function (dataItem) {
                return dataItem.te !== undefined && dataItem.ta !== undefined;
            },
            postprocessDatapoints: function (datapoints) {
                const trendline = CalculateTrendLine(datapoints);
                datapoints.length = 0;
                if (trendline !== undefined) {
                    datapoints.push([trendline.x0, trendline.y0]);
                    datapoints.push([trendline.x1, trendline.y1]);
                }
            },
            label: 'Tt',
            template: {
                name: $.t('Trendline') + ' ' + $.t('Temperature'),
                zIndex: 1,
                tooltip: {
                    valueSuffix: ' ' + degreeSuffix,
                },
                color: 'rgba(255,3,3,0.8)',
                dashStyle: 'LongDash',
                yAxis: 0,
                visible: false
            }
        };
    }

    function baseParams(jquery) {
        return {
            jquery: jquery
        };
    }
    function angularParams(location, route, scope, timeout, element) {
        return {
            location: location,
            route: route,
            scope: scope,
            timeout: timeout,
            element: element
        };
    }
    function domoticzParams(globals, api, datapointApi) {
        return {
            globals: globals,
            api: api,
            datapointApi: datapointApi
        };
    }
    var customChartTemplate =
        '<div class="chart noselect">' +
            '<div class="chart-title-center">' +
                '<div class="chart-title-container"><h2>{{vm.chartTitle}}</h2></div>' +
            '</div>' +
            '<div class="chartarea">' +
                '<div class="chartcontainer" style="height:300px;"></div>' +
            '</div>' +
        '</div>';

    app.component('temperatureCurrentConditions', {
        require: {
            logCtrl: '^deviceTemperatureLog'
        },
        bindings: {
            device: '<'
        },
        template:
            '<div class="chart noselect current-conditions" style="padding: 10px 0;">' +
                '<div style="display:flex; justify-content:center; flex-wrap:wrap; gap:20px;">' +
                    '<div ng-repeat="card in vm.cards" style="text-align:center; min-width:140px; padding:12px 20px; ' +
                        'background:rgba(255,255,255,0.05); border-radius:8px;">' +
                        '<div style="font-size:0.85em; opacity:0.7;">{{card.label}}</div>' +
                        '<div style="font-size:1.8em; font-weight:bold;">{{card.value}}</div>' +
                        '<div style="font-size:0.85em;" ng-style="{color: card.deltaColor}">{{card.delta}}</div>' +
                    '</div>' +
                '</div>' +
            '</div>',
        controllerAs: 'vm',
        controller: function ($element, $scope) {
            const self = this;
            self.cards = [];

            self.$onInit = function () {
                var device = self.device;

                function formatDelta(current, previous, suffix, decimals) {
                    if (isNaN(current) || isNaN(previous)) return '';
                    var d = current - previous;
                    var sign = d >= 0 ? '+' : '';
                    return sign + d.toFixed(decimals) + ' ' + suffix;
                }

                function deltaColor(current, previous) {
                    if (isNaN(current) || isNaN(previous)) return '';
                    var d = current - previous;
                    if (Math.abs(d) < 0.1) return '#aaa';
                    return d > 0 ? '#ff6b6b' : '#4ecdc4';
                }

                function find24hAgo(items) {
                    var latest = items[items.length - 1];
                    var target24h = GetLocalDateTimeFromString(latest.d) - 24 * 3600000;
                    var closest = items[0];
                    var closestDiff = Math.abs(GetLocalDateTimeFromString(items[0].d) - target24h);
                    for (var i = 1; i < items.length; i++) {
                        var diff = Math.abs(GetLocalDateTimeFromString(items[i].d) - target24h);
                        if (diff < closestDiff) {
                            closestDiff = diff;
                            closest = items[i];
                        }
                    }
                    return closest;
                }

                function rebuildCards() {
                    self.cards.length = 0;
                    var items = self.logCtrl.dayGraphData;
                    var closest24h = (items && items.length >= 2) ? find24hAgo(items) : null;

                    // Temperature card
                    if (device.Temp !== undefined) {
                        var te = parseFloat(device.Temp);
                        var te24 = closest24h ? parseFloat(closest24h.te) : NaN;
                        if (!isNaN(te)) {
                            self.cards.push({
                                label: $.t('Temperature'),
                                value: te.toFixed(1) + ' ' + degreeSuffix,
                                delta: formatDelta(te, te24, degreeSuffix, 1),
                                deltaColor: deltaColor(te, te24)
                            });
                        }
                    }

                    // Humidity card
                    if (device.Humidity !== undefined) {
                        var hu = parseInt(device.Humidity, 10);
                        var hu24 = closest24h ? parseInt(closest24h.hu, 10) : NaN;
                        self.cards.push({
                            label: $.t('Humidity'),
                            value: hu + ' %',
                            delta: formatDelta(hu, hu24, '%', 0),
                            deltaColor: deltaColor(hu, hu24)
                        });
                    }

                    // Barometer card
                    if (device.Barometer !== undefined) {
                        var ba = parseFloat(device.Barometer);
                        var ba24 = closest24h ? parseFloat(closest24h.ba) : NaN;
                        self.cards.push({
                            label: $.t('Barometer'),
                            value: ba.toFixed(1) + ' hPa',
                            delta: formatDelta(ba, ba24, 'hPa', 1),
                            deltaColor: deltaColor(ba, ba24)
                        });
                    }

                    if (self.cards.length === 0) {
                        $element.hide();
                    }
                }

                // Rebuild when chart data refreshes (updates deltas)
                $scope.$watch(function () {
                    return self.logCtrl.dayGraphData;
                }, function (result) {
                    if (result && result.length >= 2) {
                        rebuildCards();
                    }
                });

                // Rebuild on live device update
                $scope.$on('device_update', function (event, updatedDevice) {
                    if (updatedDevice.idx === device.idx) {
                        device = updatedDevice;
                        self.device = updatedDevice;
                        rebuildCards();
                    }
                });
            };
        }
    });

    app.component('temperatureDiurnalChart', {
        require: {
            logCtrl: '^deviceTemperatureLog'
        },
        bindings: {
            device: '<'
        },
        template: customChartTemplate,
        controllerAs: 'vm',
        controller: function ($element, domoticzApi) {
            const self = this;
            self.chartTitle = $.t('Daily Temperature Cycle');

            self.$onInit = function () {
                domoticzApi.sendCommand('graph', {
                    sensor: 'temp', idx: self.device.idx, range: 'day'
                }).then(function (data) {
                    if (!data || !data.result || data.result.length < 10) {
                        $element.hide();
                        return;
                    }

                    var hasHum = self.device.Humidity !== undefined;
                    // Group by hour
                    var hourBuckets = {};
                    for (var h = 0; h < 24; h++) {
                        hourBuckets[h] = { temps: [], hums: [] };
                    }

                    data.result.forEach(function (item) {
                        var dateStr = item.d; // "YYYY-MM-DD HH:MM"
                        var hour = parseInt(dateStr.substring(11, 13), 10);
                        if (isNaN(hour)) return;
                        var te = parseFloat(item.te);
                        if (!isNaN(te)) hourBuckets[hour].temps.push(te);
                        if (hasHum && item.hu !== undefined) {
                            var hu = parseInt(item.hu, 10);
                            if (!isNaN(hu)) hourBuckets[hour].hums.push(hu);
                        }
                    });

                    var categories = [];
                    var tempAvg = [];
                    var tempMin = [];
                    var tempMax = [];
                    var humAvg = [];

                    for (var h = 0; h < 24; h++) {
                        categories.push(h + ':00');
                        var t = hourBuckets[h].temps;
                        if (t.length > 0) {
                            var sum = t.reduce(function (a, b) { return a + b; }, 0);
                            tempAvg.push(parseFloat((sum / t.length).toFixed(1)));
                            tempMin.push(parseFloat(Math.min.apply(null, t).toFixed(1)));
                            tempMax.push(parseFloat(Math.max.apply(null, t).toFixed(1)));
                        } else {
                            tempAvg.push(null);
                            tempMin.push(null);
                            tempMax.push(null);
                        }
                        var hu = hourBuckets[h].hums;
                        if (hu.length > 0) {
                            var hsum = hu.reduce(function (a, b) { return a + b; }, 0);
                            humAvg.push(Math.round(hsum / hu.length));
                        } else {
                            humAvg.push(null);
                        }
                    }

                    var yAxes = [{
                        title: { text: $.t('Temperature') + ' ' + degreeSuffix }
                    }];

                    var series = [
                        {
                            name: $.t('Temperature') + ' ' + $.t('Range'),
                            type: 'areasplinerange',
                            data: tempMin.map(function (min, i) { return [min, tempMax[i]]; }),
                            color: 'rgba(255,200,0,0.3)',
                            lineWidth: 0,
                            linkedTo: ':next',
                            tooltip: { valueSuffix: ' ' + degreeSuffix, valueDecimals: 1 },
                            zIndex: 0
                        },
                        {
                            name: $.t('Temperature') + ' ' + $.t('Average'),
                            type: 'spline',
                            data: tempAvg,
                            color: 'yellow',
                            lineWidth: 2,
                            marker: { enabled: true, radius: 3 },
                            tooltip: { valueSuffix: ' ' + degreeSuffix, valueDecimals: 1 },
                            zIndex: 2
                        }
                    ];

                    if (hasHum && humAvg.some(function (v) { return v !== null; })) {
                        yAxes.push({
                            title: { text: $.t('Humidity') + ' (%)' },
                            opposite: true
                        });
                        series.push({
                            name: $.t('Humidity') + ' ' + $.t('Average'),
                            type: 'spline',
                            data: humAvg,
                            color: 'limegreen',
                            yAxis: 1,
                            dashStyle: 'ShortDash',
                            lineWidth: 2,
                            marker: { enabled: true, radius: 3 },
                            tooltip: { valueSuffix: ' %', valueDecimals: 0 },
                            zIndex: 1
                        });
                    }

                    var chartElement = $element.find('.chartcontainer');
                    Highcharts.chart(chartElement[0], {
                        chart: { zoomType: 'x' },
                        title: { text: null },
                        xAxis: {
                            categories: categories,
                            crosshair: true,
                            title: { text: $.t('Hour of Day') }
                        },
                        yAxis: yAxes,
                        tooltip: { shared: true },
                        legend: { enabled: true },
                        series: series
                    });
                });
            };
        }
    });

    app.component('temperatureDistributionChart', {
        require: {
            logCtrl: '^deviceTemperatureLog'
        },
        bindings: {
            device: '<'
        },
        template: customChartTemplate,
        controllerAs: 'vm',
        controller: function ($element, domoticzApi) {
            const self = this;
            self.chartTitle = $.t('Temperature') + ' ' + $.t('Distribution');

            self.$onInit = function () {
                domoticzApi.sendCommand('graph', {
                    sensor: 'temp', idx: self.device.idx, range: 'day'
                }).then(function (data) {
                    if (!data || !data.result || data.result.length < 10) {
                        $element.hide();
                        return;
                    }

                    var temps = [];
                    data.result.forEach(function (item) {
                        var te = parseFloat(item.te);
                        if (!isNaN(te)) temps.push(te);
                    });

                    if (temps.length < 10) {
                        $element.hide();
                        return;
                    }

                    var minT = Math.floor(Math.min.apply(null, temps));
                    var maxT = Math.ceil(Math.max.apply(null, temps));
                    var range = maxT - minT;
                    var binSize = Math.max(1, Math.round(range / 15));
                    var binStart = minT;
                    var bins = [];
                    var categories = [];

                    while (binStart < maxT) {
                        var binEnd = binStart + binSize;
                        var count = 0;
                        temps.forEach(function (t) {
                            if (t >= binStart && t < binEnd) count++;
                        });
                        bins.push(count);
                        categories.push(binStart + '-' + binEnd + degreeSuffix);
                        binStart = binEnd;
                    }

                    // Color gradient: blue (cold) to red (hot)
                    var colors = bins.map(function (val, i) {
                        var ratio = i / Math.max(bins.length - 1, 1);
                        var r = Math.round(50 + ratio * 200);
                        var b = Math.round(200 - ratio * 180);
                        return 'rgba(' + r + ',80,' + b + ',0.8)';
                    });

                    var chartElement = $element.find('.chartcontainer');
                    Highcharts.chart(chartElement[0], {
                        chart: { type: 'column' },
                        title: { text: null },
                        xAxis: {
                            categories: categories,
                            crosshair: true,
                            title: { text: $.t('Temperature') + ' ' + degreeSuffix }
                        },
                        yAxis: {
                            title: { text: $.t('Readings') },
                            allowDecimals: false
                        },
                        tooltip: {
                            formatter: function () {
                                return this.x + '<br/>' +
                                    $.t('Readings') + ': <b>' + this.y + '</b>';
                            }
                        },
                        legend: { enabled: false },
                        plotOptions: {
                            column: {
                                colorByPoint: true,
                                colors: colors,
                                borderWidth: 0
                            }
                        },
                        series: [{
                            name: $.t('Readings'),
                            data: bins
                        }]
                    });
                });
            };
        }
    });

    app.component('temperatureComfortChart', {
        require: {
            logCtrl: '^deviceTemperatureLog'
        },
        bindings: {
            device: '<'
        },
        template: customChartTemplate,
        controllerAs: 'vm',
        controller: function ($element, domoticzApi) {
            const self = this;
            self.chartTitle = $.t('Comfort Zone');

            self.$onInit = function () {
                if (self.device.Humidity === undefined) {
                    $element.hide();
                    return;
                }

                domoticzApi.sendCommand('graph', {
                    sensor: 'temp', idx: self.device.idx, range: 'day'
                }).then(function (data) {
                    if (!data || !data.result || data.result.length === 0) {
                        $element.hide();
                        return;
                    }

                    var scatterData = [];
                    var n = data.result.length;

                    data.result.forEach(function (item, idx) {
                        var te = parseFloat(item.te);
                        var hu = item.hu !== undefined ? parseInt(item.hu, 10) : NaN;
                        if (isNaN(te) || isNaN(hu)) return;

                        // Convert to Celsius for comfort zone calculation if needed
                        var teC = ($.myglobals.tempsign === 'F') ? (te - 32) * 5.0 / 9.0 : te;
                        var inComfort = (teC >= 20 && teC <= 26 && hu >= 30 && hu <= 60);

                        scatterData.push({
                            x: te,
                            y: hu,
                            marker: {
                                fillColor: inComfort ? 'rgba(0,200,83,0.7)' : 'rgba(255,82,82,0.5)',
                                radius: 4
                            },
                            name: item.d
                        });
                    });

                    if (scatterData.length === 0) {
                        $element.hide();
                        return;
                    }

                    // Comfort zone polygon coordinates (in display units)
                    var comfortMinT = ($.myglobals.tempsign === 'F') ? 68 : 20;
                    var comfortMaxT = ($.myglobals.tempsign === 'F') ? 78.8 : 26;

                    var chartElement = $element.find('.chartcontainer');
                    Highcharts.chart(chartElement[0], {
                        chart: {
                            type: 'scatter',
                            zoomType: 'xy'
                        },
                        title: { text: null },
                        xAxis: {
                            title: { text: $.t('Temperature') + ' ' + degreeSuffix },
                            crosshair: true,
                            plotBands: [{
                                from: comfortMinT,
                                to: comfortMaxT,
                                color: 'rgba(0,200,83,0.08)',
                                label: {
                                    text: $.t('Comfortable'),
                                    style: { color: 'rgba(0,200,83,0.4)', fontSize: '10px' },
                                    align: 'center',
                                    verticalAlign: 'bottom'
                                }
                            }]
                        },
                        yAxis: {
                            title: { text: $.t('Humidity') + ' (%)' },
                            plotBands: [{
                                from: 30,
                                to: 60,
                                color: 'rgba(0,200,83,0.08)'
                            }]
                        },
                        tooltip: {
                            formatter: function () {
                                return this.point.name + '<br/>' +
                                    $.t('Temperature') + ': <b>' + this.x.toFixed(1) + ' ' + degreeSuffix + '</b><br/>' +
                                    $.t('Humidity') + ': <b>' + this.y + ' %</b>';
                            }
                        },
                        legend: { enabled: false },
                        plotOptions: {
                            scatter: {
                                marker: { symbol: 'circle' }
                            }
                        },
                        series: [{
                            name: $.t('Comfort Zone'),
                            data: scatterData
                        }]
                    });
                });
            };
        }
    });

    app.component('temperatureDewpointChart', {
        require: {
            logCtrl: '^deviceTemperatureLog'
        },
        bindings: {
            device: '<'
        },
        template: '<div id="dewpointchart" style="width:100%; height:300px; margin-top:10px;"></div>',
        controllerAs: 'vm',
        controller: function ($element, domoticzApi) {
            const self = this;

            self.$onInit = function () {
                if (self.device.Humidity === undefined) {
                    $element.hide();
                    return;
                }

                domoticzApi.sendCommand('graph', {
                    sensor: 'temp',
                    idx: self.device.idx,
                    range: 'day'
                }).then(function (data) {
                    if (!data || !data.result || data.result.length === 0) {
                        $element.hide();
                        return;
                    }

                    var dewPoints = [];
                    var temperatures = [];
                    data.result.forEach(function (item) {
                        var te = parseFloat(item.te);
                        var hu = parseInt(item.hu, 10);
                        if (isNaN(te) || isNaN(hu) || hu <= 0) return;

                        var ts = GetLocalDateTimeFromString(item.d);
                        // Magnus-Tetens formula
                        var a = 17.27, b = 237.7;
                        var alpha = (a * te) / (b + te) + Math.log(hu / 100.0);
                        var dp = (b * alpha) / (a - alpha);

                        if ($.myglobals.tempsign === 'F') {
                            dp = dp * 9.0 / 5.0 + 32;
                        }

                        dewPoints.push([ts, parseFloat(dp.toFixed(1))]);
                        temperatures.push([ts, te]);
                    });

                    if (dewPoints.length === 0) return;

                    Highcharts.chart($element[0].firstChild, {
                        chart: { type: 'spline', zoomType: 'x' },
                        title: { text: $.t('Dew Point') },
                        credits: { enabled: false },
                        xAxis: {
                            type: 'datetime'
                        },
                        yAxis: {
                            title: { text: $.t('Degrees') + ' ' + degreeSuffix }
                        },
                        tooltip: {
                            shared: true,
                            crosshairs: true,
                            xDateFormat: '%A, %B %e, %H:%M'
                        },
                        plotOptions: {
                            spline: {
                                lineWidth: 2,
                                states: { hover: { lineWidth: 3 } },
                                marker: { enabled: false }
                            }
                        },
                        series: [
                            {
                                name: $.t('Dew Point'),
                                data: dewPoints,
                                color: '#2b908f',
                                tooltip: { valueSuffix: ' ' + degreeSuffix, valueDecimals: 1 }
                            },
                            {
                                name: $.t('Temperature'),
                                data: temperatures,
                                color: 'yellow',
                                tooltip: { valueSuffix: ' ' + degreeSuffix, valueDecimals: 1 }
                            }
                        ]
                    });
                });
            };
        }
    });

    app.component('temperatureVsHumidityChart', {
        require: {
            logCtrl: '^deviceTemperatureLog'
        },
        bindings: {
            device: '<'
        },
        template: '<div style="width:100%; height:300px; margin-top:10px;"></div>',
        controllerAs: 'vm',
        controller: function ($element, domoticzApi) {
            const self = this;

            self.$onInit = function () {
                if (self.device.Humidity === undefined) {
                    $element.hide();
                    return;
                }

                domoticzApi.sendCommand('graph', {
                    sensor: 'temp',
                    idx: self.device.idx,
                    range: 'day'
                }).then(function (data) {
                    if (!data || !data.result || data.result.length === 0) {
                        $element.hide();
                        return;
                    }

                    var n = data.result.length;
                    var scatterData = [];

                    data.result.forEach(function (item, idx) {
                        var te = parseFloat(item.te);
                        var hu = item.hu !== undefined ? parseInt(item.hu, 10) : NaN;
                        if (isNaN(te) || isNaN(hu)) return;

                        var ratio = idx / Math.max(n - 1, 1);
                        var lightness = Math.round(80 - ratio * 55);
                        scatterData.push({
                            x: te,
                            y: hu,
                            color: 'hsl(30,' + Math.round(50 + ratio * 30) + '%,' + lightness + '%)',
                            name: item.d
                        });
                    });

                    if (scatterData.length === 0) return;

                    Highcharts.chart($element[0].firstChild, {
                        chart: {
                            type: 'scatter',
                            zoomType: 'xy'
                        },
                        title: {
                            text: $.t('Temperature') + ' vs ' + $.t('Humidity')
                        },
                        credits: { enabled: false },
                        xAxis: {
                            title: { text: $.t('Temperature') + ' ' + degreeSuffix },
                            crosshair: true
                        },
                        yAxis: {
                            title: { text: $.t('Humidity') + ' (%)' }
                        },
                        tooltip: {
                            formatter: function () {
                                return this.point.name + '<br/>' +
                                    $.t('Temperature') + ': <b>' + this.x + ' ' + degreeSuffix + '</b><br/>' +
                                    $.t('Humidity') + ': <b>' + this.y + ' %</b>';
                            }
                        },
                        legend: {
                            enabled: false
                        },
                        plotOptions: {
                            scatter: {
                                marker: {
                                    radius: 4,
                                    symbol: 'circle'
                                }
                            }
                        },
                        series: [
                            {
                                name: $.t('Temperature vs Humidity'),
                                data: scatterData,
                                colorByPoint: true
                            }
                        ]
                    });
                });
            };
        }
    });

    function chartParams(domoticzGlobals, ctrl, isShortLogChart, timestampFromDataItem, seriesSuppliers) {
        return {
            highchartTemplate: {
                chart: {
                    type: ctrl.device.Type === 'Setpoint' ? 'line' : undefined
                }
            },
            ctrl: ctrl,
            range: ctrl.range,
            device: ctrl.device,
            sensorType: ctrl.sensorType,
            chartName:  (ctrl.device.Type === 'Humidity') ? $.t('Humidity') : $.t('Temperature'),
            autoRefreshIsEnabled: function() { return ctrl.logCtrl.autoRefresh; },
            dataSupplier: {
                yAxes:
                    [
                        {
							visible: (ctrl.device.Type !== 'Humidity'),
                            title: {
                                text: $.t('Degrees') + ' \u00B0' + ctrl.degreeType
                            },
                            labels: {
                                formatter: function () {
                                    return this.value + '\u00B0 ' + ctrl.degreeType;
                                }
                            }
                        },
                        {
                            title: {
                                text: $.t('Humidity') + ' %'
                            },
                            labels: {
                                formatter: function () {
                                    return this.value + '%';
                                }
                            },
                            showEmpty: false,
                            allowDecimals: false,	//no need to show scale with decimals
                            ceiling: 100,			//max limit for auto zoom, bug in highcharts makes this sometimes not considered.
                            floor: 0,				//min limit for auto zoom
                            minRange: 10,			//min range for auto zoom
                            opposite: (ctrl.device.Type !== 'Humidity')
                        }
                    ],
                timestampFromDataItem: timestampFromDataItem,
                isShortLogChart: isShortLogChart,
                seriesSuppliers: seriesSuppliers
            }
        };
    }
});
