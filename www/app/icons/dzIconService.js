define(['app'], function (app) {
    'use strict';

    /* ------------------------------------------------------------------
     * UI chrome icons: PNG filename -> Font Awesome class.
     *
     * These are the fixed images used by the navbar, the setup menus, the
     * data table action buttons, the trend arrows and the favourite stars.
     * They are keyed by the path as it appears in the templates, and also
     * looked up by bare file name so 'images/delete.png', './delete.png'
     * and 'delete.png' all resolve.
     *
     * Only the glyph classes are listed. Colour is deliberately left to
     * the stylesheet: a class attribute is all the resolver hands out.
     * ------------------------------------------------------------------ */
    var CHROME_ICONS = {
        /* Main navbar tabs */
        'images/desktop.png':      'fa-solid fa-gauge',
        'images/house.png':        'fa-solid fa-house',
        'images/lightbulb.png':    'fa-solid fa-lightbulb',
        'images/lightbulboff.png': 'fa-regular fa-lightbulb',
        'images/scenes.png':       'fa-solid fa-layer-group',
        'images/temperature.png':  'fa-solid fa-temperature-half',
        'images/rain.png':         'fa-solid fa-cloud-rain',
        'images/utility.png':      'fa-solid fa-bolt',

        /* Setup dropdown */
        'images/setup.png':        'fa-solid fa-gear',
        'images/hardware.png':     'fa-solid fa-microchip',
        'images/devices.png':      'fa-solid fa-sliders',
        'images/energy.png':       'fa-solid fa-charging-station',
        'images/users.png':        'fa-solid fa-users',
        'images/update.png':       'fa-solid fa-download',
        'images/log.png':          'fa-solid fa-terminal',
        'images/about.png':        'fa-solid fa-circle-info',
        'images/logout.png':       'fa-solid fa-right-from-bracket',
        'images/restart.png':      'fa-solid fa-rotate-right',
        'images/shutdown.png':     'fa-solid fa-power-off',

        /* More options sub-menu */
        'images/events.png':       'fa-solid fa-code',
        'images/customicons.png':  'fa-solid fa-icons',
        'images/variables.png':    'fa-solid fa-list',
        'images/contact.png':      'fa-solid fa-share-nodes',
        'images/camera-web.png':   'fa-solid fa-video',
        'images/security.png':     'fa-solid fa-shield-halved',
        'images/notification.png': 'fa-solid fa-bell',
        'images/floorplans.png':   'fa-solid fa-map',
        'images/report.png':       'fa-solid fa-chart-bar',

        /* Action icons (data tables, edit forms) */
        'images/delete.png':       'fa-solid fa-trash-can',
        'images/rename.png':       'fa-solid fa-pen-to-square',
        'images/add.png':          'fa-solid fa-plus',

        /* Dashboard / card inline icons */
        'images/webcam.png':       'fa-solid fa-video',
        'images/override.png':     'fa-solid fa-sliders',
        'images/next.png':         'fa-solid fa-chevron-right',
        'images/capture.png':      'fa-solid fa-camera',
        'images/location.png':     'fa-solid fa-location-dot',

        /* Trend arrows (inline in the value / status text) */
        'images/arrow_up.png':     'fa-solid fa-arrow-trend-up',
        'images/arrow_down.png':   'fa-solid fa-arrow-trend-down',
        'images/arrow_stable.png': 'fa-solid fa-right-long',
        'images/arrow_unk.png':    'fa-solid fa-question',

        /* Blinds stop (no 48 in the file name) */
        'images/blindsstop.png':   'fa-solid fa-stop',

        /* Table row ordering and set-unused */
        'images/up.png':           'fa-solid fa-arrow-up',
        'images/down.png':         'fa-solid fa-arrow-down',
        'images/remove.png':       'fa-solid fa-circle-minus',

        /* Table status / state indicators */
        'images/ok.png':           'fa-solid fa-circle-check',
        'images/failed.png':       'fa-solid fa-circle-xmark',
        'images/unknown.png':      'fa-solid fa-circle-question',
        'images/sleep.png':        'fa-solid fa-moon',
        'images/heal.png':         'fa-solid fa-heart-pulse',

        /* Table column header icons */
        'images/battery-ok.png':   'fa-solid fa-battery-full',
        'images/battery-low.png':  'fa-solid fa-battery-quarter',
        'images/battery.png':      'fa-solid fa-battery-half',
        'images/air_signal.png':   'fa-solid fa-signal',

        /* Report trend icons (g prefix is the gas variant) */
        'images/equal.png':        'fa-solid fa-minus',
        'images/gup.png':          'fa-solid fa-arrow-trend-up',
        'images/gdown.png':        'fa-solid fa-arrow-trend-down',
        'images/gequal.png':       'fa-solid fa-minus',

        /* Favourite stars. The solid/regular pair is the on/off state. */
        'images/favorite.png':     'fa-solid fa-star',
        'images/nofavorite.png':   'fa-regular fa-star'
    };

    /* ------------------------------------------------------------------
     * Device type icons: lower cased TypeImg -> Font Awesome class.
     *
     * TypeImg is the base name of the PNG that Domoticz would otherwise
     * render (see RFX_Type_Desc() and the TypeImg overrides in
     * GetJSonDevices), which is why keying on it needs no extra table.
     *
     * One glyph per type, not one per state: on/off is a colour
     * difference in the PNG set, and colour is out of scope here.
     * ------------------------------------------------------------------ */
    var TYPE_ICONS = {
        /* Lights and dimmers */
        'light':               'fa-solid fa-lightbulb',
        'dimmer':              'fa-solid fa-circle-half-stroke',
        'glight':              'fa-solid fa-lightbulb',
        'strip':               'fa-solid fa-grip-lines',

        /* RGB / colour */
        'rgb':                 'fa-solid fa-palette',

        /* Switches and push buttons */
        'generic':             'fa-solid fa-toggle-on',
        'push':                'fa-solid fa-circle-dot',
        'onoff':               'fa-solid fa-power-off',
        'pushon':              'fa-solid fa-circle-dot',

        /* Contacts and doors */
        'contact':             'fa-solid fa-door-closed',
        'door':                'fa-solid fa-door-open',
        'window':              'fa-solid fa-window-maximize',

        /* Blinds and shades */
        'blinds':              'fa-solid fa-chevron-down',
        'blindsopen':          'fa-solid fa-chevron-up',

        /* Climate */
        'heating':             'fa-solid fa-fire',
        'cooling':             'fa-solid fa-snowflake',
        'radiator':            'fa-solid fa-fire-flame-curved',
        'fireplace':           'fa-solid fa-fire',
        'fan':                 'fa-solid fa-fan',
        'ac':                  'fa-solid fa-snowflake',
        'ehome':               'fa-solid fa-house-chimney',

        /* Water and irrigation */
        'water':               'fa-solid fa-droplet',
        'tap':                 'fa-solid fa-faucet',
        'irrigation':          'fa-solid fa-hand-holding-droplet',
        'pool':                'fa-solid fa-water-ladder',
        'pump':                'fa-solid fa-pump-soap',

        /* Energy and power */
        'solar':               'fa-solid fa-solar-panel',
        'pv':                  'fa-solid fa-solar-panel',
        'inverter':            'fa-solid fa-bolt',
        'charger':             'fa-solid fa-charging-station',
        'laadpaal':            'fa-solid fa-charging-station',
        'wallsocket':          'fa-solid fa-plug',
        'current':             'fa-solid fa-bolt',

        /* Media and entertainment */
        'tv':                  'fa-solid fa-tv',
        'media':               'fa-solid fa-play',
        'speaker':             'fa-solid fa-volume-high',
        'amplifier':           'fa-solid fa-volume-high',
        'logitechmediaserver': 'fa-solid fa-music',
        'remote':              'fa-solid fa-gamepad',

        /* Computing and phones */
        'computer':            'fa-solid fa-display',
        'computerpc':          'fa-solid fa-computer',
        'harddisk':            'fa-solid fa-hard-drive',
        'phone':               'fa-solid fa-phone',
        'printer':             'fa-solid fa-print',

        /* Security and alarms */
        'alarm':               'fa-solid fa-bell',
        'smoke':               'fa-solid fa-triangle-exclamation',
        'motion':              'fa-solid fa-person-running',
        'security':            'fa-solid fa-shield-halved',

        /* Appliances */
        'coffee':              'fa-solid fa-mug-hot',
        'washingmachine':      'fa-solid fa-shirt',
        'christmastree':       'fa-solid fa-tree',

        /* Sensors and meters */
        'temp':                'fa-solid fa-temperature-half',
        'humidity':            'fa-solid fa-droplet',
        'baro':                'fa-solid fa-gauge',
        'rain':                'fa-solid fa-cloud-showers-heavy',
        'wind':                'fa-solid fa-wind',
        'uv':                  'fa-solid fa-sun',
        'lux':                 'fa-solid fa-sun',
        'visibility':          'fa-solid fa-eye',
        'radiation':           'fa-solid fa-radiation',
        'gauge':               'fa-solid fa-gauge',
        'counter':             'fa-solid fa-hashtag',
        'percentage':          'fa-solid fa-percent',
        'scale':               'fa-solid fa-scale-balanced',
        'gas':                 'fa-solid fa-gas-pump',
        'leaf':                'fa-solid fa-leaf',
        'moisture':            'fa-solid fa-hand-holding-droplet',
        'soil':                'fa-solid fa-seedling',
        'air':                 'fa-solid fa-wind',
        'airmeasure':          'fa-solid fa-lungs',
        'sun':                 'fa-solid fa-sun',
        'victron':             'fa-solid fa-car-battery',

        /* Locks */
        'doorlock':            'fa-solid fa-lock',
        'doorlockcontact':     'fa-solid fa-lock',

        /* Energy meters */
        'smartmeter':          'fa-solid fa-bolt',
        'p1smartmeter':        'fa-solid fa-bolt',
        'electricityusage':    'fa-solid fa-bolt',

        /* Air quality */
        'airquality':          'fa-solid fa-smog',
        'pm25':                'fa-solid fa-smog',
        'co2':                 'fa-solid fa-cloud',
        'co':                  'fa-solid fa-cloud',

        /* Water leak / flood */
        'leaksensor':          'fa-solid fa-droplet',
        'flood':               'fa-solid fa-droplet',

        /* Curtains, distinct from roller blinds */
        'curtain':             'fa-solid fa-table-columns',

        /* Presence / PIR */
        'presence':            'fa-solid fa-circle-dot',
        'pir':                 'fa-solid fa-person-running',

        /* Misc */
        'text':                'fa-solid fa-align-left',
        'alert':               'fa-solid fa-circle-exclamation',
        'clock':               'fa-solid fa-clock',
        'mode':                'fa-solid fa-sliders',
        'doorbell':            'fa-solid fa-bell',
        'adjust':              'fa-solid fa-sliders',
        'custom':              'fa-solid fa-gear',

        /* Scenes and groups */
        'scene':               'fa-solid fa-layer-group',
        'group':               'fa-solid fa-layer-group'
    };

    /* TypeImg values that do not match a TYPE_ICONS key directly.
       The first block comes with the mapping tables; the second covers the
       TypeImg strings Domoticz itself emits (RFX_Type_Desc() column 2 and
       the overrides in GetJSonDevices), which use different words for the
       same concepts. */
    var TYPE_ALIASES = {
        'hum':           'humidity',
        'temphum':       'temp',
        'temphumbaroew': 'temp',
        'zwavemelding':  'alarm',
        'elec':          'electricityusage',

        'lightbulb':     'light',        /* every Lighting/Switch protocol */
        'temperature':   'temp',         /* Temp, Humidity, Baro, Thermostat, Radiator */
        'temp + rain':   'temp',
        'setpoint':      'temp',
        'bbq':           'temp',
        'evohome':       'heating',
        'weather':       'sun',
        'general':       'gauge',        /* pTypeGeneral catch-all */
        'utility':       'gauge',
        'siren':         'alarm',
        'pushoff':       'push',
        'override_mini': 'adjust'
    };

    /* A resolved class ends up in a class attribute, so keep it to the
       characters a CSS class list can contain. The server already validates
       what it stores, but an icon can also come from an older database or a
       third party API caller. */
    var SAFE_CLASS_RE = /^[A-Za-z0-9 _-]+$/;

    /* Bare file name -> class, so callers can pass any form of the path.
       Built once: every CHROME_ICONS key is 'images/<name>.png', so the base
       names are unique by construction. */
    var CHROME_ICONS_BY_NAME = {};
    Object.keys(CHROME_ICONS).forEach(function (key) {
        CHROME_ICONS_BY_NAME[key.substring(key.lastIndexOf('/') + 1)] = CHROME_ICONS[key];
    });

    app.factory('dzIconService', ['domoticzApi', 'dzDefaultSwitchIcons', function (domoticzApi, dzDefaultSwitchIcons) {

        /* idx -> Font Awesome class for the built-in icon set. null until the
           first successful load; {} once loaded so a failure is not retried on
           every single card. */
        var builtinFaClasses = null;
        var builtinRequest = null;

        return {
            resolve: resolve,
            resolveIconClass: resolveIconClass,
            chromeIconFor: chromeIconFor,
            typeIconFor: typeIconFor,
            preloadBuiltinIcons: loadBuiltinIcons
        };

        /* Resolve the icon for a device.
           Returns { kind: 'font', cls: '...' } or { kind: 'img', src: '...' }.

           Precedence, most specific first:
             1. device.Icon      - an explicit per-device choice, so it wins
                                   over anything derived from the device type.
             2. the built-in icon set's Font Awesome class for CustomImage
                                 - the user picked that icon in the device
                                   editor, which is more specific than the type.
                                   A CustomImage with no glyph goes straight to
                                   step 4, never to the type map: the chosen
                                   image outranks a type default.
             3. the type map     - a sensible default for the device type.
             4. the PNG the web UI has always rendered, so anything we have no
                glyph for looks exactly like it did before. */
        function resolve(device, isActive) {
            if (!device) {
                return { kind: 'img', src: 'images/unknown.png' };
            }

            var active = isDeviceActive(device, isActive);

            var iconClass = resolveIconClass(device.Icon, active);
            if (iconClass) {
                return { kind: 'font', cls: iconClass };
            }

            /* 0 means "no custom image", 100 and up are the ZIP-uploaded user
               images which have no glyph, so only 1..99 can carry a class. */
            var customImage = parseInt(device.CustomImage, 10);
            if (customImage > 0) {
                var builtinClass = customImage < 100 ? builtinFaClassFor(customImage) : null;
                if (builtinClass) {
                    return { kind: 'font', cls: builtinClass };
                }

                /* Someone picked this image in the device editor, so the type
                   map must not talk over it. Skip straight to the PNG: that is
                   an uploaded icon, or a built-in one from a switch_icons.txt
                   that carries no Font Awesome class. */
                return { kind: 'img', src: legacyImageFor(device, active) };
            }

            var typeClass = typeIconFor(device.TypeImg);
            if (typeClass) {
                return { kind: 'font', cls: typeClass };
            }

            return { kind: 'img', src: legacyImageFor(device, active) };
        }

        /* Turn a raw DeviceStatus.Icon payload into a class string.
           Shape: {"t":"fa","on":"fa-solid fa-lightbulb","off":"fa-regular fa-lightbulb"}
           where "off" is optional and falls back to "on".
           Returns null for anything empty, malformed or unsafe, which makes
           the caller fall through to the next precedence step. */
        function resolveIconClass(icon, isActive) {
            if (!icon) {
                return null;
            }

            var parsed = icon;
            if (typeof icon === 'string') {
                try {
                    parsed = JSON.parse(icon);
                } catch (e) {
                    return null;
                }
            }
            if (!parsed || typeof parsed !== 'object') {
                return null;
            }

            /* "t" names the provider: 'fa' for the bundled Font Awesome, or the
               class prefix of an installed icon library. Either way the stored
               class is used verbatim, because a library brings its own class
               names along with its stylesheet. */
            var cls = isActive ? parsed.on : (parsed.off || parsed.on);
            if (typeof cls !== 'string') {
                return null;
            }
            cls = cls.replace(/\s+/g, ' ').trim();

            return (cls && SAFE_CLASS_RE.test(cls)) ? cls : null;
        }

        /* Font Awesome class for one of the fixed UI images, or null.
           Accepts 'images/delete.png', 'delete.png' or './images/delete.png'. */
        function chromeIconFor(src) {
            if (!src || typeof src !== 'string') {
                return null;
            }

            var path = src.split('?')[0].split('#')[0];
            if (CHROME_ICONS[path]) {
                return CHROME_ICONS[path];
            }

            return CHROME_ICONS_BY_NAME[path.substring(path.lastIndexOf('/') + 1)] || null;
        }

        /* Font Awesome class for a TypeImg value, or null when the type has no
           glyph yet and the PNG should be kept. */
        function typeIconFor(typeImg) {
            if (!typeImg || typeof typeImg !== 'string') {
                return null;
            }

            var key = typeImg.toLowerCase();
            return TYPE_ICONS[key] || TYPE_ICONS[TYPE_ALIASES[key]] || null;
        }

        /* Fetch the built-in icon set once and keep the Font Awesome classes.
           The same list already backs the icon picker in the device editor. */
        function loadBuiltinIcons() {
            if (builtinRequest) {
                return builtinRequest;
            }

            builtinRequest = domoticzApi.sendCommand('custom_light_icons', {})
                .then(function (data) {
                    var classes = {};
                    (data.result || []).forEach(function (item) {
                        if (item && item.FaClass) {
                            classes[item.idx] = item.FaClass;
                        }
                    });
                    builtinFaClasses = classes;
                    return classes;
                })
                .catch(function () {
                    /* An older server has no FaClass field, and the request can
                       simply fail. Either way, remember an empty set so every
                       device falls through to the type map instead of queueing
                       another request per card. */
                    builtinFaClasses = {};
                    return builtinFaClasses;
                });

            return builtinRequest;
        }

        function builtinFaClassFor(customImage) {
            if (builtinFaClasses === null) {
                /* resolve() is synchronous because it is called from templates.
                   Start the fetch and skip this step for now: the next digest
                   after it lands picks the glyph up. */
                loadBuiltinIcons();
                return null;
            }

            return builtinFaClasses[customImage] || null;
        }

        /* Mirrors deviceFactory's DeviceIcon.isConfigurable(). */
        function isConfigurable(device) {
            return ['Light/Switch', 'Lighting 1', 'Lighting 2', 'Lighting 5', 'Lighting 6', 'Color Switch', 'Home Confort', 'Thermostat 3'].indexOf(device.Type) !== -1 &&
                [0, 2, 7, 9, 10, 11, 17, 18, 19, 20].indexOf(device.SwitchTypeVal) !== -1;
        }

        /* Mirrors deviceFactory's Device.isActive(), for callers that hand us a
           plain device object from the JSON API rather than a Device instance. */
        function isDeviceActive(device, isActive) {
            if (isActive !== undefined && isActive !== null) {
                return !!isActive;
            }
            if (typeof device.isActive === 'function') {
                return !!device.isActive();
            }

            var status = device.Status;
            return !!status && (
                ['On', 'Chime', 'Group On', 'Panic', 'Mixed'].indexOf(status) !== -1
                || status.indexOf('Set ') === 0
                || status.indexOf('NightMode') === 0
                || status.indexOf('Disco ') === 0);
        }

        /* The PNG the web UI rendered before this service existed. Kept as a
           faithful copy of deviceFactory's DeviceIcon.getIcon(), including its
           mix of === and == on CustomImage, so the fallback cannot regress. */
        function legacyImageFor(device, isActive) {
            var image;
            var typeImg = device.TypeImg || '';

            if (isConfigurable(device)) {
                if (device.CustomImage === 0) {
                    /* isConfigurable() already limits SwitchTypeVal to the keys
                       of dzDefaultSwitchIcons; the guard is only there so an
                       unexpected value cannot throw inside a template. */
                    var defaults = dzDefaultSwitchIcons[device.SwitchTypeVal] || dzDefaultSwitchIcons[0];
                    image = defaults[isActive ? 0 : 1];
                } else {
                    image = device.Image + '48_' + (isActive ? 'On' : 'Off') + '.png';
                }
            } else if (typeImg.indexOf('Alert') === 0) {
                image = 'Alert48_' + Math.min(device.Level, 4) + '.png';
            } else if (typeImg.indexOf('motion') === 0) {
                image = isActive ? 'motion.png' : 'motionoff.png';
            } else if (typeImg.indexOf('smoke') === 0) {
                image = isActive ? 'smoke.png' : 'smokeoff.png';
            } else if (device.Type === 'Scene' || device.Type === 'Group') {
                image = isActive ? 'push.png' : 'pushoff.png';
            } else if (device.CustomImage == 0) {
                image = typeImg + '.png';
            } else {
                image = device.Image + '48_On.png';
            }

            return 'images/' + image;
        }
    }]);
});
