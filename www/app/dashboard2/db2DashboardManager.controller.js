define([
    'app',
    'dashboard2/dashboard2Service'
], function(app) {
    'use strict';

    /**
     * Db2DashboardManagerCtrl
     * Used with $uibModal.
     * Resolved: layouts (array), currentId (string), onSwitch (function)
     */
    app.controller('Db2DashboardManagerCtrl', [
        '$scope', '$uibModalInstance', '$timeout',
        'dashboard2Service', 'layouts', 'currentId', 'onSwitch',
        function($scope, $uibModalInstance, $timeout,
                 dashboard2Service, layouts, currentId, onSwitch) {

        $scope.layouts   = angular.copy(layouts);
        $scope.newName   = '';
        $scope.busy      = false;

        // ── Open a layout ──────────────────────────────────────
        $scope.openLayout = function(layout) {
            onSwitch(layout.id);
            $uibModalInstance.close(layout.id);
        };

        // ── Set as default ─────────────────────────────────────
        $scope.setDefault = function(layout) {
            $scope.layouts.forEach(function(l) { l.isDefault = false; });
            layout.isDefault = true;
            dashboard2Service.saveLayout(
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
            dashboard2Service.saveLayout(
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
            dashboard2Service.copyLayout(layout.id, copyName)
                .then(function() {
                    return dashboard2Service.listLayouts();
                })
                .then(function(updated) {
                    $scope.layouts = updated;
                })
                .catch(function(err) {
                    alert('Failed to copy dashboard: ' + err);
                });
        };

        // ── Delete ─────────────────────────────────────────────
        $scope.deleteLayout = function(layout) {
            if ($scope.layouts.length <= 1) { return; }
            if (!confirm('Delete "' + layout.name + '"? This cannot be undone.')) { return; }

            dashboard2Service.deleteLayout(layout.id).then(function() {
                var idx = $scope.layouts.indexOf(layout);
                if (idx !== -1) { $scope.layouts.splice(idx, 1); }
                // If the deleted layout was the current one, switch to the first remaining
                if (layout.id === currentId && $scope.layouts.length > 0) {
                    onSwitch($scope.layouts[0].id);
                }
            }).catch(function(err) {
                alert('Failed to delete dashboard: ' + err);
            });
        };

        // ── Create new ─────────────────────────────────────────
        $scope.createNew = function() {
            var name = ($scope.newName || '').trim();
            if (!name) { return; }

            var id              = dashboard2Service.generateId();
            var isFirst         = $scope.layouts.length === 0;
            var emptyData       = {
                version: 1, columns: 12, rowHeight: 60,
                margin: 8, animate: true, widgets: []
            };

            $scope.busy = true;
            dashboard2Service.saveLayout(
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
                alert('Failed to create dashboard: ' + err);
            });
        };

        // ── Reset current layout to starter ───────────────────
        $scope.resetCurrentToDefault = function() {
            if (!confirm('Reset current dashboard to the starter layout? All current widgets will be removed.')) {
                return;
            }

            var layout = null;
            for (var i = 0; i < $scope.layouts.length; i++) {
                if ($scope.layouts[i].id === currentId) { layout = $scope.layouts[i]; break; }
            }
            if (!layout) { return; }

            var starterData = {
                version: 1, columns: 12, rowHeight: 60,
                margin: 8, animate: true,
                widgets: [
                    { id: dashboard2Service.generateId(), type: 'clock',
                      x: 0, y: 0, w: 2, h: 2, minW: 2, minH: 1, config: {} },
                    { id: dashboard2Service.generateId(), type: 'sun-info',
                      x: 2, y: 0, w: 2, h: 2, minW: 2, minH: 2, config: {} },
                    { id: dashboard2Service.generateId(), type: 'weather',
                      x: 4, y: 0, w: 4, h: 2, minW: 2, minH: 2, config: {} },
                    { id: dashboard2Service.generateId(), type: 'activity-log',
                      x: 0, y: 2, w: 6, h: 4, minW: 2, minH: 3, config: { maxItems: 15 } },
                    { id: dashboard2Service.generateId(), type: 'system-status',
                      x: 6, y: 2, w: 3, h: 2, minW: 2, minH: 2, config: {} }
                ]
            };

            $scope.busy = true;
            dashboard2Service.saveLayout(
                { id: currentId, name: layout.name, isDefault: layout.isDefault },
                starterData
            ).then(function() {
                $scope.busy = false;
                onSwitch(currentId);
                $uibModalInstance.close(currentId);
            }).catch(function(err) {
                $scope.busy = false;
                alert('Failed to reset dashboard: ' + err);
            });
        };

        $scope.dismiss = function() {
            $uibModalInstance.dismiss('cancel');
        };
    }]);
});
