define([
    'app',
    'dashboardDynamic/widgetRegistry.service',
    'dashboardDynamic/ddVisibility.service'
], function(app, widgetRegistry, ddVisibility) {
    'use strict';

    widgetRegistry.register({
        type:        'rss-feed',
        label:       'RSS Feed',
        description: 'Displays items from an RSS or Atom feed with configurable item count and auto-refresh',
        category:    'Information',
        icon:        'fa-solid fa-rss',
        defaultW:    4,
        defaultH:    4,
        minW:        3,
        minH:        2,
        maxW:        12,
        maxH:        10,
        transparentBackground: true,
        configSchema: [
            {
                key:     'showBackground',
                type:    'boolean',
                label:   'Show panel background',
                default: true
            },
            {
                key:      'feedUrl',
                type:     'text',
                label:    'Feed URL (RSS or Atom)',
                required: true
            },
            {
                key:      'title',
                type:     'text',
                label:    'Title (optional, defaults to feed title)',
                required: false
            },
            {
                key:      'maxItems',
                type:     'number',
                step:     1,
                label:    'Max items (1–20)',
                default:  5,
                min:      1,
                max:      20
            },
            {
                key:      'refreshInterval',
                type:     'number',
                step:     1,
                label:    'Refresh interval (seconds)',
                default:  300,
                min:      30
            },
            {
                key:      'showImages',
                type:     'boolean',
                label:    'Show images/thumbnails',
                default:  true
            },
            {
                key:      'showDate',
                type:     'boolean',
                label:    'Show item date',
                default:  true
            },
            {
                key:      'openInNewTab',
                type:     'boolean',
                label:    'Open links in new tab',
                default:  true
            }
        ]
    });

    app.directive('ddRssFeedWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/rss-feed.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', '$q', 'ddVisibility', function($scope, $http, $interval, $q, ddVisibility) {
                var ctrl = this;

                ctrl.loading     = false;
                ctrl.loadError   = false;
                ctrl.feedTitle   = '';
                ctrl.items       = [];
                ctrl.isSingleItem = false;

                var cancelToken  = null;
                var refreshTimer = null;

                function stripHtml(html) {
                    if (!html) { return ''; }
                    var div = document.createElement('div');
                    div.innerHTML = html.replace(/<[^>]+>/g, ' ');
                    return (div.textContent || div.innerText || '').replace(/\s+/g, ' ').trim();
                }

                function relativeTime(dateStr) {
                    if (!dateStr) { return ''; }
                    var now  = Date.now();
                    var then = new Date(dateStr).getTime();
                    if (isNaN(then)) { return dateStr; }
                    var diff = Math.floor((now - then) / 1000);
                    if (diff < 60)   { return 'just now'; }
                    if (diff < 3600) { return Math.floor(diff / 60) + 'm ago'; }
                    if (diff < 86400){ return Math.floor(diff / 3600) + 'h ago'; }
                    return Math.floor(diff / 86400) + 'd ago';
                }

                ctrl.relativeTime = relativeTime;

                ctrl.linkTarget = function() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    return (cfg.openInNewTab !== false) ? '_blank' : '_self';
                };

                function applyItems(items, feedTitle, cfg, maxItems) {
                    var cfgTitle = cfg.title;
                    ctrl.feedTitle = cfgTitle || feedTitle || '';
                    ctrl.items = items.slice(0, maxItems);
                }

                function parseXml(xmlStr, cfg, maxItems) {
                    try {
                        var doc = new DOMParser().parseFromString(xmlStr, 'text/xml');
                        var rootName = doc.documentElement && doc.documentElement.localName;
                        var isAtom = (rootName === 'feed');
                        var items = [];
                        var feedTitle = '';

                        if (isAtom) {
                            var titleEl = doc.querySelector('feed > title');
                            feedTitle = titleEl ? titleEl.textContent : '';
                            var entries = doc.querySelectorAll('entry');
                            Array.prototype.forEach.call(entries, function(e) {
                                var linkEl = e.querySelector('link');
                                items.push({
                                    title:       (e.querySelector('title') || {textContent:''}).textContent,
                                    link:        linkEl ? (linkEl.getAttribute('href') || linkEl.textContent || '#') : '#',
                                    description: stripHtml((e.querySelector('summary') || e.querySelector('content') || {textContent:''}).textContent),
                                    pubDate:     (e.querySelector('updated') || e.querySelector('published') || {textContent:''}).textContent,
                                    thumbnail:   ''
                                });
                            });
                        } else {
                            var chan = doc.querySelector('channel');
                            if (!chan) { ctrl.loadError = true; return; }
                            feedTitle = (chan.querySelector('title') || {textContent:''}).textContent;
                            var nodes = doc.querySelectorAll('item');
                            Array.prototype.forEach.call(nodes, function(item) {
                                var thumb = '';
                                var enc = item.querySelector('enclosure');
                                if (enc && (enc.getAttribute('type') || '').indexOf('image') >= 0) {
                                    thumb = enc.getAttribute('url') || '';
                                }
                                var mt = item.getElementsByTagNameNS('http://search.yahoo.com/mrss/', 'thumbnail')[0];
                                if (mt) { thumb = mt.getAttribute('url') || ''; }
                                var linkEl = item.querySelector('link');
                                var link = (linkEl && (linkEl.textContent || (linkEl.nextSibling && linkEl.nextSibling.nodeValue) || linkEl.getAttribute('href'))) || '#';
                                items.push({
                                    title:       (item.querySelector('title') || {textContent:''}).textContent,
                                    link:        link.trim() || '#',
                                    description: stripHtml((item.querySelector('description') || {textContent:''}).textContent),
                                    pubDate:     (item.querySelector('pubDate') || {textContent:''}).textContent,
                                    thumbnail:   thumb
                                });
                            });
                        }
                        applyItems(items, feedTitle, cfg, maxItems);
                    } catch(e) {
                        ctrl.loadError = true;
                    }
                }

                function fetchViaProxy(cfg, maxItems, proxyIndex) {
                    // Only Domoticz's own server-side proxy: public CORS proxies now
                    // answer 401/5xx, and the feed URL should not leave the network anyway.
                    var proxies = [
                        '__domoticz__'
                    ];
                    var idx = proxyIndex || 0;
                    if (idx >= proxies.length) {
                        ctrl.loading   = false;
                        ctrl.loadError = true;
                        return;
                    }
                    var url, useServerProxy = false;
                    if (proxies[idx] === '__domoticz__') {
                        url = 'json.htm?type=command&param=fetchurl&url=' + encodeURIComponent(cfg.feedUrl);
                        useServerProxy = true;
                    } else {
                        url = proxies[idx] + encodeURIComponent(cfg.feedUrl);
                    }
                    var httpOpts = { timeout: cancelToken ? cancelToken.promise : undefined };
                    if (!useServerProxy) {
                        httpOpts.transformResponse = function(data) { return data; };
                    }
                    return $http.get(url, httpOpts).then(function(resp) {
                        ctrl.loading = false;
                        var xml;
                        if (useServerProxy) {
                            // Domoticz fetchurl returns JSON; tolerate a string body as well.
                            var d = resp.data;
                            if (typeof d === 'string') {
                                try { d = JSON.parse(d); } catch(e) { d = null; }
                            }
                            if (!d || d.status !== 'OK') {
                                fetchViaProxy(cfg, maxItems, idx + 1);
                                return;
                            }
                            xml = d.data;
                        } else {
                            xml = (typeof resp.data === 'string') ? resp.data : (resp.data && resp.data.contents);
                        }
                        if (!xml) { fetchViaProxy(cfg, maxItems, idx + 1); return; }
                        parseXml(xml, cfg, maxItems);
                    }).catch(function(err) {
                        if (err && err.status === -1) { return; } // cancelled — do not fall through
                        fetchViaProxy(cfg, maxItems, idx + 1);
                    });
                }

                ctrl.load = function() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    if (!cfg.feedUrl) { return; }

                    var maxItems = parseInt(cfg.maxItems, 10) || 5;
                    if (maxItems < 1)  { maxItems = 1; }
                    if (maxItems > 20) { maxItems = 20; }

                    ctrl.isSingleItem = (maxItems === 1);
                    ctrl.loading      = true;
                    ctrl.loadError    = false;

                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();

                    // Fetched through Domoticz's own server-side fetchurl (same origin, no third party).
                    fetchViaProxy(cfg, maxItems);
                };

                function stopTimer() {
                    if (refreshTimer) { $interval.cancel(refreshTimer); refreshTimer = null; }
                }

                function startTimer() {
                    stopTimer();
                    var cfg      = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var interval = parseInt(cfg.refreshInterval, 10) || 300;
                    if (interval < 30) { interval = 30; }
                    refreshTimer = $interval(ctrl.load, interval * 1000);
                }

                $scope.$on('dd:page:hidden',  function() { stopTimer(); });
                $scope.$on('dd:page:visible', function() { ctrl.load(); startTimer(); });

                $scope.$on('$destroy', function() {
                    if (cancelToken)  { cancelToken.resolve(); cancelToken = null; }
                    if (refreshTimer) { $interval.cancel(refreshTimer); refreshTimer = null; }
                });

                $scope.$on('dd:widget:refresh', ctrl.load);

                $scope.$watch(
                    function() {
                        return ctrl.widgetDef && ctrl.widgetDef.config && ctrl.widgetDef.config.feedUrl;
                    },
                    function(val, old) {
                        if (val !== old) {
                            ctrl.load();
                            startTimer();
                        }
                    }
                );

                ctrl.$onInit = function() {
                    ctrl.load();
                    if (!ddVisibility.isHidden()) { startTimer(); }
                };
            }]
        };
    }]);
});
