define(['app', 'livesocket'], function(app) {

    var addDeviceModal = {
        templateUrl: 'app/devices/deviceAddModal.html',
        controllerAs: '$ctrl',
        controller: function($scope, deviceApi) {
            var $ctrl = this;
            init();

            function init() {
                $ctrl.device = Object.assign($scope.device);
                $ctrl.isLightDevice = $ctrl.device.Type.indexOf('Light') === 0 || $ctrl.device.Type.indexOf('Security') === 0;
                $ctrl.isMainDevice = true;

                if ($ctrl.isLightDevice) {
                    deviceApi.getLightsDevices().then(function(devices) {
                        $ctrl.mainDevices = devices;
                    });
                }
            }

            $ctrl.addDevice = function() {
                $ctrl.isSaving = true;
                var mainDevice = $ctrl.isMainDevice ? undefined : $ctrl.mainDevice;

                deviceApi.includeDevice($ctrl.device.idx, $ctrl.device.Name, mainDevice)
                    .then($scope.$close);
            }
        }
    };

    var renameDeviceModal = {
        templateUrl: 'app/devices/deviceRenameModal.html',
        controllerAs: '$ctrl',
        controller: function($scope, deviceApi, sceneApi) {
            var $ctrl = this;
            $ctrl.device = Object.assign($scope.device);

            $ctrl.renameDevice = function() {
                $ctrl.isSaving = true;

                var savingPromise = ['Scene', 'Group'].includes($ctrl.device.Type)
                    ? sceneApi.renameScene($ctrl.device.idx, $ctrl.device.Name)
                    : deviceApi.renameDevice($ctrl.device.idx, $ctrl.device.Name);

                savingPromise.then(function() {
                    $scope.$close();
                });
            }
        }
    };

    app.component('devicesTable', {
        bindings: {
            devices: '<',
            onUpdate: '&'
        },
        template: '<table id="devices" class="display" width="100%"></table>',
        controller: function($scope, $element, $uibModal, $route, bootbox, dataTableDefaultSettings, deviceApi) {
            var $ctrl = this;
            var table;

            $ctrl.$onInit = function() {
                table = $element.find('table').dataTable(Object.assign({}, dataTableDefaultSettings, {
                    select: {
                        style: 'multi',
                        className: 'row_selected',
                        selector: '.js-select-row'
                    },
                    order: [[13, 'desc']],
                    columns: [
                        {
                            title: renderSelectorTitle(),
                            width: '16px',
                            orderable: false,
                            defaultContent: selectorRenderer()
                        },
                        {
                            title: renderDeviceStateTitle(),
                            width: '16px',
                            data: 'idx',
                            orderable: false,
                            render: iconRenderer
                        },
                        { title: $.t('Idx'), width: '30px', data: 'idx' },
                        { title: $.t('Hardware'), width: '100px', data: 'HardwareName' },
                        {
                            title: $.t('ID'),
                            width: '70px',
                            data: 'ID',
                            render: idRenderer
                        },
                        { title: $.t('Unit'), width: '40px', data: 'Unit' },
                        { title: $.t('Name'), width: '200px', data: 'Name' },
                        { title: $.t('Type'), width: '110px', data: 'Type' },
                        { title: $.t('SubType'), width: '110px', data: 'SubType' },
                        { title: $.t('Data'), data: 'Data' },
                        { title: renderSignalLevelTitle(), width: '30px', data: 'SignalLevel' },
                        {
                            title: renderBatteryLevelTitle(),
                            width: '30px',
                            type: 'number',
                            data: 'BatteryLevel',
                            render: batteryLevelRenderer
                        },
                        {
                            title: '',
                            className: 'actions-column',
                            width: '80px',
                            data: 'idx',
                            orderable: false,
                            render: actionsRenderer
                        },
                        { title: $.t('Last Seen'), width: '150px', data: 'LastUpdate', type: 'date-us' },
                    ]
                }));

                table.on('click', '.js-include-device', function() {
                    var row = table.api().row($(this).closest('tr')).data();
                    var scope = $scope.$new(true);
                    scope.device = row;

                    $uibModal
                        .open(Object.assign({ scope: scope }, addDeviceModal)).result
                        .then($ctrl.onUpdate);

                    $scope.$apply();
                });

                table.on('click', '.js-exclude-device', function() {
                    var row = table.api().row($(this).closest('tr')).data();

                    bootbox.confirm('Are you sure to remove this Device from your used devices?')
                        .then(function() {
                            return deviceApi.excludeDevice(row.idx);
                        })
                        .then($ctrl.onUpdate);

                    $scope.$apply();
                });

                table.on('click', '.js-rename-device', function() {
                    var row = table.api().row($(this).closest('tr')).data();
                    var scope = $scope.$new(true);
                    scope.device = row;

                    $uibModal
                        .open(Object.assign({ scope: scope }, renameDeviceModal)).result
                        .then($ctrl.onUpdate);

                    $scope.$apply();
                });

                table.on('click', '.js-show-log', function() {
                    var device = table.api().row($(this).closest('tr')).data();

                    device.openCustomLog('#devicescontent', function() {
                        $route.reload();
                    });

                    $scope.$apply();
                });

                table.on('click', '.js-remove-device', function() {
                    var device = table.api().row($(this).closest('tr')).data();

                    bootbox.confirm('Are you sure to delete this Device?\n\nThis action can not be undone...')
                        .then(function() {
                            return deviceApi.removeDevice(device.idx);
                        })
                        .then($ctrl.onUpdate);
                });

                table.on('click', '.js-remove-selected', function() {
                    var selected_items = [].map.call(table.api().rows({ selected: true }).data(), function(item) {
                        var obj = {
                            idx: item.idx,
                            type: item.Type
                        };
                        return obj;
                    });
                    if (selected_items.length === 0) {
                        return bootbox.alert('No Items selected to Delete!');
                    }
                    var devices = [];
                    var scenes = [];

                    selected_items.forEach(function(item) {
                        if ((item.type != 'Group') && (item.type != 'Scene')) {
                            devices.push(item.idx);
                        } else {
                            scenes.push(item.idx);
                        }
                    });

                    bootbox.confirm($.t('Are you sure you want to delete the selected Devices?') + ' (' + (devices.length + scenes.length) + ')')
                        .then(function() {
                            ShowNotify($.t("Removing..."), 30000);
                            if (devices.length > 0) {
                                ret = deviceApi.removeDevice(devices);
                            }
                            if (scenes.length > 0) {
                                ret = deviceApi.removeScene(scenes);
                            }
                            HideNotify();
                            return ret;
                        })
                        .then(function() {
                            bootbox.alert((devices.length + scenes.length) + ' ' + $.t('Devices deleted.'));
                            $ctrl.onUpdate();
                        });
                });

                table.on('click', '.js-toggle-state', function() {
                    var device = table.api().row($(this).closest('tr')).data();
                    device.toggle();
                    $scope.$apply();
                });

                table.on('change', '.js-select-devices', function() {
                    if (this.checked) {
                        table.api().rows({ page: 'current' }).select();
                    } else {
                        table.api().rows({ page: 'current' }).deselect();
                    }

                    table.find('.js-select-row').attr('checked', this.checked);
                    $scope.$apply();
                });

                table.on('select.dt', function() {
                    updateDeviceDeleteBtnState();
                    $scope.$apply();
                });

                table.on('deselect.dt', function() {
                    updateDeviceDeleteBtnState();
                    $scope.$apply();
                });

                $scope.$on('device_update', updateItem);
                $scope.$on('scene_update', updateItem);

                table.api().rows
                    .add($ctrl.devices)
                    .draw();

                updateDeviceDeleteBtnState();
            };

            $ctrl.$onChanges = function(changes) {
                if (!table) {
                    return;
                }

                if (changes.devices) {
                    table.api().clear();
                    table.api().rows
                        .add($ctrl.devices)
                        .draw();
                }
            };

            $ctrl.getSelectedRecordsCounts = function() {
                return table.api().rows({ selected: true }).count()
            };

            function updateItem(event, itemData) {
                table.api().rows().every(function() {
                    var device = this.data();

                    if (device.idx === itemData.idx && device.Type === itemData.Type) {
                        this.data(Object.assign(device, itemData));
                        table.find('.row_selected .js-select-row').prop('checked', true);
                    }
                });
            }

            function updateDeviceDeleteBtnState() {
                if ($ctrl.getSelectedRecordsCounts() > 0) {
                    table.find('.js-remove-selected').show();
                } else {
                    table.find('.js-remove-selected').hide();
                }
            }

            function selectorRenderer() {
                return '<input type="checkbox" class="noscheck js-select-row" />';
            }

            function idRenderer(value, type, device) {
                if (device.isScene()) {
                    return "-";
                }

                var ID = device.ID;
                if (typeof (device.HardwareTypeVal) != 'undefined' && device.HardwareTypeVal == 21) {
                    if (device.ID.substr(-4, 2) == '00') {
                        ID = device.ID.substr(1, device.ID.length - 2) + '<span class="ui-state-default">' + device.ID.substr(-2, 2) + '</span>';
                    } else {
                        ID = device.ID.substr(1, device.ID.length - 4) + '<span class="ui-state-default">' + device.ID.substr(-4, 2) + '</span>' + device.ID.substr(-2, 2);
                    }
                }
                /*
                Not sure why this was used
                                if (device.Type == "Lighting 1") {
                                    ID = String.fromCharCode(device.ID);
                                }
                */
                return ID;
            }

            function iconRenderer(value, type, device) {
                var itemImage = '<img src="' + device.icon.getIcon() + '" width="16" height="16">';

                var isToggleAvailable =
                    (['Light/Switch', 'Lighting 2'].includes(device.Type) && [0, 7, 9, 10].includes(device.SwitchTypeVal))
                    || device.Type === 'Color Switch'
                    || device.Type === 'Chime'
                    || device.isScene();

                if (isToggleAvailable) {
                    var title = device.isActive() ? $.t('Turn Off') : $.t('Turn On');
                    return '<button class="btn btn-icon js-toggle-state" title="' + title + '">' + itemImage + '</button>';
                } else {
                    return itemImage;
                }
            }

            function actionsRenderer(value, type, device) {
                var actions = [];
                var logLink = device.getLogLink();
                var isScene = device.isScene();
                var isCustomLog = device.isCustomLog();

                if (isScene) {
                    actions.push('<img src="images/empty16.png">');
                } else if (device.Used !== 0) {
                    actions.push('<button class="btn btn-icon js-exclude-device" title="' + $.t('Set Unused') + '"><img src="images/remove.png" /></button>');
                } else {
                    actions.push('<button class="btn btn-icon js-include-device" title="' + $.t('Add Device') + '"><img src="images/add.png" /></button>');
                }

                actions.push('<button class="btn btn-icon js-rename-device" title="' + $.t('Rename Device') + '"><img src="images/rename.png" /></button>');

                if (isScene) {
                    actions.push('<a class="btn btn-icon" href="#/Scenes/' + device.idx + '/Log" title="' + $.t('Log') + '"><img src="images/log.png" /></a>');
                } else if (logLink) {
                    actions.push('<a class="btn btn-icon" href="' + logLink + '" title="' + $.t('Log') + '"><img src="images/log.png" /></a>');
                } else if (isCustomLog) {
					actions.push('<button class="btn btn-icon js-show-log" title="' + $.t('Log') + '"><img src="images/log.png" /></button>');
                }

                actions.push('<button class="btn btn-icon js-remove-device" title="' + $.t('Remove') + '"><img src="images/delete.png" /></button>');

                return actions.join('&nbsp;');
            }

            function batteryLevelRenderer(value, type) {
                if (!['display', 'filter'].includes(type)) {
                    return value;
                }

                if (value === 255) {
                    return '-'
                }

                var className = value < 10 ? 'empty' : value < 40 ? 'half' : 'full';
                var width = Math.ceil(value * 14 / 100);
                var title = $.t('Battery level') + ': ' + value + '%';

                return '<div class="battery ' + className + '" style="width: ' + width + 'px" title="' + title + '"></div>';
            }

            function renderBatteryLevelTitle() {
                return '<img src="images/battery.png" style="transform: rotate(180deg);" title="' + $.t('Battery Level') + '">'
            }

            function renderSignalLevelTitle() {
                return '<img src="images/air_signal.png" title="' + $.t('RF Signal Level') + '">'
            }

            function renderDeviceStateTitle() {
                return '<button class="btn btn-icon js-remove-selected" title="' + $.t('Delete selected device(s)') + '"><img src="images/delete.png" /></button>';
            }

            function renderSelectorTitle() {
                return '<input class="noscheck js-select-devices" type="checkbox" />';
            }
        }
    });

    app.component('devicesFilters', {
        bindings: {
            devices: '<',
        },
        require: {
            ngModelCtrl: 'ngModel'
        },
        templateUrl: 'app/devices/deviceFilters.html',
        controller: function(domoticzApi) {
            var $ctrl = this;
            var filterAdditionalDataPromise;

            $ctrl.filters = [
                {
                    field: 'Used',
                    name: $.t('Used'),
                    display: function(value) {
                        return value === 0 ? 'No' : 'Yes';
                    },
                    parse: function(value) {
                        return parseInt(value, 10)
                    }
                },
                { field: 'HardwareName', name: $.t('Hardware') },
                { field: 'Type', name: $.t('Type') },
                {
                    field: 'PlanIDs',
                    name: $.t('Room'),
                    display: function(value) {
                        var roomPlan = $ctrl.plans.find(function(item) {
                            return parseInt(item.idx) === value;
                        });

                        if (roomPlan) {
                            return roomPlan['Name'];
                        } else {
                            return $.t('- N/A -')
                        }
                    },
                    parse: function(value) {
                        return parseInt(value, 10)
                    }
                }
            ];

            $ctrl.$onInit = function() {
                $ctrl.filters = $ctrl.filters.map(function(filter) {
                    return Object.assign({ collapsed: false }, filter)
                });

                $ctrl.ngModelCtrl.$render = function() {
                    var value = $ctrl.ngModelCtrl.$modelValue;

                    $ctrl.filterValue = Object.keys(value)
                        .filter(function(fieldName) {
                            return value[fieldName] !== undefined;
                        })
                        .reduce(function(acc, fieldName) {
                            var fieldFilterValue = value[fieldName].reduce(function(acc, fieldValue) {
                                acc[fieldValue] = true;
                                return acc
                            }, {});

                            acc[fieldName] = fieldFilterValue;
                            return acc;
                        }, {});
                };
            };

            $ctrl.$onChanges = function(changes) {
                if (!filterAdditionalDataPromise) {
                    filterAdditionalDataPromise = loadRooms();
                }

                if (changes.devices && changes.devices.currentValue) {
                    filterAdditionalDataPromise.then(function() {
                        initFilters($ctrl.devices);
                    });
                }
            };

            $ctrl.updateFilterValue = function(filterValue) {
                var value = Object.keys(filterValue).reduce(function(acc, fieldName) {
                    var filter = $ctrl.filters.find(function(item) {
                        return item.field === fieldName;
                    });

                    var filterFieldValue = Object.keys(filterValue[fieldName])
                        .filter(function(item) {
                            return filterValue[fieldName][item] === true
                        })
                        .map(function(value) {
                            return filter.parse ? filter.parse(value) : value;
                        });

                    if (filterFieldValue.length > 0) {
                        acc[fieldName] = filterFieldValue;
                    }

                    return acc;
                }, {});

                $ctrl.ngModelCtrl.$setViewValue(value);
            };

            function initFilters(devices) {
                $ctrl.filterValues = (devices || [])
                    .reduce(function(acc, device) {
                        $ctrl.filters.forEach(function(item, index) {
                            if (!acc[index]) {
                                acc[index] = []
                            }

                            if (device[item.field] === undefined) {
                                return;
                            }

                            var values = Array.isArray(device[item.field])
                                ? device[item.field]
                                : [device[item.field]];

                            values.forEach(function(value) {
                                if (!acc[index].includes(value)) {
                                    acc[index].push(value)
                                }
                            });
                        });

                        return acc;
                    }, [])
                    .map(function(values, filterIndex) {
                        var displayFn = $ctrl.filters[filterIndex].display || function(value) {
                            return value
                        };

                        values.sort(function(value1, value2) {
                            return displayFn(value1) > displayFn(value2) ? 1 : -1;
                        });

                        return values;
                    });
            }

            function loadRooms() {
                return domoticzApi.sendRequest({
                    type: 'plans',
                    displayhidden: 0,
                })
                    .then(domoticzApi.errorHandler)
                    .then(function(response) {
                        $ctrl.plans = response.result || []
                    });
            }
        }
    });

    app.component('deviceFilterByUsage', {
        templateUrl: 'app/devices/deviceFilterByUsage.html',
        require: {
            ngModelCtrl: 'ngModel'
        },
        controller: function() {
            var $ctrl = this;

            $ctrl.$onInit = function() {
                $ctrl.ngModelCtrl.$render = function() {
                    $ctrl.value = $ctrl.ngModelCtrl.$viewValue;
                };

                $ctrl.ngModelCtrl.$parsers.push(function(value) {
                    return Object.assign({}, $ctrl.ngModelCtrl.$modelValue, {
                        Used: value && value.length === 1 ? value : undefined
                    });
                });

                $ctrl.ngModelCtrl.$formatters.push(function(value) {
                    var filterValue = value && value.Used;

                    return (Array.isArray(filterValue) && filterValue.length === 1)
                        ? filterValue[0]
                        : undefined;
                });
            };

            $ctrl.setFilter = function(value) {
                var filterValue = value !== undefined
                    ? [value]
                    : [0, 1];

                $ctrl.value = value;
                $ctrl.ngModelCtrl.$setViewValue(filterValue)
            };
        }
    });

    app.controller('DevicesController', function($scope, domoticzApi, livesocket, Device) {
        var $ctrl = this;
        $ctrl.refreshDevices = refreshDevices;
        $ctrl.applyFilter = applyFilter;

        init();

        function init() {
            $ctrl.isListExpanded = false;
            $ctrl.filter = {};
            $ctrl.refreshDevices();

            $scope.$on('device_update', updateItem);
            $scope.$on('scene_update', updateItem);
        }

        function updateItem(event, itemData) {
            var device = $ctrl.devices.find(function(device) {
                return device.idx === itemData.idx && device.Type === itemData.Type;
            });

            if (device) {
                Object.assign(device, itemData);
            } else {
                $ctrl.devices.push(new Device(itemData))
            }
        }

        function refreshDevices() {
            domoticzApi.sendRequest({
                type: 'devices',
                displayhidden: 1,
                filter: 'all',
                used: 'all'
            })
                .then(domoticzApi.errorHandler)
                .then(function(response) {
                    if (response.result !== undefined) {
                        $ctrl.devices = response.result
                            .map(function(item) {
                                var isScene = ['Group', 'Scene'].includes(item.Type);

                                if (isScene) {
                                    item.HardwareName = 'Domoticz';
                                    item.ID = '-';
                                    item.Unit = '-';
                                    item.SubType = '-';
                                    item.SignalLevel = '-';
                                    item.BatteryLevel = 255;
                                }

                                return new Device(item)
                            });
                    } else {
                        $ctrl.devices = [];
                    }
                    $ctrl.applyFilter();
                });
        }

        function applyFilter() {
            $ctrl.filteredDevices = ($ctrl.devices || []).filter(function(device) {
                return Object.keys($ctrl.filter)
                    .filter(function(fieldName) {
                        return $ctrl.filter[fieldName] !== undefined
                    })
                    .every(function(fieldName) {
                        return Array.isArray(device[fieldName])
                            ? device[fieldName].some(function(value) {
                                return $ctrl.filter[fieldName].includes(value)
                            })
                            : $ctrl.filter[fieldName].includes(device[fieldName])
                    });
            });
        }
    });
});
