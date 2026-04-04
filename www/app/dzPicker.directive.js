define(['app'], function(app) {
    'use strict';

    app.directive('dzPicker', ['$document', '$timeout', function($document, $timeout) {

        var template =
            '<div class="dz-picker" ng-class="{open: isOpen}">' +
                '<button type="button" class="dz-picker-btn" ng-click="toggle($event)" ng-keydown="onBtnKeydown($event)">' +
                    '<span class="dz-picker-btn-label">{{ currentLabel || placeholder }}</span>' +
                    '<i class="fa-solid fa-chevron-down dz-picker-caret"></i>' +
                '</button>' +
                '<div class="dz-picker-dropdown" ng-style="dropdownPos">' +
                    '<input ng-if="isOpen" type="text" class="dz-picker-search"' +
                           ' ng-model="filter.q" placeholder="Type to filter..."' +
                           ' ng-keydown="onSearchKeydown($event)"' +
                           ' ng-change="onSearchChange()"' +
                           ' autocomplete="off">' +
                    '<ul class="dz-picker-list">' +
                        '<li ng-repeat="opt in filteredOptions()"' +
                            ' ng-class="{active: isSelected(opt), highlighted: $index === highlightIdx}"' +
                            ' ng-mouseenter="highlightIdx = $index"' +
                            ' ng-click="select(opt)">' +
                            '<a>{{ opt.label }}</a>' +
                        '</li>' +
                    '</ul>' +
                '</div>' +
            '</div>';

        return {
            restrict:   'E',
            require:    'ngModel',
            scope: {
                dzOptions:  '=',
                placeholder: '@',
                dzRequired:  '<'
            },
            template: template,
            link: function(scope, element, attrs, ngModel) {

                scope.isOpen       = false;
                scope.filter       = { q: '' };
                scope.highlightIdx = -1;
                scope.dropdownPos  = {};
                scope.currentLabel = '';

                var savedValue = null;

                // ── ngModel integration ──────────────────────────────────────

                ngModel.$render = function() {
                    updateLabel();
                };

                ngModel.$validators.required = function(modelValue) {
                    if (!scope.dzRequired) { return true; }
                    return !!modelValue || modelValue === 0;
                };

                scope.$watch('dzRequired', function() {
                    ngModel.$validate();
                });

                // When options load asynchronously, update the displayed label
                scope.$watch('dzOptions', function() {
                    updateLabel();
                }, true);

                function updateLabel() {
                    var val = ngModel.$viewValue;
                    if (val === undefined || val === null || val === '') {
                        scope.currentLabel = '';
                        return;
                    }
                    var opts = scope.dzOptions || [];
                    var found = null;
                    for (var i = 0; i < opts.length; i++) {
                        if (String(opts[i].value) === String(val)) {
                            found = opts[i];
                            break;
                        }
                    }
                    scope.currentLabel = found ? found.label : '';
                }

                // ── Filtered list ────────────────────────────────────────────

                scope.filteredOptions = function() {
                    var opts = scope.dzOptions || [];
                    var q = scope.filter.q;
                    if (!q) { return opts; }
                    q = q.toLowerCase();
                    var result = [];
                    for (var i = 0; i < opts.length; i++) {
                        if (opts[i].label.toLowerCase().indexOf(q) >= 0) {
                            result.push(opts[i]);
                        }
                    }
                    return result;
                };

                scope.isSelected = function(opt) {
                    return String(opt.value) === String(ngModel.$viewValue);
                };

                // ── Open / close ─────────────────────────────────────────────

                function open($event) {
                    if (scope.isOpen) { return; }
                    savedValue = ngModel.$viewValue;
                    scope.filter.q     = '';
                    scope.highlightIdx = -1;

                    var btn  = element[0].querySelector('.dz-picker-btn');
                    var rect = btn.getBoundingClientRect();
                    var dropH = 265;
                    var top   = (rect.bottom + dropH > window.innerHeight)
                                ? (rect.top - dropH)
                                : (rect.bottom + 2);
                    scope.dropdownPos = {
                        top:   top + 'px',
                        left:  rect.left + 'px',
                        width: rect.width + 'px'
                    };

                    scope.isOpen = true;

                    // Pre-highlight currently selected item and scroll it into view
                    $timeout(function() {
                        var searchEl = element[0].querySelector('.dz-picker-search');
                        if (searchEl) { searchEl.focus(); }

                        var opts = scope.filteredOptions();
                        var val  = ngModel.$viewValue;
                        if (val !== undefined && val !== null && val !== '') {
                            for (var i = 0; i < opts.length; i++) {
                                if (String(opts[i].value) === String(val)) {
                                    scope.highlightIdx = i;
                                    break;
                                }
                            }
                        }
                        scrollHighlightedIntoView();
                    }, 30);
                }

                function close(restoreValue) {
                    if (restoreValue && savedValue !== undefined) {
                        ngModel.$setViewValue(savedValue);
                        updateLabel();
                    }
                    scope.isOpen       = false;
                    scope.filter.q     = '';
                    scope.highlightIdx = -1;
                    scope.dropdownPos  = {};
                    savedValue = null;
                }

                function returnFocusToBtn() {
                    var btn = element[0].querySelector('.dz-picker-btn');
                    if (btn) { btn.focus(); }
                }

                // ── Select an item ───────────────────────────────────────────

                scope.select = function(opt) {
                    ngModel.$setViewValue(opt.value);
                    updateLabel();
                    ngModel.$validate();
                    close(false);
                    returnFocusToBtn();
                };

                // ── Toggle (button click) ────────────────────────────────────

                scope.toggle = function($event) {
                    $event.stopPropagation();
                    if (scope.isOpen) {
                        close(true);
                    } else {
                        open($event);
                    }
                };

                // ── Keyboard: trigger button ─────────────────────────────────

                scope.onBtnKeydown = function($event) {
                    var key = $event.key || $event.keyCode;
                    if (key === 'Enter' || key === 13 ||
                        key === ' '     || key === 32 ||
                        key === 'ArrowDown' || key === 40) {
                        $event.preventDefault();
                        if (!scope.isOpen) { open($event); }
                    }
                };

                // ── Keyboard: search input ───────────────────────────────────

                scope.onSearchKeydown = function($event) {
                    var key = $event.key || $event.keyCode;

                    if (key === 'ArrowDown' || key === 40) {
                        $event.preventDefault();
                        var max = scope.filteredOptions().length - 1;
                        if (scope.highlightIdx < max) {
                            scope.highlightIdx++;
                            scrollHighlightedIntoView();
                        }
                        return;
                    }

                    if (key === 'ArrowUp' || key === 38) {
                        $event.preventDefault();
                        if (scope.highlightIdx > -1) {
                            scope.highlightIdx--;
                            scrollHighlightedIntoView();
                        }
                        return;
                    }

                    if (key === 'Enter' || key === 13) {
                        $event.preventDefault();
                        if (scope.highlightIdx >= 0) {
                            var opts = scope.filteredOptions();
                            if (opts[scope.highlightIdx]) {
                                scope.select(opts[scope.highlightIdx]);
                            }
                        }
                        return;
                    }

                    if (key === 'Escape' || key === 27) {
                        $event.preventDefault();
                        close(true);
                        returnFocusToBtn();
                        return;
                    }

                    if (key === 'Tab' || key === 9) {
                        if (scope.highlightIdx >= 0) {
                            var tabOpts = scope.filteredOptions();
                            if (tabOpts[scope.highlightIdx]) {
                                $event.preventDefault();
                                scope.select(tabOpts[scope.highlightIdx]);
                            }
                        } else {
                            close(false);
                        }
                        return;
                    }
                };

                scope.onSearchChange = function() {
                    scope.highlightIdx = -1;
                };

                // ── Scroll helper ────────────────────────────────────────────

                function scrollHighlightedIntoView() {
                    $timeout(function() {
                        var list = element[0].querySelector('.dz-picker-list');
                        if (!list) { return; }
                        var items = list.querySelectorAll('li');
                        if (items[scope.highlightIdx]) {
                            items[scope.highlightIdx].scrollIntoView({ block: 'nearest' });
                        }
                    }, 0);
                }

                // ── Click outside ────────────────────────────────────────────

                function onDocClick(e) {
                    if (!scope.isOpen) { return; }
                    if (!element[0].contains(e.target)) {
                        scope.$apply(function() {
                            close(false);
                        });
                    }
                }

                $document.on('click', onDocClick);

                scope.$on('$destroy', function() {
                    $document.off('click', onDocClick);
                });
            }
        };
    }]);
});
