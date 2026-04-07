define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'google-calendar',
        label:       'Google Calendar',
        description: 'Displays upcoming events from a public ICS URL or Google Calendar API endpoint, grouped by day',
        category:    'Information',
        icon:        'fa-solid fa-calendar-days',
        defaultW:    4,
        defaultH:    4,
        minW:        3,
        minH:        3,
        maxW:        8,
        maxH:        8,
        transparentBackground: true,
        configSchema: [
            {
                key:      'calendarUrl',
                type:     'text',
                label:    'Calendar URL (public ICS or Google Calendar API)',
                required: true
            },
            {
                key:      'title',
                type:     'text',
                label:    'Title (optional)',
                required: false,
                default:  'Calendar'
            },
            {
                key:     'maxEvents',
                type:    'number',
                label:   'Max events to show',
                default: 7,
                min:     1,
                max:     50
            },
            {
                key:     'showTime',
                type:    'boolean',
                label:   'Show event time',
                default: true
            },
            {
                key:     'refreshInterval',
                type:    'number',
                label:   'Refresh interval (seconds)',
                default: 900,
                min:     60
            }
        ]
    });

    app.directive('ddGoogleCalendarWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/google-calendar.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', '$q', function($scope, $http, $interval, $q) {
                var ctrl = this;

                ctrl.loading          = false;
                ctrl.loadError        = false;
                ctrl.errorMessage     = '';
                ctrl.groups           = [];
                ctrl.title            = 'Calendar';
                ctrl.currentMonthYear = '';
                ctrl.showTime         = true;

                var cancelToken  = null;
                var refreshTimer = null;

                // ── ICS date parser ───────────────────────────────────────
                function parseICSDate(str) {
                    if (!str) { return null; }
                    // Strip VALUE=DATE: or TZID=... prefix after semicolon
                    var raw = str.replace(/^[^:]+:/, '').replace(/\r/g, '');
                    if (raw.length === 8) {
                        // All-day: YYYYMMDD
                        var y = parseInt(raw.substring(0, 4), 10);
                        var m = parseInt(raw.substring(4, 6), 10) - 1;
                        var d = parseInt(raw.substring(6, 8), 10);
                        return new Date(y, m, d);
                    }
                    // Date-time: YYYYMMDDTHHmmss[Z]
                    if (raw.length >= 15 && raw.charAt(8) === 'T') {
                        var yr  = parseInt(raw.substring(0, 4), 10);
                        var mo  = parseInt(raw.substring(4, 6), 10) - 1;
                        var dy  = parseInt(raw.substring(6, 8), 10);
                        var hr  = parseInt(raw.substring(9, 11), 10);
                        var mn  = parseInt(raw.substring(11, 13), 10);
                        var sc  = parseInt(raw.substring(13, 15), 10);
                        var utc = raw.charAt(15) === 'Z';
                        if (utc) {
                            return new Date(Date.UTC(yr, mo, dy, hr, mn, sc));
                        }
                        return new Date(yr, mo, dy, hr, mn, sc);
                    }
                    return null;
                }

                function parseICS(text) {
                    var events = [];
                    // Unfold continuation lines (lines starting with space/tab continue previous)
                    var unfolded = text.replace(/\r\n[ \t]/g, '').replace(/\r\n/g, '\n').replace(/\r/g, '\n');
                    var blocks = unfolded.split('BEGIN:VEVENT');
                    blocks.slice(1).forEach(function(block) {
                        var getField = function(name) {
                            // Match property name optionally followed by params then colon
                            var re = new RegExp('(?:^|\n)' + name + '[^:\n]*:([^\n]+)', 'i');
                            var m  = block.match(re);
                            return m ? m[1].trim() : null;
                        };

                        var dtStartRaw = getField('DTSTART');
                        var dtEndRaw   = getField('DTEND');
                        var summary    = getField('SUMMARY') || '(No title)';
                        var location   = getField('LOCATION');

                        // Unescape ICS text
                        summary = summary.replace(/\\n/g, ' ').replace(/\\,/g, ',').replace(/\\\\/g, '\\');
                        if (location) {
                            location = location.replace(/\\n/g, ' ').replace(/\\,/g, ',').replace(/\\\\/g, '\\');
                        }

                        // Determine if all-day: DTSTART may have VALUE=DATE param or be bare YYYYMMDD
                        var isAllDay = dtStartRaw && dtStartRaw.indexOf('VALUE=DATE') !== -1;
                        if (!isAllDay && dtStartRaw) {
                            var rawVal = dtStartRaw.replace(/^[^:]*:/, '').trim();
                            isAllDay = /^\d{8}$/.test(rawVal);
                        }

                        var startDate = parseICSDate(dtStartRaw);
                        var endDate   = dtEndRaw ? parseICSDate(dtEndRaw) : startDate;

                        if (startDate) {
                            events.push({
                                title:    summary,
                                start:    startDate,
                                end:      endDate,
                                allDay:   isAllDay,
                                location: location
                            });
                        }
                    });
                    return events;
                }

                // ── Date helpers ──────────────────────────────────────────
                function toDateKey(date) {
                    return date.getFullYear() + '-' +
                           String(date.getMonth() + 1).padStart(2, '0') + '-' +
                           String(date.getDate()).padStart(2, '0');
                }

                function dayLabel(date) {
                    var today    = new Date();
                    var tomorrow = new Date(today);
                    tomorrow.setDate(today.getDate() + 1);
                    if (toDateKey(date) === toDateKey(today))    { return 'Today'; }
                    if (toDateKey(date) === toDateKey(tomorrow)) { return 'Tomorrow'; }
                    var days  = ['Sunday','Monday','Tuesday','Wednesday','Thursday','Friday','Saturday'];
                    var months = ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];
                    var diff  = Math.floor((date - today) / 86400000);
                    if (diff < 7) {
                        return days[date.getDay()];
                    }
                    return days[date.getDay()] + ', ' + months[date.getMonth()] + ' ' + date.getDate();
                }

                function formatTime(date) {
                    var h = date.getHours();
                    var m = date.getMinutes();
                    var ampm = h >= 12 ? 'PM' : 'AM';
                    h = h % 12;
                    if (h === 0) { h = 12; }
                    return h + ':' + String(m).padStart(2, '0') + ' ' + ampm;
                }

                function monthYearLabel() {
                    var now    = new Date();
                    var months = ['January','February','March','April','May','June',
                                  'July','August','September','October','November','December'];
                    return months[now.getMonth()] + ' ' + now.getFullYear();
                }

                // ── Group events into ctrl.groups ─────────────────────────
                function buildGroups(events) {
                    var cfg       = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var maxEvents = parseInt(cfg.maxEvents, 10) || 7;
                    if (maxEvents < 1)  { maxEvents = 1; }
                    if (maxEvents > 50) { maxEvents = 50; }

                    var now = new Date();
                    // Filter: keep events that have not ended yet (or start in the future)
                    var upcoming = events.filter(function(ev) {
                        var compareDate = ev.allDay ? ev.start : (ev.end || ev.start);
                        return compareDate >= now || toDateKey(ev.start) === toDateKey(now);
                    });

                    // Sort ascending by start
                    upcoming.sort(function(a, b) { return a.start - b.start; });

                    // Limit total events
                    upcoming = upcoming.slice(0, maxEvents);

                    // Group by day
                    var groupMap  = {};
                    var groupKeys = [];
                    upcoming.forEach(function(ev) {
                        var key = toDateKey(ev.start);
                        if (!groupMap[key]) {
                            groupMap[key] = { date: ev.start, label: dayLabel(ev.start), events: [] };
                            groupKeys.push(key);
                        }
                        var isPast = !ev.allDay && ev.end && ev.end < now;
                        groupMap[key].events.push({
                            title:   ev.title,
                            allDay:  ev.allDay,
                            timeStr: ev.allDay ? '' : formatTime(ev.start),
                            isPast:  !!isPast,
                            location: ev.location
                        });
                    });

                    ctrl.groups = groupKeys.map(function(k) { return groupMap[k]; });
                }

                // ── Extract events from JSON API response ─────────────────
                function parseJsonApi(data) {
                    var items = data.items || [];
                    return items.map(function(item) {
                        var startRaw = item.start && (item.start.dateTime || item.start.date);
                        var endRaw   = item.end   && (item.end.dateTime   || item.end.date);
                        var isAllDay = !!(item.start && item.start.date && !item.start.dateTime);
                        var start    = startRaw ? new Date(startRaw) : null;
                        var end      = endRaw   ? new Date(endRaw)   : null;
                        if (!start || isNaN(start.getTime())) { return null; }
                        return {
                            title:    item.summary || '(No title)',
                            start:    start,
                            end:      end,
                            allDay:   isAllDay,
                            location: item.location || null
                        };
                    }).filter(Boolean);
                }

                // ── Fetch with proxy fallback ─────────────────────────────
                var ICS_PROXIES = [
                    null,           // try direct first (works for same-origin or CORS-enabled sources)
                    '__domoticz__', // server-side proxy via Domoticz backend
                    'https://corsproxy.io/?url=',
                    'https://api.codetabs.com/v1/proxy?quest='
                ];

                function fetchAndParse(cfg, proxyIndex) {
                    var idx = proxyIndex || 0;
                    if (idx >= ICS_PROXIES.length) {
                        ctrl.loading      = false;
                        ctrl.loadError    = true;
                        ctrl.errorMessage = 'Could not load calendar (CORS or network error).';
                        return;
                    }

                    var url, useServerProxy = false;
                    if (!ICS_PROXIES[idx]) {
                        url = cfg.calendarUrl;
                    } else if (ICS_PROXIES[idx] === '__domoticz__') {
                        url = 'json.htm?type=command&param=fetchurl&url=' + encodeURIComponent(cfg.calendarUrl);
                        useServerProxy = true;
                    } else {
                        url = ICS_PROXIES[idx] + encodeURIComponent(cfg.calendarUrl);
                    }
                    var isICS = /\.ics(\?.*)?$/i.test(cfg.calendarUrl) || idx > 0 || useServerProxy;

                    $http.get(url, {
                        timeout:           cancelToken ? cancelToken.promise : undefined,
                        transformResponse: useServerProxy ? undefined : [function(data) { return data; }]
                    }).then(function(resp) {
                        ctrl.loading = false;
                        var text;
                        if (useServerProxy) {
                            if (!resp.data || resp.data.status !== 'OK') {
                                fetchAndParse(cfg, idx + 1);
                                return;
                            }
                            text = resp.data.data;
                        } else {
                            text = typeof resp.data === 'string' ? resp.data : JSON.stringify(resp.data);
                        }
                        var events;
                        if (isICS || text.indexOf('BEGIN:VCALENDAR') !== -1) {
                            events = parseICS(text);
                        } else {
                            try {
                                var data = JSON.parse(text);
                                events = parseJsonApi(data);
                            } catch(e) {
                                ctrl.loadError    = true;
                                ctrl.errorMessage = 'Could not parse calendar data.';
                                return;
                            }
                        }
                        if (!events.length && idx === 0) {
                            // Direct fetch returned empty — might be a CORS silent failure; try proxy
                            fetchAndParse(cfg, idx + 1);
                            return;
                        }
                        buildGroups(events);
                    }).catch(function(err) {
                        // status -1 means request was cancelled (widget destroyed) — only stop if on a proxy attempt
                        if (err && err.status === -1 && idx > 0) {
                            ctrl.loading = false;
                            return;
                        }
                        // Try next proxy (handles CORS, network errors, 4xx on direct fetch)
                        fetchAndParse(cfg, idx + 1);
                    });
                }

                // ── Main load function ────────────────────────────────────
                ctrl.load = function() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    if (!cfg.calendarUrl) { return; }

                    ctrl.loading   = true;
                    ctrl.loadError = false;
                    ctrl.errorMessage = '';

                    ctrl.title    = cfg.title || 'Calendar';
                    ctrl.showTime = (cfg.showTime !== false);
                    ctrl.currentMonthYear = monthYearLabel();

                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();

                    fetchAndParse(cfg, 0);
                };

                function scheduleRefresh() {
                    if (refreshTimer) { $interval.cancel(refreshTimer); }
                    var cfg      = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var interval = parseInt(cfg.refreshInterval, 10) || 900;
                    if (interval < 60) { interval = 60; }
                    refreshTimer = $interval(ctrl.load, interval * 1000);
                }

                $scope.$on('$destroy', function() {
                    if (cancelToken)  { cancelToken.resolve(); cancelToken = null; }
                    if (refreshTimer) { $interval.cancel(refreshTimer); refreshTimer = null; }
                });

                $scope.$on('dd:widget:refresh', ctrl.load);

                $scope.$watch(
                    function() {
                        return ctrl.widgetDef && ctrl.widgetDef.config && ctrl.widgetDef.config.calendarUrl;
                    },
                    function(val, old) {
                        if (val !== old) {
                            ctrl.load();
                            scheduleRefresh();
                        }
                    }
                );

                ctrl.$onInit = function() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    ctrl.title    = cfg.title || 'Calendar';
                    ctrl.showTime = (cfg.showTime !== false);
                    ctrl.currentMonthYear = monthYearLabel();
                    ctrl.load();
                    scheduleRefresh();
                };
            }]
        };
    }]);
});
