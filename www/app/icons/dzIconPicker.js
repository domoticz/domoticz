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

    // Home-automation oriented shortcuts into the one flat alphabetical list Font Awesome is.
    // Stems only: which of them the installed version actually ships is resolved at runtime.
    var FA_CATEGORIES = [
        { id: 'home', label: 'Home', stems: ['house', 'house-chimney', 'door-open', 'door-closed', 'couch', 'bed', 'bath', 'shower', 'toilet', 'kitchen-set', 'stairs', 'warehouse', 'key', 'lock', 'bell', 'fingerprint'] },
        { id: 'climate', label: 'Climate', stems: ['temperature-half', 'temperature-high', 'temperature-low', 'fire', 'fire-flame-curved', 'snowflake', 'fan', 'wind', 'droplet', 'sun', 'gauge', 'smog'] },
        { id: 'weather', label: 'Weather', stems: ['cloud', 'cloud-rain', 'cloud-showers-heavy', 'cloud-sun', 'bolt', 'umbrella', 'rainbow', 'moon', 'sun', 'wind', 'snowflake', 'temperature-half'] },
        { id: 'energy', label: 'Energy', stems: ['plug', 'bolt', 'battery-full', 'battery-half', 'solar-panel', 'charging-station', 'gauge-high', 'lightbulb', 'fire-flame-simple', 'oil-can'] },
        { id: 'lighting', label: 'Lighting', stems: ['lightbulb', 'circle-half-stroke', 'sun', 'moon', 'star', 'wand-magic-sparkles', 'tv'] },
        { id: 'av', label: 'Tech / AV', stems: ['tv', 'desktop', 'laptop', 'server', 'hard-drive', 'print', 'mobile-screen', 'tablet-screen-button', 'headphones', 'volume-high', 'music', 'gamepad', 'camera', 'video', 'wifi', 'network-wired', 'satellite-dish', 'robot'] },
        { id: 'appliances', label: 'Appliances', stems: ['blender', 'mug-hot', 'utensils', 'kitchen-set', 'fire-burner', 'temperature-arrow-up', 'soap', 'jug-detergent', 'shirt', 'sink'] },
        { id: 'security', label: 'Security', stems: ['shield-halved', 'lock', 'lock-open', 'key', 'bell', 'video', 'camera', 'fingerprint', 'user-shield', 'triangle-exclamation', 'person-through-window'] },
        { id: 'outdoor', label: 'Outdoor', stems: ['car', 'car-battery', 'charging-station', 'tree', 'seedling', 'faucet', 'faucet-drip', 'water-ladder', 'trash-can', 'dumpster', 'trailer'] },
        { id: 'people', label: 'People', stems: ['user', 'users', 'baby', 'person-walking', 'child', 'paw', 'dog', 'cat'] },
        { id: 'status', label: 'Status', stems: ['circle-check', 'circle-xmark', 'triangle-exclamation', 'circle-info', 'circle-question', 'power-off', 'play', 'pause', 'stop', 'arrow-rotate-right'] }
    ];

    var FA_STYLE = 'fa-solid';
    var MAX_TILES = 300;
    var GRID_COLUMNS = 7;
    var RECENT_LIMIT = 12;
    var RECENT_KEY = 'dz-icon-recents';
    var SEARCH_DELAY = 200;
    var ASSETS_CHANGED = 'dz-webassets-changed';
    var RECONCILE_DELAY = 400;

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

    app.factory('dzIconPickerData', ['$q', '$rootScope', '$timeout', 'domoticzApi', function ($q, $rootScope, $timeout, domoticzApi) {
        var iconSetRequest = null;
        var librariesRequest = null;
        var glyphCache = {};
        var injectedLinks = {};
        var reconcileTimer = null;

        $rootScope.$on(ASSETS_CHANGED, invalidate);

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
                                    // Kept so a tile can preview the glyph the resolver will
                                    // actually draw for a built-in, instead of its PNG.
                                    FaClass: item.FaClass || '',
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
                        var rows = (data.result || [])
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

                        // Only a fetch that actually succeeded may retire links and recents.
                        reconcile(rows);
                        return rows;
                    })
                    .catch(function () {
                        return [];
                    });
            }

            return librariesRequest;
        }

        // Libraries can be installed or removed while the app stays loaded, so every memo of the
        // installed set has to go; the next consumer refetches it.
        function invalidate() {
            iconSetRequest = null;
            librariesRequest = null;
            glyphCache = {};

            if (!hasStaleArtefacts()) {
                return;
            }
            if (reconcileTimer) {
                $timeout.cancel(reconcileTimer);
            }
            // A burst of mutations shares one getwebassets, and any consumer asking in the
            // meantime shares it too because librariesRequest memoises again.
            reconcileTimer = $timeout(function () {
                reconcileTimer = null;
                libraries();
            }, RECONCILE_DELAY);
        }

        // Without a fresh list there is nothing to compare against, so only pay for one when
        // something could actually have gone stale.
        function hasStaleArtefacts() {
            if (Object.keys(injectedLinks).length) {
                return true;
            }

            return recents().some(isLibraryRecent);
        }

        function isLibraryRecent(entry) {
            return entry.kind === 'font' && (entry.provider || 'fa') !== 'fa';
        }

        function reconcile(rows) {
            var prefixes = {};
            var hrefs = {};

            rows.forEach(function (row) {
                prefixes[row.prefix] = true;
                hrefs[row.css] = true;
            });

            Object.keys(injectedLinks).forEach(function (href) {
                if (hrefs[href]) {
                    return;
                }

                var link = injectedLinks[href];
                delete injectedLinks[href];
                if (link.parentNode) {
                    link.parentNode.removeChild(link);
                }
            });

            pruneRecents(prefixes);
        }

        // A recent glyph from a removed library can no longer render, so it would sit in the rail
        // as an empty tile. Images and Font Awesome always survive.
        function pruneRecents(prefixes) {
            var list = recents();
            var kept = list.filter(function (entry) {
                return !isLibraryRecent(entry) || prefixes[entry.provider];
            });

            if (kept.length === list.length) {
                return;
            }

            try {
                window.localStorage.setItem(RECENT_KEY, JSON.stringify(kept));
            } catch (e) {
                return;
            }
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
                // Elsewhere the codepoint is the tell. Sizing and transform helpers such as
                // .ri-10x or .mdi-rotate-90 carry the glyph prefix but would render blank.
                if (!isFa && !hasGlyphContent(rule)) {
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

        function hasGlyphContent(rule) {
            // Not every icon set is a font. Sets like Iconoir ship no @font-face at all and
            // draw each icon as an SVG mask, so they declare mask-image and never a codepoint;
            // requiring content would enumerate nothing for them. Utility classes declare
            // neither, which is what this test is really filtering out.
            if (hasMaskImage(rule)) {
                return true;
            }

            var value = (readProperty(rule, 'content') || '').trim();

            if (!value || value === 'none' || value === 'normal') {
                return false;
            }

            // content:"" declares the property without a codepoint.
            return !!value.replace(/^(['"])([\s\S]*)\1$/, '$2').trim();
        }

        function hasMaskImage(rule) {
            var value = (readProperty(rule, 'mask-image') || readProperty(rule, '-webkit-mask-image') || '').trim();
            return !!value && (value !== 'none');
        }

        // Libraries differ in where they hang the codepoint: on a lone class (.ri-home-line:before)
        // or on a compound with the base class (.ph.ph-acorn:before). Both have to yield the glyph.
        function glyphClassName(selector, prefix) {
            var compound = selector.trim().replace(/::?(before|after)$/, '');

            // Class chains only. A combinator, attribute or id selector could otherwise pull in a
            // class that merely carries the prefix without being a glyph of its own.
            if (!/^(?:\.[A-Za-z0-9_-]+)+$/.test(compound)) {
                return null;
            }

            var tokens = compound.split('.');
            var name = null;

            for (var i = 1; i < tokens.length; i++) {
                // Last match wins: weight variants read .ph-fill.ph-acorn, glyph token rightmost.
                if (tokens[i].indexOf(prefix + '-') === 0) {
                    name = tokens[i];
                }
            }

            return (name && !FA_SKIP[name]) ? name : null;
        }

        function ensureStylesheet(href) {
            if (!href) {
                return $q.resolve(false);
            }

            var link = findLink(href);
            if (!link) {
                link = document.createElement('link');
                link.rel = 'stylesheet';
                link.href = href;
                document.getElementsByTagName('head')[0].appendChild(link);
                // Only links added here may ever be removed again; Domoticz owns the rest.
                injectedLinks[href] = link;
            }

            // Resolve once the stylesheet has actually parsed (its rules become readable),
            // not after a fixed delay: a large or slowly-served icon library can take well
            // over half a second, and giving up early made it look permanently empty. Poll
            // for readiness up to a generous cap; a sheet that never parses (missing file,
            // load error) resolves false so the caller can report it as a load failure.
            var deferred = $q.defer();
            var settled = false;
            function settle(value) { if (!settled) { settled = true; deferred.resolve(value); } }
            function sheetReady() {
                try {
                    // Same-origin sheets only, which the icon libraries are; a throw or an
                    // empty rule list means it has not finished parsing yet.
                    var sheet = link.sheet;
                    return !!(sheet && sheet.cssRules && sheet.cssRules.length);
                } catch (e) {
                    return false;
                }
            }
            link.addEventListener('error', function () { settle(false); });
            var waited = 0, step = 100, cap = 6000;
            (function poll() {
                if (settled) { return; }
                if (sheetReady()) { settle(true); return; }
                if (waited >= cap) { settle(false); return; }
                waited += step;
                $timeout(poll, step);
            })();
            return deferred.promise;
        }

        function findLink(href) {
            var links = document.getElementsByTagName('link');

            for (var i = 0; i < links.length; i++) {
                if ((links[i].getAttribute('href') || '') === href) {
                    return links[i];
                }
            }

            return null;
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
            if (src) {
                return { kind: 'img', src: src };
            }

            // Only the glyph style resolves to a font, so without this the classic style (the
            // default) discards the image the resolver just produced and draws the empty-square
            // placeholder instead. Mount points that pass a defaultImage still win above; this is
            // the fallback for the ones that pass the device record instead.
            if (resolved && resolved.kind === 'img') {
                return resolved;
            }

            return { kind: 'none' };
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
                            '<div class="dz-ip-chips" ng-show="$ctrl.chipsVisible">' +
                                '<a class="dz-ip-chip" ng-repeat="chip in $ctrl.chips track by chip.id"' +
                                   ' ng-class="{\'dz-ip-chip-sel\': $ctrl.category === chip.id}"' +
                                   ' ng-click="$ctrl.setCategory(chip.id)">{{ chip.label }}</a>' +
                            '</div>' +
                            '<div class="dz-ip-groups">' +
                                '<div class="dz-ip-group" ng-repeat="group in $ctrl.groups track by group.id">' +
                                    '<div class="dz-ip-group-head" ng-if="group.label">' +
                                        '<span class="dz-ip-group-name">{{ group.label }}</span>' +
                                        '<span class="dz-ip-group-count">{{ group.count }}</span>' +
                                    '</div>' +
                                    '<div class="dz-ip-grid">' +
                                        '<a class="dz-ip-tile" ng-repeat="item in group.items track by item.key"' +
                                           ' ng-class="{\'dz-ip-tile-sel\': $ctrl.isSelected(item),' +
                                           ' \'dz-ip-tile-hl\': item.flat === $ctrl.highlight}"' +
                                           ' ng-click="$ctrl.pick(item)" ng-dblclick="$ctrl.confirm()" title="{{ item.title }}">' +
                                            '<span class="dz-ip-tile-icon">' +
                                                '<i ng-if="item.cls" class="{{ item.cls }}"></i>' +
                                                '<img ng-if="!item.cls" ng-src="{{ item.src }}" alt="">' +
                                            '</span>' +
                                            '<span class="dz-ip-tile-name">{{ item.name }}</span>' +
                                        '</a>' +
                                    '</div>' +
                                '</div>' +
                            '</div>' +
                            '<div class="dz-ip-note" ng-show="!$ctrl.results.length && $ctrl.emptyLibrary"' +
                                 ' data-i18n="The stylesheet for this icon library could not be loaded">' +
                                'The stylesheet for this icon library could not be loaded</div>' +
                            '<div class="dz-ip-note" ng-show="!$ctrl.results.length && !$ctrl.emptyLibrary && $ctrl.categoryActive"' +
                                 ' data-i18n="No icons in this category match your search">' +
                                'No icons in this category match your search</div>' +
                            '<div class="dz-ip-note" ng-show="!$ctrl.results.length && !$ctrl.emptyLibrary && !$ctrl.categoryActive"' +
                                 ' data-i18n="No icons found">No icons found</div>' +
                            '<div class="dz-ip-note" ng-show="$ctrl.truncated">{{ $ctrl.truncationNote }}</div>' +
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
        controller: ['$element', '$scope', '$rootScope', '$timeout', '$location', 'dzIconPickerData', 'dzIconService', 'dzIconPickerPreview',
            function ($element, $scope, $rootScope, $timeout, $location, dzIconPickerData, dzIconService, dzIconPickerPreview) {
                var vm = this;

                var iconSet = [];
                var iconSetLoaded = false;
                var libraries = [];
                var searchTimer = null;
                // Keyed by provider, not kept on the source, because a rebuild replaces those
                // objects and a library whose css never resolves would retry forever.
                var retried = {};
                // Providers whose one retry has been and gone without producing a glyph: broken
                // stylesheet, not an empty library.
                var exhausted = {};
                var awaitingLibraries = false;

                vm.$onInit = init;
                vm.$postLink = postLink;
                vm.$onDestroy = destroy;

                vm.selectSource = selectSource;
                vm.isSourceDisabled = isSourceDisabled;
                vm.countLabel = countLabel;
                vm.setCategory = setCategory;
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
                    vm.category = '';
                    vm.chips = [];
                    vm.chipsVisible = false;
                    vm.results = [];
                    vm.groups = [];
                    vm.matchCount = 0;
                    vm.truncated = false;
                    vm.truncationNote = '';
                    vm.highlight = -1;
                    vm.sel = readSelection(vm.customImage, vm.icon);
                    vm.offOpen = !!vm.sel.off;
                    vm.slot = 'on';

                    describe();
                    buildSources();
                    selectSource(initialSource());

                    load();
                    $scope.$on(ASSETS_CHANGED, reload);
                    dzIconService.preloadBuiltinIcons().then(describe);
                }

                function load() {
                    dzIconPickerData.iconSet().then(function (items) {
                        iconSet = items;
                        iconSetLoaded = true;
                        rebuild();
                    });

                    dzIconPickerData.libraries().then(function (rows) {
                        libraries = rows;
                        rebuild();
                    });
                }

                // The picker can be open on a device page while libraries change elsewhere; the
                // selection is untouched, only the sources behind it are refetched.
                function reload() {
                    retried = {};
                    exhausted = {};
                    load();
                }

                function postLink() {
                    if ($element.i18n) {
                        $element.i18n();
                    }

                    document.addEventListener('keydown', onDocumentKeydown, true);
                    window.addEventListener('focus', onWindowFocus);

                    $timeout(function () {
                        $element.find('.dz-ip-search').trigger('focus');
                    });
                }

                function destroy() {
                    document.removeEventListener('keydown', onDocumentKeydown, true);
                    window.removeEventListener('focus', onWindowFocus);
                    if (searchTimer) {
                        $timeout.cancel(searchTimer);
                        searchTimer = null;
                    }
                }

                function rebuild() {
                    var activeId = vm.active ? vm.active.id : null;
                    buildSources();
                    describe();
                    selectSource(sourceById(activeId) || initialSource());
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

                    var searchable = list.filter(isSearchable);
                    if (searchable.length > 1) {
                        // Broadest scope first, so the rail reads everything then narrows. Recent
                        // is deliberately left out of the aggregate: it only ever repeats tiles
                        // the provider groups below it already carry.
                        list.unshift({
                            id: 'all',
                            kind: 'all',
                            title: $.t('All icons'),
                            glyph: 'fa-solid fa-magnifying-glass',
                            count: searchable.reduce(function (sum, source) {
                                return sum + (source.count || 0);
                            }, 0)
                        });
                    }

                    vm.sources = list;
                    buildCategories();
                }

                function isSearchable(source) {
                    return source.kind === 'image' || source.kind === 'glyph';
                }

                // Curated stems are only worth a chip where the installed Font Awesome actually
                // ships them; anything else would render as a blank tile.
                function buildCategories() {
                    if (vm.allowGlyphs === false) {
                        vm.chips = [];
                        vm.category = '';
                        return;
                    }

                    var available = {};
                    dzIconPickerData.glyphs('fa').forEach(function (name) {
                        available[name] = true;
                    });

                    var chips = [];
                    FA_CATEGORIES.forEach(function (category) {
                        var names = category.stems.map(function (stem) {
                            return 'fa-' + stem;
                        }).filter(function (name) {
                            return available[name];
                        });

                        if (names.length) {
                            chips.push({ id: category.id, label: $.t(category.label), names: names });
                        }
                    });

                    vm.chips = chips.length ? [{ id: '', label: $.t('All'), names: null }].concat(chips) : [];
                    if (!chipById(vm.category)) {
                        vm.category = '';
                    }
                }

                function chipById(id) {
                    var found = vm.chips.filter(function (chip) {
                        return chip.id === id;
                    });

                    return found.length ? found[0] : null;
                }

                function setCategory(id) {
                    var chip = chipById(id);

                    vm.category = chip ? chip.id : '';
                    refresh();
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

                // The query survives a source change: it is the one thing the user typed, and
                // dropping it made looking for the same word in another library a retype.
                function selectSource(source) {
                    if (!source || isSourceDisabled(source)) {
                        return;
                    }

                    vm.active = source;
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
                        vm.groups = [];
                        vm.matchCount = 0;
                        vm.truncated = false;
                        vm.truncationNote = '';
                        return;
                    }

                    var query = (vm.query || '').toLowerCase();
                    var buckets = (source.kind === 'all'
                        ? vm.sources.filter(isSearchable)
                        : [source]
                    ).map(function (member) {
                        return bucket(member, query);
                    }).filter(function (entry) {
                        return entry.matched.length;
                    });

                    // A single scope needs no heading; the rail already says which one it is.
                    if (source.kind !== 'all' && buckets.length) {
                        buckets[0].label = '';
                    }

                    vm.matchCount = buckets.reduce(function (sum, entry) {
                        return sum + entry.matched.length;
                    }, 0);
                    vm.chipsVisible = source.kind === 'glyph' && source.provider === 'fa' && !!vm.chips.length;
                    vm.categoryActive = vm.chipsVisible && !!vm.category;
                    vm.emptyLibrary = source.kind === 'glyph' && !!exhausted[source.provider];

                    materialise(buckets);

                    vm.highlight = -1;
                    retryLibrary(source);
                }

                function bucket(source, query) {
                    return {
                        id: source.id,
                        label: source.title,
                        matched: matchesOf(source, query),
                        tile: tileMaker(source)
                    };
                }

                function matchesOf(source, query) {
                    if (!offersPickable(source)) {
                        return [];
                    }
                    if (source.kind === 'recent') {
                        return source.items.filter(function (item) {
                            return matchesTile(item, query) && isPickable(item);
                        });
                    }
                    if (source.kind === 'image') {
                        return imageNames(source.uploaded).filter(function (entry) {
                            return !query || entry.search.indexOf(query) !== -1;
                        });
                    }

                    return glyphPool(source).filter(function (name) {
                        return !query || name.toLowerCase().indexOf(query) !== -1;
                    });
                }

                // A category narrows the pool the query then searches, rather than replacing it,
                // so the chip and the search box compose the way the rail and the search box do.
                function glyphPool(source) {
                    var chip = source.provider === 'fa' && vm.category ? chipById(vm.category) : null;

                    return (chip && chip.names) || dzIconPickerData.glyphs(source.provider);
                }

                // Pickability in the off slot is a property of the provider, so one probe per
                // source keeps a five-thousand-name filter from running for nothing.
                function offersPickable(source) {
                    if (source.kind === 'recent') {
                        return true;
                    }

                    return isPickable(source.kind === 'image'
                        ? { kind: 'img' }
                        : { kind: 'font', provider: source.provider });
                }

                function tileMaker(source) {
                    if (source.kind === 'image') {
                        return imageTile;
                    }
                    if (source.kind === 'recent') {
                        return function (item) {
                            return item;
                        };
                    }

                    return function (name) {
                        return glyphTile(source, name);
                    };
                }

                // Every bucket gets an equal share of the cap, so an aggregate search cannot be
                // filled entirely by whichever provider comes first. Shares are handed out
                // smallest bucket first: one that wants less than its share releases the rest to
                // the bigger ones instead of leaving the cap unspent.
                function allot(buckets) {
                    var remaining = MAX_TILES;
                    var pending = buckets.length;

                    buckets.slice().sort(function (a, b) {
                        return a.matched.length - b.matched.length;
                    }).forEach(function (entry) {
                        entry.take = Math.min(entry.matched.length, Math.ceil(remaining / pending));
                        remaining -= entry.take;
                        pending -= 1;
                    });
                }

                function materialise(buckets) {
                    var results = [];

                    allot(buckets);

                    vm.groups = buckets.map(function (entry) {
                        var items = entry.matched.slice(0, entry.take).map(function (candidate) {
                            return entry.tile(candidate);
                        });
                        items.forEach(function (item) {
                            // Keyboard navigation runs over the flat order, which the groups render in.
                            item.flat = results.length;
                            results.push(item);
                        });

                        return {
                            id: entry.id,
                            label: entry.label,
                            count: formatCount(entry.matched.length),
                            items: items
                        };
                    });

                    vm.results = results;
                    vm.truncated = vm.matchCount > results.length;
                    vm.truncationNote = vm.truncated
                        ? $.t('Showing __shown__ of __total__ - keep typing to narrow down',
                            { shown: results.length, total: vm.matchCount })
                        : '';
                }

                // "All icons" has no provider of its own, so a library that is still loading has
                // to be retried through the member source it belongs to. Retrying only the
                // selected source would leave that library missing from the combined results
                // until the user went and picked it on its own.
                function retryLibrary(source) {
                    if (source.kind === 'all') {
                        vm.sources.filter(isSearchable).forEach(retryOne);
                        return;
                    }

                    retryOne(source);
                }

                function retryOne(source) {
                    if (source.kind !== 'glyph' || source.provider === 'fa' || retried[source.provider]) {
                        return;
                    }
                    // Whether the provider enumerated anything is the signal to retry on, not
                    // whether the query matched: in the aggregate view another library having
                    // matched says nothing about this one, and a query that matches none of a
                    // loaded library's names is not a load failure.
                    if (dzIconPickerData.glyphs(source.provider).length) {
                        return;
                    }

                    retried[source.provider] = true;
                    dzIconPickerData.ensureStylesheet(source.css).then(function (added) {
                        if (added) {
                            dzIconPickerData.forgetGlyphs(source.provider);
                            // The one retry has fired. Still nothing to enumerate means the
                            // stylesheet never loaded, which is a different dead end from a
                            // query that matched nothing, and has to read as one.
                            exhausted[source.provider] = !dzIconPickerData.glyphs(source.provider).length;
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

                // Mirrors dzIconService's own gate, so a tile can never promise something the
                // resolver would not draw.
                function glyphStyle() {
                    return !!($rootScope.config && $rootScope.config.IconStyle == 1);
                }

                function imageTile(entry) {
                    var item = entry.item;

                    return {
                        key: 'img-' + item.idx,
                        kind: 'img',
                        idx: item.idx,
                        src: item.src,
                        // A tile shows what picking it will actually produce, which depends on
                        // Settings > Icon style: the glyph style stands the FaClass from
                        // switch_icons.txt in for a built-in, the classic style keeps the PNG.
                        // kind stays 'img' either way — the selection is still a CustomImage.
                        cls: glyphStyle() ? (item.FaClass || '') : '',
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
                        selectSource(sourceById(vm.sel.provider === 'fa' ? 'fa' : 'lib-' + vm.sel.provider) || vm.active);
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

                // Custom Icons opens beside the picker rather than in place of it: installing a
                // library is a detour, and routing there threw away the selection being made.
                function addLibrary() {
                    if (window.open('#/CustomIcons', '_blank')) {
                        awaitingLibraries = true;
                        return;
                    }

                    // Popup blocked, so leaving is the only way there. Say what it costs first.
                    bootbox.confirm($.t('Adding an icon library leaves this dialog and discards the icon you picked. Continue?'), function (result) {
                        if (result) {
                            $scope.$evalAsync(leaveForLibraries);
                        }
                    });
                }

                function leaveForLibraries() {
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

                // A library installed in the other tab broadcasts nothing into this one, so coming
                // back is the cue to refetch. Same path an in-tab install takes.
                function onWindowFocus() {
                    if (!awaitingLibraries) {
                        return;
                    }

                    awaitingLibraries = false;
                    $scope.$evalAsync(function () {
                        $rootScope.$broadcast(ASSETS_CHANGED);
                    });
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
