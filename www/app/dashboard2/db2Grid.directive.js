define([
    'app',
    'dashboard2/dashboard2.module'
], function(app) {
    'use strict';

    /**
     * db2-grid directive
     *
     * Usage:
     *   <div class="grid-stack db2-grid"
     *        db2-grid
     *        grid-data="activeData"
     *        edit-mode="editMode"
     *        on-change="onGridChange(widgets)">
     *   </div>
     *
     * Responsibilities:
     * - Initialize GridStack on the element
     * - Render widget placeholder cells from gridData.widgets
     * - Enable/disable drag+resize based on editMode
     * - Fire onGridChange whenever layout mutates
     * - Expose addWidgetToGrid(widgetDef) and removeWidget(id) on parent scope
     */
    app.directive('db2Grid', ['$timeout', '$compile', function($timeout, $compile) {
        return {
            restrict: 'A',
            scope: {
                gridData: '=',
                editMode: '<',
                onChange: '&'
            },
            link: function(scope, element) {
                var grid = null;

                // ── Init ─────────────────────────────────────────────
                function initGrid() {
                    var options = {
                        column:        scope.gridData.columns   || 12,
                        cellHeight:    (scope.gridData.rowHeight || 60) + 'px',
                        margin:        scope.gridData.margin     || 8,
                        animate:       scope.gridData.animate !== false,
                        draggable:     { handle: '.db2-widget-drag-handle' },
                        resizable:     { handles: 'se' },
                        disableDrag:   !scope.editMode,
                        disableResize: !scope.editMode,
                        float:         true,
                        auto:          false,
                        columnOpts: {
                            breakpoints: [
                                { w: 768,  c: 4 },
                                { w: 1024, c: 8 }
                            ]
                        }
                    };

                    grid = GridStack.init(options, element[0]);

                    // Render initial widgets
                    if (scope.gridData && scope.gridData.widgets) {
                        scope.gridData.widgets.forEach(function(w) {
                            addItemToGrid(w);
                        });
                    }

                    // Listen for grid change events
                    grid.on('change', function() {
                        syncToModel();
                    });
                }

                // ── Item management ──────────────────────────────────
                function addItemToGrid(widget) {
                    var el = createWidgetElement(widget);
                    grid.addWidget(el, {
                        x:    widget.x    || 0,
                        y:    widget.y    || 0,
                        w:    widget.w    || widget.defaultW || 3,
                        h:    widget.h    || widget.defaultH || 2,
                        minW: widget.minW || 2,
                        minH: widget.minH || 2,
                        maxW: widget.maxW || 12,
                        maxH: widget.maxH || 20,
                        id:   widget.id
                    });
                }

                function createWidgetElement(widget) {
                    // Placeholder cell; Feature 05 replaces the inner content with db2-widget-wrapper
                    var html = '<div class="db2-widget-cell" data-widget-id="' + widget.id + '">' +
                               '  <div db2-widget-wrapper' +
                               '       widget-def="getWidgetById(\'' + widget.id + '\')"' +
                               '       edit-mode="editMode"' +
                               '       on-remove="removeWidget(\'' + widget.id + '\')"' +
                               '       on-configure="configureWidget(\'' + widget.id + '\')"' +
                               '       on-clone="cloneWidget(\'' + widget.id + '\')"></div>' +
                               '</div>';
                    var compiled = $compile(html)(scope);
                    return compiled[0];
                }

                // ── Sync grid positions back to model ────────────────
                function syncToModel() {
                    if (!grid) return;
                    var items = grid.save(false); // false = positions only, no content
                    items.forEach(function(item) {
                        var widget = findWidget(item.id);
                        if (widget) {
                            widget.x = item.x;
                            widget.y = item.y;
                            widget.w = item.w;
                            widget.h = item.h;
                        }
                    });
                    scope.$applyAsync(function() {
                        if (scope.onChange) {
                            scope.onChange({ widgets: scope.gridData.widgets });
                        }
                    });
                }

                function findWidget(id) {
                    return scope.gridData && scope.gridData.widgets &&
                           scope.gridData.widgets.find(function(w) { return w.id === id; });
                }

                // Like addItemToGrid but lets GridStack find the nearest free slot
                function addItemToAutoGrid(widget) {
                    var el = createWidgetElement(widget);
                    grid.addWidget(el, {
                        autoPosition: true,
                        w:    widget.w    || widget.defaultW || 3,
                        h:    widget.h    || widget.defaultH || 2,
                        minW: widget.minW || 2,
                        minH: widget.minH || 2,
                        maxW: widget.maxW || 12,
                        maxH: widget.maxH || 20,
                        id:   widget.id
                    });
                }

                // ── Public API exposed to parent scope ───────────────
                scope.$parent.addWidgetToGrid = function(widgetDef) {
                    var widget = angular.copy(widgetDef);
                    widget.id     = widget.id || generateId();
                    // Validate ID is safe for use in HTML attribute strings
                    // (only alphanumeric, hyphens, underscores allowed)
                    if (!widget.id || !/^[a-zA-Z0-9_-]{1,64}$/.test(widget.id)) {
                        console.warn('db2Grid: skipping widget with invalid id:', widget.id);
                        return null;
                    }
                    widget.w      = widget.defaultW || 3;
                    widget.h      = widget.defaultH || 2;
                    widget.config = widget.config || {};
                    scope.gridData.widgets.push(widget);
                    addItemToAutoGrid(widget);
                    syncToModel();
                    // Scroll new widget into view
                    $timeout(function() {
                        var newEl = element[0].querySelector('[data-widget-id="' + widget.id + '"]');
                        if (newEl) { newEl.scrollIntoView({ behavior: 'smooth', block: 'nearest' }); }
                    }, 50);
                };

                scope.$parent.removeWidget = function(id) {
                    var el = element[0].querySelector('[data-widget-id="' + escapeId(id) + '"]');
                    if (el && grid) {
                        // GridStack DOM: .grid-stack-item > .grid-stack-item-content > .db2-widget-cell
                        var gsItem = el.closest('.grid-stack-item') || el.parentElement.parentElement;
                        grid.removeWidget(gsItem);
                    }
                    var idx = scope.gridData.widgets.findIndex(function(w) { return w.id === id; });
                    if (idx !== -1) scope.gridData.widgets.splice(idx, 1);
                };

                scope.$parent.cloneWidget = function(id) {
                    var source = findWidget(id);
                    if (!source) { return; }
                    var clone = angular.copy(source);
                    clone.id = generateId();
                    scope.gridData.widgets.push(clone);
                    addItemToAutoGrid(clone);
                    syncToModel();
                    // Scroll to clone and trigger drag so user can immediately place it
                    $timeout(function() {
                        var newEl = element[0].querySelector('[data-widget-id="' + clone.id + '"]');
                        if (newEl) {
                            newEl.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
                            $timeout(function() { triggerDragOnElement(newEl); }, 150);
                        }
                    }, 50);
                };

                scope.getWidgetById = function(id) {
                    return findWidget(id);
                };

                // Forward action functions onto this scope so compiled widget
                // template expressions (on-remove, on-configure, on-clone) can
                // resolve them — isolated scopes don't inherit from $parent.
                scope.removeWidget = function(id) {
                    scope.$parent.removeWidget(id);
                };

                scope.configureWidget = function(id) {
                    if (scope.$parent.configureWidget) {
                        scope.$parent.configureWidget(id);
                    }
                };

                scope.cloneWidget = function(id) {
                    scope.$parent.cloneWidget(id);
                };

                // ── Edit mode watch ──────────────────────────────────
                scope.$watch('editMode', function(newVal) {
                    if (!grid) return;
                    if (newVal) {
                        grid.enableMove(true);
                        grid.enableResize(true);
                    } else {
                        grid.enableMove(false);
                        grid.enableResize(false);
                    }
                });

                // ── Init after DOM is ready ──────────────────────────
                var unwatch = scope.$watch('gridData', function(newVal) {
                    if (newVal) {
                        unwatch();
                        $timeout(function() { initGrid(); }, 0);
                    }
                });

                // ── Cleanup ──────────────────────────────────────────
                scope.$on('$destroy', function() {
                    if (grid) {
                        grid.destroy(false);
                        grid = null;
                    }
                });

                function getGridBottom() {
                    if (!scope.gridData || !scope.gridData.widgets) { return 0; }
                    return scope.gridData.widgets.reduce(function(max, w) {
                        return Math.max(max, (w.y || 0) + (w.h || 2));
                    }, 0);
                }

                function triggerDragOnElement(el) {
                    // Find the drag handle inside the new widget and simulate a mousedown
                    // so GridStack picks it up and the user can place it by moving the mouse.
                    var handle = el.querySelector('.db2-widget-drag-handle') || el;
                    var rect = el.getBoundingClientRect();
                    var cx = rect.left + rect.width / 2;
                    var cy = rect.top + 10;
                    ['mouseenter', 'mouseover', 'mousedown'].forEach(function(type) {
                        var ev = new MouseEvent(type, {
                            bubbles: true, cancelable: true,
                            clientX: cx, clientY: cy,
                            view: window
                        });
                        handle.dispatchEvent(ev);
                    });
                }

                function generateId() {
                    if (typeof crypto !== 'undefined' && crypto.randomUUID) {
                        return crypto.randomUUID();
                    }
                    return 'w-' + Math.random().toString(36).substr(2, 9);
                }

                function escapeId(id) {
                    if (typeof CSS !== 'undefined' && CSS.escape) {
                        return CSS.escape(id);
                    }
                    return id.replace(/([^\w-])/g, '\\$1');
                }
            }
        };
    }]);
});
