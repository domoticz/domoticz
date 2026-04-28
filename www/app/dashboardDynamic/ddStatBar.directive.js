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
        var span = max - min;
        var pct = Math.min(100, Math.max(0, (numericValue - min) / span * 100));
        var currentIdx = sorted.length - 1;
        for (var i = 0; i < sorted.length; i++) {
            if (numericValue <= parseFloat(sorted[i].to)) { currentIdx = i; break; }
        }
        var stops = [];
        for (var i = 0; i <= currentIdx; i++) {
            var s = ((parseFloat(sorted[i].from) - min) / span * 100).toFixed(2);
            stops.push((sorted[i].color || '#66bb6a') + ' ' + s + '%');
        }
        stops.push((sorted[currentIdx].color || '#66bb6a') + ' 100.00%');
        var bgImage = 'linear-gradient(90deg, ' + stops.join(', ') + ')';
        var bgSize = pct > 0 ? (100 / pct * 100).toFixed(2) + '% 100%' : '10000% 100%';
        return { pct: pct, bgImage: bgImage, bgSize: bgSize };
    }

    app.directive('ddStatBar', [function() {
        return {
            restrict: 'E',
            template: '<div class="dd-stat-bar" ng-if="bar"><div class="dd-stat-bar-fill" ng-style="{\'width\': bar.pct + \'%\', \'backgroundImage\': bar.bgImage, \'backgroundSize\': bar.bgSize}"></div></div>',
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
