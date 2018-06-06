define(['app', 'timers/factories', 'timers/components'], function (app) {

    app.controller('DeviceTimersController', function ($scope, $routeParams, deviceApi, deviceLightApi, deviceRegularTimersApi, deviceSetpointTimersApi, deviceTimerOptions, deviceTimerConfigUtils, utils) {
        var vm = this;
        var deviceTimers;

        var deleteConfirmationMessage = $.t('Are you sure to delete this timers?\n\nThis action can not be undone...');
        var clearConfirmationMessage = $.t('Are you sure to delete ALL timers?\n\nThis action can not be undone!');

        vm.addTimer = addTimer;
        vm.updateTimer = updateTimer;
        vm.deleteTimer = utils.confirmDecorator(deleteTimer, deleteConfirmationMessage);
        vm.clearTimers = utils.confirmDecorator(clearTimers, clearConfirmationMessage);

        init();

        function init() {
            vm.deviceIdx = $routeParams.id;
            vm.selectedTimerIdx = null;

            deviceApi.getDeviceInfo(vm.deviceIdx).then(function (device) {
                vm.isLoaded = true;
                vm.itemName = device.Name;
                vm.colorSettingsType = device.SubType;
                vm.dimmerType = device.DimmerType;

                vm.isDimmer = device.isDimmer();
                vm.isSelector = device.isSelector();
                vm.isLED = device.isLED();
                vm.isCommandSelectionDisabled = vm.isSelector && device.LevelOffHidden;
                vm.isSetpointTimers = (device.Type === 'Thermostat' && device.SubType == 'SetPoint') || (device.Type === 'Radiator 1');

                vm.levelOptions = [];

                deviceTimers = vm.isSetpointTimers
                    ? deviceSetpointTimersApi
                    : deviceRegularTimersApi;

                if (vm.isSelector) {
                    vm.levelOptions = device.getSelectorLevelOptions();
                }

                if (vm.isLED) {
                    $scope.$watch(function () {
                        return vm.timerSettings.color + vm.timerSettings.level;
                    }, setDeviceColor);
                }

                if (!vm.isLED && vm.isDimmer) {
                    vm.levelOptions = device.getDimmerLevelOptions(1);
                }

                if (vm.levelOptions.length > 0) {
                    vm.timerSettings.level = vm.levelOptions[0].value;
                }

                refreshTimers();
            });

            vm.typeOptions = deviceTimerOptions.timerTypes;
            vm.timerSettings = deviceTimerConfigUtils.getTimerDefaultConfig();
        }

        function refreshTimers() {
            vm.selectedTimerIdx = null;

            deviceTimers.getTimers(vm.deviceIdx).then(function (items) {
                vm.timers = items;
            });
        }

        function setDeviceColor() {
            if (!vm.timerSettings.color || !vm.timerSettings.level) {
                return;
            }

            deviceLightApi.setColor(
                vm.deviceIdx,
                vm.timerSettings.color,
                vm.timerSettings.level
            );
        }

        function getTimerConfig() {
            var utils = deviceTimerConfigUtils;
            var config = Object.assign({}, vm.timerSettings);

            if (!utils.isDayScheduleApplicable(config.timertype)) {
                config.days = 0;
            }

            if ([6, 7, 10, 12].includes(config.timertype)) {
                config.days = 0x80;
            }

            if (utils.isOccurrenceApplicable(config.timertype)) {
                config.days = Math.pow(2, config.weekday);
            }

            config = Object.assign({}, config, {
                date: utils.isDateApplicable(config.timertype) ? config.date : undefined,
                mday: utils.isRDaysApplicable(config.timertype) ? config.mday : undefined,
                month: utils.isRMonthsApplicable(config.timertype) ? config.month : undefined,
                occurence: utils.isOccurrenceApplicable(config.timertype) ? config.occurence : undefined,
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
            return deviceTimers.deleteTimer(timerIdx)
                .then(refreshTimers)
                .catch(function () {
                    HideNotify();
                    ShowNotify($.t('Problem deleting timer!'), 2500, true);
                });
        }

        function clearTimers() {
            return deviceTimers
                .clearTimers(vm.deviceIdx)
                .then(refreshTimers)
                .catch(function () {
                    HideNotify();
                    ShowNotify($.t('Problem clearing timers!'), 2500, true);
                });
        }
    });
});
