define(['app'], function (app) {
    'use strict';

    /* ------------------------------------------------------------------
     * One icon picker for the whole web UI.
     *
     * It offers four sources, and hands the result back as the two fields
     * the server understands:
     *
     *   CustomImage  the icon set entry (0 = none), as it has always been
     *   Icon         {"t":"fa","on":"fa-solid fa-lightbulb","off":"..."}
     *
     * Exactly one of the two is ever set: a device with an empty Icon keeps
     * behaving the way it did before this picker existed, and "Default"
     * clears both.
     * ------------------------------------------------------------------ */

    /* Font Awesome 7 declares one rule per glyph, '.fa-lightbulb{--fa:"\f0eb"}',
       and never sets --fa on a style or utility class (fa-solid, fa-2xl,
       fa-spin, fa-fw, ...). The presence of --fa is therefore what identifies a
       glyph selector; the list below is a second line of defence in case a
       future release starts setting --fa on something that is not an icon.
       Note that .fa-0 .. .fa-9 ARE glyphs (the digits) while .fa-1x .. .fa-10x
       are sizes, so the two cannot be told apart by shape alone. */
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

    /* The style half of a Font Awesome class pair. Only the two families that
       index.html actually loads are offered. */
    var FA_STYLES = ['fa-solid', 'fa-regular'];

    var FA_SKIP = {};
    FA_NON_GLYPH.forEach(function (name) {
        FA_SKIP[name] = true;
    });

    /* A class list ends up in a class attribute and is validated the same way
       server side, so keep to what a CSS class list may contain. */
    var SAFE_CLASS_RE = /^[A-Za-z0-9 _-]+$/;

    /* How many tiles are put in the DOM at once. Font Awesome alone has well
       over a thousand glyphs; the rest is reached by searching or by More. */
    var PAGE_SIZE = 120;

    /* ------------------------------------------------------------------
     * Shared data: the icon set, the installed libraries, and the glyph
     * names scraped out of the loaded stylesheets. All cached, because the
     * picker is created and destroyed once per dialog.
     * ------------------------------------------------------------------ */
    app.factory('dzIconPickerData', ['$q', '$timeout', 'domoticzApi', function ($q, $timeout, domoticzApi) {
        var iconSetRequest = null;
        var librariesRequest = null;
        var glyphCache = {};

        return {
            iconSet: iconSet,
            libraries: libraries,
            glyphs: glyphs,
            forgetGlyphs: forgetGlyphs,
            ensureStylesheet: ensureStylesheet
        };

        /* The built-in and the ZIP-uploaded icons, in one list, from the same
           command the old ddslick dropdown used. */
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
                                /* idx 0 is the "no custom image" entry; the
                                   picker has its own Default source for it. */
                                return item.idx !== 0;
                            });
                    })
                    .catch(function () {
                        return [];
                    });
            }

            return iconSetRequest;
        }

        /* Installed icon libraries. The command is optional: an older server
           simply does not have it, and then the picker offers Font Awesome
           only. */
        function libraries() {
            if (!librariesRequest) {
                librariesRequest = domoticzApi.sendCommand('geticonlibraries', {})
                    .then(function (data) {
                        return (data.result || []).filter(function (row) {
                            return row && row.Prefix;
                        });
                    })
                    .catch(function () {
                        return [];
                    });
            }

            return librariesRequest;
        }

        /* Glyph class names for a provider prefix ('fa', 'mdi', ...), read out
           of the stylesheets the page already has. All of them are same-origin,
           so their rules are readable; the try/catch is there for the day
           someone adds a CDN link. */
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

        /* A library stylesheet that arrives after the first scan needs a new
           one, so the cache can be dropped per provider. */
        function forgetGlyphs(prefix) {
            delete glyphCache[prefix];
        }

        function collect(rules, prefix, isFa, names) {
            for (var i = 0; i < rules.length; i++) {
                var rule = rules[i];

                /* @media and @supports wrap their own rule list. */
                if (rule.cssRules && rule.cssRules.length) {
                    collect(rule.cssRules, prefix, isFa, names);
                    continue;
                }
                if (!rule.selectorText || !rule.style) {
                    continue;
                }
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

        /* '.mdi-account::before' -> 'mdi-account', and nothing for selectors
           that are not a single class of this provider. */
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

        /* Pull in a library stylesheet the page does not have yet, so its
           glyphs can be enumerated at all. */
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

            /* No load event to wait on that works across browsers for
               stylesheets in every state, so give it a moment and rescan. */
            return $timeout(angular.noop, 600).then(function () {
                return true;
            });
        }
    }]);

    /* ------------------------------------------------------------------
     * <dz-icon-picker custom-image="..." icon="..." default-image="..."
     *                 allow-glyphs="..." on-change="pick(customImage, icon)">
     *
     * Reports both fields on every change; the host decides where to put
     * them. Nothing is written to the server from here.
     * ------------------------------------------------------------------ */
    app.component('dzIconPicker', {
        template:
            '<div class="dz-icon-picker">' +
                '<div class="dz-icon-picker-current">' +
                    '<span class="dz-icon-picker-preview">' +
                        '<i ng-if="$ctrl.preview.kind === \'font\'" class="{{ $ctrl.preview.cls }}"></i>' +
                        '<img ng-if="$ctrl.preview.kind === \'img\'" ng-src="{{ $ctrl.preview.src }}">' +
                    '</span>' +
                    '<span class="dz-icon-picker-name" title="{{ $ctrl.selectionLabel }}">{{ $ctrl.selectionLabel }}</span>' +
                    '<a class="btnsmall dz-icon-picker-toggle" ng-click="$ctrl.toggle()">' +
                        '<span ng-if="!$ctrl.isOpen" data-i18n="Change">Change</span>' +
                        '<span ng-if="$ctrl.isOpen" data-i18n="Close">Close</span>' +
                    '</a>' +
                '</div>' +
                '<div class="dz-icon-picker-panel" ng-if="$ctrl.isOpen">' +
                    '<div class="dz-icon-picker-sources">' +
                        '<a ng-repeat="source in $ctrl.sources track by source.id"' +
                           ' ng-class="source.id === $ctrl.activeSource.id ? \'btnsmall-sel\' : \'btnsmall\'"' +
                           ' ng-click="$ctrl.selectSource(source)">{{ source.title }}</a>' +
                    '</div>' +
                    '<div class="dz-icon-picker-default" ng-if="$ctrl.activeSource.kind === \'default\'">' +
                        '<span data-i18n="No icon of your own - Domoticz picks one for this device type">' +
                            'No icon of your own - Domoticz picks one for this device type</span> ' +
                        '<a class="btnsmall" ng-click="$ctrl.pickDefault()">' +
                            '<span data-i18n="Use default">Use default</span>' +
                        '</a>' +
                    '</div>' +
                    '<div ng-if="$ctrl.activeSource.kind !== \'default\'">' +
                        '<div class="dz-icon-picker-tools">' +
                            '<input type="text" class="dz-icon-picker-search" ng-model="$ctrl.query"' +
                                 ' ng-change="$ctrl.refresh()" ng-keydown="$ctrl.onSearchKeydown($event)"' +
                                 ' data-i18n="[placeholder]Search" placeholder="Search" autocomplete="off">' +
                            '<select class="dz-icon-picker-style" ng-if="$ctrl.activeSource.kind === \'fa\'"' +
                                  ' ng-model="$ctrl.faStyle" ng-change="$ctrl.onStyleChange()"' +
                                  ' ng-options="style.value as style.label for style in $ctrl.faStyles"></select>' +
                        '</div>' +
                        '<div class="dz-icon-picker-grid">' +
                            '<a class="dz-icon-picker-tile" ng-repeat="item in $ctrl.results track by item.key"' +
                               ' ng-class="{selected: $ctrl.isSelected(item)}" ng-click="$ctrl.pick(item)"' +
                               ' title="{{ item.title }}">' +
                                '<i ng-if="item.kind === \'font\'" class="{{ item.cls }}"></i>' +
                                '<img ng-if="item.kind === \'img\'" ng-src="{{ item.src }}">' +
                            '</a>' +
                        '</div>' +
                        '<div class="dz-icon-picker-note">' +
                            '<span ng-if="!$ctrl.matchCount" data-i18n="No icons found">No icons found</span>' +
                            '<a class="btnsmall" ng-if="$ctrl.results.length < $ctrl.matchCount" ng-click="$ctrl.showMore()">' +
                                '<span data-i18n="More">More</span>' +
                            '</a>' +
                            '<span ng-if="$ctrl.matchCount">{{ $ctrl.results.length }} / {{ $ctrl.matchCount }}</span>' +
                        '</div>' +
                    '</div>' +
                    '<div class="dz-icon-picker-off" ng-if="$ctrl.canSetOff()">' +
                        '<label class="dz-icon-picker-off-label">' +
                            '<input type="checkbox" ng-model="$ctrl.offEnabled" ng-change="$ctrl.onOffToggle()">' +
                            '<span data-i18n="Separate icon for the off state">Separate icon for the off state</span>' +
                        '</label>' +
                        '<span ng-if="$ctrl.offEnabled">' +
                            '<a ng-class="$ctrl.target === \'on\' ? \'btnsmall-sel\' : \'btnsmall\'" ng-click="$ctrl.setTarget(\'on\')">' +
                                '<span data-i18n="On">On</span> <i class="{{ $ctrl.sel.on }}"></i>' +
                            '</a>' +
                            '<a ng-class="$ctrl.target === \'off\' ? \'btnsmall-sel\' : \'btnsmall\'" ng-click="$ctrl.setTarget(\'off\')">' +
                                '<span data-i18n="Off">Off</span> <i class="{{ $ctrl.sel.off || $ctrl.sel.on }}"></i>' +
                            '</a>' +
                        '</span>' +
                    '</div>' +
                '</div>' +
            '</div>',
        bindings: {
            customImage: '<',
            icon: '<',
            defaultImage: '<',
            allowGlyphs: '<',
            onChange: '&'
        },
        controller: ['$element', 'dzIconPickerData', function ($element, dzIconPickerData) {
            var vm = this;

            /* What the last emit() reported, so a binding update caused by our
               own change does not overwrite the in-progress selection. */
            var emitted = null;
            var iconSet = [];

            vm.$onInit = init;
            vm.$onChanges = onChanges;
            vm.$postLink = translate;

            vm.toggle = toggle;
            vm.selectSource = selectSource;
            vm.refresh = refresh;
            vm.showMore = showMore;
            vm.pick = pick;
            vm.pickDefault = pickDefault;
            vm.isSelected = isSelected;
            vm.canSetOff = canSetOff;
            vm.onOffToggle = onOffToggle;
            vm.onStyleChange = onStyleChange;
            vm.onSearchKeydown = onSearchKeydown;
            vm.setTarget = setTarget;

            function init() {
                vm.isOpen = false;
                vm.query = '';
                vm.limit = PAGE_SIZE;
                vm.results = [];
                vm.matchCount = 0;
                vm.faStyles = [
                    { value: 'fa-solid', label: $.t('Solid') },
                    { value: 'fa-regular', label: $.t('Regular') }
                ];
                vm.faStyle = FA_STYLES[0];
                vm.target = 'on';
                vm.offEnabled = false;
                vm.sel = { customImage: 0, on: '', off: '', provider: '' };

                readBindings();
                buildSources();

                dzIconPickerData.iconSet().then(function (items) {
                    iconSet = items;
                    buildSources();
                    describe();
                    refresh();
                });
            }

            function onChanges(changes) {
                /* $onChanges also fires for the initial bindings, before
                   $onInit has set any state up. */
                if (!vm.sel) {
                    return;
                }
                if (changes.allowGlyphs) {
                    buildSources();
                }
                if (!changes.customImage && !changes.icon) {
                    return;
                }
                /* A binding update caused by our own emit carries exactly what
                   we reported, and must not restart the selection. */
                if (signature(vm.customImage, serialize(parseIcon(vm.icon))) === emitted) {
                    return;
                }

                readBindings();
                describe();
                refresh();
            }

            /* The surrounding markup translates through data-i18n, and the
               picker is compiled into a jQuery dialog as well as into a route
               template, so run the pass ourselves rather than rely on whoever
               happens to own the container. */
            function translate() {
                if ($element.i18n) {
                    $element.i18n();
                }
            }

            /* -------- selection state -------- */

            function readBindings() {
                var parsed = parseIcon(vm.icon);

                vm.sel = {
                    customImage: parseInt(vm.customImage, 10) || 0,
                    on: parsed.on,
                    off: parsed.off,
                    provider: parsed.provider
                };
                vm.offEnabled = !!parsed.off;
                vm.target = 'on';

                /* Reopening on a glyph should show the style it was saved with. */
                if (parsed.provider === 'fa') {
                    FA_STYLES.forEach(function (style) {
                        if ((' ' + parsed.on + ' ').indexOf(' ' + style + ' ') !== -1) {
                            vm.faStyle = style;
                        }
                    });
                }

                describe();
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
                    on: clean(parsed.on),
                    off: typeof parsed.off === 'string' ? clean(parsed.off) : ''
                };
            }

            function clean(cls) {
                var value = String(cls).replace(/\s+/g, ' ').trim();
                return SAFE_CLASS_RE.test(value) ? value : '';
            }

            function serialize(sel) {
                if (!sel.on) {
                    return '';
                }

                var payload = { t: sel.provider || 'fa', on: sel.on };
                if (sel.off && sel.off !== sel.on) {
                    payload.off = sel.off;
                }

                return JSON.stringify(payload);
            }

            function iconJson() {
                return serialize(vm.sel);
            }

            function signature(customImage, icon) {
                return (parseInt(customImage, 10) || 0) + '|' + (icon || '');
            }

            function emit() {
                var icon = iconJson();
                emitted = signature(vm.sel.customImage, icon);

                describe();
                vm.onChange({ customImage: vm.sel.customImage, icon: icon });
            }

            /* -------- preview -------- */

            function describe() {
                if (vm.sel.on) {
                    vm.preview = { kind: 'font', cls: vm.sel.on };
                    vm.selectionLabel = vm.sel.on + (vm.sel.off ? ' / ' + vm.sel.off : '');
                    return;
                }

                if (vm.sel.customImage > 0) {
                    var item = findIcon(vm.sel.customImage);
                    vm.preview = { kind: 'img', src: item ? item.src : '' };
                    vm.selectionLabel = item ? (item.text || item.description) : ('#' + vm.sel.customImage);
                    return;
                }

                vm.preview = vm.defaultImage
                    ? { kind: 'img', src: vm.defaultImage }
                    : { kind: 'font', cls: 'fa-regular fa-square' };
                vm.selectionLabel = $.t('Default');
            }

            function findIcon(idx) {
                for (var i = 0; i < iconSet.length; i++) {
                    if (iconSet[i].idx === idx) {
                        return iconSet[i];
                    }
                }
                return null;
            }

            /* -------- sources -------- */

            function buildSources() {
                var hasUploaded = iconSet.some(function (item) {
                    return item.idx >= 100;
                });

                vm.sources = [
                    { id: 'default', kind: 'default', title: $.t('Default') },
                    { id: 'builtin', kind: 'image', title: $.t('Domoticz icons'), uploaded: false }
                ];
                if (hasUploaded) {
                    vm.sources.push({ id: 'custom', kind: 'image', title: $.t('Custom icons'), uploaded: true });
                }
                if (vm.allowGlyphs !== false) {
                    vm.sources.push({ id: 'fa', kind: 'fa', provider: 'fa', base: '', title: $.t('Font Awesome') });
                    addLibrarySources();
                }

                vm.activeSource = pickSourceFor(vm.activeSource);
            }

            function addLibrarySources() {
                dzIconPickerData.libraries().then(function (rows) {
                    if (!rows.length) {
                        return;
                    }

                    rows.forEach(function (row) {
                        var id = 'lib-' + row.Prefix;
                        var known = vm.sources.some(function (source) {
                            return source.id === id;
                        });
                        if (known) {
                            return;
                        }

                        vm.sources.push({
                            id: id,
                            kind: 'lib',
                            provider: row.Prefix,
                            /* Icon fonts are normally used as "<base> <base>-name";
                               the base class carries the font-family, the second
                               one the glyph. Prefix doubles as base unless the
                               server names one. */
                            base: row.BaseClass || row.Prefix,
                            css: row.CssFile || '',
                            title: row.Name || row.Prefix
                        });
                    });
                });
            }

            /* Open on the source the current selection came from. */
            function pickSourceFor(preferred) {
                if (preferred) {
                    var current = vm.sources.filter(function (source) {
                        return source.id === preferred.id;
                    });
                    if (current.length) {
                        return current[0];
                    }
                }

                var wanted = 'default';
                if (vm.sel.on) {
                    wanted = vm.sel.provider === 'fa' ? 'fa' : 'lib-' + vm.sel.provider;
                } else if (vm.sel.customImage >= 100) {
                    wanted = 'custom';
                } else if (vm.sel.customImage > 0) {
                    wanted = 'builtin';
                }

                var match = vm.sources.filter(function (source) {
                    return source.id === wanted;
                });

                return match.length ? match[0] : vm.sources[0];
            }

            function selectSource(source) {
                vm.activeSource = source;
                vm.query = '';
                vm.limit = PAGE_SIZE;
                vm.target = 'on';
                refresh();
            }

            /* -------- results -------- */

            function refresh() {
                var source = vm.activeSource;
                if (!source || source.kind === 'default') {
                    vm.results = [];
                    vm.matchCount = 0;
                    return;
                }

                var query = (vm.query || '').toLowerCase();
                var matches = source.kind === 'image'
                    ? imageMatches(source, query)
                    : glyphMatches(source, query);

                vm.matchCount = matches.length;
                vm.results = matches.slice(0, vm.limit);

                if (!matches.length && source.kind === 'lib' && !source.retried) {
                    /* The library stylesheet may not be on the page yet, in which
                       case there is nothing to enumerate. Load it and rescan once. */
                    source.retried = true;
                    dzIconPickerData.ensureStylesheet(source.css).then(function (added) {
                        if (added) {
                            dzIconPickerData.forgetGlyphs(source.provider);
                            refresh();
                        }
                    });
                }
            }

            function imageMatches(source, query) {
                return iconSet
                    .filter(function (item) {
                        if (source.uploaded ? item.idx < 100 : item.idx >= 100) {
                            return false;
                        }
                        if (!query) {
                            return true;
                        }
                        return (item.text + ' ' + item.description).toLowerCase().indexOf(query) !== -1;
                    })
                    .map(function (item) {
                        return {
                            key: 'img-' + item.idx,
                            kind: 'img',
                            src: item.src,
                            idx: item.idx,
                            title: item.text || item.description
                        };
                    });
            }

            function glyphMatches(source, query) {
                var style = source.kind === 'fa' ? vm.faStyle : source.base;

                return dzIconPickerData.glyphs(source.provider)
                    .filter(function (name) {
                        return !query || name.toLowerCase().indexOf(query) !== -1;
                    })
                    .map(function (name) {
                        var cls = style ? style + ' ' + name : name;
                        return {
                            key: 'glyph-' + source.provider + '-' + name,
                            kind: 'font',
                            cls: cls,
                            provider: source.provider,
                            title: cls
                        };
                    });
            }

            function showMore() {
                vm.limit += PAGE_SIZE;
                refresh();
            }

            function onStyleChange() {
                /* Restyle the glyph already chosen, so switching solid/regular
                   does not need a second click in the grid. */
                if (vm.sel.on && vm.sel.provider === 'fa') {
                    vm.sel[vm.target] = restyle(vm.sel[vm.target] || vm.sel.on);
                    emit();
                }
                refresh();
            }

            function restyle(cls) {
                var words = cls.split(' ').filter(function (word) {
                    return FA_STYLES.indexOf(word) === -1;
                });

                return vm.faStyle + ' ' + words.join(' ');
            }

            /* -------- picking -------- */

            function pick(item) {
                if (item.kind === 'img') {
                    vm.sel = { customImage: item.idx, on: '', off: '', provider: '' };
                    vm.offEnabled = false;
                    vm.target = 'on';
                    emit();
                    return;
                }

                /* The stored payload names a single provider, so the on and the
                   off glyph have to come from the same source. */
                if (vm.sel.provider && vm.sel.provider !== item.provider) {
                    vm.sel.on = '';
                    vm.sel.off = '';
                }

                vm.sel.provider = item.provider;
                vm.sel.customImage = 0;

                if (vm.target === 'off' && vm.sel.on) {
                    vm.sel.off = item.cls;
                } else {
                    vm.sel.on = item.cls;
                }

                emit();
            }

            function pickDefault() {
                vm.sel = { customImage: 0, on: '', off: '', provider: '' };
                vm.offEnabled = false;
                vm.target = 'on';
                emit();
            }

            function isSelected(item) {
                if (item.kind === 'img') {
                    return !vm.sel.on && vm.sel.customImage === item.idx;
                }
                return vm.target === 'off' ? vm.sel.off === item.cls : vm.sel.on === item.cls;
            }

            /* -------- optional off glyph -------- */

            /* dzIconService's type map holds one glyph per device type, so a
               switch resolved that way looks the same on and off. The on/off
               pair in Icon is how that is expressed, and this is where it is
               set - kept out of the way, and only once a glyph is chosen. */
            function canSetOff() {
                return !!vm.sel.on;
            }

            function onOffToggle() {
                if (vm.offEnabled) {
                    vm.target = 'off';
                } else {
                    vm.sel.off = '';
                    vm.target = 'on';
                    emit();
                }
            }

            function setTarget(target) {
                vm.target = target;
            }

            /* The Utility dialogs turn Enter into a click on their Update
               button, which would submit the dialog from the search field. */
            function onSearchKeydown(event) {
                if (event.keyCode === 13) {
                    event.preventDefault();
                    event.stopPropagation();
                }
            }

            function toggle() {
                vm.isOpen = !vm.isOpen;
                if (vm.isOpen) {
                    vm.activeSource = pickSourceFor(null);
                    vm.query = '';
                    vm.limit = PAGE_SIZE;
                    refresh();
                }
            }
        }]
    });

    /* ------------------------------------------------------------------
     * Mounting the picker from plain jQuery code.
     *
     * The Utility dialogs are jQuery UI widgets with no scope of their own,
     * so they get the picker compiled into their DOM and read the result
     * back from here when Update is pressed - the same split dzBarService
     * uses for the bar ranges.
     * ------------------------------------------------------------------ */
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

        /* container: where to render, options: { customImage, icon, defaultImage }.
           Only one dialog is ever open, so a single mount is kept and replaced. */
        function mount(container, options) {
            unmount();

            var opts = options || {};
            current.customImage = parseInt(opts.customImage, 10) || 0;
            current.icon = opts.icon || '';

            var scope = $rootScope.$new(true);
            scope.state = {
                customImage: current.customImage,
                icon: current.icon,
                defaultImage: opts.defaultImage || ''
            };
            scope.picked = function (customImage, icon) {
                current.customImage = parseInt(customImage, 10) || 0;
                current.icon = icon || '';
            };

            var element = $compile(
                '<dz-icon-picker custom-image="state.customImage" icon="state.icon"' +
                ' default-image="state.defaultImage"' +
                ' on-change="picked(customImage, icon)"></dz-icon-picker>')(scope);

            $(container).empty().append(element);
            mounted = { scope: scope, element: element };

            /* Dialogs are opened from ng-click as well as from plain jQuery
               handlers, so a digest is not guaranteed to be running. */
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
