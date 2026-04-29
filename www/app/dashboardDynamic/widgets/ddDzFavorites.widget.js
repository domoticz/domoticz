define([
    'app',
    'dashboardDynamic/widgetRegistry.service',
    'dashboardDynamic/dashboardDynamic.module',
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
                    { value: 'utility', label: 'Utility only' },
                    { value: 'scenes', label: 'Scenes only' }
                ],
                default: ''
            },
            { key: 'planIdx',   type: 'number',  label: 'Limit to room/plan (ID)', required: false },
            { key: 'showTabs',  type: 'boolean', label: 'Show category tabs',      default: true },
            { key: 'title',     type: 'text',    label: 'Custom Title',            required: false }
        ]
    });

    app.directive('ddDzFavoritesWidget', [function() {
        return {
            restrict:         'EA',
            templateUrl:      'views/dashboardDynamic/widgets/dz-favorites.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$q', 'ddDeviceClassifier',
                function($scope, $http, $q, ddDeviceClassifier) {
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
                            return ddDeviceClassifier.getDirective(d) === 'dz-light-widget';
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
                            return ddDeviceClassifier.getDirective(d) === 'dz-utility-widget' &&
                                   d.Temp === undefined && d.Rain === undefined;
                        }
                    }
                ];

                ctrl.isLight = function(d) {
                    return ddDeviceClassifier.getDirective(d) === 'dz-light-widget';
                };

                ctrl.isScene = function(d) {
                    return d && (String(d.Type).indexOf('Scene') === 0 || String(d.Type).indexOf('Group') === 0);
                };

                ctrl.isGroup = function(d) {
                    return d && String(d.Type).indexOf('Group') === 0;
                };

                ctrl.isBaro = function(d) {
                    return ctrl.activeCategory === 'weather' && d.Barometer !== undefined;
                };

                ctrl.activateScene = function(scene, cmd) {
                    $http.get('json.htm', {
                        params: { type: 'command', param: 'switchscene',
                                  idx: scene.idx, switchcmd: cmd || 'On' }
                    }).then(function(resp) {
                        if (resp.data && resp.data.status === 'OK') { load(); }
                    });
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

                    var filterKey = cfg.filter || '';

                    var deviceUrl = 'json.htm?type=command&param=getdevices&filter=all&used=true&favorite=1&order=Name';
                    if (cfg.planIdx) {
                        deviceUrl += '&plan=' + cfg.planIdx;
                    }

                    var deviceReq = (filterKey !== 'scenes')
                        ? $http.get(deviceUrl)
                        : $q.resolve({ data: {} });

                    var sceneReq = (filterKey === '' || filterKey === 'scenes')
                        ? $http.get('json.htm?type=command&param=getscenes&order=Name')
                        : $q.resolve({ data: {} });

                    $q.all([deviceReq, sceneReq])
                        .then(function(results) {
                            ctrl.loading = false;
                            var allDevices = (results[0].data && results[0].data.result) || [];
                            var allScenes  = (results[1].data && results[1].data.result) || [];
                            var favScenes  = allScenes.filter(function(s) { return s.Favorite == 1; });

                            var newCategories = [];

                            CATEGORY_DEFS.forEach(function(cat) {
                                if (filterKey && filterKey !== cat.key) { return; }
                                var devices = allDevices.filter(cat.test);
                                if (devices.length) {
                                    newCategories.push(angular.extend({}, cat, { devices: devices }));
                                }
                            });

                            if (filterKey === '' || filterKey === 'scenes') {
                                if (favScenes.length) {
                                    newCategories.push({
                                        key:     'scenes',
                                        label:   'Scenes',
                                        icon:    'images/scenes.png',
                                        devices: favScenes
                                    });
                                }
                            }

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

                $scope.$on('device_update', function(e, updated) {
                    var idx = String(updated.idx);
                    for (var i = 0; i < ctrl.categories.length; i++) {
                        var devices = ctrl.categories[i].devices;
                        for (var j = 0; j < devices.length; j++) {
                            if (String(devices[j].idx) === idx) {
                                // Check device still belongs in its current category
                                var catDef = null;
                                for (var k = 0; k < CATEGORY_DEFS.length; k++) {
                                    if (CATEGORY_DEFS[k].key === ctrl.categories[i].key) { catDef = CATEGORY_DEFS[k]; break; }
                                }
                                if (catDef && !catDef.test(updated)) {
                                    load();
                                    return;
                                }
                                devices[j] = updated;
                                updateActiveDevices();
                                return;
                            }
                        }
                    }
                    load();
                });
                $scope.$on('scene_update',  load);
                $scope.$on('dd:widget:refresh', load);

                $scope.$on('$destroy', function() {});

                ctrl.$onInit = load;
            }]
        };
    }]);
});
