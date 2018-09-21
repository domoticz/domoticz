define(['app', 'log/TextLog', 'log/TemperatureLog', 'log/LightLog', 'log/GraphLog'], function (app) {
    app.controller('DeviceLogController', function ($routeParams, domoticzApi, deviceApi) {
        var vm = this;

        vm.isTextLog = isTextLog;
        vm.isLightLog = isLightLog;
        vm.isGraphLog = isGraphLog;
        vm.isTemperatureLog = isTemperatureLog;

        init();

        function init() {
            vm.deviceIdx = $routeParams.id;

            deviceApi.getDeviceInfo(vm.deviceIdx).then(function (device) {
                vm.device = device;
                vm.pageName = device.Name;
            });
        }

        function isTextLog() {
            if (!vm.device) {
                return undefined;
            }

            return ['Text', 'Alert'].includes(vm.device.SubType) || vm.device.SwitchType === 'Media Player';
        }

        function isLightLog() {
            if (!vm.device) {
                return undefined;
            }

            if (vm.device.Type === 'Heating') {
                return (vm.device.SubType != 'Zone');
            }

            var isLightType = [
                'Light', 'Light/Switch', 'Color Switch', 'Chime',
                'Security', 'RFY', 'ASA', 'Blinds'
            ].includes(vm.device.Type);

            var isLightSwitchType = [
                'Contact', 'Door Contact', 'Dusk Sensor', 'Motion Sensor',
                'Smoke Detector', 'On/Off'
            ].includes(vm.device.SwitchType);

            return isLightType || isLightSwitchType;
        }

        function isTemperatureLog() {
            if (!vm.device) {
                return undefined;
            }

            if (vm.device.Type === 'Heating') {
                return (vm.device.SubType === 'Zone');
            }

            //This goes wrong (when we also use this log call from the weather tab), for wind sensors
            //as this is placed in weather and temperature, we might have to set a parameter in the url
            //for now, we assume it is a temperature
            return (/Temp|Thermostat|Humidity|Radiator|Wind/i).test(vm.device.Type)
        }

        function isGraphLog() {
            if (!vm.device) {
                return undefined;
            }

            return vm.device.Type === 'Usage' || [
                'Voltage', 'Current', 'Pressure', 'Custom Sensor',
                'Sound Level', 'Solar Radiation', 'Visibility', 'Distance',
                'Soil Moisture', 'Leaf Wetness', 'Waterflow', 'Lux', 'Percentage'
            ].includes(vm.device.SubType)
        }
    });
});
