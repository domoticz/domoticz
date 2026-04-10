define(['app'], function (app) {
    app.directive('dzUtilityWidget', function ($rootScope, $window, $sce, deviceApi, permissions) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                device: '<',
                dashboardType: '<',
                viewMode: '@',
                onUpdate: '&'
            },
            templateUrl: function (elem, attrs) {
                if (attrs.viewMode === 'tab') {
                    return 'views/widgets/utility_widget_tab.html';
                }
                var isMobile = window.myglobals && window.myglobals.ismobile;
                var dashboardType = window.myglobals && window.myglobals.DashboardType;
                if (isMobile || dashboardType == 2) {
                    return 'views/widgets/utility_widget_mobile.html';
                }
                return 'views/widgets/utility_widget.html';
            },
            controllerAs: 'ctrl',
            controller: function ($scope, $element) {
                var ctrl = this;
                var device = $scope.device;

                ctrl.device = device;
                ctrl.isMobile = window.myglobals && window.myglobals.ismobile;
                ctrl.dashboardType = $scope.dashboardType || (window.myglobals && window.myglobals.DashboardType);

                // Keep ctrl.device in sync when parent updates the binding
                $scope.$watch('device', function (newVal) {
                    if (newVal) {
                        device = newVal;
                        ctrl.device = newVal;
                    }
                });

                ctrl.getBackgroundClass = function () {
                    return $rootScope.GetItemBackgroundStatus(device);
                };

                ctrl.getSpanClass = function () {
                    if (ctrl.dashboardType == 1) {
                        return 'span3';
                    }
                    return 'span4';
                };

                ctrl.displayTrend = $rootScope.DisplayTrend;
                ctrl.trendState = $rootScope.TrendState;

                ctrl.isCounter = function () {
                    return typeof device.Counter !== 'undefined';
                };

                ctrl.isEnergy = function () {
                    return device.Type === 'Energy' || device.Type === 'Current/Energy' || device.Type === 'Power' || device.SubType === 'kWh';
                };

                ctrl.isSetpoint = function () {
                    return device.Type === 'Setpoint' && device.SubType === 'SetPoint';
                };

                ctrl.isThermostat6 = function () {
                    return device.Type === 'Thermostat 6';
                };

                ctrl.isText = function () {
                    return device.SubType === 'Text';
                };

                ctrl.isAlert = function () {
                    return device.SubType === 'Alert';
                };

                ctrl.isThermostatClock = function () {
                    return device.SubType === 'Thermostat Clock';
                };

                ctrl.isThermostatMode = function () {
                    return device.SubType === 'Thermostat Mode';
                };

                ctrl.isThermostatFanMode = function () {
                    return device.SubType === 'Thermostat Fan Mode';
                };

                ctrl.isThermostatOperatingState = function () {
                    return device.SubType === 'Thermostat Operating State';
                };

                ctrl.getBigText = function () {
                    var bigtext = '';

                    if ((typeof device.Usage !== 'undefined') && (typeof device.UsageDeliv === 'undefined')) {
                        bigtext = device.Usage;
                    } else if ((typeof device.Usage !== 'undefined') && (typeof device.UsageDeliv !== 'undefined')) {
                        if ((device.UsageDeliv.charAt(0) == 0) || (parseInt(device.Usage) != 0)) {
                            bigtext = device.Usage;
                        }
                        if (device.UsageDeliv.charAt(0) != 0) {
                            if (parseInt(device.Usage) > 0) {
                                bigtext += ', ';
                            }
                            bigtext += '-' + device.UsageDeliv;
                        }
                    } else if (ctrl.isCounter()) {
                        if ((device.SubType === 'Gas') || (device.SubType === 'RFXMeter counter') || (device.SubType === 'Counter Incremental')) {
                            bigtext = device.CounterToday;
                        } else if (device.SubType === 'Managed Counter') {
                            bigtext = device.Counter;
                        }
                    } else if (ctrl.isSetpoint()) {
                        bigtext = device.Data + ' ' + device.vunit;
                    } else if (device.Type === 'Radiator 1') {
                        bigtext = device.Data + '\u00B0 ' + $scope.$parent.config.TempSign;
                    } else if (ctrl.isThermostatMode() || ctrl.isThermostatFanMode() || ctrl.isThermostatOperatingState()) {
                        bigtext = device.Data;
                    } else if (typeof device.Direction !== 'undefined') {
                        var windSign = ($.myglobals && $.myglobals.windsign) ? $.myglobals.windsign : 'm/s';
                        bigtext = device.DirectionStr || '';
                        if (typeof device.Speed !== 'undefined') {
                            bigtext += ' / ' + device.Speed + ' ' + windSign;
                        } else if (typeof device.Gust !== 'undefined') {
                            bigtext += ' / ' + device.Gust + ' ' + windSign;
                        }
                    } else if (!ctrl.isText() && !ctrl.isAlert() && typeof device.Data !== 'undefined') {
                        bigtext = device.Data;
                    }

                    return bigtext;
                };

                ctrl.getStatusText = function () {
                    var status = '';

                    if (ctrl.isCounter()) {
                        if ((device.SubType === 'Gas') || (device.SubType === 'RFXMeter counter') || (device.SubType === 'Counter Incremental')) {
                            status = device.Counter;
                        } else if (device.SubType !== 'Managed Counter') {
                            if (device.Type === 'P1 Smart Meter') {
                                status = $.t('Today') + ': ' + device.CounterToday + ', ' + device.Counter;
                            } else {
                                status = $.t('Today') + ': ' + device.CounterToday + ', ' + device.Counter;
                            }
                        }
                    } else if (ctrl.isEnergy()) {
                        if (typeof device.CounterToday !== 'undefined') {
                            status = $.t('Today') + ': ' + device.CounterToday;
                        }
                    } else if (device.Type === 'Air Quality') {
                        status = device.Quality;
                    } else if (device.SubType === 'Soil Moisture') {
                        status = device.Desc;
                    } else if (ctrl.isText()) {
                        var sanitized = device.Data.replace(/([^>\r\n]?)(\r\n|\n\r|\r|\n)/g, '$1<br />$2');
                        return $sce.trustAsHtml(DOMPurify.sanitize(sanitized, {
                            ALLOWED_TAGS: ['br', 'span', 'font', 'a', 'b', 'i', 'u'],
                            ALLOWED_ATTR: ['style', 'color', 'href', 'target']
                        }));
                    } else if (ctrl.isAlert()) {
                        var sanitized = device.Data.replace(/([^>\r\n]?)(\r\n|\n\r|\r|\n)/g, '$1<br />$2');
                        return $sce.trustAsHtml(DOMPurify.sanitize(sanitized, {
                            ALLOWED_TAGS: ['br', 'span', 'font', 'a', 'b', 'i', 'u'],
                            ALLOWED_ATTR: ['style', 'color', 'href', 'target']
                        }));
                    } else if (typeof device.Direction !== 'undefined') {
                        var windSign = ($.myglobals && $.myglobals.windsign) ? $.myglobals.windsign : 'm/s';
                        var tempSign = ($rootScope.config && $rootScope.config.TempSign) ? $rootScope.config.TempSign : 'C';
                        status = device.Direction + ' ' + (device.DirectionStr || '');
                        if (typeof device.Speed !== 'undefined') {
                            status += ', ' + $.t('Speed') + ': ' + device.Speed + ' ' + windSign;
                        }
                        if (typeof device.Gust !== 'undefined') {
                            status += ', ' + $.t('Gust') + ': ' + device.Gust + ' ' + windSign;
                        }
                        if (typeof device.Temp !== 'undefined') {
                            status += '<br>' + $.t('Temp') + ': ' + device.Temp + '\u00B0 ' + tempSign;
                            if (typeof device.Chill !== 'undefined') {
                                status += ', ' + $.t('Chill') + ': ' + device.Chill + '\u00B0 ' + tempSign;
                            }
                        }
                    }

                    if (typeof device.CounterDeliv !== 'undefined' && device.CounterDeliv != 0) {
                        if (device.Type === 'P1 Smart Meter') {
                            status += '<br>' + $.t('Return') + ': ' + device.CounterDelivToday + ', ' + device.CounterDeliv;
                        } else {
                            status += '<br>' + $.t('Return') + ': ' + $.t('Today') + ': ' + device.CounterDelivToday + ', ' + device.CounterDeliv;
                        }
                    }

                    return status;
                };

                ctrl.getMobileText = function () {
                    if (ctrl.isText() || ctrl.isAlert()) {
                        // Render data as HTML: supports <br />, <b>, <a> etc. in device data
                        // Also convert raw newlines to <br /> for plain-text data
                        var text = device.Data.replace(/([^>\r\n]?)(\r\n|\n\r|\r|\n)/g, '$1<br />$2');
                        if (ctrl.isAlert()) {
                            // Alert images exist for levels 0–4 (Alert48_0.png .. Alert48_4.png)
                            var aLevel = Math.min(parseInt(device.Level) || 0, 4);
                            text += ' <img src="images/Alert48_' + aLevel + '.png" height="16" width="16">';
                        }
                        var allowedTags = ['br', 'span', 'font', 'a', 'b', 'i', 'u'];
                        var allowedAttr = ['style', 'color', 'href', 'target'];
                        if (ctrl.isAlert()) {
                            allowedTags.push('img');
                            allowedAttr.push('src', 'height', 'width');
                        }
                        return $sce.trustAsHtml(DOMPurify.sanitize(text, {
                            ALLOWED_TAGS: allowedTags,
                            ALLOWED_ATTR: allowedAttr
                        }));
                    }
                    if (ctrl.isCounter() && device.Type === 'P1 Smart Meter') {
                        var text = '';
                        if (typeof device.CounterToday !== 'undefined') {
                            text = $.t('Usage') + ': ' + device.CounterToday;
                        }
                        if (typeof device.CounterDeliv !== 'undefined' && device.CounterDeliv != 0) {
                            text += '<br />' + $.t('Return') + ': ' + device.CounterDelivToday;
                        }
                        if (typeof device.Usage !== 'undefined') {
                            var actual = device.Usage;
                            if (typeof device.UsageDeliv !== 'undefined' && parseInt(device.UsageDeliv) > 0) {
                                actual += ', -' + device.UsageDeliv;
                            }
                            text += '<br />' + $.t('Actual') + ': ' + actual;
                        }
                        return text;
                    }
                    var bigtext = ctrl.getBigText();
                    var status = ctrl.getStatusText();
                    if (bigtext && status) return bigtext + '<br />' + status;
                    return bigtext || status;
                };

                ctrl.getDeviceIcon = function () {
                    var image = '';

                    if (ctrl.isCounter()) {
                        if (device.SwitchTypeVal == 1) {
                            image = (device.CustomImage == 0) ? 'Gas48.png' : device.Image + '48_On.png';
                        } else if (device.SwitchTypeVal == 2) {
                            image = (device.CustomImage == 0) ? 'Water48_On.png' : device.Image + '48_On.png';
                        } else if (device.SwitchTypeVal == 3) {
                            image = (device.CustomImage == 0) ? 'Counter48.png' : device.Image + '48_On.png';
                        } else if (device.SwitchTypeVal == 4) {
                            image = (device.CustomImage == 0) ? 'PV48.png' : device.Image + '48_On.png';
                        } else {
                            image = (device.CustomImage == 0) ? 'Counter48.png' : device.Image + '48_On.png';
                        }
                    } else if (device.Type === 'Current') {
                        image = (device.CustomImage == 0) ? 'current48.png' : device.Image + '48_On.png';
                    } else if (ctrl.isEnergy()) {
                        if (((device.Type === 'Energy') || (device.SubType === 'kWh')) && (device.SwitchTypeVal == 4)) {
                            image = (device.CustomImage == 0) ? 'PV48.png' : device.Image + '48_On.png';
                        } else {
                            image = (device.CustomImage == 0) ? 'current48.png' : device.Image + '48_On.png';
                        }
                    } else if (device.Type === 'Air Quality') {
                        image = (device.CustomImage == 0) ? 'air48.png' : device.Image + '48_On.png';
                    } else if (device.SubType === 'Custom Sensor') {
                        image = device.Image + '48_On.png';
                    } else if (device.SubType === 'Soil Moisture') {
                        image = 'moisture48.png';
                    } else if (device.SubType === 'Percentage') {
                        image = (device.CustomImage == 0) ? 'Percentage48.png' : device.Image + '48_On.png';
                    } else if (device.SubType === 'Fan') {
                        image = 'Fan48_On.png';
                    } else if (device.SubType === 'Leaf Wetness') {
                        image = (device.CustomImage == 0) ? 'leaf48.png' : device.Image + '48_On.png';
                    } else if (device.SubType === 'Distance') {
                        image = (device.CustomImage == 0) ? 'visibility48.png' : device.Image + '48_On.png';
                    } else if (device.SubType === 'Voltage' || device.SubType === 'Current' || device.SubType === 'A/D') {
                        image = (device.CustomImage == 0) ? 'current48.png' : device.Image + '48_On.png';
                    } else if (ctrl.isText()) {
                        image = (device.CustomImage == 0) ? 'text48.png' : device.Image + '48_On.png';
                    } else if (ctrl.isAlert()) {
                        var aLevel = device.Level;
                        if (aLevel > 4) aLevel = 4;
                        image = 'Alert48_' + aLevel + '.png';
                    } else if (device.SubType === 'Pressure') {
                        image = (device.CustomImage == 0) ? 'gauge48.png' : device.Image + '48_On.png';
                    } else if (device.Type === 'Lux') {
                        image = (device.CustomImage == 0) ? 'lux48.png' : device.Image + '48_On.png';
                    } else if (device.Type === 'Weight') {
                        image = (device.CustomImage == 0) ? 'scale48.png' : device.Image + '48_On.png';
                    } else if (device.Type === 'Usage') {
                        image = (device.CustomImage == 0) ? 'current48.png' : device.Image + '48_On.png';
                    } else if (ctrl.isSetpoint() || device.Type === 'Radiator 1') {
                        image = (device.CustomImage == 0) ? 'override.png' : device.Image + '48_On.png';
                    } else if (ctrl.isThermostatClock()) {
                        image = 'clock48.png';
                    } else if (ctrl.isThermostatMode()) {
                        image = 'mode48.png';
                    } else if (ctrl.isThermostatFanMode()) {
                        image = 'mode48.png';
                    } else if (ctrl.isThermostatOperatingState()) {
                        image = 'mode48.png';
                    } else if (device.SubType === 'Sound Level') {
                        image = (device.CustomImage == 0) ? 'Speaker48_On.png' : device.Image + '48_On.png';
                    } else if (device.SubType === 'Waterflow') {
                        image = (device.CustomImage == 0) ? 'moisture48.png' : device.Image + '48_On.png';
                    } else if (typeof device.Direction !== 'undefined') {
                        image = device.DirectionStr ? 'Wind' + device.DirectionStr + '.png' : 'wind48.png';
                    } else if (typeof device.Temp !== 'undefined' || typeof device.Chill !== 'undefined') {
                        // Temperature / weather devices: use temperature-range icon
                        var tempVal = typeof device.Temp !== 'undefined' ? device.Temp : device.Chill;
                        image = (typeof GetTemp48Item === 'function') ? GetTemp48Item(tempVal) : 'Temp-48_On.png';
                    } else if (device.Type === 'Humidity') {
                        image = 'gauge48.png';
                    } else if (typeof device.Rain !== 'undefined') {
                        image = 'Rain48_On.png';
                    } else if (typeof device.UVI !== 'undefined') {
                        image = 'uv48.png';
                    } else if (typeof device.Visibility !== 'undefined') {
                        image = (device.CustomImage == 0) ? 'visibility48.png' : device.Image + '48_On.png';
                    } else if (typeof device.Radiation !== 'undefined') {
                        image = (device.CustomImage == 0) ? 'radiation48.png' : device.Image + '48_On.png';
                    } else {
                        image = (device.CustomImage == 0) ? 'current48.png' : device.Image + '48_On.png';
                    }

                    return 'images/' + image;
                };

                ctrl.showSetpointPopup = function (event) {
                    var step = device.step || 0.5;
                    var min = device.min || -200;
                    var max = device.max || 200;
                    ShowSetpointPopup(event, device.idx, device.Protected, device.Data, false, step, min, max);
                };

                ctrl.makeFavorite = function (isFavorite) {
                    deviceApi.makeFavorite(device.idx, isFavorite).then(function () {
                        if ($scope.onUpdate) {
                            $scope.onUpdate();
                        }
                    });
                };

                ctrl.isAdmin = permissions.hasPermission('Admin');

                ctrl.getTypeSubTypeText = function() {
                    return device.Type + ', ' + device.SubType;
                };

                ctrl.searchText = '';
                ctrl.updateSearchText = function() {
                    if (!device) return;
                    ctrl.searchText = GenerateLiveSearchTextL(device, ctrl.getBigText());
                };
                ctrl.updateSearchText();

                ctrl.getGraphLogLink = function() {
                    return '#/Devices/' + device.idx + '/Log';
                };

                ctrl.hasTimers = function() {
                    return device.Timers === 'true' || device.Timers === true;
                };

                ctrl.showNotifications = function() {
                    return device.ShowNotifications === true;
                };

                ctrl.hasNotifications = function() {
                    return device.Notifications === 'true' || device.Notifications === true;
                };

                ctrl.getNotificationLink = function() {
                    return '#/Devices/' + device.idx + '/Notifications';
                };

                ctrl.getTimerLink = function() {
                    return '#/Devices/' + device.idx + '/Timers';
                };

                // Edit function dispatching - calls the appropriate global edit function
                ctrl.editDevice = function() {
                    if (!permissions.hasPermission('Admin')) return;

                    if (typeof device.Counter !== 'undefined') {
                        if (device.Type === 'P1 Smart Meter') {
                            EditUtilityDevice(device.idx, escape(device.Name), escape(device.Description), device.CustomImage, device.ID, device.Unit);
                        } else {
                            EditMeterDevice(device.idx, escape(device.Name), escape(device.Description), device.SwitchTypeVal, device.AddjValue, device.AddjValue2, escape(device.ValueQuantity), escape(device.ValueUnits), device.CustomImage, device.ID, device.Unit);
                        }
                    } else if (device.SubType === 'Custom Sensor') {
                        EditCustomSensorDevice(device.idx, escape(device.Name), escape(device.Description), device.CustomImage, device.SensorType, escape(device.SensorUnit), device.ID, device.Unit);
                    } else if (device.SubType === 'Text') {
                        EditTextDevice(device.idx, escape(device.Name), escape(device.Data), escape(device.Description), device.CustomImage, device.ID, device.Unit);
                    } else if ((device.Type === 'Setpoint' && device.SubType === 'SetPoint') || device.Type === 'Radiator 1') {
                        EditSetPoint(device.idx, escape(device.Name), escape(device.Description), escape(device.vunit), device.step, device.min, device.max, device.Protected, device.CustomImage, device.ID, device.Unit);
                    } else if (device.SubType === 'Thermostat Clock') {
                        EditThermostatClock(device.idx, escape(device.Name), escape(device.Description), device.DayTime, device.Protected, device.CustomImage, device.ID, device.Unit);
                    } else if (device.SubType === 'Thermostat Mode') {
                        EditThermostatMode(device.idx, escape(device.Name), escape(device.Description), device.Mode, device.Modes, device.Protected, device.CustomImage, device.ID, device.Unit);
                    } else if (device.SubType === 'Thermostat Fan Mode') {
                        EditThermostatFanMode(device.idx, escape(device.Name), escape(device.Description), device.Mode, device.Modes, device.Protected, device.CustomImage, device.ID, device.Unit);
                    } else if ((device.Type === 'Energy' || device.SubType === 'kWh')) {
                        var energyMeterMode = device.EnergyMeterMode || '0';
                        EditEnergyDevice(device.idx, escape(device.Name), escape(device.Description), device.SwitchTypeVal, energyMeterMode, device.CustomImage, device.ID, device.Unit);
                    } else if (device.SubType === 'Distance') {
                        EditDistanceDevice(device.idx, escape(device.Name), escape(device.Description), device.SwitchTypeVal, device.CustomImage, device.ID, device.Unit);
                    } else {
                        EditUtilityDevice(device.idx, escape(device.Name), escape(device.Description), device.CustomImage, device.ID, device.Unit);
                    }
                };

                // Determine if this device type supports a log link
                ctrl.hasLogLink = function() {
                    if (typeof device.Counter !== 'undefined') return true;
                    if (device.Type === 'Air Quality') return true;
                    if (device.SubType === 'Custom Sensor') return true;
                    if (device.SubType === 'Percentage' || device.SubType === 'Fan') return true;
                    if (device.SubType === 'Soil Moisture' || device.SubType === 'Leaf Wetness' || device.SubType === 'Waterflow') return true;
                    if (device.Type === 'Lux' || device.Type === 'Weight' || device.Type === 'Usage') return true;
                    if (device.Type === 'Energy' || device.SubType === 'kWh' || device.Type === 'Power') return true;
                    if (device.Type === 'Current' || device.Type === 'Current/Energy') return true;
                    if ((device.Type === 'Setpoint' && device.SubType === 'SetPoint') || device.Type === 'Radiator 1') return true;
                    if (device.SubType === 'Text') return true;
                    if (device.SubType === 'Alert') return true;
                    if (device.SubType === 'Sound Level') return true;
                    if (device.Type === 'General' && (device.SubType === 'Voltage' || device.SubType === 'Current' || device.SubType === 'Pressure' || device.SubType === 'Distance')) return true;
                    if (device.SubType === 'Voltage' || device.SubType === 'Current' || device.SubType === 'A/D') return true;
                    return false;
                };

                // Setpoint/Radiator need timers
                ctrl.hasTimerSupport = function() {
                    return (device.Type === 'Setpoint' && device.SubType === 'SetPoint') || device.Type === 'Radiator 1';
                };

                $scope.$watch('ctrl.device', function() {
                    ctrl.updateSearchText();
                }, true);

                $element.i18n();
            },
            link: function(scope, element) {
                element.i18n();
            }
        };
    });
});
