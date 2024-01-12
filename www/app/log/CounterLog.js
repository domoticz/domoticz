define(['app', 'lodash', 'RefreshingChart', 'DataLoader', 'ChartLoader', 'log/Chart', 'log/CounterLogParams'],
    function (app, _, RefreshingChart, DataLoader, ChartLoader) {

        app.component('deviceCounterLog', {
            bindings: {
                device: '<',
                subtype: '<'
            },
            templateUrl: 'app/log/CounterLog.html',
            controllerAs: '$ctrl',
            controller: function () {
                const $ctrl = this;
                $ctrl.autoRefresh = true;
            }
        });

        app.component('counterDayChart', {
            require: {
                logCtrl: '^deviceCounterLog'
            },
            bindings: {
                device: '<'
            },
            templateUrl: 'app/log/chart-day.html',
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi, chart, counterLogParams, counterLogSubtypeRegistry) {
                const self = this;
                self.range = 'day';

                self.$onInit = function () {
                    const subtype = counterLogSubtypeRegistry.get(self.logCtrl.subtype);
                    self.chart = new RefreshingChart(
                        chart.baseParams($),
                        chart.angularParams($location, $route, $scope, $timeout, $element),
                        chart.domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        counterLogParams.chartParamsDay(domoticzGlobals, self,
                            subtype.chartParamsDayTemplate,
                            {
                                isShortLogChart: true,
                                yAxes: subtype.yAxesDay(self.device.SwitchTypeVal),
                                timestampFromDataItem: function (dataItem, yearOffset = 0) {
                                    return GetLocalDateTimeFromString(dataItem.d, yearOffset);
                                },
                                extendDataRequest: subtype.extendDataRequestDay,
                                preprocessData: subtype.preprocessDayData,
                                preprocessDataItems: subtype.preprocessDayDataItems
                            },
                            subtype.daySeriesSuppliers(self.device.SwitchTypeVal, self.device.Divider)
                        )
                    );
                }
            }
        });

        app.component('counterWeekChart', {
            require: {
                logCtrl: '^deviceCounterLog'
            },
            bindings: {
                device: '<'
            },
            templateUrl: 'app/log/chart-week.html',
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi, chart, counterLogParams, counterLogSubtypeRegistry) {
                const self = this;
                self.range = 'week';

                self.$onInit = function () {
                    const subtype = counterLogSubtypeRegistry.get(self.logCtrl.subtype);
                    self.chart = new RefreshingChart(
                        chart.baseParams($),
                        chart.angularParams($location, $route, $scope, $timeout, $element),
                        chart.domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        counterLogParams.chartParamsWeek(domoticzGlobals, self,
                            subtype.chartParamsWeekTemplate,
                            {
                                isShortLogChart: false,
                                yAxes: subtype.yAxesWeek(self.device.SwitchTypeVal),
                                timestampFromDataItem: function (dataItem, yearOffset = 0) {
                                    return GetLocalDateFromString(dataItem.d, yearOffset);
                                },
                                preprocessData: subtype.preprocessWeekData,
                                preprocessDataItems: subtype.preprocessWeekDataItems
                            },
                            subtype.weekSeriesSuppliers(self.device.SwitchTypeVal, self.device.Divider)
                        )
                    );
                }
            }
        });

        app.component('counterMonthChart', {
            require: {
                logCtrl: '^deviceCounterLog'
            },
            bindings: {
                device: '<'
            },
            templateUrl: 'app/log/chart-month.html',
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi, chart, counterLogParams, counterLogSubtypeRegistry) {
                const self = this;
                self.range = 'month';

                self.$onInit = function () {
                    const subtype = counterLogSubtypeRegistry.get(self.logCtrl.subtype);
                    self.chart = new RefreshingChart(
                        chart.baseParams($),
                        chart.angularParams($location, $route, $scope, $timeout, $element),
                        chart.domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        counterLogParams.chartParamsMonthYear(domoticzGlobals, self,
                            subtype.chartParamsMonthYearTemplate,
                            {
                                isShortLogChart: false,
                                yAxes: subtype.yAxesMonthYear(self.device.SwitchTypeVal),
                                timestampFromDataItem: function (dataItem, yearOffset = 0) {
                                    return GetLocalDateFromString(dataItem.d, yearOffset);
                                },
                                preprocessData: subtype.preprocessMonthYearData,
                                preprocessDataItems: subtype.preprocessMonthYearDataItems
                            },
                            subtype.monthYearSeriesSuppliers(self.device.SwitchTypeVal, self.device.Divider)
                        )
                    );
                }
            }
        });

        app.component('counterYearChart', {
            require: {
                logCtrl: '^deviceCounterLog'
            },
            bindings: {
                device: '<'
            },
            templateUrl: 'app/log/chart-year.html',
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi, chart, counterLogParams, counterLogSubtypeRegistry) {
                const self = this;
                self.range = 'year';

                self.$onInit = function () {
                    const subtype = counterLogSubtypeRegistry.get(self.logCtrl.subtype);
                    self.chart = new RefreshingChart(
                        chart.baseParams($),
                        chart.angularParams($location, $route, $scope, $timeout, $element),
                        chart.domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        counterLogParams.chartParamsMonthYear(domoticzGlobals, self,
                            subtype.chartParamsMonthYearTemplate,
                            {
                                isShortLogChart: false,
                                yAxes: subtype.yAxesMonthYear(self.device.SwitchTypeVal),
                                timestampFromDataItem: function (dataItem, yearOffset = 0) {
                                    return GetLocalDateFromString(dataItem.d, yearOffset);
                                },
                                preprocessData: subtype.preprocessMonthYearData,
                                preprocessDataItems: subtype.preprocessMonthYearDataItems
                            },
                            subtype.monthYearSeriesSuppliers(self.device.SwitchTypeVal, self.device.Divider)
                        )
                    );
                }
            }
        });

        app.component('counterCompareChart', {
            require: {
                logCtrl: '^deviceCounterLog'
            },
            bindings: {
                device: '<'
            },
            templateUrl: 'app/log/chart-compare.html',
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi, chart, counterLogParams, counterLogSubtypeRegistry) {
                const self = this;
                self.groupingBy = 'month';

                self.$onInit = function () {
                    const subtype = counterLogSubtypeRegistry.get(self.logCtrl.subtype);
                    self.chart = new RefreshingChart(
                        chart.baseParams($),
                        chart.angularParams($location, $route, $scope, $timeout, $element),
                        chart.domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        counterLogParams.chartParamsCompare(domoticzGlobals, self,
                            subtype.chartParamsCompareTemplate(self),
                            {
                                isShortLogChart: false,
                                yAxes: subtype.yAxesCompare(self.device.SwitchTypeVal),
                                extendDataRequest: function (dataRequest) {
                                    dataRequest['groupby'] = self.groupingBy;
                                    return subtype.extendDataRequestCompare.call(self, dataRequest);
                                },
                                preprocessData: function (data) {
									this.deviceCounterName = self.device.ValueQuantity;
									
                                    if (subtype.preprocessCompareData !== undefined) {
                                        subtype.preprocessCompareData.call(self, data);
                                    }
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
                                preprocessDataItems: subtype.preprocessCompareDataItems
                            },
                            subtype.compareSeriesSuppliers(self)
                        ),
                        new DataLoader(),
                        new ChartLoader($location),
                        null
                    );
                }
            }
        });
    }
);
