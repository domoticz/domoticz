define(['app'], function (app) {

    app.factory('deviceRegularTimersApi', function ($q, domoticzApi) {
        return createTimersApiFactory('timer', $q, domoticzApi);
    });

    app.factory('deviceSetpointTimersApi', function ($q, domoticzApi) {
        return createTimersApiFactory('setpointtimer', $q, domoticzApi);
    });

    app.factory('sceneTimersApi', function ($q, domoticzApi) {
        return createTimersApiFactory('scenetimer', $q, domoticzApi);
    });

    app.factory('deviceTimerConfigUtils', function () {
        return {
            isDateApplicable: isDateApplicable,
            isDayScheduleApplicable: isDayScheduleApplicable,
            isRDaysApplicable: isRDaysVisible,
            isOccurrenceApplicable: isOccurrenceApplicable,
            isRMonthsApplicable: isRMonthsApplicable,
            getTimerDefaultConfig: getTimerDefaultConfig,
            getTimerConfigErrors: getTimerConfigErrors,
            getTimerConfigWarnings: getTimerConfigWarnings
        };

        function isDateApplicable(timerType) {
            return timerType == 5
        }

        function isDayScheduleApplicable(timerType) {
            return ![5, 6, 7, 10, 11, 12, 13].includes(timerType);
        }

        function isRDaysVisible(timerType) {
            return [10, 12].includes(timerType);
        }

        function isOccurrenceApplicable(timerType) {
            return [11, 13].includes(timerType);
        }

        function isRMonthsApplicable(timerType) {
            return [12, 13].includes(timerType);
        }

        function getTimerDefaultConfig() {
            return {
                active: true,
                timertype: 0,
                date: '',
                hour: 0,
                min: 0,
                randomness: false,
                command: 0,
                level: null,
                tvalue: '',
                days: 0x80,
                color: '', // Empty string, intentionally illegal JSON
                mday: 1,
                month: 1,
                occurence: 1,
                weekday: 0,
            };
        }

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
            timerTypes: getTimerTypes(),
            command: getCommandOptions(),
            month: getMonthOptions(),
            monthday: getMonthdayOptions(),
            weekday: getWeekdayOptions(),
            hour: getHourOptions(),
            minute: getMinuteOptions(),
            occurence: getOccurenceOptions(),
            getLabelForValue: getLabelForValue
        };

        function getLabelForValue(options, value) {
            var option = options.find(function (option) {
                return option.value === value;
            });

            return option ? option.label : '';
        }

        function getTimerTypes() {
            return [
                'Before Sunrise', 'After Sunrise', 'On Time', 'Before Sunset', 'After Sunset',
                'Fixed Date/Time', 'Odd Day Numbers', 'Even Day Numbers', 'Odd Week Numbers',
                'Even Week Numbers', 'Monthly', 'Monthly (Weekday)', 'Yearly', 'Yearly (Weekday)',
                'Before Sun at South', 'After Sun at South', 'Before Civil Twilight Start',
                'After Civil Twilight Start', 'Before Civil Twilight End', 'After Civil Twilight End',
                'Before Nautical Twilight Start', 'After Nautical Twilight Start',
                'Before Nautical Twilight End', 'After Nautical Twilight End',
                'Before Astronomical Twilight Start', 'After Astronomical Twilight Start',
                'Before Astronomical Twilight End', 'After Astronomical Twilight End'
            ].map(function (item, index) {
                return {
                    label: $.t(item),
                    value: index
                }
            })
        }

        function getCommandOptions() {
            return [
                {label: $.t('On'), value: 0},
                {label: $.t('Off'), value: 1},
            ]
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

    function createTimersApiFactory(timerType, $q, domoticzApi) {
        return {
            getTimers: getTimers,
            addTimer: addTimer,
            updateTimer: updateTimer,
            clearTimers: clearTimers,
            deleteTimer: deleteTimer
        };

        function getTimers(deviceIdx) {
            return domoticzApi.sendRequest({type: timerType + 's', idx: deviceIdx})
                .then(function (data) {
                    return data.status === 'OK'
                        ? data.result || []
                        : $q.reject(data);
                });
        }

        function addTimer(deviceIdx, config) {
            return domoticzApi.sendCommand('add' + timerType, Object.assign({}, config, {idx: deviceIdx}));
        }

        function updateTimer(timerIdx, config) {
            return domoticzApi.sendCommand('update' + timerType, Object.assign({}, config, {idx: timerIdx}));
        }

        function clearTimers(deviceIdx) {
            return domoticzApi.sendCommand('clear' + timerType + 's', {idx: deviceIdx});
        }

        function deleteTimer(timerIdx) {
            return domoticzApi.sendCommand('delete' + timerType, {idx: timerIdx});
        }
    }
});