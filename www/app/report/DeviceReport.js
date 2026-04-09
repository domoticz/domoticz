define(['app', 'report/CounterReport', 'report/TemperatureReport', 'report/EnergyMultiCounterReport', 'report/RainReport', 'report/WindReport'], function (app) {
    app.controller('DeviceReportController', function ($route, $routeParams, $location, deviceApi) {
        var vm = this;
        vm.isTemperatureReport = isTemperatureReport;
        vm.isCounterReport = isCounterReport;
        vm.isOnlyUsage = isOnlyUsage;
        vm.isEnergyMultiCounterReport = isEnergyMultiCounterReport;
        vm.isRainReport = isRainReport;
        vm.isWindReport = isWindReport;
        vm.isNoReport = isNoReport;
        vm.selectYear = selectYear;
        vm.isCustomYear = function () { return vm.selectedYear === 'custom'; };
        vm.applyCustomDate = function () {
            var val = vm._datePickerModel;
            if (val instanceof Date && !isNaN(val.getTime())) {
                val = localDateISO(val);
            }
            if (!val || !/^\d{4}-\d{2}-\d{2}$/.test(val)) { return; }
            vm.customStartDate = val;
            localStorage.setItem('dz_report_custom_start_' + vm.deviceIdx, val);
            $route.updateParams({ year: 'custom-' + val });
            $location.replace();
        };

        function localDateISO(d) {
            var y  = d.getFullYear();
            var mo = String(d.getMonth() + 1).padStart(2, '0');
            var dy = String(d.getDate()).padStart(2, '0');
            return y + '-' + mo + '-' + dy;
        }

        init();

        function formatDateDisplay(isoDate) {
            if (!isoDate || isoDate.length < 10) { return isoDate; }
            var p = isoDate.split('-');
            var months = ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];
            return p[2] + ' ' + $.t(months[parseInt(p[1], 10) - 1]) + ' ' + p[0];
        }

        function addOneYear(dateStr) {
            var d = new Date(dateStr + 'T00:00:00');
            var origMonth = d.getMonth();
            var origDay   = d.getDate();
            d.setFullYear(d.getFullYear() + 1);
            if (origMonth === 1 && origDay === 29 && d.getMonth() === 2) {
                d.setDate(28);
            }
            return d.getFullYear() + '-'
                 + String(d.getMonth() + 1).padStart(2, '0') + '-'
                 + String(d.getDate()).padStart(2, '0');
        }

        function init() {
            vm.yearsOptions = getYearsOptions();
            vm.maxDate = new Date().toISOString().slice(0, 10);

            vm.deviceIdx = $routeParams.id;

            var yearParam = $routeParams.year || '';
            if (yearParam.indexOf('custom-') === 0) {
                var dateStr = yearParam.slice(7);
                if (/^\d{4}-\d{2}-\d{2}$/.test(dateStr)) {
                    vm.selectedYear      = 'custom';
                    vm.customStartDate   = dateStr;
                    vm._datePickerModel  = new Date(dateStr + 'T00:00:00Z');  // UTC midnight — matches what AngularJS date input expects
                    localStorage.setItem('dz_report_custom_start_' + vm.deviceIdx, dateStr);
                } else {
                    vm.selectedYear      = (new Date()).getFullYear();
                    vm.customStartDate   = '';
                    vm._datePickerModel  = null;
                }
            } else {
                vm.selectedYear      = parseInt(yearParam) || (new Date()).getFullYear();
                vm.customStartDate   = '';
                vm._datePickerModel  = null;
                var stored = localStorage.getItem('dz_report_custom_start_' + vm.deviceIdx);
                if (stored && /^\d{4}-\d{2}-\d{2}$/.test(stored)) {
                    vm._datePickerModel = new Date(stored + 'T00:00:00Z');
                    // Don't auto-navigate — user must press Set
                }
            }
            vm.selectedMonth = parseInt($routeParams.month) || undefined;
            vm.isMonthView = vm.selectedMonth > 0;

            deviceApi.getDeviceInfo(vm.deviceIdx).then(function (device) {
                var monthNames = ["January", "February", "March", "April", "May", "June",
                    "July", "August", "September", "October", "November", "December"];

                vm.device = device;

                var csDate = vm.customStartDate instanceof Date
                    ? localDateISO(vm.customStartDate)
                    : vm.customStartDate;
                vm.pageName = vm.isMonthView
                    ? vm.device.Name + ', ' + $.t(monthNames[vm.selectedMonth - 1]) + ' ' + vm.selectedYear
                    : (csDate
                        ? vm.device.Name + ' ' + formatDateDisplay(csDate) + ' \u2013 ' + formatDateDisplay(addOneYear(csDate))
                        : vm.device.Name + ' ' + vm.selectedYear);
            });
        }

        function selectYear() {
            if (vm.selectedYear === 'custom') {
                var val = vm.customStartDate;
                if (val instanceof Date) { val = localDateISO(val); }
                if (!val) { return; }
                $route.updateParams({ year: 'custom-' + val });
            } else {
                $route.updateParams({ year: vm.selectedYear });
            }
            $location.replace();
        }

        function getYearsOptions() {
            var currentYear = (new Date()).getFullYear();
            var years = [{ label: 'Custom', value: 'custom' }];
            for (var i = currentYear; i >= 2012; i--) {
                years.push({ label: String(i), value: i });
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

            // Exclude pure Wind sensors (no Temp data) as they have their own report
            if (isWindReport() && vm.device.Temp === undefined) {
                return false;
            }

            return (/Temp|Thermostat|Humidity|Radiator/i).test(vm.device.Type)
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

        function isRainReport() {
            if (!vm.device) {
                return undefined;
            }
            return vm.device.Type === 'Rain';
        }

        function isWindReport() {
            if (!vm.device) {
                return undefined;
            }
            return (vm.device.Direction !== undefined && /Wind/i.test(vm.device.Type));
        }

        function isNoReport() {
            if (!vm.device) {
                return undefined;
            }

            return !isTemperatureReport() && !isCounterReport() && !isEnergyMultiCounterReport() && !isRainReport() && !isWindReport();
        }
    });
});
