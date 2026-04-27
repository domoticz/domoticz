define(['app'], function(app) {
    'use strict';

    function computeBar(numericValue, ranges) {
        if (!Array.isArray(ranges) || ranges.length === 0) { return null; }
        var sorted = ranges.slice().filter(function(r) {
            var f = parseFloat(r.from), t = parseFloat(r.to);
            return !isNaN(f) && !isNaN(t) && t > f;
        }).sort(function(a, b) { return parseFloat(a.from) - parseFloat(b.from); });
        if (sorted.length === 0) { return null; }
        var min = parseFloat(sorted[0].from);
        var max = parseFloat(sorted[sorted.length - 1].to);
        if (isNaN(min) || isNaN(max) || max <= min) { return null; }
        var pct = Math.min(100, Math.max(0, (numericValue - min) / (max - min) * 100));
        var stops = sorted.map(function(r) { return r.color || '#66bb6a'; });
        var gradient = stops.length === 1
            ? stops[0]
            : 'linear-gradient(90deg, ' + stops.join(', ') + ')';
        return { pct: pct, gradient: gradient };
    }

    app.directive('ddStatBar', [function() {
        return {
            restrict: 'E',
            template: '<div class="dd-stat-bar" ng-if="bar"><div class="dd-stat-bar-fill" ng-style="{\'width\': bar.pct + \'%\', \'background\': bar.gradient}"></div></div>',
            scope: {
                numVal: '<',
                ranges: '<'
            },
            controller: ['$scope', function($scope) {
                function update() {
                    var val = parseFloat($scope.numVal);
                    $scope.bar = isNaN(val) ? null : computeBar(val, $scope.ranges);
                }
                $scope.$watch('numVal', update);
                $scope.$watchCollection('ranges', update);
            }]
        };
    }]);
});
