define(['app'], function (app) {
    app.factory('DeviceTemperatureReportData', function (domoticzApi) {
        return {
            fetch: fetch
        };

        function fetch(deviceIdx, year, month) {
            return domoticzApi.sendRequest({
                type: 'graph',
                sensor: 'temp',
                range: 'year',
                idx: deviceIdx,
                actyear: year,
                actmonth: month
            }).then(function (response) {
                if (!response.result || !response.result.length) {
                    return null;
                }

                var isHumidityAvailable = (typeof response.result[0].hu !== 'undefined');
                var isOnlyHumidity = ((typeof response.result[0].ta === 'undefined') && isHumidityAvailable);

                var data = getGroupedData(response.result, isOnlyHumidity);
                var source = month
                    ? data.years[year].months.find(function (item) {
                        return item.date === month;
                    })
                    : data.years[year];

                if (!source) {
                    return null;
                }

                return {
                    isOnlyHumidity: isOnlyHumidity,
                    isHumidityAvailable: isHumidityAvailable,
                    min: source.min,
                    minDate: source.minDate,
                    max: source.max,
                    maxDate: source.maxDate,
                    degreeDays: source.degreeDays,
                    items: month ? source.days : source.months
                };
            });
        }

        function getGroupedData(data, isOnlyHumidity) {
            var result = {
                years: {}
            };

            data.forEach(function (item) {
                var month = parseInt(item.d.substring(5, 7), 10);
                var year = parseInt(item.d.substring(0, 4), 10);
                var day = parseInt(item.d.substring(8, 10), 10);

                if (!result.years[year]) {
                    result.years[year] = {
                        months: {}
                    }
                };

                if (!result.years[year].months[month]) {
                    result.years[year].months[month] = {
                        date: month,
                        days: {}
                    }
                }

                result.years[year].months[month].days[day] = !isOnlyHumidity
                    ? {
                        date: item.d,
                        humidity: parseFloat(item.hu) || undefined,
                        min: parseFloat(item.tm),
                        max: parseFloat(item.te),
                        avg: parseFloat(item.ta)
                    }
                    : {
                        date: item.d,
                        min: parseFloat(item.hu),
                        max: parseFloat(item.hu),
                        avg: parseFloat(item.hu)
                    }
            });

            Object.keys(result.years).forEach(function (year) {
                var yearsData = result.years[year];
                yearsData.months = Object.values(yearsData.months);

                yearsData.months.sort(function (a, b) {
                    return a.date < b.date ? -1 : 1;
                });

                yearsData.months.forEach(function (month) {
                    month.days = Object.values(month.days);

                    month.days.sort(function (a, b) {
                        return a.date < b.date ? -1 : 1;
                    });

                    month.days = addTrendData(month.days);

                    var stats = month.days.reduce(function (acc, item) {
                        if (!acc.min || acc.min.value > item.min) {
                            acc.min = {
                                value: item.min,
                                date: item.date
                            };
                        }

                        if (!acc.max || acc.max.value < item.max) {
                            acc.max = {
                                value: item.max,
                                date: item.date
                            };
                        }

                        if (item.avg < $.myglobals.DegreeDaysBaseTemperature) {
                            acc.degreeDays = (acc.degreeDays || 0) + ($.myglobals.DegreeDaysBaseTemperature - item.avg)
                        }

                        acc.total = (acc.total || 0) + item.avg;
                        acc.totalHumidity = (acc.totalHumidity || 0) + item.humidity;

                        return acc;
                    }, {});

                    month.min = stats.min.value;
                    month.minDate = stats.min.date;
                    month.max = stats.max.value;
                    month.maxDate = stats.max.date;
                    month.avg = (stats.total / month.days.length);
                    month.degreeDays = stats.degreeDays || 0;
                    month.humidity = stats.totalHumidity
                        ? (stats.totalHumidity / month.days.length)
                        : undefined;
                });

                yearsData.months = addTrendData(yearsData.months);

                var stats = getGroupStats(yearsData.months);

                yearsData.min = stats.min.value;
                yearsData.minDate = stats.min.date;
                yearsData.max = stats.max.value;
                yearsData.maxDate = stats.max.date;
                yearsData.degreeDays = stats.degreeDays || 0;
                yearsData.avg = (stats.total / yearsData.months.length);
            });

            return result;
        }

        function getGroupStats(values) {
            return values.reduce(function (acc, item) {
                if (!acc.min || acc.min.value > item.min) {
                    acc.min = {
                        value: item.min,
                        date: item.minDate
                    };
                }

                if (!acc.max || acc.max.value < item.max) {
                    acc.max = {
                        value: item.max,
                        date: item.maxDate
                    };
                }

                acc.degreeDays = (acc.degreeDays || 0) + item.degreeDays;
                acc.total = (acc.total || 0) + item.avg;

                return acc;
            }, {})
        }

        function addTrendData(items) {
            return items.map(function (item, index) {
                var trend = 'equal'

                if (index > 0 && item.avg > items[index - 1].avg) {
                    trend = 'up'
                }

                if (index > 0 && item.avg < items[index - 1].avg) {
                    trend = 'down'
                }

                return Object.assign({}, item, {
                    avgTrend: trend
                });
            });
        }
    });

    app.controller('DeviceTemperatureReportController', function ($scope, $route, $routeParams, $location, domoticzApi, deviceApi, dataTableDefaultSettings, DeviceTemperatureReportData) {
        var vm = this;
        var $element = $('.js-report:last');

        var monthNames = ["January", "February", "March", "April", "May", "June",
            "July", "August", "September", "October", "November", "December"];

        vm.getYearsOptions = getYearsOptions;
        vm.selectYear = selectYear;

        init();

        function init() {
            vm.deviceIdx = $routeParams.id;
            vm.selectedYear = parseInt($routeParams.year) || ((new Date()).getYear() + 1900);
            vm.selectedMonth = parseInt($routeParams.month) || undefined;
            vm.isMonthView = vm.selectedMonth > 0;

            vm.degreeType = $.myglobals.tempsign;

            deviceApi.getDeviceInfo(vm.deviceIdx).then(function (device) {
                vm.title = vm.isMonthView
                    ? device.Name + ', ' + $.t(monthNames[vm.selectedMonth - 1]) + ' ' + vm.selectedYear
                    : device.Name + ' ' + vm.selectedYear;
            });

            getData();
        }

        function selectYear() {
            $route.updateParams({ year: vm.selectedYear });
            $location.replace();
        }

        function getYearsOptions() {
            var currentYear = ((new Date()).getYear() + 1900);
            var years = [];

            for (var i = 2012; i <= currentYear; i++) {
                years.push(i);
            }

            return years;
        }

        function getData() {
            DeviceTemperatureReportData
                .fetch(vm.deviceIdx, vm.selectedYear, vm.selectedMonth)
                .then(function (data) {
                    if (!data) {
                        vm.noDataAvailable = true;
                        return;
                    }

                    var isVariationChartVisible = !(data.isOnlyHumidity && vm.isMonthView);

                    showTable(data);
                    showAverageChart(data);

                    if (isVariationChartVisible) {
                        showVariationChart(data);
                    } else {
                        $element.find('#variation-graph').remove();
                    }

                    if (!data.isOnlyHumidity) {
                        vm.min = {
                            date: dateFormat(data.minDate, 'd') + ' ' + $.t(dateFormat(data.minDate, 'mmmm')),
                            value: data.min
                        };

                        vm.max = {
                            date: dateFormat(data.maxDate, 'd') + ' ' + $.t(dateFormat(data.maxDate, 'mmmm')),
                            value: data.max
                        };

                        if (data.degreeDays) {
                            vm.degreeDays = data.degreeDays.toFixed(1);
                        }
                    }
                });
        }

        function showTable(data) {
            var table = $element.find('#reporttable');
            var columns = [];

            var humidityRenderer = function (data) {
                return data.toFixed(0) + '%';
            };

            var temperatureRenderer = function (data) {
                return data.toFixed(1) + '\u00B0' + vm.degreeType;
            };

            if (vm.isMonthView) {
                columns.push({
                    title: $.t('Day'),
                    data: 'date',
                    render: function (data) {
                        return $.t(dateFormat(data, "d"));
                    }
                });

                columns.push({
                    title: '',
                    data: 'date',
                    render: function (data) {
                        return $.t(dateFormat(data, "dddd"));
                    }
                });
            } else {
                columns.push({
                    title: $.t('Month'),
                    data: 'date',
                    render: function (data) {
                        var link = '<a href="#/Devices/' + vm.deviceIdx + '/TemperatureReport/' + vm.selectedYear + '/' + data + '"><img src="images/next.png" /></a>';
                        return data.toString().padStart(2, '0') + '. ' + $.t(monthNames[data - 1]) + ' ' + link;
                    }
                });
            }

            if (data.isHumidityAvailable && !data.isOnlyHumidity) {
                columns.push({ title: $.t('Humidity'), data: 'humidity', render: humidityRenderer })
            }

            if (data.isOnlyHumidity && vm.isMonthView) {
                columns.push({ title: $.t('Humidity'), data: 'avg', render: humidityRenderer })
            } else if (data.isOnlyHumidity) {
                columns.push({ title: $.t('Avg. Hum'), data: 'avg', render: humidityRenderer });
                columns.push({ title: $.t('Min. Hum'), data: 'min', render: humidityRenderer });
                columns.push({ title: $.t('Max. Hum'), data: 'max', render: humidityRenderer });
            } else {
                columns.push({ title: $.t('Avg. Temp'), data: 'avg', render: temperatureRenderer });
                columns.push({ title: $.t('Min. Temp'), data: 'min', render: temperatureRenderer });
                columns.push({ title: $.t('Max. Temp'), data: 'max', render: temperatureRenderer });
            }

            columns.push({
                title: '<>',
                orderable: false,
                data: 'avgTrend',
                render: function (data) {
                    return '<img src="images/' + data + '.png">'
                }
            });

            table.dataTable(Object.assign({}, dataTableDefaultSettings, {
                sDom: '<"H"rC>t<"F">',
                columns: columns,
                pageLength: 50,
                order: [[0, 'asc']]
            }));

            table.dataTable().api().rows
                .add(data.items)
                .draw();
        }

        function showAverageChart(data) {
            var chartElement = $element.find('#usagegraph');

            var chartData = data.items.map(function (item) {
                var color = 'rgba(3,190,252,0.8)';

                if (item.min === data.min) {
                    color = '#1100CC';
                }

                if (item.max === data.max) {
                    color = '#FF0000';
                }

                return {
                    x: vm.isMonthView
                        ? +(new Date(item.date))
                        : Date.UTC(vm.selectedYear, item.date - 1, 1),
                    y: parseFloat(item.avg.toFixed(1)),
                    color: color
                }
            });

            chartElement.highcharts({
                credits: {
                    enabled: true,
                    href: "http://www.domoticz.com",
                    text: "Domoticz.com"
                },
                title: {
                    text: data.isOnlyHumidity
                        ? $.t('Average Humidity')
                        : $.t('Average Temperature')
                },
                xAxis: {
                    type: 'datetime'
                },
                yAxis: {
                    maxPadding: 0.2,
                    title: {
                        text: data.isOnlyHumidity
                            ? $.t('Humidity')
                            : $.t('Temperature')
                    }
                },
                tooltip: {
                    valueSuffix: data.isOnlyHumidity
                        ? '%'
                        : ' °' + vm.degreeType

                },
                plotOptions: {
                    column: {
                        minPointLength: 4,
                        pointPadding: 0.1,
                        groupPadding: 0,
                        dataLabels: {
                            enabled: !vm.isMonthView,
                            color: 'white',
                            format: data.isOnlyHumidity
                                ? '{y}%'
                                : '{y} °' + vm.degreeType
                        }
                    }
                },
                legend: {
                    enabled: true
                },
                series: [{
                    type: 'column',
                    name: data.isOnlyHumidity
                        ? $.t('Humidity')
                        : $.t('Temperature'),
                    showInLegend: false,
                    yAxis: 0,
                    color: 'rgba(3,190,252,0.8)',
                    data: chartData
                }]
            });
        }

        function showVariationChart(data) {
            var chartElement = $element.find('#variation-graph');

            var chartData = data.items.map(function (item) {
                return {
                    x: vm.isMonthView
                        ? +(new Date(item.date))
                        : Date.UTC(vm.selectedYear, item.date - 1, 1),
                    low: parseFloat(item.min.toFixed(1)),
                    high: parseFloat(item.max.toFixed(1)),
                }
            });

            var avgData = data.items.map(function (item) {
                return {
                    x: vm.isMonthView
                        ? +(new Date(item.date))
                        : Date.UTC(vm.selectedYear, item.date - 1, 1),
                    y: parseFloat(item.avg.toFixed(1))
                }
            });

            chartElement.highcharts({
                credits: {
                    enabled: true,
                    href: "http://www.domoticz.com",
                    text: "Domoticz.com"
                },
                title: {
                    text: data.isOnlyHumidity
                        ? $.t('Humidity Variation')
                        : $.t('Temperature Variation')
                },
                xAxis: {
                    type: 'datetime'
                },
                yAxis: {
                    maxPadding: 0.2,
                    title: {
                        text: data.isOnlyHumidity
                            ? $.t('Humidity')
                            : $.t('Temperature')
                    }
                },
                tooltip: {
                    shared: true,
                    valueSuffix: data.isOnlyHumidity
                        ? '%'
                        : ' °' + vm.degreeType
                },
                plotOptions: {
                    columnrange: {
                        dataLabels: {
                            enabled: !vm.isMonthView,
                            format: data.isOnlyHumidity
                                ? '{y}%'
                                : '{y} °' + vm.degreeType
                        }
                    }
                },
                legend: {
                    enabled: false
                },
                series: [
                    {
                        type: 'spline',
                        name: data.isOnlyHumidity
                            ? $.t('Average Humidity')
                            : $.t('Average Temperature'),
                        color: 'yellow',
                        data: avgData
                    }, {
                        type: 'areasplinerange',
                        name: data.isOnlyHumidity
                            ? $.t('Humidity Variation')
                            : $.t('Temperature Variation'),
                        color: 'rgb(3,190,252)',
                        fillOpacity: 0.3,
                        data: chartData
                    }
                ]
            });
        }
    });
});