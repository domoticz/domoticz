define(['app', 'components/rgbw-picker/RgbwPicker'], function (app) {

    app.component('deviceIconSelect', {
        template: '<select id="icon-select"></select>',
        bindings: {
            switchType: '<'
        },
        require: {
            ngModelCtrl: 'ngModel'
        },
        controller: function ($element, domoticzApi, dzDefaultSwitchIcons) {
            var vm = this;
            var switch_icons = [];

            function updateSelector(bFromUser) {
                switch_icons[0].imageSrc = dzDefaultSwitchIcons[vm.switchType]
                    ? 'images/' + dzDefaultSwitchIcons[vm.switchType][0]
                    : 'images/Generic48_On.png';

                $element.find('#icon-select').ddslick('destroy');
                $element.find('#icon-select').ddslick({
                    data: switch_icons,
                    width: 260,
                    height: 390,
                    selectText: 'Select Switch Icon',
                    imagePosition: 'left',
                    onSelected: function (data) {
                        vm.ngModelCtrl.$setViewValue(data.selectedData.value);
                    }
                });
                if (bFromUser) {
					$element.find('#icon-select').ddslick('select', { index: 0 });
				}

                vm.ngModelCtrl.$render();
            }

            vm.$onInit = function () {
                // TODO: Add caching mechanism for this request
                domoticzApi.sendRequest({
                    type: 'custom_light_icons'
                }).then(function (data) {
                    switch_icons = (data.result || [])
                        .filter(function (item) {
                            return item.idx !== 0;
                        })
                        .map(function (item) {
                            return {
                                text: item.text,
                                value: item.idx,
                                selected: false,
                                description: item.description,
                                imageSrc: 'images/' + item.imageSrc + '48_On.png'
                            };
                        });

                    switch_icons.unshift({
                        text: 'Default',
                        value: 0,
                        selected: false,
                        description: 'Default icon'
                    });

                    updateSelector(false);
                });

                vm.ngModelCtrl.$render = function () {
                    var value = vm.ngModelCtrl.$modelValue;

                    switch_icons.forEach(function (item, index) {
                        if (item.value === value) {
                            $element.find('#icon-select').ddslick('select', { index: index });
                        }
                    });
                };
            };

            vm.$onChanges = function (changes) {
                if (changes.switchType && switch_icons.length > 0) {
                    updateSelector(true);
                }
            }
        }
    });

    app.component('deviceColorSettings', {
        templateUrl: 'views/rgbwPicker.html',
        bindings: {
            device: '='
        },
        controller: function ($scope, domoticzApi, deviceLightApi) {
            var $ctrl = this;

            $ctrl.$onInit = init;
            $ctrl.isRelativeDimmer = isRelativeDimmer;
            $ctrl.isRelativeColorTemperature = isRelativeColorTemperature;
            $ctrl.brightnessUp = withDevice(deviceLightApi.brightnessUp);
            $ctrl.brightnessDown = withDevice(deviceLightApi.brightnessDown);
            $ctrl.nightLight = withDevice(deviceLightApi.nightLight);
            $ctrl.fullLight = withDevice(deviceLightApi.fullLight);
            $ctrl.switchOn = withDevice(deviceLightApi.switchOn);
            $ctrl.switchOff = withDevice(deviceLightApi.switchOff);
            $ctrl.colorWarmer = withDevice(deviceLightApi.colorWarmer);
            $ctrl.colorColder = withDevice(deviceLightApi.colorColder);

            function init() {
                var getColor = function () {
                    return $ctrl.device.Color + $ctrl.device.LevelInt;
                };

                $scope.$watch(getColor, function (newValue, oldValue) {
                    if (oldValue === newValue) {
                        return;
                    }

                    deviceLightApi.setColor($ctrl.device.idx, $ctrl.device.Color, $ctrl.device.LevelInt)
                });
            }

            function withDevice(fn) {
                return function () {
                    fn($ctrl.device.idx);
                }
            }

            // returns true if the light does not support absolute dimming, used to control display of Brightness Up / Brightness Down buttons
            function isRelativeDimmer() {
                return $ctrl.device.DimmerType && $ctrl.device.DimmerType === 'rel';
            }

            // returns true if the light does not support absolute color temperature, used to control display of Warmer White / Colder White buttons
            function isRelativeColorTemperature() {
                var ledType = getLEDType($ctrl.device.SubType);

                return $ctrl.device.DimmerType && $ctrl.device.DimmerType === 'rel' && ledType.bHasTemperature;
            }
        }
    });

    app.component('deviceSubdevicesEditor', {
        templateUrl: 'views/device/editSubDevices.html',
        bindings: {
            deviceIdx: '<'
        },
        controllerAs: 'vm',
        controller: function ($scope, $element, domoticzApi, deviceApi, dataTableDefaultSettings) {
            var vm = this;
            var table;

            vm.$onInit = init;
            vm.addSubDevice = addSubDevice;
            vm.deleteSubDevice = deleteSubDevice;
            vm.clearSubDevices = clearSubDevices;

            function init() {
                table = $element.find('#subdevicestable').dataTable(Object.assign({}, dataTableDefaultSettings, {
                    sDom: '<"H"frC>t<"F"i>',
                    columns: [
                        { title: $.t('Name'), data: 'Name' }
                    ],
                    select: {
                        className: 'row_selected',
                        style: 'single'
                    },
                }));

                table.on('select.dt', function (e, dt, type, indexes) {
                    var item = dt.rows(indexes).data()[0];
                    vm.selectedSubDeviceIdx = item.ID;
                    $scope.$apply();
                });

                // TODO: Add caching mechanism for this request
                deviceApi.getLightsDevices().then(function (devices) {
                    vm.lightsAndSwitches = devices;
                });

                refreshTable();
            }

            function refreshTable() {
                table.api().clear().draw();

                domoticzApi.sendCommand('getsubdevices', {
                    idx: vm.deviceIdx
                }).then(function (data) {
                    table.api().rows.add(data.result || []).draw();
                });
            }

            function addSubDevice() {
                domoticzApi.sendCommand('addsubdevice', {
                    idx: vm.deviceIdx,
                    subidx: vm.subDeviceIdx
                })
                    .then(refreshTable)
                    .catch(function () {
                        ShowNotify($.t('Problem adding Sub/Slave Device!'), 2500, true);
                    })
            }

            function deleteSubDevice() {
                bootbox.confirm($.t('Are you sure to delete this Sub/Slave Device?\n\nThis action can not be undone...'), function (result) {
                    if (result !== true) {
                        return
                    }

                    domoticzApi.sendCommand('deletesubdevice', {
                        idx: vm.selectedSubDeviceIdx
                    }).then(refreshTable);
                });
            }

            function clearSubDevices() {
                bootbox.confirm($.t('Are you sure to delete ALL Sub/Slave Devices?\n\nThis action can not be undone!'), function (result) {
                    if (result !== true) {
                        return;
                    }

                    domoticzApi.sendCommand('deleteallsubdevices', {
                        idx: vm.deviceIdx
                    }).then(refreshTable);
                });
            }
        }
    });

    app.component('deviceLevelNamesTable', {
        templateUrl: 'views/device/levelNamesTable.html',
        require: {
            ngModelCtrl: 'ngModel'
        },
        controllerAs: 'vm',
        controller: function ($element, $scope, $modal, permissions) {
            var vm = this;

            vm.$onInit = init;
            vm.addLevel = addLevel;
            vm.isAddLevelAvailable = isAddLevelAvailable;

            function init() {
                var table = $element.find('table').dataTable({
                    info: false,
                    searching: false,
                    paging: false,
                    columns: [
                        { title: $.t('Level'), data: 'level', render: levelRenderer },
                        { title: $.t('Level name'), data: 'name' },
                        { title: $.t('Order'), data: 'level', render: orderRenderer },
                        { title: '', data: 'level', render: actionsRenderer },
                    ],
                });

                table.on('click', '.js-order-up', function () {
                    var row = table.api().row($(this).closest('tr')).data();
                    switchLevels(row.level, row.level - 1);
                    $scope.$apply();
                });

                table.on('click', '.js-order-down', function () {
                    var row = table.api().row($(this).closest('tr')).data();
                    switchLevels(row.level, row.level + 1);
                    $scope.$apply();
                });

                table.on('click', '.js-update', function () {
                    var row = table.api().row($(this).closest('tr')).data();

                    var scope = $scope.$new(true);
                    scope.name = row.name;

                    $modal.open({
                        templateUrl: 'views/device/levelRenameModal.html',
                        scope: scope
                    }).result.then(function (name) {
                        setLevelName(row.level, name);
                    });
                });

                table.on('click', '.js-delete', function () {
                    var row = table.api().row($(this).closest('tr')).data();
                    deleteLevel(row.level);
                    $scope.$apply();
                });

                vm.ngModelCtrl.$render = function () {
                    var items = vm.ngModelCtrl.$modelValue;

                    table.api().rows().clear();
                    table.api().rows.add(items).draw();
                };
            }

            function setLevelName(index, name) {
                var levels = vm.ngModelCtrl.$modelValue;
                levels[index].name = name;

                vm.ngModelCtrl.$setViewValue(levels);
                vm.ngModelCtrl.$render();
            }

            function switchLevels(indexA, indexB) {
                var levels = vm.ngModelCtrl.$modelValue;
                var levelAName = levels[indexA].name;
                var levelBName = levels[indexB].name;

                levels[indexA].name = levelBName;
                levels[indexB].name = levelAName;

                vm.ngModelCtrl.$setViewValue(levels);
                vm.ngModelCtrl.$render();
            }

            function isAddLevelAvailable() {
                var isNotEmpty = !!vm.newLevelName;
                var isUnique = !vm.ngModelCtrl.$modelValue.find(function (item) {
                    return item.name === vm.newLevelName
                });

                return isNotEmpty && isUnique;
            }

            function addLevel() {
                var levels = vm.ngModelCtrl.$modelValue;
                var levelName = vm.newLevelName.replace(/[:;|<>]/g, '').trim();

                var newItem = {
                    level: levels.length,
                    name: levelName,
                    action: ''
                };

                vm.ngModelCtrl.$setViewValue(levels.concat(newItem));
                vm.ngModelCtrl.$render();
                vm.newLevelName = '';
            }

            function deleteLevel(index) {
                var levels = vm.ngModelCtrl.$modelValue
                    .filter(function (item) {
                        return item.level !== index;
                    })
                    .map(function (item, index) {
                        return Object.assign({}, item, { level: index });
                    });

                vm.ngModelCtrl.$setViewValue(levels);
                vm.ngModelCtrl.$render();
            }

            function levelRenderer(level) {
                return level * 10;
            }

            function orderRenderer(level) {
                var images = [];

                if (level < (vm.ngModelCtrl.$modelValue.length - 1)) {
                    images.push('<img src="images/down.png" class="lcursor js-order-down" width="16" height="16"></img>');
                } else {
                    images.push('<img src="images/empty16.png" width="16" height="16"></img>');
                }

                if (level > 0) {
                    images.push('<img src="images/up.png" class="lcursor js-order-up" width="16" height="16"></img>');
                }

                return images.join('&nbsp;');
            }

            function actionsRenderer(level) {
                var actions = [];

                if (permissions.hasPermission('Admin')) {
                    actions.push('<img src="images/rename.png" title="' + $.t('Rename') + '" class="lcursor js-update" width="16" height="16"></img>');
                    actions.push('<img src="images/delete.png" title="' + $.t('Delete') + '" class="lcursor js-delete" width="16" height="16"></img>');
                }

                return actions.join('&nbsp;');
            }
        }
    });

    app.component('deviceLevelActionsTable', {
        templateUrl: 'views/device/levelActionsTable.html',
        require: {
            ngModelCtrl: 'ngModel'
        },
        controller: function ($element, $scope, $modal, permissions) {
            var vm = this;

            vm.$onInit = init;

            function init() {
                var table = $element.find('table').dataTable({
                    searching: false,
                    info: false,
                    paging: false,
                    columns: [
                        { title: $.t('Level'), data: 'level', render: levelRenderer },
                        { title: $.t('Action'), data: 'action', render: actionRenderer },
                        { title: '', data: null, render: actionsRenderer }
                    ]
                });

                table.on('click', '.js-delete', function () {
                    var row = table.api().row($(this).closest('tr')).data();
                    clearAction(row.level);
                    $scope.$apply();
                });

                table.on('click', '.js-update', function () {
                    var row = table.api().row($(this).closest('tr')).data();

                    var scope = $scope.$new(true);
                    scope.action = row.action;

                    $modal.open({
                        templateUrl: 'views/device/editSelectorActionModal.html',
                        scope: scope
                    }).result.then(function (action) {
                        setLevelAction(row.level, action);
                    });
                });

                vm.ngModelCtrl.$render = function () {
                    var items = vm.ngModelCtrl.$modelValue;

                    table.api().rows().clear();
                    table.api().rows.add(items).draw();
                };
            }

            function setLevelAction(index, action) {
                var levels = vm.ngModelCtrl.$modelValue;
                levels[index].action = action;

                vm.ngModelCtrl.$setViewValue(levels);
                vm.ngModelCtrl.$render();
            }

            function clearAction(index) {
                var items = vm.ngModelCtrl.$modelValue;

                items[index].action = '';

                vm.ngModelCtrl.$setViewValue(items);
                vm.ngModelCtrl.$render();
            }

            function levelRenderer(level) {
                return level * 10;
            }

            var escapeHTML = function (unsafe) {
                return unsafe.replace(/[&<"']/g, function (m) {
                    switch (m) {
                        case '&':
                            return '&amp;';
                        case '<':
                            return '&lt;';
                        case '"':
                            return '&quot;';
                        default:
                            return '&#039;';
                    }
                });
            };

            function actionRenderer(action) {
                return escapeHTML(action);
            }

            function actionsRenderer() {
                var actions = [];

                if (permissions.hasPermission('Admin')) {
                    actions.push('<img src="images/rename.png" title="' + $.t('Edit') + '" class="lcursor js-update" width="16" height="16"></img>');
                    actions.push('<img src="images/delete.png" title="' + $.t('Clear') + '" class="lcursor js-delete" width="16" height="16"></img>');
                }

                return actions.join('&nbsp;');
            }
        }
    });

    app.controller('DeviceLightEditController', function ($timeout, $routeParams, $window, domoticzApi, deviceApi) {
        var vm = this;
        var $element = $('.js-view:last');

        vm.updateDevice = updateDevice;
        vm.removeDevice = removeDevice;
        vm.isSecurityDevice = isSecurityDevice;
        vm.isMotionAvailable = isMotionAvailable;
        vm.isOnDelayAvailable = isOnDelayAvailable;
        vm.isOffDelayAvailable = isOffDelayAvailable;
        vm.isSwitchIconAvailable = isSwitchIconAvailable;
        vm.isLevelsAvailable = isLevelsAvailable;
        vm.isOnActionAvailable = isOnActionAvailable;
        vm.isOffActionAvailable = isOffActionAvailable;
        vm.isColorSettingsAvailable = isColorSettingsAvailable;
        vm.isWhiteSettingsAvailable = isWhiteSettingsAvailable;

        init();

        function init() {
            vm.deviceIdx = $routeParams.id;
            vm.switchTypeOptions = [];

            deviceApi.getDeviceInfo(vm.deviceIdx).then(function (device) {
                vm.device = device;
                vm.device.StrParam1 = b64DecodeUnicode(vm.device.StrParam1);
                vm.device.StrParam2 = b64DecodeUnicode(vm.device.StrParam2);

                var defaultLevelNames = ['Off', 'Level1', 'Level2', 'Level3'];

                var levelNames = device.getLevels() || defaultLevelNames;
                var levelActions = device.getLevelActions();

                vm.levels = levelNames.map(function (level, index) {
                    return {
                        level: index,
                        name: level,
                        action: levelActions[index] ? levelActions[index] : ''
                    };
                });
            });

            $element.find('#switch-types-template > option').each(function () {
                vm.switchTypeOptions.push({
                    label: $(this).text(),
                    value: parseInt($(this).val())
                });
            });
        }

        function updateDevice() {
            var options = [];

            if (isLevelsAvailable()) {
                var levelParams = vm.levels.reduce(function (acc, level) {
                    return {
                        names: acc.names.concat(level.name),
                        actions: acc.actions.concat(level.action)
                    };
                }, { names: [], actions: [] });

                options.push('LevelNames:' + levelParams.names.join('|'));
                options.push('LevelActions:' + levelParams.actions.join('|'));
                options.push('SelectorStyle:' + vm.device.SelectorStyle);
                options.push('LevelOffHidden:' + vm.device.LevelOffHidden);
            }
            var params = {
                type: 'setused',
                name: vm.device.Name,
                description: vm.device.Description,
                strparam1: b64EncodeUnicode(vm.device.StrParam1),
                strparam2: b64EncodeUnicode(vm.device.StrParam2),
                protected: vm.device.Protected,
                options: b64EncodeUnicode(options.join(';')),
                addjvalue: vm.device.AddjValue,
                addjvalue2: vm.device.AddjValue2,
                customimage: vm.device.CustomImage,
                switchtype: vm.device.SwitchTypeVal,
                used: true
            };

            deviceApi.updateDeviceInfo(vm.deviceIdx, params).then(function () {
                $window.history.back();
            });
        }

        function removeDevice() {
            bootbox.confirm($.t('Are you sure to remove this Light/Switch?'), function (result) {
                if (!result) {
                    return;
                }

                deviceApi.removeDevice(vm.deviceIdx).then(function () {
                    $window.history.back();
                });
            });
        }

        function isSecurityDevice() {
            return vm.device.Type === 'Security';
        }

        function isMotionAvailable() {
            return vm.device.SwitchTypeVal === 8;
        }

        function isOnActionAvailable() {
            return vm.device.SwitchTypeVal !== 18;
        }

        function isOffActionAvailable() {
            return isOnActionAvailable();
        }

        function isOnDelayAvailable() {
            return [0, 7, 9, 11, 18].includes(vm.device.SwitchTypeVal);
        }

        function isOffDelayAvailable() {
            return [0, 7, 9, 11, 18, 19, 20].includes(vm.device.SwitchTypeVal);
        }

        function isSwitchIconAvailable() {
            return vm.device.icon.isConfigurable();
        }

        function isLevelsAvailable() {
            return vm.device.SwitchTypeVal === 18;
        }

        function isColorSettingsAvailable() {
            return vm.device.isLED();
        }

        function isWhiteSettingsAvailable() {
            return vm.device.SubType === 'White';
        }
    });
});
