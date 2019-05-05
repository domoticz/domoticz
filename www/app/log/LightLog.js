define(['app', 'log/components/DeviceLevelChart', 'log/components/DeviceOnOffChart', 'log/components/DeviceTextLogTable'], function (app) {

    app.component('deviceLightLog', {
        bindings: {
            deviceIdx: '<'
        },
        templateUrl: 'app/log/LightLog.html',
        controller: DeviceLightLogController,
        controllerAs: 'vm'
    });

    function DeviceLightLogController(domoticzApi, deviceApi, permissions) {
        var vm = this;

        vm.clearLog = clearLog;
        vm.$onInit = init;

        function init() {
            refreshLog();
        }

        function refreshLog() {
            domoticzApi
                .sendRequest({
                    type: 'lightlog',
                    idx: vm.deviceIdx
                })
                .then(function (data) {
                    if (typeof data.result === 'undefined') {
                        return;
                    }

                    vm.log = (data.result || []).sort(function (a, b) {
                        return a.Date < b.Date ? -1 : 1;
                    });

                    vm.isSelector = data.HaveSelector;
                    vm.isDimmer = data.HaveDimmer;
                });
        }

        function clearLog() {
            if (!permissions.hasPermission('Admin')) {
                var message = $.t('You do not have permission to do that!');
                HideNotify();
                ShowNotify(message, 2500, true);
                return;
            }

            bootbox.confirm($.t('Are you sure to delete the Log?\n\nThis action can not be undone!'), function (result) {
                if (result !== true) {
                    return;
                }

                domoticzApi
                    .sendCommand('clearlightlog', {
                        idx: vm.deviceIdx
                    })
                    .then(refreshLog)
                    .catch(function () {
                        HideNotify();
                        ShowNotify($.t('Problem clearing the Log!'), 2500, true);
                    });
            });
        }
    }
});

