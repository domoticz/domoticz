define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    var LOG_NORM   = 0x0000001;
    var LOG_STATUS = 0x0000002;
    var LOG_ERROR  = 0x0000004;
    var LOG_ALL    = 0xFFFFFFF;

    var _activeWidgets = 0;

    widgetRegistry.register({
        type:        'domoticz-log',
        label:       'System Log',
        description: 'Recent Domoticz system log entries with Normal/Status/Error filter toggles',
        category:    'System',
        icon:        'fa-solid fa-list',
        defaultW:    6,
        defaultH:    4,
        minW:        4,
        minH:        3,
        maxW:        12,
        maxH:        10,
        transparentBackground: true,
        configSchema: [
            {
                key:      'title',
                type:     'text',
                label:    'Title (optional)',
                required: false
            },
            {
                key:     'showBackground',
                type:    'boolean',
                label:   'Show panel background',
                default: true
            },
            {
                key:      'maxEntries',
                type:     'number',
                label:    'Max entries',
                default:  50
            },
            {
                key:      'showNormal',
                type:     'boolean',
                label:    'Show Normal entries',
                default:  true
            },
            {
                key:      'showStatus',
                type:     'boolean',
                label:    'Show Status entries',
                default:  true
            },
            {
                key:      'showError',
                type:     'boolean',
                label:    'Show Error entries',
                default:  true
            }
        ]
    });

    app.directive('ddDomoticzLogWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/domoticz-log.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$q', 'livesocket', function($scope, $http, $q, livesocket) {
                var ctrl = this;

                ctrl.title           = 'System Log';
                ctrl.allEntries      = [];
                ctrl.filteredEntries = [];
                ctrl.counts          = { normal: 0, status: 0, error: 0 };
                ctrl.loading         = false;
                ctrl.loadError       = false;

                var cancelToken  = null;

                function cfg() {
                    return (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                }

                function isShowNormal() {
                    var c = cfg();
                    return c.showNormal !== false;
                }

                function isShowStatus() {
                    var c = cfg();
                    return c.showStatus !== false;
                }

                function isShowError() {
                    var c = cfg();
                    return c.showError !== false;
                }

                function levelLabel(level) {
                    switch (level) {
                        case LOG_STATUS: return 'STATUS';
                        case LOG_ERROR:  return 'ERROR';
                        default:         return 'NORM';
                    }
                }

                function levelClass(level) {
                    switch (level) {
                        case LOG_STATUS: return 'dd-log-entry-status';
                        case LOG_ERROR:  return 'dd-log-entry-error';
                        default:         return 'dd-log-entry-normal';
                    }
                }

                function parseMessage(message) {
                    var match = message.match(/^(\S+\s+\S+)\s+(.*)$/);
                    return {
                        timestamp: match ? match[1] : '',
                        text:      match ? match[2] : message
                    };
                }

                function applyFilter() {
                    ctrl.filteredEntries = ctrl.allEntries.filter(function(entry) {
                        if (entry.level === LOG_NORM   && !isShowNormal()) { return false; }
                        if (entry.level === LOG_STATUS && !isShowStatus()) { return false; }
                        if (entry.level === LOG_ERROR  && !isShowError())  { return false; }
                        return true;
                    });
                }

                function computeCounts() {
                    var n = 0, s = 0, e = 0;
                    for (var i = 0; i < ctrl.allEntries.length; i++) {
                        var lvl = ctrl.allEntries[i].level;
                        if (lvl === LOG_NORM)   { n++; }
                        else if (lvl === LOG_STATUS) { s++; }
                        else if (lvl === LOG_ERROR)  { e++; }
                    }
                    ctrl.counts = { normal: n, status: s, error: e };
                }

                ctrl.load = function() {
                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();

                    ctrl.loading   = true;
                    ctrl.loadError = false;

                    $http.get('json.htm', {
                        params:  { type: 'command', param: 'getlog', lastlogtime: 0, loglevel: LOG_ALL },
                        timeout: cancelToken.promise
                    }).then(function(resp) {
                        ctrl.loading = false;
                        var data = resp.data;
                        if (!data || !data.result) { return; }

                        var maxEntries = parseInt(cfg().maxEntries, 10) || 50;
                        var raw = data.result;

                        // Keep only the last maxEntries items (API returns oldest first)
                        if (raw.length > maxEntries) {
                            raw = raw.slice(raw.length - maxEntries);
                        }

                        ctrl.allEntries = raw.map(function(item) {
                            var parsed = parseMessage(item.message || '');
                            return {
                                level:      item.level,
                                levelLabel: levelLabel(item.level),
                                levelClass: levelClass(item.level),
                                timestamp:  parsed.timestamp,
                                text:       parsed.text
                            };
                        }).reverse(); // newest first

                        computeCounts();
                        applyFilter();
                    }).catch(function(err) {
                        if (err.status === -1) { return; } // cancelled
                        ctrl.loading   = false;
                        ctrl.loadError = true;
                    });
                };

                ctrl.toggleNormal = function() {
                    var c = cfg();
                    c.showNormal = !isShowNormal();
                    applyFilter();
                };

                ctrl.toggleStatus = function() {
                    var c = cfg();
                    c.showStatus = !isShowStatus();
                    applyFilter();
                };

                ctrl.toggleError = function() {
                    var c = cfg();
                    c.showError = !isShowError();
                    applyFilter();
                };

                ctrl.isShowNormal = isShowNormal;
                ctrl.isShowStatus = isShowStatus;
                ctrl.isShowError  = isShowError;

                $scope.$on('log', function(e, entry) {
                    var maxEntries = parseInt(cfg().maxEntries, 10) || 50;
                    ctrl.allEntries.unshift({
                        level:      entry.level,
                        levelLabel: levelLabel(entry.level),
                        levelClass: levelClass(entry.level),
                        timestamp:  parseMessage(entry.message).timestamp,
                        text:       parseMessage(entry.message).text
                    });
                    if (ctrl.allEntries.length > maxEntries) {
                        ctrl.allEntries.length = maxEntries;
                    }
                    computeCounts();
                    applyFilter();
                });

                $scope.$on('dd:widget:refresh', ctrl.load);

                $scope.$on('$destroy', function() {
                    if (cancelToken) { cancelToken.resolve(); cancelToken = null; }
                    if (--_activeWidgets === 0) { livesocket.unsubscribeFrom('log'); }
                });

                ctrl.$onInit = function() {
                    ctrl.title = cfg().title || 'System Log';
                    if (_activeWidgets++ === 0) { livesocket.subscribeTo('log'); }
                    ctrl.load();
                };

                $scope.$watch(
                    function() { return cfg().maxEntries; },
                    function(val, old) { if (val !== old) { ctrl.load(); } }
                );
            }]
        };
    }]);
});
