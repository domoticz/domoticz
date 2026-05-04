define(['app', 'widgets/dzBar'], function (app) {

    app.factory('utilityEditService', ['dzBarService', function (dzBarService) {

        function openUtilityDialog(device) {
            var showBar = (device.SubType !== 'Alert' && device.SubType !== 'Thermostat Operating State' && device.SubType !== 'Text');
            var isText = (device.SubType === 'Text');
            // Alert and ThermostatOpState icons are driven by level value; custom image has no effect
            var showIcon = (device.SubType !== 'Alert' && device.SubType !== 'Thermostat Operating State');
            var dialogId = isText ? '#dialog-edittextdevice' : '#dialog-editutilitydevice';
            $.devIdx = device.idx;
            $(dialogId + ' #deviceidx').text(device.idx);
            $(dialogId + ' #deviceid').text(device.ID);
            $(dialogId + ' #deviceunit').text(device.Unit);
            $(dialogId + ' #devicename').val(device.Name);
            $(dialogId + ' #devicedescription').val(device.Description);
            var $iconRow = $(dialogId + ' #combosensoricon').closest('tr');
            if (isText) {
                $(dialogId + ' #devicetext').val(device.Data);
                showIcon = (device.ShowIcon !== '0');
                $(dialogId + ' #deviceshowicon').prop('checked', showIcon);
                $(dialogId + ' #deviceshowicon').off('change').on('change', function () {
                    if ($(this).is(':checked')) {
                        $iconRow.show();
                        if (!$(dialogId + ' #combosensoricon').data('ddslick')) {
                            $(dialogId + ' #combosensoricon').ddslick({
                                data: $.ddData,
                                width: 260,
                                height: 490,
                                selectText: "Sensor Icon",
                                imagePosition: "left"
                            });
                            $.each($.ddData, function (i, item) {
                                if (item.value == device.CustomImage) {
                                    $(dialogId + ' #combosensoricon').ddslick('select', { index: i });
                                }
                            });
                        }
                    } else {
                        $iconRow.hide();
                    }
                });
            }
            if (showIcon) {
                $iconRow.show();
                $(dialogId + ' #combosensoricon').ddslick({
                    data: $.ddData,
                    width: 260,
                    height: isText ? 490 : 390,
                    selectText: "Sensor Icon",
                    imagePosition: "left"
                });
                //find our custom image index and select it
                $.each($.ddData, function (i, item) {
                    if (item.value == device.CustomImage) {
                        $(dialogId + ' #combosensoricon').ddslick('select', { index: i });
                    }
                });
            } else {
                $iconRow.hide();
            }
            if (!isText) {
                var $utilForm = $(dialogId + ' form');
                $utilForm.find('.dz-bar-btn').remove();
                if (showBar) {
                    // Utility devices store bar ranges as a flat JSON array in Color (unlike
                    // weather/temperature which use a keyed object via loadForKey/getFullColorJson).
                    dzBarService.setColorJson(device.Color || '');
                    dzBarService.attachBarButton($utilForm, device.idx, device.Name);
                }
            }
            $(dialogId).i18n().dialog('open');
        }

        function openCustomSensorDialog(device) {
            var isDistance = (device.SubType === 'Distance');
            var dialogId = isDistance ? '#dialog-editdistancedevice' : '#dialog-editcustomsensordevice';
            $.devIdx = device.idx;
            if (!isDistance) {
                $.sensorType = device.SensorType;
            }
            $(dialogId + ' #deviceidx').text(device.idx);
            $(dialogId + ' #deviceid').text(device.ID);
            $(dialogId + ' #deviceunit').text(device.Unit);
            $(dialogId + ' #devicename').val(device.Name);
            $(dialogId + ' #devicedescription').val(device.Description);
            if (isDistance) {
                $(dialogId + ' #combometertype').val(device.SwitchTypeVal);
            } else {
                $(dialogId + ' #sensoraxis').val(device.SensorUnit);
            }
            $(dialogId + ' #combosensoricon').ddslick({
                data: $.ddData,
                width: 260,
                height: 390,
                selectText: "Sensor Icon",
                imagePosition: "left"
            });
            //find our custom image index and select it
            $.each($.ddData, function (i, item) {
                if (item.value == device.CustomImage) {
                    $(dialogId + ' #combosensoricon').ddslick('select', { index: i });
                }
            });
            var $form = $(dialogId + ' form');
            $form.find('.dz-bar-btn').remove();
            dzBarService.setColorJson(device.Color || '');
            dzBarService.attachBarButton($form, device.idx, device.Name);
            $(dialogId).i18n().dialog('open');
        }

        function openMeterDialog(device) {
            var isEnergy = (device.SubType === 'kWh' || device.Type === 'Energy');
            var dialogId = isEnergy ? '#dialog-editenergydevice' : '#dialog-editmeterdevice';
            $.devIdx = device.idx;
            $(dialogId + ' #deviceidx').text(device.idx);
            $(dialogId + ' #deviceid').text(device.ID);
            $(dialogId + ' #deviceunit').text(device.Unit);
            $(dialogId + ' #devicename').val(device.Name);
            $(dialogId + ' #devicedescription').val(device.Description);
            $(dialogId + ' #combometertype').val(device.SwitchTypeVal);
            if (isEnergy) {
                var EnergyMeterMode = device.EnergyMeterMode || '0';
                $(dialogId + ' input:radio[name=EnergyMeterMode][value="' + EnergyMeterMode + '"]').attr('checked', true);
                $(dialogId + ' input:radio[name=EnergyMeterMode][value="' + EnergyMeterMode + '"]').prop('checked', true);
                $(dialogId + ' input:radio[name=EnergyMeterMode][value="' + EnergyMeterMode + '"]').trigger('change');
            } else {
                $(dialogId + ' #meterdivider').val(device.AddjValue2);
                $(dialogId + ' #meteroffset').val(device.AddjValue);
                $(dialogId + ' #valuequantity').val(device.ValueQuantity);
                $(dialogId + ' #valueunits').val(device.ValueUnits);
                $(dialogId + ' #metertable #customcounter').hide();
                if (device.SwitchTypeVal == 3) { //Counter
                    $(dialogId + ' #metertable #customcounter').show();
                }
                $(dialogId + ' #combometertype').off('change').on('change', function () {
                    $(dialogId + ' #metertable #customcounter').hide();
                    var meterType = $(dialogId + ' #combometertype').val();
                    if (meterType == 3) { //Counter
                        if (($(dialogId + ' #valuequantity').val() == '')
                            && ($(dialogId + ' #valueunits').val() == '')) {
                            $(dialogId + ' #valuequantity').val('Custom');
                        }
                        $(dialogId + ' #metertable #customcounter').show();
                    }
                });
            }
            $(dialogId + ' #combosensoricon').ddslick({
                data: $.ddData,
                width: 260,
                height: 390,
                selectText: "Sensor Icon",
                imagePosition: "left"
            });
            //find our custom image index and select it
            $.each($.ddData, function (i, item) {
                if (item.value == device.CustomImage) {
                    $(dialogId + ' #combosensoricon').ddslick('select', { index: i });
                }
            });
            var $form = $(dialogId + ' form');
            $form.find('.dz-bar-btn').remove();
            dzBarService.setColorJson(device.Color || '');
            dzBarService.attachBarButton($form, device.idx, device.Name);
            $(dialogId).i18n().dialog('open');
        }

        function openSetPointDialog(device) {
            HandleProtection(device.Protected, function () {
                $.devIdx = device.idx;
                $("#dialog-editsetpointdevice #deviceidx").text(device.idx);
                $("#dialog-editsetpointdevice #deviceid").text(device.ID);
                $("#dialog-editsetpointdevice #deviceunit").text(device.Unit);
                $("#dialog-editsetpointdevice #devicename").val(device.Name);
                $("#dialog-editsetpointdevice #devicedescription").val(device.Description);
                $('#dialog-editsetpointdevice #protected').prop('checked', (device.Protected == true));
                $("#dialog-editsetpointdevice #unit").val(device.vunit);
                $("#dialog-editsetpointdevice #step").val(device.step);
                $("#dialog-editsetpointdevice #min").val(device.min);
                $("#dialog-editsetpointdevice #max").val(device.max);
                $('#dialog-editsetpointdevice #combosensoricon').ddslick({
                    data: $.ddData,
                    width: 260,
                    height: 390,
                    selectText: "Sensor Icon",
                    imagePosition: "left"
                });
                //find our custom image index and select it
                $.each($.ddData, function (i, item) {
                    if (item.value == device.CustomImage) {
                        $('#dialog-editsetpointdevice #combosensoricon').ddslick('select', { index: i });
                    }
                });
                var $setpointForm = $('#dialog-editsetpointdevice form');
                $setpointForm.find('.dz-bar-btn').remove();
                dzBarService.setColorJson(device.Color || '');
                dzBarService.attachBarButton($setpointForm, device.idx, device.Name);
                $("#dialog-editsetpointdevice").i18n().dialog("open");
            });
        }

        function openThermostatDialog(device) {
            HandleProtection(device.Protected, function () {
                $.devIdx = device.idx;
                if (device.SubType === 'Thermostat Clock') {
                    var sarray = (device.DayTime || '').split(';');
                    $("#dialog-editthermostatclockdevice #deviceidx").text(device.idx);
                    $("#dialog-editthermostatclockdevice #deviceid").text(device.ID);
                    $("#dialog-editthermostatclockdevice #deviceunit").text(device.Unit);
                    $("#dialog-editthermostatclockdevice #devicename").val(device.Name);
                    $("#dialog-editthermostatclockdevice #devicedescription").val(device.Description);
                    $('#dialog-editthermostatclockdevice #protected').prop('checked', (device.Protected == true));
                    $("#dialog-editthermostatclockdevice #comboclockday").val(parseInt(sarray[0]));
                    $("#dialog-editthermostatclockdevice #clockhour").val(sarray[1]);
                    $("#dialog-editthermostatclockdevice #clockminute").val(sarray[2]);
                    $('#dialog-editthermostatclockdevice #combosensoricon').ddslick({
                        data: $.ddData,
                        width: 260,
                        height: 390,
                        selectText: "Sensor Icon",
                        imagePosition: "left"
                    });
                    //find our custom image index and select it
                    $.each($.ddData, function (i, item) {
                        if (item.value == device.CustomImage) {
                            $('#dialog-editthermostatclockdevice #combosensoricon').ddslick('select', { index: i });
                        }
                    });
                    $("#dialog-editthermostatclockdevice").i18n().dialog("open");
                } else {
                    // Thermostat Mode or Thermostat Fan Mode — both use the same dialog
                    $.isFan = (device.SubType === 'Thermostat Fan Mode');
                    var sarray = (device.Modes || '').split(';');
                    $("#dialog-editthermostatmode #deviceidx").text(device.idx);
                    $("#dialog-editthermostatmode #deviceid").text(device.ID);
                    $("#dialog-editthermostatmode #deviceunit").text(device.Unit);
                    $("#dialog-editthermostatmode #devicename").val(device.Name);
                    $("#dialog-editthermostatmode #devicedescription").val(device.Description);
                    $('#dialog-editthermostatmode #protected').prop('checked', (device.Protected == true));
                    //populate mode combo
                    $("#dialog-editthermostatmode #combomode").html("");
                    var ii = 0;
                    while (ii < sarray.length - 1) {
                        var option = $('<option />');
                        option.attr('value', sarray[ii]).text(sarray[ii + 1]);
                        $("#dialog-editthermostatmode #combomode").append(option);
                        ii += 2;
                    }
                    $('#dialog-editthermostatmode #combosensoricon').ddslick({
                        data: $.ddData,
                        width: 260,
                        height: 390,
                        selectText: "Sensor Icon",
                        imagePosition: "left"
                    });
                    //find our custom image index and select it
                    $.each($.ddData, function (i, item) {
                        if (item.value == device.CustomImage) {
                            $('#dialog-editthermostatmode #combosensoricon').ddslick('select', { index: i });
                        }
                    });
                    $("#dialog-editthermostatmode #combomode").val(parseInt(device.Mode));
                    $("#dialog-editthermostatmode").i18n().dialog("open");
                }
            });
        }

        function openDialog(device) {
            if (typeof device.Counter !== 'undefined') {
                if (device.Type === 'P1 Smart Meter') {
                    openUtilityDialog(device);
                } else {
                    openMeterDialog(device);
                }
            } else if (device.SubType === 'Custom Sensor') {
                openCustomSensorDialog(device);
            } else if (device.SubType === 'Distance') {
                openCustomSensorDialog(device);
            } else if (device.SubType === 'Text') {
                openUtilityDialog(device);
            } else if ((device.Type === 'Setpoint' && device.SubType === 'SetPoint') || device.Type === 'Radiator 1') {
                openSetPointDialog(device);
            } else if (device.SubType === 'Thermostat Clock' ||
                       device.SubType === 'Thermostat Mode' ||
                       device.SubType === 'Thermostat Fan Mode') {
                openThermostatDialog(device);
            } else {
                // Handles Alert, Thermostat Operating State, and other generic utility devices —
                // openUtilityDialog suppresses the bar button and icon picker for those subtypes.
                openUtilityDialog(device);
            }
        }

        return {
            openDialog: openDialog,
            openUtilityDialog: openUtilityDialog,
            openMeterDialog: openMeterDialog,
            openCustomSensorDialog: openCustomSensorDialog,
            openSetPointDialog: openSetPointDialog,
            openThermostatDialog: openThermostatDialog
        };
    }]);

    app.directive('dzUtilityWidget', ['$rootScope', '$sce', 'deviceApi', 'permissions', 'utilityEditService', function ($rootScope, $sce, deviceApi, permissions, utilityEditService) {
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
            controller: ['$scope', '$element', function ($scope, $element) {
                var ctrl = this;
                var device = $scope.device;

                function scopedDeviceHtml(data, prefix) {
                    var scopeId = prefix + String(parseInt(device.idx, 10) || 0);
                    return $sce.trustAsHtml('<div id="' + scopeId + '">' + sanitizeHTML(data, scopeId) + '</div>');
                }

                ctrl.device = device;
                ctrl.isMobile = window.myglobals && window.myglobals.ismobile;
                ctrl.dashboardType = $scope.dashboardType || (window.myglobals && window.myglobals.DashboardType);

                ctrl.barNumVal = undefined;
                ctrl.barRanges = undefined;

                function isBarSupported() {
                    if (device.Type === 'P1 Smart Meter') return true;
                    if (device.Type === 'Usage' && device.SubType === 'Electric') return true;
                    if (device.SubType === 'kWh') return true;
                    if (device.SubType === 'Percentage') return true;
                    if (device.SubType === 'Gas' || device.SubType === 'RFXMeter counter' || device.SubType === 'Counter Incremental') return true;
                    if (device.SubType === 'Managed Counter') return true;
                    if (device.SubType === 'Custom Sensor') return true;
                    if (device.Type === 'Lux') return true;
                    if (device.SubType === 'Voltage') return true;
                    if (device.SubType === 'Current' || device.Type === 'Current') return true;
                    if (device.Type === 'Setpoint' && device.SubType === 'SetPoint') return true;
                    if (device.Type === 'Air Quality') return true;
                    if (device.SubType === 'Pressure') return true;
                    if (device.SubType === 'Distance') return true;
                    if (device.Type === 'Weight') return true;
                    if (device.SubType === 'Sound Level') return true;
                    if (device.SubType === 'Waterflow') return true;
                    if (device.SubType === 'Fan') return true;
                    if (device.SubType === 'Leaf Wetness') return true;
                    if (device.SubType === 'Soil Moisture') return true;
                    if (device.SubType === 'A/D') return true;
                    if (device.Type === 'Energy' || device.Type === 'Power' || device.Type === 'Current/Energy') return true;
                    if (device.Type === 'Radiator 1') return true;
                    return false;
                }

                ctrl.getBarRanges = function() {
                    var color = device.Color;
                    if (!color || typeof color !== 'string') { return []; }
                    var trimmed = color.trim();
                    if (trimmed.charAt(0) !== '[') { return []; }
                    try { return JSON.parse(trimmed); } catch(e) { return []; }
                };

                function updateBar() {
                    if (!isBarSupported()) { ctrl.barNumVal = undefined; ctrl.barRanges = undefined; return; }
                    ctrl.barRanges = ctrl.getBarRanges();
                    if (!ctrl.barRanges.length) { ctrl.barNumVal = undefined; return; }
                    var dataStr;
                    if (device.Type === 'P1 Smart Meter') {
                        var usageVal  = parseFloat((device.Usage      || '').replace(',', '.'));
                        var delivVal  = parseFloat((device.UsageDeliv || '').replace(',', '.'));
                        if (!isNaN(usageVal) && !isNaN(delivVal)) {
                            ctrl.barNumVal = usageVal - delivVal;
                            return;
                        }
                        dataStr = device.Usage || '';
                    } else if (device.SubType === 'kWh' || device.Type === 'Energy' || device.Type === 'Power' || device.Type === 'Current/Energy') {
                        dataStr = device.Usage || '';
                    } else if (device.SubType === 'Gas' || device.SubType === 'RFXMeter counter' || device.SubType === 'Counter Incremental') {
                        dataStr = device.CounterToday || '';
                    } else if (device.SubType === 'Managed Counter') {
                        dataStr = device.Counter || '';
                    } else if (device.Type === 'Setpoint' && device.SubType === 'SetPoint') {
                        dataStr = device.SetPoint !== undefined ? String(device.SetPoint) : (device.Data || '');
                    } else {
                        dataStr = device.Data || '';
                    }
                    var m = dataStr.match(/^(-?[\d.,]+)/);
                    ctrl.barNumVal = m ? parseFloat(m[1].replace(',', '.')) : undefined;
                }

                // Keep ctrl.device in sync when parent updates the binding.
                // Note: RefreshItem uses angular.extend (mutates in place), so a reference
                // watch on 'device' never fires. Watch the actual value field instead.
                $scope.$watch('device', function (newVal) {
                    if (newVal) {
                        device = newVal;
                        ctrl.device = newVal;
                        updateBar();
                    }
                });
                $scope.$watch(function () {
                    var d = $scope.device;
                    return d ? (d.SetPoint !== undefined ? d.SetPoint : d.Data) : null;
                }, function (newVal, oldVal) {
                    if (newVal !== oldVal && $scope.device) {
                        device = $scope.device;
                        ctrl.device = $scope.device;
                        updateBar();
                    }
                });

                updateBar();

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

                ctrl.hideTextIcon = function () {
                    return device.ShowIcon === '0';
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
                    } else if (typeof device.Rain !== 'undefined') {
                        bigtext = device.Rain + ' mm';
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
                            status = device.Counter + (device.vunit ? ' ' + device.vunit : '');
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
                        return scopedDeviceHtml(device.Data, 'dz-uw-');
                    } else if (ctrl.isAlert()) {
                        return scopedDeviceHtml(device.Data, 'dz-ua-');
                    } else if (typeof device.Rain !== 'undefined' && typeof device.RainRate !== 'undefined') {
                        status = $.t('Rain rate') + ': ' + device.RainRate + ' mm/h';
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
                        if (ctrl.isText()) {
                            return scopedDeviceHtml(device.Data, 'dz-um-');
                        }
                        var scopeId = 'dz-ua-' + String(parseInt(device.idx, 10) || 0);
                        var aLevel = Math.min(parseInt(device.Level) || 0, 4);
                        var img = '<img src="images/Alert48_' + aLevel + '.png" height="16" width="16">';
                        var html = sanitizeHTML(device.Data, scopeId) + ' ' + img;
                        return $sce.trustAsHtml('<div id="' + scopeId + '">' + html + '</div>');
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

                // Edit function dispatching - delegates to utilityEditService
                ctrl.editDevice = function() {
                    if (!permissions.hasPermission('Admin')) return;
                    utilityEditService.openDialog(device);
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
            }],
            link: function(scope, element) {
                element.i18n();
            }
        };
    }]);

});
