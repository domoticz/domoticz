define(['app'], function(app) {
    'use strict';

    /**
     * dashboardDynamicService
     * Handles all communication with the DashboardDynamic backend API.
     * All methods return $q promises resolving to the API result.
     */
    app.factory('dashboardDynamicService', ['$http', '$q', function($http, $q) {

        var baseUrl = 'json.htm?type=command&param=';

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
                params.layout = JSON.stringify(layoutData);
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
            generateId:   generateId
        };
    }]);
});
