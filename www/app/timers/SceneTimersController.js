define(['app', 'timers/factories', 'timers/components', 'scenes/factories'], function (app) {
    app.controller('SceneTimersController', function ($routeParams, sceneApi, sceneTimersApi, deviceTimerOptions, deviceTimerConfigUtils) {
        var vm = this;

        vm.addTimer = addTimer;
        vm.updateTimer = updateTimer;
        vm.deleteTimer = deleteTimer;
        vm.clearTimers = clearTimers;

        init();

        function init() {
            vm.sceneIdx = $routeParams.id;
            vm.selectedTimerIdx = null;

            sceneApi.getSceneInfo(vm.sceneIdx).then(function (scene) {
                vm.isLoaded = true;
                vm.itemName = scene.Name;
                vm.colorSettingsType = 'Scene';

                vm.isCommandSelectionDisabled = scene.Type === 'Scene';
                vm.isSetpointTimers = false;

                vm.levelOptions = [];
                refreshTimers();
            });

            vm.typeOptions = deviceTimerOptions.timerTypes;
            vm.timerSettings = deviceTimerConfigUtils.getTimerDefaultConfig();
        }

        function refreshTimers() {
            vm.selectedTimerIdx = null;

            sceneTimersApi.getTimers(vm.sceneIdx).then(function (items) {
                vm.timers = items;
            });
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
                color: undefined,
                tvalue: undefined,
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

            sceneTimersApi
                .addTimer(vm.sceneIdx, config)
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

            sceneTimersApi
                .updateTimer(timerIdx, config)
                .then(refreshTimers)
                .catch(function () {
                    HideNotify();
                    ShowNotify($.t('Problem updating timer!'), 2500, true);
                });
        }

        function deleteTimer(timerIdx) {
            bootbox.confirm($.t('Are you sure to delete this timers?\n\nThis action can not be undone...'), function (result) {
                if (result != true) {
                    return;
                }

                sceneTimersApi
                    .deleteTimer(timerIdx)
                    .then(refreshTimers)
                    .catch(function () {
                        HideNotify();
                        ShowNotify($.t('Problem deleting timer!'), 2500, true);
                    });
            });
        }

        function clearTimers() {
            bootbox.confirm($.t('Are you sure to delete ALL timers?\n\nThis action can not be undone!'), function (result) {
                if (result != true) {
                    return;
                }

                sceneTimersApi
                    .clearTimers(vm.sceneIdx)
                    .then(refreshTimers)
                    .catch(function () {
                        HideNotify();
                        ShowNotify($.t('Problem clearing timers!'), 2500, true);
                    })
            });
        }
    });
});