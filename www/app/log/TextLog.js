define(['app', 'log/components/DeviceTextLogTable'], function(app) {

    app.component('deviceTextLog', {
        bindings: {
            deviceIdx: '<'
        },
        templateUrl: 'app/log/TextLog.html',
        controller: DeviceTextLogController,
        controllerAs: 'vm',
    });

    function DeviceTextLogController($scope, $routeParams, domoticzApi, deviceApi, permissions) {
        var vm = this;

        vm.autoRefresh = true;
        vm.clearLog = clearLog;
        vm.$onInit = init;

        function init() {
            refreshLog();

            $scope.$on('device_update', function(event, device) {
                if (vm.autoRefresh && device.idx === vm.deviceIdx) {
                    refreshLog();
                }
            });
        }

        function refreshLog() {
            domoticzApi.sendCommand('gettextlog', {
                idx: vm.deviceIdx
            }).then(function(data) {
                if (typeof data.result !== 'undefined') {
                  for (var i = 0; i < data.result.length; i++) {
                      var dataTemp = data.result[i]['Data'].replace(/([^>\r\n]?)(\r\n|\n\r|\r|\n)/g, '$1<br />$2');
                      data.result[i]['Data'] = dataTemp;
                  }
                }
                vm.log = data.result;
            });
        }

        function clearLog() {
            if (!permissions.hasPermission('Admin')) {
                var message = $.t('You do not have permission to do that!');
                HideNotify();
                ShowNotify(message, 2500, true);
                return;
            }

            bootbox.confirm($.t('Are you sure to delete the Log?\n\nThis action can not be undone!'), function(result) {
                if (result !== true) {
                    return;
                }

                domoticzApi
                    .sendCommand('clearlightlog', {
                        idx: vm.deviceIdx
                    })
                    .then(refreshLog)
                    .catch(function() {
                        HideNotify();
                        ShowNotify($.t('Problem clearing the Log!'), 2500, true);
                    });
            });
        }
    }
});
