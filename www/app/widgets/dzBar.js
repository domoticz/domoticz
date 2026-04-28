define(['app'], function(app) {
    'use strict';

    app.factory('dzBarService', function() {
        var _colorJson = '';
        var _sensorKey = null;
        var _fullColorObj = {};

        function computeBar(numericValue, ranges) {
            if (!Array.isArray(ranges) || ranges.length === 0) { return null; }

            // Allow descending ranges; only reject zero-width
            var valid = ranges.slice().filter(function(r) {
                var f = parseFloat(r.from), t = parseFloat(r.to);
                return !isNaN(f) && !isNaN(t) && t !== f;
            });
            if (valid.length === 0) { return null; }

            // Axis direction from first valid range
            var isRTL = parseFloat(valid[0].from) > parseFloat(valid[0].to);

            // Sort in axis order
            var sorted = valid.sort(function(a, b) {
                return isRTL
                    ? parseFloat(b.from) - parseFloat(a.from)
                    : parseFloat(a.from) - parseFloat(b.from);
            });

            var axisStart = parseFloat(sorted[0].from);
            var axisEnd   = parseFloat(sorted[sorted.length - 1].to);
            var axisMin   = Math.min(axisStart, axisEnd);
            var axisMax   = Math.max(axisStart, axisEnd);
            var span      = axisMax - axisMin;
            if (span === 0) { return null; }

            // Ranges sorted by numeric min (ascending) — used for all gradient building
            // so stop positions are always in left-to-right order within the fill div
            var gradRanges = sorted.slice().sort(function(a, b) {
                return Math.min(parseFloat(a.from), parseFloat(a.to)) - Math.min(parseFloat(b.from), parseFloat(b.to));
            });

            // Helper: build proportional gradient stops for a fill spanning [rangeStart, rangeEnd]
            function buildStops(rangeStart, rangeEnd) {
                var fillWidth = rangeEnd - rangeStart;
                var stops = [], lastColor = null;
                for (var i = 0; i < gradRanges.length; i++) {
                    var rMin  = Math.min(parseFloat(gradRanges[i].from), parseFloat(gradRanges[i].to));
                    var rMax  = Math.max(parseFloat(gradRanges[i].from), parseFloat(gradRanges[i].to));
                    var color = gradRanges[i].color || '#66bb6a';
                    var oStart = Math.max(rMin, rangeStart);
                    var oEnd   = Math.min(rMax, rangeEnd);
                    if (oEnd <= oStart) { continue; }
                    stops.push(color + ' ' + ((oStart - rangeStart) / fillWidth * 100).toFixed(2) + '%');
                    lastColor = color;
                }
                if (lastColor) { stops.push(lastColor + ' 100%'); }
                return stops.length ? 'linear-gradient(to right, ' + stops.join(', ') + ')' : 'none';
            }

            // Center-origin mode: axis spans both negative and positive
            if (axisMin < 0 && axisMax > 0) {
                var center    = (axisMin + axisMax) / 2;
                var centerPct = (center - axisMin) / span * 100;
                var clampedVal = Math.min(axisMax, Math.max(axisMin, numericValue));
                var fillWidth = Math.abs(clampedVal - center);
                if (fillWidth === 0) { return { isCenterOrigin: true, centerPct: centerPct, fillPct: 0, fillDirection: 'left', bgImage: 'none' }; }
                var fillDirection = clampedVal < center ? 'left' : 'right';
                var fillPct = fillWidth / span * 100;
                // Gradient traverses zones between the fill edges (always ascending in value space)
                var fillMin = Math.min(clampedVal, center);
                var fillMax = Math.max(clampedVal, center);
                return { isCenterOrigin: true, centerPct: centerPct, fillPct: fillPct, fillDirection: fillDirection, bgImage: buildStops(fillMin, fillMax) };
            }

            // Standard LTR fill (unchanged bgImage + bgSize approach)
            if (!isRTL) {
                var pct = Math.min(100, Math.max(0, (numericValue - axisMin) / span * 100));
                var currentIdx = sorted.length - 1;
                for (var i = 0; i < sorted.length; i++) {
                    if (numericValue <= parseFloat(sorted[i].to)) { currentIdx = i; break; }
                }
                var stops = [];
                for (var i = 0; i <= currentIdx; i++) {
                    var s = ((parseFloat(sorted[i].from) - axisMin) / span * 100).toFixed(2);
                    stops.push((sorted[i].color || '#66bb6a') + ' ' + s + '%');
                }
                stops.push((sorted[currentIdx].color || '#66bb6a') + ' 100.00%');
                var bgImage = 'linear-gradient(90deg, ' + stops.join(', ') + ')';
                var bgSize = pct > 0 ? (100 / pct * 100).toFixed(2) + '% 100%' : '10000% 100%';
                return { pct: pct, bgImage: bgImage, bgSize: bgSize, isRTL: false };
            }

            // Standard RTL fill (anchored at right edge; gradient proportional to fill width)
            var clampedVal = Math.min(axisMax, Math.max(axisMin, numericValue));
            var pct = Math.abs(clampedVal - axisStart) / span * 100;
            // Fill left edge = lower numeric value, right edge = axisStart (higher numeric value)
            return { pct: pct, bgImage: buildStops(Math.min(clampedVal, axisStart), Math.max(clampedVal, axisStart)), isRTL: true };
        }

        return {
            computeBar: computeBar,
            setColorJson: function(json) { _colorJson = json || ''; },
            getColorJson: function() { return _colorJson; },
            loadForKey: function(deviceColorJson, sensorKey) {
                _sensorKey = sensorKey;
                _fullColorObj = {};
                if (deviceColorJson && typeof deviceColorJson === 'string') {
                    var trimmed = deviceColorJson.trim();
                    if (trimmed.charAt(0) === '{') {
                        try { _fullColorObj = JSON.parse(trimmed); } catch(e) { _fullColorObj = {}; }
                    }
                }
                var ranges = Array.isArray(_fullColorObj[sensorKey]) ? _fullColorObj[sensorKey] : [];
                _colorJson = ranges.length ? JSON.stringify(ranges) : '';
            },
            getFullColorJson: function() {
                var ranges = [];
                if (_colorJson && _colorJson.trim().charAt(0) === '[') {
                    try { ranges = JSON.parse(_colorJson.trim()); } catch(e) {}
                }
                if (_sensorKey) {
                    if (ranges.length) {
                        _fullColorObj[_sensorKey] = ranges;
                    } else {
                        delete _fullColorObj[_sensorKey];
                    }
                }
                var keys = Object.keys(_fullColorObj);
                return keys.length ? JSON.stringify(_fullColorObj) : '';
            },
            attachBarButton: function($form, idx, name) {
                $form.css('position', 'relative');
                $('<a class="btnsmall dz-bar-btn"><i class="fa-solid fa-chart-bar"></i></a>')
                    .css({ position: 'absolute', top: '4px', right: '4px' })
                    .on('click', function() { if (document.activeElement) document.activeElement.blur(); window.dzOpenBarPopup(idx, name); })
                    .appendTo($form);
            }
        };
    });

    app.directive('dzBar', ['dzBarService', function(dzBarService) {
        return {
            restrict: 'E',
            scope: { value: '<', ranges: '<' },
            template: '<div class="dz-uw-bar" ng-if="bar">' +
                        '<div ng-if="!bar.isCenterOrigin && !bar.isRTL" class="dz-uw-bar-fill" ng-style="{\'width\': bar.pct + \'%\', \'backgroundImage\': bar.bgImage, \'backgroundSize\': bar.bgSize}"></div>' +
                        '<div ng-if="!bar.isCenterOrigin && bar.isRTL" class="dz-uw-bar-fill dz-uw-bar-fill--rtl" ng-style="{\'width\': bar.pct + \'%\', \'backgroundImage\': bar.bgImage}"></div>' +
                        '<div ng-if="bar.isCenterOrigin" class="dz-uw-bar-center-wrap">' +
                            '<div ng-if="bar.fillDirection===\'left\'" class="dz-uw-bar-fill dz-uw-bar-fill--left" ng-style="{\'right\': (100 - bar.centerPct) + \'%\', \'width\': bar.fillPct + \'%\', \'backgroundImage\': bar.bgImage}"></div>' +
                            '<div ng-if="bar.fillDirection===\'right\'" class="dz-uw-bar-fill dz-uw-bar-fill--right" ng-style="{\'left\': bar.centerPct + \'%\', \'width\': bar.fillPct + \'%\', \'backgroundImage\': bar.bgImage}"></div>' +
                            '<div class="dz-uw-bar-center-line" ng-style="{\'left\': bar.centerPct + \'%\'}"></div>' +
                        '</div>' +
                      '</div>',
            link: function(scope) {
                function update() {
                    var val = parseFloat(scope.value);
                    scope.bar = (!isNaN(val) && Array.isArray(scope.ranges) && scope.ranges.length)
                        ? dzBarService.computeBar(val, scope.ranges)
                        : null;
                }
                scope.$watch('value', update);
                scope.$watchCollection('ranges', update);
                update();
            }
        };
    }]);

    app.controller('dzBarSettingsCtrl', [
        '$scope', '$uibModalInstance', 'deviceIdx', 'deviceName', 'currentRanges',
        function($scope, $uibModalInstance, deviceIdx, deviceName, currentRanges) {
            $scope.deviceName = deviceName;
            $scope.ranges = currentRanges.map(function(r) {
                return { from: r.from, to: r.to, color: r.color || '#66bb6a' };
            });
            $scope.newRange = { from: '', to: '', color: '#66bb6a' };

            function parseDecimal(v) {
                return parseFloat(String(v).replace(',', '.'));
            }

            $scope.directionError = false;

            $scope.addRange = function() {
                var from = parseDecimal($scope.newRange.from);
                var to   = parseDecimal($scope.newRange.to);
                if (isNaN(from) || isNaN(to) || from === to) { return; }
                $scope.ranges.push({ from: from, to: to, color: $scope.newRange.color || '#66bb6a' });
                $scope.directionError = false;
                var isRTL = $scope.ranges[0] && parseDecimal($scope.ranges[0].from) > parseDecimal($scope.ranges[0].to);
                $scope.ranges.sort(function(a, b) {
                    return isRTL ? parseDecimal(b.from) - parseDecimal(a.from) : parseDecimal(a.from) - parseDecimal(b.from);
                });
                $scope.newRange = { from: '', to: '', color: $scope.newRange.color || '#66bb6a' };
            };

            $scope.removeRange = function(index) {
                $scope.ranges.splice(index, 1);
                $scope.directionError = false;
            };

            $scope.clearRanges = function() {
                $scope.ranges = [];
                $scope.directionError = false;
            };

            $scope.seedDefaults = function() {
                $scope.ranges = [
                    { from: 0,   to: 500,  color: '#66bb6a' },
                    { from: 500, to: 3000, color: '#DF2D3A' }
                ];
            };

            $scope.save = function() {
                $scope.ranges.forEach(function(r) {
                    r.from = parseDecimal(r.from);
                    r.to   = parseDecimal(r.to);
                });
                var hasAsc = false, hasDesc = false;
                $scope.ranges.forEach(function(r) {
                    if (r.from < r.to) { hasAsc  = true; }
                    if (r.from > r.to) { hasDesc = true; }
                });
                if (hasAsc && hasDesc) { $scope.directionError = true; return; }
                $scope.directionError = false;
                $scope.ranges.sort(function(a, b) {
                    return hasDesc ? b.from - a.from : a.from - b.from;
                });
                $uibModalInstance.close($scope.ranges);
            };

            $scope.cancel = function() {
                $uibModalInstance.dismiss('cancel');
            };
        }
    ]);

    if ($.ui && $.ui.dialog && $.ui.dialog.prototype._allowInteraction) {
        var _origAllowInteraction = $.ui.dialog.prototype._allowInteraction;
        $.ui.dialog.prototype._allowInteraction = function(event) {
            if ($(event.target).closest('.modal, .modal-dialog').length) { return true; }
            return _origAllowInteraction.call(this, event);
        };
    }

    window.dzOpenBarPopup = function(idx, name) {
        var injector = angular.element(document.body).injector();
        var $uibModal = injector.get('$uibModal');
        var $timeout  = injector.get('$timeout');
        var barSvc    = injector.get('dzBarService');
        var colorJson = barSvc.getColorJson();
        var currentRanges = [];
        if (colorJson && colorJson.trim().charAt(0) === '[') {
            try { currentRanges = JSON.parse(colorJson); } catch(e) {}
        }
        $timeout(function() {
            $uibModal.open({
                templateUrl: 'views/widgets/bar_settings.html',
                controller:  'dzBarSettingsCtrl',
                resolve: {
                    deviceIdx:     function() { return idx; },
                    deviceName:    function() { return name; },
                    currentRanges: function() { return currentRanges; }
                }
            }).result.then(function(ranges) {
                barSvc.setColorJson(ranges.length ? JSON.stringify(ranges) : '');
            }, angular.noop);
        });
    };

    function parseRangesForKey(colorStr, key) {
        if (!colorStr || typeof colorStr !== 'string') return [];
        var trimmed = colorStr.trim();
        if (trimmed.charAt(0) !== '{') return [];
        try {
            var obj = JSON.parse(trimmed);
            return Array.isArray(obj[key]) ? obj[key] : [];
        } catch(e) { return []; }
    }

    app.directive('dzWeatherBar', [function() {
        return {
            restrict: 'E',
            scope: { device: '<' },
            template: '<dz-bar value="numVal" ranges="ranges"></dz-bar>',
            link: function(scope) {
                function getSensorKey(d) {
                    if (typeof d.Speed !== 'undefined') return 'speed';
                    if (typeof d.Rain !== 'undefined') return 'rain';
                    if (typeof d.Barometer !== 'undefined') return 'baro';
                    if (typeof d.UVI !== 'undefined') return 'uvi';
                    return 'data';
                }
                function update() {
                    var d = scope.device;
                    if (!d) { scope.ranges = []; scope.numVal = undefined; return; }
                    var ranges = parseRangesForKey(d.Color, getSensorKey(d));
                    scope.ranges = ranges;
                    if (!ranges.length) { scope.numVal = undefined; return; }
                    var numVal;
                    if (typeof d.Speed !== 'undefined') numVal = parseFloat(d.Speed);
                    else if (typeof d.Rain !== 'undefined') numVal = parseFloat(d.Rain);
                    else if (typeof d.Barometer !== 'undefined') numVal = parseFloat(d.Barometer);
                    else if (typeof d.UVI !== 'undefined') numVal = parseFloat(d.UVI);
                    else { var m = (d.Data || '').match(/^([\d.,]+)/); numVal = m ? parseFloat(m[1].replace(',', '.')) : NaN; }
                    scope.numVal = !isNaN(numVal) ? numVal : undefined;
                }
                scope.$watch(function() {
                    var d = scope.device;
                    if (!d) return null;
                    var pv = typeof d.Speed !== 'undefined' ? d.Speed :
                             typeof d.Rain !== 'undefined' ? d.Rain :
                             typeof d.Barometer !== 'undefined' ? d.Barometer :
                             typeof d.UVI !== 'undefined' ? d.UVI : d.Data;
                    return (d.Color || '') + '||' + pv;
                }, update);
                update();
            }
        };
    }]);

    app.directive('dzTempBar', [function() {
        return {
            restrict: 'E',
            scope: { device: '<' },
            template: '<dz-bar value="numVal" ranges="ranges"></dz-bar>',
            link: function(scope) {
                function update() {
                    var d = scope.device;
                    if (!d) { scope.ranges = []; scope.numVal = undefined; return; }
                    var key = typeof d.Temp !== 'undefined' ? 'temp' : 'hum';
                    var ranges = parseRangesForKey(d.Color, key);
                    scope.ranges = ranges;
                    if (!ranges.length) { scope.numVal = undefined; return; }
                    var numVal;
                    if (typeof d.Temp !== 'undefined') numVal = parseFloat(d.Temp);
                    else if (typeof d.Humidity !== 'undefined') numVal = parseFloat(d.Humidity);
                    else numVal = NaN;
                    scope.numVal = !isNaN(numVal) ? numVal : undefined;
                }
                scope.$watch(function() {
                    var d = scope.device;
                    if (!d) return null;
                    var pv = typeof d.Temp !== 'undefined' ? d.Temp : d.Humidity;
                    return (d.Color || '') + '||' + pv;
                }, update);
                update();
            }
        };
    }]);
});
