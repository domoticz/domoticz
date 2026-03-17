define(['app', 'lodash', 'RefreshingChart', 'DataLoader', 'ChartLoader', 'log/Chart', 'log/CounterLogParams',
        'log/components/CounterStatChart',
        'log/components/CounterCalendarHeatmapChart',
        'log/components/CounterCumulativeChart',
        'log/components/CounterWeeklyHeatmapChart'],
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

        app.component('counterCurrentConditions', {
            require: {
                logCtrl: '^deviceCounterLog'
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
                            '<div ng-if="!card.lines" style="font-size:1.8em; font-weight:bold;">{{card.value}}</div>' +
                            '<div ng-if="card.lines" ng-repeat="line in card.lines" ' +
                                'style="font-size:1.3em; font-weight:bold; white-space:nowrap;" title="{{line.tooltip}}">' +
                                '<i class="fa {{line.icon}}" style="font-size:0.8em;" ng-style="{color: line.iconColor}"></i> {{line.value}}' +
                            '</div>' +
                        '</div>' +
                    '</div>' +
                '</div>',
            controllerAs: 'vm',
            controller: function ($element, $scope, chart) {
                const self = this;
                self.cards = [];

                self.$onInit = function () {
                    var device = self.device;
                    var isP1 = (device.Type === 'P1 Smart Meter' && device.SubType === 'Energy');

                    // Determine the correct unit based on device type, matching chart axes
                    function getChartUnit() {
                        switch (device.SwitchTypeVal) {
                            case chart.deviceTypes.EnergyUsed:
                            case chart.deviceTypes.EnergyGenerated:
                                return chart.valueUnits.energy(chart.valueMultipliers.m1000);
                            case chart.deviceTypes.Gas:
                                return chart.valueUnits.gas(chart.valueMultipliers.m1);
                            case chart.deviceTypes.Water:
                                return chart.valueUnits.water(chart.valueMultipliers.m1);
                            default:
                                return device.getUnit();
                        }
                    }
                    var chartUnit = isP1 ? 'kWh' : getChartUnit();

                    // Detect if P1 has return/delivery capability
                    var hasReturn = isP1 && device.CounterDeliv !== undefined && device.CounterDeliv !== null
                        && parseFloat(device.CounterDeliv) > 0;

                    function buildDeviceCards() {
                        // Current Usage (e.g. "11325 Watt")
                        if (device.Usage !== undefined && device.Usage !== null) {
                            var usage = String(device.Usage);
                            if (usage !== '' && usage !== '0' && usage !== '0 Watt') {
                                self.cards.push({
                                    label: $.t('Usage'),
                                    value: usage
                                });
                            }
                        }

                        // Current Delivery (P1 Energy, only if device has return)
                        if (hasReturn && device.UsageDeliv !== undefined && device.UsageDeliv !== null) {
                            var usageDeliv = String(device.UsageDeliv);
                            if (usageDeliv !== '' && usageDeliv !== '0' && usageDeliv !== '0 Watt') {
                                self.cards.push({
                                    label: $.t('Delivery'),
                                    value: usageDeliv
                                });
                            }
                        }

                        // Today card
                        if (device.CounterToday !== undefined && device.CounterToday !== null && String(device.CounterToday) !== '') {
                            var todayValue = String(device.CounterToday);
                            // Water: backend sends m3, convert to chart unit (Liter) to match charts
                            if (device.SwitchTypeVal === chart.deviceTypes.Water && todayValue.indexOf('Liter') === -1) {
                                var numVal = parseFloat(todayValue) * 1000;
                                todayValue = Math.round(numVal) + ' ' + chartUnit;
                            }
                            if (hasReturn && device.CounterDelivToday !== undefined && device.CounterDelivToday !== null
                                    && parseFloat(device.CounterDelivToday) > 0) {
                                self.cards.push({ label: $.t('Today'), lines: [
                                    { icon: 'fa-arrow-down', iconColor: '#ff6b6b', value: todayValue, tooltip: $.t('Usage') },
                                    { icon: 'fa-arrow-up', iconColor: '#4ecdc4', value: String(device.CounterDelivToday), tooltip: $.t('Return') }
                                ]});
                            } else {
                                self.cards.push({ label: $.t('Today'), value: todayValue });
                            }
                        }
                    }

                    buildDeviceCards();

                    // Update device-property cards on device_update
                    $scope.$on('device_update', function (event, updatedDevice) {
                        if (updatedDevice.idx === device.idx) {
                            self.device = updatedDevice;
                            device = updatedDevice;
                            hasReturn = isP1 && device.CounterDeliv !== undefined && device.CounterDeliv !== null
                                && parseFloat(device.CounterDeliv) > 0;
                            var yearCards = self.cards.splice(self._deviceCardCount);
                            self.cards.length = 0;
                            buildDeviceCards();
                            self._deviceCardCount = self.cards.length;
                            self.cards.push.apply(self.cards, yearCards);
                        }
                    });

                    self._deviceCardCount = self.cards.length;

                    // Watch for year graph data shared by counterYearChart
                    $scope.$watch(function () {
                        return self.logCtrl.yearGraphData;
                    }, function (result) {
                        if (!result || result.length === 0) {
                            return;
                        }

                        // Clear year cards, keep device cards
                        self.cards.splice(self._deviceCardCount);

                        var now = new Date();
                        var currentYear = String(now.getFullYear());
                        var currentMonth = currentYear + '-' + ('0' + (now.getMonth() + 1)).slice(-2);
                        if (isP1) {
                            var monthUsage = 0, monthReturn = 0;
                            var yearUsage = 0, yearReturn = 0;

                            result.forEach(function (item) {
                                var v = parseFloat(item.v1 || 0) + parseFloat(item.v2 || 0);
                                var r = parseFloat(item.r1 || 0) + parseFloat(item.r2 || 0);
                                if (item.d.substring(0, 7) === currentMonth) {
                                    monthUsage += v;
                                    monthReturn += r;
                                }
                                if (item.d.substring(0, 4) === currentYear) {
                                    yearUsage += v;
                                    yearReturn += r;
                                }
                            });

                            if (hasReturn) {
                                // This Month with usage + return lines
                                self.cards.push({ label: $.t('This Month'), lines: [
                                    { icon: 'fa-arrow-down', iconColor: '#ff6b6b', value: monthUsage.toFixed(3) + ' ' + chartUnit, tooltip: $.t('Usage') },
                                    { icon: 'fa-arrow-up', iconColor: '#4ecdc4', value: monthReturn.toFixed(3) + ' ' + chartUnit, tooltip: $.t('Return') }
                                ]});
                                // This Year with usage + return lines
                                self.cards.push({ label: $.t('This Year'), lines: [
                                    { icon: 'fa-arrow-down', iconColor: '#ff6b6b', value: yearUsage.toFixed(3) + ' ' + chartUnit, tooltip: $.t('Usage') },
                                    { icon: 'fa-arrow-up', iconColor: '#4ecdc4', value: yearReturn.toFixed(3) + ' ' + chartUnit, tooltip: $.t('Return') }
                                ]});
                            } else {
                                // No return: simple values, no icons
                                self.cards.push({ label: $.t('This Month'), value: monthUsage.toFixed(3) + ' ' + chartUnit });
                                self.cards.push({ label: $.t('This Year'), value: yearUsage.toFixed(3) + ' ' + chartUnit });
                            }
                        } else {
                            // Non-P1: single value for month and year
                            // Water values from API are in m³, charts multiply by 1000 to show liters
                            var valueFactor = (device.SwitchTypeVal === chart.deviceTypes.Water) ? 1000 : 1;
                            var decimals = (device.SwitchTypeVal === chart.deviceTypes.Water) ? 0 : 3;
                            var monthTotal = 0, yearTotal = 0;
                            result.forEach(function (item) {
                                var v = parseFloat(item.v || 0) * valueFactor;
                                if (item.d.substring(0, 7) === currentMonth) {
                                    monthTotal += v;
                                }
                                if (item.d.substring(0, 4) === currentYear) {
                                    yearTotal += v;
                                }
                            });

                            self.cards.push({
                                label: $.t('This Month'),
                                value: monthTotal.toFixed(decimals) + ' ' + chartUnit
                            });
                            self.cards.push({
                                label: $.t('This Year'),
                                value: yearTotal.toFixed(decimals) + ' ' + chartUnit
                            });
                        }

                        if (self.cards.length === 0) {
                            $element.hide();
                        }
                    });
                };
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

        app.component('counterHourChart', {
            require: {
                logCtrl: '^deviceCounterLog'
            },
            bindings: {
                device: '<'
            },
            templateUrl: 'app/log/chart-hour.html',
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi, chart, counterLogParams, counterLogSubtypeRegistry) {
                const self = this;
                self.range = 'hour';
                self.resolution = 60;
                self.priceResolution = ($.myglobals && $.myglobals.PriceResolution) || 60;

                self.$onInit = function () {
                    const subtype = counterLogSubtypeRegistry.get(self.logCtrl.subtype);
                    self.chart = new RefreshingChart(
                        chart.baseParams($),
                        chart.angularParams($location, $route, $scope, $timeout, $element),
                        chart.domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        counterLogParams.chartParamsHour(domoticzGlobals, self,
                            subtype.chartParamsHourTemplate,
                            {
                                isShortLogChart: false,
                                yAxes: subtype.yAxesHour(self.device.SwitchTypeVal),
                                timestampFromDataItem: function (dataItem, yearOffset = 0) {
                                    return GetLocalDateTimeFromString(dataItem.d, yearOffset);
                                },
                                preprocessData: subtype.preprocessHourData,
                                preprocessDataItems: subtype.preprocessHourDataItems
                            },
                            subtype.hourSeriesSuppliers(self.device.SwitchTypeVal, self.device.Divider)
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
                    var origPreprocessData = subtype.preprocessMonthYearData;
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
                                preprocessData: function (data) {
                                    // Share year data with conditions component
                                    self.logCtrl.yearGraphData = data.result;
                                    if (origPreprocessData) {
                                        origPreprocessData.call(this, data);
                                    }
                                },
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
