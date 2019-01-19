define([
    'app',
    'hardware/setup/BleBox',
    'hardware/setup/Kodi',
    'hardware/setup/MySensors',
    'hardware/setup/PanasonicTV',
    'hardware/setup/Pinger',
    'hardware/setup/WakeOnLan',
    'hardware/setup/ZWave',
], function (app) {
    app.controller('HardwareSetupController', function ($routeParams, domoticzApi) {
        var vm = this;
        init();

        function init() {
            vm.hardwareId = $routeParams.id;

            domoticzApi.sendRequest({ type: 'hardware' })
                .then(domoticzApi.errorHandler)
                .then(function (response) {
                    vm.hardware = response.result.find(function (item) {
                        return item.idx === vm.hardwareId;
                    });
                });
        }
    });
});
