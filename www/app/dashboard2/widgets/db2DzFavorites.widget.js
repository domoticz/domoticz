define([
    'app',
    'dashboard2/widgetRegistry.service',
    'dashboard2/dashboard2.module',
    'widgets/dzLightWidget',
    'widgets/dzUtilityWidget'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'dz-favorites',
        label:       'Favorites',
        description: 'All favorite devices, grouped by category (reproduces the current Dashboard tabs)',
        category:    'Devices',
        icon:        'fa-solid fa-star',
        defaultW:    6,
        defaultH:    6,
        minW:        3,
        minH:        4,
        maxW:        12,
        maxH:        20,
        configSchema: [
            {
                key:     'filter',
                type:    'select',
                label:   'Category filter',
                options: [
                    { value: '',        label: 'All categories' },
                    { value: 'lights',  label: 'Switches & Lights only' },
                    { value: 'temp',    label: 'Temperature only' },
                    { value: 'weather', label: 'Weather only' },
                    { value: 'utility', label: 'Utility only' }
                ],
                default: ''
            },
            { key: 'planIdx',   type: 'number',  label: 'Limit to room/plan (ID)', required: false },
            { key: 'showTabs',  type: 'boolean', label: 'Show category tabs',      default: true },
            { key: 'title',     type: 'text',    label: 'Custom Title',            required: false }
        ]
    });

    app.directive('db2DzFavoritesWidget', [function() {
        return {
            restrict:         'EA',
            templateUrl:      'views/dashboard2/widgets/dz-favorites.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', 'db2DeviceClassifier',
                function($scope, $http, $interval, db2DeviceClassifier) {
                var ctrl = this;
                ctrl.categories     = [];
                ctrl.activeCategory = null;
                ctrl.activeDevices  = [];
                ctrl.loading        = false;
                ctrl.error          = null;

                var CATEGORY_DEFS = [
                    {
                        key:  'lights',
                        label: 'Switches',
                        icon:  'images/lightbulb.png',
                        test:  function(d) {
                            return db2DeviceClassifier.getDirective(d) === 'dz-light-widget';
                        }
                    },
                    {
                        key:  'temp',
                        label: 'Temperature',
                        icon:  'images/temperature.png',
                        test:  function(d) {
                            return d.Temp !== undefined || d.Humidity !== undefined;
                        }
                    },
                    {
                        key:  'weather',
                        label: 'Weather',
                        icon:  'images/rain.png',
                        test:  function(d) {
                            return d.Rain !== undefined || d.Barometer !== undefined ||
                                   d.Direction !== undefined || d.UVI !== undefined;
                        }
                    },
                    {
                        key:  'utility',
                        label: 'Utility',
                        icon:  'images/utility.png',
                        test:  function(d) {
                            return db2DeviceClassifier.getDirective(d) === 'dz-utility-widget' &&
                                   d.Temp === undefined && d.Rain === undefined;
                        }
                    }
                ];

                ctrl.isLight = function(d) {
                    return db2DeviceClassifier.getDirective(d) === 'dz-light-widget';
                };

                ctrl.isScene = function(d) {
                    return db2DeviceClassifier.getDirective(d) === 'dz-scene-widget';
                };

                ctrl.showTabs = function() {
                    var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                    var show = !cfg || cfg.showTabs !== false;
                    return show && ctrl.categories.length > 1;
                };

                function load() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    ctrl.loading = true;
                    ctrl.error   = null;

                    var url = 'json.htm?type=command&param=getdevices&filter=all&used=true&favorite=1&order=Name';
                    if (cfg.planIdx) {
                        url += '&plan=' + cfg.planIdx;
                    }

                    $http.get(url)
                        .then(function(resp) {
                            ctrl.loading = false;
                            var all = (resp.data && resp.data.result) || [];

                            var filterKey = cfg.filter || '';
                            var newCategories = [];

                            CATEGORY_DEFS.forEach(function(cat) {
                                if (filterKey && filterKey !== cat.key) { return; }
                                var devices = all.filter(cat.test);
                                if (devices.length) {
                                    newCategories.push(angular.extend({}, cat, { devices: devices }));
                                }
                            });

                            ctrl.categories = newCategories;

                            // Keep the active category if it still exists; otherwise default to first
                            var stillExists = ctrl.categories.some(function(c) {
                                return c.key === ctrl.activeCategory;
                            });
                            if (!stillExists) {
                                ctrl.activeCategory = ctrl.categories.length ? ctrl.categories[0].key : null;
                            }

                            updateActiveDevices();
                        })
                        .catch(function() {
                            ctrl.loading = false;
                            ctrl.error   = 'Failed to load devices';
                        });
                }

                function updateActiveDevices() {
                    var cat = null;
                    for (var i = 0; i < ctrl.categories.length; i++) {
                        if (ctrl.categories[i].key === ctrl.activeCategory) {
                            cat = ctrl.categories[i];
                            break;
                        }
                    }
                    ctrl.activeDevices = cat ? cat.devices : [];
                }

                $scope.$watch('ctrl.activeCategory', updateActiveDevices);

                $scope.$on('device_update', load);
                $scope.$on('scene_update',  load);
                $scope.$on('db2:widget:refresh', load);

                var timer = $interval(load, 60000);
                $scope.$on('$destroy', function() { $interval.cancel(timer); });

                ctrl.$onInit = load;
            }]
        };
    }]);
});
