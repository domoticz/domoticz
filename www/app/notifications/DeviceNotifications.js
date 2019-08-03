define(['app', 'notifications/constants', 'notifications/factories'], function (app, deviceNotificationsConstants) {

    app.component('deviceNotificationsTable', {
        bindings: {
            notifications: '<',
            device: '<',
            onSelect: '&'
        },
        template: '<table class="display"></table>',
        controller: function ($scope, $element, dataTableDefaultSettings) {
            var $ctrl = this;
            var table;

            $ctrl.$onInit = function () {
                table = $element.find('table').dataTable(Object.assign({}, dataTableDefaultSettings, {
                    columns: [
                        {title: $.t('Type'), width: '120px', data: 'Params', render: typeRenderer},
                        {title: $.t('When'), width: '160px', data: 'Params', render: whenRenderer},
                        {title: $.t('Active Systems'), data: 'ActiveSystems', render: activeSystemsRenderer},
                        {title: $.t('Custom Message'), data: 'CustomMessage', render: customMessageRenderer},
                        {title: $.t('Priority'), width: '120px', data: 'Priority', render: priorityRenderer},
                        {title: $.t('Ignore Interval'), width: '120px', data: 'SendAlways', render: sendAlwaysRenderer},
                        {title: $.t('Recovery'), width: '80px', data: 'Params', render: recoveryRenderer}
                    ]
                })).api();

                table.on('select.dt', function (e, dt, type, indexes) {
                    var item = dt.rows(indexes).data()[0];
                    $ctrl.onSelect({value: item});
                    $scope.$apply();
                });

                table.on('deselect.dt', function () {
                    $ctrl.onSelect(null);
                    $scope.$apply();
                });
            };

            $ctrl.$onChanges = function (changes) {
                if (!table) {
                    return;
                }

                if (changes.notifications) {
                    table.clear();
                    table.rows
                        .add($ctrl.notifications)
                        .draw();
                }
            };

            function typeRenderer(value) {
                var parts = value.split(';');
                var type = parts[0];

                return deviceNotificationsConstants.typeNameByTypeMap[type] || '';
            }

            function whenRenderer(value) {
                var parts = value.split(';');
                var type = parts[0];
                var condition = parts[1] || '';
                var nvalue = parts[2];

                if (type === 'S' && $ctrl.device.isSelector()) {
                    var levels = $ctrl.device.getLevels();
                    return $.t('On') + ' (' + levels[nvalue / 10] + ')';
                } else if (deviceNotificationsConstants.whenByTypeMap[type]) {
                    return deviceNotificationsConstants.whenByTypeMap[type]
                } else if (deviceNotificationsConstants.whenByConditionMap[condition]) {
                    var data = [
                        deviceNotificationsConstants.whenByConditionMap[condition]
                    ];

                    if (nvalue) {
                        data.push(nvalue);
                    }

                    if (deviceNotificationsConstants.unitByTypeMap[type]) {
                        data.push(deviceNotificationsConstants.unitByTypeMap[type]);
                    }

                    return data.join('&nbsp;');
                }
            }

            function priorityRenderer(value) {
                return deviceNotificationsConstants.priorities[value + 2];
            }

            function activeSystemsRenderer(value) {
                return value.length > 0 ? value : $.t('All');
            }

            function customMessageRenderer(value) {
                var parts = value.split(';;');
                return parts.length > 0 ? parts[0] : value;
            }

            function sendAlwaysRenderer(value) {
                return $.t(value === true ? 'Yes' : 'No')
            }

            function recoveryRenderer(value) {
                var parts = value.split(';');
                return $.t(parseInt(parts[3]) ? 'Yes' : 'No');
            }
        }
    });

    app.component('deviceNotificationForm', {
        templateUrl: 'app/notifications/notificationForm.html',
        bindings: {
            device: '<',
            notificationTypes: '<',
            notifiers: '<',
        },
        require: {
            ngModelCtrl: 'ngModel'
        },
        controller: function () {
            var vm = this;

            vm.$onInit = init;
            vm.isAdditionalOptionsAvailable = isAdditionalOptionsAvailable;
            vm.getValueUnit = getValueUnit;
            vm.updateViewValue = updateViewValue;

            function init() {
                vm.conditionOptions = deviceNotificationsConstants.whenByConditionMap;
                vm.priorityOptions = deviceNotificationsConstants.priorities;

                if (vm.device.isSelector()) {
                    vm.levelOptions = vm.device.getSelectorLevelOptions();
                } else if (vm.device.isDimmer()) {
                    vm.levelOptions = vm.device.getDimmerLevelOptions(5)
                }

                vm.ngModelCtrl.$render = function () {
                    var notification = vm.ngModelCtrl.$modelValue;
                    var activeSystems = notification.ActiveSystems
                        ? notification.ActiveSystems.split(';')
                        : [];

                    var params = notification.Params.split(';');

                    vm.notification = {
                        type: params[0],
                        condition: params[1],
                        level: parseInt(params[2]),
                        priority: notification.Priority,
                        customMessage: notification.CustomMessage,
                        ignoreInterval: notification.SendAlways,
                        sendRecovery: params[3] ? parseInt(params[3]) === 1 : false,
                        systems: vm.notifiers.reduce(function (acc, item) {
                            acc[item.name] = activeSystems.length === 0 || activeSystems.includes(item.name)
                            return acc;
                        }, {})
                    };
                };
            }

            function updateViewValue() {
                var activeSystems = Object
                    .keys(vm.notification.systems)
                    .filter(function (key) {
                        return vm.notification.systems[key] === true;
                    });

                var params = [vm.notification.type];

                if (vm.isAdditionalOptionsAvailable()) {
                    params.push(vm.notification.condition || '=');
                    params.push(vm.notification.level);
                    params.push(vm.notification.sendRecovery ? '1' : '0');
                } else if (vm.notification.type === 'S' && vm.notification.level) {
                    params.push(vm.notification.condition || '=');
                    params.push(vm.notification.level);
                }

                var value = Object.assign({}, vm.ngModelCtrl.$modelValue, {
                    Params: params.join(';'),
                    Priority: vm.notification.priority,
                    CustomMessage: vm.notification.customMessage,
                    SendAlways: vm.notification.ignoreInterval,
                    ActiveSystems: activeSystems.length === vm.notifiers.length
                        ? ''
                        : activeSystems.join(';')
                });

                vm.ngModelCtrl.$setViewValue(value);
            }

            function isAdditionalOptionsAvailable() {
                return deviceNotificationsConstants.whenByTypeMap[vm.notification.type] === undefined;
            }

            function getValueUnit() {
                return deviceNotificationsConstants.unitByTypeMap[vm.notification.type] || '';
            }
        }
    });

    app.controller('DeviceNotificationsController', function ($routeParams, $q, domoticzApi, deviceApi, deviceNotificationsApi, permissions, utils) {
        var vm = this;

        var deleteConfirmationMessage = $.t('Are you sure to delete this notification?\n\nThis action can not be undone...');
        var clearConfirmationMessage = $.t('Are you sure to delete ALL notifications?\n\nThis action can not be undone!!');

        vm.selectNotification = selectNotification;
        vm.addNotification = addNotification;
        vm.updateNotification = updateNotification;
        vm.deleteNotification = utils.confirmDecorator(deleteNotification, deleteConfirmationMessage);
        vm.clearNotifications = utils.confirmDecorator(clearNotifications, clearConfirmationMessage);

        init();

        function init() {
            vm.notification = {
                Priority: 0,
                CustomMessage: '',
                SendAlways: false,
                ActiveSystems: ''
            };

            vm.deviceIdx = $routeParams.id;

            var deviceRequest = deviceApi.getDeviceInfo(vm.deviceIdx).then(function (device) {
                vm.pageName = device.Name;
                vm.device = device;
            });

            var notificationTypesRequest = deviceNotificationsApi.getNotificationTypes(vm.deviceIdx).then(function (notificationTypes) {
                vm.notificationTypes = notificationTypes;
                vm.notification.Params = vm.notificationTypes[0].ptag;
            });

            var notificationsRequest = refreshNotifications();

            $q.all([deviceRequest, notificationsRequest, notificationTypesRequest]).then(function () {
                vm.isLoaded = true
            });
        }

        function addNotification() {
            return deviceNotificationsApi
                .addNotification(vm.deviceIdx, getNotificationData(vm.notification))
                .catch(function () {
                    ShowNotify($.t('Problem adding notification!<br>Duplicate Value?'), 2500, true);
                })
                .then(refreshNotifications);
        }

        function updateNotification() {
            return deviceNotificationsApi
                .updateNotification(vm.deviceIdx, vm.notification.idx, getNotificationData(vm.notification))
                .then(refreshNotifications);
        }

        function deleteNotification() {
            return deviceNotificationsApi
                .deleteNotification(vm.notification.idx)
                .then(refreshNotifications);
        }

        function clearNotifications() {
            return deviceNotificationsApi
                .clearNotifications(vm.deviceIdx)
                .then(refreshNotifications);
        }

        function refreshNotifications() {
            return deviceNotificationsApi.getNotifications(vm.deviceIdx).then(function (response) {
                vm.notifications = response.result || [];
                vm.notifiers = response.notifiers;
            });
        }

        function getNotificationData(notification) {
            var params = notification.Params.split(';');

            var norificationType = vm.notificationTypes.find(function (item) {
                return item.ptag === params[0];
            });

            return {
                ttype: norificationType.val,
                twhen: params[1]
                    ? Object.keys(deviceNotificationsConstants.whenByConditionMap).indexOf(params[1])
                    : 0,
                tvalue: params[2] || 0,
                //tmsg: encodeURIComponent(notification.CustomMessage),
                tmsg: notification.CustomMessage,
                tsystems: notification.ActiveSystems,
                tpriority: notification.Priority,
                tsendalways: notification.SendAlways,
                trecovery: params[3] && parseInt(params[3]) === 1
            }
        }

        function selectNotification(value) {
            if (value) {
                vm.notification = Object.assign({}, value);
            } else {
                vm.notification.idx = undefined
            }
        }
    });
});