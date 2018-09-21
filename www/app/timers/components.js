define(['app', 'timers/factories'], function (app) {
    app.component('timerForm', {
        templateUrl: 'views/timers/timerForm.html',
        bindings: {
            timerSettings: '=',
            levelOptions: '<',
            colorSettingsType: '<',
            isCommandSelectionDisabled: '<',
            isSetpointTimers: '<'
        },
        controllerAs: 'vm',
        controller: function ($scope, $element, deviceTimerOptions, deviceTimerConfigUtils) {
            var vm = this;

            vm.$onInit = init;
            vm.isLevelVisible = isLevelVisible;
            vm.isDayScheduleVisible = isDayScheduleVisible;
            vm.isRDaysVisible = isRDaysVisible;
            vm.isROccurenceVisible = isROccurenceVisible;
            vm.isRMonthsVisible = isRMonthsVisible;
            vm.isDateVisible = isDateVisible;
            vm.isDaySelectonAvailable = isDaySelectonAvailable;

            function init() {
                vm.typeOptions = deviceTimerOptions.timerTypes;
                vm.commandOptions = deviceTimerOptions.command;
                vm.monthOptions = deviceTimerOptions.month;
                vm.dayOptions = deviceTimerOptions.monthday;
                vm.weekdayOptions = deviceTimerOptions.weekday;
                vm.hourOptions = deviceTimerOptions.hour;
                vm.minuteOptions = deviceTimerOptions.minute;
                vm.occurenceOptions = deviceTimerOptions.occurence;

                vm.isColorSettingsAvailable = isLED(vm.colorSettingsType);

                initDatePicker();
                initDaysSelection();

                if (vm.isColorSettingsAvailable) {
                    initLEDLightSettings();
                }
            }

            function initDatePicker() {
                var nowTemp = new Date();
                var now = new Date(nowTemp.getFullYear(), nowTemp.getMonth(), nowTemp.getDate(), 0, 0, 0, 0);
                var element = $element.find("#sdate");

                element.datepicker({
                    minDate: now,
                    defaultDate: now,
                    dateFormat: window.myglobals.DateFormat,
                    showWeek: true,
                    firstDay: 1,
                    onSelect: function (date) {
                        vm.timerSettings.date = date;
                        $scope.$apply();
                    }
                });

                $scope.$watch('vm.timerSettings.date', function (newValue) {
                    if ((+new Date(newValue)) !== (+element.datepicker('getDate'))) {
                        element.datepicker('setDate', newValue);
                    }
                });
            }

            function initDaysSelection() {
                vm.week = {
                    type: "Everyday",
                    days: {
                        mon: true,
                        tue: true,
                        wed: true,
                        thu: true,
                        fri: true,
                        sat: true,
                        sun: true
                    }
                };

                $scope.$watch('vm.week.type', function (value) {
                    if (value === 'Everyday') {
                        vm.week.days.mon = vm.week.days.tue = vm.week.days.wed = vm.week.days.thu = vm.week.days.fri = true;
                        vm.week.days.sat = vm.week.days.sun = true;
                    } else if (value === 'Weekdays') {
                        vm.week.days.mon = vm.week.days.tue = vm.week.days.wed = vm.week.days.thu = vm.week.days.fri = true;
                        vm.week.days.sat = vm.week.days.sun = false;
                    } else if (value === 'Weekends') {
                        vm.week.days.mon = vm.week.days.tue = vm.week.days.wed = vm.week.days.thu = vm.week.days.fri = false;
                        vm.week.days.sat = vm.week.days.sun = true;
                    }
                });

                $scope.$watch('vm.week', function (value) {
                    var days = 0x00;

                    if (value.type === 'Everyday') {
                        days = 0x80;
                    } else if (value.type === 'Weekdays') {
                        days = 0x100;
                    } else if (value.type === 'Weekends') {
                        days = 0x200
                    } else {
                        days |= value.days.mon && 0x01;
                        days |= value.days.tue && 0x02;
                        days |= value.days.wed && 0x04;
                        days |= value.days.thu && 0x08;
                        days |= value.days.fri && 0x10;
                        days |= value.days.sat && 0x20;
                        days |= value.days.sun && 0x40;
                    }

                    vm.timerSettings.days = days;
                }, true);

                $scope.$watch('vm.timerSettings.days', function (value) {
                    if (value & 0x80) {
                        vm.week.type = 'Everyday';
                    } else if (value & 0x100) {
                        vm.week.type = 'Weekdays';
                    } else if (value & 0x200) {
                        vm.week.type = 'Weekends';
                    } else {
                        vm.week.type = 'SelectedDays';
                        vm.week.days.mon = !!(value & 0x01);
                        vm.week.days.tue = !!(value & 0x02);
                        vm.week.days.wed = !!(value & 0x04);
                        vm.week.days.thu = !!(value & 0x08);
                        vm.week.days.fri = !!(value & 0x10);
                        vm.week.days.sat = !!(value & 0x20);
                        vm.week.days.sun = !!(value & 0x40);
                    }
                });
            }

            function initLEDLightSettings() {
                var maxDimLevel = 100; // Always 100 for LED type

                var onColorChange = function (idx, color, brightness) {
                    vm.timerSettings.color = color;
                    vm.timerSettings.level = brightness;
                    $scope.$apply();
                };

                var getColor = function() {
                    return vm.timerSettings.color + vm.timerSettings.level;
                };

                $scope.$watch(getColor, function () {
                    ShowRGBWPicker(
                        '#TimersLedColor',
                        null,
                        0,
                        maxDimLevel,
                        vm.timerSettings.level || 0,
                        vm.timerSettings.color,
                        vm.colorSettingsType,
                        vm.dimmerType,
                        onColorChange
                    );
                });
            }

            function isLevelVisible() {
                return vm.levelOptions && vm.levelOptions.length > 0;
            }

            function isDateVisible() {
                return deviceTimerConfigUtils.isDateApplicable(vm.timerSettings.timertype);
            }

            function isDayScheduleVisible() {
                return deviceTimerConfigUtils.isDayScheduleApplicable(vm.timerSettings.timertype);
            }

            function isRDaysVisible() {
                return deviceTimerConfigUtils.isRDaysApplicable(vm.timerSettings.timertype);
            }

            function isROccurenceVisible() {
                return deviceTimerConfigUtils.isOccurrenceApplicable(vm.timerSettings.timertype);
            }

            function isRMonthsVisible() {
                return deviceTimerConfigUtils.isRMonthsApplicable(vm.timerSettings.timertype);
            }

            function isDaySelectonAvailable() {
                return vm.week.type === 'SelectedDays';
            }
        }
    });

    app.component('timersTable', {
        templateUrl: 'views/timers/timersTable.html',
        bindings: {
            timerSettings: '=',
            selectedTimerIdx: '=',
            timers: '<',
            levelOptions: '<',
            isSetpointTimers: '<'
        },
        controller: function ($scope, $element, deviceTimerOptions, dataTableDefaultSettings) {
            var vm = this;
            var table;

            vm.$onInit = init;
            vm.$onChanges = function (changes) {
                if (!table) {
                    return;
                }

                if (changes.timers) {
                    table.api().clear();
                    table.api().rows
                        .add(vm.timers)
                        .draw();
                }
            };

            function init () {
                var columns = [
                    {title: $.t('Active'), data: 'Active', render: activeRenderer},
                    {title: $.t('Type'), data: 'Type', render: timerTypeRenderer},
                    {title: $.t('Date'), data: 'Date', type: 'date'},
                    {title: $.t('Time'), data: 'Time'},
                    {title: $.t('Randomness'), data: 'Randomness', render: activeRenderer},
                    {title: $.t('Command'), data: 'idx', render: commandRenderer},
                    {title: $.t('Days'), data: 'idx', render: daysRenderer},
                ];

                table = $element.find('table').dataTable(Object.assign({}, dataTableDefaultSettings, {
                    columns: columns
                }));

                table.api().column('4').visible(vm.isSetpointTimers === false);

                table.on('select.dt', function (e, dt, type, indexes) {
                    var timer = dt.rows(indexes).data()[0];

                    vm.selectedTimerIdx = timer.idx;
                    vm.timerSettings.active = timer.Active === 'true';
                    vm.timerSettings.randomness = timer.Randomness === 'true';
                    vm.timerSettings.timertype = timer.Type;
                    vm.timerSettings.hour = parseInt(timer.Time.substring(0, 2));
                    vm.timerSettings.min = parseInt(timer.Time.substring(3, 5));
                    vm.timerSettings.command = timer.Cmd;
                    vm.timerSettings.color = timer.Color;
                    vm.timerSettings.level = timer.Level;
                    vm.timerSettings.tvalue = timer.Temperature;
                    vm.timerSettings.date = timer.Date.replace(/-/g, '/');
                    vm.timerSettings.days = timer.Days;
                    vm.timerSettings.mday = timer.MDay;
                    vm.timerSettings.weekday = Math.log(parseInt(timer.Days)) / Math.log(2);
                    vm.timerSettings.occurence = timer.Occurence;

                    $scope.$apply();
                });

                table.on('deselect.dt', function () {
                    vm.selectedTimerIdx = null;
                    $scope.$apply();
                });
            }

            function getTimerByIdx(idx) {
                return vm.timers.find(function (timer) {
                    return timer.idx === idx;
                });
            }

            function activeRenderer(value) {
                return value === 'true'
                    ? $.t('Yes')
                    : $.t('No');
            }

            function timerTypeRenderer(value) {
                return deviceTimerOptions.timerTypes[value].label
            }

            function commandRenderer(value) {
                var timer = getTimerByIdx(value);
                var command = timer.Cmd === 1 ? 'Off' : 'On';

                if (vm.isSetpointTimers) {
                    return $.t('Temperature') + ', ' + timer.Temperature;
                } else if (command === 'On' && vm.levelOptions.length > 0) {
                    var levelName = deviceTimerOptions.getLabelForValue(vm.levelOptions, timer.Level);
                    return $.t(command) + " (" + levelName + ")";
                } else if (timer.Color) {
                    return getTimerColorBox(timer) + $.t(command) + " (" + timer.Level + "%)";
                }

                return $.t(command);
            }

            function daysRenderer(value) {
                var timer = getTimerByIdx(value);
                var separator = timer.Type > 5 ? ' ' : ', ';
                var DayStr = getDayStr(timer);

                return DayStr
                    .split(separator)
                    .map(function (item) {
                        return $.t(item)
                    })
                    .join(separator);
            }

            function getDayStr(timer) {
                var DayStrOrig = '';

                if ((timer.Type <= 4) || (timer.Type === 8) || (timer.Type === 9) || ((timer.Type >= 14) && (timer.Type <= 27))) {
                    var dayflags = parseInt(timer.Days);
                    
                    if (dayflags & 0x80) {
                        DayStrOrig = 'Everyday';
                    } else if (dayflags & 0x100) {
                        DayStrOrig = 'Weekdays';
                    } else if (dayflags & 0x200) {
                        DayStrOrig = 'Weekends';
                    } else {
                        var days = [];

                        if (dayflags & 0x01) {
                            days.push('Mon');
                        }
                        if (dayflags & 0x02) {
                            days.push('Tue');
                        }
                        if (dayflags & 0x04) {
                            days.push('Wed');
                        }
                        if (dayflags & 0x08) {
                            days.push('Thu');
                        }
                        if (dayflags & 0x10) {
                            days.push('Fri');
                        }
                        if (dayflags & 0x20) {
                            days.push('Sat');
                        }
                        if (dayflags & 0x40) {
                            days.push('Sun');
                        }

                        DayStrOrig = days.join(', ');
                    }
                }
                else if (timer.Type === 10) {
                    DayStrOrig = 'Monthly on Day ' + timer.MDay;
                }
                else if (timer.Type === 11) {
                    var Weekday = Math.log(parseInt(timer.Days)) / Math.log(2);

                    DayStrOrig = [
                        'Monthly on',
                        deviceTimerOptions.occurence[timer.Occurence - 1].label,
                        deviceTimerOptions.weekday[Weekday].label
                    ].join(' ');
                }
                else if (timer.Type === 12) {
                    DayStrOrig = [
                        'Yearly on',
                        timer.MDay,
                        deviceTimerOptions.month[timer.Month - 1].label
                    ].join(' ');
                }
                else if (timer.Type === 13) {
                    var Weekday = Math.log(parseInt(timer.Days)) / Math.log(2);

                    DayStrOrig = [
                        'Yearly on',
                        deviceTimerOptions.occurence[timer.Occurence - 1].label,
                        deviceTimerOptions.weekday[Weekday].label,
                        'in',
                        deviceTimerOptions.month[timer.Month - 1].label
                    ].join(' ');
                }

                return DayStrOrig;
            }

            function getTimerColorBox(timer) {
                if (!timer.Color) {
                    return '';
                }

                var color = JSON.parse(timer.Color);
                var backgroundColor;

                if (color.m === 1 || color.m === 2) { // White or color temperature
                    var whex = Math.round(255 * timer.Level / 100).toString(16);
                    if (whex.length === 1) {
                        whex = "0" + whex;
                    }

                    backgroundColor = '#' + whex + whex + whex;
                }

                if (color.m === 3 || color.m === 4) { // RGB or custom
                    backgroundColor = 'rgb(' + [color.r, color.g, color.b].join(', ') + ')';
                }

                return '<div class="ex-color-box" style="background-color: ' + backgroundColor + ';"></div>';
            }
        }
    })
});
