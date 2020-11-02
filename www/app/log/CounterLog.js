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
                                yAxes: subtype.yAxesDay,
                                timestampFromDataItem: function (dataItem, yearOffset = 0) {
                                    return GetLocalDateTimeFromString(dataItem.d, yearOffset);
                                },
                                extendDataRequest: subtype.extendDataRequestDay
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
                                yAxes: subtype.yAxesWeek,
                                timestampFromDataItem: function (dataItem, yearOffset = 0) {
                                    return GetLocalDateFromString(dataItem.d, yearOffset);
                                }
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
                                yAxes: subtype.yAxesMonthYear,
                                timestampFromDataItem: function (dataItem, yearOffset = 0) {
                                    return GetLocalDateFromString(dataItem.d, yearOffset);
                                }
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
                                yAxes: subtype.yAxesMonthYear,
                                timestampFromDataItem: function (dataItem, yearOffset = 0) {
                                    return GetLocalDateFromString(dataItem.d, yearOffset);
                                }
                            },
                            subtype.monthYearSeriesSuppliers(self.device.SwitchTypeVal)
                        )
                    );
                }
            }
        });
    }
);
