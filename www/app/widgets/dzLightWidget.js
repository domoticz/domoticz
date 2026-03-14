define(['app'], function (app) {
    app.directive('dzLightWidget', function ($rootScope, $timeout, deviceApi, deviceLightApi, domoticzApi, permissions) {
        // Ensure SwitchModal is available globally for Evohome onclick handlers
        if (typeof window.SwitchModal === 'undefined') {
            window.SwitchModal = function (idx, name, status) {
                ShowNotify($.t('Setting Evohome ') + ' ' + $.t(name));
                $.ajax({
                    url: "json.htm?type=command&param=switchmodal&idx=" + idx + "&status=" + status + "&action=1",
                    async: false,
                    dataType: 'json',
                    success: function (data) {
                        if (data.status == "ERROR") {
                            HideNotify();
                            bootbox.alert($.t('Problem sending switch command'));
                        }
                        setTimeout(function () { HideNotify(); }, 1000);
                    },
                    error: function () {
                        HideNotify();
                        bootbox.alert($.t('Problem sending switch command'));
                    }
                });
            };
        }

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
                    return 'views/widgets/light_widget_tab.html';
                }
                var isMobile = window.myglobals && window.myglobals.ismobile;
                var dashboardType = window.myglobals && window.myglobals.DashboardType;
                if (isMobile || dashboardType == 2) {
                    return 'views/widgets/light_widget_mobile.html';
                }
                return 'views/widgets/light_widget.html';
            },
            controllerAs: 'ctrl',
            controller: function ($scope, $element, $location) {
                var ctrl = this;
                var device = $scope.device;

                ctrl.device = device;
                ctrl.isMobile = window.myglobals && window.myglobals.ismobile;
                ctrl.dashboardType = $scope.dashboardType || (window.myglobals && window.myglobals.DashboardType);

                // Keep ctrl.device in sync when parent replaces the binding
                $scope.$watch('device', function (newVal) {
                    if (newVal) {
                        device = newVal;
                        ctrl.device = newVal;
                        ctrl.updateSelectorLevels();
                        ctrl.updateSearchText();
                    }
                });

                // Update selector levels when the active level changes (works with angular.extend in-place updates)
                $scope.$watch('device.LevelInt', function () {
                    ctrl.updateSelectorLevels();
                });

                ctrl.getBackgroundClass = function () {
                    return $rootScope.GetItemBackgroundStatus(device);
                };

                ctrl.getStatusText = function () {
                    if (device.SubType == "Evohome") {
                        return ctrl.evoDisplayTextMode(device.Status);
                    } else if (device.SwitchType === "Selector") {
                        return b64DecodeUnicode(device.LevelNames).split('|')[(device.LevelInt / 10)];
                    } else {
                        return TranslateStatusShort(device.Status);
                    }
                };

                ctrl.evoDisplayTextMode = function (status) {
                    if (status == "Auto") return "Normal";
                    if (status == "AutoWithEco") return "Economy";
                    if (status == "DayOff") return "Day Off";
                    if (status == "HeatingOff") return "Heating Off";
                    return status;
                };

                ctrl.isActive = function () {
                    return device.Status && (
                        ['On', 'Chime', 'Group On', 'All On', 'Panic', 'Mixed'].includes(device.Status) ||
                        device.Status.indexOf('Set ') === 0 ||
                        device.Status.indexOf('NightMode') === 0 ||
                        device.Status.indexOf('Disco ') === 0
                    );
                };

                ctrl.isDimmer = function () {
                    return ['Dimmer', 'Blinds Percentage', 'Blinds % + Stop', 'TPI'].includes(device.SwitchType);
                };

                ctrl.isBlinds = function () {
                    return device.SwitchType && device.SwitchType.indexOf('Blinds') >= 0;
                };

                ctrl.isBlindsOpen = function () {
                    return device.Status === 'Open' ||
                        (device.Status && device.Status.indexOf('Set ') === 0) ||
                        device.Status === 'Stopped';
                };

                ctrl.isSelector = function () {
                    return device.SwitchType === 'Selector';
                };

                ctrl.isRGB = function () {
                    return device.SubType && (device.SubType.indexOf('RGB') >= 0 || device.SubType.indexOf('WW') >= 0);
                };

                ctrl.isEvohome = function () {
                    return device.SubType === 'Evohome';
                };

                ctrl.hasStopButton = function () {
                    return (device.SubType == "RAEX") ||
                        (device.SubType && device.SubType.indexOf('A-OK') == 0) ||
                        (device.SubType && device.SubType.indexOf('Hasta') >= 0) ||
                        (device.SubType && device.SubType.indexOf('Media Mount') == 0) ||
                        (device.SubType && device.SubType.indexOf('Forest') == 0) ||
                        (device.SubType && device.SubType.indexOf('Chamberlain') == 0) ||
                        (device.SubType && device.SubType.indexOf('Sunpery') == 0) ||
                        (device.SubType && device.SubType.indexOf('Dolat') == 0) ||
                        (device.SubType && device.SubType.indexOf('ASP') == 0) ||
                        (device.SubType == "Harrison") ||
                        (device.SubType && device.SubType.indexOf('RFY') == 0) ||
                        (device.SubType && device.SubType.indexOf('ASA') == 0) ||
                        (device.SubType && device.SubType.indexOf('DC106') == 0) ||
                        (device.SubType && device.SubType.indexOf('Confexx') == 0) ||
                        (device.SwitchType && device.SwitchType.indexOf("Venetian Blinds") == 0) ||
                        (device.SwitchType && device.SwitchType.indexOf("Stop") >= 0);
                };

                ctrl.selectorLevels = [];

                ctrl.updateSelectorLevels = function () {
                    if (!device.LevelNames) {
                        ctrl.selectorLevels.length = 0;
                        return;
                    }
                    var levels = b64DecodeUnicode(device.LevelNames).split('|');
                    var idx = 0;
                    for (var i = 0; i < levels.length; i++) {
                        if (i === 0 && device.LevelOffHidden) continue;
                        if (idx < ctrl.selectorLevels.length) {
                            ctrl.selectorLevels[idx].name = levels[i];
                            ctrl.selectorLevels[idx].value = i * 10;
                            ctrl.selectorLevels[idx].isActive = (i * 10) === device.LevelInt;
                        } else {
                            ctrl.selectorLevels.push({
                                name: levels[i],
                                value: i * 10,
                                isActive: (i * 10) === device.LevelInt
                            });
                        }
                        idx++;
                    }
                    ctrl.selectorLevels.length = idx;
                };
                ctrl.updateSelectorLevels();

                ctrl.getSelectorLevels = function () {
                    return ctrl.selectorLevels;
                };

                ctrl.switchLight = function (cmd) {
                    if (!permissions.hasPermission("User")) {
                        ShowNotify($.t('You do not have permission to do that!'), 2500, true);
                        return;
                    }

                    if (device.Protected) {
                        bootbox.prompt($.t("Please enter Password") + ":", function (result) {
                            if (result === null || result === "") return;
                            SwitchLightInt(device.idx, cmd, result);
                        });
                    } else {
                        ctrl.executeSwitchCommand(cmd);
                    }
                };

                ctrl.executeSwitchCommand = function (cmd) {
                    var promise;

                    if (cmd === 'On') {
                        promise = deviceLightApi.switchOn(device.idx);
                    } else if (cmd === 'Off') {
                        promise = deviceLightApi.switchOff(device.idx);
                    } else {
                        promise = domoticzApi.sendCommand('switchlight', {
                            idx: device.idx,
                            switchcmd: cmd,
                            level: 0
                        });
                    }

                    if (promise) {
                        promise.then(function () {
                            if ($scope.onUpdate) {
                                $scope.onUpdate();
                            }
                        }).catch(function () {
                            // Error already handled by API layer (bootbox alert)
                        });
                    }
                };

                ctrl.setDimLevel = function (level) {
                    if (!permissions.hasPermission("User")) {
                        ShowNotify($.t('You do not have permission to do that!'), 2500, true);
                        return;
                    }

                    if (device.Protected) {
                        bootbox.prompt($.t("Please enter Password") + ":", function (result) {
                            if (result === null || result === "") return;
                            domoticzApi.sendCommand('switchlight', {
                                idx: device.idx,
                                switchcmd: 'Set Level',
                                level: level,
                                passcode: result
                            });
                        });
                    } else {
                        ctrl.executeSetLevel(level);
                    }
                };

                ctrl.executeSetLevel = function (level) {
                    domoticzApi.sendCommand('switchlight', {
                        idx: device.idx,
                        switchcmd: 'Set Level',
                        level: level
                    }).then(function () {
                        if ($scope.onUpdate) {
                            $scope.onUpdate();
                        }
                    }).catch(function () {
                        ShowNotify($.t('Problem sending switch command'), 2500, true);
                    });
                };

                ctrl.setSelectorLevel = function (level, levelName) {
                    if (!permissions.hasPermission("User")) {
                        ShowNotify($.t('You do not have permission to do that!'), 2500, true);
                        return;
                    }

                    if (device.Protected) {
                        bootbox.prompt($.t("Please enter Password") + ":", function (result) {
                            if (result === null || result === "") return;
                            domoticzApi.sendCommand('switchlight', {
                                idx: device.idx,
                                switchcmd: 'Set Level',
                                level: level,
                                passcode: result
                            });
                        });
                    } else {
                        ctrl.executeSetSelectorLevel(level, levelName);
                    }
                };

                ctrl.executeSetSelectorLevel = function (level, levelName) {
                    domoticzApi.sendCommand('switchlight', {
                        idx: device.idx,
                        switchcmd: 'Set Level',
                        level: level
                    }).then(function () {
                        if ($scope.onUpdate) {
                            $scope.onUpdate();
                        }
                    }).catch(function () {
                        ShowNotify($.t('Problem sending switch command'), 2500, true);
                    });
                };

                ctrl.initSelectorDropdown = function () {
                    var levels = ctrl.getSelectorLevels();
                    for (var i = 0; i < levels.length; i++) {
                        if (levels[i].isActive) {
                            ctrl.selectedLevel = levels[i];
                            break;
                        }
                    }
                };

                ctrl.selectorLevel = null;
                ctrl.initSelectorLevel = function () {
                    ctrl.selectorLevel = device.LevelInt;
                };

                ctrl.onSelectorChange = function () {
                    if (ctrl.selectorLevel !== null) {
                        var levels = b64DecodeUnicode(device.LevelNames).split('|');
                        var levelName = levels[ctrl.selectorLevel / 10] || '';
                        ctrl.setSelectorLevel(ctrl.selectorLevel, levelName);
                    }
                };

                ctrl.setColor = function (color, brightness) {
                    deviceLightApi.setColor(device.idx, color, brightness).then(function () {
                        if ($scope.onUpdate) {
                            $scope.onUpdate();
                        }
                    });
                };

                ctrl.showEvohomeModal = function () {
                    var modes = [
                        { "name": "Normal", "data": "Auto" },
                        { "name": "Economy", "data": "AutoWithEco" },
                        { "name": "Away", "data": "Away" },
                        { "name": "Day Off", "data": "DayOff" },
                        { "name": "Custom", "data": "Custom" },
                        { "name": "Heating Off", "data": "HeatingOff" }
                    ];

                    var modalContent = '<ul class="ui-listview ui-listview-inset ui-corner-all ui-shadow">';
                    modalContent += '<li class="ui-li-divider ui-bar-inherit ui-first-child">' + $.t('Choose an action') + '</li>';

                    modes.forEach(function (mode) {
                        modalContent += '<li><a href="#" class="ui-btn ui-btn-icon-right ui-icon-' + mode.data + '" onclick="SwitchModal(\'' + device.idx + '\',\'' + mode.name + '\',\'' + mode.data + '\');return false;">' + $.t(mode.name) + '</a></li>';
                    });

                    modalContent += '</ul>';

                    bootbox.alert(modalContent);
                };

                ctrl.getDeviceIcon = function () {
                    // Special types with non-standard icon naming
                    if (device.SwitchType == 'Doorbell') {
                        return 'images/doorbell48.png';
                    }
                    if (ctrl.isBlinds()) {
                        return ctrl.isBlindsOpen() ? 'images/blindsopen48sel.png' : 'images/blinds48sel.png';
                    }
                    if (device.SwitchType == 'Smoke Detector') {
                        return ctrl.isActive() ? 'images/smoke48on.png' : 'images/smoke48off.png';
                    }
                    if (device.SwitchType == 'Motion Sensor') {
                        return ctrl.isActive() ? 'images/motion48-on.png' : 'images/motion48-off.png';
                    }
                    if (device.SwitchType == 'Dusk Sensor') {
                        return device.Status == 'On' ? 'images/uvdark.png' : 'images/uvsunny.png';
                    }
                    if (device.SubType == 'Security Panel') {
                        return 'images/security48.png';
                    }

                    // X10 Siren uses siren-on/off images (not standard naming)
                    if (device.SwitchType == 'X10 Siren') {
                        return ctrl.isActive() ? 'images/siren-on.png' : 'images/siren-off.png';
                    }

                    // TPI uses Fireplace images
                    if (device.SwitchType == 'TPI') {
                        return device.Status != 'Off' ? 'images/Fireplace48_On.png' : 'images/Fireplace48_Off.png';
                    }

                    // Fan subtypes always show Fan48_On.png
                    if (device.SubType && (
                        device.SubType.indexOf('Itho') == 0 ||
                        device.SubType.indexOf('Lucci') == 0 ||
                        device.SubType.indexOf('Falmec') == 0 ||
                        device.SubType.indexOf('Westinghouse') == 0
                    )) {
                        return 'images/Fan48_On.png';
                    }

                    // Security type devices (Type == "Security", not SwitchType)
                    if (device.Type == 'Security') {
                        if (device.SubType && device.SubType.indexOf('remote') > 0) {
                            return 'images/remote48.png';
                        }
                        if (device.SubType == 'X10 security') {
                            return device.Status.indexOf('Normal') >= 0 ? 'images/security48.png' : 'images/Alarm48_On.png';
                        }
                        if (device.SubType == 'X10 security motion') {
                            return device.Status == 'No Motion' ? 'images/security48.png' : 'images/Alarm48_On.png';
                        }
                        if (device.Status && (device.Status.indexOf('Alarm') >= 0 || device.Status.indexOf('Tamper') >= 0)) {
                            return 'images/Alarm48_On.png';
                        }
                        if (device.SubType && device.SubType.indexOf('Meiantech') >= 0) {
                            return 'images/security48.png';
                        }
                        if (device.SubType && device.SubType.indexOf('KeeLoq') >= 0) {
                            return 'images/pushon48.png';
                        }
                        return 'images/security48.png';
                    }

                    // Door Lock / Door Lock Inverted use InternalState (not Status) to pick icon
                    if (device.SwitchType == 'Door Lock' || device.SwitchType == 'Door Lock Inverted') {
                        var lockImage = device.Image || 'Light';
                        lockImage = lockImage.charAt(0).toUpperCase() + lockImage.slice(1);
                        var isUnlocked = device.InternalState == 'Unlocked';
                        return 'images/' + lockImage + '48_' + (isUnlocked ? 'On' : 'Off') + '.png';
                    }

                    // Use device.Image as-is from backend
                    // Backend sets Image="Light" when CustomImage==0, or custom icon root when CustomImage!=0
                    var image = device.Image;
                    if (!image) {
                        image = 'Light';
                    }
                    // Capitalize first letter to match file naming convention
                    image = image.charAt(0).toUpperCase() + image.slice(1);

                    // RGB/LED dimmers with default image use RGB icon
                    if (ctrl.isDimmer() && ctrl.isRGB() && device.CustomImage == 0) {
                        return 'images/RGB48_' + (ctrl.isActive() ? 'On' : 'Off') + '.png';
                    }

                    // Push On Button always shows On icon, Push Off Button always shows Off icon
                    if (device.SwitchType == 'Push On Button') {
                        return 'images/' + image + '48_On.png';
                    }
                    if (device.SwitchType == 'Push Off Button') {
                        return 'images/' + image + '48_Off.png';
                    }

                    return 'images/' + image + '48_' + (ctrl.isActive() ? 'On' : 'Off') + '.png';
                };

                ctrl.getTableId = function () {
                    if ($scope.viewMode === 'tab') {
                        // Tab view uses full-size table IDs
                        if (ctrl.isBlinds() && ctrl.hasStopButton()) {
                            return 'itemtabletrippleicon';
                        } else if (ctrl.isBlinds() || device.SwitchType === 'Media Player') {
                            return 'itemtabledoubleicon';
                        } else {
                            return 'itemtable';
                        }
                    }
                    // Dashboard uses small table IDs
                    if (ctrl.isBlinds() && ctrl.hasStopButton()) {
                        return 'itemtablesmalltrippleicon';
                    } else if (ctrl.isBlinds() || device.SwitchType === 'Media Player') {
                        return 'itemtablesmalldoubleicon';
                    } else {
                        return 'itemtablesmall';
                    }
                };

                ctrl.getSpanClass = function () {
                    if (ctrl.dashboardType == 1) {
                        return 'span3';
                    }
                    return 'span4';
                };

                ctrl.isClickable = function () {
                    // Read-only sensors are not clickable
                    var readOnly = ['Door Contact', 'Contact', 'Motion Sensor', 'Dusk Sensor'];
                    if (readOnly.includes(device.SwitchType)) return false;
                    // TPI is read-only when Unit is outside 64-95 range
                    if (device.SwitchType == 'TPI' && (device.Unit < 64 || device.Unit > 95)) return false;
                    return true;
                };

                ctrl.deviceIconClick = function ($event) {
                    if (!ctrl.isClickable()) return;

                    // Fan subtypes - show specialized popups
                    if (device.SubType && device.SubType.indexOf('Itho') == 0) {
                        ShowIthoPopup($event || event, device.idx, device.Protected, window.myglobals.ismobile);
                        return;
                    }
                    if (device.SubType && device.SubType.indexOf('Lucci Air DC') == 0) {
                        ShowLucciDCPopup($event || event, device.idx, device.Protected, window.myglobals.ismobile);
                        return;
                    }
                    if (device.SubType && (device.SubType.indexOf('Lucci') == 0 || device.SubType.indexOf('Westinghouse') == 0)) {
                        ShowLucciPopup($event || event, device.idx, device.Protected, window.myglobals.ismobile);
                        return;
                    }
                    if (device.SubType && device.SubType.indexOf('Falmec') == 0) {
                        ShowFalmecPopup($event || event, device.idx, device.Protected, window.myglobals.ismobile);
                        return;
                    }

                    // Thermostat 3 - uses Type not SwitchType
                    if (device.Type == 'Thermostat 3') {
                        ShowTherm3Popup($event || event, device.idx, device.Protected, device.MaxDimLevel, device.LevelInt, '');
                        return;
                    }

                    if (ctrl.isDimmer() && ctrl.isRGB()) {
                        ShowRGBWPopup($event || event, device.idx, device.Protected,
                            device.MaxDimLevel, device.LevelInt,
                            device.Color, device.SubType, device.DimmerType);
                        return;
                    }

                    if (device.SwitchType == 'Push On Button') {
                        ctrl.switchLight('On');
                    } else if (device.SwitchType == 'Push Off Button') {
                        ctrl.switchLight('Off');
                    } else if (device.SwitchType == 'Door Lock') {
                        // Unlocked → send 'On' (lock), Locked → send 'Off' (unlock)
                        ctrl.switchLight(device.InternalState == 'Unlocked' ? 'On' : 'Off');
                    } else if (device.SwitchType == 'Door Lock Inverted') {
                        // Unlocked → send 'Off' (lock), Locked → send 'On' (unlock)
                        ctrl.switchLight(device.InternalState == 'Unlocked' ? 'Off' : 'On');
                    } else if (device.Type == 'Security') {
                        // Security type devices have specialized click logic
                        if (device.SubType && device.SubType.indexOf('remote') > 0) {
                            if (device.Status.indexOf('Arm') >= 0 || device.Status.indexOf('Panic') >= 0) {
                                ctrl.switchLight('Off');
                            } else {
                                ArmSystem(device.idx, 'On', device.Protected);
                            }
                        } else if (device.SubType == 'X10 security') {
                            if (device.Status.indexOf('Normal') >= 0) {
                                ctrl.switchLight(device.Status == 'Normal Delayed' ? 'Alarm Delayed' : 'Alarm');
                            } else {
                                ctrl.switchLight(device.Status == 'Alarm Delayed' ? 'Normal Delayed' : 'Normal');
                            }
                        } else if (device.SubType == 'X10 security motion') {
                            if (device.Status == 'No Motion') {
                                ctrl.switchLight('Motion');
                            } else {
                                ctrl.switchLight('No Motion');
                            }
                        } else if (device.SubType && device.SubType.indexOf('Meiantech') >= 0) {
                            if (device.Status.indexOf('Arm') >= 0 || device.Status.indexOf('Panic') >= 0) {
                                ctrl.switchLight('Off');
                            } else {
                                ArmSystemMeiantech(device.idx, 'On', device.Protected);
                            }
                        } else if (device.SubType && device.SubType.indexOf('KeeLoq') >= 0) {
                            ctrl.switchLight('On');
                        }
                        // Other security types (Alarm/Tamper state) are not clickable
                    } else if (device.SwitchType == 'Smoke Detector') {
                        // Smoke detector always sends 'On' (to trigger alarm)
                        ctrl.switchLight('On');
                    } else if (device.SwitchType == 'X10 Siren') {
                        ctrl.switchLight(ctrl.isActive() ? 'Off' : 'On');
                    } else if (ctrl.isSelector()) {
                        // For selectors, clicking the icon turns off (only if not LevelOffHidden)
                        if (device.LevelInt > 0 && !device.LevelOffHidden) {
                            ctrl.setSelectorLevel(0, 'Off');
                        }
                    } else {
                        // Toggle
                        ctrl.switchLight(ctrl.isActive() ? 'Off' : 'On');
                    }
                };

                ctrl.isMediaPlayer = function () {
                    return device.SwitchType === 'Media Player';
                };

                ctrl.isMediaPlayerActive = function () {
                    return device.Status !== 'Off' && device.Status !== '0' && device.Status !== 'Disconnected';
                };

                ctrl.isMediaPlayerDisconnected = function () {
                    return device.Status === 'Disconnected';
                };

                ctrl.showMediaRemote = function () {
                    ShowMediaRemote(escape(device.Name), device.idx, device.HardwareType);
                };

                ctrl.getMediaPlayerIcon = function () {
                    var image = device.CustomImage == 0 ? device.TypeImg : device.Image;
                    if (!image) image = 'Light';
                    image = image.charAt(0).toUpperCase() + image.slice(1);
                    if (device.Status !== 'Off' && device.Status !== '0' && device.Status !== 'Disconnected') {
                        return 'images/' + image + '48_On.png';
                    }
                    return 'images/' + image + '48_Off.png';
                };

                ctrl.getMediaPlayerStatusText = function () {
                    var status = device.Data || '';
                    if (status === '0') status = '';
                    return status;
                };

                ctrl.isRegularSwitch = function () {
                    var specialTypes = [
                        'Doorbell',
                        'Push On Button',
                        'Push Off Button',
                        'Door Contact',
                        'Contact',
                        'Motion Sensor',
                        'Smoke Detector',
                        'Dusk Sensor',
                        'Door Lock',
                        'Door Lock Inverted',
                        'Security Panel',
                        'Media Player'
                    ];

                    return !ctrl.isDimmer() &&
                           !ctrl.isBlinds() &&
                           !ctrl.isSelector() &&
                           !ctrl.isEvohome() &&
                           !specialTypes.includes(device.SwitchType);
                };

                ctrl.showTimers = function () {
                    var noTimerTypes = [
                        'Door Contact',
                        'Contact',
                        'Motion Sensor',
                        'Smoke Detector',
                        'Doorbell',
                        'Dusk Sensor'
                    ];

                    return !noTimerTypes.includes(device.SwitchType);
                };

                ctrl.isSmokeSensor = function () {
                    return device.SwitchType === 'Smoke Detector';
                };

                ctrl.isSmokeActive = function () {
                    return device.Status === 'Panic' || device.Status === 'On';
                };

                ctrl.resetSmokeAlarm = function () {
                    if (!permissions.hasPermission('Admin')) return;
                    if (typeof ResetSecurityStatus !== 'undefined') {
                        ResetSecurityStatus(device.idx, 'Normal', function() {
                            if ($scope.onUpdate) {
                                $scope.onUpdate();
                            }
                        });
                    }
                };

                ctrl.isRFY = function () {
                    return device.Type === 'RFY';
                };

                ctrl.showRFYSetup = function ($event) {
                    ShowRFYPopup($event || event, device.idx, device.Protected, window.myglobals.ismobile);
                };

                // Tab mode specific methods
                ctrl.isAdmin = permissions.hasPermission('Admin');

                ctrl.getTypeSubTypeText = function () {
                    return device.Type + ', ' + device.SubType + ', ' + device.SwitchType;
                };

                ctrl.searchText = '';
                ctrl.updateSearchText = function () {
                    if (!device) return;
                    var bigtext = TranslateStatusShort(device.Status);
                    ctrl.searchText = GenerateLiveSearchTextL(device, bigtext);
                };
                ctrl.updateSearchText();

                ctrl.getGraphLogLink = function () {
                    return '#/Devices/' + device.idx + '/Log';
                };

                ctrl.hasTimers = function () {
                    return device.Timers === 'true' || device.Timers === true;
                };

                ctrl.hasNotifications = function () {
                    return device.Notifications === 'true' || device.Notifications === true;
                };

                ctrl.getNotificationLink = function () {
                    return '#/Devices/' + device.idx + '/Notifications';
                };

                ctrl.getTimerLink = function () {
                    return '#/Devices/' + device.idx + '/Timers';
                };

                ctrl.makeFavorite = function (isFavorite) {
                    if (!permissions.hasPermission('User')) {
                        ShowNotify($.t('You do not have permission to do that!'), 2500, true);
                        return;
                    }
                    deviceApi.makeFavorite(device.idx, isFavorite).then(function () {
                        if ($scope.onUpdate) {
                            $scope.onUpdate();
                        }
                    });
                };

                ctrl.editDevice = function () {
                    if (!permissions.hasPermission('Admin')) return;
                    $location.path('/Devices/' + device.idx + '/LightEdit');
                };

                $scope.$watch('device.Status', function () {
                    ctrl.updateSearchText();
                });

                $element.i18n();
            },
            link: function (scope, element, attrs) {
                function resizeSliders() {
                    var nameWidth = element.find('#name').width();
                    if (nameWidth > 0) {
                        if (scope.viewMode === 'tab') {
                            // Tab mode: sliders inside <td id="type"> with inline margin-left
                            element.find('.dimslidernorm').width(nameWidth - 100);
                            element.find('.dimsmall').width(nameWidth - 140);
                            element.find('.dimsmall3').width(nameWidth - 170);
                        } else {
                            // Dashboard mode
                            element.find('.dimslidernorm').width(nameWidth - 40);
                            element.find('.dimslidersmalldouble').width(nameWidth - 85);
                            element.find('.dimslidersmalltripple').width(nameWidth - 115);
                        }
                    }
                    return nameWidth > 0;
                }

                function initSliders() {
                    element.find('.dimslider').each(function() {
                        var $slider = $(this);
                        if (!$slider.hasClass('ui-slider')) {
                            $slider.css('visibility', 'hidden');
                            $slider.slider({
                                range: "min",
                                min: 0,
                                max: 15,
                                value: 4,
                                create: function (event, ui) {
                                    $(this).slider("option", "max", $(this).data('maxlevel'));
                                    $(this).slider("option", "type", $(this).data('type'));
                                    $(this).slider("option", "isprotected", $(this).data('isprotected'));
                                    $(this).slider("value", $(this).data('svalue'));
                                    if ($(this).data('disabled'))
                                        $(this).slider("option", "disabled", true);
                                },
                                slide: function (event, ui) {
                                    clearInterval($.setDimValue);
                                    var maxValue = $(this).slider("option", "max");
                                    var dtype = $(this).slider("option", "type");
                                    var isled = $(this).data('isled');
                                    var isProtected = $(this).slider("option", "isprotected");
                                    var fPercentage = parseInt((100.0 / maxValue) * ui.value);
                                    var idx = $(this).data('idx');
                                    var deviceElem = element.closest('.itemBlock');
                                    if (deviceElem.length > 0) {
                                        var bigtext = fPercentage + " %";
                                        deviceElem.find('#bigtext').text(bigtext);

                                        // Update icon for non-blinds non-LED dimmers
                                        if ((dtype != "blinds") && !isled) {
                                            var imgElem = deviceElem.find('#img img.lcursor');
                                            if (imgElem.length > 0) {
                                                var imgname = imgElem.attr('src');
                                                if (imgname) {
                                                    imgname = imgname.substring(imgname.lastIndexOf("/") + 1, imgname.lastIndexOf("_O") + 2);
                                                    if (dtype == "relay")
                                                        imgname = "Fireplace48_O";

                                                    var newSrc = fPercentage == 0 ?
                                                        'images/' + imgname + 'ff.png' :
                                                        'images/' + imgname + 'n.png';
                                                    imgElem.attr('src', newSrc);
                                                }
                                            }
                                        }
                                    }
                                    if (dtype != "relay" && typeof SetDimValue !== 'undefined')
                                        $.setDimValue = setInterval(function () { SetDimValue(idx, ui.value); }, 500);
                                },
                                stop: function (event, ui) {
                                    var idx = $(this).data('idx');
                                    var dtype = $(this).slider("option", "type");
                                    if (dtype == "relay" && typeof SetDimValue !== 'undefined')
                                        SetDimValue(idx, ui.value);
                                }
                            });
                            $slider.css('visibility', 'visible');
                        }
                    });
                }

                function initWidgets(retryCount) {
                    retryCount = retryCount || 0;

                    initSliders();

                    if (!resizeSliders() && retryCount < 5) {
                        $timeout(function() {
                            initWidgets(retryCount + 1);
                        }, 100);
                        return;
                    }

                    // Initialize selector dropdowns
                    element.find('.selectorlevels select').each(function() {
                        var $select = $(this);
                        if (!$select.data('ui-selectmenu')) {
                            $select.selectmenu({
                                width: false
                            });
                        }
                    });

                    element.i18n();
                }

                // Initialize dimsliders and selector dropdowns after Angular renders the template
                scope.$watch(function() {
                    return element.find('.dimslider').length + element.find('.selectorlevels select').length;
                }, function(newVal) {
                    if (newVal > 0) {
                        $timeout(function() {
                            initWidgets();
                        }, 50);
                    }
                });

                // Update slider value when device.LevelInt changes (e.g. WebSocket updates)
                scope.$watch('device.LevelInt', function(newVal) {
                    if (typeof newVal !== 'undefined') {
                        element.find('.dimslider').each(function() {
                            var $slider = $(this);
                            if ($slider.hasClass('ui-slider')) {
                                $slider.slider('value', newVal);
                            }
                        });
                    }
                });

                // Recalculate slider widths on window resize
                var resizeHandler = function() {
                    resizeSliders();
                };
                $(window).on('resize', resizeHandler);

                scope.$on('$destroy', function() {
                    $(window).off('resize', resizeHandler);
                });

            }
        };
    });
});
