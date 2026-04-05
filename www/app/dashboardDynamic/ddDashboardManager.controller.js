define([
    'app',
    'dashboardDynamic/dashboardDynamicService'
], function(app) {
    'use strict';

    app.controller('DdDashboardManagerCtrl', [
        '$scope', '$uibModalInstance',
        'layouts', 'kioskSettings', 'onKioskChange', 'standbySettings', 'onStandbyChange',
        function($scope, $uibModalInstance,
                 layouts, kioskSettings, onKioskChange, standbySettings, onStandbyChange) {

        $scope.layouts = angular.copy(layouts);

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

        $scope.dismiss = function() {
            $uibModalInstance.dismiss('cancel');
        };
    }]);
});
