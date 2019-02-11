define(['angularAMD', 'angular', 'angular-route'], function (angularAMD) {
    var module = angular.module('app.routes', ['ngRoute']);

    module.config(function ($routeProvider, $locationProvider) {
        $locationProvider.hashPrefix('');

        $routeProvider
            .when('/Dashboard', angularAMD.route({
                templateUrl: 'views/dashboard.html',
                controller: 'DashboardController'
            }))
            .when('/Devices', angularAMD.route({
                templateUrl: 'app/devices/Devices.html',
                controller: 'DevicesController',
                controllerUrl: 'app/devices/Devices.js',
                controllerAs: '$ctrl'
            }))
            .when('/Devices/:id/Timers', angularAMD.route({
                templateUrl: 'views/timers.html',
                controller: 'DeviceTimersController',
                controllerUrl: 'app/timers/DeviceTimersController.js',
                controllerAs: 'vm'
            }))
            .when('/Devices/:id/Notifications', angularAMD.route({
                templateUrl: 'views/notifications.html',
                controller: 'DeviceNotificationsController',
                controllerUrl: 'app/notifications/DeviceNotifications.js',
                controllerAs: 'vm'
            }))
            .when('/Devices/:id/LightEdit', angularAMD.route({
                templateUrl: 'views/device_light_edit.html',
                controller: 'DeviceLightEditController',
                controllerUrl: 'app/DeviceLightEdit.js',
                controllerAs: 'vm'
            }))
            .when('/Devices/:id/Log', angularAMD.route({
                templateUrl: 'app/log/DeviceLog.html',
                controller: 'DeviceLogController',
                controllerUrl: 'app/log/DeviceLog.js',
                controllerAs: 'vm'
            }))
            .when('/Devices/:id/Report/:year?/:month?', angularAMD.route({
                templateUrl: 'app/report/DeviceReport.html',
                controller: 'DeviceReportController',
                controllerUrl: 'app/report/DeviceReport.js',
                controllerAs: 'vm'
            }))
            .when('/DPFibaro', angularAMD.route({
                templateUrl: 'views/dpfibaro.html',
                controller: 'DPFibaroController',
                permission: 'Admin'
            }))
            .when('/DPHttp', angularAMD.route({
                templateUrl: 'views/dphttp.html',
                controller: 'DPHttpController',
                permission: 'Admin'
            }))
            .when('/DPInflux', angularAMD.route({
                templateUrl: 'views/dpinflux.html',
                controller: 'DPInfluxController',
                permission: 'Admin'
            }))
            .when('/DPGooglePubSub', angularAMD.route({
                templateUrl: 'views/dpgooglepubsub.html',
                controller: 'DPGooglePubSubController',
                permission: 'Admin'
            }))
            .when('/Events', angularAMD.route({
                templateUrl: 'app/events/Events.html',
                controller: 'EventsController',
                controllerUrl: 'app/events/Events.js',
                controllerAs: '$ctrl',
                permission: 'Admin'
            }))
            .when('/Floorplans', angularAMD.route({
                templateUrl: 'views/floorplans.html',
                controller: 'FloorplanController'
            }))
            .when('/Floorplanedit', angularAMD.route({
                templateUrl: 'views/floorplanedit.html',
                controller: 'FloorplanEditController',
                permission: 'Admin'
            }))
            .when('/Forecast', angularAMD.route({
                templateUrl: 'views/forecast.html',
                controller: 'ForecastController'
            }))
            .when('/Frontpage', angularAMD.route({
                templateUrl: 'views/frontpage.html',
                controller: 'FrontpageController'
            }))
            .when('/Hardware', angularAMD.route({
                templateUrl: 'app/hardware/Hardware.html',
                controller: 'HardwareController',
                controllerUrl: 'app/hardware/Hardware.js',
                permission: 'Admin'
            }))
            .when('/Hardware/:id', angularAMD.route({
                templateUrl: 'app/hardware/HardwareSetup.html',
                controller: 'HardwareSetupController',
                controllerUrl: 'app/hardware/HardwareSetup.js',
                controllerAs: '$ctrl',
                permission: 'Admin'
            }))
            .when('/History', angularAMD.route({
                templateUrl: 'views/history.html',
                controller: 'HistoryController'
            }))
            .when('/LightSwitches', angularAMD.route({
                templateUrl: 'views/lights.html',
                controller: 'LightsController',
            }))
            .when('/Lights', angularAMD.route({
                templateUrl: 'views/lights.html',
                controller: 'LightsController'
            }))
            .when('/Log', angularAMD.route({
                templateUrl: 'views/log.html',
                controller: 'LogController',
                permission: 'Admin'
            }))
            .when('/Login', angularAMD.route({
                templateUrl: 'views/login.html',
                controller: 'LoginController'
            }))
            .when('/Logout', angularAMD.route({
                templateUrl: 'views/logout.html',
                controller: 'LogoutController'
            }))
            .when('/Offline', angularAMD.route({
                templateUrl: 'views/offline.html',
                controller: 'OfflineController'
            }))
            .when('/Notification', angularAMD.route({
                templateUrl: 'views/notification.html',
                controller: 'NotificationController',
                permission: 'Admin'
            }))
            .when('/RestoreDatabase', angularAMD.route({
                templateUrl: 'views/restoredatabase.html',
                controller: 'RestoreDatabaseController',
                permission: 'Admin'
            }))
            .when('/RFXComFirmware', angularAMD.route({
                templateUrl: 'views/rfxcomfirmware.html',
                controller: 'RFXComFirmwareController',
                permission: 'Admin'
            }))
            .when('/Cam', angularAMD.route({
                templateUrl: 'views/cam.html',
                controller: 'CamController',
                permission: 'Admin'
            }))
            .when('/CustomIcons', angularAMD.route({
                templateUrl: 'views/customicons.html',
                controller: 'CustomIconsController',
                permission: 'Admin'
            }))
            .when('/Roomplan', angularAMD.route({
                templateUrl: 'views/roomplan.html',
                controller: 'RoomplanController',
                permission: 'Admin'
            }))
            .when('/Timerplan', angularAMD.route({
                templateUrl: 'views/timerplan.html',
                controller: 'TimerplanController',
                permission: 'Admin'
            }))
            .when('/Scenes', angularAMD.route({
                templateUrl: 'views/scenes.html',
                controller: 'ScenesController'
            }))
            .when('/Scenes/:id/Log', angularAMD.route({
                templateUrl: 'app/log/SceneLog.html',
                controller: 'SceneLogController',
                controllerUrl: 'app/log/SceneLog.js',
                controllerAs: 'vm'
            }))
            .when('/Scenes/:id/Timers', angularAMD.route({
                templateUrl: 'views/timers.html',
                controller: 'SceneTimersController',
                controllerUrl: 'app/timers/SceneTimersController.js',
                controllerAs: 'vm'
            }))
            .when('/Setup', angularAMD.route({
                templateUrl: 'views/setup.html',
                controller: 'SetupController',
                permission: 'Admin'
            }))
            .when('/Temperature', angularAMD.route({
                templateUrl: 'views/temperature.html',
                controller: 'TemperatureController',
                controllerAs: 'ctrl'
            }))
            .when('/Temperature/CustomTempLog', angularAMD.route({
                templateUrl: 'views/temperature_custom_temp_log.html',
                controller: 'TemperatureCustomLogController',
                controllerAs: 'ctrl'
            }))
            .when('/Update', angularAMD.route({
                templateUrl: 'views/update.html',
                controller: 'UpdateController',
                permission: 'Admin'
            }))
            .when('/Users', angularAMD.route({
                templateUrl: 'views/users.html',
                controller: 'UsersController',
                permission: 'Admin'
            }))
            .when('/UserVariables', angularAMD.route({
                templateUrl: 'views/uservariables.html',
                controller: 'UserVariablesController',
                permission: 'Admin'
            }))
            .when('/Utility', angularAMD.route({
                templateUrl: 'views/utility.html',
                controller: 'UtilityController'
            }))
            .when('/Weather', angularAMD.route({
                templateUrl: 'views/weather.html',
                controller: 'WeatherController',
                controllerAs: 'ctrl'
            }))
            .when('/ZWaveTopology', angularAMD.route({
                templateUrl: 'zwavetopology.html',
                controller: 'ZWaveTopologyController',
                permission: 'Admin'
            }))
            .when('/Mobile', angularAMD.route({
                templateUrl: 'views/mobile_notifications.html',
                controller: 'MobileNotificationsController',
                permission: 'Admin'
            }))
            .when('/About', angularAMD.route({
                templateUrl: 'views/about.html',
                controller: 'AboutController'
            }))
            .when('/Custom/:custompage', angularAMD.route({
                    templateUrl: function (params) {
                        return 'templates/' + params.custompage + '.html';
                    },
                    controller: 'DummyController'
                })
            )
            .otherwise({
                redirectTo: '/Dashboard'
            });

        // Use html5 mode.
        //$locationProvider.html5Mode(true);
    });

    return module;
});
