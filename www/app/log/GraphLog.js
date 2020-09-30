define(['app', 'RefreshingChart'],
    function (app, RefreshingChart) {

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

        app.component('refreshingDayChart', {
            require: {
                logCtrl: '^deviceGraphLog'
            },
            bindings: {
                device: '<',
                deviceType: '<',
                degreeType: '<'
            },
            template: '<div></div>',
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi) {

                const self = this;
                self.range = 'day';

                self.$onInit = function () {
                    new RefreshingChart(
                        baseParams($),
                        angularParams($location, $route, $scope, $element),
                        domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        chartParams(
                            self.range,
                            self.device,
                            domoticzGlobals.Get5MinuteHistoryDaysGraphTitle(),
                            function() { return self.logCtrl.autoRefresh; },
                            {
                                valueAxes: function() {
                                    return [{
                                        title: {
                                            text: domoticzGlobals.axisTitleForDevice(self.device)
                                        },
                                        labels: {
                                            formatter: function () {
                                                const value = self.device.getUnit() === 'vM' ? Highcharts.numberFormat(this.value, 0) : this.value;
                                                return value + ' ' + self.device.getUnit();
                                            }
                                        }
                                    }];
                                },
                                dateFromDataItem: function(dataItem) {
                                    return GetLocalDateTimeFromString(dataItem.d);
                                },
                                dataPointIsShort: function() {
                                    return true;
                                },
                                seriesSuppliers: [
                                    {
                                        showInLegend: false,
                                        name: domoticzGlobals.sensorNameForDevice(self.device),
                                        colorIndex: 0,
                                        valueKey: domoticzGlobals.valueKeyForDevice(self.device),
                                        valueFromDataItem: function(dataItem) {
                                            return dataItem[this.valueKey];
                                        }
                                    }
                                ]
                            })
                    );
                }
            }
        });

        app.component('refreshingMonthChart', {
            require: {
                logCtrl: '^deviceGraphLog'
            },
            bindings: {
                device: '<',
                deviceType: '<',
                degreeType: '<',
                chartTitle: '@'
            },
            template: '<div></div>',
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $element, domoticzDataPointApi, domoticzApi, domoticzGlobals) {

                const self = this;
                self.range = 'month';

                self.$onInit = function () {
                    new RefreshingChart(
                        baseParams($),
                        angularParams($location, $route, $scope, $element),
                        domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        chartParams(
                            self.range,
                            self.device,
                            $.t(self.chartTitle),
                            function() { return self.logCtrl.autoRefresh; },
                            {
                                valueAxes: function() {
                                    return [{
                                        title: {
                                            text: domoticzGlobals.axisTitleForDevice(self.device)
                                        },
                                        labels: {
                                            formatter: function () {
                                                const value = self.device.getUnit() === 'vM' ? Highcharts.numberFormat(this.value, 0) : this.value;
                                                return value + ' ' + self.device.getUnit();
                                            }
                                        }
                                    }];
                                },
                                dateFromDataItem: function(dataItem) {
                                    return GetLocalDateFromString(dataItem.d);
                                },
                                dataPointIsShort: function() {
                                    return false;
                                },
                                seriesSuppliers: [
                                    {
                                        colorIndex: 3,
                                        name: 'min',
                                        valueKey: domoticzGlobals.valueKeyForDevice(self.device),
                                        valueFromDataItem: function(dataItem) {
                                            return dataItem[this.valueKey + '_min'];
                                        }
                                    },
                                    {
                                        colorIndex: 2,
                                        name: 'max',
                                        valueKey: domoticzGlobals.valueKeyForDevice(self.device),
                                        valueFromDataItem: function(dataItem) {
                                            return dataItem[this.valueKey + '_max'];
                                        }
                                    },
                                    {
                                        colorIndex: 0,
                                        name: 'avg',
                                        valueKey: domoticzGlobals.valueKeyForDevice(self.device),
                                        valueFromDataItem: function(dataItem) {
                                            return dataItem[this.valueKey + '_avg'];
                                        }
                                    }
                                ]
                            })
                    );
                }
            }
        });

        app.component('refreshingYearChart', {
            require: {
                logCtrl: '^deviceGraphLog'
            },
            bindings: {
                device: '<',
                deviceType: '<',
                degreeType: '<',
                chartTitle: '@'
            },
            template: '<div></div>',
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $element, domoticzDataPointApi, domoticzApi, domoticzGlobals) {

                const self = this;
                self.range = 'year';

                self.$onInit = function () {
                    new RefreshingChart(
                        baseParams($),
                        angularParams($location, $route, $scope, $element),
                        domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        chartParams(
                            self.range,
                            self.device,
                            $.t(self.chartTitle),
                            function() { return self.logCtrl.autoRefresh; },
                            {
                                valueAxes: function() {
                                    return [{
                                        title: {
                                            text: domoticzGlobals.axisTitleForDevice(self.device)
                                        },
                                        labels: {
                                            formatter: function () {
                                                const value = self.device.getUnit() === 'vM' ? Highcharts.numberFormat(this.value, 0) : this.value;
                                                return value + ' ' + self.device.getUnit();
                                            }
                                        }
                                    }];
                                },
                                dateFromDataItem: function(dataItem) {
                                    return GetLocalDateFromString(dataItem.d);
                                },
                                dataPointIsShort: function() {
                                    return false;
                                },
                                seriesSuppliers: [
                                    {
                                        colorIndex: 3,
                                        name: 'min',
                                        valueKey: domoticzGlobals.valueKeyForDevice(self.device),
                                        valueFromDataItem: function(dataItem) {
                                            return dataItem[this.valueKey + '_min'];
                                        }
                                    },
                                    {
                                        colorIndex: 2,
                                        name: 'max',
                                        valueKey: domoticzGlobals.valueKeyForDevice(self.device),
                                        valueFromDataItem: function(dataItem) {
                                            return dataItem[this.valueKey + '_max'];
                                        }
                                    },
                                    {
                                        colorIndex: 0,
                                        name: 'avg',
                                        valueKey: domoticzGlobals.valueKeyForDevice(self.device),
                                        valueFromDataItem: function(dataItem) {
                                            return dataItem[this.valueKey + '_avg'];
                                        }
                                    }
                                ]
                            })
                    );
                }
            }
        });

        function baseParams(jquery) {
            return {
                jquery: jquery
            };
        }
        function angularParams(location, route, scope, element) {
            return {
                location: location,
                route: route,
                scope: scope,
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
        function chartParams(range, device, chartTitle, autoRefreshIsEnabled, dataSupplier) {
            return {
                range: range,
                device: device,
                chartTitle: chartTitle,
                autoRefreshIsEnabled: autoRefreshIsEnabled,
                dataSupplier: dataSupplier
            };
        }
    }
);
