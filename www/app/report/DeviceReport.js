define(['app', 'report/CounterReport', 'report/TemperatureReport', 'report/EnergyMultiCounterReport'], function (app) {
    app.controller('DeviceReportController', function ($route, $routeParams, $location, deviceApi) {
        var vm = this;
        vm.isTemperatureReport = isTemperatureReport;
        vm.isCounterReport = isCounterReport;
        vm.isOnlyUsage = isOnlyUsage;
        vm.isEnergyMultiCounterReport = isEnergyMultiCounterReport;
        vm.isNoReport = isNoReport;
        vm.getYearsOptions = getYearsOptions;
        vm.selectYear = selectYear;

        init();

        function init() {
            vm.deviceIdx = $routeParams.id;

            vm.selectedYear = parseInt($routeParams.year) || ((new Date()).getFullYear());
            vm.selectedMonth = parseInt($routeParams.month) || undefined;
            vm.isMonthView = vm.selectedMonth > 0;

            deviceApi.getDeviceInfo(vm.deviceIdx).then(function (device) {
                var monthNames = ["January", "February", "March", "April", "May", "June",
                    "July", "August", "September", "October", "November", "December"];

                vm.device = device;

                vm.pageName = vm.isMonthView
                    ? vm.device.Name + ', ' + $.t(monthNames[vm.selectedMonth - 1]) + ' ' + vm.selectedYear
                    : vm.device.Name + ' ' + vm.selectedYear;
            });
        }

        function selectYear() {
            $route.updateParams({ year: vm.selectedYear });
            $location.replace();
        }

        function getYearsOptions() {
            var currentYear = ((new Date()).getFullYear());
            var years = [];

            for (var i = 2012; i <= currentYear; i++) {
                years.push(i);
            }

            return years;
        }

        function isTemperatureReport() {
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

        function isCounterReport() {
            if (!vm.device) {
                return undefined;
            }
            return ['Power', 'Energy', 'RFXMeter'].includes(vm.device.Type)
                || isOnlyUsage()
                || ['kWh'].includes(vm.device.SubType)
                || ['YouLess counter'].includes(vm.device.SubType)
                || ['Counter Incremental'].includes(vm.device.SubType)
                || (vm.device.Type === 'P1 Smart Meter' && vm.device.SubType !== 'Energy');
        }

        function isOnlyUsage() {
            if (!vm.device) {
                return undefined;
            }
            return ['Managed Counter'].includes(vm.device.SubType);
        }

        function isEnergyMultiCounterReport() {
            if (!vm.device) {
                return undefined;
            }

            return (vm.device.Type === 'P1 Smart Meter' && vm.device.SubType === 'Energy')
        }

        function isNoReport() {
            if (!vm.device) {
                return undefined;
            }

            return !isTemperatureReport() && !isCounterReport() && !isEnergyMultiCounterReport()
        }
    });
});
