define(['app', 'log/Chart', 'log/TextLog', 'log/TemperatureLog', 'log/LightLog', 'log/GraphLog', 'log/CounterLog', 'log/CounterLogCounter', 'log/CounterLogInstantAndCounter', 'log/CounterLogP1Energy', 'log/RainLog', 'log/SetpointLog', 'log/AirQualityLog', 'log/BarometerLog', 'log/WindLog', 'log/UVLog', 'log/FanLog', 'log/CurrentLog'], function (app) {
    app.controller('DeviceLogController', function ($location, $routeParams, domoticzApi, deviceApi, chart) {
        var vm = this;

        vm.isTextLog = isTextLog;
        vm.isLightLog = isLightLog;
        vm.isGraphLog = isGraphLog;
		vm.isRainLog = isRainLog;
        vm.isTemperatureLog = isTemperatureLog;
        vm.isSetpointLog = isSetpointLog;
		vm.isAirQualityLog = isAirQualityLog;
		vm.isBarometerLog = isBarometerLog;
		vm.isWindLog = isWindLog;
		vm.isUvLog = isUvLog;
		vm.isFanLog = isFanLog;
		vm.isCurrentLog = isCurrentLog;
        vm.isReportAvailable = isReportAvailable;
        vm.isInstantAndCounterLog = isInstantAndCounterLog;
        vm.isP1EnergyLog = isP1EnergyLog;
        vm.isCounterLog = isCounterLog;
        vm.isEnergyUsedDevice = isEnergyUsedDevice;
        vm.isGasDevice = isGasDevice;
        vm.isWaterDevice = isWaterDevice;
        vm.isCounterDevice = isCounterDevice;
        vm.isEnergyGeneratedDevice = isEnergyGeneratedDevice;
        vm.isTimeDevice = isTimeDevice;

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

            return ['Text', 'Alert'].includes(vm.device.SubType)
                || vm.device.SwitchType === 'Media Player';
        }

        function isLightLog() {
            if (!vm.device) {
                return undefined;
            }

            if (vm.device.Type === 'Heating') {
                return ((vm.device.SubType !== 'Zone') && (vm.device.SubType !== 'Hot Water'));
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
            // Exclude Wind sensors as they now have their own log
            if (isWindLog()) {
                return false;
            }
            return (/Temp|Thermostat|Humidity|RFXSensor|Radiator/i).test(vm.device.Type)
        }

        function isSetpointLog() {
            if (!vm.device) {
                return undefined;
            }
            return (/Setpoint/i).test(vm.device.Type)
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

        function isRainLog() {
            if (!vm.device) {
                return undefined;
            }
            return (vm.device.Type === 'Rain');
        }

		function isAirQualityLog() {
			if (!vm.device) {
				return undefined;
			}
			return (vm.device.Type === 'Air Quality');
		}

		function isBarometerLog() {
			if (!vm.device) {
				return undefined;
			}
			return (vm.device.SubType === 'Barometer');
		}

		function isWindLog() {
			if (!vm.device) {
				return undefined;
			}
			// Wind devices have a Direction property
			return (vm.device.Direction !== undefined && /Wind/i.test(vm.device.Type));
		}

		function isUvLog() {
			if (!vm.device) {
				return undefined;
			}
			// UV devices have a UVI property
			return (vm.device.UVI !== undefined);
		}

		function isFanLog() {
			if (!vm.device) {
				return undefined;
			}
			return (vm.device.SubType === 'Fan');
		}

		function isCurrentLog() {
			if (!vm.device) {
				return undefined;
			}
			return (vm.device.Type === 'Current');
		}

        function isP1EnergyLog() {
            if (!vm.device) {
                return undefined;
            }

            return (vm.device.Type === 'P1 Smart Meter' && vm.device.SubType === 'Energy')
        }

        function isCounterLog() {
            if (!vm.device) {
                return undefined;
            }
            if (isP1EnergyLog()) {
                return false;
            }
            if (isInstantAndCounterLog()) {
                return false;
            }

            return vm.device.Type === 'RFXMeter'
                || (vm.device.Type === 'P1 Smart Meter' && vm.device.SubType === 'Gas')
                || (typeof vm.device.Counter != 'undefined' && !isInstantAndCounterLog());
        }

        function isEnergyUsedDevice() {
            return vm.device.SwitchTypeVal === chart.deviceTypes.EnergyUsed;
        }

        function isGasDevice() {
            return vm.device.SwitchTypeVal === chart.deviceTypes.Gas;
        }

        function isWaterDevice() {
            return vm.device.SwitchTypeVal === chart.deviceTypes.Water;
        }

        function isCounterDevice() {
            return vm.device.SwitchTypeVal === chart.deviceTypes.Counter;
        }

        function isEnergyGeneratedDevice() {
            return vm.device.SwitchTypeVal === chart.deviceTypes.EnergyGenerated;
        }

        function isTimeDevice() {
            return vm.device.SwitchTypeVal === chart.deviceTypes.Time;
        }

        function isInstantAndCounterLog() {
            if (!vm.device) {
                return undefined;
            }
            if (isP1EnergyLog()) {
                return false;
            }

            return ['Power', 'Energy'].includes(vm.device.Type)
                || ['kWh'].includes(vm.device.SubType)
                || (vm.device.Type === 'YouLess Meter' && [0, 4].includes(vm.device.SwitchTypeVal));
        }

        function isReportAvailable() {
            if (!vm.device) {
                return undefined;
            }

            return isTemperatureLog()
                || ((isInstantAndCounterLog() || isCounterLog() || isP1EnergyLog()) && [0, 1, 2, 3, 4].includes(vm.device.SwitchTypeVal));
        }
    });
});
