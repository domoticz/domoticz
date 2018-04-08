define(['app'], function (app) {

    app.component('deviceIconSelect', {
        template: '<select id="icon-select"></select>',
        require: {
            ngModelCtrl: 'ngModel'
        },
        controller: function ($element, domoticzApi) {
            var vm = this;
            var icons = [];

            vm.$onInit = function () {
                // TODO: Add caching mechanism for this request
                domoticzApi.sendRequest({
                    type: 'custom_light_icons'
                }).then(function (data) {
                    icons = (data.result || []).map(function (item) {
                        return {
                            text: item.text,
                            value: item.idx,
                            selected: false,
                            description: item.description,
                            imageSrc: 'images/' + item.imageSrc + '48_On.png'
                        };
                    });

                    $element.find('#icon-select').ddslick({
                        data: icons,
                        width: 260,
                        height: 390,
                        selectText: "Select Switch Icon",
                        imagePosition: "left",
                        onSelected: function (data) {
                            vm.ngModelCtrl.$setViewValue(data.selectedData.value);
                        }
                    });

                    vm.ngModelCtrl.$render();
                });

                vm.ngModelCtrl.$render = function () {
                    var value = vm.ngModelCtrl.$modelValue;

                    icons.forEach(function (item, index) {
                        if (item.value === value) {
                            $element.find('#icon-select').ddslick('select', {index: index});
                        }
                    });
                };
            };
        }
    });

    // TODO: Try to reuse this component
    app.component('rgbwPicker', {
        templateUrl: 'views/rgbwPicker.html',
        bindings: {
            device: '<'
        },
        controller: function ($element, domoticzApi, deviceLightApi) {
            var $ctrl = this;

            $ctrl.$onInit = init;
            $ctrl.isBrightnessAvailable = isBrightnessAvailable;
            $ctrl.isLightControlAvailable = isLightControlAvailable;
            $ctrl.brightnessUp = withDevice(deviceLightApi.brightnessUp);
            $ctrl.brightnessDown = withDevice(deviceLightApi.brightnessDown);
            $ctrl.nighLight = withDevice(deviceLightApi.nighLight);
            $ctrl.fullLight = withDevice(deviceLightApi.fullLight);
            $ctrl.switchOff = withDevice(deviceLightApi.switchOff);

            function init() {
                var maxDimLevel = 100;
                var className = 'js-rgbwPicker-' + (+new Date());
                $element.addClass(className);

                ShowRGBWPicker(
                    '.' + className,
                    $ctrl.device.idx,
                    $ctrl.device.Protected,
                    $ctrl.device.MaxDimLevel || maxDimLevel,
                    $ctrl.device.LevelInt || 0,
                    '',
                    $ctrl.device.SubType,
                    updateColor
                )
            }

            function withDevice(fn) {
                return function () {
                    fn($ctrl.device.idx);
                }
            }

            function isBrightnessAvailable() {
                var ledType = getLEDType($ctrl.device.SubType);
                var isColorSwitch = $ctrl.device.Type === 'Color Switch';

                return ledType.bHasRGB && $ctrl.device.Unit === 0 && isColorSwitch
            }

            function isLightControlAvailable() {
                var ledType = getLEDType($ctrl.device.SubType);
                var isColorSwitch = $ctrl.device.Type === 'Color Switch';

                return (ledType.bHasRGB === true && $ctrl.device.Unit === 0 && isColorSwitch)
                    || (ledType.bHasTemperature && isColorSwitch)
                    || (ledType.bHasWhite && !ledType.bHasTemperature && isColorSwitch)
            }

            function updateColor(deviceIdx, JSONColor, dimlevel) {
                deviceLightApi.setColor(deviceIdx, JSONColor, dimlevel);
            }
        }
    });

    app.component('whitePicker', {
        templateUrl: 'views/whitePicker.html',
        bindings: {
            deviceIdx: '<'
        },
        controller: function (deviceLightApi) {
            var $ctrl = this;

            $ctrl.brightnessUp = withDevice(deviceLightApi.brightnessUp);
            $ctrl.brightnessDown = withDevice(deviceLightApi.brightnessDown);
            $ctrl.nighLight = withDevice(deviceLightApi.nighLight);
            $ctrl.fullLight = withDevice(deviceLightApi.fullLight);
            $ctrl.colorWarmer = withDevice(deviceLightApi.colorWarmer);
            $ctrl.colorColder = withDevice(deviceLightApi.colorColder);

            function withDevice(fn) {
                return function () {
                    fn($ctrl.deviceIdx);
                }
            }
        }
    });

    app.component('deviceSubdevicesEditor', {
        templateUrl: 'views/device/editSubDevices.html',
        bindings: {
            deviceIdx: '<'
        },
        controllerAs: 'vm',
        controller: function ($scope, $element, domoticzApi, dataTableDefaultSettings) {
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
                        {title: $.t('Name'), data: 'Name'}
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
                domoticzApi.sendCommand('getlightswitches').then(function (data) {
                    vm.lightsAndSwitches = (data.result || [])
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
                bootbox.confirm($.t("Are you sure to delete this Sub/Slave Device?\n\nThis action can not be undone..."), function (result) {
                    if (result !== true) {
                        return
                    }

                    domoticzApi.sendCommand('deletesubdevice', {
                        idx: vm.selectedSubDeviceIdx
                    }).then(refreshTable);
                });
            }

            function clearSubDevices() {
                bootbox.confirm($.t("Are you sure to delete ALL Sub/Slave Devices?\n\nThis action can not be undone!"), function (result) {
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
                        {title: $.t('Level'), data: 'level', render: levelRenderer},
                        {title: $.t('Level name'), data: 'name'},
                        {title: $.t('Order'), data: 'level', render: orderRenderer},
                        {title: '', data: 'level', render: actionsRenderer},
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
                    }).result.then(function(name) {
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
                var isLessThanLimit = vm.ngModelCtrl.$modelValue.length < 11;
                var isNotEmpty = !!vm.newLevelName;
                var isUnique = !vm.ngModelCtrl.$modelValue.find(function (item) {
                    return item.name === vm.newLevelName
                });

                return isLessThanLimit && isNotEmpty && isUnique;
            }

            function addLevel() {
                var levels = vm.ngModelCtrl.$modelValue;
                var levelName = vm.newLevelName.replace(/[:;|<>]/g, "").trim();

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
                        return Object.assign({}, item, {level: index});
                    });

                vm.ngModelCtrl.$setViewValue(levels);
                vm.ngModelCtrl.$render();
            }

            function levelRenderer(level) {
                return level * 10;
            }

            function orderRenderer(level) {
                var images = [];

                if (level > 0 && level < (vm.ngModelCtrl.$modelValue.length - 1)) {
                    images.push('<img src="images/down.png" class="lcursor js-order-down" width="16" height="16"></img>');
                } else {
                    images.push('<img src="images/empty16.png" width="16" height="16"></img>');
                }

                if (level >= 2) {
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

                return level === 0
                    ? ''
                    : actions.join('&nbsp;');
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
                        {title: $.t('Level'), data: 'level', render: levelRenderer},
                        {title: $.t('Action'), data: 'action'},
                        {title: '', data: null, render: actionsRenderer}
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
                    }).result.then(function(action) {
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
                vm.device.StrParam1 = atob(vm.device.StrParam1);
                vm.device.StrParam2 = atob(vm.device.StrParam2);

                var levelNames = device.LevelNames
                    ? device.LevelNames.split('|')
                    : ['Off', 'Level1', 'Level2', 'Level3'];
                var levelActions = device.LevelActions
                    ? device.LevelActions.split('|')
                    : [];

                vm.levels = levelNames.map(function (level, index) {
                    return {
                        level: index,
                        name: level,
                        action: levelActions[index]
                            ? unescape(levelActions[index])
                            : ''
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
                        names: acc.names.concat(encodeURIComponent(level.name)),
                        actions: acc.actions.concat(encodeURIComponent(level.action))
                    };
                }, {names: [], actions: []});

                options.push('LevelNames:' + levelParams.names.join('|'));
                options.push('LevelActions:' + levelParams.actions.join('|'));
                options.push('SelectorStyle:' + vm.device.SelectorStyle);
                options.push('LevelOffHidden:' + vm.device.LevelOffHidden);
            }

            var params = {
                type: 'setused',
                name: vm.device.Name,
                description: vm.device.Description,
                strparam1: btoa(vm.device.StrParam1),
                strparam2: btoa(vm.device.StrParam2),
                protected: vm.device.Protected,
                options: btoa(encodeURIComponent(options.join(';'))),
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

                if (vm.device.IsSubDevice) {
                    bootbox.confirm({
                        message: [
                            $.t('This device is used as a Sub/Slave device for other Devices.'),
                            $.t('Do you want to remove this device from those devices too?')
                        ].join('<br>'),
                        buttons: {
                            confirm: {
                                label: $.t('Yes'),
                                className: 'btn-default'
                            },
                            cancel: {
                                label: $.t('No'),
                                className: 'btn-default'
                            }
                        },
                        callback: function (removeSubDevices) {
                            deviceApi.updateDeviceInfo(vm.deviceIdx, {
                                used: false,
                                RemoveSubDevices: removeSubDevices
                            }).then(function () {
                                $window.history.back();
                            });
                        }
                    })
                } else {
                    deviceApi.updateDeviceInfo(vm.deviceIdx, {
                        used: false,
                        RemoveSubDevices: false
                    }).then(function () {
                        $window.history.back();
                    });
                }
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
            return [0, 7, 9, 18].includes(vm.device.SwitchTypeVal);
        }

        function isOffDelayAvailable() {
            return [0, 7, 9, 18, 19, 20].includes(vm.device.SwitchTypeVal);
        }

        function isSwitchIconAvailable() {
            return [0, 7, 17, 18].includes(vm.device.SwitchTypeVal);
        }

        function isLevelsAvailable() {
            return vm.device.SwitchTypeVal === 18;
        }

        function isColorSettingsAvailable() {
            return isLED(vm.device.SubType);
        }

        function isWhiteSettingsAvailable() {
            return vm.device.SubType === 'White';
        }
    });
});