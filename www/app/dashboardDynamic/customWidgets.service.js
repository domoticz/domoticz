define([
    'app',
    'angularAMD',
    'dashboardDynamic/widgetRegistry.service'
], function(app, angularAMD, widgetRegistry) {
    'use strict';

    /**
     * customWidgets
     *
     * Discovers and loads third-party dashboard widgets. Packages can be shipped
     * by a theme, by a Python plugin, or simply dropped into www/widgets; the
     * server finds them all and reports them through 'getcustomwidgets'.
     *
     * Loading happens in two stages. The manifest a package ships carries the
     * whole widget descriptor -- label, icon, default size, config schema -- so
     * populating the widget picker needs no third-party code to run. Only when a
     * widget is actually placed on a dashboard do we fetch its javascript and
     * register its directive, which keeps a broken package from taking the
     * picker (or the rest of the dashboard) down with it.
     *
     * See docs/custom-widgets.md for the package format and the API contract.
     */
    var SUPPORTED_API_VERSION = 1;

    // Angular directive names are ours to assign, so a package can never collide
    // with a built-in widget's element name no matter what it calls itself.
    var DIRECTIVE_PREFIX = 'ddCustom';
    var DIRECTIVE_SUFFIX = 'Widget';

    /* Turn a widget type ('ng-tour', 'ng_tour') into a directive name fragment
     * ('NgTour'). */
    function toPascalCase(type) {
        return type
            .split(/[-_]+/)
            .filter(function(part) { return part.length > 0; })
            .map(function(part) { return part.charAt(0).toUpperCase() + part.slice(1); })
            .join('');
    }

    /* Angular normalises 'ddCustomNgTourWidget' to the element <dd-custom-ng-tour-widget>. */
    function toElementTag(directiveName) {
        return directiveName.replace(/([a-z0-9])([A-Z])/g, '$1-$2').toLowerCase();
    }

    app.factory('customWidgets', ['$http', '$q', '$rootScope', function($http, $q, $rootScope) {

        var _discovery = null;      // memoised discovery promise
        var _packages = [];
        var _injectedCss = {};      // css url -> true
        var _usedDirectiveNames = {};

        /* Package base urls are webroot-relative ('styles/x/widgets/y/') or a
         * query-based asset route for plugins ('customwidgetasset?...&file=').
         * In both cases appending the asset name yields the right url. */
        function assetUrl(baseUrl, asset) {
            return asset ? baseUrl + asset : null;
        }

        function claimDirectiveName(type) {
            var base = DIRECTIVE_PREFIX + toPascalCase(type) + DIRECTIVE_SUFFIX;
            var name = base;
            // Distinct types can normalise to the same name ('ng-tour' vs 'ng_tour'),
            // so make sure each one still ends up with its own directive.
            for (var i = 2; _usedDirectiveNames[name]; i++) {
                name = base + i;
            }
            _usedDirectiveNames[name] = true;
            return name;
        }

        function injectCss(url) {
            if (!url || _injectedCss[url]) { return; }
            _injectedCss[url] = true;
            var link = document.createElement('link');
            link.rel = 'stylesheet';
            link.type = 'text/css';
            link.href = url;
            document.getElementsByTagName('head')[0].appendChild(link);
        }

        /* Register one manifest descriptor into the shared widget registry.
         * Returns true when the widget was accepted. */
        function registerDescriptor(pkg, widget) {
            var type = widget.type;

            // Built-in widgets always win: a package must not be able to
            // shadow (or hijack) a stock widget type.
            if (widgetRegistry.get(type)) {
                console.warn('customWidgets: package "' + pkg.id + '" declares widget type "' + type +
                             '" which already exists; skipping it');
                return false;
            }

            var directiveName = claimDirectiveName(type);

            var descriptor = angular.extend({}, widget, {
                custom: true,
                directiveTag: toElementTag(directiveName),
                directiveName: directiveName,
                baseUrl: pkg.baseUrl,
                entryUrl: assetUrl(pkg.baseUrl, widget.entry),
                cssUrl: assetUrl(pkg.baseUrl, widget.css),
                templateUrl: assetUrl(pkg.baseUrl, widget.template),
                // Shown in the widget picker so a user can tell where a widget came from
                provider: {
                    id: pkg.id,
                    name: pkg.name || pkg.id,
                    author: pkg.author || '',
                    version: pkg.version || '',
                    source: pkg.source,
                    origin: pkg.origin || ''
                }
            });

            widgetRegistry.register(descriptor);
            return true;
        }

        /**
         * Ask the server which widget packages are installed and register every
         * widget they declare. Memoised: the scan runs once per page load.
         *
         * Never rejects. A server that does not know the command, or a package
         * that is broken, must not stop the dashboard from rendering.
         */
        function discover() {
            if (_discovery) { return _discovery; }

            _discovery = $http.get('json.htm?type=command&param=getcustomwidgets')
                .then(function(resp) {
                    var data = resp.data || {};
                    if (data.status !== 'OK' || !angular.isArray(data.result)) {
                        return [];
                    }
                    if (data.apiVersion !== SUPPORTED_API_VERSION) {
                        console.warn('customWidgets: server speaks widget api version ' + data.apiVersion +
                                     ', this dashboard speaks ' + SUPPORTED_API_VERSION + '; skipping custom widgets');
                        return [];
                    }

                    var accepted = 0;
                    data.result.forEach(function(pkg) {
                        if (!pkg || !angular.isArray(pkg.widgets)) { return; }
                        var registered = pkg.widgets.filter(function(widget) {
                            return widget && widget.type && widget.entry && registerDescriptor(pkg, widget);
                        });
                        if (registered.length) {
                            _packages.push(pkg);
                            accepted += registered.length;
                        }
                    });

                    if (accepted) {
                        console.info('customWidgets: registered ' + accepted + ' custom widget(s) from ' +
                                     _packages.length + ' package(s)');
                    }
                    return _packages;
                })
                .catch(function() {
                    // No such command, no network, malformed reply: carry on without custom widgets.
                    return [];
                });

            return _discovery;
        }

        /* Build the Angular directive definition for a loaded widget module.
         *
         * A module either exports a directive definition object, or a factory
         * that receives its package context and returns one -- the factory form
         * exists so a widget can resolve urls inside its own package. */
        function buildDirective(descriptor, moduleExport) {
            var ddo = angular.isFunction(moduleExport)
                ? moduleExport({
                      type: descriptor.type,
                      baseUrl: descriptor.baseUrl,
                      templateUrl: descriptor.templateUrl
                  })
                : moduleExport;

            if (!ddo || !angular.isObject(ddo)) {
                throw new Error('widget module did not return a directive definition object');
            }

            // Defaults chosen to match what the built-in widgets declare, so a
            // minimal custom widget can be a controller and a template.
            var definition = angular.extend({
                scope: { widgetDef: '=', editMode: '<' },
                controllerAs: 'ctrl',
                bindToController: true
            }, ddo);

            // The wrapper renders the widget as an element, so this is not the
            // package's choice to make.
            definition.restrict = 'E';

            if (!definition.template && !definition.templateUrl && descriptor.templateUrl) {
                definition.templateUrl = descriptor.templateUrl;
            }
            if (!definition.template && !definition.templateUrl) {
                throw new Error('widget module has neither a template nor a templateUrl');
            }

            return definition;
        }

        /**
         * Fetch a custom widget's javascript and register its directive.
         * Memoised per descriptor and safe to call from several widgets at once.
         * Resolves once the widget's element name is compilable.
         */
        function load(descriptor) {
            if (descriptor.$loadPromise) { return descriptor.$loadPromise; }

            var deferred = $q.defer();

            if (!descriptor.entryUrl) {
                deferred.reject('widget has no entry script');
                descriptor.$loadPromise = deferred.promise;
                return descriptor.$loadPromise;
            }

            injectCss(descriptor.cssUrl);

            // requirejs treats an id ending in '.js' (or containing '?') as a
            // plain url, so these resolve against the document rather than the
            // 'app' module base.
            require([descriptor.entryUrl], function(moduleExport) {
                try {
                    var definition = buildDirective(descriptor, moduleExport);
                    // angularAMD caches the providers from config time, which is
                    // what lets us add a directive after the app has bootstrapped.
                    angularAMD.getCachedProvider('$compileProvider')
                        .directive(descriptor.directiveName, function() { return definition; });
                    descriptor.$loaded = true;
                    deferred.resolve(descriptor);
                } catch (err) {
                    console.error('customWidgets: "' + descriptor.type + '" failed to initialise', err);
                    deferred.reject((err && err.message) || 'widget failed to initialise');
                }
                $rootScope.$applyAsync();
            }, function(err) {
                console.error('customWidgets: could not load ' + descriptor.entryUrl, err);
                deferred.reject('could not load ' + descriptor.entryUrl);
                $rootScope.$applyAsync();
            });

            descriptor.$loadPromise = deferred.promise;
            return descriptor.$loadPromise;
        }

        return {
            discover: discover,
            load: load,
            getPackages: function() { return _packages.slice(); },
            apiVersion: SUPPORTED_API_VERSION
        };
    }]);
});
