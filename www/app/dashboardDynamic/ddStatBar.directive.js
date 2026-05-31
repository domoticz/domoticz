define(['app', 'widgets/dzBar'], function(app) {
    'use strict';

    app.directive('ddStatBar', ['dzBarService', function(dzBarService) {
        return {
            restrict: 'E',
            template: '<div class="dd-stat-bar" ng-if="bar">' +
                        '<div ng-if="!bar.isCenterOrigin && !bar.isRTL" class="dd-stat-bar-fill" ng-style="{\'width\': bar.pct + \'%\', \'backgroundImage\': bar.bgImage, \'backgroundSize\': bar.bgSize}"></div>' +
                        '<div ng-if="!bar.isCenterOrigin && bar.isRTL" class="dd-stat-bar-fill dd-stat-bar-fill--rtl" ng-style="{\'width\': bar.pct + \'%\', \'backgroundImage\': bar.bgImage}"></div>' +
                        '<div ng-if="bar.isCenterOrigin" class="dd-stat-bar-center-wrap">' +
                            '<div ng-if="bar.fillDirection===\'left\'" class="dd-stat-bar-fill dd-stat-bar-fill--left" ng-style="{\'right\': (100 - bar.centerPct) + \'%\', \'width\': bar.fillPct + \'%\', \'backgroundImage\': bar.bgImage}"></div>' +
                            '<div ng-if="bar.fillDirection===\'right\'" class="dd-stat-bar-fill dd-stat-bar-fill--right" ng-style="{\'left\': bar.centerPct + \'%\', \'width\': bar.fillPct + \'%\', \'backgroundImage\': bar.bgImage}"></div>' +
                            '<div class="dd-stat-bar-center-line" ng-style="{\'left\': bar.centerPct + \'%\'}"></div>' +
                        '</div>' +
                      '</div>',
            scope: {
                numVal: '<',
                ranges: '<'
            },
            controller: ['$scope', function($scope) {
                function update() {
                    var val = parseFloat($scope.numVal);
                    $scope.bar = (!isNaN(val) && Array.isArray($scope.ranges) && $scope.ranges.length)
                        ? dzBarService.computeBar(val, $scope.ranges)
                        : null;
                }
                $scope.$watch('numVal', update);
                $scope.$watchCollection('ranges', update);
                update();
            }]
        };
    }]);
});
