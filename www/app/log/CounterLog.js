define(['app', 'lodash', 'RefreshingChart', 'log/Chart', 'log/CounterLogParams'],
    function (app, _, RefreshingChart) {

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
                                preprocessData: subtype.preprocessData,
                                preprocessDataItems: subtype.preprocessDataItems
                            },
                            subtype.daySeriesSuppliers(self.device.SwitchTypeVal)
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
                                preprocessData: subtype.preprocessData,
                                preprocessDataItems: subtype.preprocessDataItems
                            },
                            subtype.weekSeriesSuppliers(self.device.SwitchTypeVal)
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
                                preprocessData: subtype.preprocessData,
                                preprocessDataItems: subtype.preprocessDataItems
                            },
                            subtype.monthYearSeriesSuppliers(self.device.SwitchTypeVal)
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
                                preprocessData: subtype.preprocessData,
                                preprocessDataItems: subtype.preprocessDataItems
                            },
                            subtype.monthYearSeriesSuppliers(self.device.SwitchTypeVal)
                        )
                    );
                }
            }
        });

        app.component('counterYearCompareChart', {
            require: {
                logCtrl: '^deviceCounterLog'
            },
            bindings: {
                device: '<'
            },
            templateUrl: 'app/log/chart-year-compare.html',
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
                        counterLogParams.chartParamsCompare(domoticzGlobals, self,
                            subtype.chartParamsCompareTemplate,
                            {
                                isShortLogChart: false,
                                yAxes: subtype.yAxesCompare(self.device.SwitchTypeVal),
                                extendDataRequest: function (dataRequest) {
                                    dataRequest['granularity'] = 'year';
                                    return dataRequest;
                                },
                                preprocessData: function (data) {
                                    if (subtype.preprocessData !== undefined) {
                                        subtype.preprocessData.call(this, data);
                                    }
                                    this.firstYear = data.firstYear;
                                },
                                preprocessDataItems: subtype.preprocessDataItems
                            },
                            subtype.compareSeriesSuppliers(self.device.SwitchTypeVal)
                        )
                    );
                }
            }
        });
    }
);
