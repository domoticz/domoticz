define(['app', 'icons/dzIconService'], function (app) {
    'use strict';

    var FA_NON_GLYPH = [
        'fa', 'fa-brands', 'fa-classic', 'fa-duotone', 'fa-light', 'fa-regular',
        'fa-sharp', 'fa-solid', 'fa-thin', 'fab', 'far', 'fas',
        'fa-fw', 'fa-ul', 'fa-li', 'fa-border', 'fa-pull-left', 'fa-pull-right',
        'fa-spin', 'fa-spin-pulse', 'fa-spin-reverse', 'fa-pulse', 'fa-beat',
        'fa-fade', 'fa-beat-fade', 'fa-bounce', 'fa-shake', 'fa-flip',
        'fa-flip-horizontal', 'fa-flip-vertical', 'fa-flip-both',
        'fa-rotate-90', 'fa-rotate-180', 'fa-rotate-270', 'fa-rotate-by',
        'fa-rotate-reverse', 'fa-stack', 'fa-stack-1x', 'fa-stack-2x',
        'fa-inverse', 'fa-sr-only', 'fa-sr-only-focusable', 'fa-swap-opacity',
        'fa-width-auto', 'fa-layers', 'fa-layers-text', 'fa-layers-counter',
        'fa-2xs', 'fa-xs', 'fa-sm', 'fa-lg', 'fa-xl', 'fa-2xl',
        'fa-1x', 'fa-2x', 'fa-3x', 'fa-4x', 'fa-5x', 'fa-6x', 'fa-7x',
        'fa-8x', 'fa-9x', 'fa-10x'
    ];

    var FA_SKIP = {};
    FA_NON_GLYPH.forEach(function (name) {
        FA_SKIP[name] = true;
    });

    var SAFE_CLASS_RE = /^[A-Za-z0-9 _-]+$/;

    var FA_STYLE = 'fa-solid';
    var MAX_TILES = 300;
    var GRID_COLUMNS = 7;
    var RECENT_LIMIT = 12;
    var RECENT_KEY = 'dz-icon-recents';
    var SEARCH_DELAY = 200;

    // jQuery UI 1.12 modal dialogs drag focus back inside .ui-dialog, which would make the
    // body-level picker impossible to type in.
    if (window.jQuery && $.ui && $.ui.dialog && !$.ui.dialog.prototype._dzPickerAware) {
        $.widget('ui.dialog', $.ui.dialog, {
            _dzPickerAware: true,
            _allowInteraction: function (event) {
                return !!$(event.target).closest('.dz-iconpicker-modal').length || this._super(event);
            }
        });
    }

    function parseIcon(icon) {
        var empty = { provider: '', on: '', off: '' };

        if (!icon) {
            return empty;
        }

        var parsed = icon;
        if (typeof icon === 'string') {
            try {
                parsed = JSON.parse(icon);
            } catch (e) {
                return empty;
            }
        }
        if (!parsed || typeof parsed !== 'object' || typeof parsed.on !== 'string') {
            return empty;
        }

        return {
            provider: typeof parsed.t === 'string' ? parsed.t : 'fa',
            on: cleanClass(parsed.on),
            off: typeof parsed.off === 'string' ? cleanClass(parsed.off) : ''
        };
    }

    function cleanClass(cls) {
        var value = String(cls).replace(/\s+/g, ' ').trim();
        return SAFE_CLASS_RE.test(value) ? value : '';
    }

    function serializeIcon(sel) {
        if (!sel.on) {
            return '';
        }

        var payload = { t: sel.provider || 'fa', on: sel.on };
        if (sel.off && sel.off !== sel.on) {
            payload.off = sel.off;
        }

        return JSON.stringify(payload);
    }

    function readSelection(customImage, icon) {
        var parsed = parseIcon(icon);

        return {
            customImage: parsed.on ? 0 : (parseInt(customImage, 10) || 0),
            on: parsed.on,
            off: parsed.off,
            provider: parsed.provider
        };
    }

    function signature(customImage, icon) {
        return (parseInt(customImage, 10) || 0) + '|' + (icon || '');
    }

    function glyphNameOf(cls) {
        var words = String(cls).split(' ').filter(function (word) {
            return word && !FA_SKIP[word];
        });

        return words.length ? words[words.length - 1] : String(cls);
    }

    function formatCount(count) {
        if (count < 1000) {
            return String(count);
        }

        var thousands = count / 1000;
        return (thousands >= 10 ? Math.round(thousands) : Math.round(thousands * 10) / 10) + 'k';
    }

    app.factory('dzIconPickerData', ['$q', '$timeout', 'domoticzApi', function ($q, $timeout, domoticzApi) {
        var iconSetRequest = null;
        var librariesRequest = null;
        var glyphCache = {};

        return {
            iconSet: iconSet,
            libraries: libraries,
            glyphs: glyphs,
            forgetGlyphs: forgetGlyphs,
            ensureStylesheet: ensureStylesheet,
            recents: recents,
            addRecent: addRecent,
            recentKey: recentKey
        };

        function iconSet() {
            if (!iconSetRequest) {
                iconSetRequest = domoticzApi.sendCommand('custom_light_icons', {})
                    .then(function (data) {
                        return (data.result || [])
                            .filter(function (item) {
                                return !!item;
                            })
                            .map(function (item) {
                                return {
                                    idx: parseInt(item.idx, 10) || 0,
                                    text: item.text || '',
                                    description: item.description || '',
                                    src: 'images/' + item.imageSrc + '48_On.png'
                                };
                            })
                            .filter(function (item) {
                                return item.idx !== 0;
                            });
                    })
                    .catch(function () {
                        return [];
                    });
            }

            return iconSetRequest;
        }

        function libraries() {
            if (!librariesRequest) {
                librariesRequest = domoticzApi.sendCommand('getwebassets', {})
                    .then(function (data) {
                        return (data.result || [])
                            .filter(function (row) {
                                return row && row.name && /\.css$/i.test(row.name);
                            })
                            .map(function (row) {
                                var prefix = row.name.split('.')[0].toLowerCase();
                                return {
                                    prefix: prefix,
                                    css: row.path || ('assets/' + row.name),
                                    // Libraries installed before titles existed have none stored.
                                    title: row.Title || prefix
                                };
                            })
                            .filter(function (row) {
                                return !!row.prefix;
                            });
                    })
                    .catch(function () {
                        return [];
                    });
            }

            return librariesRequest;
        }

        function glyphs(prefix) {
            if (glyphCache[prefix]) {
                return glyphCache[prefix];
            }

            var isFa = (prefix === 'fa');
            var names = {};
            var sheets = document.styleSheets;

            for (var i = 0; i < sheets.length; i++) {
                var rules = null;
                try {
                    rules = sheets[i].cssRules;
                } catch (e) {
                    rules = null;
                }
                if (rules) {
                    collect(rules, prefix, isFa, names);
                }
            }

            glyphCache[prefix] = Object.keys(names).sort();
            return glyphCache[prefix];
        }

        function forgetGlyphs(prefix) {
            delete glyphCache[prefix];
        }

        function collect(rules, prefix, isFa, names) {
            for (var i = 0; i < rules.length; i++) {
                var rule = rules[i];

                if (rule.cssRules && rule.cssRules.length) {
                    collect(rule.cssRules, prefix, isFa, names);
                    continue;
                }
                if (!rule.selectorText || !rule.style) {
                    continue;
                }
                // FA 7 declares every glyph as .fa-name{--fa:"\e00d"}; utility classes do not.
                if (isFa && !readProperty(rule, '--fa')) {
                    continue;
                }

                rule.selectorText.split(',').forEach(function (selector) {
                    var name = glyphClassName(selector, prefix);
                    if (name) {
                        names[name] = true;
                    }
                });
            }
        }

        function readProperty(rule, property) {
            try {
                return rule.style.getPropertyValue(property);
            } catch (e) {
                return '';
            }
        }

        function glyphClassName(selector, prefix) {
            var match = selector.trim()
                .replace(/::?(before|after)$/, '')
                .match(/^\.([A-Za-z0-9_-]+)$/);

            if (!match) {
                return null;
            }

            var name = match[1];
            if (name.indexOf(prefix + '-') !== 0 || FA_SKIP[name]) {
                return null;
            }

            return name;
        }

        function ensureStylesheet(href) {
            if (!href) {
                return $q.resolve(false);
            }

            var links = document.getElementsByTagName('link');
            for (var i = 0; i < links.length; i++) {
                if ((links[i].getAttribute('href') || '') === href) {
                    return $q.resolve(false);
                }
            }

            var link = document.createElement('link');
            link.rel = 'stylesheet';
            link.href = href;
            document.getElementsByTagName('head')[0].appendChild(link);

            return $timeout(angular.noop, 600).then(function () {
                return true;
            });
        }

        function recentKey(entry) {
            return entry.kind === 'img'
                ? 'img-' + entry.idx
                : 'font-' + (entry.provider || 'fa') + '-' + entry.cls;
        }

        function isValidRecent(entry) {
            if (!entry || typeof entry !== 'object') {
                return false;
            }
            if (entry.kind === 'img') {
                return (parseInt(entry.idx, 10) || 0) > 0;
            }

            return entry.kind === 'font' && typeof entry.cls === 'string' && SAFE_CLASS_RE.test(entry.cls);
        }

        function recents() {
            var raw;
            try {
                raw = window.localStorage.getItem(RECENT_KEY);
            } catch (e) {
                return [];
            }

            var list;
            try {
                list = JSON.parse(raw || '[]');
            } catch (e) {
                return [];
            }

            return Array.isArray(list) ? list.filter(isValidRecent).slice(0, RECENT_LIMIT) : [];
        }

        function addRecent(entry) {
            if (!isValidRecent(entry)) {
                return;
            }

            var key = recentKey(entry);
            var list = recents().filter(function (item) {
                return recentKey(item) !== key;
            });
            list.unshift(entry);

            try {
                window.localStorage.setItem(RECENT_KEY, JSON.stringify(list.slice(0, RECENT_LIMIT)));
            } catch (e) {
                return;
            }
        }
    }]);

    // One resolution path for the compact trigger and the modal swatch, so the two can never
    // disagree with each other or with the widget that renders the same device.
    app.factory('dzIconPickerPreview', ['dzIconService', function (dzIconService) {
        return {
            of: of
        };

        function of(device, sel, item, defaultImage) {
            if (sel.on) {
                return { kind: 'font', cls: sel.on };
            }

            var resolved = device ? dzIconService.resolveSelection(device, sel.customImage, '') : null;
            if (resolved && resolved.kind === 'font') {
                return resolved;
            }

            var src = sel.customImage > 0 ? (item ? item.src : '') : defaultImage;

            return src ? { kind: 'img', src: src } : { kind: 'none' };
        }
    }]);

    function swatch(expr, sizeClass, stateClass) {
        return '<span class="dz-ip-swatch ' + sizeClass + ' ' + stateClass + '">' +
            '<i ng-if="' + expr + '.kind === \'font\'" class="{{ ' + expr + '.cls }}"></i>' +
            '<img ng-if="' + expr + '.kind === \'img\'" ng-src="{{ ' + expr + '.src }}" alt="">' +
            '<i ng-if="' + expr + '.kind === \'none\'" class="fa-regular fa-square"></i>' +
            '</span>';
    }

    app.component('dzIconPickerModal', {
        template:
            '<div class="dz-iconpicker-modal" ng-click="$ctrl.onBackdrop($event)" ng-keydown="$ctrl.onKeydown($event)">' +
                '<div class="dz-ip-dialog">' +
                    '<div class="dz-ip-head">' +
                        '<span class="dz-ip-title" data-i18n="Choose an icon">Choose an icon</span>' +
                        '<input type="text" class="dz-ip-search" ng-model="$ctrl.query" ng-change="$ctrl.onQueryChange()"' +
                             ' data-i18n="[placeholder]Search" placeholder="Search" autocomplete="off">' +
                        '<a class="dz-ip-close" ng-click="$ctrl.cancel()" title="{{ $ctrl.labels.close }}">' +
                            '<i class="fa-solid fa-xmark"></i>' +
                        '</a>' +
                    '</div>' +
                    '<div class="dz-ip-body">' +
                        '<div class="dz-ip-rail">' +
                            '<a class="dz-ip-rail-row" ng-repeat="source in $ctrl.sources track by source.id"' +
                               ' ng-class="{\'dz-ip-rail-sel\': $ctrl.active && source.id === $ctrl.active.id,' +
                               ' \'dz-ip-rail-dis\': $ctrl.isSourceDisabled(source)}"' +
                               ' ng-click="$ctrl.selectSource(source)" title="{{ source.title }}">' +
                                '<i class="dz-ip-rail-glyph {{ source.glyph }}"></i>' +
                                '<span class="dz-ip-rail-name">{{ source.title }}</span>' +
                                '<span class="dz-ip-rail-count">{{ $ctrl.countLabel(source) }}</span>' +
                            '</a>' +
                            '<span class="dz-ip-rail-sep" ng-show="$ctrl.allowGlyphs !== false"></span>' +
                            '<a class="dz-ip-rail-row dz-ip-rail-add" ng-show="$ctrl.allowGlyphs !== false"' +
                               ' ng-click="$ctrl.addLibrary()">' +
                                '<i class="dz-ip-rail-glyph fa-solid fa-plus"></i>' +
                                '<span class="dz-ip-rail-name" data-i18n="Add library">Add library</span>' +
                            '</a>' +
                        '</div>' +
                        '<div class="dz-ip-grid-wrap">' +
                            '<div class="dz-ip-grid">' +
                                '<a class="dz-ip-tile" ng-repeat="item in $ctrl.results track by item.key"' +
                                   ' ng-class="{\'dz-ip-tile-sel\': $ctrl.isSelected(item),' +
                                   ' \'dz-ip-tile-hl\': $index === $ctrl.highlight}"' +
                                   ' ng-click="$ctrl.pick(item)" ng-dblclick="$ctrl.confirm()" title="{{ item.title }}">' +
                                    '<span class="dz-ip-tile-icon">' +
                                        '<i ng-if="item.kind === \'font\'" class="{{ item.cls }}"></i>' +
                                        '<img ng-if="item.kind === \'img\'" ng-src="{{ item.src }}" alt="">' +
                                    '</span>' +
                                    '<span class="dz-ip-tile-name">{{ item.name }}</span>' +
                                '</a>' +
                            '</div>' +
                            '<div class="dz-ip-note" ng-show="!$ctrl.results.length" data-i18n="No icons found">No icons found</div>' +
                            '<div class="dz-ip-note" ng-show="$ctrl.truncated" data-i18n="Keep typing to narrow down">Keep typing to narrow down</div>' +
                        '</div>' +
                    '</div>' +
                    '<div class="dz-ip-foot">' +
                        '<div class="dz-ip-foot-top">' +
                            '<span class="dz-ip-foot-label" data-i18n="now">now</span>' +
                            '<span class="dz-ip-slot" ng-class="{\'dz-ip-slot-active\': $ctrl.slot === \'on\'}"' +
                                 ' ng-click="$ctrl.setSlot(\'on\')">' +
                                swatch('$ctrl.candidate', 'dz-ip-swatch-48', 'dz-ip-on') +
                                swatch('$ctrl.candidate', 'dz-ip-swatch-16', 'dz-ip-on') +
                            '</span>' +
                            '<a class="dz-ip-disclose" ng-show="$ctrl.canSetOff()" ng-click="$ctrl.toggleOff()">' +
                                '<i class="fa-solid" ng-class="$ctrl.offOpen ? \'fa-caret-down\' : \'fa-caret-right\'"></i> ' +
                                '<span data-i18n="different icon when off">different icon when off</span>' +
                            '</a>' +
                        '</div>' +
                        '<div class="dz-ip-off" ng-show="$ctrl.offOpen && $ctrl.canSetOff()">' +
                            '<span class="dz-ip-slot" ng-class="{\'dz-ip-slot-active\': $ctrl.slot === \'off\'}"' +
                                 ' ng-click="$ctrl.setSlot(\'off\')">' +
                                swatch('$ctrl.offCandidate', 'dz-ip-swatch-48', 'dz-ip-off') +
                                swatch('$ctrl.offCandidate', 'dz-ip-swatch-16', 'dz-ip-off') +
                            '</span>' +
                            '<span class="dz-ip-off-hint" data-i18n="Pick the off icon from the grid">Pick the off icon from the grid</span>' +
                        '</div>' +
                        '<div class="dz-ip-actions">' +
                            '<a class="btnstyle3 dz-ip-default"' +
                               ' ng-class="{\'btnstyle3-sel\': $ctrl.isDefaultCandidate()}" ng-click="$ctrl.useDefault()">' +
                                '<span data-i18n="Use default">Use default</span>' +
                            '</a>' +
                            '<span class="dz-ip-actions-gap"></span>' +
                            '<a class="btnstyle3" ng-click="$ctrl.cancel()"><span data-i18n="Cancel">Cancel</span></a>' +
                            '<a class="btnstyle3" ng-click="$ctrl.confirm()"><span data-i18n="Select">Select</span></a>' +
                        '</div>' +
                    '</div>' +
                '</div>' +
            '</div>',
        bindings: {
            customImage: '<',
            icon: '<',
            device: '<',
            defaultImage: '<',
            allowGlyphs: '<',
            onSelect: '&',
            onCancel: '&'
        },
        controller: ['$element', '$scope', '$timeout', '$location', 'dzIconPickerData', 'dzIconService', 'dzIconPickerPreview',
            function ($element, $scope, $timeout, $location, dzIconPickerData, dzIconService, dzIconPickerPreview) {
                var vm = this;

                var iconSet = [];
                var iconSetLoaded = false;
                var libraries = [];
                var searchTimer = null;

                vm.$onInit = init;
                vm.$postLink = postLink;
                vm.$onDestroy = destroy;

                vm.selectSource = selectSource;
                vm.isSourceDisabled = isSourceDisabled;
                vm.countLabel = countLabel;
                vm.onQueryChange = onQueryChange;
                vm.pick = pick;
                vm.isSelected = isSelected;
                vm.setSlot = setSlot;
                vm.toggleOff = toggleOff;
                vm.canSetOff = canSetOff;
                vm.isDefaultCandidate = isDefaultCandidate;
                vm.useDefault = useDefault;
                vm.confirm = confirm;
                vm.cancel = cancel;
                vm.addLibrary = addLibrary;
                vm.onKeydown = onKeydown;
                vm.onBackdrop = onBackdrop;

                function init() {
                    vm.labels = { close: $.t('Close') };
                    vm.query = '';
                    vm.results = [];
                    vm.matchCount = 0;
                    vm.truncated = false;
                    vm.highlight = -1;
                    vm.sel = readSelection(vm.customImage, vm.icon);
                    vm.offOpen = !!vm.sel.off;
                    vm.slot = 'on';

                    describe();
                    buildSources();
                    selectSource(initialSource(), true);

                    dzIconPickerData.iconSet().then(function (items) {
                        iconSet = items;
                        iconSetLoaded = true;
                        rebuild();
                    });

                    dzIconPickerData.libraries().then(function (rows) {
                        libraries = rows;
                        rebuild();
                    });

                    dzIconService.preloadBuiltinIcons().then(describe);
                }

                function postLink() {
                    if ($element.i18n) {
                        $element.i18n();
                    }

                    document.addEventListener('keydown', onDocumentKeydown, true);

                    $timeout(function () {
                        $element.find('.dz-ip-search').trigger('focus');
                    });
                }

                function destroy() {
                    document.removeEventListener('keydown', onDocumentKeydown, true);
                    if (searchTimer) {
                        $timeout.cancel(searchTimer);
                        searchTimer = null;
                    }
                }

                function rebuild() {
                    var activeId = vm.active ? vm.active.id : null;
                    buildSources();
                    describe();
                    selectSource(sourceById(activeId) || initialSource(), true);
                }

                function buildSources() {
                    var list = [];
                    var recentItems = recentTiles();

                    list.push({
                        id: 'recent',
                        kind: 'recent',
                        title: $.t('Recent'),
                        glyph: 'fa-solid fa-star',
                        items: recentItems,
                        count: recentItems.length
                    });
                    list.push({
                        id: 'builtin',
                        kind: 'image',
                        uploaded: false,
                        title: 'Domoticz',
                        glyph: 'fa-solid fa-house',
                        count: imageNames(false).length
                    });

                    var uploaded = imageNames(true);
                    if (uploaded.length) {
                        list.push({
                            id: 'custom',
                            kind: 'image',
                            uploaded: true,
                            title: $.t('Custom'),
                            glyph: 'fa-solid fa-upload',
                            count: uploaded.length
                        });
                    }

                    if (vm.allowGlyphs !== false) {
                        list.push({
                            id: 'fa',
                            kind: 'glyph',
                            provider: 'fa',
                            style: FA_STYLE,
                            title: 'Font Awesome',
                            glyph: 'fa-solid fa-font',
                            count: dzIconPickerData.glyphs('fa').length
                        });

                        libraries.forEach(function (row) {
                            list.push({
                                id: 'lib-' + row.prefix,
                                kind: 'glyph',
                                provider: row.prefix,
                                style: row.prefix,
                                css: row.css,
                                title: row.title,
                                glyph: 'fa-solid fa-icons',
                                count: dzIconPickerData.glyphs(row.prefix).length
                            });
                        });
                    }

                    vm.sources = list;
                }

                function sourceById(id) {
                    if (!id) {
                        return null;
                    }

                    var found = vm.sources.filter(function (source) {
                        return source.id === id;
                    });

                    return found.length ? found[0] : null;
                }

                function initialSource() {
                    var wanted = 'builtin';

                    if (vm.sel.on) {
                        wanted = vm.sel.provider === 'fa' ? 'fa' : 'lib-' + vm.sel.provider;
                    } else if (vm.sel.customImage >= 100) {
                        wanted = 'custom';
                    } else if (vm.sel.customImage > 0) {
                        wanted = 'builtin';
                    } else if (dzIconPickerData.recents().length) {
                        wanted = 'recent';
                    }

                    return sourceById(wanted) || vm.sources[0];
                }

                function countLabel(source) {
                    return formatCount(source.count || 0);
                }

                function isSourceDisabled(source) {
                    if (vm.slot !== 'off') {
                        return false;
                    }

                    return source.kind === 'image' || (source.kind === 'glyph' && source.provider !== vm.sel.provider);
                }

                function selectSource(source, keepQuery) {
                    if (!source || isSourceDisabled(source)) {
                        return;
                    }

                    vm.active = source;
                    if (!keepQuery) {
                        vm.query = '';
                    }
                    refresh();
                }

                function onQueryChange() {
                    if (searchTimer) {
                        $timeout.cancel(searchTimer);
                    }
                    searchTimer = $timeout(refresh, SEARCH_DELAY);
                }

                function refresh() {
                    searchTimer = null;

                    var source = vm.active;
                    if (!source) {
                        vm.results = [];
                        vm.matchCount = 0;
                        vm.truncated = false;
                        return;
                    }

                    var query = (vm.query || '').toLowerCase();
                    var matched;

                    if (source.kind === 'recent') {
                        matched = source.items.filter(function (item) {
                            return matchesTile(item, query) && isPickable(item);
                        });
                    } else if (source.kind === 'image') {
                        matched = imageNames(source.uploaded).filter(function (item) {
                            return !query || item.search.indexOf(query) !== -1;
                        }).map(imageTile);
                    } else {
                        matched = dzIconPickerData.glyphs(source.provider).filter(function (name) {
                            return !query || name.toLowerCase().indexOf(query) !== -1;
                        });
                    }

                    vm.matchCount = matched.length;
                    vm.truncated = matched.length > MAX_TILES;

                    var capped = vm.truncated ? matched.slice(0, MAX_TILES) : matched;
                    vm.results = source.kind === 'glyph'
                        ? capped.map(function (name) {
                            return glyphTile(source, name);
                        })
                        : capped;

                    vm.highlight = -1;
                    retryLibrary(source);
                }

                function retryLibrary(source) {
                    if (source.kind !== 'glyph' || source.provider === 'fa' || vm.matchCount || source.retried) {
                        return;
                    }

                    source.retried = true;
                    dzIconPickerData.ensureStylesheet(source.css).then(function (added) {
                        if (added) {
                            dzIconPickerData.forgetGlyphs(source.provider);
                            rebuild();
                        }
                    });
                }

                function matchesTile(item, query) {
                    return !query || (item.name + ' ' + (item.title || '')).toLowerCase().indexOf(query) !== -1;
                }

                function imageNames(uploaded) {
                    return iconSet.filter(function (item) {
                        return uploaded ? item.idx >= 100 : item.idx < 100;
                    }).map(function (item) {
                        return {
                            item: item,
                            search: (item.text + ' ' + item.description).toLowerCase()
                        };
                    });
                }

                function imageTile(entry) {
                    var item = entry.item;

                    return {
                        key: 'img-' + item.idx,
                        kind: 'img',
                        idx: item.idx,
                        src: item.src,
                        name: item.text || item.description || ('#' + item.idx),
                        title: item.description || item.text
                    };
                }

                function glyphTile(source, name) {
                    var cls = source.style ? source.style + ' ' + name : name;

                    return {
                        key: 'glyph-' + source.provider + '-' + name,
                        kind: 'font',
                        cls: cls,
                        provider: source.provider,
                        name: name,
                        title: cls
                    };
                }

                function recentTiles() {
                    return dzIconPickerData.recents().map(function (entry, index) {
                        if (entry.kind === 'img') {
                            var item = findIcon(entry.idx);
                            if (!item && iconSetLoaded) {
                                return null;
                            }

                            return {
                                key: 'recent-img-' + entry.idx + '-' + index,
                                kind: 'img',
                                idx: entry.idx,
                                src: item ? item.src : '',
                                name: item ? (item.text || item.description) : ('#' + entry.idx),
                                title: item ? (item.description || item.text) : ('#' + entry.idx)
                            };
                        }

                        return {
                            key: 'recent-font-' + index,
                            kind: 'font',
                            cls: entry.cls,
                            provider: entry.provider || 'fa',
                            name: glyphNameOf(entry.cls),
                            title: entry.cls
                        };
                    }).filter(function (tile) {
                        return !!tile;
                    });
                }

                function findIcon(idx) {
                    for (var i = 0; i < iconSet.length; i++) {
                        if (iconSet[i].idx === idx) {
                            return iconSet[i];
                        }
                    }

                    return null;
                }

                function isPickable(item) {
                    if (vm.slot !== 'off') {
                        return true;
                    }

                    return item.kind === 'font' && item.provider === vm.sel.provider;
                }

                function pick(item) {
                    if (!isPickable(item)) {
                        return;
                    }

                    if (item.kind === 'img') {
                        vm.sel = { customImage: item.idx, on: '', off: '', provider: '' };
                        vm.offOpen = false;
                        vm.slot = 'on';
                    } else if (vm.slot === 'off') {
                        vm.sel.off = item.cls;
                    } else {
                        if (vm.sel.provider !== item.provider) {
                            vm.sel.off = '';
                        }
                        vm.sel.provider = item.provider;
                        vm.sel.on = item.cls;
                        vm.sel.customImage = 0;
                    }

                    describe();
                }

                function isSelected(item) {
                    if (item.kind === 'img') {
                        return !vm.sel.on && vm.sel.customImage === item.idx;
                    }
                    if (vm.slot === 'off') {
                        return vm.sel.off === item.cls;
                    }

                    return vm.sel.on === item.cls;
                }

                function canSetOff() {
                    return !!vm.sel.on;
                }

                function setSlot(slot) {
                    if (slot === 'off' && !canSetOff()) {
                        return;
                    }

                    vm.slot = slot;
                    if (slot === 'off') {
                        vm.offOpen = true;
                        selectSource(sourceById(vm.sel.provider === 'fa' ? 'fa' : 'lib-' + vm.sel.provider) || vm.active, true);
                    }
                    refresh();
                }

                function toggleOff() {
                    if (vm.offOpen) {
                        vm.offOpen = false;
                        vm.sel.off = '';
                        vm.slot = 'on';
                        describe();
                        refresh();
                        return;
                    }

                    setSlot('off');
                }

                function isDefaultCandidate() {
                    return !vm.sel.on && !vm.sel.customImage;
                }

                function useDefault() {
                    vm.sel = { customImage: 0, on: '', off: '', provider: '' };
                    vm.offOpen = false;
                    vm.slot = 'on';
                    describe();
                    refresh();
                }

                function describe() {
                    var item = vm.sel.customImage > 0 ? findIcon(vm.sel.customImage) : null;
                    var offCls = vm.sel.off || vm.sel.on;

                    vm.candidate = dzIconPickerPreview.of(vm.device, vm.sel, item, vm.defaultImage);
                    // The off slot only ever holds a glyph, never an image or a type default.
                    vm.offCandidate = offCls ? { kind: 'font', cls: offCls } : { kind: 'none' };
                }

                function confirm() {
                    var icon = serializeIcon(vm.sel);

                    if (icon) {
                        dzIconPickerData.addRecent({ kind: 'font', provider: vm.sel.provider || 'fa', cls: vm.sel.on });
                    } else if (vm.sel.customImage > 0) {
                        dzIconPickerData.addRecent({ kind: 'img', idx: vm.sel.customImage });
                    }

                    vm.onSelect({ customImage: vm.sel.customImage, icon: icon });
                }

                function cancel() {
                    vm.onCancel();
                }

                function addLibrary() {
                    // The dialogs jQuery UI parked on <body> outlive the route change otherwise.
                    $('.ui-dialog-content:visible').each(function () {
                        try {
                            $(this).dialog('close');
                        } catch (e) {
                            return;
                        }
                    });

                    vm.onCancel();
                    $location.path('/CustomIcons');
                }

                function onBackdrop(event) {
                    if ($(event.target).hasClass('dz-iconpicker-modal')) {
                        cancel();
                    }
                }

                function onDocumentKeydown(event) {
                    if (event.keyCode !== 27) {
                        return;
                    }

                    event.preventDefault();
                    event.stopPropagation();
                    $scope.$evalAsync(cancel);
                }

                function onKeydown(event) {
                    var key = event.keyCode;

                    if (key === 13) {
                        event.preventDefault();
                        onEnter();
                        return;
                    }
                    if (key !== 37 && key !== 38 && key !== 39 && key !== 40) {
                        return;
                    }
                    if ((key === 37 || key === 39) && $(event.target).hasClass('dz-ip-search')) {
                        return;
                    }
                    if (!vm.results.length) {
                        return;
                    }

                    event.preventDefault();
                    moveHighlight(key);
                }

                function onEnter() {
                    var item = vm.highlight >= 0 ? vm.results[vm.highlight] : null;

                    if (item && !isSelected(item)) {
                        pick(item);
                        return;
                    }

                    confirm();
                }

                function moveHighlight(key) {
                    var last = vm.results.length - 1;
                    var index = vm.highlight;

                    if (index < 0) {
                        index = 0;
                    } else if (key === 37) {
                        index -= 1;
                    } else if (key === 39) {
                        index += 1;
                    } else if (key === 38) {
                        index -= GRID_COLUMNS;
                    } else {
                        index += GRID_COLUMNS;
                    }

                    vm.highlight = Math.max(0, Math.min(last, index));
                    scrollHighlightIntoView();
                }

                function scrollHighlightIntoView() {
                    $timeout(function () {
                        var tile = $element.find('.dz-ip-tile').get(vm.highlight);
                        if (tile && tile.scrollIntoView) {
                            tile.scrollIntoView({ block: 'nearest' });
                        }
                    });
                }
            }]
    });

    app.factory('dzIconPickerDialog', ['$compile', '$rootScope', '$q', function ($compile, $rootScope, $q) {
        var opened = null;

        return {
            open: open,
            close: close
        };

        function open(options) {
            close();

            var opts = options || {};
            var deferred = $q.defer();
            var scope = $rootScope.$new(true);

            scope.state = {
                customImage: parseInt(opts.customImage, 10) || 0,
                icon: opts.icon || '',
                device: opts.device || null,
                defaultImage: opts.defaultImage || '',
                allowGlyphs: opts.allowGlyphs !== false
            };
            scope.select = function (customImage, icon) {
                close();
                deferred.resolve({ customImage: parseInt(customImage, 10) || 0, icon: icon || '' });
            };
            scope.cancel = function () {
                close();
                deferred.resolve(null);
            };

            var element = $compile(
                '<dz-icon-picker-modal custom-image="state.customImage" icon="state.icon"' +
                ' device="state.device"' +
                ' default-image="state.defaultImage" allow-glyphs="state.allowGlyphs"' +
                ' on-select="select(customImage, icon)" on-cancel="cancel()"></dz-icon-picker-modal>')(scope);

            $('body').append(element);
            opened = { scope: scope, element: element };

            if (!$rootScope.$$phase) {
                scope.$digest();
            }

            return deferred.promise;
        }

        function close() {
            if (!opened) {
                return;
            }

            opened.scope.$destroy();
            opened.element.remove();
            opened = null;
        }
    }]);

    app.component('dzIconPicker', {
        template:
            '<span class="dz-icon-field">' +
                '<span class="dz-icon-field-preview" title="{{ $ctrl.selectionLabel }}">' +
                    '<i ng-if="$ctrl.preview.kind === \'font\'" class="{{ $ctrl.preview.cls }}"></i>' +
                    '<img ng-if="$ctrl.preview.kind === \'img\'" ng-src="{{ $ctrl.preview.src }}" alt="">' +
                    '<i ng-if="$ctrl.preview.kind === \'none\'" class="fa-regular fa-square"></i>' +
                '</span>' +
                '<a class="btnsmall dz-icon-field-btn" ng-click="$ctrl.change()">' +
                    '<span data-i18n="Change…">Change…</span>' +
                '</a>' +
            '</span>',
        bindings: {
            customImage: '<',
            icon: '<',
            device: '<',
            defaultImage: '<',
            allowGlyphs: '<',
            onChange: '&'
        },
        controller: ['$element', 'dzIconPickerData', 'dzIconPickerDialog', 'dzIconService', 'dzIconPickerPreview',
            function ($element, dzIconPickerData, dzIconPickerDialog, dzIconService, dzIconPickerPreview) {
                var vm = this;

                var emitted = null;
                var iconSet = [];

                vm.$onInit = init;
                vm.$onChanges = onChanges;
                vm.$postLink = translate;
                vm.change = change;

                function init() {
                    vm.sel = readSelection(vm.customImage, vm.icon);
                    describe();

                    dzIconPickerData.iconSet().then(function (items) {
                        iconSet = items;
                        describe();
                    });

                    dzIconService.preloadBuiltinIcons().then(describe);
                }

                function onChanges(changes) {
                    if (!vm.sel) {
                        return;
                    }
                    if (!changes.customImage && !changes.icon && !changes.defaultImage && !changes.device) {
                        return;
                    }
                    if (signature(vm.customImage, serializeIcon(readSelection(vm.customImage, vm.icon))) === emitted) {
                        describe();
                        return;
                    }

                    vm.sel = readSelection(vm.customImage, vm.icon);
                    describe();
                }

                function translate() {
                    if ($element.i18n) {
                        $element.i18n();
                    }
                }

                function change() {
                    dzIconPickerDialog.open({
                        customImage: vm.sel.customImage,
                        icon: serializeIcon(vm.sel),
                        device: vm.device,
                        defaultImage: vm.defaultImage,
                        allowGlyphs: vm.allowGlyphs
                    }).then(function (result) {
                        if (!result) {
                            return;
                        }

                        vm.sel = readSelection(result.customImage, result.icon);
                        emit();
                    });
                }

                function emit() {
                    var icon = serializeIcon(vm.sel);
                    emitted = signature(vm.sel.customImage, icon);

                    describe();
                    vm.onChange({ customImage: vm.sel.customImage, icon: icon });
                }

                function describe() {
                    var item = vm.sel.customImage > 0 ? findIcon(vm.sel.customImage) : null;

                    vm.preview = dzIconPickerPreview.of(vm.device, vm.sel, item, vm.defaultImage);
                    vm.selectionLabel = label(item);
                }

                function label(item) {
                    if (vm.sel.on) {
                        return vm.sel.on + (vm.sel.off ? ' / ' + vm.sel.off : '');
                    }
                    if (vm.sel.customImage > 0) {
                        return item ? (item.text || item.description) : ('#' + vm.sel.customImage);
                    }

                    return $.t('Default');
                }

                function findIcon(idx) {
                    for (var i = 0; i < iconSet.length; i++) {
                        if (iconSet[i].idx === idx) {
                            return iconSet[i];
                        }
                    }

                    return null;
                }
            }]
    });

    app.factory('dzIconPickerService', ['$compile', '$rootScope', function ($compile, $rootScope) {
        var current = { customImage: 0, icon: '' };
        var mounted = null;

        return {
            mount: mount,
            unmount: unmount,
            getCustomImage: function () {
                return current.customImage;
            },
            getIcon: function () {
                return current.icon;
            }
        };

        function mount(container, options) {
            unmount();

            var opts = options || {};
            current.customImage = parseInt(opts.customImage, 10) || 0;
            current.icon = opts.icon || '';

            var scope = $rootScope.$new(true);
            scope.state = {
                customImage: current.customImage,
                icon: current.icon,
                device: opts.device || null,
                defaultImage: opts.defaultImage || ''
            };
            scope.picked = function (customImage, icon) {
                current.customImage = parseInt(customImage, 10) || 0;
                current.icon = icon || '';
            };

            var element = $compile(
                '<dz-icon-picker custom-image="state.customImage" icon="state.icon"' +
                ' device="state.device" default-image="state.defaultImage"' +
                ' on-change="picked(customImage, icon)"></dz-icon-picker>')(scope);

            $(container).empty().append(element);
            mounted = { scope: scope, element: element };

            if (!$rootScope.$$phase) {
                scope.$digest();
            }

            return element;
        }

        function unmount() {
            if (!mounted) {
                return;
            }

            mounted.scope.$destroy();
            mounted.element.remove();
            mounted = null;
            current.customImage = 0;
            current.icon = '';
        }
    }]);
});
