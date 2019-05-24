define(['app', 'log/TextLog', 'log/TemperatureLog', 'log/LightLog', 'log/GraphLog'], function (app) {
    app.controller('DeviceLogController', function ($routeParams, domoticzApi, deviceApi) {
        var vm = this;

        vm.isTextLog = isTextLog;
        vm.isLightLog = isLightLog;
        vm.isGraphLog = isGraphLog;
        vm.isTemperatureLog = isTemperatureLog;
        vm.isReportAvailable = isReportAvailable;

        init();

        function init() {
            vm.deviceIdx = $routeParams.id;

            deviceApi.getDeviceInfo(vm.deviceIdx).then(function (device) {
                vm.device = device;
                vm.pageName = device.Name;

                if (isSmartLog()) {
                    ShowSmartLog('.js-device-log-content', 'ShowUtilities', device.idx, device.Name, device.SwitchTypeVal);
                } else if (isCounterLogSpline()) {
                    ShowCounterLogSpline('.js-device-log-content', 'ShowUtilities', device.idx, device.Name, device.SwitchTypeVal);
                } else if (isCounterLog()) {
                    ShowCounterLog('.js-device-log-content', 'ShowUtilities', device.idx, device.Name, device.SwitchTypeVal);
                }
            });
        }

        function isTextLog() {
            if (!vm.device) {
                return undefined;
            }

            return ['Text', 'Alert'].includes(vm.device.SubType)
                || vm.device.SwitchType === 'Media Player';
        }

        function isLightLog() {
            if (!vm.device) {
                return undefined;
            }

            if (vm.device.Type === 'Heating') {
                return ((vm.device.SubType != 'Zone') && (vm.device.SubType != 'Hot Water'));
            }

            var isLightType = [
				'Lighting 1', 'Lighting 2', 'Lighting 3', 'Lighting 4', 'Lighting 5',
                'Light', 'Light/Switch', 'Color Switch', 'Chime',
                'Security', 'RFY', 'ASA', 'Blinds'
            ].includes(vm.device.Type);

            var isLightSwitchType = [
                'Contact', 'Door Contact', 'Doorbell', 'Dusk Sensor', 'Motion Sensor',
                'Smoke Detector', 'On/Off', 'Dimmer'
            ].includes(vm.device.SwitchType);

            return (isLightType || isLightSwitchType) && !isTextLog();
        }

        function isTemperatureLog() {
            if (!vm.device) {
                return undefined;
            }

            if (vm.device.Type === 'Heating') {
                return ((vm.device.SubType === 'Zone') || (vm.device.SubType === 'Hot Water'));
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

            return vm.device.Type === 'Usage' || vm.device.Type === 'Weight' || [
                'Voltage', 'Current', 'Pressure', 'Custom Sensor',
                'Sound Level', 'Solar Radiation', 'Visibility', 'Distance',
                'Soil Moisture', 'Leaf Wetness', 'Waterflow', 'Lux', 'Percentage'
            ].includes(vm.device.SubType)
        }

        function isSmartLog() {
            if (!vm.device) {
                return undefined;
            }

            return (vm.device.Type == 'P1 Smart Meter' && vm.device.SubType == 'Energy')
        }

        function isCounterLog() {
            if (!vm.device) {
                return undefined;
            }

            if (isCounterLogSpline() || isSmartLog()) {
                return false;
            }

            return vm.device.Type === 'RFXMeter'
                || (vm.device.Type == 'P1 Smart Meter' && vm.device.SubType == 'Gas')
                || (typeof vm.device.Counter != 'undefined' && !isCounterLogSpline());
        }

        function isCounterLogSpline() {
            if (!vm.device) {
                return undefined;
            }

            return ['Power', 'Energy'].includes(vm.device.Type)
                || ['kWh'].includes(vm.device.SubType)
                || (vm.device.Type == 'YouLess Meter' && [0, 4].includes(vm.device.SwitchTypeVal));
        }

        function isReportAvailable() {
            if (!vm.device) {
                return undefined;
            }

            return isTemperatureLog()
                || ((isCounterLogSpline() || isCounterLog() || isSmartLog()) && [0, 1, 2, 3, 4].includes(vm.device.SwitchTypeVal));
        }
    });
});
