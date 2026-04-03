define([
    'app',
    'dashboardDynamic/dashboardDynamicService'
], function(app) {
    'use strict';

    /**
     * DdDashboardManagerCtrl
     * Used with $uibModal.
     * Resolved: layouts (array), currentId (string), onSwitch (function)
     */
    app.controller('DdDashboardManagerCtrl', [
        '$scope', '$uibModalInstance', '$timeout',
        'dashboardDynamicService', 'layouts', 'currentId', 'onSwitch',
        'kioskSettings', 'onKioskChange', 'standbySettings', 'onStandbyChange', 'bootbox',
        function($scope, $uibModalInstance, $timeout,
                 dashboardDynamicService, layouts, currentId, onSwitch,
                 kioskSettings, onKioskChange, standbySettings, onStandbyChange, bootbox) {

        $scope.layouts   = angular.copy(layouts);
        $scope.newName   = '';
        $scope.busy      = false;

        // ── Kiosk settings ─────────────────────────────────────
        var _kioskDefaults = { enabled: false, layoutIds: [], interval: 30, loop: true };
        $scope.kiosk = angular.extend({}, _kioskDefaults, kioskSettings || {});

        $scope.saveKioskSettings = function(patch) {
            angular.extend($scope.kiosk, patch);
            onKioskChange($scope.kiosk);
        };

        $scope.toggleKioskLayout = function(id) {
            var idx = $scope.kiosk.layoutIds.indexOf(id);
            if (idx === -1) {
                $scope.kiosk.layoutIds.push(id);
            } else {
                $scope.kiosk.layoutIds.splice(idx, 1);
            }
            onKioskChange($scope.kiosk);
        };

        // ── Standby settings ────────────────────────────────────
        var _standbyDefaults = { enabled: false, timeout: 5, opacity: 5, blackout: false };
        $scope.standby = angular.extend({}, _standbyDefaults, standbySettings || {});

        $scope.saveStandbySettings = function(patch) {
            angular.extend($scope.standby, patch);
            onStandbyChange($scope.standby);
        };

        // ── Open a layout ──────────────────────────────────────
        $scope.openLayout = function(layout) {
            onSwitch(layout.id);
            $uibModalInstance.close(layout.id);
        };

        // ── Set as default ─────────────────────────────────────
        $scope.setDefault = function(layout) {
            $scope.layouts.forEach(function(l) { l.isDefault = false; });
            layout.isDefault = true;
            dashboardDynamicService.saveLayout(
                { id: layout.id, name: layout.name, isDefault: true },
                null
            ).catch(function(err) {
                console.error('Failed to set default:', err);
            });
        };

        // ── Inline rename ──────────────────────────────────────
        $scope.startRename = function(layout) {
            layout._editing  = true;
            layout._editName = layout.name;
        };

        $scope.finishRename = function(layout) {
            var name = (layout._editName || '').trim();
            if (!name) { $scope.cancelRename(layout); return; }
            layout.name     = name;
            layout._editing = false;
            dashboardDynamicService.saveLayout(
                { id: layout.id, name: name, isDefault: layout.isDefault },
                null
            ).catch(function(err) {
                console.error('Rename failed:', err);
            });
        };

        $scope.cancelRename = function(layout) {
            layout._editing = false;
        };

        // ── Copy ───────────────────────────────────────────────
        $scope.copyLayout = function(layout) {
            var copyName = 'Copy of ' + layout.name;
            dashboardDynamicService.copyLayout(layout.id, copyName)
                .then(function() {
                    return dashboardDynamicService.listLayouts();
                })
                .then(function(updated) {
                    $scope.layouts = updated;
                })
                .catch(function(err) {
                    bootbox.alert('Failed to copy dashboard: ' + err);
                });
        };

        // ── Delete ─────────────────────────────────────────────
        $scope.deleteLayout = function(layout) {
            if ($scope.layouts.length <= 1) { return; }
            bootbox.confirm('Delete "' + layout.name + '"? This cannot be undone.').then(function() {
                dashboardDynamicService.deleteLayout(layout.id).then(function() {
                    var idx = $scope.layouts.indexOf(layout);
                    if (idx !== -1) { $scope.layouts.splice(idx, 1); }
                    // If the deleted layout was the current one, switch to the first remaining
                    if (layout.id === currentId && $scope.layouts.length > 0) {
                        onSwitch($scope.layouts[0].id);
                    }
                }).catch(function(err) {
                    bootbox.alert('Failed to delete dashboard: ' + err);
                });
            }).catch(angular.noop);
        };

        // ── Create new ─────────────────────────────────────────
        $scope.createNew = function() {
            var name = ($scope.newName || '').trim();
            if (!name) { return; }

            var id              = dashboardDynamicService.generateId();
            var isFirst         = $scope.layouts.length === 0;
            var emptyData       = {
                version: 1, columns: 12, rowHeight: 60,
                margin: 8, animate: true, widgets: []
            };

            $scope.busy = true;
            dashboardDynamicService.saveLayout(
                { id: id, name: name, isDefault: isFirst },
                emptyData
            ).then(function() {
                $scope.layouts.push({
                    id:        id,
                    name:      name,
                    isDefault: isFirst,
                    updated:   new Date().toISOString()
                });
                $scope.newName = '';
                $scope.busy    = false;
                onSwitch(id);
                $uibModalInstance.close(id);
            }).catch(function(err) {
                $scope.busy = false;
                bootbox.alert('Failed to create dashboard: ' + err);
            });
        };

        $scope.dismiss = function() {
            $uibModalInstance.dismiss('cancel');
        };
    }]);
});
