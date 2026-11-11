define(['app'], function(app) {
    'use strict';

    /**
     * dashboardDynamicService
     * Handles all communication with the DashboardDynamic backend API.
     * All methods return $q promises resolving to the API result.
     */
    app.factory('dashboardDynamicService', ['$http', '$q', function($http, $q) {

        var baseUrl = 'json.htm?type=command&param=';

        // Layout version 2 uses a 24 column grid. Widget registry sizes (minW, maxW, defaultW)
        // stay in 12 column units and are scaled by the grid (see ddGrid getDescConstraints).
        var LAYOUT_VERSION = 2;
        var GRID_COLUMNS   = 24;

        /**
         * Bring a layout to the current version, in place. A version 1 layout (12 columns),
         * also one a plugin writes, gets its x and w doubled. Only the loaded copy changes:
         * the stored layout keeps its version until the user saves it again.
         * Returns the layout.
         */
        function migrateLayout(data) {
            if (!data || typeof data !== 'object') {
                return data;
            }
            // Skip broken entries (a plugin may write null) so the dashboard still loads
            data.widgets = Array.isArray(data.widgets)
                ? data.widgets.filter(function(w) { return w && typeof w === 'object'; })
                : [];
            var version = parseInt(data.version, 10) || 1;
            if (version < 2) {
                var factor = GRID_COLUMNS / (parseInt(data.columns, 10) || 12);
                data.widgets.forEach(function(w) {
                    if (w.x !== undefined) { w.x = Math.round((+w.x || 0) * factor); }
                    if (w.w !== undefined) { w.w = Math.max(1, Math.round((+w.w || 0) * factor)); }
                    if (w.minW !== undefined) { w.minW = Math.max(1, Math.round((+w.minW || 0) * factor)); }
                });
                data.columns = GRID_COLUMNS;
                data.version = LAYOUT_VERSION;
            }
            return data;
        }

        /**
         * Return a shallow copy of the widgets array sorted in reading order:
         * top-to-bottom (y ascending), then left-to-right (x ascending).
         * Missing or non-numeric coordinates are treated as 0. The input array
         * is never mutated.
         */
        function sortWidgetsByPosition(widgets) {
            return (widgets || []).slice().sort(function(a, b) {
                var ay = +a.y || 0, by = +b.y || 0;
                if (ay !== by) { return ay - by; }
                return (+a.x || 0) - (+b.x || 0);
            });
        }

        function apiGet(param, extraParams) {
            var url = baseUrl + param;
            if (extraParams) {
                url += '&' + Object.keys(extraParams)
                    .map(function(k) {
                        return encodeURIComponent(k) + '=' + encodeURIComponent(extraParams[k]);
                    })
                    .join('&');
            }
            return $http.get(url).then(function(resp) {
                if (resp.data && resp.data.status === 'OK') {
                    return resp.data;
                }
                return $q.reject((resp.data && resp.data.message) || 'Unknown error');
            });
        }

        function apiPost(param, data) {
            var url = baseUrl + param;
            var body = Object.keys(data)
                .map(function(k) {
                    return encodeURIComponent(k) + '=' + encodeURIComponent(data[k]);
                })
                .join('&');
            return $http({
                method: 'POST',
                url: url,
                data: body,
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' }
            }).then(function(resp) {
                if (resp.data && resp.data.status === 'OK') {
                    return resp.data;
                }
                return $q.reject((resp.data && resp.data.message) || 'Unknown error');
            });
        }

        /** List all dashboard layouts for the current user (metadata only) */
        function listLayouts() {
            return apiGet('getdashboardlayouts').then(function(r) {
                return r.result || [];
            });
        }

        /** Load a full layout (including widget data) by ID */
        function loadLayout(id) {
            return apiGet('getdashboardlayout', { id: id }).then(function(r) {
                var layout = r.result;
                if (typeof layout.layout === 'string') {
                    try {
                        layout.layout = JSON.parse(layout.layout);
                    } catch (e) {
                        layout.layout = { version: 1, widgets: [] };
                    }
                }
                migrateLayout(layout.layout);
                // Migrate legacy type names
                if (layout.layout && Array.isArray(layout.layout.widgets)) {
                    layout.layout.widgets.forEach(function(widget) {
                        if (widget.type === 'db2HtmlWidget') widget.type = 'html-widget';
                    });
                }
                return layout;
            });
        }

        /** Save a layout. layoutMeta: { id, name, isDefault }, layoutData: widget array (or null to skip updating layout) */
        function saveLayout(layoutMeta, layoutData) {
            var params = {
                id:        layoutMeta.id,
                name:      layoutMeta.name,
                isDefault: layoutMeta.isDefault ? 'true' : 'false'
            };
            if (layoutData !== null && layoutData !== undefined) {
                // Persist widgets in reading order so DOM, tab and z-order match
                // the visual layout on next load. Keeps other layout keys intact.
                var serializable = angular.extend({}, layoutData, {
                    widgets: sortWidgetsByPosition(layoutData.widgets)
                });
                params.layout = JSON.stringify(serializable);
            }
            return apiPost('savedashboardlayout', params);
        }

        /** Delete a layout by ID */
        function deleteLayout(id) {
            return apiPost('deletedashboardlayout', { id: id });
        }

        /** Copy a layout, giving it a new name */
        function copyLayout(id, newName) {
            return apiPost('copydashboardlayout', { id: id, newname: newName });
        }

        /**
         * Sort favorited devices into the dashboard's starter sections.
         * A combo Temp/Hum/Baro device lands in both Temperature (its temp/hum card)
         * and Weather (its barometer), matching the classic Favorites layout.
         */
        function categorizeFavorites(devices) {
            var result = { lights: [], temperature: [], weather: [], utility: [] };
            devices.forEach(function(d) {
                // Skip scenes/groups, handled separately via getscenes
                if (d.Type.indexOf('Scene') === 0 || d.Type.indexOf('Group') === 0) { return; }

                if (d.Type.indexOf('Light')        === 0 || d.Type.indexOf('Security') === 0 ||
                    d.Type.indexOf('Blind')        === 0 || d.Type.indexOf('Curtain')  === 0 ||
                    d.Type.indexOf('Color Switch') === 0 || d.Type.indexOf('Chime')    === 0 ||
                    d.Type.indexOf('Thermostat')   === 0 || d.Type.indexOf('Heating')  === 0 ||
                    d.Type.indexOf('ASA')          === 0 || d.Type.indexOf('Fan')      === 0 ||
                    d.SubType === 'Smartwares Mode' || d.SubType === 'Relay' ||
                    (d.SubType && (d.SubType.indexOf('Itho') === 0 || d.SubType.indexOf('Lucci') === 0 ||
                                   d.SubType.indexOf('Westinghouse') === 0 || d.SubType.indexOf('Falmec') === 0)) ||
                    (d.Type.indexOf('Value') === 0 && typeof d.SwitchType !== 'undefined')
                ) {
                    result.lights.push(d);
                } else if (typeof d.Temp !== 'undefined' || typeof d.Humidity !== 'undefined' || typeof d.Chill !== 'undefined') {
                    result.temperature.push(d);
                    if (typeof d.Rain !== 'undefined' || typeof d.Visibility !== 'undefined' ||
                        typeof d.UVI  !== 'undefined' || typeof d.Radiation  !== 'undefined' ||
                        typeof d.Direction !== 'undefined' || typeof d.Barometer !== 'undefined') {
                        result.weather.push(d);
                    }
                } else if (typeof d.Rain !== 'undefined' || typeof d.Visibility !== 'undefined' ||
                           typeof d.UVI  !== 'undefined' || typeof d.Radiation  !== 'undefined' ||
                           typeof d.Direction !== 'undefined' || typeof d.Barometer !== 'undefined') {
                    result.weather.push(d);
                } else if (
                    d.Type === 'Lux'          || d.Type === 'Air Quality'  || d.Type === 'Counter'  ||
                    d.Type === 'Current'      || d.Type === 'Energy'       || d.Type === 'Current/Energy' ||
                    d.Type === 'Power'        || d.Type === 'Gas'          || d.Type === 'Water'    ||
                    d.Type === 'Weight'       || d.Type === 'Usage'        || d.Type === 'Radiator 1' ||
                    d.Type === 'P1 Smart Meter' ||
                    d.SubType === 'kWh'       || d.SubType === 'Percentage' || d.SubType === 'Voltage' ||
                    d.SubType === 'Distance'  || d.SubType === 'Current'   || d.SubType === 'Text'  ||
                    d.SubType === 'Alert'     || d.SubType === 'Pressure'  || d.SubType === 'A/D'   ||
                    d.SubType === 'Thermostat Mode' || d.SubType === 'Thermostat Fan Mode' ||
                    d.SubType === 'Fan'       || d.SubType === 'Smartwares' || d.SubType === 'Waterflow' ||
                    d.SubType === 'Sound Level' || d.SubType === 'Custom Sensor' ||
                    d.SubType === 'Thermostat Clock' || d.SubType === 'Soil Moisture' ||
                    d.SubType === 'Leaf Wetness' ||
                    d.SubType === 'Gas'       || d.SwitchType === 'Gas'    || d.SwitchTypeVal === 1 ||
                    d.SubType === 'Counter Incremental' || d.SubType === 'Managed Counter' ||
                    (d.Type === 'Setpoint' && d.SubType === 'SetPoint')
                ) {
                    result.utility.push(d);
                } else {
                    // Catch-all: any favorited device that doesn't fit a known category
                    // goes to utility so it always appears on the dashboard
                    result.utility.push(d);
                }
            });
            return result;
        }

        /**
         * Build the starter widget layout from the favorited devices (the layout seeded on
         * first run and by "Reset to favorites"). Returns an array of widget descriptors.
         */
        function buildFavoritesWidgets(all) {
            all = Array.isArray(all) ? all : [];
            var cats    = categorizeFavorites(all);
            var scenes  = all.filter(function(d) {
                return d.Type.indexOf('Scene') === 0 || d.Type.indexOf('Group') === 0;
            });
            var widgets = [];
            var y       = 0;
            var GRID_W  = GRID_COLUMNS;
            var COLS    = 4, W = GRID_W / COLS, H = 2, HEADER_H = 1;

            // resolveType (optional) lets a section emit a different widget type per item,
            // e.g. a Barometer device gets the dedicated 'baro' widget instead of the generic card.
            function addSection(label, items, widgetType, configKey, resolveType) {
                widgetType = widgetType || 'dz-device';
                configKey  = configKey  || 'deviceIdx';
                if (!items.length) { return; }
                widgets.push({
                    id: generateId(), type: 'text-note',
                    x: 0, y: y, w: GRID_W, h: HEADER_H, minH: 1,
                    config: { content: label, fontSize: 16, textAlign: 'left' }
                });
                y += HEADER_H;
                items.forEach(function(item, i) {
                    var type = widgetType, key = configKey;
                    if (resolveType) {
                        var r = resolveType(item);
                        if (r) { type = r.type || type; key = r.configKey || key; }
                    }
                    var cfg = {};
                    cfg[key] = String(item.idx);
                    widgets.push({
                        id: generateId(), type: type,
                        x: (i % COLS) * W, y: y + Math.floor(i / COLS) * H,
                        w: W, h: H, config: cfg
                    });
                });
                y += Math.ceil(items.length / COLS) * H;
            }

            // A device whose only weather reading is a barometer (a Temp/Hum/Baro combo, or a
            // standalone barometer) gets the dedicated 'baro' widget: pressure, forecast and a link
            // to the barometer trend log. A multi-sensor weather device (rain, wind, UV, ...) keeps
            // the generic device card so none of its other readings are dropped.
            function weatherWidgetFor(d) {
                var hasOtherWeather = typeof d.Rain !== 'undefined' || typeof d.Visibility !== 'undefined' ||
                                      typeof d.UVI  !== 'undefined' || typeof d.Radiation  !== 'undefined' ||
                                      typeof d.Direction !== 'undefined';
                if (typeof d.Barometer !== 'undefined' && !hasOtherWeather) {
                    return { type: 'baro', configKey: 'deviceIdx' };
                }
                return null;
            }

            addSection('Scenes:',          scenes,  'dz-scene', 'sceneIdx');
            addSection('Lights/Switches:', cats.lights);
            addSection('Temperature:',     cats.temperature);
            addSection('Weather:',         cats.weather, 'dz-device', 'deviceIdx', weatherWidgetFor);
            addSection('Utility:',         cats.utility);

            return widgets;
        }

        /** Generate a UUID (browser-native or polyfill) */
        function generateId() {
            if (typeof crypto !== 'undefined' && crypto.randomUUID) {
                return crypto.randomUUID();
            }
            return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
                var r = Math.random() * 16 | 0;
                var v = c === 'x' ? r : (r & 0x3 | 0x8);
                return v.toString(16);
            });
        }

        return {
            listLayouts:  listLayouts,
            loadLayout:   loadLayout,
            saveLayout:   saveLayout,
            deleteLayout: deleteLayout,
            copyLayout:   copyLayout,
            generateId:   generateId,
            migrateLayout: migrateLayout,
            buildFavoritesWidgets: buildFavoritesWidgets,
            LAYOUT_VERSION: LAYOUT_VERSION,
            GRID_COLUMNS:  GRID_COLUMNS
        };
    }]);
});
