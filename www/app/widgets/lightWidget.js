/**
 * Light Widget Directive - Example Implementation
 * 
 * This is a proof-of-concept showing how to convert jQuery-based
 * light widgets to Angular directives.
 * 
 * Usage:
 *   <dz-light-widget item="light" config="config"></dz-light-widget>
 */

define(['app'], function (app) {
    'use strict';

    app.directive('dzLightWidget', function() {
        return {
            restrict: 'E',
            scope: {
                item: '=',              // Device data object
                config: '=',            // Global configuration
                ordering: '='           // Allow widget ordering
            },
            templateUrl: 'views/widgets/light_widget.html',
            controller: 'LightWidgetController',
            controllerAs: 'ctrl',
            link: function(scope, element, attrs) {
                // DOM-specific setup if needed
                // Example: Initialize tooltips, drag-drop, etc.
            }
        };
    });

    app.controller('LightWidgetController', ['$scope', '$element', 'deviceApi', 
        function($scope, $element, deviceApi) {
        
        var ctrl = this;
        var item = $scope.item;
        
        // Computed properties
        ctrl.getStatusText = function() {
            return item.Status || 'Unknown';
        };
        
        ctrl.getBackgroundClass = function() {
            var statusClasses = {
                'On': 'statusOn',
                'Off': 'statusOff',
                'Dimmed': 'statusDimmed'
            };
            return statusClasses[item.Status] || 'statusNormal';
        };
        
        ctrl.getDeviceImage = function() {
            // Logic to determine device image
            if (item.Image) {
                return item.Image;
            }
            // Default based on type
            return 'Light48.png';
        };
        
        ctrl.isDimmer = function() {
            return item.SwitchType === 'Dimmer' || 
                   item.SubType === 'RGBW' || 
                   item.SubType === 'RGB';
        };
        
        ctrl.supportsColor = function() {
            return item.SubType === 'RGBW' || 
                   item.SubType === 'RGB';
        };
        
        // Actions
        ctrl.toggleDevice = function() {
            var newStatus = item.Status === 'On' ? 'Off' : 'On';
            deviceApi.switchDevice(item.idx, newStatus)
                .then(function(response) {
                    // Update will come via device_update event
                    console.log('Device toggled:', item.idx);
                })
                .catch(function(error) {
                    console.error('Error toggling device:', error);
                });
        };
        
        ctrl.dimDevice = function(level) {
            deviceApi.dimDevice(item.idx, level)
                .then(function(response) {
                    console.log('Device dimmed to:', level);
                })
                .catch(function(error) {
                    console.error('Error dimming device:', error);
                });
        };
        
        ctrl.setColor = function(color) {
            deviceApi.setColor(item.idx, color)
                .then(function(response) {
                    console.log('Color set to:', color);
                })
                .catch(function(error) {
                    console.error('Error setting color:', error);
                });
        };
        
        ctrl.editDevice = function() {
            // Open edit dialog
            // This would call existing edit function or show modal
            if (typeof EditLightDevice === 'function') {
                EditLightDevice(
                    item.idx, 
                    item.Name, 
                    item.Description,
                    item.Protected
                );
            }
        };
        
        ctrl.toggleFavorite = function() {
            var newFavorite = item.Favorite ? 0 : 1;
            deviceApi.makeFavorite(item.idx, newFavorite)
                .then(function(response) {
                    item.Favorite = newFavorite;
                })
                .catch(function(error) {
                    console.error('Error toggling favorite:', error);
                });
        };
        
        ctrl.showLog = function() {
            window.location.href = '#/Devices/' + item.idx + '/Log';
        };
        
        // Watch for item changes
        $scope.$watch('item', function(newVal, oldVal) {
            if (newVal !== oldVal) {
                // Item updated via device_update event
                // Angular automatically re-renders
            }
        }, true);
    }]);
});
