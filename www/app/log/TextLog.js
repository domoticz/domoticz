define(['app', 'log/components'], function (app) {
    app.controller('DeviceTextLogController', function ($routeParams, domoticzApi, deviceApi, permissions) {
        var vm = this;

        vm.clearLog = clearLog;

        init();

        function init() {
            vm.deviceIdx = $routeParams.id;

            deviceApi.getDeviceInfo(vm.deviceIdx).then(function (device) {
                vm.pageName = device.Name;
            });

            refreshLog();
        }

        function refreshLog() {
            domoticzApi.sendRequest({
                type: 'textlog',
                idx: vm.deviceIdx
            }).then(function (data) {
                vm.log = data.result
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
    });
});