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
            },
            { key: 'showBackground', type: 'boolean', label: 'Show panel background', default: true }
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

                // ── ICS field helpers ─────────────────────────────────────
                var WEEKDAY_NUM = { SU: 0, MO: 1, TU: 2, WE: 3, TH: 4, FR: 5, SA: 6 };

                function unescapeICSText(val) {
                    return val.replace(/\\n/gi, ' ').replace(/\\,/g, ',').replace(/\\;/g, ';').replace(/\\\\/g, '\\');
                }

                function addDays(date, n) {
                    var d = new Date(date.getTime());
                    d.setDate(d.getDate() + n);
                    return d;
                }

                // Local-time key used to match EXDATE / RECURRENCE-ID values
                // against generated occurrence starts
                function occKey(date, dateOnly) {
                    var k = String(date.getFullYear()) +
                            String(date.getMonth() + 1).padStart(2, '0') +
                            String(date.getDate()).padStart(2, '0');
                    if (dateOnly) { return k; }
                    return k + 'T' +
                           String(date.getHours()).padStart(2, '0') +
                           String(date.getMinutes()).padStart(2, '0') +
                           String(date.getSeconds()).padStart(2, '0');
                }

                function icsValueToKey(raw) {
                    if (!raw) { return null; }
                    var val = raw.replace(/^[^:]+:/, '').replace(/\r/g, '').trim();
                    var d = parseICSDate(val);
                    if (!d) { return null; }
                    return occKey(d, /^\d{8}$/.test(val));
                }

                function parseRRule(str) {
                    var rule = {};
                    str.split(';').forEach(function(part) {
                        var idx = part.indexOf('=');
                        if (idx > 0) { rule[part.substring(0, idx).toUpperCase()] = part.substring(idx + 1); }
                    });
                    return rule;
                }

                // ── Recurrence expansion ──────────────────────────────────
                // Expands an RRULE series into concrete occurrences inside
                // [windowStart, windowEnd]. Supports the RRULE features that
                // calendar exports actually emit: FREQ=DAILY/WEEKLY/MONTHLY/
                // YEARLY, INTERVAL, COUNT, UNTIL, BYDAY (weekly), BYMONTHDAY.
                // EXDATE and RECURRENCE-ID overrides suppress their occurrence.
                function expandSeries(ev, rule, overrides, windowStart, windowEnd, out) {
                    var freq = (rule.FREQ || '').toUpperCase();
                    if (freq !== 'DAILY' && freq !== 'WEEKLY' && freq !== 'MONTHLY' && freq !== 'YEARLY') {
                        out.push({ title: ev.title, start: ev.start, end: ev.end, allDay: ev.allDay, location: ev.location });
                        return;
                    }
                    var interval = parseInt(rule.INTERVAL, 10) || 1;
                    if (interval < 1) { interval = 1; }
                    var count   = rule.COUNT ? parseInt(rule.COUNT, 10) : 0;
                    var until   = rule.UNTIL ? parseICSDate(rule.UNTIL) : null;
                    var durMs   = (ev.end && ev.start) ? (ev.end.getTime() - ev.start.getTime()) : 0;
                    var maxIter = 5000;
                    var counted = 0;
                    var iter    = 0;

                    // Occurrences are generated in chronological order;
                    // returns false when the series is exhausted
                    function emitOccurrence(occStart) {
                        if (until && occStart > until) { return false; }
                        if (count) {
                            counted++;
                            if (counted > count) { return false; }
                        }
                        if (occStart > windowEnd) { return false; }
                        if (occStart >= windowStart) {
                            var key = occKey(occStart, ev.allDay);
                            if (!ev.exKeys[key] && !overrides[ev.uid + '|' + key]) {
                                out.push({
                                    title:    ev.title,
                                    start:    occStart,
                                    end:      durMs ? new Date(occStart.getTime() + durMs) : occStart,
                                    allDay:   ev.allDay,
                                    location: ev.location
                                });
                            }
                        }
                        return true;
                    }

                    if (freq === 'DAILY') {
                        var cur = new Date(ev.start.getTime());
                        // Without COUNT the pre-window occurrences don't matter — jump ahead
                        if (!count && windowStart > cur) {
                            var behind = Math.floor((windowStart.getTime() - cur.getTime()) / (86400000 * interval)) - 1;
                            if (behind > 0) { cur = addDays(cur, behind * interval); }
                        }
                        while (iter++ < maxIter) {
                            if (!emitOccurrence(cur)) { break; }
                            cur = addDays(cur, interval);
                        }
                        return;
                    }

                    if (freq === 'WEEKLY') {
                        var dayNums = [];
                        if (rule.BYDAY) {
                            rule.BYDAY.split(',').forEach(function(tok) {
                                tok = tok.trim().toUpperCase().replace(/^[+-]?\d+/, '');
                                if (WEEKDAY_NUM.hasOwnProperty(tok)) { dayNums.push(WEEKDAY_NUM[tok]); }
                            });
                        }
                        if (!dayNums.length) { dayNums = [ev.start.getDay()]; }
                        // Day offsets inside a Monday-based week, chronological
                        var offsets = dayNums.map(function(d) { return (d + 6) % 7; }).sort(function(a, b) { return a - b; });
                        var weekStart = addDays(ev.start, -((ev.start.getDay() + 6) % 7));
                        if (!count && windowStart > weekStart) {
                            var weeksBehind = Math.floor((windowStart.getTime() - weekStart.getTime()) / (86400000 * 7 * interval)) - 1;
                            if (weeksBehind > 0) { weekStart = addDays(weekStart, weeksBehind * interval * 7); }
                        }
                        var stop = false;
                        while (!stop && iter < maxIter) {
                            for (var i = 0; i < offsets.length; i++) {
                                iter++;
                                var occ = addDays(weekStart, offsets[i]);
                                if (occ < ev.start) { continue; }
                                if (!emitOccurrence(occ)) { stop = true; break; }
                            }
                            weekStart = addDays(weekStart, interval * 7);
                        }
                        return;
                    }

                    if (freq === 'MONTHLY') {
                        var monthDays = [];
                        if (rule.BYMONTHDAY) {
                            rule.BYMONTHDAY.split(',').forEach(function(v) {
                                var n = parseInt(v, 10);
                                if (!isNaN(n) && n !== 0) { monthDays.push(n); }
                            });
                        }
                        if (!monthDays.length) { monthDays = [ev.start.getDate()]; }
                        var mIdx  = 0;
                        var stopM = false;
                        while (!stopM && iter < maxIter) {
                            var monthBase = new Date(ev.start.getFullYear(), ev.start.getMonth() + mIdx * interval, 1);
                            var lastDay   = new Date(monthBase.getFullYear(), monthBase.getMonth() + 1, 0).getDate();
                            var days = [];
                            monthDays.forEach(function(n) {
                                var dnum = n > 0 ? n : lastDay + n + 1;   // negative = counted from month end
                                if (dnum >= 1 && dnum <= lastDay) { days.push(dnum); }
                            });
                            days.sort(function(a, b) { return a - b; });
                            for (var k = 0; k < days.length; k++) {
                                if (k > 0 && days[k] === days[k - 1]) { continue; }   // e.g. BYMONTHDAY=31,-1 in a 31-day month
                                iter++;
                                var mOcc = new Date(monthBase.getFullYear(), monthBase.getMonth(), days[k],
                                                    ev.start.getHours(), ev.start.getMinutes(), ev.start.getSeconds());
                                if (mOcc < ev.start) { continue; }
                                if (!emitOccurrence(mOcc)) { stopM = true; break; }
                            }
                            mIdx++;
                        }
                        return;
                    }

                    // YEARLY
                    var yIdx = 0;
                    while (iter++ < maxIter) {
                        var yOcc = new Date(ev.start.getFullYear() + yIdx * interval, ev.start.getMonth(), ev.start.getDate(),
                                            ev.start.getHours(), ev.start.getMinutes(), ev.start.getSeconds());
                        yIdx++;
                        if (yOcc.getMonth() !== ev.start.getMonth()) { continue; }   // Feb 29 in a non-leap year
                        if (!emitOccurrence(yOcc)) { break; }
                    }
                }

                function parseICS(text) {
                    // Unfold continuation lines (lines starting with space/tab continue previous)
                    var unfolded = text.replace(/\r\n[ \t]/g, '').replace(/\r\n/g, '\n').replace(/\r/g, '\n');
                    var blocks = unfolded.split('BEGIN:VEVENT');
                    var singles   = [];
                    var recurring = [];
                    var overrides = {};   // uid|occurrence-key of occurrences replaced by their own VEVENT

                    blocks.slice(1).forEach(function(block) {
                        var getField = function(name) {
                            // Match property name optionally followed by params then colon
                            var re = new RegExp('(?:^|\n)' + name + '[^:\n]*:([^\n]+)', 'i');
                            var m  = block.match(re);
                            return m ? m[1].trim() : null;
                        };
                        var getFieldAll = function(name) {
                            var re = new RegExp('(?:^|\n)' + name + '[^:\n]*:([^\n]+)', 'ig');
                            var vals = [];
                            var m;
                            while ((m = re.exec(block)) !== null) { vals.push(m[1].trim()); }
                            return vals;
                        };

                        var dtStartRaw = getField('DTSTART');
                        var dtEndRaw   = getField('DTEND');
                        var summary    = getField('SUMMARY') || '(No title)';
                        var location   = getField('LOCATION');
                        var uid        = getField('UID') || '';
                        var rruleRaw   = getField('RRULE');
                        var recurIdRaw = getField('RECURRENCE-ID');
                        var status     = getField('STATUS');

                        // Unescape ICS text
                        summary = unescapeICSText(summary);
                        if (location) { location = unescapeICSText(location); }

                        // Determine if all-day: DTSTART may have VALUE=DATE param or be bare YYYYMMDD
                        var isAllDay = dtStartRaw && dtStartRaw.indexOf('VALUE=DATE') !== -1;
                        if (!isAllDay && dtStartRaw) {
                            var rawVal = dtStartRaw.replace(/^[^:]*:/, '').trim();
                            isAllDay = /^\d{8}$/.test(rawVal);
                        }

                        var startDate = parseICSDate(dtStartRaw);
                        var endDate   = dtEndRaw ? parseICSDate(dtEndRaw) : startDate;
                        if (!startDate) { return; }

                        // A VEVENT with RECURRENCE-ID replaces one occurrence of its
                        // series: suppress the generated occurrence, show this instead
                        if (recurIdRaw && uid) {
                            overrides[uid + '|' + icsValueToKey(recurIdRaw)] = true;
                        }
                        if (status && status.toUpperCase().indexOf('CANCELLED') !== -1) { return; }

                        var exKeys = {};
                        getFieldAll('EXDATE').forEach(function(line) {
                            line.split(',').forEach(function(v) {
                                var exKey = icsValueToKey(v);
                                if (exKey) { exKeys[exKey] = true; }
                            });
                        });

                        var ev = {
                            uid:      uid,
                            title:    summary,
                            start:    startDate,
                            end:      endDate,
                            allDay:   isAllDay,
                            location: location,
                            exKeys:   exKeys
                        };
                        if (rruleRaw && !recurIdRaw) {
                            ev.rrule = parseRRule(rruleRaw);
                            recurring.push(ev);
                        } else {
                            singles.push(ev);
                        }
                    });

                    var events = [];
                    singles.forEach(function(ev) {
                        events.push({ title: ev.title, start: ev.start, end: ev.end, allDay: ev.allDay, location: ev.location });
                    });
                    if (recurring.length) {
                        var now         = new Date();
                        var windowStart = new Date(now.getFullYear(), now.getMonth(), now.getDate() - 1);
                        var windowEnd   = addDays(windowStart, 61);   // 60-day lookahead from today
                        recurring.forEach(function(ev) {
                            expandSeries(ev, ev.rrule, overrides, windowStart, windowEnd, events);
                        });
                    }
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
