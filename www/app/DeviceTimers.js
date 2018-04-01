define(['app'], function (app) {

    app.factory('deviceRegularTimers', function ($q, domoticzApi) {
        return {
            addTimer: addTimer,
            updateTimer: updateTimer,
            clearTimers: clearTimers,
            deleteTimer: deleteTimer,
            getDeviceTimers: getDeviceTimers
        };

        
        function getDeviceTimers(deviceIdx) {
            return domoticzApi.sendRequest({ type: 'timers', idx: deviceIdx })
                .then(function (data) {
                    return data.status === 'OK'
                        ? data.result || []
                        : $q.reject(data);
                });
        }

        function addTimer(deviceIdx, config) {
            return domoticzApi.sendCommand('addtimer', Object.assign({}, config, { idx: deviceIdx }));
        }

        function updateTimer(timerIdx, config) {
            return domoticzApi.sendCommand('updatetimer', Object.assign({}, config, { idx: timerIdx }));
        }

        function clearTimers(deviceIdx) {
            return domoticzApi.sendCommand('cleartimers', { idx: deviceIdx });
        }

        function deleteTimer(timerIdx) {
            return domoticzApi.sendCommand('deletetimer', { idx: timerIdx });
        }
    });

    app.factory('deviceSetpointTimers', function($q, domoticzApi) {
        return {
            addTimer: addTimer,
            updateTimer: updateTimer,
            clearTimers: clearTimers,
            deleteTimer: deleteTimer,
            getDeviceTimers: getDeviceTimers
        };

        function getDeviceTimers(deviceIdx) {
            return domoticzApi.sendRequest({ type: 'setpointtimers', idx: deviceIdx })
                .then(function (data) {
                    return data.status === 'OK'
                        ? data.result || []
                        : $q.reject(data);
                });
        }

        function addTimer(deviceIdx, config) {
            return domoticzApi.sendCommand('addsetpointtimer', Object.assign({}, config, { idx: deviceIdx }));
        }

        function updateTimer(timerIdx, config) {
            return domoticzApi.sendCommand('updatesetpointtimer', Object.assign({}, config, { idx: timerIdx }));
        }

        function clearTimers(deviceIdx) {
            return domoticzApi.sendCommand('clearsetpointtimers', { idx: deviceIdx });
        }

        function deleteTimer(timerIdx) {
            return domoticzApi.sendCommand('deletesetpointtimer', { idx: timerIdx });
        }
    });

    app.factory('deviceTimerConfigUtils', function() {
        return {
            getTimerConfigErrors: getTimerConfigErrors,
            getTimerConfigWarnings: getTimerConfigWarnings
        };

        function getTimerConfigErrors(config) {
            if (config.timertype == 5) {
                if (!config.date) {
                    return $.t('Please select a Date!');
                }

                //Check if date/time is valid
                var pickedDate = new Date(config.date);
                var checkDate = new Date(pickedDate.getFullYear(), pickedDate.getMonth(), pickedDate.getDate(), config.hour, config.min, 0, 0);
                var nowDate = new Date();

                if (checkDate < nowDate) {
                    return $.t('Invalid Date selected!');
                }
            }

            if (config.timertype == 12) {
                if ([4, 6, 9, 11].includes(config.month) && config.mday == 31) {
                    return $.t('This month does not have 31 days!');
                }

                if (config.month == 2 && config.mday > 29) {
                    return $.t('February does not have more than 29 days!')
                }
            }

            if (![5, 6, 7, 10, 11, 12, 13].includes(config.timertype) && config.days == 0) {
                return $.t('Please select some days!');
            }
        }

        function getTimerConfigWarnings(config) {
            if (config.timertype == 10 && config.mday > 28) {
                return $.t('Not al months have this amount of days, some months will be skipped!');
            }

            if (config.timertype == 12 && config.month == 2 && config.mday == 29) {
                return $.t('Not all years have this date, some years will be skipped!');
            }
        }
    });

    app.factory('deviceTimerOptions', function () {
        return {
            command: getCommandOptions(),
            dimmerLevel: getDimmerLevelOptions(),
            month: getMonthOptions(),
            monthday: getMonthdayOptions(),
            weekday: getWeekdayOptions(),
            hour: getHourOptions(),
            minute: getMinuteOptions(),
            occurence: getOccurenceOptions(),
            getLabelForValue: getLabelForValue
        };

        function getLabelForValue(options, value) {
            var option = options.find(function(option) {
                return option.value === value;
            });

            return option ? option.label : '';
        }

        function getCommandOptions() {
            return [
                { label: $.t('On'), value: 0 },
                { label: $.t('Off'), value: 1 },
            ]
        }

        function getDimmerLevelOptions() {
            var options = [];

            for (var i = 1; i <= 100; i++) {
                options.push({
                    label: i + '%',
                    value: i
                });
            }

            return options;
        }

        function getMonthOptions() {
            return ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September', 'October', 'November', 'December']
                .map(function (month, index) {
                    return {
                        label: $.t(month),
                        value: index + 1
                    };
                });
        }

        function getMonthdayOptions() {
            var options = [];

            for (var i = 1; i <= 31; i++) {
                options.push({
                    label: i,
                    value: i
                });
            }

            return options;
        }

        function getWeekdayOptions() {
            return ['Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday', 'Sunday']
                .map(function (day, index) {
                    return {
                        label: $.t(day),
                        value: index
                    }
                });
        }

        function getHourOptions() {
            var options = [];

            for (var i = 0; i < 24; i++) {
                options.push({
                    label: ("0" + i).slice(-2),
                    value: i
                });
            }

            return options;
        }

        function getMinuteOptions() {
            var options = [];

            for (var i = 0; i < 60; i++) {
                options.push({
                    label: ("0" + i).slice(-2),
                    value: i
                });
            }

            return options;
        }

        function getOccurenceOptions() {
            return ['First', '2nd', '3rd', '4th', 'Last']
                .map(function (item, index) {
                    return {
                        label: $.t(item),
                        value: index + 1
                    }
                });
        }
    });

    app.controller('DeviceTimersController', function ($scope, $rootScope, $routeParams, $location, deviceApi, deviceRegularTimers, deviceSetpointTimers, deviceTimerOptions, deviceTimerConfigUtils) {
        var vm = this;
        var $element = $('.js-device-timers:last');
        var setColorTimerId;
        var timers = [];
        var deviceTimers;

        vm.addTimer = addTimer;
        vm.updateTimer = updateTimer;
        vm.deleteTimer = deleteTimer;
        vm.clearTimers = clearTimers;
        vm.isLevelVisible = isLevelVisible;
        vm.isDayScheduleVisible = isDayScheduleVisible;
        vm.isRDaysVisible = isRDaysVisible;
        vm.isROccurenceVisible = isROccurenceVisible;
        vm.isRMonthsVisible = isRMonthsVisible;
        vm.isDateVisible = isDateVisible;
        vm.isDaySelectonAvailable = isDaySelectonAvailable;

        init();

        function init() {
            vm.deviceIdx = $routeParams.id;
            vm.selectedTimerIdx = null;

            deviceApi.getDeviceInfo(vm.deviceIdx).then(function (device) {
                vm.deviceName = device.Name;
                vm.isDimmer = ['Dimmer', 'Blinds Percentage', 'Blinds Percentage Inverted', 'TPI'].includes(device.SwitchType);
                vm.isSelector = device.SubType === "Selector Switch";
                vm.subType = device.SubType;
                vm.isLED = (isLED(device.SubType));
                vm.isCommandSelectionDisabled = vm.isSelector && device.LevelOffHidden;
                vm.isSetpointTimers = (device.Type === 'Thermostat' && device.SubType == 'SetPoint') || (device.Type === 'Radiator 1');

                deviceTimers = vm.isSetpointTimers
                    ? deviceSetpointTimers
                    : deviceRegularTimers;

                if (vm.isSelector) {
                    vm.levelOptions = device.LevelNames
                        .split('|')
                        .slice(1)
                        .map(function (levelName, index) {
                            return {
                                label: levelName,
                                value: (index + 1) * 10
                            }
                        });

                    vm.timerSettings.level = vm.levelOptions[0].value;
                }

                if (vm.isLED) {
                    initLEDLightSettings();
                }

                refreshTimers();
            });

            $rootScope.RefreshTimeAndSun();
            $element.i18n();

            fillTimerEditFormOptions();

            vm.timerSettings = {
                active: true,
                timertype: vm.typeOptions[0].value,
                date: '',
                hour: vm.hourOptions[0].value,
                min: vm.minuteOptions[0].value,
                randomness: false,
                command: vm.commandOptions[0].value,
                level: vm.levelOptions[0].value,
                tvalue: '',
                days: 0x80,
                color: "", // Empty string, intentionally illegal JSON
                mday: vm.dayOptions[0].value,
                month: 1,
                occurence: vm.occurenceOptions[0].value,
                weekday: vm.weekdayOptions[0].value,
            };

            initDatePicker();
            initDaysSelection();
        };

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
            vm.lightSettings = {
                color: "", // Empty string, intentionally illegal JSON
                s: 180,
                brightness: 100
            };

            let MaxDimLevel = 100; // Always 100 for LED type
            ShowRGBWPicker('#TimersLedColor', vm.deviceIdx, 0, MaxDimLevel, vm.lightSettings.brightness, vm.lightSettings.color, vm.subType);

            // TODO: Rewrite..
            SetColValue = function (idx, color, brightness) {
                clearInterval($.setColValue);
                vm.lightSettings.color = color;
                vm.lightSettings.brightness = brightness;
                        $scope.$apply();
            }

            $scope.$watch('vm.lightSettings', function(newValue, oldValue) {
                let MaxDimLevel = 100; // Always 100 for LED type
                ShowRGBWPicker('#TimersLedColor', vm.deviceIdx, 0, MaxDimLevel, vm.lightSettings.brightness, vm.lightSettings.color, vm.subType);

                setColorDebounce();
            }, true)
        }

        function initDatePicker() {
            var nowTemp = new Date();
            var now = new Date(nowTemp.getFullYear(), nowTemp.getMonth(), nowTemp.getDate(), 0, 0, 0, 0);
            var element = $element.find("#sdate");

            element.datepicker({
                minDate: now,
                defaultDate: now,
                dateFormat: "mm/dd/yy",
                showWeek: true,
                firstDay: 1,
                onSelect: function(date) {
                    vm.timerSettings.date = date;
                    $scope.$apply();
                }
            });

            $scope.$watch('vm.timerSettings.date', function(newValue) {
                if ((+new Date(newValue)) !== (+element.datepicker('getDate'))) {
                    element.datepicker('setDate', newValue);
                }                
            });
        }

        function setColorDebounce() {
            clearTimeout(setColorTimerId);

            setColorTimerId = setTimeout(function () {
                deviceApi.setColor(
                    vm.deviceIdx,
                    vm.lightSettings.color,
                    vm.lightSettings.brightness
                );
            }, 400);
        }

        function fillTimerEditFormOptions() {
            vm.typeOptions = [];
            vm.commandOptions = deviceTimerOptions.command;
            vm.monthOptions = deviceTimerOptions.month;
            vm.dayOptions = deviceTimerOptions.monthday;
            vm.weekdayOptions = deviceTimerOptions.weekday;
            vm.hourOptions = deviceTimerOptions.hour;
            vm.minuteOptions = deviceTimerOptions.minute;
            vm.occurenceOptions = deviceTimerOptions.occurence;
            vm.levelOptions = deviceTimerOptions.dimmerLevel;

            $element.find('#types-template > option').each(function () {
                vm.typeOptions.push({
                    label: $(this).text(),
                    value: parseInt($(this).val())
                });
            });
        }

        function refreshTimers() {
            var isDimmer = vm.isDimmer;
            var isSelector = vm.isSelector;
            var isLED = vm.isLED;
            var containerId = '#timertable';
            var timersTable;

			vm.selectedTimerIdx = null;

            if ($.fn.dataTable.isDataTable(containerId)) {
                timersTable = $element.find(containerId).dataTable();
            } else {
                timersTable = $element.find(containerId).dataTable({
                    "sDom": '<"H"lfrC>t<"F"ip>',
                    "aaSorting": [[0, "desc"]],
                    "bSortClasses": false,
                    "bProcessing": true,
                    "bStateSave": true,
                    "bJQueryUI": true,
                    lengthMenu: [[25, 50, 100, -1], [25, 50, 100, "All"]],
                    pageLength: 25,
                    pagingType: 'full_numbers',
                    select: {
                        className: 'row_selected',
                        style: 'single'
                    },
                    language: $.DataTableLanguage
                });

                timersTable.api().column('4').visible(vm.isSetpointTimers === false);

                timersTable.on('select.dt', function (e, dt, type, indexes) {
                    var data = dt.rows(indexes).data()[0];
                    var timer = timers.find(function (item) {
                        return item.idx === data.DT_RowId
                    });

                    vm.selectedTimerIdx = timer.idx;
                    vm.timerSettings.active = timer.Active === 'true';
                    vm.timerSettings.randomness = timer.Randomness === 'true';
                    vm.timerSettings.timertype = timer.Type;
                    vm.timerSettings.hour = parseInt(timer.Time.substring(0, 2));
                    vm.timerSettings.min = parseInt(timer.Time.substring(3, 5));
                    vm.timerSettings.command = timer.Cmd;
                    vm.timerSettings.level = timer.Level;
                    vm.timerSettings.tvalue = timer.Temperature;
                    vm.timerSettings.date = timer.Date.replace(/-/g, '/');
                    vm.timerSettings.days = timer.Days;
                    vm.timerSettings.mday = timer.MDay;
                    vm.timerSettings.weekday = Math.log(parseInt(timer.Days)) / Math.log(2);
                    vm.timerSettings.occurence = timer.Occurence

                    if (vm.isLED) {
                        vm.lightSettings.color = timer.Color;
                        vm.lightSettings.brightness = timer.Level;
                    }

                    $scope.$apply();
                });

                timersTable.on('deselect.dt', function (e, dt, type, indexes) {
                    vm.selectedTimerIdx = null;
                    $scope.$apply();
                });
            }

            timersTable.fnClearTable();

            deviceTimers.getDeviceTimers(vm.deviceIdx).then(function (items) {
                timers = items;
                timers.forEach(function (timer) {
                    var active = timer.Active === 'true' ? 'Yes' : 'No';
                    var Command = timer.Cmd === 1 ? 'Off' : 'On';
                    var tCommand;

                    if (vm.isSetpointTimers) {
                        tCommand = $.t('Temperature') + ', ' + timer.Temperature;
                    } else if (Command === 'On' && isSelector) {
                        var levelName = deviceTimerOptions.getLabelForValue(vm.levelOptions, timer.Level);
                        tCommand = Command + " (" + levelName + ")";
                    } else if (Command === 'On' && isDimmer) {
                        tCommand = $.t(Command) + " (" + timer.Level + "%)";

                        if (isLED) {
                            let color = {};
                            try {
                                color = JSON.parse(timer.Color);
                            }
                            catch(e) {
                                // forget about it :)
                            }
                            //TODO: Refactor to some nice helper function
                            //TODO: Calculate color if color mode is white/temperature.
                            let rgbhex = "808080";
                            if (color.m == 1 || color.m == 2) { // White or color temperature
                                let whex = Math.round(255*timer.Level/100).toString(16);
                                if( whex.length == 1) {
                                    whex = "0" + whex;
                                }
                                rgbhex = whex + whex + whex;
                            }
                            if (color.m == 3 || color.m == 4) { // RGB or custom
                                let rhex = Math.round(color.r).toString(16);
                                if( rhex.length == 1) {
                                    rhex = "0" + rhex;
                                }
                                let ghex = Math.round(color.g).toString(16);
                                if( ghex.length == 1) {
                                    ghex = "0" + ghex;
                                }
                                let bhex = Math.round(color.b).toString(16);
                                if( bhex.length == 1) {
                                    bhex = "0" + bhex;
                                }
                                rgbhex = rhex + ghex + bhex;
                            }
                            tCommand += '<div id="picker4" class="ex-color-box" style="background-color: #' + rgbhex + ';"></div>';
                        }
                    } else {
                        tCommand = Command;
                    }

                    var separator = timer.Type > 5 ? ' ' : ', ';
                    var DayStr = getDayStr(timer);
                    var DayStrTranslated = DayStr
                        .split(separator)
                        .map(function (item) {
                            return $.t(item)
                        })
                        .join(separator);

                    var rEnabled = timer.Randomness == "true" ? "Yes" : "No";

                    timersTable.fnAddData({
                        "DT_RowId": timer.idx,
                        "0": $.t(active),
                        "1": vm.typeOptions[timer.Type].label,
                        "2": timer.Date,
                        "3": timer.Time,
                        "4": $.t(rEnabled),
                        "5": $.t(tCommand),
                        "6": DayStrTranslated
                    });
                })
            });
        }

        function getDayStr(timer) {
            var DayStrOrig = "";

            if ((timer.Type <= 4) || (timer.Type == 8) || (timer.Type == 9) || ((timer.Type >= 14) && (timer.Type <= 27))) {
                var dayflags = parseInt(timer.Days);
                if (dayflags & 0x80)
                    DayStrOrig = "Everyday";
                else if (dayflags & 0x100)
                    DayStrOrig = "Weekdays";
                else if (dayflags & 0x200)
                    DayStrOrig = "Weekends";
                else {
                    if (dayflags & 0x01) {
                        if (DayStrOrig != "") DayStrOrig += ", ";
                        DayStrOrig += "Mon";
                    }
                    if (dayflags & 0x02) {
                        if (DayStrOrig != "") DayStrOrig += ", ";
                        DayStrOrig += "Tue";
                    }
                    if (dayflags & 0x04) {
                        if (DayStrOrig != "") DayStrOrig += ", ";
                        DayStrOrig += "Wed";
                    }
                    if (dayflags & 0x08) {
                        if (DayStrOrig != "") DayStrOrig += ", ";
                        DayStrOrig += "Thu";
                    }
                    if (dayflags & 0x10) {
                        if (DayStrOrig != "") DayStrOrig += ", ";
                        DayStrOrig += "Fri";
                    }
                    if (dayflags & 0x20) {
                        if (DayStrOrig != "") DayStrOrig += ", ";
                        DayStrOrig += "Sat";
                    }
                    if (dayflags & 0x40) {
                        if (DayStrOrig != "") DayStrOrig += ", ";
                        DayStrOrig += "Sun";
                    }
                }
            }
            else if (timer.Type == 10) {
                DayStrOrig = "Monthly on Day " + timer.MDay;
            }
            else if (timer.Type == 11) {
                var Weekday = Math.log(parseInt(timer.Days)) / Math.log(2);
                DayStrOrig = "Monthly on " + vm.occurenceOptions[timer.Occurence - 1].label + " " + vm.weekdayOptions[Weekday].label;
            }
            else if (timer.Type == 12) {
                DayStrOrig = "Yearly on " + timer.MDay + " " + vm.monthOptions[timer.Month - 1].label;
            }
            else if (timer.Type == 13) {
                var Weekday = Math.log(parseInt(timer.Days)) / Math.log(2);
                DayStrOrig = "Yearly on " + vm.occurenceOptions[timer.Occurence - 1].label + " " + vm.weekdayOptions[Weekday].label + " in " + vm.monthOptions[timer.Month - 1].label;
            }

            return DayStrOrig;
        }

        function getTimerConfig() {
            var config = Object.assign({}, vm.timerSettings);

            if (!isDayScheduleVisible()) {
                config.days = 0;
            }

            if ([6, 7, 10, 12].includes(config.timertype)) {
                config.days = 0x80;
            }

            if (isROccurenceVisible()) {
                config.days = Math.pow(2, config.weekday);
            }

            if (vm.isLED) {
                config.level = vm.lightSettings.brightness;
                config.color = vm.lightSettings.color;
            }

            config = Object.assign({}, config, {
                date: isDateVisible() ? config.date : undefined,
                mday: isRDaysVisible() ? config.mday : undefined,
                month: isRMonthsVisible() ? config.month : undefined,
                occurence: isROccurenceVisible() ? config.occurence : undefined,
                weekday: undefined,
                color: vm.isLED ? config.color : undefined,
                tvalue: vm.isSetpointTimers ? config.tvalue : undefined,
                command: vm.isSetpointTimers ? undefined : config.command,
                randomness: vm.isSetpointTimers ? undefined : config.randomness
            });

            var error = deviceTimerConfigUtils.getTimerConfigErrors(config);

            if (error) {
                ShowNotify(error, 2500, true);
                return false;
            }

            var warning = deviceTimerConfigUtils.getTimerConfigWarnings(config);

            if (warning) {
                ShowNotify(error, 2500, true);
            }

            return config;
        }

        function addTimer() {
            const config = getTimerConfig();

            if (!config) {
                return false;
            }

            deviceTimers
                .addTimer(vm.deviceIdx, config)
                .then(refreshTimers)
                .catch(function () {
                    HideNotify();
                    ShowNotify($.t('Problem adding timer!'), 2500, true);
                });
        }

        function updateTimer(timerIdx) {
            const config = getTimerConfig();

            if (!config) {
                return false;
            }

            deviceTimers
                .updateTimer(timerIdx, config)
                .then(refreshTimers)
                .catch(function () {
                    HideNotify();
                    ShowNotify($.t('Problem updating timer!'), 2500, true);
                });
        }

        function deleteTimer(timerIdx) {
            bootbox.confirm($.t("Are you sure to delete this timers?\n\nThis action can not be undone..."), function (result) {
                if (result != true) {
                    return;
                }

                deviceTimers.deleteTimer(timerIdx)
                    .then(refreshTimers)
                    .catch(function () {
                        HideNotify();
                        ShowNotify($.t('Problem deleting timer!'), 2500, true);
                    });
            });
        }

        function clearTimers() {
            bootbox.confirm($.t("Are you sure to delete ALL timers?\n\nThis action can not be undone!"), function (result) {
                if (result != true) {
                    return;
                }

                deviceTimers
                    .clearTimers(vm.deviceIdx)
                    .then(refreshTimers)
                    .catch(function () {
                        HideNotify();
                        ShowNotify($.t('Problem clearing timers!'), 2500, true);
                    })
            });
        }

        function isLevelVisible() {
            if ((!vm.isLED && vm.isDimmer) || vm.isSelector) {
                return true;
            }

            return false;
        }

        function isDateVisible() {
            return vm.timerSettings.timertype == 5
        }

        function isDayScheduleVisible() {
            return ![5, 6, 7, 10, 11, 12, 13].includes(vm.timerSettings.timertype);
        }

        function isRDaysVisible() {
            return [10, 12].includes(vm.timerSettings.timertype);
        }

        function isROccurenceVisible() {
            return [11, 13].includes(vm.timerSettings.timertype);
        }

        function isRMonthsVisible() {
            return [12, 13].includes(vm.timerSettings.timertype);
        }

        function isDaySelectonAvailable() {
            return vm.week.type === 'SelectedDays';
        }
    });
});
